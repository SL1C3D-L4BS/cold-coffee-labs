//! Server capability descriptor (ADR-0011 §2.2).

use serde::{Deserialize, Serialize};
use serde_json::{json, Value as JsonValue};

/// What BLD advertises to the client during `initialize`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ServerCapabilities {
    /// Whether the server supports `tools/*`.
    pub tools: ToolsCapability,
    /// Whether the server supports `resources/*`.
    pub resources: ResourcesCapability,
    /// Whether the server supports `prompts/*`.
    pub prompts: PromptsCapability,
    /// Whether the server supports elicitation.
    pub elicitation: ElicitationCapability,
}

impl ServerCapabilities {
    /// The capability profile BLD ships with in Phase 9.
    pub fn default_for_bld() -> Self {
        Self {
            tools: ToolsCapability { list_changed: true },
            resources: ResourcesCapability {
                list_changed: true,
                subscribe: true,
            },
            prompts: PromptsCapability { list_changed: false },
            elicitation: ElicitationCapability {
                form: true,
                url: true,
            },
        }
    }

    /// Pack as JSON value for the `initialize` response.
    pub fn to_value(&self) -> JsonValue {
        json!({
            "tools": { "listChanged": self.tools.list_changed },
            "resources": {
                "listChanged": self.resources.list_changed,
                "subscribe": self.resources.subscribe,
            },
            "prompts": { "listChanged": self.prompts.list_changed },
            "elicitation": {
                "form": self.elicitation.form,
                "url": self.elicitation.url,
            }
        })
    }
}

/// Tools sub-capability.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolsCapability {
    /// Whether the server emits `notifications/tools/list_changed`.
    pub list_changed: bool,
}

/// Resources sub-capability.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ResourcesCapability {
    /// Whether the server emits `notifications/resources/list_changed`.
    pub list_changed: bool,
    /// Whether the server supports per-resource subscription.
    pub subscribe: bool,
}

/// Prompts sub-capability.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PromptsCapability {
    /// Whether the server emits `notifications/prompts/list_changed`.
    pub list_changed: bool,
}

/// Elicitation sub-capability (ADR-0011 §2.3).
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ElicitationCapability {
    /// Whether form-mode elicitation is supported.
    pub form: bool,
    /// Whether url-mode elicitation is supported.
    pub url: bool,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_profile_advertises_tools_and_elicitation() {
        let c = ServerCapabilities::default_for_bld();
        let v = c.to_value();
        assert_eq!(v.get("tools").unwrap().get("listChanged").unwrap(), true);
        assert_eq!(
            v.get("elicitation").unwrap().get("form").unwrap(),
            true
        );
        assert_eq!(v.get("elicitation").unwrap().get("url").unwrap(), true);
    }
}
