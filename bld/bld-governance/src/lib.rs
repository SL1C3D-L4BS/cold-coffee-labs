//! bld-governance — three-tier authorization, audit log, elicitation.
//! Phase 9. Phase 1 stub.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

/// Authorization tier. Phase 9 expands this into a real system.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Tier {
    /// Read-only — safe inspection of state.
    Read,
    /// Editable — non-destructive mutation inside the undo stack.
    Edit,
    /// Destructive — requires explicit human confirmation.
    Destructive,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tiers_compare_distinctly() {
        assert_ne!(Tier::Read, Tier::Destructive);
    }
}
