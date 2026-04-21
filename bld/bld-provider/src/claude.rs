//! Anthropic Claude provider (cloud). Feature-gated `cloud-claude`.
//!
//! We implement against the Anthropic `messages` API (2024-06-04 schema)
//! with the `anthropic-version: 2024-10-22` header. This is a direct
//! `reqwest` implementation rather than `rig-core` so the wire contract
//! is completely under our control — ADR-0010 §2.1 treats rig-core as a
//! carrier, not a dependency.
//!
//! Endpoints:
//! - `POST /v1/messages`             — non-streaming
//! - `POST /v1/messages` (SSE)       — streaming (set `"stream": true`)
//!
//! Tool calling uses Anthropic's `tools` param. We marshal `ToolSpec` ->
//! `{"name", "description", "input_schema"}` exactly as the API expects.
//! Responses carry `stop_reason = "tool_use"` when the model wants to
//! call a tool; each tool call appears as a `tool_use` content block.

use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use async_trait::async_trait;
use futures::stream::BoxStream;
use futures::StreamExt;
use serde_json::{json, Value as JsonValue};

use crate::keystore::{KeyStore, Secret};
use crate::{
    ChatMessage, ChatRequest, ChatResponse, ILLMProvider, ProviderError, ProviderHealth,
    ProviderKind, Role, StreamEvent, ToolCallEvent, ToolSpec,
};

/// Default Anthropic API base.
pub const ANTHROPIC_API_BASE: &str = "https://api.anthropic.com";

/// The Anthropic API version header we pin to.
pub const ANTHROPIC_VERSION: &str = "2024-10-22";

/// Default primary model (ADR-0010 §2.1).
pub const DEFAULT_MODEL: &str = "claude-opus-4-7-20260401";

/// Claude provider configuration.
pub struct ClaudeConfig {
    /// API base URL (override for staging / proxy).
    pub base: String,
    /// Anthropic-Version header.
    pub anthropic_version: String,
    /// Models exposed. Router picks from this list.
    pub models: Vec<&'static str>,
    /// Secret store for the API key.
    pub keystore: Arc<dyn KeyStore>,
    /// Timeout per request.
    pub timeout: std::time::Duration,
}

impl ClaudeConfig {
    /// Build from an existing keystore.
    pub fn new(keystore: Arc<dyn KeyStore>) -> Self {
        Self {
            base: ANTHROPIC_API_BASE.to_owned(),
            anthropic_version: ANTHROPIC_VERSION.to_owned(),
            models: vec![DEFAULT_MODEL, "claude-sonnet-4-5-20260401"],
            keystore,
            timeout: std::time::Duration::from_secs(60),
        }
    }
}

/// Claude cloud provider.
pub struct ClaudeProvider {
    cfg: ClaudeConfig,
    http: reqwest::Client,
    consecutive_failures: AtomicU32,
}

impl ClaudeProvider {
    /// Construct with a config. Builds the HTTP client lazily.
    pub fn new(cfg: ClaudeConfig) -> Result<Self, ProviderError> {
        let http = reqwest::Client::builder()
            .timeout(cfg.timeout)
            .build()
            .map_err(|e| ProviderError::Transport(e.to_string()))?;
        Ok(Self {
            cfg,
            http,
            consecutive_failures: AtomicU32::new(0),
        })
    }

    fn fetch_secret(&self) -> Result<Secret, ProviderError> {
        self.cfg
            .keystore
            .get("claude")
            .map_err(|e| ProviderError::Auth(e.to_string()))
    }

    fn mark_ok(&self) {
        self.consecutive_failures.store(0, Ordering::SeqCst);
    }

    fn mark_fail(&self) {
        self.consecutive_failures.fetch_add(1, Ordering::SeqCst);
    }

    fn build_body(&self, req: &ChatRequest, stream: bool) -> JsonValue {
        let messages: Vec<JsonValue> = req
            .messages
            .iter()
            .filter(|m| m.role != Role::System)
            .map(render_message)
            .collect();
        let tools: Vec<JsonValue> = req.tools.iter().map(render_tool).collect();

        let mut body = json!({
            "model": req.model,
            "max_tokens": req.max_tokens.unwrap_or(4096),
            "messages": messages,
            "stream": stream,
        });
        if let Some(ref sys) = req.system {
            body["system"] = json!(sys);
        }
        if !tools.is_empty() {
            body["tools"] = json!(tools);
        }
        if let Some(t) = req.temperature {
            body["temperature"] = json!(t);
        }
        body
    }
}

fn render_message(m: &ChatMessage) -> JsonValue {
    match m.role {
        Role::User => json!({
            "role": "user",
            "content": [{ "type": "text", "text": m.content }],
        }),
        Role::Assistant => json!({
            "role": "assistant",
            "content": [{ "type": "text", "text": m.content }],
        }),
        Role::Tool => json!({
            "role": "user",
            "content": [{
                "type": "tool_result",
                "tool_use_id": m.tool_call_id.clone().unwrap_or_default(),
                "content": m.content,
            }]
        }),
        // Systems are hoisted to the top-level `system` param; if one slips
        // through here, coerce to user.
        Role::System => json!({
            "role": "user",
            "content": [{ "type": "text", "text": m.content }],
        }),
    }
}

fn render_tool(t: &ToolSpec) -> JsonValue {
    json!({
        "name": t.name,
        "description": t.description,
        "input_schema": t.input_schema,
    })
}

fn parse_response(v: JsonValue, model: String) -> Result<ChatResponse, ProviderError> {
    let content = v
        .get("content")
        .and_then(|c| c.as_array())
        .ok_or_else(|| ProviderError::Decode("missing content".into()))?;
    let mut text = String::new();
    let mut tool_calls = Vec::new();
    for block in content {
        match block.get("type").and_then(|t| t.as_str()) {
            Some("text") => {
                if let Some(s) = block.get("text").and_then(|t| t.as_str()) {
                    text.push_str(s);
                }
            }
            Some("tool_use") => {
                let id = block
                    .get("id")
                    .and_then(|v| v.as_str())
                    .unwrap_or("")
                    .to_owned();
                let name = block
                    .get("name")
                    .and_then(|v| v.as_str())
                    .unwrap_or("")
                    .to_owned();
                let arguments = block.get("input").cloned().unwrap_or(JsonValue::Null);
                tool_calls.push(ToolCallEvent {
                    id,
                    name,
                    arguments,
                });
            }
            _ => {}
        }
    }
    let stop_reason = v
        .get("stop_reason")
        .and_then(|v| v.as_str())
        .map(str::to_owned);
    let usage = v.get("usage");
    let input_tokens = usage
        .and_then(|u| u.get("input_tokens"))
        .and_then(|v| v.as_u64())
        .map(|n| n as u32);
    let output_tokens = usage
        .and_then(|u| u.get("output_tokens"))
        .and_then(|v| v.as_u64())
        .map(|n| n as u32);

    Ok(ChatResponse {
        model,
        content: text,
        tool_calls,
        stop_reason,
        input_tokens,
        output_tokens,
    })
}

#[async_trait]
impl ILLMProvider for ClaudeProvider {
    fn kind(&self) -> ProviderKind {
        ProviderKind::Claude
    }

    fn health(&self) -> ProviderHealth {
        // Three consecutive failures flips us to Unhealthy (ADR-0016 §2.3).
        if self.consecutive_failures.load(Ordering::SeqCst) >= 3 {
            ProviderHealth::Unhealthy
        } else {
            ProviderHealth::Healthy
        }
    }

    fn models(&self) -> Vec<&'static str> {
        self.cfg.models.clone()
    }

    async fn complete(&self, req: ChatRequest) -> Result<ChatResponse, ProviderError> {
        let key = self.fetch_secret()?;
        let body = self.build_body(&req, false);
        let url = format!("{}/v1/messages", self.cfg.base);
        let resp = self
            .http
            .post(&url)
            .header("x-api-key", key.expose())
            .header("anthropic-version", &self.cfg.anthropic_version)
            .header("content-type", "application/json")
            .json(&body)
            .send()
            .await
            .map_err(|e| {
                self.mark_fail();
                ProviderError::Transport(e.to_string())
            })?;
        let status = resp.status();
        let text = resp
            .text()
            .await
            .map_err(|e| ProviderError::Transport(e.to_string()))?;
        if !status.is_success() {
            self.mark_fail();
            return Err(match status.as_u16() {
                401 | 403 => ProviderError::Auth(text),
                429 | 500..=599 => ProviderError::Retryable(text),
                _ => ProviderError::Other(format!("{status}: {text}")),
            });
        }
        let v: JsonValue = serde_json::from_str(&text)
            .map_err(|e| ProviderError::Decode(e.to_string()))?;
        self.mark_ok();
        parse_response(v, req.model)
    }

    async fn complete_stream(
        &self,
        req: ChatRequest,
    ) -> Result<BoxStream<'static, Result<StreamEvent, ProviderError>>, ProviderError> {
        use eventsource_stream::Eventsource;
        let key = self.fetch_secret()?;
        let body = self.build_body(&req, true);
        let url = format!("{}/v1/messages", self.cfg.base);
        let resp = self
            .http
            .post(&url)
            .header("x-api-key", key.expose())
            .header("anthropic-version", &self.cfg.anthropic_version)
            .header("content-type", "application/json")
            .json(&body)
            .send()
            .await
            .map_err(|e| {
                self.mark_fail();
                ProviderError::Transport(e.to_string())
            })?;
        if !resp.status().is_success() {
            self.mark_fail();
            let s = resp.status();
            let t = resp.text().await.unwrap_or_default();
            return Err(ProviderError::Other(format!("{s}: {t}")));
        }
        self.mark_ok();
        let model = req.model.clone();
        let byte_stream = resp.bytes_stream().eventsource();
        let mapped = byte_stream.filter_map(move |chunk| {
            let model = model.clone();
            async move {
                match chunk {
                    Ok(ev) => parse_sse_event(&ev.event, &ev.data, &model),
                    Err(e) => Some(Err(ProviderError::Transport(e.to_string()))),
                }
            }
        });
        Ok(mapped.boxed())
    }
}

fn parse_sse_event(
    event: &str,
    data: &str,
    model: &str,
) -> Option<Result<StreamEvent, ProviderError>> {
    if data.is_empty() {
        return None;
    }
    // Anthropic SSE events: message_start, content_block_delta,
    // content_block_stop, message_stop. We only emit TextDelta on
    // `content_block_delta(text_delta)`, ToolCall on `content_block_stop`
    // of a `tool_use` block, and End on `message_stop`.
    let v: JsonValue = match serde_json::from_str(data) {
        Ok(v) => v,
        Err(e) => return Some(Err(ProviderError::Decode(e.to_string()))),
    };
    match event {
        "content_block_delta" => {
            if let Some(delta) = v.get("delta") {
                if delta.get("type").and_then(|t| t.as_str()) == Some("text_delta") {
                    if let Some(t) = delta.get("text").and_then(|t| t.as_str()) {
                        return Some(Ok(StreamEvent::TextDelta(t.to_owned())));
                    }
                }
            }
            None
        }
        "message_stop" => {
            // Defer to whatever the server sent in the closing usage block.
            let final_resp = ChatResponse {
                model: model.to_owned(),
                content: String::new(),
                tool_calls: vec![],
                stop_reason: Some("end_turn".into()),
                input_tokens: None,
                output_tokens: None,
            };
            Some(Ok(StreamEvent::End(final_resp)))
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn build_body_hoists_system_and_maps_messages() {
        let ks = Arc::new(crate::keystore::EnvKeyStore::new().with_override("claude", "x"));
        let p = ClaudeProvider::new(ClaudeConfig::new(ks)).unwrap();
        let req = ChatRequest {
            model: DEFAULT_MODEL.into(),
            system: Some("you are a helpful engine copilot".into()),
            messages: vec![
                ChatMessage {
                    role: Role::User,
                    content: "hi".into(),
                    tool_call_id: None,
                    tool_name: None,
                },
                ChatMessage {
                    role: Role::Assistant,
                    content: "hello".into(),
                    tool_call_id: None,
                    tool_name: None,
                },
            ],
            tools: vec![ToolSpec {
                name: "scene.create_entity".into(),
                description: "Create an entity".into(),
                input_schema: json!({"type": "object"}),
            }],
            temperature: Some(0.0),
            max_tokens: Some(512),
            stream: false,
            structured_output: None,
        };
        let b = p.build_body(&req, false);
        assert_eq!(b["system"], "you are a helpful engine copilot");
        assert_eq!(b["stream"], false);
        assert_eq!(b["messages"].as_array().unwrap().len(), 2);
        assert_eq!(b["tools"][0]["name"], "scene.create_entity");
    }

    #[test]
    fn parse_response_collects_text_and_tool_use() {
        let v = json!({
            "content": [
                {"type": "text", "text": "Let me call a tool."},
                {"type": "tool_use", "id": "t_1", "name": "scene.create_entity",
                 "input": {"name": "Cube"}}
            ],
            "stop_reason": "tool_use",
            "usage": {"input_tokens": 30, "output_tokens": 12}
        });
        let r = parse_response(v, "claude-x".into()).unwrap();
        assert_eq!(r.content, "Let me call a tool.");
        assert_eq!(r.tool_calls.len(), 1);
        assert_eq!(r.tool_calls[0].name, "scene.create_entity");
        assert_eq!(r.stop_reason.as_deref(), Some("tool_use"));
        assert_eq!(r.input_tokens, Some(30));
    }

    #[test]
    fn health_degrades_after_three_failures() {
        let ks = Arc::new(crate::keystore::EnvKeyStore::new().with_override("claude", "x"));
        let p = ClaudeProvider::new(ClaudeConfig::new(ks)).unwrap();
        assert_eq!(p.health(), ProviderHealth::Healthy);
        p.mark_fail();
        p.mark_fail();
        assert_eq!(p.health(), ProviderHealth::Healthy);
        p.mark_fail();
        assert_eq!(p.health(), ProviderHealth::Unhealthy);
        p.mark_ok();
        assert_eq!(p.health(), ProviderHealth::Healthy);
    }

    #[test]
    fn sse_parses_text_delta_and_stop() {
        let ev = parse_sse_event(
            "content_block_delta",
            r#"{"type":"content_block_delta","index":0,"delta":{"type":"text_delta","text":"Hi"}}"#,
            "claude-x",
        )
        .unwrap()
        .unwrap();
        assert!(matches!(ev, StreamEvent::TextDelta(ref s) if s == "Hi"));
        let ev = parse_sse_event("message_stop", "{}", "claude-x").unwrap().unwrap();
        assert!(matches!(ev, StreamEvent::End(_)));
    }
}
