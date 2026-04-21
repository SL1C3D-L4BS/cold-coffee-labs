//! Three-tier authorization policy (ADR-0015 §2.1).
//!
//! The policy is deliberately small:
//!
//! - `Read` — always allowed, no elicitation, no cache.
//! - `Mutate` — elicitation required on first call; per-(tool_id, args-shape)
//!   cache with a configurable expiry (default 1 h).
//! - `Execute` — elicitation every call, no cache.
//!
//! The `Governance` struct is the decision engine. It is deterministic
//! given a clock and an `ElicitationHandler`; this makes it
//! property-testable and suitable for the Phase 24 mutation-testing gate.

use std::collections::HashMap;
use std::time::{Duration, Instant, SystemTime};

use serde::{Deserialize, Serialize};

use crate::elicitation::{ElicitationHandler, ElicitationMode, ElicitationRequest};

/// Authorization tier attached to every `#[bld_tool]` at compile time.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Tier {
    /// Read-only — always allowed, no elicitation, no cache.
    Read,
    /// Non-destructive mutation — cached elicitation per session.
    Mutate,
    /// Destructive / side-effectful — elicitation every call.
    Execute,
}

impl std::fmt::Display for Tier {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Tier::Read => f.write_str("Read"),
            Tier::Mutate => f.write_str("Mutate"),
            Tier::Execute => f.write_str("Execute"),
        }
    }
}

/// Governance error type.
#[derive(Debug, thiserror::Error)]
pub enum GovernanceError {
    /// A grant or revocation referred to a tool not in the registry.
    #[error("unknown tool: {0}")]
    UnknownTool(String),
    /// Internal logic error (should not fire in practice).
    #[error("internal: {0}")]
    Internal(&'static str),
}

/// Whether the tool is allowed to run.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum GovernanceDecision {
    /// First-time approval from the user.
    Approved,
    /// Approved from the per-session cache.
    ApprovedCached,
    /// Always-approved by explicit user grant (slash `/grant`).
    ApprovedGranted,
    /// Read tier — never asked.
    Implicit,
    /// User denied the elicitation.
    Denied,
}

impl GovernanceDecision {
    /// Whether this decision allows the tool to run.
    pub fn is_allowed(&self) -> bool {
        !matches!(self, GovernanceDecision::Denied)
    }

    /// Stable short string for audit logs.
    pub fn as_audit_str(&self) -> &'static str {
        match self {
            GovernanceDecision::Approved => "Approved",
            GovernanceDecision::ApprovedCached => "ApprovedCached",
            GovernanceDecision::ApprovedGranted => "ApprovedGranted",
            GovernanceDecision::Implicit => "Implicit",
            GovernanceDecision::Denied => "Denied",
        }
    }
}

/// Approval cache — per-session.
///
/// Key shape: `(tool_id, args_shape_fingerprint)`. The fingerprint is a
/// structural hash of the JSON args, so `scene.create_entity("Cube")` and
/// `scene.create_entity("Sphere")` share a cache entry (both just differ
/// in a scalar string), while `component.set(e, {hp: 5})` and
/// `component.set(e, {position: [1,2,3]})` do not.
#[derive(Debug, Default)]
pub struct ApprovalCache {
    entries: HashMap<(String, u64), Instant>,
    grants: std::collections::HashSet<String>,
}

impl ApprovalCache {
    /// New empty cache.
    pub fn new() -> Self {
        Self::default()
    }

    /// Check if a tool/args-shape is cached and not expired.
    pub fn check(&self, tool_id: &str, shape: u64, ttl: Duration) -> bool {
        match self.entries.get(&(tool_id.to_owned(), shape)) {
            Some(when) => when.elapsed() < ttl,
            None => false,
        }
    }

    /// Record approval.
    pub fn record(&mut self, tool_id: &str, shape: u64) {
        self.entries.insert((tool_id.to_owned(), shape), Instant::now());
    }

    /// `/grant <tool>` — always-approve within session (and optionally persist).
    pub fn grant(&mut self, tool_id: &str) {
        self.grants.insert(tool_id.to_owned());
    }

    /// `/revoke <tool>` — remove a session grant.
    pub fn revoke(&mut self, tool_id: &str) {
        self.grants.remove(tool_id);
    }

    /// Check for an always-approve grant.
    pub fn is_granted(&self, tool_id: &str) -> bool {
        self.grants.contains(tool_id)
    }

    /// Forget all cached approvals (e.g. on session close).
    pub fn clear(&mut self) {
        self.entries.clear();
        self.grants.clear();
    }
}

/// Structural fingerprint of a JSON args value.
///
/// Walks the value recursively, hashing its *shape* (which keys exist,
/// which types, nested structure) but not scalar values (beyond type-tag).
/// This gives us "same-shape args" as the cache key per ADR-0015 §2.1.
pub fn args_shape(v: &serde_json::Value) -> u64 {
    let mut hasher = blake3::Hasher::new();
    fn walk(v: &serde_json::Value, h: &mut blake3::Hasher) {
        use serde_json::Value;
        match v {
            Value::Null => h.update(b"N"),
            Value::Bool(_) => h.update(b"B"),
            Value::Number(_) => h.update(b"I"),
            Value::String(_) => h.update(b"S"),
            Value::Array(a) => {
                h.update(b"A");
                // We only use the element type of the first item for shape
                // purposes; homogeneous arrays fingerprint identically.
                if let Some(first) = a.first() {
                    walk(first, h);
                }
                h
            }
            Value::Object(o) => {
                h.update(b"O");
                let mut keys: Vec<&String> = o.keys().collect();
                keys.sort();
                for k in keys {
                    h.update(k.as_bytes());
                    h.update(b":");
                    #[allow(clippy::indexing_slicing)]
                    let v = &o[k];
                    walk(v, h);
                }
                h
            }
        };
    }
    walk(v, &mut hasher);
    let digest = hasher.finalize();
    // Fold 32 bytes to u64.
    let bytes = digest.as_bytes();
    u64::from_le_bytes([
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
    ])
}

/// The governance decision engine.
///
/// `Governance` is cheap to construct and holds per-session state. A new
/// session instantiates a fresh `Governance` with a fresh `ApprovalCache`.
pub struct Governance {
    cache: ApprovalCache,
    ttl: Duration,
    handler: Box<dyn ElicitationHandler + Send>,
}

impl Governance {
    /// Build a governance with a custom elicitation handler and default TTL.
    pub fn new(handler: Box<dyn ElicitationHandler + Send>) -> Self {
        Self {
            cache: ApprovalCache::new(),
            ttl: Duration::from_secs(60 * 60),
            handler,
        }
    }

    /// Override the approval TTL (default 1 hour).
    pub fn with_ttl(mut self, ttl: Duration) -> Self {
        self.ttl = ttl;
        self
    }

    /// Enforce policy on a prospective tool invocation.
    pub fn require(
        &mut self,
        tool_id: &str,
        tier: Tier,
        args: &serde_json::Value,
    ) -> Result<GovernanceDecision, GovernanceError> {
        match tier {
            Tier::Read => Ok(GovernanceDecision::Implicit),
            Tier::Mutate => self.require_mutate(tool_id, args),
            Tier::Execute => self.require_execute(tool_id, args),
        }
    }

    fn require_mutate(
        &mut self,
        tool_id: &str,
        args: &serde_json::Value,
    ) -> Result<GovernanceDecision, GovernanceError> {
        if self.cache.is_granted(tool_id) {
            return Ok(GovernanceDecision::ApprovedGranted);
        }
        let shape = args_shape(args);
        if self.cache.check(tool_id, shape, self.ttl) {
            return Ok(GovernanceDecision::ApprovedCached);
        }
        let req = ElicitationRequest::confirm(
            tool_id,
            Tier::Mutate,
            args.clone(),
            ElicitationMode::Form,
        );
        let resp = self.handler.elicit(&req).map_err(|_| {
            GovernanceError::Internal("elicitation handler returned an error")
        })?;
        if resp.approved {
            self.cache.record(tool_id, shape);
            Ok(GovernanceDecision::Approved)
        } else {
            Ok(GovernanceDecision::Denied)
        }
    }

    fn require_execute(
        &mut self,
        tool_id: &str,
        args: &serde_json::Value,
    ) -> Result<GovernanceDecision, GovernanceError> {
        // Execute is never cached per ADR-0015 §2.1. Grants also do NOT
        // apply to Execute — the policy is deliberate.
        let req = ElicitationRequest::confirm(
            tool_id,
            Tier::Execute,
            args.clone(),
            ElicitationMode::Form,
        );
        let resp = self.handler.elicit(&req).map_err(|_| {
            GovernanceError::Internal("elicitation handler returned an error")
        })?;
        if resp.approved {
            Ok(GovernanceDecision::Approved)
        } else {
            Ok(GovernanceDecision::Denied)
        }
    }

    /// Slash `/grant <tool>` — always-approve this tool in this session.
    pub fn grant(&mut self, tool_id: &str) {
        self.cache.grant(tool_id);
    }

    /// Slash `/revoke <tool>`.
    pub fn revoke(&mut self, tool_id: &str) {
        self.cache.revoke(tool_id);
    }

    /// Clear all caches and grants (end of session).
    pub fn reset(&mut self) {
        self.cache.clear();
    }

    /// Current wall time — used by audit records. Exposed for test determinism.
    pub fn now_utc() -> SystemTime {
        SystemTime::now()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::elicitation::{ElicitError, StaticHandler};
    use serde_json::json;

    #[test]
    fn read_tier_is_implicit() {
        let h = StaticHandler::always_approve();
        let mut g = Governance::new(Box::new(h));
        let d = g
            .require("docs.search", Tier::Read, &json!({"q": "hi"}))
            .expect("ok");
        assert_eq!(d, GovernanceDecision::Implicit);
    }

    #[test]
    fn mutate_first_time_asks_then_caches() {
        let h = StaticHandler::always_approve();
        let mut g = Governance::new(Box::new(h));
        let args = json!({"name": "Cube"});
        let d1 = g.require("scene.create_entity", Tier::Mutate, &args).unwrap();
        assert_eq!(d1, GovernanceDecision::Approved);
        let d2 = g.require("scene.create_entity", Tier::Mutate, &args).unwrap();
        assert_eq!(d2, GovernanceDecision::ApprovedCached);
    }

    #[test]
    fn mutate_same_shape_different_scalar_caches() {
        let h = StaticHandler::always_approve();
        let mut g = Governance::new(Box::new(h));
        let a = json!({"name": "Cube"});
        let b = json!({"name": "Sphere"});
        g.require("scene.create_entity", Tier::Mutate, &a).unwrap();
        let d = g.require("scene.create_entity", Tier::Mutate, &b).unwrap();
        assert_eq!(d, GovernanceDecision::ApprovedCached);
    }

    #[test]
    fn mutate_different_shape_reprompts() {
        let h = StaticHandler::always_approve();
        let mut g = Governance::new(Box::new(h));
        let a = json!({"name": "Cube"});
        let b = json!({"name": "Cube", "parent": 3});
        g.require("scene.create_entity", Tier::Mutate, &a).unwrap();
        let d = g.require("scene.create_entity", Tier::Mutate, &b).unwrap();
        assert_eq!(d, GovernanceDecision::Approved);
    }

    #[test]
    fn execute_never_caches() {
        let h = StaticHandler::always_approve();
        let mut g = Governance::new(Box::new(h));
        let args = json!({"target": "gameplay_module"});
        g.require("build.build_target", Tier::Execute, &args).unwrap();
        let d = g.require("build.build_target", Tier::Execute, &args).unwrap();
        assert_eq!(d, GovernanceDecision::Approved);
    }

    #[test]
    fn grant_always_approves_mutate() {
        let h = StaticHandler::always_deny();
        let mut g = Governance::new(Box::new(h));
        g.grant("scene.create_entity");
        let d = g
            .require("scene.create_entity", Tier::Mutate, &json!({}))
            .unwrap();
        assert_eq!(d, GovernanceDecision::ApprovedGranted);
    }

    #[test]
    fn grant_does_not_override_execute() {
        let h = StaticHandler::always_deny();
        let mut g = Governance::new(Box::new(h));
        g.grant("build.build_target");
        let d = g
            .require("build.build_target", Tier::Execute, &json!({}))
            .unwrap();
        assert_eq!(d, GovernanceDecision::Denied);
    }

    #[test]
    fn deny_blocks() {
        let h = StaticHandler::always_deny();
        let mut g = Governance::new(Box::new(h));
        let d = g
            .require("scene.destroy_entity", Tier::Mutate, &json!({"e": 1}))
            .unwrap();
        assert!(!d.is_allowed());
    }

    #[test]
    fn args_shape_is_scalar_insensitive() {
        let a = args_shape(&json!({"name": "Cube", "pos": [0, 0, 0]}));
        let b = args_shape(&json!({"name": "Sphere", "pos": [5, 1, 2]}));
        assert_eq!(a, b);
    }

    #[test]
    fn args_shape_is_key_sensitive() {
        let a = args_shape(&json!({"name": "Cube"}));
        let b = args_shape(&json!({"label": "Cube"}));
        assert_ne!(a, b);
    }

    #[test]
    fn ttl_expiry_reprompts() {
        let h = StaticHandler::always_approve();
        let mut g =
            Governance::new(Box::new(h)).with_ttl(std::time::Duration::from_millis(0));
        let args = json!({"n": 1});
        g.require("scene.create_entity", Tier::Mutate, &args).unwrap();
        // TTL=0 means every call looks expired.
        std::thread::sleep(std::time::Duration::from_millis(1));
        let d = g.require("scene.create_entity", Tier::Mutate, &args).unwrap();
        assert_eq!(d, GovernanceDecision::Approved);
    }

    #[test]
    #[allow(unused_variables)]
    fn error_propagation_from_handler() {
        use crate::elicitation::ElicitationResponse;
        struct Broken;
        impl crate::elicitation::ElicitationHandler for Broken {
            fn elicit(&mut self, _r: &ElicitationRequest) -> Result<ElicitationResponse, ElicitError> {
                Err(ElicitError::NoUiAttached)
            }
        }
        let mut g = Governance::new(Box::new(Broken));
        let r = g.require("scene.create_entity", Tier::Mutate, &json!({}));
        assert!(r.is_err());
    }
}
