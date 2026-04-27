#pragma once
// editor/viewport/editor_camera.hpp
// Orbit / Fly / Ortho editor camera.
// Spec ref: Phase 7 §6.3.
// When the viewport is hovered, input is sampled from Dear ImGui (mouse delta,
// buttons, WASD) so navigation respects ImGui capture; GLFW remains the
// scroll source via EditorApplication's callback. Gizmo / frame hotkeys (W/E/R,
// Space, F) are gated in `ViewportPanel::on_event` when the Viewport dock has
// ImGui focus or hover (W1C.2) so GLFW key events do not fire while typing elsewhere.

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

namespace gw::editor {

enum class CameraMode { Orbit, Fly, Ortho };
enum class OrthoAxis  { X, Y, Z };

struct CameraState {
    glm::vec3  position    = {0.f, 2.f, 6.f};
    glm::vec3  target      = {0.f, 0.f, 0.f};
    float      fov_y_deg   = 60.f;
    float      near_z      = 0.1f;
    float      far_z       = 10000.f;
    CameraMode mode        = CameraMode::Orbit;
    OrthoAxis  ortho_axis  = OrthoAxis::Y;
    float      orbit_dist  = 6.f;
    float      orbit_yaw   = 0.f;   // radians
    float      orbit_pitch = 0.4f;  // radians (positive = above target)
    float      fly_yaw     = 0.f;
    float      fly_pitch   = 0.f;
    float      fly_speed   = 5.f;   // m/s
};

class EditorCamera {
public:
    EditorCamera() = default;

    // Call every frame inside the viewport ImGui window.
    // viewport_hovered: true if the imgui window has mouse focus.
    void update(float dt, bool viewport_hovered, GLFWwindow* window);

    // Frame the selection: point orbit target at `world_pos`.
    void frame(glm::vec3 world_pos, float radius = 1.f);

    // Hotkey: switch to ortho axis (X/Y/Z keys from viewport).
    void set_ortho(OrthoAxis axis);
    void set_mode(CameraMode m);

    [[nodiscard]] glm::mat4 view()           const noexcept;
    [[nodiscard]] glm::mat4 projection(float aspect) const noexcept;
    [[nodiscard]] glm::mat4 view_projection(float aspect) const noexcept;

    [[nodiscard]] glm::vec3 position()  const noexcept { return state_.position; }
    [[nodiscard]] glm::vec3 target()    const noexcept { return state_.target; }
    [[nodiscard]] CameraMode mode()     const noexcept { return state_.mode; }

    [[nodiscard]] const CameraState& state() const noexcept { return state_; }
    CameraState& state() noexcept { return state_; }

private:
    void update_orbit(float dt, bool hovered, GLFWwindow* w);
    void update_fly(float dt, bool hovered, GLFWwindow* w);
    void update_ortho(float dt, bool hovered, GLFWwindow* w);

    CameraState state_;
    bool        rmb_down_ = false;
    bool        mmb_down_ = false;
};

}  // namespace gw::editor
