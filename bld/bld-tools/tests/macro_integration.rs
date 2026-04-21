//! Integration test for `#[bld_tool]`.
//!
//! Exercises the attribute macro end-to-end: attribute parsing, descriptor
//! emission, and `inventory::submit` registration. Running with
//! `--features macros` in the bld-tools crate pulls in `bld-tools-macros`
//! and activates the macro.
//!
//! These tests live as an *integration* test (under `tests/`) rather than
//! a `#[cfg(test)]` unit test so they exercise the macro exactly as a
//! downstream crate would use it.

#![cfg(feature = "macros")]

use bld_governance::tier::Tier;
use bld_tools::{bld_tool, registry};

#[bld_tool(
    id = "integration.simple",
    tier = "Read",
    summary = "A simple smoke-test tool."
)]
#[allow(dead_code)]
fn integration_simple(_args: serde_json::Value) -> serde_json::Value {
    serde_json::json!({"ok": true})
}

#[bld_tool(
    id = "integration.mutate",
    tier = "Mutate",
    summary = "A mutating smoke-test tool."
)]
#[allow(dead_code)]
fn integration_mutate(_args: serde_json::Value) -> serde_json::Value {
    serde_json::json!({"changed": 1})
}

#[bld_tool(
    id = "integration.execute",
    tier = "Execute",
    summary = "An executing smoke-test tool."
)]
#[allow(dead_code)]
fn integration_execute(_args: serde_json::Value) -> serde_json::Value {
    serde_json::json!({"ran": true})
}

#[test]
fn macro_emits_descriptors_and_registers_them() {
    let simple = registry::find("integration.simple").expect("descriptor missing");
    assert_eq!(simple.id, "integration.simple");
    assert_eq!(simple.tier, Tier::Read);
    assert!(simple.summary.contains("smoke"));

    let mutate = registry::find("integration.mutate").expect("mutate descriptor missing");
    assert_eq!(mutate.tier, Tier::Mutate);

    let exec = registry::find("integration.execute").expect("execute descriptor missing");
    assert_eq!(exec.tier, Tier::Execute);
}

#[test]
fn macro_registered_tools_appear_in_registry_all() {
    let all_ids: Vec<&str> = registry::all().map(|d| d.id).collect();
    assert!(all_ids.contains(&"integration.simple"));
    assert!(all_ids.contains(&"integration.mutate"));
    assert!(all_ids.contains(&"integration.execute"));
}

#[bld_tool(
    id = "integration.add_one",
    tier = "Read",
    summary = "Add one to an integer input."
)]
#[allow(dead_code)]
fn integration_add_one(args: AddArgs) -> Result<AddOut, String> {
    Ok(AddOut { sum: args.n + 1 })
}

#[derive(serde::Deserialize)]
struct AddArgs {
    n: i64,
}

#[derive(serde::Serialize)]
struct AddOut {
    sum: i64,
}

#[test]
fn macro_emits_dispatch_entry_for_typed_inputs() {
    let entry = registry::find_dispatch("integration.add_one")
        .expect("dispatch entry for integration.add_one missing");
    let out = (entry.dispatch)(&serde_json::json!({"n": 41})).expect("dispatch ok");
    assert_eq!(out.get("sum").and_then(|v| v.as_i64()), Some(42));
}

#[test]
fn macro_emits_dispatch_entry_for_value_passthrough() {
    let entry = registry::find_dispatch("integration.simple")
        .expect("dispatch entry for integration.simple missing");
    let out = (entry.dispatch)(&serde_json::json!({})).expect("dispatch ok");
    assert_eq!(out.get("ok"), Some(&serde_json::json!(true)));
}

#[test]
fn dispatch_count_matches_inventory() {
    let names: Vec<&str> = registry::dispatch_entries().map(|e| e.id).collect();
    assert!(names.contains(&"integration.simple"));
    assert!(names.contains(&"integration.add_one"));
}
