// editor/vscript/vscript_panel.cpp
#include "vscript_panel.hpp"

#include "engine/vscript/parser.hpp"

#include "editor/theme/graph_theme.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <charconv>
#include <cstdio>
#include <fstream>
#include <variant>

namespace gw::editor {

namespace {

// Canonical demo graph used to seed a fresh panel and to power the
// "Reset demo" recovery button. Kept in a single place so the seed text
// and the reset text can never drift out of sync.
constexpr const char* kDemoSeedText =
    "graph demo {\n"
    "  input a : int = 3\n"
    "  input b : int = 4\n"
    "  output sum : int\n"
    "  node 1 : get_input {\n"
    "    in name : string = \"a\"\n"
    "    out value : int\n"
    "  }\n"
    "  node 2 : get_input {\n"
    "    in name : string = \"b\"\n"
    "    out value : int\n"
    "  }\n"
    "  node 3 : add {\n"
    "    in lhs : int\n"
    "    in rhs : int\n"
    "    out value : int\n"
    "  }\n"
    "  node 4 : set_output {\n"
    "    in name : string = \"sum\"\n"
    "    in value : int\n"
    "  }\n"
    "  edge 1.value -> 3.lhs\n"
    "  edge 2.value -> 3.rhs\n"
    "  edge 3.value -> 4.value\n"
    "}\n";

// Pin id packing: ImNodes uses a flat int space. We pack
// (node_id << 16) | (pin_index << 1) | (is_output) so every pin is
// uniquely addressable as an int.
//
// Limit: 0xFFFF = 65535 nodes, 0x7FFF = 32767 pins per node — far beyond
// what a hand-authored graph will ever reach.
int make_pin_id(std::uint32_t node, std::uint16_t pin_index, bool is_output) {
    return static_cast<int>((node << 16) |
                            (static_cast<std::uint32_t>(pin_index) << 1) |
                            (is_output ? 1u : 0u));
}

int make_node_id(std::uint32_t n) { return static_cast<int>(n); }

int make_link_id(std::size_t i) { return 0x1000000 + static_cast<int>(i); }

const char* kind_color_tag(gw::vscript::NodeKind k) {
    using gw::vscript::NodeKind;
    switch (k) {
        case NodeKind::Const:     return "const";
        case NodeKind::GetInput:  return "input";
        case NodeKind::SetOutput: return "output";
        case NodeKind::Add:
        case NodeKind::Sub:
        case NodeKind::Mul:       return "arith";
        case NodeKind::Select:    return "control";
    }
    return "unknown";
}

std::string format_value(const gw::vscript::Value& v) {
    using namespace gw::vscript;
    struct Visitor {
        std::string operator()(std::int64_t x) const { return std::to_string(x); }
        std::string operator()(double x) const {
            char b[32];
            std::snprintf(b, sizeof(b), "%.6g", x);
            return b;
        }
        std::string operator()(bool x) const { return x ? "true" : "false"; }
        std::string operator()(const std::string& s) const { return "\"" + s + "\""; }
        std::string operator()(const Vec3Value& v) const {
            char b[64];
            std::snprintf(b, sizeof(b), "vec3(%.3g,%.3g,%.3g)", v.x, v.y, v.z);
            return b;
        }
        std::string operator()(std::monostate) const { return "<void>"; }
    };
    return std::visit(Visitor{}, v);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
VScriptPanel::VScriptPanel() {
    // Each VScriptPanel owns its own ImNodes context so position state is
    // scoped to the panel. Falling back to the global context would tangle
    // multiple script windows if the user opens two.
    ImNodesContext* ctx = ImNodes::CreateContext();
    imnodes_ctx_ = ctx;

    // Seed the editor with a tiny non-trivial demo script so the panel
    // shows something meaningful on first open. The user can paste over
    // this buffer at any time.
    (void)load_text(kDemoSeedText);
    // Reserve slack up-front so the very first keystroke in InputTextMultiline
    // does not have to trigger a CallbackResize round-trip — removes an entire
    // class of "buffer pointer churn" bugs during live editing.
    text_buf_.reserve(text_buf_.size() + 1024);
}

VScriptPanel::~VScriptPanel() {
    if (imnodes_ctx_) {
        ImNodes::DestroyContext(static_cast<ImNodesContext*>(imnodes_ctx_));
        imnodes_ctx_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Public entry points (also used by tests).
// ---------------------------------------------------------------------------
bool VScriptPanel::load_text(std::string_view text) {
    text_buf_ = std::string{text};
    parse_error_.clear();
    exec_error_.clear();
    last_result_.reset();
    auto g = gw::vscript::parse(text);
    if (!g) {
        parse_error_ = g.error().to_string();
        graph_.reset();
        return false;
    }
    const auto err = gw::vscript::validate(*g);
    if (!err.empty()) {
        parse_error_ = err;
        graph_.reset();
        return false;
    }
    graph_ = std::move(*g);
    ++generation_;
    // Default grid layout — the load_layout call below overwrites any node
    // we have a saved position for, so missing positions fall back to a
    // readable column-per-depth arrangement.
    layout_.clear();
    for (std::size_t i = 0; i < graph_->nodes.size(); ++i) {
        const auto& n = graph_->nodes[i];
        layout_[n.id] = {
            static_cast<float>(120 * (1 + (i % 4))),
            static_cast<float>(80  * (1 + (i / 4)))
        };
    }
    return true;
}

bool VScriptPanel::load_file(const std::string& path) {
    std::ifstream f{path, std::ios::binary | std::ios::ate};
    if (!f.good()) return false;
    const auto sz = f.tellg();
    f.seekg(0);
    std::string buf(static_cast<std::size_t>(sz), '\0');
    f.read(buf.data(), sz);
    if (!load_text(buf)) return false;
    current_path_ = path;
    // Look for sidecar layout next to the script.
    load_layout(path + ".layout.json");
    return true;
}

bool VScriptPanel::save_file(const std::string& path) const {
    if (!graph_) return false;
    std::string out = gw::vscript::serialize(*graph_);
    std::ofstream f{path, std::ios::binary};
    if (!f.good()) return false;
    f.write(out.data(), static_cast<std::streamsize>(out.size()));
    save_layout(path + ".layout.json");
    return true;
}

// ---------------------------------------------------------------------------
// Layout sidecar — minimal hand-rolled JSON so we avoid dragging nlohmann
// into editor-only code. The format is
//     { "<node_id>": [x, y], "<node_id>": [x, y] }
// Whitespace-tolerant, not strictly conforming JSON (no unicode escapes etc).
// ---------------------------------------------------------------------------
namespace {

bool is_ws(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

void skip_ws(std::string_view s, std::size_t& i) {
    while (i < s.size() && is_ws(s[i])) ++i;
}

} // namespace

void VScriptPanel::load_layout(const std::string& path) {
    std::ifstream f{path, std::ios::binary | std::ios::ate};
    if (!f.good()) return;
    const auto sz = f.tellg();
    f.seekg(0);
    std::string buf(static_cast<std::size_t>(sz), '\0');
    f.read(buf.data(), sz);
    std::string_view s = buf;
    std::size_t i = 0;
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '{') return;
    ++i;
    while (true) {
        skip_ws(s, i);
        if (i < s.size() && s[i] == '}') break;
        if (i >= s.size() || s[i] != '"') return;  // malformed
        ++i;
        auto key_begin = i;
        while (i < s.size() && s[i] != '"') ++i;
        if (i >= s.size()) return;
        std::string_view key = s.substr(key_begin, i - key_begin);
        ++i;
        skip_ws(s, i);
        if (i >= s.size() || s[i] != ':') return;
        ++i; skip_ws(s, i);
        if (i >= s.size() || s[i] != '[') return;
        ++i; skip_ws(s, i);
        auto num_begin = i;
        while (i < s.size() && s[i] != ',' && s[i] != ']') ++i;
        if (i >= s.size()) return;
        std::string xs{s.substr(num_begin, i - num_begin)};
        ++i; skip_ws(s, i);
        num_begin = i;
        while (i < s.size() && s[i] != ']') ++i;
        if (i >= s.size()) return;
        std::string ys{s.substr(num_begin, i - num_begin)};
        ++i; skip_ws(s, i);
        std::uint32_t node_id = 0;
        std::from_chars(key.data(), key.data() + key.size(), node_id);
        float xv = 0.f, yv = 0.f;
        try { xv = std::stof(xs); yv = std::stof(ys); }
        catch (...) { /* silently skip malformed entries */ }
        layout_[node_id] = {xv, yv};
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        break;
    }
}

void VScriptPanel::save_layout(const std::string& path) const {
    std::ofstream f{path, std::ios::binary};
    if (!f.good()) return;
    f << "{\n";
    bool first = true;
    for (const auto& [id, pos] : layout_) {
        if (!first) f << ",\n";
        f << "  \"" << id << "\": [" << pos.first << ", " << pos.second << "]";
        first = false;
    }
    f << "\n}\n";
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void VScriptPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    draw_toolbar();
    ImGui::Separator();

    // Left: text IR buffer (ground truth). Right: node graph projection.
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const float text_w  = avail_w * 0.38f;

    ImGui::BeginChild("vscript_text", {text_w, 0.f}, true);
    {
        ImGui::TextUnformatted("IR (ground-truth)");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.f);
        if (ImGui::SmallButton("Reset demo")) {
            (void)load_text(kDemoSeedText);
            text_buf_.reserve(text_buf_.size() + 1024);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Parse")) {
            (void)load_text(text_buf_);
        }
        // Multi-line editable buffer — grows dynamically via the resize
        // callback below. We pass `capacity()+1` as the buffer size
        // (`data()` is guaranteed null-terminated), and the
        // CallbackResize handler re-reserves whenever ImGui needs more
        // space than we currently have.
        ImGui::InputTextMultiline(
            "##vscript_text",
            text_buf_.data(),
            text_buf_.capacity() + 1,
            {-1.f, -1.f},
            ImGuiInputTextFlags_AllowTabInput |
            ImGuiInputTextFlags_CallbackResize,
            [](ImGuiInputTextCallbackData* data) -> int {
                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                    auto* s = static_cast<std::string*>(data->UserData);
                    // Reserve headroom so subsequent keystrokes don't
                    // thrash the allocator frame-by-frame.
                    if (static_cast<std::size_t>(data->BufSize) > s->capacity() + 1) {
                        s->reserve(static_cast<std::size_t>(data->BufSize));
                    }
                    s->resize(static_cast<std::size_t>(data->BufTextLen));
                    data->Buf = s->data();
                }
                return 0;
            },
            &text_buf_);
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("vscript_right", {0.f, 0.f}, true);
    draw_graph_canvas();
    ImGui::Separator();
    draw_preview_panel();
    ImGui::EndChild();

    ImGui::End();
}

void VScriptPanel::draw_toolbar() {
    if (ImGui::SmallButton("Save")) {
        std::string path = current_path_.empty()
            ? std::string{"content/untitled.gwvs"}
            : current_path_;
        (void)save_file(path);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("path:");
    ImGui::SameLine();
    ImGui::TextUnformatted(current_path_.empty() ? "<unsaved>" : current_path_.c_str());

    if (!parse_error_.empty()) {
        // Use a dedicated wrapped line so long diagnostics (with byte
        // values from the lexer) are never clipped by the toolbar width.
        ImGui::PushStyleColor(ImGuiCol_Text, gw::editor::theme::active_destructive_imgui());
        ImGui::TextWrapped("%s", parse_error_.c_str());
        ImGui::PopStyleColor();
    }
}

void VScriptPanel::draw_graph_canvas() {
    ImNodes::SetCurrentContext(static_cast<ImNodesContext*>(imnodes_ctx_));
    gw::editor::theme::apply_graph_theme_to_current_imnodes(
        gw::editor::theme::ThemeRegistry::instance().active().graph);
    ImNodes::BeginNodeEditor();

    if (!graph_) {
        ImNodes::EndNodeEditor();
        ImGui::TextDisabled("No valid graph — see Parse error above.");
        return;
    }

    // Push saved positions *before* drawing nodes so ImNodes can place them
    // on first frame. After the first frame, ImNodes remembers positions
    // internally; we only overwrite when the graph was just loaded.
    apply_layout();

    const auto& ggraph = gw::editor::theme::ThemeRegistry::instance().active().graph;
    for (const auto& node : graph_->nodes) {
        const int nid = make_node_id(node.id);
        gw::editor::theme::push_vscript_node_style(node.kind, ggraph);
        ImNodes::BeginNode(nid);

        ImNodes::BeginNodeTitleBar();
        ImGui::Text("%u : %s", node.id,
                    std::string{gw::vscript::node_kind_name(node.kind)}.c_str());
        if (!node.debug_name.empty()) ImGui::TextDisabled("%s", node.debug_name.c_str());
        ImGui::TextDisabled("%s", kind_color_tag(node.kind));
        ImNodes::EndNodeTitleBar();

        for (std::size_t i = 0; i < node.inputs.size(); ++i) {
            const auto& pin = node.inputs[i];
            ImNodes::BeginInputAttribute(
                make_pin_id(node.id, static_cast<std::uint16_t>(i), false));
            if (pin.literal.has_value()) {
                ImGui::Text("%s : %s = %s",
                            pin.name.c_str(),
                            std::string{gw::vscript::type_name(pin.type)}.c_str(),
                            format_value(*pin.literal).c_str());
            } else {
                ImGui::Text("%s : %s",
                            pin.name.c_str(),
                            std::string{gw::vscript::type_name(pin.type)}.c_str());
            }
            ImNodes::EndInputAttribute();
        }
        for (std::size_t i = 0; i < node.outputs.size(); ++i) {
            const auto& pin = node.outputs[i];
            ImNodes::BeginOutputAttribute(
                make_pin_id(node.id, static_cast<std::uint16_t>(i), true));
            ImGui::Text("%s : %s",
                        pin.name.c_str(),
                        std::string{gw::vscript::type_name(pin.type)}.c_str());
            ImNodes::EndOutputAttribute();
        }

        ImNodes::EndNode();
        gw::editor::theme::pop_vscript_node_style();
    }

    for (std::size_t i = 0; i < graph_->edges.size(); ++i) {
        const auto& e = graph_->edges[i];
        // Translate src/dst pin names to indices so we can recover the
        // packed ImNodes id.
        auto pin_index_of = [](const std::vector<gw::vscript::Pin>& pins,
                                std::string_view name) -> std::uint16_t {
            for (std::size_t i = 0; i < pins.size(); ++i) {
                if (pins[i].name == name) return static_cast<std::uint16_t>(i);
            }
            return 0;
        };
        const auto* src = graph_->find_node(e.src_node);
        const auto* dst = graph_->find_node(e.dst_node);
        if (!src || !dst) continue;
        const int src_id = make_pin_id(e.src_node,
            pin_index_of(src->outputs, e.src_pin), true);
        const int dst_id = make_pin_id(e.dst_node,
            pin_index_of(dst->inputs, e.dst_pin), false);
        ImNodes::Link(make_link_id(i), src_id, dst_id);
    }

    ImNodes::EndNodeEditor();
    // Pull positions back so "Save" persists the user's drag gestures. This
    // is cheap (per-node call) and only writes the in-memory map.
    harvest_layout();
}

void VScriptPanel::apply_layout() {
    if (!graph_) return;
    for (const auto& n : graph_->nodes) {
        auto it = layout_.find(n.id);
        if (it == layout_.end()) continue;
        ImNodes::SetNodeGridSpacePos(
            make_node_id(n.id),
            ImVec2{it->second.first, it->second.second});
    }
}

void VScriptPanel::harvest_layout() {
    if (!graph_) return;
    for (const auto& n : graph_->nodes) {
        const ImVec2 p = ImNodes::GetNodeGridSpacePos(make_node_id(n.id));
        layout_[n.id] = {p.x, p.y};
    }
}

void VScriptPanel::draw_preview_panel() {
    if (!graph_) return;
    ImGui::TextUnformatted("Interpreter preview");
    if (ImGui::SmallButton("Run")) {
        exec_error_.clear();
        auto r = gw::vscript::execute(*graph_);
        if (r) {
            last_result_ = std::move(*r);
        } else {
            last_result_.reset();
            exec_error_ = r.error().message;
        }
    }
    if (!exec_error_.empty()) {
        ImGui::TextColored(gw::editor::theme::active_destructive_imgui(), "exec error: %s",
                           exec_error_.c_str());
        return;
    }
    if (!last_result_) {
        ImGui::TextDisabled("click Run to evaluate");
        return;
    }
    if (ImGui::BeginTable("outputs", 2,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("output");
        ImGui::TableSetupColumn("value");
        ImGui::TableHeadersRow();
        for (const auto& [k, v] : last_result_->outputs) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(k.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(format_value(v).c_str());
        }
        ImGui::EndTable();
    }
}

}  // namespace gw::editor
