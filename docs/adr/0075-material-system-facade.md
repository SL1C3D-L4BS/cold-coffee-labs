# ADR 0075 — Material system façade (`MaterialWorld`, templates, instances)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Waves:** 17B (core), 17C (authoring)
- **Companions:** 0074 (shader pipeline), 0076 (`.gwmat` v1), 0082.

## 1. Context

Greywater's renderer is PBR metal-roughness by default and is moving to
realistic low-poly (surface-driven appearance, high-quality lighting).
Wave 17B introduces a *first-class* material subsystem separated into:

- **`MaterialTemplate`** — pipeline permutation, reflection-derived
  descriptor layout, default parameter block.
- **`MaterialInstance`** — a **256-byte** parameter block + up to 16
  texture-slot handles bound at draw time.
- **`MaterialWorld`** — the PIMPL façade (mirrors
  `PlatformServicesWorld` / `A11yWorld`); single RAII owner of every
  template and every instance, reports stats, hosts the reload chain.

## 2. Decision

### 2.1 Façade shape

`MaterialWorld` is a **PIMPL** class. Public surface (matches the Phase-
16 pattern):

```cpp
class MaterialWorld {
 public:
  MaterialWorld();
  ~MaterialWorld();
  [[nodiscard]] bool initialize(MaterialConfig,
                                gw::config::CVarRegistry*,
                                gw::events::CrossSubsystemBus* = nullptr,
                                gw::jobs::Scheduler* = nullptr);
  void shutdown();
  [[nodiscard]] bool initialized() const noexcept;

  void step(double dt_seconds);  // drains reload chain

  [[nodiscard]] MaterialTemplateId  create_template(const MaterialTemplateDesc&);
  [[nodiscard]] MaterialInstanceId  create_instance(MaterialTemplateId);
  void                              destroy_instance(MaterialInstanceId);
  void                              set_param(MaterialInstanceId, std::string_view key, float v);
  void                              set_param(MaterialInstanceId, std::string_view key, glm::vec4 v);
  void                              set_texture(MaterialInstanceId, std::uint32_t slot, TextureHandle);
  [[nodiscard]] MaterialStats       stats() const noexcept;
  [[nodiscard]] bool                reload(MaterialInstanceId);  // hot-reload one
  [[nodiscard]] std::size_t         reload_all();
  // ... see engine/render/material/material_world.hpp
};
```

### 2.2 Template → Instance

- A `MaterialTemplate` owns a `gw::render::shader::PermutationTable`
  slice (ADR-0074). Permutation axes match glTF PBR extensions:
  `clearcoat`, `specular`, `sheen`, `transmission` (each a binary axis)
  + the base `metal-rough` / `dielectric` quality tier.
- A `MaterialInstance` is a 256-byte POD parameter block + a
  16-slot texture table. The block layout is **inferred** from the
  permutation's SPIR-V reflection (ADR-0074 §2 reflect).

### 2.3 glTF PBR extension matrix

The renderer ships with five opaque-PBR permutations:

| template | KHR extensions |
|---|---|
| `pbr_opaque/metal_rough` | — |
| `pbr_opaque/dielectric` | `KHR_materials_specular` |
| `pbr_opaque/clearcoat` | `KHR_materials_clearcoat` |
| `pbr_opaque/sheen` | `KHR_materials_sheen` |
| `pbr_opaque/transmission` | `KHR_materials_transmission` |

All five share one `MaterialTemplate` with the permutation axes
orthogonal. Combined permutations (e.g. clearcoat + sheen) are legal
but unused in the shipping art pack today — pre-cooked in
`studio-*` presets only.

### 2.4 Hot-reload chain

Wave 17C wires the reload chain:

```
.hlsl changed → shader_manager file-watch → recompile + reflect
               → MaterialTemplate pipeline swap (atomic)
               → MaterialInstance parameter re-layout
               → frame_graph pass rebind next frame
```

Hot-reload work goes through the new `jobs::material_reload` lane
(ADR-0082 §3). `mat.auto_reload` CVar throttles the chain to ≤ 5 Hz.

### 2.5 Authoring (Wave 17C)

The façade exposes:

- `list_templates()` / `list_instances()` — read-only iteration.
- `describe_instance(MaterialInstanceId)` — serialized JSON blob the
  editor panel renders.
- `preview_sphere(MaterialTemplateId)` — returns a lightweight scene
  description (a sphere with a chosen template) for the in-editor
  material preview. Engine-pure; the editor app consumes this as an
  ImGui panel input in Phase 18+.

## 3. Non-goals

- No node-graph material editor. Graph authoring is intentionally
  deferred to Phase 22 tooling.
- No procedural substance-style materials. Shipping pack is glTF-PBR.

## 4. Header-quarantine invariant

Public `engine/render/material/*.hpp` may **not** include any HLSL/DXC/
Slang/Vulkan-specific SDK header. Vulkan pipeline handles live behind
opaque IDs (`TextureHandle`, `PipelineHandle`).

## 5. CVars (§17B/C, ≥ 8)

| Name | Default | Notes |
|---|---|---|
| `mat.default_template` | `pbr_opaque/metal_rough` | initial template for new instances |
| `mat.instance_budget` | 4096 | hard cap |
| `mat.texture_cache_mb` | 512 | texture-slot cache |
| `mat.auto_reload` | true | Wave 17C |
| `mat.preview_sphere` | true | editor convenience |
| `mat.quality_tier` | `high` | low / med / high |
| `mat.sheen_enabled` | true | gates the sheen permutation |
| `mat.transmission_enabled` | true | gates transmission permutation |

## 6. Console commands

`mat.list`, `mat.reload <id>`, `mat.set <id> <key> <value>`,
`mat.dump <id>`, `mat.preview <template>`.

## 7. Perf

See `docs/perf/phase17_budgets.md`. 1024 instances upload ≤ 1.5 ms GPU;
per-frame CPU upload ≤ 0.3 ms.

## 8. Test plan

- `phase17_material_template_test.cpp` — 14 cases.
- `phase17_material_instance_test.cpp` — 12 cases.
- `phase17_gwmat_test.cpp` — 10 cases (ADR-0076).

## 9. Consequences

- **Positive.** Clear ownership, deterministic permutation paths,
  scriptable material authoring via console, hot-reload survives the
  entire running session.
- **Negative.** Fixed 256-byte parameter block excludes exotic BRDFs
  (deferred until Phase 22 *procedural materials*).

## 10. Hand-off (frozen for Phase 20 planetary rendering)

`MaterialTemplate` ABI is frozen at the end of Phase 17. Planetary
rendering (Phase 20) inherits it as a first-class input.
