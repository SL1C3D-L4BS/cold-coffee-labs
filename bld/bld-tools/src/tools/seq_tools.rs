//! Sequencer BLD tools — opcode bridge to `dispatch_run_command` (Phase 18-B).

use bld_bridge::run_command;
use bld_ffi::bld_ffi_seq_export_json_clone;

use crate::bld_tool;
use serde::Serialize;

fn push_u32_le(out: &mut Vec<u8>, v: u32) {
    out.extend_from_slice(&v.to_le_bytes());
}

fn push_u64_le(out: &mut Vec<u8>, v: u64) {
    out.extend_from_slice(&v.to_le_bytes());
}

fn payload_create(frame_rate: u32, duration_frames: u32, name: &str) -> Vec<u8> {
    let mut v = Vec::with_capacity(8 + name.len() + 1);
    push_u32_le(&mut v, frame_rate);
    push_u32_le(&mut v, duration_frames);
    v.extend_from_slice(name.as_bytes());
    v.push(0);
    v
}

fn payload_u64_u32(seq_handle: u64, track_id: u32) -> Vec<u8> {
    let mut v = Vec::with_capacity(12);
    push_u64_le(&mut v, seq_handle);
    push_u32_le(&mut v, track_id);
    v
}

fn payload_u64_u8(seq_handle: u64, flag: u8) -> Vec<u8> {
    let mut v = Vec::with_capacity(9);
    push_u64_le(&mut v, seq_handle);
    v.push(flag);
    v
}

fn payload_u64_only(seq_handle: u64) -> Vec<u8> {
    let mut v = Vec::with_capacity(8);
    push_u64_le(&mut v, seq_handle);
    v
}

/// `seq.create_sequence` — opcode `0x0001`.
#[bld_tool(
    id = "seq.create_sequence",
    tier = "Mutate",
    summary = "Create an empty .gwseq in the sequencer; returns synthetic sequence handle bits."
)]
fn seq_create_sequence(
    args: CreateSeqArgs,
) -> Result<CreateSeqOut, String> {
    let p = payload_create(args.frame_rate, args.duration_frames, &args.name);
    let id = run_command(0x0001, &p).map_err(|e| e.to_string())?;
    Ok(CreateSeqOut { seq_handle: id })
}

#[derive(serde::Deserialize)]
struct CreateSeqArgs {
    frame_rate: u32,
    duration_frames: u32,
    #[serde(default = "default_name")]
    name: String,
}

fn default_name() -> String {
    "sequence.gwseq".to_owned()
}

#[derive(Serialize)]
struct CreateSeqOut {
    seq_handle: u64,
}

/// `seq.add_transform_track` — opcode `0x0002`.
#[bld_tool(
    id = "seq.add_transform_track",
    tier = "Mutate",
    summary = "Append an empty position transform track; returns new track id (pass track_id 0 for auto)."
)]
fn seq_add_transform_track(
    args: AddTrackArgs,
) -> Result<AddTrackOut, String> {
    let p = payload_u64_u32(args.seq_handle, args.track_id);
    let id = run_command(0x0002, &p).map_err(|e| e.to_string())?;
    Ok(AddTrackOut { track_id: id })
}

#[derive(serde::Deserialize)]
struct AddTrackArgs {
    seq_handle: u64,
    #[serde(default)]
    track_id: u32,
}

#[derive(Serialize)]
struct AddTrackOut {
    track_id: u64,
}

/// `seq.add_keyframe` — reserved (returns structured not-implemented).
#[bld_tool(
    id = "seq.add_keyframe",
    tier = "Mutate",
    summary = "Reserved — use the Seq panel / command stack for keyframe edits in Phase 18-B."
)]
fn seq_add_keyframe(_args: serde_json::Value) -> Result<serde_json::Value, String> {
    Err("seq.add_keyframe is not implemented over run_command (use the editor panel)".to_owned())
}

/// `seq.play` — opcode `0x0004`.
#[bld_tool(
    id = "seq.play",
    tier = "Mutate",
    summary = "Assign sequence to the seq player entity and start playback."
)]
fn seq_play(args: PlayArgs) -> Result<PlayOut, String> {
    let p = payload_u64_u8(args.seq_handle, if args.r#loop { 1 } else { 0 });
    let _id = run_command(0x0004, &p).map_err(|e| e.to_string())?;
    Ok(PlayOut { ok: true })
}

#[derive(serde::Deserialize)]
struct PlayArgs {
    seq_handle: u64,
    #[serde(default)]
    r#loop: bool,
}

#[derive(Serialize)]
struct PlayOut {
    ok: bool,
}

/// `seq.export_summary` — opcode `0x0005`, then read JSON from bld-ffi.
#[bld_tool(
    id = "seq.export_summary",
    tier = "Read",
    summary = "Export a JSON summary of the sequence (tracks, keyframe counts) via the host."
)]
fn seq_export_summary(
    args: ExportArgs,
) -> Result<serde_json::Value, String> {
    let p = payload_u64_only(args.seq_handle);
    let _id = run_command(0x0005, &p).map_err(|e| e.to_string())?;
    let s = bld_ffi_seq_export_json_clone();
    if s.is_empty() {
        return Err("export returned empty JSON (is the sequence registered?)".to_owned());
    }
    serde_json::from_str(&s).map_err(|e| format!("parse export JSON: {e}"))
}

#[derive(serde::Deserialize)]
struct ExportArgs {
    seq_handle: u64,
}
