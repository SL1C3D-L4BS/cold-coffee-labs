// editor/panels/audit/bake_panel.cpp — Wave 1: live gw_cook capture + cancel.

#include "editor/panels/audit/bake_panel.hpp"

#include "engine/platform/process.hpp"

#include <imgui.h>

#include <cstdio>
#include <filesystem>

namespace gw::editor::panels::audit {
namespace {

[[nodiscard]] std::filesystem::path cook_executable_path() {
    namespace fs = std::filesystem;
    const std::string exe = gw::platform::current_executable_path();
    if (exe.empty()) {
        return {};
    }
    const fs::path dir = fs::path{exe}.parent_path();
#if defined(_WIN32)
    const fs::path cook = dir / "gw_cook.exe";
#else
    const fs::path cook = dir / "gw_cook";
#endif
    return cook;
}

} // namespace

BakePanel::~BakePanel() {
    cancel_.store(true, std::memory_order_relaxed);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void BakePanel::append_log(std::string chunk) {
    std::lock_guard lock{log_mu_};
    if (log_.size() > 1'000'000) {
        log_.erase(0, log_.size() / 2);
    }
    log_ += chunk;
}

void BakePanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    const auto cook_exe = cook_executable_path();
    if (cook_exe.empty() || !std::filesystem::exists(cook_exe)) {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.45f, 1.f),
            "gw_cook not found beside the editor binary (%s expected).",
            cook_exe.string().c_str());
        ImGui::TextDisabled("Build the cook CLI target and place it in the same output directory as gw_editor.");
        ImGui::End();
        return;
    }

    if (!ctx.project_root) {
        ImGui::TextDisabled("Open a project with a content/ tree to run the cooker.");
        ImGui::End();
        return;
    }

    const std::filesystem::path content_dir = *ctx.project_root / "content";
    std::error_code             ec;
    if (!std::filesystem::is_directory(content_dir, ec)) {
        ImGui::TextColored(ImVec4(1.f, 0.55f, 0.35f, 1.f), "Missing directory: %s", content_dir.string().c_str());
        ImGui::End();
        return;
    }

    ImGui::Text("Cooker: %s", cook_exe.string().c_str());
    ImGui::Text("Input:  %s", content_dir.string().c_str());
    ImGui::Separator();
    ImGui::Checkbox("Force re-cook", &force_cook_);
    ImGui::SameLine();
    ImGui::Checkbox("Verbose stdout", &verbose_cook_);
    ImGui::InputText("Filter glob (optional)", filter_glob_.data(), filter_glob_.size());

    const bool busy = running_.load(std::memory_order_relaxed);
    if (busy) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Run gw_cook##bake_start") && !busy) {
        log_.clear();
        cancel_.store(false, std::memory_order_relaxed);
        running_.store(true, std::memory_order_relaxed);
        const std::filesystem::path cook_copy     = cook_exe;
        const std::filesystem::path content_copy  = content_dir;
        const std::filesystem::path project_copy  = *ctx.project_root;
        const bool                  force         = force_cook_;
        const bool                  verbose       = verbose_cook_;
        const std::string           filter{filter_glob_.data()};

        if (worker_.joinable()) {
            worker_.join();
        }
        worker_ = std::thread([this, cook_copy, content_copy, project_copy, force, verbose, filter]() {
            std::vector<std::string> argv;
            if (force) {
                argv.emplace_back("--force");
            }
            if (verbose) {
                argv.emplace_back("--verbose");
            }
            if (!filter.empty()) {
                argv.emplace_back("--filter");
                argv.push_back(filter);
            }
            const auto manifest_path = project_copy / ".greywater" / "last_cook_manifest.json";
            std::error_code            mec;
            std::filesystem::create_directories(manifest_path.parent_path(), mec);
            argv.emplace_back("--manifest");
            argv.push_back(manifest_path.string());
            argv.push_back(content_copy.string());

            const auto res = gw::platform::run_command_capture(cook_copy.string(), argv, &cancel_);
            append_log(res.combined_output);
            if (!res.combined_output.empty() && res.combined_output.back() != '\n') {
                append_log("\n");
            }
            {
                char tail[128];
                (void)std::snprintf(tail, sizeof tail, "--- gw_cook finished (exit %d) ---\n", res.exit_code);
                append_log(tail);
            }
            last_exit_.store(res.exit_code, std::memory_order_relaxed);
            running_.store(false, std::memory_order_relaxed);
        });
    }
    if (busy) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel##bake_cancel")) {
            cancel_.store(true, std::memory_order_relaxed);
        }
    }

    ImGui::Text("Last exit code: %d", last_exit_.load(std::memory_order_relaxed));
    ImGui::Separator();
    ImGui::BeginChild("bake_log", ImVec2(0, -4.f), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard lock{log_mu_};
        ImGui::TextUnformatted(log_.c_str(), log_.c_str() + log_.size());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f) {
        ImGui::SetScrollHereY(1.f);
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace gw::editor::panels::audit
