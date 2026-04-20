# 17 — Greywater Multiplayer

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Network model — hybrid server-authoritative

Greywater uses a **server-authoritative model** with two netcode techniques, chosen per gameplay context:

| Gameplay         | Netcode                    | Justification                              |
| ---------------- | -------------------------- | ------------------------------------------ |
| PvP combat       | **Rollback**               | Low latency, responsive feel               |
| Base building    | Server-authoritative       | Anti-cheat, consistent world state         |
| Economy          | Server-authoritative       | Secure transactions                        |
| Large-scale sim  | **Deterministic lockstep** | Consistent state across peers              |
| Space flight     | Client prediction + reconcile | Smooth controls, server correction       |

Both rollback and lockstep share a foundation: **deterministic simulation**. Greywater's engine-core command bus (`engine/core/command/`), ECS, and physics (Jolt in lockstep mode) are deterministic by design — see `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §2.6.

## 2. Rollback netcode

### 2.1 Principle

Rollback netcode:

- Never waits for remote inputs to render local player actions.
- Predicts remote player inputs when they haven't arrived.
- **Rewinds and resimulates** when predictions turn out wrong.

### 2.2 Rollback manager

```cpp
namespace gw::net {

class RollbackManager {
public:
    void save_state(i32 frame, const GameState& state);
    GameState load_state(i32 frame);
    void advance_frame(const PlayerInputs& local, const RemoteInputs& remote);

private:
    static constexpr i32 k_max_rollback_frames = 16;  // at 60 Hz ≈ 266 ms window
    RingBuffer<GameState,    k_max_rollback_frames> state_history_;
    RingBuffer<PlayerInputs, k_max_rollback_frames> input_history_;
    i32 current_frame_ = 0;
};

void RollbackManager::advance_frame(const PlayerInputs& local, const RemoteInputs& remote) {
    save_state(current_frame_, game_state_);

    // Simulate with current inputs (prediction applied where remote is missing).
    game_state_.simulate(local, remote);

    // Late remote input that contradicts the prediction → rollback.
    if (remote.frame < current_frame_ && remote.differs_from_prediction()) {
        GameState corrected = load_state(remote.frame - 1);
        for (i32 f = remote.frame; f <= current_frame_; ++f) {
            corrected.simulate(input_history_[f].local, input_history_[f].remote);
        }
        game_state_ = corrected;
    }

    ++current_frame_;
}

} // namespace gw::net
```

### 2.3 Requirements on the simulation

All networked gameplay code runs inside a deterministic sandbox:

- **No sampling of system time.** Frame-step time is passed in.
- **No unseeded RNG.** Every RNG is seeded from the shared frame seed.
- **No FPS-dependent logic.** Fixed-timestep physics.
- **Jolt in deterministic mode.** Required.

## 3. ECS-based server

### 3.1 Replication

Server maintains the authoritative ECS world. **Only changed components are replicated.**

```cpp
struct ComponentDelta {
    EntityID    entity;
    ComponentID component;
    u32         field_offset;
    Span<const u8> field_data;
};

std::vector<ComponentDelta> compute_delta(const World& world, u64 last_ack_frame);
```

### 3.2 Interest management

Only entities near each player are replicated to that player. Based on chunk coordinates:

```cpp
class InterestManager {
public:
    SmallVec<EntityID, 256> relevant_entities_for(Vec3_f64 player_pos, f32 radius) const;
private:
    HashMap<ChunkCoord, std::vector<EntityID>> chunk_entities_;
};
```

The relevance radius is per-component. Combat-relevant components have a larger radius than cosmetic ones.

### 3.3 Reliability tiers per component

| Tier        | Examples                            | Delivery                    |
| ----------- | ----------------------------------- | --------------------------- |
| Reliable    | Inventory, building state           | Retransmit until acked      |
| Unreliable  | Transform, velocity                 | Send latest; drop old       |
| Sequenced   | Animation state                     | Drop out-of-order           |

## 4. Anti-cheat

### 4.1 Server-side physics validation

The server runs the same deterministic simulation and validates client inputs:

```cpp
bool validate_player_movement(const PlayerInput& input,
                               const Transform& before,
                               const Transform& after) {
    Transform expected = simulate_movement(before, input);
    f32 error = length(expected.position - after.position);
    return error < k_max_allowed_error;
}
```

### 4.2 Replay verification

Suspicious sessions can be replayed deterministically from the input log. Discrepancies between the replay and the recorded state flag the session.

### 4.3 Integrity

- SHA-256 hashes on all cooked assets — client must match.
- Server-authoritative movement, damage, inventory.
- External anti-cheat (EAC) integration via `IAntiCheat` interface (`engine/net/anticheat/` — studio-internal addendum, not in open repo).

## 5. Voice

- **Codec:** Opus (20 ms frames).
- **Transport:** same GameNetworkingSockets connection as gameplay.
- **Spatialization:** via Steam Audio, using the same listener setup as in-world audio (see `19_AUDIO_DESIGN.md`).
- **Moderation:** opt-in STT → text moderation pipeline (post-v1).

## 6. Dedicated server

```cpp
class DedicatedServer {
public:
    void run() {
        world_ = ecs::World::create();
        network_ = net::NetworkManager::create();

        while (running_) {
            accumulator_ += delta_time_;
            while (accumulator_ >= k_fixed_dt) {
                world_->tick(k_fixed_dt);
                accumulator_ -= k_fixed_dt;
            }

            network_->receive_inputs();
            network_->send_world_state();
        }
    }
};
```

### 6.1 Scalability

- **Sharding:** different star systems on different server instances.
- **Persistence:** periodic state snapshots to a SQLite database (matches the local save system — same format, different storage).
- **Orchestration:** containerized, studio-internal; post-v1 for in-house server hosting.

---

*Deterministic simulation makes rollback possible. Rollback makes combat feel responsive. Server authority makes cheating hard. Greywater has all three, because the engine was designed for them from day one.*
