//! `CompositeDispatcher` — prefix-routed fan-out to inner dispatchers.
//!
//! Multiple tool families live in different subsystems:
//!
//! | Prefix            | Dispatcher                                   |
//! |-------------------|----------------------------------------------|
//! | `rag.*`, `docs.*` | [`crate::RagDispatcher`]                     |
//! | `project.*`       | [`crate::RagDispatcher`] (read-only views)   |
//! | `scene.*`, `component.*`, `editor.*`, `runtime.enter_play`/`runtime.exit_play` | [`crate::BridgeDispatcher`] |
//! | `build.*`, `code.*`, `vscript.*`, `runtime.*`, `debug.*` | tool-specific dispatchers (Wave 9D) |
//! | otherwise         | inventory lookup via [`bld_tools::registry::find_dispatch`] |
//!
//! This type is a thin router; each inner dispatcher owns its own state.
//! The fallback inventory path lets `#[bld_tool]`-registered tools work
//! without having to name them explicitly in the routing table.

use std::sync::Arc;

use serde_json::Value;

use crate::ToolDispatcher;

/// A single routing rule.
pub struct RoutingRule {
    /// Prefix to match (e.g. `"scene."`). Empty string is always last and
    /// acts as a catch-all.
    pub prefix: String,
    /// Dispatcher to invoke on match.
    pub dispatcher: Arc<dyn ToolDispatcher>,
}

impl std::fmt::Debug for RoutingRule {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RoutingRule")
            .field("prefix", &self.prefix)
            .finish()
    }
}

/// Prefix-based dispatcher fan-out.
#[derive(Default)]
pub struct CompositeDispatcher {
    /// Routing rules in priority order (first match wins).
    rules: Vec<RoutingRule>,
    /// Whether to fall back to [`bld_tools::registry::find_dispatch`].
    inventory_fallback: bool,
}

impl std::fmt::Debug for CompositeDispatcher {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CompositeDispatcher")
            .field("rules", &self.rules)
            .field("inventory_fallback", &self.inventory_fallback)
            .finish()
    }
}

impl CompositeDispatcher {
    /// New empty composite.
    pub fn new() -> Self {
        Self::default()
    }

    /// Enable (default: off) the inventory fallback. When on, tool ids
    /// that don't match any prefix route are looked up via the
    /// `#[bld_tool]` registry.
    pub fn with_inventory_fallback(mut self, enable: bool) -> Self {
        self.inventory_fallback = enable;
        self
    }

    /// Add a routing rule. Order matters — first match wins.
    pub fn route(mut self, prefix: impl Into<String>, d: Arc<dyn ToolDispatcher>) -> Self {
        self.rules.push(RoutingRule {
            prefix: prefix.into(),
            dispatcher: d,
        });
        self
    }

    /// Add many exact-id routes to the same dispatcher.
    pub fn route_exact(
        mut self,
        ids: impl IntoIterator<Item = &'static str>,
        d: Arc<dyn ToolDispatcher>,
    ) -> Self {
        for id in ids {
            self.rules.push(RoutingRule {
                prefix: id.to_owned(),
                dispatcher: d.clone(),
            });
        }
        self
    }
}

impl ToolDispatcher for CompositeDispatcher {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        for rule in &self.rules {
            if rule.prefix.is_empty()
                || tool_id == rule.prefix
                || tool_id.starts_with(&rule.prefix)
            {
                return rule.dispatcher.dispatch(tool_id, args);
            }
        }
        if self.inventory_fallback {
            if let Some(entry) = bld_tools::registry::find_dispatch(tool_id) {
                return (entry.dispatch)(args);
            }
        }
        Err(format!("no dispatcher matched tool id `{tool_id}`"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{BridgeDispatcher, MockHostBridge};
    use serde_json::json;

    struct Echo(&'static str);
    impl ToolDispatcher for Echo {
        fn dispatch(&self, _t: &str, _a: &Value) -> Result<Value, String> {
            Ok(json!({"from": self.0}))
        }
    }

    #[test]
    fn first_matching_prefix_wins() {
        let d = CompositeDispatcher::new()
            .route("scene.", Arc::new(Echo("scene")))
            .route("component.", Arc::new(Echo("component")));
        assert_eq!(
            d.dispatch("scene.create_entity", &json!({})).unwrap(),
            json!({"from": "scene"})
        );
        assert_eq!(
            d.dispatch("component.add", &json!({})).unwrap(),
            json!({"from": "component"})
        );
    }

    #[test]
    fn unknown_prefix_errors_without_fallback() {
        let d = CompositeDispatcher::new().route("scene.", Arc::new(Echo("scene")));
        let err = d.dispatch("build.compile", &json!({})).unwrap_err();
        assert!(err.contains("no dispatcher"));
    }

    #[test]
    fn routes_to_bridge_for_mutations() {
        let bd = Arc::new(BridgeDispatcher::new(MockHostBridge::new()));
        let d = CompositeDispatcher::new().route("scene.", bd);
        let out = d
            .dispatch("scene.create_entity", &json!({"name": "X"}))
            .unwrap();
        assert_eq!(out.get("ok").unwrap(), true);
        assert!(out.get("handle").unwrap().as_u64().is_some());
    }

    #[test]
    fn exact_route_overrides_later_prefix() {
        let d = CompositeDispatcher::new()
            .route_exact(["rag.index_file"], Arc::new(Echo("index_exact")))
            .route("rag.", Arc::new(Echo("rag_prefix")));
        assert_eq!(
            d.dispatch("rag.index_file", &json!({})).unwrap(),
            json!({"from": "index_exact"})
        );
        assert_eq!(
            d.dispatch("rag.query", &json!({})).unwrap(),
            json!({"from": "rag_prefix"})
        );
    }
}
