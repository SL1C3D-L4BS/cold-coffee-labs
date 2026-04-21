//! Slash-command parser + dispatcher (ADR-0015 §2.6 — governance UX).
//!
//! The agent panel treats any chat message whose first non-whitespace
//! character is `/` as a slash command. Slash commands are routed through
//! this module instead of reaching the LLM.
//!
//! Supported commands:
//!
//! | Command              | Effect                                               |
//! |----------------------|------------------------------------------------------|
//! | `/grant <tool>`      | Always-approve `<tool>` this session.                |
//! | `/revoke <tool>`     | Remove an always-approve grant.                      |
//! | `/reset`             | Clear the approval cache + grants.                   |
//! | `/offline [on\|off]` | Toggle (or set) offline mode.                        |
//! | `/model <name>`      | Switch the default LLM model for this session.       |
//! | `/sessions`          | List active sessions (handled by the SessionManager).|
//! | `/help`              | List available commands.                             |
//!
//! Each command parses into a [`SlashCommand`]; the agent applies it via
//! [`SlashDispatcher`] against a [`Governance`][crate::Governance] plus a
//! [`SessionContext`]. Unknown commands return an `Unknown` variant so
//! the UI can surface a helpful error.

use std::collections::BTreeMap;

use serde::{Deserialize, Serialize};

/// Parsed slash command.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum SlashCommand {
    /// `/grant <tool>`
    Grant(String),
    /// `/revoke <tool>`
    Revoke(String),
    /// `/reset`
    Reset,
    /// `/offline` (toggle), `/offline on`, `/offline off`
    Offline(Option<bool>),
    /// `/model <name>`
    Model(String),
    /// `/sessions`
    Sessions,
    /// `/help`
    Help,
    /// Anything else.
    Unknown {
        /// Raw command word (without the leading `/`).
        head: String,
        /// Remaining argv.
        args: Vec<String>,
    },
}

/// Result of executing a slash command.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SlashResult {
    /// Human-readable message for the chat UI.
    pub message: String,
    /// If this command mutated governance, `true`; useful for
    /// cache-invalidation hints.
    pub mutated: bool,
}

/// Try to parse a line as a slash command.
///
/// Returns `None` if the input doesn't start with `/`. Unknown commands
/// still parse successfully (as [`SlashCommand::Unknown`]); the caller
/// decides the UX.
pub fn parse(line: &str) -> Option<SlashCommand> {
    let trimmed = line.trim_start();
    let body = trimmed.strip_prefix('/')?;
    let mut toks = body.split_whitespace();
    let head = toks.next()?.to_owned();
    let args: Vec<String> = toks.map(str::to_owned).collect();
    Some(classify(&head, &args))
}

fn classify(head: &str, args: &[String]) -> SlashCommand {
    match head {
        "grant" => match args.first() {
            Some(t) => SlashCommand::Grant(t.clone()),
            None => SlashCommand::Unknown {
                head: head.into(),
                args: args.to_vec(),
            },
        },
        "revoke" => match args.first() {
            Some(t) => SlashCommand::Revoke(t.clone()),
            None => SlashCommand::Unknown {
                head: head.into(),
                args: args.to_vec(),
            },
        },
        "reset" => SlashCommand::Reset,
        "offline" => {
            let v = match args.first().map(String::as_str) {
                Some("on") | Some("true") | Some("1") => Some(true),
                Some("off") | Some("false") | Some("0") => Some(false),
                Some(_) => {
                    return SlashCommand::Unknown {
                        head: head.into(),
                        args: args.to_vec(),
                    }
                }
                None => None,
            };
            SlashCommand::Offline(v)
        }
        "model" => match args.first() {
            Some(m) => SlashCommand::Model(m.clone()),
            None => SlashCommand::Unknown {
                head: head.into(),
                args: args.to_vec(),
            },
        },
        "sessions" => SlashCommand::Sessions,
        "help" | "?" => SlashCommand::Help,
        _ => SlashCommand::Unknown {
            head: head.into(),
            args: args.to_vec(),
        },
    }
}

/// Minimal session-visible state that slash commands can tweak.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct SessionContext {
    /// Offline-mode flag.
    pub offline: bool,
    /// Currently-selected model id.
    pub model: String,
    /// Arbitrary session metadata surfaced by `/sessions`.
    pub metadata: BTreeMap<String, String>,
}

/// Dispatcher that interprets [`SlashCommand`]s against a
/// [`Governance`][crate::Governance] + [`SessionContext`].
#[derive(Debug)]
pub struct SlashDispatcher;

impl SlashDispatcher {
    /// Apply the command, mutating `governance` and `ctx` as appropriate.
    pub fn dispatch(
        command: &SlashCommand,
        governance: &mut crate::Governance,
        ctx: &mut SessionContext,
    ) -> SlashResult {
        match command {
            SlashCommand::Grant(tool) => {
                governance.grant(tool);
                SlashResult {
                    message: format!("granted: {tool}"),
                    mutated: true,
                }
            }
            SlashCommand::Revoke(tool) => {
                governance.revoke(tool);
                SlashResult {
                    message: format!("revoked: {tool}"),
                    mutated: true,
                }
            }
            SlashCommand::Reset => {
                governance.reset();
                SlashResult {
                    message: "governance reset".into(),
                    mutated: true,
                }
            }
            SlashCommand::Offline(v) => {
                ctx.offline = v.unwrap_or(!ctx.offline);
                SlashResult {
                    message: if ctx.offline {
                        "offline mode: ON".into()
                    } else {
                        "offline mode: OFF".into()
                    },
                    mutated: true,
                }
            }
            SlashCommand::Model(m) => {
                ctx.model = m.clone();
                SlashResult {
                    message: format!("model switched to {m}"),
                    mutated: true,
                }
            }
            SlashCommand::Sessions => SlashResult {
                message: "session list: use SessionManager::list".into(),
                mutated: false,
            },
            SlashCommand::Help => SlashResult {
                message: HELP_TEXT.into(),
                mutated: false,
            },
            SlashCommand::Unknown { head, args } => SlashResult {
                message: format!(
                    "unknown command `/{head}`{}. Type `/help`.",
                    if args.is_empty() {
                        String::new()
                    } else {
                        format!(" (args: {})", args.join(" "))
                    }
                ),
                mutated: false,
            },
        }
    }
}

const HELP_TEXT: &str = "\
Slash commands:
  /grant <tool>    always-approve <tool> in this session
  /revoke <tool>   remove an /grant
  /reset           clear approval cache and grants
  /offline [on|off] toggle offline mode (no cloud LLM)
  /model <name>    switch the active model
  /sessions        list active sessions
  /help            show this text";

#[cfg(test)]
mod tests {
    use super::*;
    use crate::elicitation::StaticHandler;
    use crate::Governance;

    #[test]
    fn parse_returns_none_on_non_slash() {
        assert!(parse("hello world").is_none());
        assert!(parse("   ").is_none());
    }

    #[test]
    fn parse_grant_and_revoke() {
        assert_eq!(
            parse("/grant scene.create_entity").unwrap(),
            SlashCommand::Grant("scene.create_entity".into())
        );
        assert_eq!(
            parse("/revoke scene.create_entity").unwrap(),
            SlashCommand::Revoke("scene.create_entity".into())
        );
    }

    #[test]
    fn parse_offline_variants() {
        assert_eq!(parse("/offline").unwrap(), SlashCommand::Offline(None));
        assert_eq!(parse("/offline on").unwrap(), SlashCommand::Offline(Some(true)));
        assert_eq!(parse("/offline off").unwrap(), SlashCommand::Offline(Some(false)));
        assert_eq!(parse("/offline true").unwrap(), SlashCommand::Offline(Some(true)));
        // Malformed value => Unknown
        match parse("/offline maybe").unwrap() {
            SlashCommand::Unknown { head, .. } => assert_eq!(head, "offline"),
            other => panic!("expected Unknown, got {other:?}"),
        }
    }

    #[test]
    fn parse_model_reset_help() {
        assert_eq!(
            parse("/model claude-3.5").unwrap(),
            SlashCommand::Model("claude-3.5".into())
        );
        assert_eq!(parse("/reset").unwrap(), SlashCommand::Reset);
        assert_eq!(parse("/help").unwrap(), SlashCommand::Help);
        assert_eq!(parse("/?").unwrap(), SlashCommand::Help);
    }

    #[test]
    fn parse_unknown_surfaces_head_and_args() {
        match parse("/teleport home").unwrap() {
            SlashCommand::Unknown { head, args } => {
                assert_eq!(head, "teleport");
                assert_eq!(args, vec!["home".to_string()]);
            }
            other => panic!("expected Unknown, got {other:?}"),
        }
    }

    #[test]
    fn dispatch_grant_and_revoke_mutate_governance() {
        let mut g = Governance::new(Box::new(StaticHandler::always_deny()));
        let mut ctx = SessionContext::default();

        let r1 = SlashDispatcher::dispatch(
            &SlashCommand::Grant("scene.create_entity".into()),
            &mut g,
            &mut ctx,
        );
        assert!(r1.mutated);

        // Now the tool should be granted.
        let d = g
            .require(
                "scene.create_entity",
                crate::Tier::Mutate,
                &serde_json::json!({}),
            )
            .unwrap();
        assert_eq!(d, crate::GovernanceDecision::ApprovedGranted);

        SlashDispatcher::dispatch(
            &SlashCommand::Revoke("scene.create_entity".into()),
            &mut g,
            &mut ctx,
        );
        let d2 = g
            .require(
                "scene.create_entity",
                crate::Tier::Mutate,
                &serde_json::json!({}),
            )
            .unwrap();
        assert!(!d2.is_allowed());
    }

    #[test]
    fn dispatch_offline_toggles() {
        let mut g = Governance::new(Box::new(StaticHandler::always_approve()));
        let mut ctx = SessionContext::default();
        assert!(!ctx.offline);

        SlashDispatcher::dispatch(&SlashCommand::Offline(None), &mut g, &mut ctx);
        assert!(ctx.offline);

        SlashDispatcher::dispatch(&SlashCommand::Offline(Some(false)), &mut g, &mut ctx);
        assert!(!ctx.offline);
    }

    #[test]
    fn dispatch_model_switches_and_reset_clears() {
        // Use the always-deny handler so we can observe that /reset
        // actually cleared the grant (otherwise the model switch
        // assertion would pass vacuously).
        let mut g = Governance::new(Box::new(StaticHandler::always_deny()));
        let mut ctx = SessionContext::default();

        SlashDispatcher::dispatch(
            &SlashCommand::Model("claude-opus-4".into()),
            &mut g,
            &mut ctx,
        );
        assert_eq!(ctx.model, "claude-opus-4");

        g.grant("x");
        // Before reset: granted.
        let before = g
            .require("x", crate::Tier::Mutate, &serde_json::json!({}))
            .unwrap();
        assert_eq!(before, crate::GovernanceDecision::ApprovedGranted);

        SlashDispatcher::dispatch(&SlashCommand::Reset, &mut g, &mut ctx);

        // After reset: grant gone, handler denies, so Mutate is Denied.
        let after = g
            .require("x", crate::Tier::Mutate, &serde_json::json!({}))
            .unwrap();
        assert!(!after.is_allowed());
    }

    #[test]
    fn help_message_lists_commands() {
        let mut g = Governance::new(Box::new(StaticHandler::always_approve()));
        let mut ctx = SessionContext::default();
        let r = SlashDispatcher::dispatch(&SlashCommand::Help, &mut g, &mut ctx);
        assert!(r.message.contains("/grant"));
        assert!(r.message.contains("/offline"));
        assert!(!r.mutated);
    }
}
