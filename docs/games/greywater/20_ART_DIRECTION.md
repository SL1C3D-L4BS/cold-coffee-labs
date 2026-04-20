# 20 — Greywater Art Direction: Realistic Low-Poly

**Status:** Canonical (binding for all art, VFX, shader, and content decisions)
**Last revised:** 2026-04-19

---

## 1. The directive

**Greywater's visual style is "realistic low-poly."**

Not blocky voxel. Not hyper-real. The intersection:

- **Reduced polygon counts** on meshes.
- **Flat or subtle-gradient shading** for surfaces.
- **Physically-based, realistic lighting** — PBR for materials, atmospheric scattering, real shadow cascades, IBL for ambient.
- **Strong, restrained color palettes** per biome and per planet type.
- **Clean silhouettes** authored deliberately.
- **Atmosphere-driven mood** — the sky does most of the emotional work.

Think of it as **"realistic light on simple geometry."** The lighting model is where we invest visual budget; the geometry is where we spend restraint.

## 2. The seven properties of Greywater's visual style

Every piece of Greywater art — terrain, props, characters, vehicles, UI — embodies all seven:

1. **Low polygon count** relative to the viewing budget. Silhouette-first modelling; no interior detail the camera never sees.
2. **Clean silhouette.** Fewer polygons means each polygon matters. Silhouettes are authored deliberately, not left to chance.
3. **Flat or subtle-gradient shading** as the default. Per-face normals via screen-space derivatives. `SMOOTH_SHADING` is reserved for organic creature skin and hero props.
4. **Physically-based materials** with *restrained* parameters — mid-range roughness (0.4–0.8), low metalness except for intentionally metallic surfaces. No chrome. No glossy plastic. No wet-puddle specular highlights on otherwise-matte surfaces.
5. **Strong, per-biome color palettes.** Five principal hues per scene. Saturation is a special effect, not a default. Atmospheric composition drives sky hue; sky hue drives mood.
6. **Realistic lighting on simple geometry.** Cascaded shadow maps, IBL, physically-based atmospheric scattering. The light is where we invest the visual budget.
7. **Atmosphere-driven mood.** The sky and the scattering do more emotional work than the props. A desert planet's orange horizon, an ice world's blue shadows, a toxic world's sickly green — this is the principal mood lever.

**What this rules out by construction:** blocky voxels, hyper-realistic photo-scanned geometry, hand-painted photoreal textures, glossy-plastic shaders, any style that depends on per-pixel detail rather than per-polygon intent.

## 3. Shading rules

### 3.1 Default surface shading

**Flat-shaded** by default. Per-face normals via screen-space derivatives in the fragment shader:

```glsl
// Flat shading without pre-baked normals — preserves low-poly look
// without doubling vertex count to split normals at edges.
vec3 flat_normal = normalize(cross(dFdx(world_pos), dFdy(world_pos)));
```

Variants (shader permutations):
- `FLAT_SHADING` (default)
- `SUBTLE_GRADIENT` — smooth normals weighted by face adjacency, producing a soft-shaded but still-low-poly look
- `SMOOTH_SHADING` — used sparingly, for organic creature skin and hero props

### 3.2 Materials

PBR (metallic/roughness) with **restraint**:
- Most surfaces: mid-range roughness (0.4 – 0.8), low metalness.
- Snow, water, wet rock: higher roughness variation within a single material.
- Rare metallic surfaces: weathered alloys, ship hulls.
- No chrome. No glossy "plastic" look. No wet-puddle highlights on otherwise-matte surfaces.

### 3.3 Textures

- **Small textures.** Base-color maps at 512 × 512 for environment props; 1024 × 1024 for hero props; 2048 × 2048 only for vehicle / character hero meshes.
- **BC7 compression** for base color, normal, ORM (occlusion-roughness-metalness) channels.
- **Procedural detail** where possible — noise-based weathering, edge-wear, rim-dust driven by mesh curvature in the shader, not baked.

## 4. Palettes

### 4.1 Per-planet-type palette

Each planet type has a dominant and recessive palette, expressed as five-color ramps. The palette drives:

- Terrain shader base color
- Sky color at horizon and zenith
- Atmospheric scattering coefficients
- Vegetation color shift
- Fog and ambient tint

Example (Desert):

| Role              | Hex        | Notes                           |
| ----------------- | ---------- | ------------------------------- |
| Sky zenith        | `#E2B477`  | Warm amber                      |
| Sky horizon       | `#D97F42`  | Dusk orange                     |
| Terrain mid       | `#B08258`  | Weathered sand                  |
| Terrain shadow    | `#5E3B2A`  | Deep umber                      |
| Vegetation accent | `#728044`  | Hardy scrub                     |

Each planet type (Desert, Ocean, Ice, Toxic, Volcanic, Earth-like) has its own canonical palette set. Within a type, **per-planet palette shift** produces variety without breaking the family — a Desert planet can be orange-dominant, red-dominant, or yellow-dominant.

### 4.2 Color discipline

- **No more than five principal hues** per scene.
- **Saturation is a special effect**, not a default. Muted saturation for everyday; saturated hues for hazards (lava, toxic), anomalies, and UI-critical signals.
- **Never use pure white or pure black as scene colors.** Always tinted. Obsidian (`#0B1215`) or Crema (`#E8DDC8`) as starting points.

## 5. Geometry budget (soft targets)

| Asset class                    | Target tris        | Notes                              |
| ------------------------------ | ------------------ | ---------------------------------- |
| Environment prop (rock, crate) | 100 – 500          | Silhouette-first; no interior detail |
| Tree (LOD 0)                   | 200 – 1 500        | Flat-shaded canopy clusters        |
| Building piece                 | 200 – 800          | Modular grid-aligned                |
| Wildlife (small, LOD 0)        | 800 – 2 000        | Low-poly with clean skinning        |
| Wildlife (large, LOD 0)        | 2 000 – 5 000      | Hero creatures                      |
| Player character (LOD 0)       | 4 000 – 8 000      | Most poly investment here           |
| Vehicle (small, LOD 0)         | 3 000 – 6 000      | Hard-surface, functional readability |
| Capital ship (LOD 0)           | 10 000 – 40 000    | Large but silhouette-dominated      |

**LOD chain:** 4 levels minimum (LOD 0 → 1 → 2 → 3), each ~50 % triangle reduction. Impostor at LOD 3 for distant vegetation and structures.

## 6. Lighting

### 6.1 The sky does the work

Atmospheric scattering is the primary mood tool. Sunrise/sunset lighting is authored via:
- Sun direction and intensity
- Rayleigh / Mie coefficients (per planet type)
- Ground albedo reflection

A cold blue alpine dusk and a hot red desert dawn share the same shader — different parameters, different mood.

### 6.2 Shadows

- **Cascaded shadow maps** (directional sun / star).
- 4 cascades on RX 580; 2 KM shadow range.
- No RT shadows (post-v1 where hardware permits).
- Soft shadow filter: PCF kernel sized by cascade.

### 6.3 Indirect lighting

- **IBL** — prefiltered environment maps per biome.
- **Screen-space GI** (SSGI) as a cheap approximation; budget-gated.
- No RT-GI, no baked lightmaps (too much content authoring overhead for procedural worlds).

### 6.4 Time of day

Each planet has its own orbital period. Day-night cycle is driven by the orbit, not a fixed 24-hour timer. Lighting evolves with the sun's altitude.

## 7. Atmospheric effects

- **Volumetric clouds** — raymarched, Perlin-Worley (see `15_ATMOSPHERE_AND_SPACE.md §2`).
- **Fog** — exponential height-based, tinted by the atmosphere's scattering coefficients.
- **Rain / snow / dust** — particle-based, GPU-driven, density-aware.
- **Auroras** — thermosphere-layer procedural effect, rare and stunning.
- **Heat haze** — screen-space distortion near volcanic zones.

## 8. VFX

- **GPU-driven particles** only (see `02_SYSTEMS §2` — VFX subsystem).
- **Flat-colored particles with additive or subtractive blending** — no texture-heavy particle sheets.
- **Explosions** — combination of expanding shells (low-poly) + particles + volumetric smoke.
- **Plasma re-entry** — per-hull-surface fragment shader, not a particle effect — it wraps the mesh.

## 9. UI

In-game UI (RmlUi, see `docs/01_ENGINE_PHILOSOPHY.md §3`) adheres to the *Cold Drip* palette (Obsidian / Greywater / Brew / Crema / Signal / Bean). No ornamentation. No Material-Design shadows. Flat panels with 1-pixel strokes.

HUD is **minimal and diegetic**. Health, ammunition, vitals are shown on the player's wrist HUD, not as fixed screen overlays.

## 10. What this rules out

- **No over-detailed rock / cliff meshes.** Silhouette carries the shape; normal maps carry the detail.
- **No baked lightmaps.** Procedural worlds cannot practically be lightmapped.
- **No chrome / mirror / glossy materials as a default.** Rare, and always earned.
- **No particle-heavy vegetation cards.** Real geometry at LOD 0.
- **No skeuomorphic UI.** Flat and clean.
- **No hand-painted photoreal textures.** We don't have that art budget and the world is procedural.

## 11. Shader permutation flags (summary)

| Flag              | Default | Purpose                                       |
| ----------------- | ------- | --------------------------------------------- |
| `FLAT_SHADING`    | on      | Per-face normals via dFdx/dFdy                |
| `SUBTLE_GRADIENT` | off     | Smooth normals with soft falloff              |
| `PBR_STANDARD`    | on      | Metallic/roughness PBR                        |
| `IBL_PREFILTERED` | on      | Use prefiltered env map                       |
| `CSM_4CASCADES`   | on      | Four-cascade directional shadows              |
| `SSGI`            | on      | Screen-space global illumination              |
| `ATMOSPHERE_IN`   | on      | Apply atmospheric scattering as post          |
| `UNDERWATER`      | off     | Caustics + fog when listener below water line |

---

*Realistic light on simple geometry. The atmosphere carries the mood. The palette is restrained. The silhouette is intentional. Low-poly, yes — but anything but cheap.*
