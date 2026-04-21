#pragma once
// tools/cook/cook_graph.hpp
// CookGraph — DAG of assets to cook with topological sort (Kahn's algorithm).
// Phase 6 spec §5.3.

#include "engine/assets/asset_types.hpp"
#include "engine/assets/asset_error.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace gw::tools::cook {

using gw::assets::AssetPath;
using gw::assets::CookKey;
using gw::assets::CookPlatform;
using gw::assets::CookConfig;
using gw::assets::AssetResult;
using gw::assets::AssetError;
using gw::assets::AssetErrorCode;

struct CookNode {
    std::filesystem::path source;
    std::filesystem::path output;
    AssetPath             cooked_path;   // relative output path
    std::vector<std::size_t> deps;       // indices into node list
    CookKey               cache_key;
    bool                  needs_cook = true;
};

struct CookStats {
    uint32_t total    = 0;
    uint32_t cooked   = 0;
    uint32_t cached   = 0;
    uint32_t errors   = 0;
    uint32_t wall_ms  = 0;
};

class CookGraph {
public:
    CookGraph(std::filesystem::path input_root,
               std::filesystem::path output_root,
               CookPlatform platform = CookPlatform::Host,
               CookConfig   config   = CookConfig::Release);

    // Discover all cookable source files under input_root.
    void discover();

    // Add a specific source file.
    void add_asset(const std::filesystem::path& source);

    // Mark nodes that already have a valid cache hit as needs_cook=false.
    void resolve_cache(/* CookCache& */ void* cache);

    // Kahn's topological sort (O(V+E)).
    // Returns an error if the graph contains a cycle.
    [[nodiscard]] AssetResult<std::vector<std::size_t>> topological_order() const;

    [[nodiscard]] const std::vector<CookNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::vector<CookNode>&       nodes()       noexcept { return nodes_; }

private:
    std::filesystem::path input_root_;
    std::filesystem::path output_root_;
    CookPlatform          platform_;
    CookConfig            config_;

    std::vector<CookNode> nodes_;
    std::unordered_map<std::string, std::size_t> source_to_index_;

    [[nodiscard]] std::filesystem::path compute_output(
        const std::filesystem::path& source) const;

    [[nodiscard]] static bool is_cookable(const std::filesystem::path& ext);
};

} // namespace gw::tools::cook
