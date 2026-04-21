//! Hybrid router provider (ADR-0016 §2.3 — Wave 9F).
//!
//! `HybridRouter` wraps multiple concrete providers and routes each
//! request based on:
//!
//! 1. The [`OfflineFlag`] — when ON, only [`ProviderKind::Mistralrs`] /
//!    [`ProviderKind::Llama`] / [`ProviderKind::Mock`] are eligible.
//! 2. Per-provider [`ProviderHealth`] — `Unhealthy` and `Disabled`
//!    providers are skipped.
//! 3. A user-configured priority list (first match wins).
//!
//! On a successful call the router records the winning provider. On
//! failure it falls through to the next eligible provider in the
//! priority list. If every candidate fails the error from the last
//! attempt is surfaced.
//!
//! The router itself is `Send + Sync` and clone-free — each provider
//! arrives as an `Arc<dyn ILLMProvider>` so the agent can hold one
//! instance and cheaply share it across turns.

use std::sync::{atomic::AtomicBool, atomic::Ordering, Arc, Mutex};

use async_trait::async_trait;
use futures::stream::BoxStream;

use crate::{
    ChatRequest, ChatResponse, ILLMProvider, ProviderError, ProviderHealth, ProviderKind,
    StreamEvent,
};

/// Shared offline-mode flag. Flip at runtime via `/offline` slash command.
#[derive(Debug, Default)]
pub struct OfflineFlag {
    inner: AtomicBool,
}

impl OfflineFlag {
    /// New flag, default OFF.
    pub const fn new() -> Self {
        Self {
            inner: AtomicBool::new(false),
        }
    }

    /// Is offline mode ON?
    pub fn is_on(&self) -> bool {
        self.inner.load(Ordering::Acquire)
    }

    /// Set offline mode.
    pub fn set(&self, on: bool) {
        self.inner.store(on, Ordering::Release);
    }
}

/// Does this provider kind require network connectivity?
pub fn requires_network(kind: ProviderKind) -> bool {
    matches!(kind, ProviderKind::Claude)
}

/// Metadata kept per provider inside the router.
struct Entry {
    provider: Arc<dyn ILLMProvider>,
    /// Rolling failure count. Reset on success. Used only for routing
    /// heuristics; `ILLMProvider::health()` remains authoritative.
    failures: Mutex<u32>,
}

/// The router.
pub struct HybridRouter {
    entries: Vec<Entry>,
    offline: Arc<OfflineFlag>,
    max_failures: u32,
    last_used: Mutex<Option<ProviderKind>>,
}

impl std::fmt::Debug for HybridRouter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let kinds: Vec<ProviderKind> = self.entries.iter().map(|e| e.provider.kind()).collect();
        f.debug_struct("HybridRouter")
            .field("providers", &kinds)
            .field("offline", &self.offline.is_on())
            .field("max_failures", &self.max_failures)
            .finish()
    }
}

impl HybridRouter {
    /// New router from a priority-ordered list of providers.
    pub fn new(providers: Vec<Arc<dyn ILLMProvider>>, offline: Arc<OfflineFlag>) -> Self {
        Self {
            entries: providers
                .into_iter()
                .map(|p| Entry {
                    provider: p,
                    failures: Mutex::new(0),
                })
                .collect(),
            offline,
            max_failures: 3,
            last_used: Mutex::new(None),
        }
    }

    /// Override the transient-failure threshold (default 3).
    pub fn with_max_failures(mut self, n: u32) -> Self {
        self.max_failures = n;
        self
    }

    /// Inspect which provider served the most recent call.
    pub fn last_used(&self) -> Option<ProviderKind> {
        #[allow(clippy::expect_used)]
        *self.last_used.lock().expect("last_used mutex poisoned")
    }

    /// Return an ordered list of eligible providers given the current
    /// offline flag + health.
    pub fn eligible_kinds(&self) -> Vec<ProviderKind> {
        self.entries
            .iter()
            .filter(|e| self.is_eligible(e))
            .map(|e| e.provider.kind())
            .collect()
    }

    fn is_eligible(&self, e: &Entry) -> bool {
        if matches!(e.provider.health(), ProviderHealth::Disabled) {
            return false;
        }
        if self.offline.is_on() && requires_network(e.provider.kind()) {
            return false;
        }
        let failures = {
            #[allow(clippy::expect_used)]
            *e.failures.lock().expect("failures mutex poisoned")
        };
        if failures >= self.max_failures
            && matches!(e.provider.health(), ProviderHealth::Unhealthy)
        {
            return false;
        }
        true
    }

    fn record_success(&self, kind: ProviderKind, entry: &Entry) {
        #[allow(clippy::expect_used)]
        {
            *entry.failures.lock().expect("failures mutex poisoned") = 0;
            *self.last_used.lock().expect("last_used mutex poisoned") = Some(kind);
        }
    }

    fn record_failure(&self, entry: &Entry) {
        #[allow(clippy::expect_used)]
        let mut f = entry.failures.lock().expect("failures mutex poisoned");
        *f = f.saturating_add(1);
    }
}

#[async_trait]
impl ILLMProvider for HybridRouter {
    fn kind(&self) -> ProviderKind {
        // The router doesn't have a single kind; return the first
        // eligible one, else Mock as a neutral default.
        self.eligible_kinds().into_iter().next().unwrap_or(ProviderKind::Mock)
    }

    fn health(&self) -> ProviderHealth {
        if self.eligible_kinds().is_empty() {
            ProviderHealth::Disabled
        } else {
            ProviderHealth::Healthy
        }
    }

    fn models(&self) -> Vec<&'static str> {
        let mut out = Vec::new();
        for e in &self.entries {
            if self.is_eligible(e) {
                out.extend(e.provider.models());
            }
        }
        out.dedup();
        out
    }

    async fn complete(&self, req: ChatRequest) -> Result<ChatResponse, ProviderError> {
        let mut last: Option<ProviderError> = None;
        for e in &self.entries {
            if !self.is_eligible(e) {
                continue;
            }
            let kind = e.provider.kind();
            match e.provider.complete(req.clone()).await {
                Ok(resp) => {
                    self.record_success(kind, e);
                    return Ok(resp);
                }
                Err(err) => {
                    self.record_failure(e);
                    last = Some(err);
                }
            }
        }
        Err(last.unwrap_or_else(|| {
            ProviderError::Other(if self.offline.is_on() {
                "hybrid router: no eligible offline providers".into()
            } else {
                "hybrid router: no eligible providers".into()
            })
        }))
    }

    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError> {
        // Streaming fallback is strictly first-match: we don't mid-stream
        // re-route. If the chosen provider fails to initiate, we try the
        // next; once a stream has started, errors are passed through.
        let mut last: Option<ProviderError> = None;
        for e in &self.entries {
            if !self.is_eligible(e) {
                continue;
            }
            let kind = e.provider.kind();
            match e.provider.complete_stream(req.clone()).await {
                Ok(s) => {
                    self.record_success(kind, e);
                    return Ok(s);
                }
                Err(err) => {
                    self.record_failure(e);
                    last = Some(err);
                }
            }
        }
        Err(last.unwrap_or_else(|| {
            ProviderError::Other("hybrid router: no eligible providers for stream".into())
        }))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::mock::{MockProvider, ScriptStep};
    use crate::{ChatMessage, ChatResponse, Role};

    fn req() -> ChatRequest {
        ChatRequest {
            model: "mock-default".into(),
            system: None,
            messages: vec![ChatMessage {
                role: Role::User,
                content: "hi".into(),
                tool_call_id: None,
                tool_name: None,
            }],
            tools: vec![],
            temperature: Some(0.0),
            max_tokens: Some(16),
            stream: false,
            structured_output: None,
        }
    }

    fn reply(text: &str) -> ChatResponse {
        ChatResponse {
            model: "mock".into(),
            content: text.into(),
            tool_calls: vec![],
            stop_reason: Some("end_turn".into()),
            input_tokens: Some(1),
            output_tokens: Some(1),
        }
    }

    #[tokio::test]
    async fn first_healthy_wins() {
        let a = MockProvider::new().with_kind(ProviderKind::Claude);
        a.push(ScriptStep::Reply(reply("from-claude")));
        let b = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        b.push(ScriptStep::Reply(reply("from-mistral")));

        let router = HybridRouter::new(
            vec![Arc::new(a), Arc::new(b)],
            Arc::new(OfflineFlag::new()),
        );
        let r = router.complete(req()).await.unwrap();
        assert_eq!(r.content, "from-claude");
        assert_eq!(router.last_used(), Some(ProviderKind::Claude));
    }

    #[tokio::test]
    async fn falls_through_on_error() {
        let a = MockProvider::new().with_kind(ProviderKind::Claude);
        a.push(ScriptStep::Error(ProviderError::Retryable("rate".into())));
        let b = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        b.push(ScriptStep::Reply(reply("from-mistral")));

        let router = HybridRouter::new(
            vec![Arc::new(a), Arc::new(b)],
            Arc::new(OfflineFlag::new()),
        );
        let r = router.complete(req()).await.unwrap();
        assert_eq!(r.content, "from-mistral");
        assert_eq!(router.last_used(), Some(ProviderKind::Mistralrs));
    }

    #[tokio::test]
    async fn offline_mode_skips_cloud() {
        let a = MockProvider::new().with_kind(ProviderKind::Claude);
        a.push(ScriptStep::Reply(reply("cloud")));
        let b = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        b.push(ScriptStep::Reply(reply("local")));

        let flag = Arc::new(OfflineFlag::new());
        flag.set(true);
        let router = HybridRouter::new(vec![Arc::new(a), Arc::new(b)], flag);
        let r = router.complete(req()).await.unwrap();
        assert_eq!(r.content, "local");
        assert_eq!(router.last_used(), Some(ProviderKind::Mistralrs));
    }

    #[tokio::test]
    async fn disabled_provider_skipped() {
        let disabled = MockProvider::new()
            .with_kind(ProviderKind::Claude)
            .with_health(ProviderHealth::Disabled);
        disabled.push(ScriptStep::Reply(reply("should not run")));
        let ok = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        ok.push(ScriptStep::Reply(reply("local")));

        let router = HybridRouter::new(
            vec![Arc::new(disabled), Arc::new(ok)],
            Arc::new(OfflineFlag::new()),
        );
        let r = router.complete(req()).await.unwrap();
        assert_eq!(r.content, "local");
    }

    #[tokio::test]
    async fn empty_eligible_surfaces_error() {
        let disabled = MockProvider::new()
            .with_kind(ProviderKind::Claude)
            .with_health(ProviderHealth::Disabled);
        let flag = Arc::new(OfflineFlag::new());
        flag.set(true);
        let router = HybridRouter::new(vec![Arc::new(disabled)], flag);
        let e = router.complete(req()).await.unwrap_err();
        match e {
            ProviderError::Other(m) => assert!(m.contains("no eligible")),
            _ => panic!("expected Other"),
        }
    }

    #[tokio::test]
    async fn offline_flag_toggles_live() {
        let flag = Arc::new(OfflineFlag::new());
        flag.set(true);
        assert!(flag.is_on());
        flag.set(false);
        assert!(!flag.is_on());
    }

    #[test]
    fn eligible_kinds_respects_offline_and_disabled() {
        let claude = MockProvider::new().with_kind(ProviderKind::Claude);
        let mistral = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        let flag = Arc::new(OfflineFlag::new());
        let router = HybridRouter::new(
            vec![Arc::new(claude), Arc::new(mistral)],
            flag.clone(),
        );
        assert_eq!(
            router.eligible_kinds(),
            vec![ProviderKind::Claude, ProviderKind::Mistralrs]
        );
        flag.set(true);
        assert_eq!(router.eligible_kinds(), vec![ProviderKind::Mistralrs]);
    }

    #[tokio::test]
    async fn stream_falls_through_like_complete() {
        let a = MockProvider::new().with_kind(ProviderKind::Claude);
        a.push(ScriptStep::Error(ProviderError::Retryable("rate".into())));
        let b = MockProvider::new().with_kind(ProviderKind::Mistralrs);
        b.push(ScriptStep::Reply(reply("ok")));

        let router = HybridRouter::new(
            vec![Arc::new(a), Arc::new(b)],
            Arc::new(OfflineFlag::new()),
        );
        let _stream = router.complete_stream(req()).await.unwrap();
        assert_eq!(router.last_used(), Some(ProviderKind::Mistralrs));
    }
}
