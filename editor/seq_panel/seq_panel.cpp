// editor/seq_panel/seq_panel.cpp

#include "seq_panel.hpp"

#include "engine/memory/arena_allocator.hpp"
#include "engine/scene/seq/gwseq_format.hpp"
#include "engine/scene/seq/seq_camera.hpp"
#include "engine/scene/seq/sequencer_world.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace gw::editor::seq {

namespace {

using gw::seq::GwseqTrackType;
using gw::seq::GwseqTrackView;
using gw::seq::SeqPlayerComponent;
using gw::seq::Vec3_f64;

}  // namespace

SeqPanel::SeqPanel() = default;

void SeqPanel::render_toolbar(EditorContext& ctx) {
    if (ctx.world == nullptr || ctx.sequencer == nullptr) {
        ImGui::TextUnformatted("SequencerWorld not bound");
        return;
    }
    gw::ecs::Entity player{};
    ctx.world->for_each<SeqPlayerComponent>([&](const gw::ecs::Entity e, const SeqPlayerComponent&) {
        player = e;
    });
    if (!ctx.world->is_alive(player)) {
        ImGui::TextUnformatted("No SeqPlayerComponent in world");
        return;
    }
    auto* pl = ctx.world->get_component<SeqPlayerComponent>(player);
    if (pl == nullptr) return;

    if (ImGui::Button("Play")) {
        pl->playing = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        pl->playing = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        pl->playing         = false;
        pl->play_head_frame = 0.0;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &pl->loop);
}

void SeqPanel::render_timeline(EditorContext& ctx) {
    if (ctx.sequencer == nullptr) return;
    if (!active_seq_.valid()) {
        ImGui::TextUnformatted("No active sequence — use BLD seq.create_sequence or load .gwseq");
        return;
    }
    gw::seq::GwseqReader* r = ctx.sequencer->reader_for(active_seq_);
    if (r == nullptr) {
        ImGui::TextUnformatted("Sequence not registered on SequencerWorld");
        return;
    }
    const std::uint32_t dur  = r->header().duration_frames;
    const std::uint32_t fps  = r->header().frame_rate;
    double ph = 0.0;
    if (ctx.world) {
        ctx.world->for_each<SeqPlayerComponent>([&](const gw::ecs::Entity, const SeqPlayerComponent& p) {
            ph = p.play_head_frame;
        });
    }
    float phf = static_cast<float>(ph);
    if (ImGui::SliderFloat("Scrub", &phf, 0.f, static_cast<float>(std::max(1u, dur)), "%.1f") && ctx.world) {
        ctx.world->for_each<SeqPlayerComponent>([&](const gw::ecs::Entity, SeqPlayerComponent& p) {
            p.play_head_frame = static_cast<double>(phf);
        });
    }
    // Timeline canvas
    const ImVec2 origin   = ImGui::GetCursorScreenPos();
    const float  row_h    = 24.f;
    const float  label_w  = 120.f;
    const float  time_w   = std::max(200.f, ImGui::GetContentRegionAvail().x - label_w);
    const ImU32 line_col  = ImGui::GetColorU32(ImGuiCol_Text, 0.6f);
    const ImU32 play_col  = IM_COL32(255, 120, 40, 255);
    const ImU32 key_col   = IM_COL32(200, 200, 255, 255);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    int         ti = 0;
    for (const auto& tr : r->tracks()) {
        const float y0 = origin.y + static_cast<float>(ti) * row_h;
        char        buf[64];
        std::snprintf(buf, sizeof(buf), "id %u", tr.track_id);
        ImGui::SetCursorScreenPos(ImVec2(origin.x, y0));
        ImGui::TextUnformatted(buf);
        // Row background
        dl->AddRectFilled(ImVec2(origin.x + label_w, y0), ImVec2(origin.x + label_w + time_w, y0 + row_h - 2.f),
                          ImGui::GetColorU32(ImGuiCol_FrameBg, 0.4f), 2.f);
        // Keyframe diamonds
        gw::memory::ArenaAllocator arena(32u * 1024u);
        if (tr.track_type == GwseqTrackType::Transform) {
            GwseqTrackView<Vec3_f64> v3{};
            if (const auto rr = r->read_track(tr.track_id, arena, v3); rr.has_value() && !v3.empty()) {
                for (const auto& kf : v3.frames) {
                    const float x =
                        origin.x + label_w + static_cast<float>(kf.frame) * timeline_zoom_ * 0.1f;
                    const float cx = std::clamp(x, origin.x + label_w, origin.x + label_w + time_w);
                    dl->AddNgonFilled(ImVec2(cx, y0 + row_h * 0.5f), 5.f, key_col, 4);
                }
            } else {
                arena.reset();
            }
        }
        arena.reset();
        ++ti;
    }
    // Playhead
    const float px = origin.x + label_w + static_cast<float>(ph) * timeline_zoom_ * 0.1f;
    dl->AddLine(ImVec2(px, origin.y), ImVec2(px, origin.y + static_cast<float>(std::max(1, ti)) * row_h),
                play_col, 2.f);

    ImGui::Dummy(ImVec2(time_w + label_w, static_cast<float>(std::max(1, ti)) * row_h + 8.f));
    ImGui::TextDisabled("fps=%u  duration=%u f", fps, dur);
    (void)line_col;
}

void SeqPanel::render_camera_preview(EditorContext& ctx) const {
    if (ctx.cinematic == nullptr) {
        ImGui::TextUnformatted("CinematicCameraSystem not bound");
        return;
    }
    if (ctx.scene_color_descriptor == nullptr) {
        ImGui::TextUnformatted("No scene color descriptor (viewport not ready)");
        return;
    }
    const auto& v = ctx.cinematic->output();
    if (v.valid) {
        ImGui::Image(ctx.scene_color_descriptor, ImVec2(240.f, 135.f));
    } else {
        ImGui::TextUnformatted("No active cinematic camera");
    }
}

void SeqPanel::on_imgui_render(EditorContext& ctx) {
    if (!visible_) return;
    ImGui::SetNextWindowSize(ImVec2(720.f, 420.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }
    render_toolbar(ctx);
    ImGui::Separator();
    render_timeline(ctx);
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Camera preview", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_camera_preview(ctx);
    }
    ImGui::End();
}

}  // namespace gw::editor::seq
