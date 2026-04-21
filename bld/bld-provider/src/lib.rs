//! `bld-provider` — LLM transport abstraction for BLD (ADR-0010).
//!
//! The `ILLMProvider` trait is the only surface the agent loop sees; every
//! concrete provider (Claude cloud, mistral.rs local, llama-cpp-2 local,
//! tests) implements it.
//!
//! What ships here in Phase 9A:
//!
//! - `ILLMProvider` — async trait, streaming + non-streaming completion
//!   surfaces, health-probe, model metadata.
//! - `ProviderKind` — routing enum, plus `ProviderHealth` for the hybrid
//!   router (`MockProvider` is `Healthy` by default; cloud providers
//!   fall to `Unhealthy` after three consecutive failures).
//! - `ChatMessage` / `ChatTurn` / `ChatRequest` / `ChatResponse`
//!   — serde-backed shapes the agent feeds in, identical to Anthropic's
//!     `messages` endpoint shape so we can marshal to Claude with zero
//!     conversion, and identical enough to OpenAI/Gemini/local-LLM that
//!     adapters are trivial.
//! - `KeyStore` — secret retrieval facade. Default impl reads
//!   `ANTHROPIC_API_KEY` from the environment. The `keyring` feature adds
//!   OS-native keychain storage.
//! - `mock::MockProvider` — deterministic test provider. Always on; every
//!   test uses it. Responses are scripted.
//!
//! Real cloud providers live under feature flags so the base build never
//! pulls reqwest/rustls or rig-core unless explicitly enabled.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

use async_trait::async_trait;
use futures::stream::BoxStream;
use serde::{Deserialize, Serialize};

pub mod hybrid;
pub mod keystore;
pub mod local;
pub mod mock;

#[cfg(feature = "cloud-claude")]
pub mod claude;

pub use hybrid::{requires_network, HybridRouter, OfflineFlag};
pub use keystore::{EnvKeyStore, KeyStore, KeyStoreError};
pub use local::{LlamaProvider, MistralrsProvider};
pub use mock::MockProvider;

/// Provider error type.
#[derive(Debug, Clone, thiserror::Error)]
pub enum ProviderError {
    /// The request was malformed.
    #[error("bad request: {0}")]
    BadRequest(String),
    /// Rate-limited or temporary server error; caller should retry.
    #[error("retryable: {0}")]
    Retryable(String),
    /// Authentication failed (missing / invalid API key).
    #[error("auth: {0}")]
    Auth(String),
    /// The underlying transport (network / IO) failed.
    #[error("transport: {0}")]
    Transport(String),
    /// The response could not be parsed.
    #[error("decode: {0}")]
    Decode(String),
    /// The request was cancelled by the caller.
    #[error("cancelled")]
    Cancelled,
    /// Provider-specific error with context.
    #[error("provider: {0}")]
    Other(String),
}

/// Routing kind for the hybrid-router (ADR-0016 §2.3).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ProviderKind {
    /// Anthropic Claude (cloud).
    Claude,
    /// Local mistral.rs engine.
    Mistralrs,
    /// Local llama-cpp-2 engine.
    Llama,
    /// Mock provider for tests.
    Mock,
}

/// Provider health as reported by the last probe / call.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProviderHealth {
    /// Ready for use.
    Healthy,
    /// Recent failures; router may skip.
    Unhealthy,
    /// Explicitly disabled (offline mode, user toggle).
    Disabled,
}

/// Role of a message in a chat turn.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Role {
    /// System / developer prompt.
    System,
    /// User (agent runner or human).
    User,
    /// Assistant (LLM).
    Assistant,
    /// Tool result injected back into the conversation.
    Tool,
}

/// One chat message.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ChatMessage {
    /// Speaker.
    pub role: Role,
    /// Text content.
    pub content: String,
    /// Optional tool-call id, present when `role = Tool`.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub tool_call_id: Option<String>,
    /// Optional tool name being responded to.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub tool_name: Option<String>,
}

/// Tool descriptor as passed to the provider.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolSpec {
    /// Tool id (dotted).
    pub name: String,
    /// Description.
    pub description: String,
    /// JSON-Schema for the input.
    pub input_schema: serde_json::Value,
}

/// Request payload sent to `ILLMProvider::complete`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ChatRequest {
    /// Model id (provider-specific, e.g. `"claude-opus-4-7-20260401"`).
    pub model: String,
    /// System prompt, sent out-of-band where the provider allows it.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub system: Option<String>,
    /// Conversation so far, oldest first.
    pub messages: Vec<ChatMessage>,
    /// Tools available on this turn.
    #[serde(default)]
    pub tools: Vec<ToolSpec>,
    /// Temperature (0.0 = deterministic). Clamped by the provider.
    #[serde(default)]
    pub temperature: Option<f32>,
    /// Max output tokens.
    #[serde(default)]
    pub max_tokens: Option<u32>,
    /// Whether to use streaming.
    #[serde(default)]
    pub stream: bool,
    /// Structured-output schema. Providers that support JSON-Schema
    /// guided decoding (Claude, mistral.rs) use it; others ignore it.
    #[serde(default)]
    pub structured_output: Option<serde_json::Value>,
}

/// One tool call emitted by the assistant.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolCallEvent {
    /// Unique call id (mirrored back via `ChatMessage::tool_call_id`).
    pub id: String,
    /// Tool name.
    pub name: String,
    /// Arguments JSON.
    pub arguments: serde_json::Value,
}

/// Final response from a non-streaming call.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ChatResponse {
    /// Model that served the response.
    pub model: String,
    /// Assistant text (may be empty if the response is entirely tool calls).
    pub content: String,
    /// Tool calls emitted.
    #[serde(default)]
    pub tool_calls: Vec<ToolCallEvent>,
    /// Stop reason (provider-dependent: "end_turn", "tool_use", "max_tokens").
    #[serde(default)]
    pub stop_reason: Option<String>,
    /// Input token count (if reported).
    #[serde(default)]
    pub input_tokens: Option<u32>,
    /// Output token count.
    #[serde(default)]
    pub output_tokens: Option<u32>,
}

/// One chunk of a streamed response.
#[derive(Debug, Clone)]
pub enum StreamEvent {
    /// Text delta.
    TextDelta(String),
    /// A tool call was completed.
    ToolCall(ToolCallEvent),
    /// The stream has ended. Carries the final aggregate.
    End(ChatResponse),
    /// A non-fatal warning (e.g. token budget).
    Warning(String),
}

/// The core trait. Provider crates implement this.
#[async_trait]
pub trait ILLMProvider: Send + Sync {
    /// Stable kind tag (for routing + metrics).
    fn kind(&self) -> ProviderKind;

    /// Current health.
    fn health(&self) -> ProviderHealth;

    /// Models this provider exposes.
    fn models(&self) -> Vec<&'static str>;

    /// Non-streaming completion.
    async fn complete(&self, req: ChatRequest) -> Result<ChatResponse, ProviderError>;

    /// Streaming completion. Returns a boxed stream of events ending in
    /// exactly one `StreamEvent::End`.
    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError>;

    /// Keep-alive / health probe (ADR-0010 §2.5). Default impl sends a
    /// tiny ping request.
    async fn ping(&self) -> Result<(), ProviderError> {
        let req = ChatRequest {
            model: self
                .models()
                .first()
                .copied()
                .unwrap_or("default")
                .to_owned(),
            system: None,
            messages: vec![ChatMessage {
                role: Role::User,
                content: "ping".into(),
                tool_call_id: None,
                tool_name: None,
            }],
            tools: vec![],
            temperature: Some(0.0),
            max_tokens: Some(4),
            stream: false,
            structured_output: None,
        };
        self.complete(req).await.map(|_| ())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn provider_kinds_roundtrip() {
        for k in [
            ProviderKind::Claude,
            ProviderKind::Mistralrs,
            ProviderKind::Llama,
            ProviderKind::Mock,
        ] {
            let s = serde_json::to_string(&k).unwrap();
            let p: ProviderKind = serde_json::from_str(&s).unwrap();
            assert_eq!(k, p);
        }
    }

    #[test]
    fn role_roundtrip() {
        for r in [Role::System, Role::User, Role::Assistant, Role::Tool] {
            let s = serde_json::to_string(&r).unwrap();
            let p: Role = serde_json::from_str(&s).unwrap();
            assert_eq!(r, p);
        }
    }
}
