// editor/viewport/editor_camera.cpp
#include "editor_camera.hpp"

#include <imgui.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <algorithm>
#include <cmath>

namespace gw::editor {

namespace {
constexpr float kMinPitch   = -1.55f;   // ~-89°
constexpr float kMaxPitch   =  1.55f;
[[maybe_unused]] constexpr float kMinDist   = 0.05f;
[[maybe_unused]] constexpr float kOrthoZoom = 10.f; // initial ortho half-size (Phase 10 ortho polish)
}

// ---------------------------------------------------------------------------
glm::mat4 EditorCamera::view() const noexcept {
    return glm::lookAt(state_.position, state_.target, glm::vec3{0.f, 1.f, 0.f});
}

glm::mat4 EditorCamera::projection(float aspect) const noexcept {
    if (state_.mode == CameraMode::Ortho) {
        float h = state_.orbit_dist;
        float w = h * aspect;
        return glm::ortho(-w, w, -h, h, -state_.far_z, state_.far_z);
    }
    return glm::perspective(glm::radians(state_.fov_y_deg),
                             aspect,
                             state_.near_z, state_.far_z);
}

glm::mat4 EditorCamera::view_projection(float aspect) const noexcept {
    return projection(aspect) * view();
}

// ---------------------------------------------------------------------------
void EditorCamera::frame(glm::vec3 world_pos, float radius) {
    state_.target     = world_pos;
    state_.orbit_dist = radius * 3.f;
    // Recompute position from spherical coords.
    float sp = std::sin(state_.orbit_pitch);
    float cp = std::cos(state_.orbit_pitch);
    float sy = std::sin(state_.orbit_yaw);
    float cy = std::cos(state_.orbit_yaw);
    state_.position = state_.target +
        glm::vec3{cp * sy, sp, cp * cy} * state_.orbit_dist;
}

void EditorCamera::set_ortho(OrthoAxis axis) {
    state_.mode       = CameraMode::Ortho;
    state_.ortho_axis = axis;
    switch (axis) {
    case OrthoAxis::X:
        state_.position = state_.target + glm::vec3{state_.orbit_dist, 0.f, 0.f};
        break;
    case OrthoAxis::Y:
        state_.position = state_.target + glm::vec3{0.f, state_.orbit_dist, 0.f};
        break;
    case OrthoAxis::Z:
        state_.position = state_.target + glm::vec3{0.f, 0.f, state_.orbit_dist};
        break;
    }
}

void EditorCamera::set_mode(CameraMode m) {
    state_.mode = m;
}

// ---------------------------------------------------------------------------
void EditorCamera::update(float dt, bool viewport_hovered, GLFWwindow* window) {
    switch (state_.mode) {
    case CameraMode::Orbit: update_orbit(dt, viewport_hovered, window); break;
    case CameraMode::Fly:   update_fly  (dt, viewport_hovered, window); break;
    case CameraMode::Ortho: update_ortho(dt, viewport_hovered, window); break;
    }
}

void EditorCamera::update_orbit(float dt, bool hovered, GLFWwindow* w) {
    (void)w;
    if (!hovered) { rmb_down_ = mmb_down_ = false; return; }

    const ImGuiIO& io = ImGui::GetIO();
    const double   dx = static_cast<double>(io.MouseDelta.x);
    const double   dy = static_cast<double>(io.MouseDelta.y);

    const bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool mmb = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (rmb && rmb_down_) {
        state_.orbit_yaw   += static_cast<float>(dx) * 0.005f;
        state_.orbit_pitch -= static_cast<float>(dy) * 0.005f;
        state_.orbit_pitch  = std::clamp(state_.orbit_pitch, kMinPitch, kMaxPitch);
    }
    rmb_down_ = rmb;

    if (mmb && mmb_down_) {
        glm::mat4 v  = view();
        glm::vec3 right  = glm::vec3{v[0][0], v[1][0], v[2][0]};
        glm::vec3 up     = glm::vec3{v[0][1], v[1][1], v[2][1]};
        float pan_speed  = state_.orbit_dist * 0.001f;
        state_.target   -= right * static_cast<float>(dx) * pan_speed;
        state_.target   += up    * static_cast<float>(dy) * pan_speed;
    }
    mmb_down_ = mmb;

    // Scroll → dolly.
    // ImGui captures scroll; we rely on the viewport panel setting io.WantCaptureMouse
    // to false when the viewport is hovered. Read the raw GLFW scroll via the
    // EditorApplication's scroll accumulator.
    // (Scroll delta injected from the EditorApplication's GLFW callback.)

    // Rebuild position from spherical coords.
    float sp = std::sin(state_.orbit_pitch);
    float cp = std::cos(state_.orbit_pitch);
    float sy = std::sin(state_.orbit_yaw);
    float cy = std::cos(state_.orbit_yaw);
    state_.position = state_.target +
        glm::vec3{cp * sy, sp, cp * cy} * state_.orbit_dist;

    (void)dt;
}

void EditorCamera::update_fly(float dt, bool hovered, GLFWwindow* w) {
    (void)w;
    if (!hovered) { rmb_down_ = false; return; }

    const ImGuiIO& io = ImGui::GetIO();
    const double   dx = static_cast<double>(io.MouseDelta.x);
    const double   dy = static_cast<double>(io.MouseDelta.y);

    const bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (rmb) {
        if (rmb_down_) {
            state_.fly_yaw   += static_cast<float>(dx) * 0.003f;
            state_.fly_pitch -= static_cast<float>(dy) * 0.003f;
            state_.fly_pitch  = std::clamp(state_.fly_pitch, kMinPitch, kMaxPitch);
        }
        rmb_down_ = true;

        // WASD + QE movement.
        float speed = state_.fly_speed;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
            speed *= 4.f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            speed *= 0.25f;

        glm::vec3 fwd{
            std::cos(state_.fly_pitch) * std::sin(state_.fly_yaw),
            std::sin(state_.fly_pitch),
            std::cos(state_.fly_pitch) * std::cos(state_.fly_yaw)};
        glm::vec3 right  = glm::normalize(glm::cross(fwd, {0.f, 1.f, 0.f}));
        glm::vec3 up{0.f, 1.f, 0.f};

        float  s = speed * dt;
        if (ImGui::IsKeyDown(ImGuiKey_W)) state_.position += fwd * s;
        if (ImGui::IsKeyDown(ImGuiKey_S)) state_.position -= fwd * s;
        if (ImGui::IsKeyDown(ImGuiKey_A)) state_.position -= right * s;
        if (ImGui::IsKeyDown(ImGuiKey_D)) state_.position += right * s;
        if (ImGui::IsKeyDown(ImGuiKey_E)) state_.position += up * s;
        if (ImGui::IsKeyDown(ImGuiKey_Q)) state_.position -= up * s;

        state_.target = state_.position + fwd;
    } else {
        rmb_down_ = false;
    }
}

void EditorCamera::update_ortho(float dt, bool hovered, GLFWwindow* w) {
    (void)w;
    if (!hovered) { mmb_down_ = false; return; }

    const ImGuiIO& io = ImGui::GetIO();
    const double   dx = static_cast<double>(io.MouseDelta.x);
    const double   dy = static_cast<double>(io.MouseDelta.y);

    const bool mmb = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    if (mmb && mmb_down_) {
        float pan = state_.orbit_dist * 0.002f;
        switch (state_.ortho_axis) {
        case OrthoAxis::Y:
            state_.target.x -= static_cast<float>(dx) * pan;
            state_.target.z += static_cast<float>(dy) * pan;
            break;
        case OrthoAxis::X:
            state_.target.z -= static_cast<float>(dx) * pan;
            state_.target.y -= static_cast<float>(dy) * pan;
            break;
        case OrthoAxis::Z:
            state_.target.x -= static_cast<float>(dx) * pan;
            state_.target.y -= static_cast<float>(dy) * pan;
            break;
        }
        set_ortho(state_.ortho_axis);  // rebuild position
    }
    mmb_down_ = mmb;
    (void)dt;
    (void)w;
}

}  // namespace gw::editor
