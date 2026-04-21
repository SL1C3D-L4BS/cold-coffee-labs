# ADR 0019 — `.kaudio` cooked format & music streamer (Wave 10C)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10C opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017 (mixer), Phase 6 asset pipeline (`engine/assets/*`), `docs/games/greywater/19_AUDIO_DESIGN.md` §4 (streaming music)

## 1. Context

Greywater ships ~500 MB of audio in the v1 demo: ~40 MB baked SFX, the remainder as streamed music and dialogue. Wave 10C lands:

1. A **cooked container** — `.kaudio` — that keeps both the tight baked-f32 SFX path and the seek-friendly streaming codec path under one extension.
2. A **streaming music pipeline** — job-scheduled decode into a SPSC ring buffer, drained by the audio callback, with the **audio thread never blocking on I/O**.
3. A **decoder registry** — plug-in `IDecoder`s so WAV/Vorbis/Opus/FLAC all round-trip identically.

## 2. Decision

### 2.1 `.kaudio` file layout (little-endian, version 1)

```
offset  size  field
0x00    4     magic        "KAUD"
0x04    2     version      u16 = 1
0x06    2     codec        u16 enum { raw_f32, vorbis, opus, flac }
0x08    1     channels     u8
0x09    1     reserved
0x0A    2     flags        u16 bitfield (loop, streaming, seekable)
0x0C    4     sample_rate  u32
0x10    8     num_frames   u64
0x18    8     loop_start   u64 (frames; 0 = none)
0x20    8     loop_end     u64
0x28    8     seek_table   u64 (file offset; 0 = none)
0x30    8     body_offset  u64
0x38    8     body_bytes   u64
0x40    16    content_hash xxHash3-128 of the body (cook key)
...padding to 64...
0x40+   --    body         codec-native bytes
```

Flags:
- `0x01` loop enabled
- `0x02` streaming (force `MusicStreamer`)
- `0x04` seekable (seek table present)

The seek table — when present — sits at `seek_table`:

```
u32 entries
entries × { u64 frame_index, u64 file_offset }
```

Quantised at 250 ms by default. Target seek latency ≤ 5 ms for a cached file.

### 2.2 AssetCooker subcommand

`gw_cook audio IN.(wav|ogg|opus|flac) --out OUT.kaudio --codec (raw|vorbis|opus) [--stream]`

Wired into the Phase-6 cook CLI. Metadata (loop points, bus hint, accessibility flags) comes from a sibling `IN.asset.toml`. The cooker:

- Decodes source to f32 mono/stereo
- Writes the chosen codec
- Builds seek table if `--stream`
- Stamps `content_hash` = xxHash3-128 of the body
- Produces deterministic output (same input → same bytes)

### 2.3 MusicStreamer

```cpp
class MusicStreamer {
public:
    MusicStreamer(std::unique_ptr<IStreamDecoder>, RingBufferCfg);
    ~MusicStreamer() = default;

    // Main-thread API
    void start();
    void stop();
    void seek(u64 frame);

    // Audio-thread pull (drains ring; never blocks)
    std::size_t pull(std::span<float> out);

    MusicStreamStats stats() const noexcept;  // underruns, queued frames, ...
};
```

- Ring buffer default: **500 ms** at 48 kHz stereo = 48 000 samples.
- Decode job pulls 100 ms pages via `engine/jobs/` I/O pool.
- Audio-thread `pull` is wait-free. Underrun → emit silence + increment a counter on `MusicStreamStats`.
- Fault tolerance: decode error marks stream dead; audio-thread gets silence until `start()` re-initialises.

### 2.4 Crossfade

`AudioService::crossfade_music(AudioClipId new_track, float duration_s)` — uses two `MusicStreamer` slots on the Music bus group. Old fades out linearly; new fades in on the same curve; the old is torn down when its volume reaches 0.

### 2.5 Ducking & category controls

Authoring rules live in `assets/audio/audio_mixing.toml`:

```toml
[rule.combat_ducks_music]
when = "combat_intensity > 0.5"
bus  = "music"
delta_db = -6.0
attack_ms = 80
release_ms = 300

[rule.vacuum_mutes_master]
when = "atmospheric_density < 0.001"
bus  = "sfx"
mute = true
```

`AudioMixerSystem::update(GameState)` evaluates rules each frame. Rules are data — changing them does not require an engine rebuild. The rule evaluator is a straight-line condition parser (no branching beyond `>`, `<`, `==`).

Per-category mutes (GAG Hearing/Basic):

```toml
[bus.master]  { volume = 1.0, mute = false }
[bus.sfx]     { volume = 1.0, mute = false }
[bus.music]   { volume = 0.8, mute = false }
[bus.dialogue]{ volume = 1.0, mute = false }
[bus.ambience]{ volume = 0.8, mute = false }
```

## 3. Non-goals

- No streaming SFX (all SFX is baked — 40 MB budget).
- No runtime re-cook.
- No Vorbis/Opus in Wave 10A (added in 10C, gated by CPM option).

## 4. Verification

- Round-trip cook+decode yields f32 PCM within ±1 ULP for `raw_f32`; PSNR ≥ 40 dB for Vorbis/Opus against source.
- 30-minute streaming soak: 0 underruns at 48 kHz stereo, flat RAM (< 1 MB growth after warmup).
- Seek storm: 1 000 random seeks, p99 < 5 ms.

## 5. Consequences

Positive:
- One format, one cooker, one reader; codec lives in the header byte.
- Format has a version field — we can bump it in Phase 20 without breaking old assets (version-aware decoder).
- Ducking is data, not code — audio designer owns it.

Negative:
- Seek table is optional — if a cooker run forgets `--stream`, music becomes non-seekable. Mitigation: cooker warns when `duration > 30 s` without streaming.

## 6. Rollback

Streaming can be disabled by forcing `codec = raw_f32`. Files are naturally fast-path; larger disk but simpler.
