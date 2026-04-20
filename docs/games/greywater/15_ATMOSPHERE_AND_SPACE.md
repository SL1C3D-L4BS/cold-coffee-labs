# 15 — Greywater Atmosphere & Space Transition

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Atmospheric scattering model

Greywater implements a **physically-based atmospheric scattering model** in a single compute pass that renders accurately from ground to space views **without** high-dimensional lookup tables and **without** artifacts.

### 1.1 Core algorithm

The model computes:

- **Rayleigh scattering** — molecular; produces blue sky.
- **Mie scattering** — aerosols; produces haze, sunset warmth.
- **Multiple scattering** — approximated via a cheap second-order term.
- **Absorption** — ozone, water vapor.

```glsl
vec3 atmospheric_scattering(
    vec3 ray_origin,
    vec3 ray_direction,
    vec3 sun_direction,
    AtmosphereParams params
);
```

**Why this method on RX 580:**
- Runs in a single compute dispatch; no LUT generation step.
- Dynamic parameters — atmosphere composition, cloud density, weather — update every frame without heavy pre-compute.
- ~2 ms at 1080p on Polaris in our reference implementation. Fits the budget.

### 1.2 Per-planet atmosphere parameters

```cpp
struct AtmosphereParams {
    f32 planet_radius;         // meters
    f32 atmosphere_thickness;  // meters
    vec3 rayleigh_coefficients;
    f32 mie_coefficient;
    f32 mie_anisotropy;        // forward scattering bias
    vec3 ozone_absorption;
    f32 sun_intensity;
    vec3 sun_color;
};
```

Each planet type in `docs/games/greywater/13_PROCEDURAL_UNIVERSE.md` has its own canonical atmosphere configuration. Desert planets have elevated Mie; toxic planets have sickly absorption spectra; Earth-like follows real-Earth parameters.

## 2. Volumetric clouds

Raymarched in a compute shader; sampled from a precomputed 3D Perlin-Worley density texture.

```glsl
vec4 raymarch_clouds(vec3 ray_origin, vec3 ray_direction, CloudParams p) {
    f32 t = 0.0;
    vec4 accumulated = vec4(0.0);

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 pos = ray_origin + ray_direction * t;
        f32 density = sample_cloud_density(pos);

        if (density > 0.0) {
            vec3 lighting = compute_cloud_lighting(pos, sun_direction);
            accumulated += vec4(lighting * density, density) * (1.0 - accumulated.a);
        }

        t += step_size(t);
        if (t > max_distance || accumulated.a > 0.99) break;
    }
    return accumulated;
}
```

RX 580 budget: 32 march steps at 1/4 resolution + upsample. Adaptive step size increases with distance.

## 3. Atmospheric layer model

Each planet has **five physical layers**. As the player ascends or descends, physics, audio, and visuals interpolate smoothly between them.

| Layer         | Altitude range    | Physics                                | Audio                 | Visual                               |
| ------------- | ----------------- | -------------------------------------- | --------------------- | ------------------------------------ |
| Troposphere   | 0 – 12 km         | Full drag, lift; weather; combat       | Normal                | Full scattering; clouds              |
| Stratosphere  | 12 – 50 km        | Thin air; reduced drag                 | Muffled               | Reduced scattering                   |
| Mesosphere    | 50 – 85 km        | Transition physics                     | Deep, rumbling only   | Minimal atmosphere                   |
| Thermosphere  | 85 – 600 km       | Orbital transition begins              | Almost silent         | Auroras, ionosphere glow             |
| Exosphere     | 600 km+           | Space; orbital mechanics dominate      | Vacuum (no sound)     | No atmosphere; starfield             |

```cpp
struct AtmosphericLayerCoefficients {
    f32 drag;            // 0.0 (vacuum) to 1.0 (sea level)
    f32 lift;            // same
    f32 heating_rate;    // 0.0 (sea level) to 1.0 (re-entry)
    f32 audio_lowpass_cutoff_hz;
};

AtmosphericLayerCoefficients sample_layer(f32 altitude_m, AtmosphereParams p) noexcept;
```

## 4. Re-entry physics

When a vehicle enters a planet's atmosphere at high velocity:

```cpp
struct ReEntryState {
    f32 velocity;                 // m/s
    f32 atmospheric_density;
    f32 heat_shield_integrity;    // 0.0 – 1.0
    f32 structural_integrity;     // 0.0 – 1.0
};

void update_reentry(ReEntryState& s, f32 dt) noexcept {
    // Heating proportional to v³ · √ρ
    f32 heating = std::pow(s.velocity, 3.0f) * std::sqrt(s.atmospheric_density);
    s.heat_shield_integrity -= heating * dt * k_shield_wear_rate;

    // Drag proportional to v² · ρ
    f32 drag = s.velocity * s.velocity * s.atmospheric_density * k_drag_coefficient;
    s.velocity -= drag * dt;

    // Structural stress from heating + aerodynamic forces
    f32 stress = heating * k_heating_stress + drag * k_drag_stress;
    s.structural_integrity -= stress * dt;
}
```

Plasma VFX activate when heating exceeds a threshold; blackbody-radiation shader tints ship hull from red through white. If `structural_integrity` reaches zero, the vehicle breaks apart (Jolt physics debris), and the player may eject.

## 5. Orbital mechanics

Simplified Newtonian gravity with real-time trajectory prediction. All positions and velocities are `f64`.

```cpp
struct OrbitalBody {
    f64      mass;      // kg
    f64      radius;    // meters
    Vec3_f64 position;
    Vec3_f64 velocity;
};

Vec3_f64 gravitational_acceleration(Vec3_f64 position, const OrbitalBody& body) noexcept {
    Vec3_f64 dir = body.position - position;
    f64 dist_sq = length_squared(dir);
    f64 dist = std::sqrt(dist_sq);
    f64 mag = G * body.mass / dist_sq;
    return (dir / dist) * mag;
}
```

**Trajectory preview** is rendered at orbital altitude by integrating the above at a coarse step for the next ~10 minutes of in-game time.

## 6. Space rendering

### 6.1 Starfield

Deterministic from the galaxy seed:

```glsl
vec4 render_starfield(vec3 view_direction, u64 galaxy_seed);
```

Stars are procedurally placed by hashing the view direction into a binning function, sampling the galaxy's star-density LUT.

### 6.2 Nebulae and distant galaxies

Background galaxies and nebulae are impostor sprites with parallax. Their positions are deterministic from the galaxy-cluster seed.

### 6.3 Planets at distance

- **Far distance:** impostor texture (cubemap per planet, content-hashed cache).
- **Medium distance:** procedural shader (silhouette + atmosphere).
- **Close approach:** full terrain mesh (§14).

Transitions between distance tiers are cross-faded over ~1 second of simulation time to hide popping.

## 7. Asteroid fields

Procedural asteroid belts rendered with GPU instancing. Each asteroid's rotation and orbit is animated on the GPU. Collision detection uses Jolt; resource harvesting is a gameplay concern.

---

*Ground to atmosphere to orbit to space is one simulation. No transitions. No "now you are in space" mode changes. The horizon curves away and keeps curving until you see the black.*
