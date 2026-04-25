//! Request routing and per-method dispatch.
//!
//! `McpServer` is a pure-logic state machine. It takes in parsed `Request`
//! values and returns `Response`s (or nothing, for notifications). I/O is
//! the caller's problem. This keeps the server unit-testable without
//! spinning a tokio runtime in the test process.
//!
//! ```text
//! caller --- Request --> McpServer.handle() --- Response --> caller
//! ```
//!
//! ADR-0011 §2.2 mandates the following methods for Phase 9A:
//! - `initialize`              (required)
//! - `ping`                    (keep-alive, ADR-0010 §2.5)
//! - `tools/list`              (enumerate `#[bld_tool]` descriptors)
//! - `tools/call`              (dispatch via `Handler`)
//! - `shutdown`                (signals graceful close; sync-only)
//!
//! Additional methods (`resources/*`, `prompts/*`) are scaffolded as
//! `method_not_found` placeholders so smoke clients that probe them
//! still get a well-formed error.

use std::sync::atomic::{AtomicBool, Ordering};

use serde_json::{json, Value as JsonValue};

use crate::capabilities::ServerCapabilities;
use crate::schema::{
    error_codes, ContentBlock, InitializeParams, InitializeResult, Request, Response,
    ServerInfo, Tool, ToolCallParams, ToolCallResult, ToolsListResult,
};

/// Errors produced by the server during handling.
#[derive(Debug, thiserror::Error)]
pub enum ServerError {
    /// Invalid params.
    #[error("invalid params: {0}")]
    InvalidParams(String),
    /// The tool handler threw an error.
    #[error("tool error: {0}")]
    Tool(String),
    /// Governance denied the call.
    #[error("governance denied")]
    Denied,
}

/// What a tool looks like to the server. Implemented by the agent crate.
///
/// We use a trait object so the server is generic over the tool registry.
/// The default implementation is `StubHandler`, which reports every call
/// as not-yet-implemented — useful in smoke tests.
pub trait Handler: Send + Sync {
    /// Resolve + dispatch a tool call. `args` is JSON, `result` is JSON.
    ///
    /// Implementors are expected to:
    /// 1. Look up the tool by id in `bld-tools::registry`.
    /// 2. Run secret filter and governance gate.
    /// 3. Dispatch to the tool body.
    /// 4. Return the result JSON.
    fn call(&self, tool_id: &str, args: &JsonValue) -> Result<JsonValue, ServerError>;

    /// List every tool this handler knows about as MCP `Tool` descriptors.
    fn list(&self) -> Vec<Tool>;
}

/// Zero-tool handler — every call returns `Unimplemented`. Used in Phase 9A
/// smoke tests before Wave 9B lands the real dispatch.
#[derive(Debug, Default)]
pub struct StubHandler;

impl Handler for StubHandler {
    fn call(&self, tool_id: &str, _args: &JsonValue) -> Result<JsonValue, ServerError> {
        Err(ServerError::Tool(format!(
            "tool {tool_id} not implemented yet (Phase 9A stub)"
        )))
    }

    fn list(&self) -> Vec<Tool> {
        tools_from_registry()
    }
}

/// Maps `tools/call` to `#[bld_tool]` dispatch shims registered in the
/// current binary. Descriptor-only tools (taxonomy stubs, Sacrilege
/// scaffolds) return a clear error until a body is linked.
#[derive(Debug, Default)]
pub struct RegistryDispatchHandler;

impl Handler for RegistryDispatchHandler {
    fn call(&self, tool_id: &str, args: &JsonValue) -> Result<JsonValue, ServerError> {
        match bld_tools::registry::find_dispatch(tool_id) {
            Some(entry) => (entry.dispatch)(args).map_err(ServerError::Tool),
            None => Err(ServerError::Tool(format!(
                "tool {tool_id} has no Rust dispatch entry (descriptor-only or unknown)"
            ))),
        }
    }

    fn list(&self) -> Vec<Tool> {
        tools_from_registry()
    }
}

fn tools_from_registry() -> Vec<Tool> {
    bld_tools::registry::all()
        .map(|d| Tool {
            name: d.id.to_owned(),
            description: d.summary.to_owned(),
            input_schema: json!({ "type": "object", "additionalProperties": true }),
            tier: Some(format!("{}", d.tier)),
        })
        .collect()
}

/// The MCP server state.
pub struct McpServer {
    handler: Box<dyn Handler>,
    capabilities: ServerCapabilities,
    initialized: AtomicBool,
    shutdown_requested: AtomicBool,
    server_name: &'static str,
    server_version: &'static str,
}

impl McpServer {
    /// Build a server with the given tool handler and default capabilities.
    pub fn new(handler: Box<dyn Handler>) -> Self {
        Self {
            handler,
            capabilities: ServerCapabilities::default_for_bld(),
            initialized: AtomicBool::new(false),
            shutdown_requested: AtomicBool::new(false),
            server_name: "BLD",
            server_version: env!("CARGO_PKG_VERSION"),
        }
    }

    /// Whether a shutdown request has been received.
    pub fn shutdown_requested(&self) -> bool {
        self.shutdown_requested.load(Ordering::SeqCst)
    }

    /// Whether `initialize` has completed.
    pub fn initialized(&self) -> bool {
        self.initialized.load(Ordering::SeqCst)
    }

    /// Handle a single JSON-RPC request. Notifications return `None`.
    pub fn handle(&self, req: Request) -> Option<Response> {
        // Notifications have no `id` and no response is sent.
        let is_notification = req.id.is_none();

        let result = self.dispatch(&req);

        if is_notification {
            return None;
        }
        let resp = match result {
            Ok(v) => req.ok(v),
            Err(ServerError::InvalidParams(m)) => {
                req.error(error_codes::INVALID_PARAMS, m, None)
            }
            Err(ServerError::Tool(m)) => {
                req.error(error_codes::TOOL_EXECUTION_ERROR, m, None)
            }
            Err(ServerError::Denied) => {
                req.error(error_codes::GOVERNANCE_DENIED, "denied by user", None)
            }
        };
        Some(resp)
    }

    fn dispatch(&self, req: &Request) -> Result<JsonValue, ServerError> {
        match req.method.as_str() {
            "initialize" => self.on_initialize(req),
            "ping" => Ok(json!({"ok": true})),
            "shutdown" => {
                self.shutdown_requested.store(true, Ordering::SeqCst);
                Ok(JsonValue::Null)
            }
            "tools/list" => {
                let res = ToolsListResult {
                    tools: self.handler.list(),
                };
                Ok(serde_json::to_value(res).unwrap_or(JsonValue::Null))
            }
            "tools/call" => self.on_tools_call(req),
            // Stubs for methods we advertise but don't fully implement yet.
            "resources/list" => Ok(json!({ "resources": [] })),
            "prompts/list" => Ok(json!({ "prompts": [] })),
            other => Err(ServerError::InvalidParams(format!(
                "method not supported: {other}"
            ))),
        }
    }

    fn on_initialize(&self, req: &Request) -> Result<JsonValue, ServerError> {
        let params: InitializeParams = match &req.params {
            Some(p) => serde_json::from_value(p.clone())
                .map_err(|e| ServerError::InvalidParams(e.to_string()))?,
            None => {
                return Err(ServerError::InvalidParams(
                    "initialize requires params".into(),
                ))
            }
        };
        if params.protocol_version != crate::MCP_SCHEMA_VERSION {
            tracing::warn!(
                "client protocol version {} != server {}",
                params.protocol_version,
                crate::MCP_SCHEMA_VERSION
            );
        }
        self.initialized.store(true, Ordering::SeqCst);
        let result = InitializeResult {
            protocol_version: crate::MCP_SCHEMA_VERSION.to_owned(),
            server_info: ServerInfo {
                name: self.server_name.to_owned(),
                version: self.server_version.to_owned(),
            },
            capabilities: self.capabilities.to_value(),
        };
        serde_json::to_value(result).map_err(|e| ServerError::InvalidParams(e.to_string()))
    }

    fn on_tools_call(&self, req: &Request) -> Result<JsonValue, ServerError> {
        let params: ToolCallParams = match &req.params {
            Some(p) => serde_json::from_value(p.clone())
                .map_err(|e| ServerError::InvalidParams(e.to_string()))?,
            None => {
                return Err(ServerError::InvalidParams(
                    "tools/call requires params".into(),
                ))
            }
        };
        match self.handler.call(&params.name, &params.arguments) {
            Ok(result) => {
                let text = serde_json::to_string(&result).unwrap_or_default();
                let content = ToolCallResult {
                    content: vec![ContentBlock::Text { text }],
                    is_error: false,
                };
                serde_json::to_value(content)
                    .map_err(|e| ServerError::Tool(e.to_string()))
            }
            Err(e) => {
                let text = e.to_string();
                let content = ToolCallResult {
                    content: vec![ContentBlock::Text { text }],
                    is_error: true,
                };
                serde_json::to_value(content)
                    .map_err(|e| ServerError::Tool(e.to_string()))
            }
        }
    }
}

// =============================================================================
// Async stdio driver (feature-gated; only compiled with --features runtime).
// =============================================================================

#[cfg(feature = "runtime")]
pub mod stdio {
    //! Tokio stdio driver for `McpServer`. ADR-0011 §2.1.

    use tokio::io::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt, BufReader};
    use tokio::io::{AsyncRead, AsyncWrite};

    use super::{McpServer, Request};

    /// Run the MCP server against an async reader/writer pair until EOF or
    /// `shutdown` is received.
    pub async fn run<R, W>(server: &McpServer, reader: R, mut writer: W) -> std::io::Result<()>
    where
        R: AsyncRead + Unpin,
        W: AsyncWrite + Unpin,
    {
        let mut reader = BufReader::new(reader);
        loop {
            if server.shutdown_requested() {
                break;
            }
            let body = match read_frame_async(&mut reader).await? {
                Some(b) => b,
                None => break,
            };
            let parsed: Result<Request, _> = serde_json::from_slice(&body);
            match parsed {
                Ok(req) => {
                    if let Some(resp) = server.handle(req) {
                        let body = serde_json::to_vec(&resp)?;
                        write_frame_async(&mut writer, &body).await?;
                    }
                }
                Err(e) => {
                    // PARSE_ERROR response has id=null; emit directly.
                    let body = serde_json::to_vec(&serde_json::json!({
                        "jsonrpc": "2.0",
                        "id": null,
                        "error": {
                            "code": super::error_codes::PARSE_ERROR,
                            "message": format!("parse error: {e}"),
                        }
                    }))?;
                    write_frame_async(&mut writer, &body).await?;
                }
            }
        }
        Ok(())
    }

    async fn read_frame_async<R: AsyncRead + Unpin>(
        r: &mut BufReader<R>,
    ) -> std::io::Result<Option<Vec<u8>>> {
        let mut content_length: Option<usize> = None;
        let mut header_bytes = 0usize;
        loop {
            let mut line = String::new();
            let n = r.read_line(&mut line).await?;
            if n == 0 {
                if header_bytes == 0 {
                    return Ok(None);
                }
                return Err(std::io::Error::new(
                    std::io::ErrorKind::UnexpectedEof,
                    "eof mid-header",
                ));
            }
            header_bytes += n;
            if line == "\r\n" || line == "\n" {
                break;
            }
            if let Some((key, value)) = line.split_once(':') {
                if key.trim().eq_ignore_ascii_case("content-length") {
                    let v = value.trim();
                    content_length = Some(v.parse().map_err(|_| {
                        std::io::Error::new(
                            std::io::ErrorKind::InvalidData,
                            format!("bad content-length: {v}"),
                        )
                    })?);
                }
            }
        }
        let cl = content_length.ok_or_else(|| {
            std::io::Error::new(std::io::ErrorKind::InvalidData, "missing content-length")
        })?;
        let mut buf = vec![0u8; cl];
        r.read_exact(&mut buf).await?;
        Ok(Some(buf))
    }

    async fn write_frame_async<W: AsyncWrite + Unpin>(
        w: &mut W,
        body: &[u8],
    ) -> std::io::Result<()> {
        let header = format!("Content-Length: {}\r\n\r\n", body.len());
        w.write_all(header.as_bytes()).await?;
        w.write_all(body).await?;
        w.flush().await?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    fn server() -> McpServer {
        McpServer::new(Box::new(RegistryDispatchHandler))
    }

    #[test]
    fn initialize_returns_server_info_and_caps() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(1)),
            method: "initialize".into(),
            params: Some(json!({
                "protocolVersion": crate::MCP_SCHEMA_VERSION,
                "clientInfo": {"name": "test", "version": "0.0"},
                "capabilities": {}
            })),
        };
        let resp = s.handle(req).unwrap();
        if let crate::schema::ResponsePayload::Ok { result } = resp.payload {
            assert_eq!(result.get("protocolVersion").unwrap(), crate::MCP_SCHEMA_VERSION);
            assert_eq!(result.get("serverInfo").unwrap().get("name").unwrap(), "BLD");
            assert!(result.get("capabilities").is_some());
        } else {
            panic!("expected Ok response");
        }
        assert!(s.initialized());
    }

    #[test]
    fn ping_works() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(5)),
            method: "ping".into(),
            params: None,
        };
        let resp = s.handle(req).unwrap();
        if let crate::schema::ResponsePayload::Ok { result } = resp.payload {
            assert_eq!(result.get("ok").unwrap(), true);
        } else {
            panic!("expected Ok");
        }
    }

    #[test]
    fn tools_list_includes_seed() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(2)),
            method: "tools/list".into(),
            params: None,
        };
        let resp = s.handle(req).unwrap();
        if let crate::schema::ResponsePayload::Ok { result } = resp.payload {
            let tools = result.get("tools").unwrap().as_array().unwrap();
            assert!(
                tools.iter().any(|t| t.get("name").unwrap() == "project.list_scenes"),
                "project.list_scenes must be listed"
            );
        } else {
            panic!("expected Ok");
        }
    }

    #[test]
    fn tools_call_unknown_tool_returns_tool_error_content() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(3)),
            method: "tools/call".into(),
            params: Some(json!({"name": "nope", "arguments": {}})),
        };
        let resp = s.handle(req).unwrap();
        if let crate::schema::ResponsePayload::Ok { result } = resp.payload {
            assert_eq!(result.get("isError").unwrap(), true);
        } else {
            panic!("expected Ok with isError content");
        }
    }

    #[test]
    fn unknown_method_errors() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(4)),
            method: "not/a/method".into(),
            params: None,
        };
        let resp = s.handle(req).unwrap();
        assert!(matches!(resp.payload, crate::schema::ResponsePayload::Err { .. }));
    }

    #[test]
    fn notifications_return_no_response() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: None,
            method: "ping".into(),
            params: None,
        };
        assert!(s.handle(req).is_none());
    }

    #[test]
    fn shutdown_sets_flag() {
        let s = server();
        let req = Request {
            jsonrpc: "2.0".into(),
            id: Some(crate::schema::RequestId::Num(99)),
            method: "shutdown".into(),
            params: None,
        };
        let _ = s.handle(req);
        assert!(s.shutdown_requested());
    }
}
