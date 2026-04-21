//! MCP wire-protocol schema (subset used by BLD). ADR-0011 §2.1.
//!
//! This is JSON-RPC 2.0 with method names scoped by protocol (e.g.
//! `initialize`, `tools/list`, `tools/call`). We only model what BLD
//! emits/consumes; the full MCP surface is documented in the spec and
//! any unknown method yields `method_not_found`.

use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;

/// JSON-RPC 2.0 Request.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Request {
    /// Protocol version, always `"2.0"`.
    pub jsonrpc: String,
    /// Request id. Absent for notifications.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub id: Option<RequestId>,
    /// Method name.
    pub method: String,
    /// Parameters.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub params: Option<JsonValue>,
}

/// JSON-RPC 2.0 Response.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Response {
    /// Protocol version.
    pub jsonrpc: String,
    /// Corresponds to the originating `Request.id`.
    pub id: RequestId,
    /// Either `result` xor `error`.
    #[serde(flatten)]
    pub payload: ResponsePayload,
}

/// The `result` XOR `error` payload.
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(untagged)]
pub enum ResponsePayload {
    /// Success.
    Ok {
        /// Method-dependent result value.
        result: JsonValue,
    },
    /// Failure.
    Err {
        /// Error details.
        error: ErrorObject,
    },
}

/// JSON-RPC 2.0 Error Object.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ErrorObject {
    /// Numeric error code (see `error_codes` module).
    pub code: i32,
    /// Human-readable message.
    pub message: String,
    /// Optional structured data.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<JsonValue>,
}

/// JSON-RPC 2.0 id: integer or string.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq, Hash)]
#[serde(untagged)]
pub enum RequestId {
    /// Numeric id.
    Num(i64),
    /// String id.
    Str(String),
}

impl RequestId {
    /// Build an id from a raw JSON value if possible.
    pub fn from_value(v: &JsonValue) -> Option<Self> {
        match v {
            JsonValue::Number(n) => n.as_i64().map(RequestId::Num),
            JsonValue::String(s) => Some(RequestId::Str(s.clone())),
            _ => None,
        }
    }
}

/// Canonical JSON-RPC error codes.
pub mod error_codes {
    /// Parse error — invalid JSON.
    pub const PARSE_ERROR: i32 = -32700;
    /// Invalid request.
    pub const INVALID_REQUEST: i32 = -32600;
    /// Method not found.
    pub const METHOD_NOT_FOUND: i32 = -32601;
    /// Invalid params.
    pub const INVALID_PARAMS: i32 = -32602;
    /// Internal server error.
    pub const INTERNAL_ERROR: i32 = -32603;
    /// Server-defined: tool execution failed.
    pub const TOOL_EXECUTION_ERROR: i32 = -32000;
    /// Server-defined: tool unknown.
    pub const TOOL_UNKNOWN: i32 = -32001;
    /// Server-defined: governance denied.
    pub const GOVERNANCE_DENIED: i32 = -32002;
}

// -----------------------------------------------------------------------------
// initialize
// -----------------------------------------------------------------------------

/// Client-side params for `initialize`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InitializeParams {
    /// Client protocol version string (e.g. `"2025-11-25"`).
    #[serde(rename = "protocolVersion")]
    pub protocol_version: String,
    /// Client info.
    #[serde(rename = "clientInfo")]
    pub client_info: ClientInfo,
    /// Client capabilities.
    #[serde(default)]
    pub capabilities: JsonValue,
}

/// Server result for `initialize`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InitializeResult {
    /// Server protocol version.
    #[serde(rename = "protocolVersion")]
    pub protocol_version: String,
    /// Server info.
    #[serde(rename = "serverInfo")]
    pub server_info: ServerInfo,
    /// Server capabilities.
    pub capabilities: JsonValue,
}

/// Client descriptor.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClientInfo {
    /// Client name (e.g. `"Greywater_Engine Editor"`).
    pub name: String,
    /// Client version.
    pub version: String,
}

/// Server descriptor.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ServerInfo {
    /// Server name — always `"BLD"`.
    pub name: String,
    /// Server version — matches Cargo package version.
    pub version: String,
}

// -----------------------------------------------------------------------------
// tools
// -----------------------------------------------------------------------------

/// A tool descriptor as reported by `tools/list`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Tool {
    /// Dotted tool id.
    pub name: String,
    /// Short description.
    pub description: String,
    /// JSON-Schema for the input object.
    #[serde(rename = "inputSchema")]
    pub input_schema: JsonValue,
    /// Authorization tier as a string (`"Read"` / `"Mutate"` / `"Execute"`).
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub tier: Option<String>,
}

/// `tools/list` result.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolsListResult {
    /// List of tools.
    pub tools: Vec<Tool>,
}

/// `tools/call` params.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolCallParams {
    /// Tool id.
    pub name: String,
    /// Arguments JSON object.
    #[serde(default)]
    pub arguments: JsonValue,
}

/// `tools/call` result.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolCallResult {
    /// Content array (MCP multimodal). For BLD this is always length 1 text.
    pub content: Vec<ContentBlock>,
    /// Whether the call should be treated as an error by the client.
    #[serde(rename = "isError", default)]
    pub is_error: bool,
}

/// Structured content block.
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ContentBlock {
    /// Plain text (or JSON-as-text).
    Text {
        /// Text body.
        text: String,
    },
}

impl Request {
    /// Build an error response for this request.
    pub fn error(&self, code: i32, message: impl Into<String>, data: Option<JsonValue>) -> Response {
        let id = self.id.clone().unwrap_or(RequestId::Num(0));
        Response {
            jsonrpc: crate::JSONRPC_VERSION.to_owned(),
            id,
            payload: ResponsePayload::Err {
                error: ErrorObject {
                    code,
                    message: message.into(),
                    data,
                },
            },
        }
    }

    /// Build an ok response for this request.
    pub fn ok(&self, result: JsonValue) -> Response {
        let id = self.id.clone().unwrap_or(RequestId::Num(0));
        Response {
            jsonrpc: crate::JSONRPC_VERSION.to_owned(),
            id,
            payload: ResponsePayload::Ok { result },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn roundtrip_request() {
        let r = Request {
            jsonrpc: "2.0".into(),
            id: Some(RequestId::Num(7)),
            method: "tools/list".into(),
            params: None,
        };
        let s = serde_json::to_string(&r).unwrap();
        let parsed: Request = serde_json::from_str(&s).unwrap();
        assert_eq!(parsed.method, "tools/list");
    }

    #[test]
    fn roundtrip_response_ok() {
        let rq = Request {
            jsonrpc: "2.0".into(),
            id: Some(RequestId::Num(1)),
            method: "ping".into(),
            params: None,
        };
        let resp = rq.ok(json!({"ok": true}));
        let s = serde_json::to_string(&resp).unwrap();
        assert!(s.contains("\"result\""));
        assert!(!s.contains("\"error\""));
    }

    #[test]
    fn roundtrip_response_err() {
        let rq = Request {
            jsonrpc: "2.0".into(),
            id: Some(RequestId::Str("x".into())),
            method: "bad".into(),
            params: None,
        };
        let resp = rq.error(error_codes::METHOD_NOT_FOUND, "nope", None);
        let s = serde_json::to_string(&resp).unwrap();
        assert!(s.contains("\"error\""));
    }

    #[test]
    fn request_id_numeric_or_string() {
        let v1 = json!(12);
        let v2 = json!("abc");
        let v3 = json!(null);
        assert_eq!(RequestId::from_value(&v1), Some(RequestId::Num(12)));
        assert_eq!(RequestId::from_value(&v2), Some(RequestId::Str("abc".into())));
        assert_eq!(RequestId::from_value(&v3), None);
    }
}
