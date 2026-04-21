//! `bld-agent` — the Plan-Act-Verify loop (ADR-0014).
//!
//! The loop drives an `ILLMProvider` through a bounded sequence of turns:
//!
//! 1. **Plan**: the model receives the user prompt, system prompt, tool
//!    catalog, and recent context; it emits either a plain assistant
//!    message or a tool-call request.
//! 2. **Act**: for each tool call, the agent runs the governance gate,
//!    dispatches the tool, and records the result to the audit log.
//! 3. **Verify**: after acting, the agent sends the tool result back to
//!    the model so it can check the invariant it's been told to check
//!    (per-step verification) and decide whether to continue or finish.
//!
//! Guardrails (ADR-0014 §2.4):
//! - Hard cap on total steps per turn (default 16).
//! - Hard wall-clock budget (default 120 s).
//! - Early exit if the model says `end_turn` or the governance layer
//!   denies.
//!
//! This Phase 9A file lands the skeleton: types, budget enforcement, and
//! a synchronous "one-pass" driver that exercises all the wiring without
//! requiring a real provider. Wave 9D replaces the stub tool-dispatcher
//! with the full registry + hot-reload flow.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

pub mod bridge_dispatch;
pub mod build_tools;
pub mod composite;
pub mod tools_impl;
pub use bridge_dispatch::{
    BridgeCall, BridgeDispatcher, CommandEntry, CommandStack, HostBridge, MockHostBridge,
};
pub use composite::CompositeDispatcher;
pub use tools_impl::{FileLoader, FsFileLoader, RagDispatcher};

use std::sync::Arc;
use std::time::{Duration, Instant};

use bld_governance::{
    audit::{AuditLog, TierDecision, ToolOutcome},
    tier::{Governance, GovernanceDecision, Tier},
};
use bld_provider::{
    ChatMessage, ChatRequest, ILLMProvider, ProviderError, Role, ToolCallEvent, ToolSpec,
};
use bld_tools::registry;
use serde::{Deserialize, Serialize};

/// Agent budget + behaviour knobs (ADR-0014 §2.4).
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AgentBudget {
    /// Hard cap on steps (tool calls) per turn.
    pub max_steps: u32,
    /// Hard wall-clock budget per turn.
    pub max_wallclock: Duration,
    /// Max tokens the provider is allowed to return per step.
    pub max_output_tokens: u32,
}

impl Default for AgentBudget {
    fn default() -> Self {
        Self {
            max_steps: 16,
            max_wallclock: Duration::from_secs(120),
            max_output_tokens: 4096,
        }
    }
}

/// Error type raised by the agent loop.
#[derive(Debug, thiserror::Error)]
pub enum AgentError {
    /// The provider returned an error that cannot be retried here.
    #[error("provider: {0}")]
    Provider(#[from] ProviderError),
    /// Governance denied a tool call and the user elected to stop.
    #[error("governance denied: {0}")]
    Denied(String),
    /// Step cap exceeded before `end_turn`.
    #[error("step cap exceeded")]
    StepCap,
    /// Wall-clock budget exhausted.
    #[error("time budget exceeded")]
    TimeBudget,
    /// Audit log failed.
    #[error("audit: {0}")]
    Audit(#[from] bld_governance::audit::AuditError),
    /// Tool not registered in the inventory.
    #[error("unknown tool: {0}")]
    UnknownTool(String),
}

/// A trait for dispatching tool calls. Wave 9D lands the real impl over
/// the `#[bld_tool]` registry + C++ bridge; here we use a stub for tests.
pub trait ToolDispatcher: Send + Sync {
    /// Run a tool and return its JSON result.
    fn dispatch(
        &self,
        tool_id: &str,
        args: &serde_json::Value,
    ) -> Result<serde_json::Value, String>;
}

/// Stub dispatcher — every call returns `{"ok": true}`. Useful for unit
/// tests where the provider already returned a scripted tool call and
/// we just want the loop to run to end_turn.
#[derive(Debug, Default)]
pub struct StubDispatcher;

impl ToolDispatcher for StubDispatcher {
    fn dispatch(
        &self,
        _tool_id: &str,
        _args: &serde_json::Value,
    ) -> Result<serde_json::Value, String> {
        Ok(serde_json::json!({"ok": true}))
    }
}

/// One turn's transcript.
#[derive(Debug, Default, Clone, Serialize, Deserialize)]
pub struct TurnTranscript {
    /// ULID of the turn.
    pub turn_id: String,
    /// The messages exchanged, oldest first.
    pub messages: Vec<ChatMessage>,
    /// Tool calls emitted this turn.
    pub tool_calls: Vec<ExecutedToolCall>,
    /// Whether the turn ended normally (`end_turn`).
    pub completed: bool,
    /// How many steps the turn consumed.
    pub steps_used: u32,
    /// Total wall-clock elapsed.
    pub elapsed_ms: u64,
}

/// Record of an executed tool call.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExecutedToolCall {
    /// Tool id.
    pub tool_id: String,
    /// Arguments.
    pub args: serde_json::Value,
    /// Result JSON (if successful).
    pub result: Option<serde_json::Value>,
    /// Error string (if failed).
    pub error: Option<String>,
    /// The governance decision that was recorded.
    pub tier_decision: TierDecision,
}

/// The main driver.
pub struct Agent {
    /// LLM provider.
    pub provider: Arc<dyn ILLMProvider>,
    /// Tool dispatcher.
    pub dispatcher: Arc<dyn ToolDispatcher>,
    /// Governance layer (authorization + elicitation).
    pub governance: Arc<std::sync::Mutex<Governance>>,
    /// Audit log.
    pub audit: Arc<std::sync::Mutex<AuditLog>>,
    /// System prompt.
    pub system_prompt: String,
    /// Budget.
    pub budget: AgentBudget,
    /// Model id to request on each turn.
    pub model: String,
}

impl Agent {
    /// Build a new agent.
    pub fn new(
        provider: Arc<dyn ILLMProvider>,
        dispatcher: Arc<dyn ToolDispatcher>,
        governance: Arc<std::sync::Mutex<Governance>>,
        audit: Arc<std::sync::Mutex<AuditLog>>,
        model: impl Into<String>,
    ) -> Self {
        Self {
            provider,
            dispatcher,
            governance,
            audit,
            system_prompt: DEFAULT_SYSTEM_PROMPT.into(),
            budget: AgentBudget::default(),
            model: model.into(),
        }
    }

    /// Run one turn: user message -> tool calls -> final reply.
    ///
    /// Returns the transcript. On budget exhaustion, returns the
    /// partial transcript alongside `AgentError::StepCap` /
    /// `AgentError::TimeBudget`.
    pub async fn run_turn(
        &self,
        user_prompt: &str,
    ) -> Result<TurnTranscript, (AgentError, TurnTranscript)> {
        let start = Instant::now();
        let turn_id = ulid::Ulid::new().to_string();
        let mut transcript = TurnTranscript {
            turn_id: turn_id.clone(),
            messages: vec![ChatMessage {
                role: Role::User,
                content: user_prompt.to_owned(),
                tool_call_id: None,
                tool_name: None,
            }],
            ..Default::default()
        };

        let tools = self.build_tool_catalog();

        for step in 0..self.budget.max_steps {
            if start.elapsed() > self.budget.max_wallclock {
                transcript.elapsed_ms = start.elapsed().as_millis() as u64;
                return Err((AgentError::TimeBudget, transcript));
            }
            transcript.steps_used = step + 1;

            let req = ChatRequest {
                model: self.model.clone(),
                system: Some(self.system_prompt.clone()),
                messages: transcript.messages.clone(),
                tools: tools.clone(),
                temperature: Some(0.0),
                max_tokens: Some(self.budget.max_output_tokens),
                stream: false,
                structured_output: None,
            };

            let resp = match self.provider.complete(req).await {
                Ok(r) => r,
                Err(e) => {
                    transcript.elapsed_ms = start.elapsed().as_millis() as u64;
                    return Err((AgentError::Provider(e), transcript));
                }
            };

            // Record the assistant message.
            transcript.messages.push(ChatMessage {
                role: Role::Assistant,
                content: resp.content.clone(),
                tool_call_id: None,
                tool_name: None,
            });

            if resp.tool_calls.is_empty() {
                // Model is done.
                transcript.completed = true;
                transcript.elapsed_ms = start.elapsed().as_millis() as u64;
                return Ok(transcript);
            }

            // Act on each tool call. Early-exit on denial.
            for call in &resp.tool_calls {
                match self.handle_tool_call(call, &turn_id, &mut transcript).await {
                    Ok(()) => {}
                    Err(e) => {
                        transcript.elapsed_ms = start.elapsed().as_millis() as u64;
                        return Err((e, transcript));
                    }
                }
            }
        }

        transcript.elapsed_ms = start.elapsed().as_millis() as u64;
        Err((AgentError::StepCap, transcript))
    }

    fn build_tool_catalog(&self) -> Vec<ToolSpec> {
        registry::all()
            .map(|d| ToolSpec {
                name: d.id.to_owned(),
                description: d.summary.to_owned(),
                input_schema: serde_json::json!({
                    "type": "object",
                    "additionalProperties": true
                }),
            })
            .collect()
    }

    async fn handle_tool_call(
        &self,
        call: &ToolCallEvent,
        turn_id: &str,
        transcript: &mut TurnTranscript,
    ) -> Result<(), AgentError> {
        let desc = registry::find(&call.name)
            .ok_or_else(|| AgentError::UnknownTool(call.name.clone()))?;

        // Governance gate.
        let decision = {
            #[allow(clippy::expect_used)]
            let mut g = self.governance.lock().expect("governance mutex poisoned");
            g.require(desc.id, desc.tier, &call.arguments)
                .map_err(|e| AgentError::Denied(e.to_string()))?
        };

        let tier_decision = TierDecision::from(decision.clone());
        if matches!(decision, GovernanceDecision::Denied) {
            // Record the denial, surface a Tool message, keep going so
            // the model can adapt.
            self.record_audit(
                turn_id,
                desc.id,
                desc.tier,
                &tier_decision,
                &call.arguments,
                &serde_json::Value::Null,
                0,
                ToolOutcome::Denied,
                Some("user denied".into()),
            )?;
            transcript.tool_calls.push(ExecutedToolCall {
                tool_id: call.name.clone(),
                args: call.arguments.clone(),
                result: None,
                error: Some("denied".into()),
                tier_decision,
            });
            transcript.messages.push(ChatMessage {
                role: Role::Tool,
                content: "denied by user".into(),
                tool_call_id: Some(call.id.clone()),
                tool_name: Some(call.name.clone()),
            });
            return Ok(());
        }

        let t0 = Instant::now();
        let result = self.dispatcher.dispatch(&call.name, &call.arguments);
        let elapsed = t0.elapsed().as_millis() as u64;

        match result {
            Ok(v) => {
                self.record_audit(
                    turn_id,
                    desc.id,
                    desc.tier,
                    &tier_decision,
                    &call.arguments,
                    &v,
                    elapsed,
                    ToolOutcome::Ok,
                    None,
                )?;
                transcript.tool_calls.push(ExecutedToolCall {
                    tool_id: call.name.clone(),
                    args: call.arguments.clone(),
                    result: Some(v.clone()),
                    error: None,
                    tier_decision,
                });
                transcript.messages.push(ChatMessage {
                    role: Role::Tool,
                    content: serde_json::to_string(&v).unwrap_or_default(),
                    tool_call_id: Some(call.id.clone()),
                    tool_name: Some(call.name.clone()),
                });
            }
            Err(e) => {
                self.record_audit(
                    turn_id,
                    desc.id,
                    desc.tier,
                    &tier_decision,
                    &call.arguments,
                    &serde_json::Value::Null,
                    elapsed,
                    ToolOutcome::Err,
                    Some(e.clone()),
                )?;
                transcript.tool_calls.push(ExecutedToolCall {
                    tool_id: call.name.clone(),
                    args: call.arguments.clone(),
                    result: None,
                    error: Some(e.clone()),
                    tier_decision,
                });
                transcript.messages.push(ChatMessage {
                    role: Role::Tool,
                    content: format!("error: {e}"),
                    tool_call_id: Some(call.id.clone()),
                    tool_name: Some(call.name.clone()),
                });
            }
        }

        Ok(())
    }

    #[allow(clippy::too_many_arguments)]
    fn record_audit(
        &self,
        turn_id: &str,
        tool_id: &str,
        tier: Tier,
        tier_decision: &TierDecision,
        args: &serde_json::Value,
        result: &serde_json::Value,
        elapsed_ms: u64,
        outcome: ToolOutcome,
        error: Option<String>,
    ) -> Result<(), AgentError> {
        #[allow(clippy::expect_used)]
        let mut log = self.audit.lock().expect("audit mutex poisoned");
        log.append(
            turn_id,
            tool_id,
            tier,
            tier_decision.clone(),
            args,
            result,
            elapsed_ms,
            None,
            outcome,
            error,
        )?;
        Ok(())
    }
}

/// Default BLD system prompt (ADR-0014 §2.1). Kept short so every token
/// left in the context budget goes to retrieved chunks and the user
/// conversation.
pub const DEFAULT_SYSTEM_PROMPT: &str = "\
You are Brewed Logic Directive (BLD), the native AI copilot for the \
Greywater_Engine editor. You edit scenes, code, and build configuration \
via structured tool calls. Keep responses terse, cite file paths and \
line numbers when helpful, and call tools eagerly when they would let \
you verify your plan.";

#[cfg(test)]
mod tests {
    use super::*;
    use bld_governance::tier::Governance;
    use bld_governance::elicitation::StaticHandler;
    use bld_provider::mock::{MockProvider, ScriptStep};
    use bld_provider::ChatResponse;

    fn make_agent(provider: Arc<dyn ILLMProvider>) -> (Agent, tempfile::TempDir) {
        let dir = tempfile::tempdir().unwrap();
        let audit = AuditLog::open(dir.path(), "sess-agent").unwrap();
        let gov = Governance::new(Box::new(StaticHandler::always_approve()));
        (
            Agent::new(
                provider,
                Arc::new(StubDispatcher),
                Arc::new(std::sync::Mutex::new(gov)),
                Arc::new(std::sync::Mutex::new(audit)),
                "mock-default",
            ),
            dir,
        )
    }

    #[tokio::test]
    async fn agent_ends_turn_when_model_stops() {
        let p = Arc::new(MockProvider::new());
        p.push_text("all done");
        let (agent, _dir) = make_agent(p);
        let t = agent.run_turn("hello").await.unwrap();
        assert!(t.completed);
        assert_eq!(t.steps_used, 1);
        assert_eq!(t.tool_calls.len(), 0);
    }

    #[tokio::test]
    async fn agent_runs_tool_then_ends() {
        let p = Arc::new(MockProvider::new());
        // Step 1: model asks for tool.
        p.push(ScriptStep::Reply(ChatResponse {
            model: "mock-default".into(),
            content: "thinking".into(),
            tool_calls: vec![ToolCallEvent {
                id: "c1".into(),
                name: "project.list_scenes".into(),
                arguments: serde_json::json!({}),
            }],
            stop_reason: Some("tool_use".into()),
            input_tokens: None,
            output_tokens: None,
        }));
        // Step 2: model wraps up.
        p.push_text("done");
        let (agent, _dir) = make_agent(p);
        let t = agent.run_turn("list scenes").await.unwrap();
        assert!(t.completed);
        assert_eq!(t.tool_calls.len(), 1);
        assert_eq!(t.tool_calls[0].tool_id, "project.list_scenes");
    }

    #[tokio::test]
    async fn agent_respects_step_cap() {
        let p = Arc::new(MockProvider::new());
        // Every step asks for another tool => never ends.
        for i in 0..4 {
            p.push(ScriptStep::Reply(ChatResponse {
                model: "mock-default".into(),
                content: format!("step {i}"),
                tool_calls: vec![ToolCallEvent {
                    id: format!("c{i}"),
                    name: "project.list_scenes".into(),
                    arguments: serde_json::json!({}),
                }],
                stop_reason: Some("tool_use".into()),
                input_tokens: None,
                output_tokens: None,
            }));
        }
        let (mut agent, _dir) = make_agent(p);
        agent.budget.max_steps = 3;
        let err = agent.run_turn("loop").await.unwrap_err();
        let (e, transcript) = err;
        assert!(matches!(e, AgentError::StepCap));
        assert_eq!(transcript.steps_used, 3);
        assert_eq!(transcript.tool_calls.len(), 3);
    }

    #[tokio::test]
    async fn unknown_tool_short_circuits() {
        let p = Arc::new(MockProvider::new());
        p.push(ScriptStep::Reply(ChatResponse {
            model: "mock-default".into(),
            content: "call something".into(),
            tool_calls: vec![ToolCallEvent {
                id: "c".into(),
                name: "nope.does_not_exist".into(),
                arguments: serde_json::json!({}),
            }],
            stop_reason: Some("tool_use".into()),
            input_tokens: None,
            output_tokens: None,
        }));
        let (agent, _dir) = make_agent(p);
        let err = agent.run_turn("x").await.unwrap_err();
        assert!(matches!(err.0, AgentError::UnknownTool(_)));
    }
}
