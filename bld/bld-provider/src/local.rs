//! Feature-gated local-inference provider stubs (ADR-0016 — Wave 9F).
//!
//! Two separate features flip these on:
//!
//! * `local-mistralrs` → pulls `mistralrs` and exposes [`MistralrsProvider`].
//! * `local-llama`     → pulls `llama-cpp-2` and exposes [`LlamaProvider`].
//!
//! Both are **unconfigured stubs** that return a clear error when the
//! model weights path is not provided. The goal of the Phase-9F slice is
//! to land the `ILLMProvider` plumbing + the hybrid router without
//! forcing everyone who builds `bld-agent` to download a 4 GB GGUF file.
//!
//! The stubs advertise themselves as `Unhealthy` until
//! [`MistralrsProvider::with_model`] / [`LlamaProvider::with_model`]
//! promotes them. The hybrid router will then skip or engage them
//! accordingly.

use async_trait::async_trait;
use futures::stream::{self, BoxStream};
use futures::StreamExt;

use crate::{
    ChatRequest, ChatResponse, ILLMProvider, ProviderError, ProviderHealth, ProviderKind,
    StreamEvent,
};

/// mistral.rs-backed local provider.
pub struct MistralrsProvider {
    model_path: Option<String>,
    model_name: String,
}

impl MistralrsProvider {
    /// New unconfigured provider (advertises `Unhealthy`).
    pub fn new() -> Self {
        Self {
            model_path: None,
            model_name: "mistral-nemo-instruct-2407".into(),
        }
    }

    /// Promote to configured by giving a GGUF / safetensors path.
    pub fn with_model(mut self, path: impl Into<String>) -> Self {
        self.model_path = Some(path.into());
        self
    }

    /// Override the advertised model name.
    pub fn with_name(mut self, name: impl Into<String>) -> Self {
        self.model_name = name.into();
        self
    }
}

impl Default for MistralrsProvider {
    fn default() -> Self {
        Self::new()
    }
}

#[async_trait]
impl ILLMProvider for MistralrsProvider {
    fn kind(&self) -> ProviderKind {
        ProviderKind::Mistralrs
    }

    fn health(&self) -> ProviderHealth {
        if self.model_path.is_some() {
            // Real implementation would probe the runtime; the stub
            // trusts the presence of a path.
            ProviderHealth::Healthy
        } else {
            ProviderHealth::Unhealthy
        }
    }

    fn models(&self) -> Vec<&'static str> {
        // Interning is hard here because the name is owned; for the stub
        // we expose a static placeholder and the real provider can
        // override later.
        vec!["mistral-nemo-local"]
    }

    async fn complete(&self, _req: ChatRequest) -> Result<ChatResponse, ProviderError> {
        if self.model_path.is_none() {
            return Err(ProviderError::Other(format!(
                "mistralrs provider: no model configured (call with_model). \
                 advertised name: {}",
                self.model_name
            )));
        }
        Err(ProviderError::Other(
            "mistralrs provider: real inference path not wired in this build \
             (feature `local-mistralrs-runtime` pending)"
                .into(),
        ))
    }

    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError> {
        let r = self.complete(req).await?;
        Ok(stream::iter(vec![Ok(StreamEvent::End(r))]).boxed())
    }
}

/// llama-cpp-2-backed local provider.
pub struct LlamaProvider {
    model_path: Option<String>,
}

impl LlamaProvider {
    /// New unconfigured provider.
    pub fn new() -> Self {
        Self { model_path: None }
    }

    /// Promote to configured by giving a GGUF path.
    pub fn with_model(mut self, path: impl Into<String>) -> Self {
        self.model_path = Some(path.into());
        self
    }
}

impl Default for LlamaProvider {
    fn default() -> Self {
        Self::new()
    }
}

#[async_trait]
impl ILLMProvider for LlamaProvider {
    fn kind(&self) -> ProviderKind {
        ProviderKind::Llama
    }

    fn health(&self) -> ProviderHealth {
        if self.model_path.is_some() {
            ProviderHealth::Healthy
        } else {
            ProviderHealth::Unhealthy
        }
    }

    fn models(&self) -> Vec<&'static str> {
        vec!["llama-3.1-8b-gguf-local"]
    }

    async fn complete(&self, _req: ChatRequest) -> Result<ChatResponse, ProviderError> {
        if self.model_path.is_none() {
            return Err(ProviderError::Other(
                "llama provider: no model configured (call with_model)".into(),
            ));
        }
        Err(ProviderError::Other(
            "llama provider: real inference path not wired in this build \
             (feature `local-llama-runtime` pending)"
                .into(),
        ))
    }

    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError> {
        let r = self.complete(req).await?;
        Ok(stream::iter(vec![Ok(StreamEvent::End(r))]).boxed())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mistralrs_unconfigured_is_unhealthy() {
        let p = MistralrsProvider::new();
        assert_eq!(p.health(), ProviderHealth::Unhealthy);
    }

    #[test]
    fn mistralrs_configured_is_healthy() {
        let p = MistralrsProvider::new().with_model("/tmp/model.gguf");
        assert_eq!(p.health(), ProviderHealth::Healthy);
    }

    #[tokio::test]
    async fn mistralrs_unconfigured_errors_clearly() {
        let p = MistralrsProvider::new();
        let req = ChatRequest {
            model: "mistral-nemo-local".into(),
            system: None,
            messages: vec![],
            tools: vec![],
            temperature: None,
            max_tokens: None,
            stream: false,
            structured_output: None,
        };
        let err = p.complete(req).await.unwrap_err();
        let msg = format!("{err}");
        assert!(msg.contains("no model"));
    }

    #[test]
    fn llama_unconfigured_is_unhealthy() {
        let p = LlamaProvider::new();
        assert_eq!(p.health(), ProviderHealth::Unhealthy);
        assert_eq!(p.kind(), ProviderKind::Llama);
    }

    #[tokio::test]
    async fn llama_configured_stub_returns_pending_error() {
        let p = LlamaProvider::new().with_model("/tmp/llama.gguf");
        let req = ChatRequest {
            model: "llama-3.1-8b".into(),
            system: None,
            messages: vec![],
            tools: vec![],
            temperature: None,
            max_tokens: None,
            stream: false,
            structured_output: None,
        };
        let err = p.complete(req).await.unwrap_err();
        let msg = format!("{err}");
        assert!(msg.contains("pending"));
    }
}
