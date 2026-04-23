#pragma once

#include <cstdint>
#include <string_view>

namespace gw::services::editor_copilot {

enum class CopilotToolId : std::uint16_t {
    ConceptToMaterial = 1,
    EncounterSuggest  = 2,
    ExploitDetect     = 3,
    SceneHeatmap      = 4,
    VoiceLineGenerate = 5,
    DialogueReach     = 6,
};

struct CopilotRequest {
    CopilotToolId    tool      = CopilotToolId::ConceptToMaterial;
    std::string_view input_json{};           // UTF-8 JSON, caller-owned
    std::uint64_t    request_id = 0;
};

struct CopilotResult {
    bool             ok         = false;
    std::string_view output_json{};
    std::uint64_t    elapsed_ns = 0;
};

} // namespace gw::services::editor_copilot
