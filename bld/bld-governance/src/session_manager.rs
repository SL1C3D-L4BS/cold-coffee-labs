//! Multi-session bookkeeping (ADR-0015 §2.5).
//!
//! A BLD session corresponds to one agent-panel tab in the editor. Each
//! session has:
//!
//! * its own [`Governance`] (independent approval cache + grants);
//! * its own [`AuditLog`] on disk (`<logs>/<session_id>.ndjson`);
//! * its own [`SessionContext`] (model/offline state, slash-command-tweakable);
//! * a keep-alive heartbeat so the editor can reap idle sessions.
//!
//! This module does NOT own the LLM provider — that's application-level
//! glue in `bld-agent`. It does own the *indexing* and *lifecycle*
//! metadata so the agent panel (C++) can enumerate and dispose of
//! sessions cleanly.
//!
//! Thread-safety: the manager is `Send + Sync`. Per-session state is
//! fetched under a short-lived lock; long-running work should clone the
//! handle it needs and drop the lock before doing I/O.

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use crate::audit::AuditLog;
use crate::elicitation::{ElicitationHandler, StaticHandler};
use crate::slash::SessionContext;
use crate::tier::Governance;

/// Per-session handle. Wrap in an `Arc` to share.
pub struct Session {
    /// Stable session id (ULID).
    pub id: String,
    /// Display name shown in the UI.
    pub display_name: String,
    /// Governance layer (lock briefly).
    pub governance: Mutex<Governance>,
    /// Audit log on disk (lock briefly).
    pub audit: Mutex<AuditLog>,
    /// Mutable session context (model, offline flag).
    pub context: Mutex<SessionContext>,
    /// Heartbeat — used by keep-alive + reaper.
    pub heartbeat: Mutex<Instant>,
    /// Path to the audit log on disk.
    pub audit_path: PathBuf,
}

impl std::fmt::Debug for Session {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Session")
            .field("id", &self.id)
            .field("display_name", &self.display_name)
            .field("audit_path", &self.audit_path)
            .finish()
    }
}

impl Session {
    /// Update the heartbeat (sets it to now).
    pub fn touch(&self) {
        #[allow(clippy::expect_used)]
        let mut hb = self.heartbeat.lock().expect("session heartbeat poisoned");
        *hb = Instant::now();
    }

    /// Get the age (since last heartbeat).
    pub fn age(&self) -> Duration {
        #[allow(clippy::expect_used)]
        let hb = self.heartbeat.lock().expect("session heartbeat poisoned");
        hb.elapsed()
    }
}

/// Factory for elicitation handlers. Each session gets its own handler
/// so panels can elicit independently.
pub trait ElicitationFactory: Send + Sync {
    /// Build a handler for a fresh session.
    fn build(&self, session_id: &str) -> Box<dyn ElicitationHandler + Send>;
}

/// Factory that hands out [`StaticHandler`]s — useful for tests and the
/// bootstrap path where no panel has registered yet.
pub struct StaticElicitationFactory {
    approve: bool,
}

impl StaticElicitationFactory {
    /// New factory that always approves.
    pub fn always_approve() -> Self {
        Self { approve: true }
    }
    /// New factory that always denies.
    pub fn always_deny() -> Self {
        Self { approve: false }
    }
}

impl ElicitationFactory for StaticElicitationFactory {
    fn build(&self, _session_id: &str) -> Box<dyn ElicitationHandler + Send> {
        if self.approve {
            Box::new(StaticHandler::always_approve())
        } else {
            Box::new(StaticHandler::always_deny())
        }
    }
}

/// Multi-session registry.
pub struct SessionManager {
    root: PathBuf,
    factory: Arc<dyn ElicitationFactory>,
    sessions: Mutex<HashMap<String, Arc<Session>>>,
    /// Reaper threshold. A session whose heartbeat age exceeds this
    /// value is considered idle.
    pub idle_ttl: Duration,
}

impl std::fmt::Debug for SessionManager {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("SessionManager")
            .field("root", &self.root)
            .field("idle_ttl", &self.idle_ttl)
            .finish()
    }
}

impl SessionManager {
    /// New manager rooted at `root` (audit logs live under
    /// `<root>/<session_id>/`). Idle TTL defaults to 30 min.
    pub fn new(root: impl AsRef<Path>, factory: Arc<dyn ElicitationFactory>) -> Self {
        Self {
            root: root.as_ref().to_path_buf(),
            factory,
            sessions: Mutex::new(HashMap::new()),
            idle_ttl: Duration::from_secs(30 * 60),
        }
    }

    /// Override the idle TTL.
    pub fn with_idle_ttl(mut self, ttl: Duration) -> Self {
        self.idle_ttl = ttl;
        self
    }

    /// Create a new session and register it.
    pub fn create(&self, display_name: impl Into<String>) -> Result<Arc<Session>, String> {
        let display_name = display_name.into();
        let id = ulid::Ulid::new().to_string();
        let audit = AuditLog::open(&self.root, &id).map_err(|e| format!("audit open: {e}"))?;
        let audit_path = self.root.join(format!("{id}.ndjson"));
        let gov = Governance::new(self.factory.build(&id));

        let session = Arc::new(Session {
            id: id.clone(),
            display_name,
            governance: Mutex::new(gov),
            audit: Mutex::new(audit),
            context: Mutex::new(SessionContext::default()),
            heartbeat: Mutex::new(Instant::now()),
            audit_path,
        });

        #[allow(clippy::expect_used)]
        self.sessions
            .lock()
            .expect("session map poisoned")
            .insert(id, session.clone());
        Ok(session)
    }

    /// Look up a session by id.
    pub fn get(&self, id: &str) -> Option<Arc<Session>> {
        #[allow(clippy::expect_used)]
        self.sessions
            .lock()
            .expect("session map poisoned")
            .get(id)
            .cloned()
    }

    /// Drop a session (the [`AuditLog`] file is flushed by its `Drop`).
    pub fn destroy(&self, id: &str) -> bool {
        #[allow(clippy::expect_used)]
        self.sessions
            .lock()
            .expect("session map poisoned")
            .remove(id)
            .is_some()
    }

    /// Snapshot the session list.
    pub fn list(&self) -> Vec<SessionSummary> {
        #[allow(clippy::expect_used)]
        let map = self.sessions.lock().expect("session map poisoned");
        map.values()
            .map(|s| SessionSummary {
                id: s.id.clone(),
                display_name: s.display_name.clone(),
                age_ms: s.age().as_millis() as u64,
            })
            .collect()
    }

    /// Touch a session's heartbeat (the keep-alive primitive).
    pub fn keep_alive(&self, id: &str) -> bool {
        match self.get(id) {
            Some(s) => {
                s.touch();
                true
            }
            None => false,
        }
    }

    /// Reap idle sessions and return their ids.
    pub fn reap_idle(&self) -> Vec<String> {
        let mut reaped = Vec::new();
        #[allow(clippy::expect_used)]
        let mut map = self.sessions.lock().expect("session map poisoned");
        map.retain(|id, s| {
            if s.age() > self.idle_ttl {
                reaped.push(id.clone());
                false
            } else {
                true
            }
        });
        reaped
    }
}

/// Light-weight snapshot for `/sessions`.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct SessionSummary {
    /// Session id.
    pub id: String,
    /// Display name.
    pub display_name: String,
    /// Age of the last heartbeat in ms.
    pub age_ms: u64,
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_manager() -> (SessionManager, tempfile::TempDir) {
        let dir = tempfile::tempdir().unwrap();
        let mgr = SessionManager::new(dir.path(), Arc::new(StaticElicitationFactory::always_approve()));
        (mgr, dir)
    }

    #[test]
    fn create_returns_unique_ids() {
        let (mgr, _d) = make_manager();
        let a = mgr.create("Tab A").unwrap();
        let b = mgr.create("Tab B").unwrap();
        assert_ne!(a.id, b.id);
        assert_eq!(mgr.list().len(), 2);
    }

    #[test]
    fn get_by_id_roundtrips() {
        let (mgr, _d) = make_manager();
        let s = mgr.create("T").unwrap();
        let got = mgr.get(&s.id).unwrap();
        assert_eq!(got.id, s.id);
    }

    #[test]
    fn destroy_removes_from_list() {
        let (mgr, _d) = make_manager();
        let s = mgr.create("T").unwrap();
        assert!(mgr.destroy(&s.id));
        assert!(mgr.get(&s.id).is_none());
    }

    #[test]
    fn keep_alive_touches_heartbeat() {
        let (mgr, _d) = make_manager();
        let s = mgr.create("T").unwrap();
        std::thread::sleep(Duration::from_millis(5));
        let age_before = s.age();
        assert!(mgr.keep_alive(&s.id));
        let age_after = s.age();
        assert!(age_after < age_before);
    }

    #[test]
    fn reap_drops_stale_sessions() {
        let (mut mgr, _d) = make_manager();
        mgr.idle_ttl = Duration::from_millis(1);
        let s = mgr.create("T").unwrap();
        std::thread::sleep(Duration::from_millis(5));
        let reaped = mgr.reap_idle();
        assert!(reaped.contains(&s.id));
        assert!(mgr.get(&s.id).is_none());
    }

    #[test]
    fn governance_is_per_session() {
        let (mgr, _d) = make_manager();
        let a = mgr.create("A").unwrap();
        let b = mgr.create("B").unwrap();
        #[allow(clippy::expect_used)]
        {
            a.governance.lock().expect("gov").grant("scene.create_entity");
        }
        #[allow(clippy::expect_used)]
        let b_decision = b
            .governance
            .lock()
            .expect("gov")
            .require(
                "scene.create_entity",
                crate::Tier::Mutate,
                &serde_json::json!({}),
            )
            .unwrap();
        // B granted nothing yet; handler in factory is always-approve, so
        // Approved (not ApprovedGranted).
        assert_eq!(b_decision, crate::GovernanceDecision::Approved);
    }
}
