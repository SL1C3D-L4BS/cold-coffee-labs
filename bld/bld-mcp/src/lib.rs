//! `bld-mcp` — Model Context Protocol 2025-11-25 transport for BLD.
//!
//! ADR-0011 mandates stdio as the default transport with Streamable HTTP as
//! a secondary option. We implement a minimal, spec-correct JSON-RPC 2.0
//! framing layer in pure Rust here. The `rmcp` crate is feature-gated for
//! when the editor wants the official wire implementation; the handshake
//! and message shapes match either way so either transport can drive the
//! same `McpServer` handler.
//!
//! Modules:
//! - `schema`  — wire-protocol types (Initialize, Tool, ToolCall, Result, Error)
//! - `frame`   — Content-Length framed JSON-RPC over async byte streams
//! - `server`  — request routing + tools/list / tools/call dispatch
//! - `capabilities` — server capability descriptor
//!
//! The library has NO global state. Every `McpServer` is independent —
//! this is how multi-session (Surface P v3, ADR-0015) works.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

/// MCP schema version we implement (ADR-0011).
pub const MCP_SCHEMA_VERSION: &str = "2025-11-25";

/// JSON-RPC 2.0 protocol version.
pub const JSONRPC_VERSION: &str = "2.0";

pub mod capabilities;
pub mod frame;
pub mod schema;
pub mod server;

pub use capabilities::ServerCapabilities;
pub use schema::{ErrorObject, Request, Response, ResponsePayload};
pub use server::{Handler, McpServer, ServerError};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn schema_version_is_pinned() {
        assert_eq!(MCP_SCHEMA_VERSION, "2025-11-25");
    }
}
