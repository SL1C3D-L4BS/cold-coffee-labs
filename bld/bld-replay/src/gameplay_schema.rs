//! `bld-replay::gameplay_schema` — Part C §22 scaffold (ADR-0015 + ADR-0107b).
//!
//! Gameplay replay schema separate from the existing BLD audit replay. Records
//! input stream + seed + version + per-tick bit-hash and reproduces bitwise
//! on replay. Shareable speedrun files, target ≤ 1 MB each.

use serde::{Deserialize, Serialize};

/// Schema version. Bumped whenever the binary layout changes. Versioned
/// mismatch rejects the replay.
pub const GAMEPLAY_REPLAY_VERSION: u32 = 1;

/// Compact header stamped at the top of a gameplay replay.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GameplayReplayHeader {
    /// `GAMEPLAY_REPLAY_VERSION` at record time.
    pub version: u32,
    /// Engine build SHA (cooked from `engine/core/version.hpp`).
    pub build_sha: String,
    /// Full 128-bit seed: `seed_hi << 64 | seed_lo`.
    pub seed_lo: u64,
    /// Upper 64 bits of the 128-bit seed.
    pub seed_hi: u64,
    /// Initial scene hash for determinism validation.
    pub initial_scene_hash: u64,
    /// Tick rate (Hz). Must match the netcode tick rate (see docs/09).
    pub tick_rate_hz: u32,
    /// ISO-8601 UTC timestamp for humans only. Not part of bit-hash.
    pub recorded_at: String,
}

/// One tick of recorded input + sampled state hash.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GameplayReplayFrame {
    /// Monotonic simulation tick, 0-based.
    pub tick: u64,
    /// Compressed input blob (per-tick action bitfield + analog axes).
    pub input: Vec<u8>,
    /// 64-bit state hash sampled at the end of the tick. Used to abort on
    /// divergence during replay.
    pub state_hash: u64,
}

/// Full replay payload. Serialised with bincode for size; JSON manifest
/// available for tooling via `to_manifest_json`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GameplayReplay {
    /// Replay header.
    pub header: GameplayReplayHeader,
    /// Per-tick frames in sim order.
    pub frames: Vec<GameplayReplayFrame>,
}

impl GameplayReplay {
    /// Verify that every frame is in strictly increasing tick order and
    /// that the header version matches the current schema.
    ///
    /// # Errors
    /// Returns a human-readable message when the version mismatches or
    /// frames are out of order. Callers reject the replay in that case.
    pub fn validate(&self) -> Result<(), String> {
        if self.header.version != GAMEPLAY_REPLAY_VERSION {
            return Err(format!(
                "version {} != {}",
                self.header.version, GAMEPLAY_REPLAY_VERSION
            ));
        }
        let mut last_tick: Option<u64> = None;
        for f in &self.frames {
            if let Some(lt) = last_tick {
                if f.tick <= lt {
                    return Err(format!("frames out of order: {} <= {}", f.tick, lt));
                }
            }
            last_tick = Some(f.tick);
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn header() -> GameplayReplayHeader {
        GameplayReplayHeader {
            version: GAMEPLAY_REPLAY_VERSION,
            build_sha: "deadbeef".into(),
            seed_lo: 0,
            seed_hi: 0,
            initial_scene_hash: 0,
            tick_rate_hz: 64,
            recorded_at: "2026-04-22T00:00:00Z".into(),
        }
    }

    #[test]
    fn validate_accepts_monotonic_frames() {
        let r = GameplayReplay {
            header: header(),
            frames: vec![
                GameplayReplayFrame { tick: 0, input: vec![], state_hash: 0 },
                GameplayReplayFrame { tick: 1, input: vec![], state_hash: 0 },
            ],
        };
        assert!(r.validate().is_ok());
    }

    #[test]
    fn validate_rejects_out_of_order() {
        let r = GameplayReplay {
            header: header(),
            frames: vec![
                GameplayReplayFrame { tick: 1, input: vec![], state_hash: 0 },
                GameplayReplayFrame { tick: 0, input: vec![], state_hash: 0 },
            ],
        };
        assert!(r.validate().is_err());
    }

    #[test]
    fn validate_rejects_version_mismatch() {
        let mut h = header();
        h.version = GAMEPLAY_REPLAY_VERSION + 1;
        let r = GameplayReplay { header: h, frames: vec![] };
        assert!(r.validate().is_err());
    }
}
