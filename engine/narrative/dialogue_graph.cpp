// engine/narrative/dialogue_graph.cpp — Phase 22 scaffold.

#include "engine/narrative/dialogue_graph.hpp"

#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace gw::narrative {

const DialogueLine* DialogueGraph::find(DialogueLineId id) const noexcept {
    for (const DialogueLine& ln : lines_) {
        if (ln.id == id) {
            return &ln;
        }
    }
    return nullptr;
}

void parse_dialogue_graph_fuzz_harness(const std::span<const std::uint8_t> data) noexcept {
    if (data.empty()) {
        return;
    }
    std::string text;
    text.reserve(data.size());
    for (const std::uint8_t b : data) {
        if (b == '\n' || b == '\r') {
            text.push_back('\n');
        } else if (b >= 0x20U && b < 0x7FU) {
            text.push_back(static_cast<char>(b));
        } else if (b == '\t') {
            text.push_back(' ');
        }
    }
    std::vector<DialogueLine>       lines;
    std::vector<std::string>         pool;
    lines.reserve(8U);
    std::string_view                 cur{text};
    while (!cur.empty()) {
        const std::size_t br = cur.find('\n');
        const std::string_view line = (br == std::string_view::npos) ? cur : cur.substr(0, br);
        if (br == std::string_view::npos) {
            cur = {};
        } else {
            cur.remove_prefix(br + 1U);
        }
        if (line.empty()) {
            continue;
        }
        // `id|text` — id is decimal digits, text is the tail.
        const std::size_t bar = line.find('|');
        if (bar == std::string_view::npos) {
            continue;
        }
        const std::string_view id_s = line.substr(0, bar);
        const std::string_view rest = line.substr(bar + 1U);
        if (id_s.empty() || id_s.size() > 16U) {
            continue;
        }
        char   id_buf[20]{};
        (void)std::memcpy(id_buf, id_s.data(), id_s.size());
        char*  end  = nullptr;
        const unsigned long idv = std::strtoul(id_buf, &end, 10);
        if (end == id_buf || idv == 0UL) {
            continue;
        }
        pool.push_back(std::string{rest});
        DialogueLine dl{};
        dl.id    = DialogueLineId{static_cast<std::uint32_t>(idv > 0xFFFFFFFFUL ? 1UL : idv)};
        dl.text  = std::string_view{pool.back()};
        lines.push_back(dl);
        if (lines.size() > 32U) {
            break;
        }
    }
    if (lines.empty()) {
        return;
    }
    DialogueGraph g;
    g.attach_blob(std::span<const DialogueLine>{lines.data(), lines.size()});
    (void)g.find(lines[0].id);
    if (lines.size() > 1U) {
        (void)g.find(lines[lines.size() - 1U].id);
    }
}

} // namespace gw::narrative
