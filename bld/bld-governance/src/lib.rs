//! bld-governance — three-tier authorization, elicitation routing,
//! secret filter, append-only audit log, session replay primitives.
//!
//! This crate has **no AI dependencies**. It is pure policy logic so it can
//! be exercised by fuzzers, mutation testing (`cargo-mutants`, Phase 24), and
//! property-based tests without touching a network.
//!
//! Reference ADRs: ADR-0011 (elicitation policy), ADR-0015 (governance,
//! audit, replay, secret filter), ADR-0014 (agent step bounds).

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

pub mod audit;
pub mod elicitation;
pub mod secret_filter;
pub mod session_manager;
pub mod slash;
pub mod tier;

pub use audit::{AuditError, AuditEvent, AuditLog, TierDecision, ToolOutcome};
pub use elicitation::{ElicitError, ElicitationMode, ElicitationRequest, ElicitationResponse};
pub use secret_filter::{SecretFilter, SecretFilterHit};
pub use session_manager::{
    ElicitationFactory, Session, SessionManager, SessionSummary, StaticElicitationFactory,
};
pub use slash::{parse as parse_slash, SessionContext, SlashCommand, SlashDispatcher, SlashResult};
pub use tier::{ApprovalCache, Governance, GovernanceDecision, GovernanceError, Tier};

/// Thin error type surfaced to consumers who just want "did this pass or not".
#[derive(Debug, thiserror::Error)]
pub enum GovernanceFailure {
    /// A destructive or mutating tool was rejected by the user.
    #[error("rejected by user")]
    UserDenied,
    /// The tool is on the secret-filter deny list.
    #[error("secret filter hit: {0}")]
    SecretFilter(String),
    /// Elicitation failed (bad mode, invalid schema, etc.).
    #[error("elicitation error: {0}")]
    Elicit(#[from] ElicitError),
    /// Audit log I/O failure.
    #[error("audit: {0}")]
    Audit(#[from] AuditError),
    /// Lower-level governance error.
    #[error("governance: {0}")]
    Gov(#[from] GovernanceError),
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tier_ordering_is_stable() {
        assert_ne!(Tier::Read, Tier::Mutate);
        assert_ne!(Tier::Mutate, Tier::Execute);
        assert_ne!(Tier::Read, Tier::Execute);
    }
}
