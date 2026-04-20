# 16 — Greywater Survival & Base Building

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Core gameplay loop

**Gather → Craft → Build → Raid → Dominate → Survive.**

Every system in the simulation supports this loop. The universe is hostile. Resources are scarce. Territory must be claimed and defended. Bases are built, raided, and either survive or collapse.

## 2. Resources

### 2.1 Resource categories

| Category  | Examples                                | Primary use              |
| --------- | --------------------------------------- | ------------------------ |
| Minerals  | Iron, Copper, Titanium, Rare Earth      | Building, crafting       |
| Organic   | Wood, Fiber, Leather, Food              | Construction, consumable |
| Fuel      | Coal, Oil, Uranium, Solar               | Power, vehicles          |
| Exotic    | Alien artifacts, Ancient tech           | Advanced crafting        |

### 2.2 Resource distribution

Resources are placed deterministically from the planetary geology seed (see `13_PROCEDURAL_UNIVERSE.md §6`). Density functions differ per resource:

- Iron ore — Worley-noise clusters in ancient volcanic regions.
- Rare earth — cellular-noise hotspots in impact craters.
- Fossil fuels — Perlin-noise layers in sedimentary basins.
- Exotic — structure-template placement at alien ruins and anomalies.

Scarcity is a feature, not a balancing oversight.

## 3. Crafting

### 3.1 Recipe database

```cpp
struct CraftingRecipe {
    u64                  recipe_id;
    std::array<Ingredient, k_max_ingredients> ingredients;
    u32                  ingredient_count;
    Item                 result;
    f32                  craft_time_sec;
    WorkbenchTier        required_workbench;
};
```

Recipes are data-driven. The recipe database is hot-reloadable.

### 3.2 Workbench tiers

| Tier | Workbench          | Unlocks                                          |
| ---- | ------------------ | ------------------------------------------------ |
| 1    | Campfire           | Basic tools, cooked food                         |
| 2    | Workbench          | Wood structures, simple weapons                  |
| 3    | Furnace            | Metal smelting, advanced tools                   |
| 4    | Assembly Machine   | Firearms, electronics                            |
| 5    | Fabricator         | Exotic tech, spacecraft components               |

Each tier requires specific resources to build, gating progression naturally.

## 4. Base building

### 4.1 Modular grid

Buildings are constructed from modular pieces on a grid:

```cpp
enum class PieceType   { Foundation, Wall, Floor, Door, Window, Roof, Stair, Ceiling };
enum class MaterialTier { Wood, Stone, Metal, Armored, Shielded };

struct BuildingPiece {
    u64           piece_id;
    Vec3_f64      world_pos;   // snap-grid aligned
    Quat          rotation;
    PieceType     type;
    MaterialTier  material;
    f32           health;
};

struct Building {
    u64                 building_id;
    std::vector<BuildingPiece> pieces;
    u64                 owner_id;     // or faction
    Vec3_f64            world_origin;
};
```

Grid snap size is a per-biome parameter — some planets let you build smoother curves than others. (A visual-direction choice to keep the realistic low-poly silhouette clean.)

### 4.2 Structural integrity

Greywater's structural-integrity model is graph-based. **Unsupported structures collapse.**

```cpp
struct IntegrityNode {
    Vec3_f64         position;
    f32              support_value;   // 0.0 = unsupported, 1.0 = fully supported
    std::vector<u32> connected_nodes;
};

void compute_structural_integrity(Building& b) noexcept {
    // Build connectivity graph.
    // Ground-connected nodes are roots (support = 1.0).
    // Propagate support outward with per-material decay.
    // Nodes with support below threshold are "weak":
    //   - take extra damage under environmental stress
    //   - collapse if integrity reaches 0.0
}
```

**Destruction propagation:**
1. A piece is destroyed (combat, environmental damage, time).
2. It is removed from the graph.
3. Integrity is recomputed for all connected components.
4. Any component that loses ground connection begins collapsing.
5. Collapsing pieces become Jolt physics debris.

### 4.3 Environmental damage

| Damage type      | Effect                                            |
| ---------------- | ------------------------------------------------- |
| Storms           | Gradual damage to exposed pieces                  |
| Radiation        | Degrades electronics; requires shielding          |
| Space exposure   | Instant destruction of unsealed areas             |
| Corrosion        | Acid rain on toxic planets                        |
| Seismic          | Volcanic planets shake bases periodically         |
| Temperature      | Extreme cold or heat damages unprotected pieces   |

## 5. Raiding

### 5.1 Explosives

| Explosive        | Damage      | Radius  | Cost        |
| ---------------- | ----------- | ------- | ----------- |
| Satchel charge   | High        | Small   | Moderate    |
| C4               | Very high   | Medium  | High        |
| Rockets          | High        | Large   | Very high   |
| Breaching charge | Medium      | Tiny    | Low         |

### 5.2 Weak-point targeting

Players can identify weak points (low-integrity nodes) and target them for efficient raiding. This is a skill-mechanic, not a UI feature — the exposed support structure is the hint.

### 5.3 Raid defense

- **Auto-turrets** — automated defense with ammunition consumption.
- **Traps** — landmines, spike traps, flame turrets.
- **Shields** — energy shields (late-game, Tier-5 crafting).
- **Alarm systems** — notify owner when base is attacked; alerts over the comms system.

## 6. Economy

### 6.1 Player-driven economy

- **Trading posts** — player-built marketplaces.
- **Currency** — credits, earned from resource sales and contracts.
- **Supply/demand** — dynamic pricing based on regional availability.
- **Black markets** — space stations with illegal goods; reputation-gated.

### 6.2 Trade routes

```cpp
struct TradeRoute {
    u64      route_id;
    Vec3_f64 origin_station;
    Vec3_f64 destination_station;
    std::vector<CargoItem> cargo;
    f32      travel_time_sec;
    f32      risk_factor;   // pirate attack probability
};
```

Players can establish and defend trade routes.

## 7. Territory control

### 7.1 Claim system

```cpp
struct TerritoryClaim {
    u64      claim_id;
    u64      owner_id;
    u64      faction_id;     // optional
    Vec3_f64 center;
    f32      radius;
    u64      timestamp;
};
```

Players claim territory by placing **Claim Flags**. Claims consume upkeep resources — abandoned claims decay.

### 7.2 Benefits of control

- Resource generation bonus within claim radius
- Building cost reduction
- Radar coverage
- Automatic defense turrets

### 7.3 Warfare

Factions can declare war and contest territories. Victory conditions (configurable by server):

- Destroy enemy claim flag
- Hold territory for N minutes without contest
- Eliminate all enemy players in zone within a time window

---

*The universe is hostile. Gather, craft, build, raid, dominate, survive. Or don't.*
