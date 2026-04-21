//! Elicitation policy (ADR-0011 §2.3).
//!
//! - `form` mode: structured confirmation (diff panel, approve/deny,
//!   radio selection). Secret-shaped fields are **forbidden** by
//!   compile-time regex — see `validate_form_request`.
//! - `url` mode: external browser flow (OAuth, API-key entry). Used for any
//!   field matching the secret regex list.

use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};

use crate::tier::Tier;

/// The two MCP elicitation modes from the 2025-11-25 spec.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ElicitationMode {
    /// In-editor form. Diff panel, confirm/deny, radio group. Never secrets.
    Form,
    /// External URL (browser). Used for OAuth, API-key entry, etc.
    Url,
}

/// A single elicitation request.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ElicitationRequest {
    /// Mode (form or url).
    pub mode: ElicitationMode,
    /// Tool id this request is gating.
    pub tool_id: String,
    /// Tier of the tool.
    pub tier: Tier,
    /// The prompt the user sees (UI renders markdown-safely).
    pub prompt: String,
    /// For form mode: the structured fields to show; for url: ignored.
    pub form_fields: Vec<FormField>,
    /// For url mode: the URL to open; for form: ignored.
    pub url: Option<String>,
}

impl ElicitationRequest {
    /// Convenience constructor: "confirm execution of tool X with args Y".
    pub fn confirm(
        tool_id: &str,
        tier: Tier,
        args: serde_json::Value,
        mode: ElicitationMode,
    ) -> Self {
        let pretty = serde_json::to_string_pretty(&args).unwrap_or_else(|_| args.to_string());
        Self {
            mode,
            tool_id: tool_id.to_owned(),
            tier,
            prompt: format!(
                "Approve `{tool}` ({tier})?\n\nArguments:\n```json\n{pretty}\n```",
                tool = tool_id,
                tier = tier,
                pretty = pretty
            ),
            form_fields: vec![FormField {
                name: "approve".to_owned(),
                kind: FormFieldKind::Bool,
                description: "Approve this action".to_owned(),
            }],
            url: None,
        }
    }
}

/// Field in a form-mode request.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FormField {
    /// Field name / key.
    pub name: String,
    /// Logical type.
    pub kind: FormFieldKind,
    /// User-facing description.
    pub description: String,
}

/// Supported form field kinds.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum FormFieldKind {
    /// Boolean (approve / deny).
    Bool,
    /// Integer choice (1..N).
    Integer,
    /// Short free-text (non-secret).
    ShortText,
    /// Long free-text (non-secret).
    LongText,
}

/// User response to an elicitation.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ElicitationResponse {
    /// Whether the action is allowed.
    pub approved: bool,
    /// Optional free-text feedback.
    pub feedback: Option<String>,
    /// For URL mode: a token the caller expects to receive back (e.g. OAuth code).
    pub url_payload: Option<serde_json::Value>,
}

impl ElicitationResponse {
    /// A plain approval without any extra payload.
    pub fn approve() -> Self {
        Self {
            approved: true,
            feedback: None,
            url_payload: None,
        }
    }
    /// A plain denial.
    pub fn deny() -> Self {
        Self {
            approved: false,
            feedback: None,
            url_payload: None,
        }
    }
}

/// Elicitation errors.
#[derive(Debug, thiserror::Error)]
pub enum ElicitError {
    /// A form-mode request carried a secret-shaped field. REJECTED.
    #[error("form-mode elicitation may not request secret fields: {0}")]
    SecretInForm(String),
    /// The handler has no attached UI (headless without a mock).
    #[error("no elicitation UI attached")]
    NoUiAttached,
    /// The URL-mode flow failed externally.
    #[error("url elicitation failed: {0}")]
    UrlFailure(String),
    /// Malformed request.
    #[error("malformed request: {0}")]
    Malformed(String),
}

/// Secret-name regex list. Fields whose `name` matches any of these are
/// forbidden in `form` mode and must be routed through `url` mode.
///
/// The list is compile-time constant per ADR-0011 §2.3.
static SECRET_NAMES: &[&str] = &[
    r"(?i)password",
    r"(?i)api[_-]?key",
    r"(?i)secret",
    r"(?i)token",
    r"(?i)passphrase",
    r"(?i)credential",
    r"(?i)ssn",
    r"(?i)card[_-]?number",
    r"(?i)cvv",
    r"(?i)auth[_-]?code",
    r"(?i)bearer",
];

static SECRET_RE: Lazy<Regex> = Lazy::new(|| {
    let joined = SECRET_NAMES.join("|");
    // Safe at startup; we build a known-good regex from a static list.
    #[allow(clippy::unwrap_used)]
    Regex::new(&format!("({joined})")).unwrap()
});

/// Validate a prospective elicitation request. Rejects form-mode secrets.
pub fn validate_form_request(req: &ElicitationRequest) -> Result<(), ElicitError> {
    if req.mode != ElicitationMode::Form {
        return Ok(());
    }
    for field in &req.form_fields {
        if SECRET_RE.is_match(&field.name) {
            return Err(ElicitError::SecretInForm(field.name.clone()));
        }
    }
    Ok(())
}

/// Pluggable UI handler. `Governance` holds one of these.
pub trait ElicitationHandler {
    /// Present the request to the user; return a response.
    fn elicit(&mut self, req: &ElicitationRequest) -> Result<ElicitationResponse, ElicitError>;
}

/// Headless test handler — canned responses.
pub struct StaticHandler {
    approve: bool,
    calls: usize,
}

impl StaticHandler {
    /// Always returns approve.
    pub fn always_approve() -> Self {
        Self { approve: true, calls: 0 }
    }

    /// Always returns deny.
    pub fn always_deny() -> Self {
        Self { approve: false, calls: 0 }
    }

    /// Number of times `elicit` was called.
    pub fn call_count(&self) -> usize {
        self.calls
    }
}

impl ElicitationHandler for StaticHandler {
    fn elicit(&mut self, req: &ElicitationRequest) -> Result<ElicitationResponse, ElicitError> {
        self.calls += 1;
        validate_form_request(req)?;
        Ok(if self.approve {
            ElicitationResponse::approve()
        } else {
            ElicitationResponse::deny()
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn form_mode_rejects_secret_fields() {
        let req = ElicitationRequest {
            mode: ElicitationMode::Form,
            tool_id: "bad.tool".into(),
            tier: Tier::Mutate,
            prompt: "no".into(),
            form_fields: vec![FormField {
                name: "api_key".into(),
                kind: FormFieldKind::ShortText,
                description: "".into(),
            }],
            url: None,
        };
        let r = validate_form_request(&req);
        assert!(matches!(r, Err(ElicitError::SecretInForm(_))));
    }

    #[test]
    fn secret_regex_case_insensitive() {
        for name in [
            "API_KEY",
            "apiKey",
            "PASSWORD",
            "userSecret",
            "auth_code",
            "BearerToken",
            "cvv",
            "ssn",
        ] {
            assert!(SECRET_RE.is_match(name), "{} should match", name);
        }
    }

    #[test]
    fn secret_regex_does_not_match_innocent_names() {
        for name in ["name", "label", "count", "entity_id", "position", "radius"] {
            assert!(!SECRET_RE.is_match(name), "{} should not match", name);
        }
    }

    #[test]
    fn url_mode_accepts_any_fields() {
        let req = ElicitationRequest {
            mode: ElicitationMode::Url,
            tool_id: "good.tool".into(),
            tier: Tier::Mutate,
            prompt: "enter key in browser".into(),
            form_fields: vec![FormField {
                name: "api_key".into(),
                kind: FormFieldKind::ShortText,
                description: "".into(),
            }],
            url: Some("https://example/".into()),
        };
        validate_form_request(&req).expect("url mode passes through");
    }

    #[test]
    fn handler_call_count_increments() {
        let mut h = StaticHandler::always_approve();
        let req = ElicitationRequest::confirm(
            "t",
            Tier::Mutate,
            serde_json::json!({}),
            ElicitationMode::Form,
        );
        h.elicit(&req).unwrap();
        h.elicit(&req).unwrap();
        assert_eq!(h.call_count(), 2);
    }
}
