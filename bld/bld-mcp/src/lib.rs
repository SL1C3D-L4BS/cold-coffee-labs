//! bld-mcp — MCP (Model Context Protocol 2025-11-25) transport for BLD.
//! Phase 9 lands the real implementation (docs/05 Phase 9, docs/02 §13).
//! This crate is a compile-clean stub for Phase 1.

#![warn(missing_docs)]

/// Schema version we target. docs/00 §2.5.
pub const MCP_SCHEMA_VERSION: &str = "2025-11-25";

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn schema_version_is_pinned() {
        assert_eq!(MCP_SCHEMA_VERSION, "2025-11-25");
    }
}
