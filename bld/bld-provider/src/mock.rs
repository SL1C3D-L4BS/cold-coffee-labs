//! Deterministic mock provider for tests and offline smoke.
//!
//! `MockProvider` plays back a canned script of responses. Every call
//! advances the script. If the script is empty the provider errors. This
//! gives agent-level tests full control over "what the model said"
//! without network or local inference setup.

use std::sync::Mutex;

use async_trait::async_trait;
use futures::stream::{self, BoxStream};
use futures::StreamExt;

use crate::{
    ChatRequest, ChatResponse, ILLMProvider, ProviderError, ProviderHealth, ProviderKind,
    StreamEvent,
};

/// A single scripted reply.
#[derive(Debug, Clone)]
pub enum ScriptStep {
    /// Return a full chat response.
    Reply(ChatResponse),
    /// Return an error.
    Error(ProviderError),
}

/// Deterministic mock provider.
pub struct MockProvider {
    kind: ProviderKind,
    script: Mutex<std::collections::VecDeque<ScriptStep>>,
    health: ProviderHealth,
    models: Vec<&'static str>,
}

impl MockProvider {
    /// Build with a custom kind tag. Default = `ProviderKind::Mock`.
    pub fn new() -> Self {
        Self {
            kind: ProviderKind::Mock,
            script: Mutex::new(Default::default()),
            health: ProviderHealth::Healthy,
            models: vec!["mock-default"],
        }
    }

    /// Set the kind tag — useful when simulating a specific backend.
    pub fn with_kind(mut self, kind: ProviderKind) -> Self {
        self.kind = kind;
        self
    }

    /// Override the exposed model list.
    pub fn with_models(mut self, models: Vec<&'static str>) -> Self {
        self.models = models;
        self
    }

    /// Set the reported health.
    pub fn with_health(mut self, h: ProviderHealth) -> Self {
        self.health = h;
        self
    }

    /// Append a scripted response.
    pub fn push(&self, step: ScriptStep) {
        #[allow(clippy::expect_used)]
        self.script
            .lock()
            .expect("MockProvider mutex poisoned")
            .push_back(step);
    }

    /// Convenience: push a simple text reply.
    pub fn push_text(&self, text: impl Into<String>) {
        self.push(ScriptStep::Reply(ChatResponse {
            model: "mock-default".into(),
            content: text.into(),
            tool_calls: vec![],
            stop_reason: Some("end_turn".into()),
            input_tokens: Some(1),
            output_tokens: Some(1),
        }));
    }

    /// Number of remaining steps.
    pub fn remaining(&self) -> usize {
        #[allow(clippy::expect_used)]
        self.script
            .lock()
            .expect("MockProvider mutex poisoned")
            .len()
    }
}

impl Default for MockProvider {
    fn default() -> Self {
        Self::new()
    }
}

#[async_trait]
impl ILLMProvider for MockProvider {
    fn kind(&self) -> ProviderKind {
        self.kind
    }

    fn health(&self) -> ProviderHealth {
        self.health
    }

    fn models(&self) -> Vec<&'static str> {
        self.models.clone()
    }

    async fn complete(&self, _req: ChatRequest) -> Result<ChatResponse, ProviderError> {
        #[allow(clippy::expect_used)]
        let step = self
            .script
            .lock()
            .expect("MockProvider mutex poisoned")
            .pop_front()
            .ok_or_else(|| {
                ProviderError::Other("mock script exhausted".to_owned())
            })?;
        match step {
            ScriptStep::Reply(r) => Ok(r),
            ScriptStep::Error(e) => Err(e),
        }
    }

    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError> {
        let resp = self.complete(req).await?;
        // Chunk the text into ~8-byte deltas for realism.
        let mut events: Vec<Result<StreamEvent, ProviderError>> = Vec::new();
        let bytes = resp.content.as_bytes();
        let mut i = 0;
        while i < bytes.len() {
            let end = (i + 8).min(bytes.len());
            let slice = &bytes[i..end];
            let s = std::str::from_utf8(slice).unwrap_or_default();
            if !s.is_empty() {
                events.push(Ok(StreamEvent::TextDelta(s.to_owned())));
            }
            i = end;
        }
        for tc in &resp.tool_calls {
            events.push(Ok(StreamEvent::ToolCall(tc.clone())));
        }
        events.push(Ok(StreamEvent::End(resp)));
        Ok(stream::iter(events).boxed())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ChatMessage, Role};

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
            max_tokens: Some(100),
            stream: false,
            structured_output: None,
        }
    }

    #[tokio::test]
    async fn scripted_reply_roundtrip() {
        let p = MockProvider::new();
        p.push_text("hello there");
        let r = p.complete(req()).await.unwrap();
        assert_eq!(r.content, "hello there");
        assert_eq!(p.remaining(), 0);
    }

    #[tokio::test]
    async fn empty_script_errors() {
        let p = MockProvider::new();
        let e = p.complete(req()).await.unwrap_err();
        assert!(matches!(e, ProviderError::Other(_)));
    }

    #[tokio::test]
    async fn streaming_emits_delta_then_end() {
        let p = MockProvider::new();
        p.push_text("hello world");
        let mut s = p.complete_stream(req()).await.unwrap();
        let mut text = String::new();
        let mut saw_end = false;
        while let Some(ev) = s.next().await {
            let ev = ev.unwrap();
            match ev {
                StreamEvent::TextDelta(t) => text.push_str(&t),
                StreamEvent::End(_) => {
                    saw_end = true;
                }
                _ => {}
            }
        }
        assert_eq!(text, "hello world");
        assert!(saw_end);
    }

    #[tokio::test]
    async fn ping_uses_complete() {
        let p = MockProvider::new();
        p.push_text("pong");
        p.ping().await.unwrap();
    }

    #[tokio::test]
    async fn error_step_surfaces() {
        let p = MockProvider::new();
        p.push(ScriptStep::Error(ProviderError::Retryable("rate".into())));
        let e = p.complete(req()).await.unwrap_err();
        assert!(matches!(e, ProviderError::Retryable(_)));
    }
}
