# 14 — Greywater Planetary Systems

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Spherical planet representation

Greywater planets are **true spheres**, not flat maps. Terrain is generated on a spherical coordinate system and projected to 3D Cartesian coordinates using `float64`.

### 1.1 Coordinate systems

```cpp
struct SphericalCoord {
    f64 latitude;   // radians, -π/2 to +π/2
    f64 longitude;  // radians, -π to +π
    f64 altitude;   // meters above sea level
};

Vec3_f64 spherical_to_cartesian(SphericalCoord sc, f64 planet_radius) noexcept;
```

### 1.2 Quad-tree cube-sphere subdivision

Planets are subdivided using a cube-sphere:

- **Root:** 6 quad faces (one per cube face).
- **Subdivision:** each face recursively quad-subdivides based on distance to camera and screen-space error metric.
- **LOD seam handling:** constrained subdivision differences (max 1 level between neighbors); stitched skirts hide cracks.

Alternative considered: icosahedron (20 triangular faces). Rejected because quad grids parallelize better on the RX 580's GCN architecture.

## 2. Terrain generation

### 2.1 Height function

Deterministic, pure, queryable at any direction on the sphere:

```cpp
f32 planet_height(Vec3_f64 direction, u64 seed) noexcept;  // See 13_PROCEDURAL_UNIVERSE.md
```

### 2.2 GPU compute mesh generation

Terrain meshes are generated entirely on the GPU via compute shaders:

```glsl
#version 460
layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0) uniform Uniforms {
    mat4  view_proj;
    dvec3 planet_center;
    f64   planet_radius;
    u64   seed;
    i32   lod_level;
};

layout(binding = 1) buffer VertexBuffer {
    Vertex vertices[];
};

void main() {
    ivec2 grid_pos = ivec2(gl_GlobalInvocationID.xy);
    vec3 direction = compute_direction_from_grid(grid_pos, lod_level);
    f32 height = compute_planet_height(direction, planet_radius, seed);
    dvec3 position = planet_center + dvec3(direction) * (planet_radius + height);

    uint idx = grid_pos.y * grid_size.x + grid_pos.x;
    vertices[idx] = Vertex(
        vec3(position - camera_origin),   // converted to float for rendering
        compute_normal(position),
        compute_uv(direction)
    );
}
```

The double-precision planet center minus the camera origin is converted to `float` for the final vertex — this keeps precision where it matters (world-space) without giving up GPU performance on the hot path.

### 2.3 Hardware tessellation

Close to the camera, tessellation shaders add geometry detail without CPU cost. Configurable cap on tessellation level — RX 580's geometry shader budget is real, not infinite.

## 3. Cave systems (3D volume)

Caves are generated using 3D Worley noise and cellular automata:

```cpp
f32 cave_density(Vec3_f64 world_pos, u64 seed) noexcept {
    f32 cave  = worley3d(world_pos * 0.1,  seed);
    f32 erode = perlin3d(world_pos * 0.05, seed + 0xABCD);
    return cave * erode;
}
```

**Marching Cubes** extracts the iso-surface. Runs on the compute queue; produces mesh chunks on-demand.

## 4. Oceans and water

### 4.1 Ocean sphere

A fixed-height ocean sphere is rendered at sea level per planet.

- **Surface:** FFT-based deep-water wave simulation (Tier B — gated on RX 580 compute budget).
- **Underwater:** volumetric fog, caustics, light absorption by depth.
- **Shoreline:** dynamic foam and wave breaking based on depth gradient.

### 4.2 Rivers and lakes

Generated via **hydraulic erosion simulation** as a one-time planet-generation step (cached):

```cpp
void simulate_hydraulic_erosion(HeightMap& hm, u64 seed, int iterations) noexcept;
```

Produces river networks and lake basins that are then baked into the planet's height function's secondary channel.

## 5. Vegetation instancing

Flora is rendered with GPU instancing + LOD:

| LOD | Representation        | Instance count per chunk |
| --- | --------------------- | ------------------------ |
| 0   | Full 3D (low-poly)    | up to 2 000              |
| 1   | Simplified mesh       | up to 1 000              |
| 2   | Billboard             | up to 500                |
| 3   | Impostor texture      | up to 200                |

Indirect draw calls via `vkCmdDrawIndexedIndirect`; culling and LOD selection in a compute shader.

**Reference style (realistic low-poly):** tree meshes are 200–1500 triangles with flat-shaded canopy clusters and clean silhouettes. No alpha-test leaf cards at LOD 0.

## 6. Occlusion culling

Three layers:

1. **View-frustum culling** — compute shader on the culling queue.
2. **Hierarchical Z-buffer occlusion culling** — software, per-cluster.
3. **Hardware occlusion queries** — optional for fine-grained cases (rare).

## 7. Floating camera origin

To maintain `float32` precision in the render pipeline at planetary scale:

```cpp
void update_floating_origin(Vec3_f64 camera_pos) noexcept {
    if (length_squared(camera_pos) > k_origin_threshold_sq) {
        Vec3_f64 offset = camera_pos;
        for (auto& entity : world.entities()) {
            entity.transform.position -= offset;   // float64 -= float64
        }
        camera_pos -= offset;
        world.origin_shift.emit(offset);  // event for subsystems that cache world positions
    }
}
```

The event is consumed by physics, audio, and any subsystem that holds cached world-space positions.

## 8. Performance targets (RX 580)

| Scenario                                   | Target                 |
| ------------------------------------------ | ---------------------- |
| Standing on Earth-class planet, 2 km view  | ≥ 60 FPS @ 1080p       |
| Low orbit, atmospheric scattering + clouds | ≥ 60 FPS @ 1080p       |
| Chunk stream-in burst (5 chunks/frame)     | No single-frame stall  |
| LOD transitions                            | Invisible to the eye   |
| Planet-radius transition (float precision) | No visible jitter      |

---

*Planets are spheres. Precision is `f64`. Geometry is low-poly. Lighting is realistic. The horizon curves.*
