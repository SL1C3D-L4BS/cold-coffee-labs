# ADR 0049 — Reliability tiers + priority scheduler

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14D)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 087), extends ADR-0047 + ADR-0048

## 1. Context

Not every replicated byte deserves the same delivery guarantee. Combat-authoritative fields must arrive, in order, or the hit rewind (ADR-0050) lies. Cosmetic fields are throw-away — an old one is worse than none. Animation state is "latest-wins." A single channel can't serve all three, and under a per-peer bandwidth ceiling the scheduler must pick *which* component gets the next byte when it can't all fit.

## 2. Decision

### 2.1 Reliability tiers (per-component enum)

| Tier | Semantics | Example components |
|---|---|---|
| **Reliable** | ACK ring + retransmit after `rtt × net.rel.rtt_retransmit_mult` (default 1.5), sequenced delivery | HealthComponent, InventoryOp, ChatMessage |
| **Unreliable** | fire-and-drop; newer sequence wins; older on arrival is dropped | TransformComponent, VelocityComponent |
| **Sequenced** | latest-wins; out-of-order arrivals discarded | AnimationStateComponent, VoiceFrame |

The tier is declared on the component via `GW_NET_RELIABILITY(kind)` alongside `GW_NET_REPLICATE(...)`. Absent declaration defaults to `Unreliable`.

### 2.2 Per-peer bandwidth ceiling

`net.send_budget_kbps` (default 96 KiB/s on 16c, enforced as ≤ 32 KiB/s on 2c+server) caps each peer's *down* stream. The scheduler uses **Deficit Round Robin (DRR)** across reliability tiers, with per-component `priority_hint` (0 = cosmetic, 255 = combat-authoritative) used as the DRR weight.

### 2.3 Starvation guard

Any component that has been deferred ≥ `net.prio.starvation_frames` (default 8) gets its priority boosted by +16 for the next tick and keeps boosting until delivered. Once delivered the hint resets. This bounds worst-case latency at `(starvation_frames × tick_hz_inv) × boost_ceiling` frames — 133 ms for 2c+server in default config.

### 2.4 Retransmit + ack windows

Reliable messages carry a 32-bit per-peer sequence and an ack bitmap in each inbound packet (standard 32-bit bitmap: bit i = ack for seq − i). Retransmit queues live per peer and are also arena-backed.

### 2.5 Channel IDs

```
ChannelId:
   0: System (connect/handshake/disconnect)
   1: Replication-Reliable
   2: Replication-Unreliable
   3: Replication-Sequenced
   4: Voice (always Unreliable-Sequenced)
   5: RPC-Reliable (engine + gameplay calls)
   6..31: reserved for game-specific tiers
```

The transport layer (ADR-0047) demuxes on channel id; the reliability layer is per-channel.

## 3. Consequences

- A malicious client can't starve combat; the hard boundary is per-peer bandwidth, not per-component.
- Game code opts into reliability by annotation, not by writing its own retransmit loop.
- Voice never blocks combat: voice is on its own channel with a lower priority tier, and voice packets drop first when the budget is tight.

## 4. Alternatives considered

- **FEC-only unreliability** — rejected for our reliable tier; FEC is voice-only (covers brief packet loss without a retransmit round trip).
- **Static per-component bandwidth caps** — rejected; DRR adapts to traffic automatically and starvation guard provides the backstop.
- **Leave priority to the gameplay layer** — rejected; networking owns latency ceilings so game code doesn't have to reimplement them per feature.
