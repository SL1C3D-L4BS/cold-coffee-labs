# ADR 0023 — Engine core events: typed pub/sub, zero-alloc in-frame bus, cross-subsystem bus

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11A kickoff)
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes:** none
- **Related:** ADR-0022 §5 (Phase-10 → Phase-11 inheritance), blueprint §3.19

## Context

Phase 10 landed `AudioService` and `InputService` behind null/stub backends, with device-hotplug and audio-cue signals ready to cross subsystem boundaries. Phase 11 must turn those into a *shipping runtime* by introducing the shared **event bus** every subsystem (input, audio, UI, console, CVar registry, BLD over the C-ABI) talks on.

Three distinct event behaviours are needed, and **only** three:

1. **Synchronous direct pub/sub** — UI listens to a CVar change and re-skins immediately; audio listens to `set_bus_volume` and applies; pattern: compile-time dispatch, no heap.
2. **In-frame deferred queue** — gameplay fires 300 `AudioMarkerCrossed` events during a cut-scene; the subtitle system drains them once at end-of-frame; pattern: double-buffered ring, zero-alloc steady-state.
3. **Cross-subsystem (C-ABI) bus** — BLD (Rust) publishes a `ConfigCVarChanged` from a plan-act-verify loop; the engine drains it next frame; pattern: type-erased payload header + opaque bytes, dispatched off the jobs pool.

## Decision

Ship three primitives in `engine/core/events/`:

- `EventBus<T>` — compile-time dispatch. `subscribe(handler) → SubscriptionHandle`; `publish(const T&)` walks a small-buffer vector of handlers. Reentrancy-safe (iteration uses a guarded index; unsubscribe-during-dispatch defers to after the walk). Zero heap on publish once the subscriber table is warm.
- `InFrameQueue<T>` — double-buffered SPMC queue. Writers fill the back buffer under a seqlock; `drain(fn)` swaps front/back and walks the front buffer. Capacity is fixed at registration; overflow increments a dropped-counter and returns `false`. **Zero allocations after registration.**
- `CrossSubsystemBus` — fixed-size (default 256 slots) ring of `{EventKind : u32, size : u16, flags : u16, bytes[248]}` records. Publishers call `publish(kind, std::span<const std::byte>)`; subscribers iterate with a visitor. Used across the C-ABI from BLD.

The engine-level event catalogue lives in `events_core.hpp` and is frozen as Phase 11 ships:

| Event | Carrier | Lands on |
|---|---|---|
| `DeviceHotplug` | `EventBus<DeviceHotplug>` | input service + dev console |
| `ActionTriggered` | `InFrameQueue<ActionTriggered>` | HUD + audio cue system |
| `WindowResized` | `EventBus<WindowResized>` | UI service + render swapchain |
| `FocusChanged` | `EventBus<FocusChanged>` | UI service + input (release-all-keys) |
| `AudioMarkerCrossed` | `InFrameQueue<AudioMarkerCrossed>` | caption channel |
| `ConfigCVarChanged` | `EventBus<ConfigCVarChanged>` | every subsystem |
| `ConsoleCommandExecuted` | `EventBus<ConsoleCommandExecuted>` | audit log + BLD replay |
| `UILayoutComplete` | `InFrameQueue<UILayoutComplete>` | render pass ordering |

A compile-flag `GW_EVENT_TRACE=1` wires an `EventTraceSink` that appends a binary `.event_trace` file per run — same binary shape as Phase 10's `.input_trace`, so the same tooling inspects both.

## Perf contract

| Gate | Target | Notes |
|---|---|---|
| `publish(empty)` single-thread | ≤ 12 ns | no subscribers, warm cache |
| `drain` 1 000 events | ≤ 40 µs / 0 B heap | `InFrameQueue<T>` |
| `CrossSubsystemBus` 1 000 events × 3 subs | ≤ 120 µs / 0 B heap | jobs-dispatched |

All three gates enforced by `gw_perf_gate`'s `events_*` tests.

## Tests (≥ 10)

1. publish-before-subscribe drops the event cleanly (no crash, no alloc)
2. unsubscribe during dispatch defers removal to the end of the walk
3. `InFrameQueue<T>` front/back swap preserves ordering across drains
4. `CrossSubsystemBus` payload alignment is 16-byte for SSE/NEON consumers
5. subscriber-table saturation triggers an `events_full` counter, not an abort
6. `.event_trace` determinism replay across x64/ARM64 (pinned fixtures)
7. reentrancy guard — publishing inside a handler defers the inner publish
8. subscriber lifetime — destroying the subscription during dispatch is safe
9. alignment — every event type satisfies `alignof(T) ≤ 16`
10. cross-platform byte order — event trace bytes are identical on x64 and ARM64

## Consequences

- Phase 10's ad-hoc device-hotplug callback in `InputService::update` gets retired in 11A and routed through `EventBus<DeviceHotplug>`; the callback is kept as a thin forward for one release.
- BLD's Rust side gains a C-ABI `gw_event_bus_publish(kind, ptr, len)` that maps to `CrossSubsystemBus::publish`. The full Surface H v2 wiring lands in Phase 18, but the seam is open now.
- Editor ImGui panels keep their synchronous `on_change` style via `EventBus<T>` — no double-dispatch.

## Alternatives considered

- **entt::dispatcher / sigslot / libcallme** — too much runtime dispatch overhead (`std::function`-based), not zero-alloc.
- **Single giant event union + visitor per frame** — simpler, but loses compile-time type safety and balloons the CPU-cache footprint when few subsystems care about a given event.
- **Broadcast every event over the cross-subsystem bus** — too slow; we measure 120 µs for 1 000 events, so every cheap synchronous edge would cost µs instead of ns.

Rejected. `EventBus<T>` + `InFrameQueue<T>` + `CrossSubsystemBus` is the minimum surface that covers every known 11A–11F use-case and forces new use-cases to motivate themselves against one of the three.
