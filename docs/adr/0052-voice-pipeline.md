# ADR 0052 — Voice pipeline: capture → Opus → transport → Steam Audio

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Waves 14H + 14I)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 091–092)

## 1. Context

Voice chat in *Two Client Night*: ≤ 150 ms round-trip on LAN, audibly correct spatialization, per-pair mute/block, opt-in push-to-talk default, FEC tolerant of 2 % packet loss. Opus at 20 ms frames, 48 kHz mono, 24 kbps CBR covers speech for RX 580 CPU headroom. Steam Audio already wires Phase 10's HRTF + atmosphere filter — voice reuses the same listener.

## 2. Decision

### 2.1 Pipeline

```
IVoiceCapture (mic @ 48 kHz mono, 20 ms frames)
  → VoiceCodec::encode (Opus, 24 kbps CBR, complexity 5, FEC on)
    → [seq:u16][len:u16][payload:N]
      → transport channel 4 (Unreliable-Sequenced, priority = Voice tier)
        → VoiceCodec::decode
          → VoiceSpatializer (Steam Audio HRTF + atmosphere filter)
            → audio mixer "voice" bus
              → output device
```

### 2.2 Public API

```cpp
class VoiceSystem {
public:
    [[nodiscard]] bool enable_push_to_talk() const noexcept;
    void set_push_to_talk(bool on) noexcept;
    void hold_transmit(bool hold) noexcept;

    void mute_peer(PeerId, bool muted) noexcept;
    [[nodiscard]] bool peer_muted(PeerId) const noexcept;

    [[nodiscard]] VoiceStats stats() const noexcept;
};
```

`VoiceStats` exposes `{encode_time_us, decode_time_us, packets_sent, packets_lost, fec_recovered}` for debug + perf gates.

### 2.3 Frame layout (wire)

| Field | Bits | Notes |
|---|---|---|
| seq | 16 | Rolls over; out-of-order arrival discarded |
| len | 16 | Opus payload length (bytes) |
| payload | 8 × len | Opus-encoded data |

Frame size at 24 kbps / 20 ms = 60 bytes + 4 header = 64 bytes. Three simultaneous speakers on a 2c+server sandbox = 9.6 KiB/s/client + 6-byte per-packet header × 150 pps ≈ 11.5 KiB/s — fits easily under `net.send_budget_kbps`.

### 2.4 Null backend

Without `GW_ENABLE_OPUS` the codec is a **null passthrough** that stores the raw 20 ms PCM frame and returns it on decode (exact round-trip for tests). The null capture is a seeded sine-wave source so round-trip SNR is testable against a reference.

### 2.5 Spatialization

`VoiceSpatializer` takes the decoded PCM + the speaker's world position (replicated via `TransformComponent`) and submits to `engine/audio`'s `IAudioService::voice_bus()`. When `GW_ENABLE_PHONON` is off, the spatializer falls back to constant-power panning by source azimuth — enough to keep the `voice_spatial_directionality_test` within 6 dB.

### 2.6 Budgets

| Stage | Budget on RX 580 CPU |
|---|---|
| Capture pull | ≤ 0.3 ms |
| Opus encode 20 ms CBR 24 kbps c5 | ≤ 1.1 ms |
| Transport + network | 10–60 ms |
| Opus decode | ≤ 0.4 ms |
| HRTF spatialize | ≤ 1.0 ms |
| Output device latency | ≤ 20 ms |
| **Total round-trip (LAN)** | **≤ 150 ms** |

## 3. Consequences

- Voice never competes with combat-authoritative replication for a reliable slot; it's Unreliable-Sequenced (ADR-0049) on its own channel.
- Per-peer mute/block is enforced at the mixer — muted peers never decode, saving CPU.
- STT caption plumbing (ADR-0055 §a11y) has a hook at the decoder so transcribers can subscribe without changes to the hot path.

## 4. Alternatives considered

- **libsamplerate in the path** — rejected; capture is already 48 kHz mono.
- **Multi-channel voice (stereo)** — rejected; doubles bandwidth, HRTF spatialization is mono-source by contract.
- **Speex** — rejected; Opus is the RFC-standard descendant and ships better quality per bit at speech band.
