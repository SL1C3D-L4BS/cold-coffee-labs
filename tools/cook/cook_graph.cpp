#include "cook_graph.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <queue>
#include <unordered_set>

namespace gw::tools::cook {

static const std::unordered_set<std::string> kCookableExts = {
    ".glb", ".gltf",
    ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr", ".exr"
};

CookGraph::CookGraph(std::filesystem::path input_root,
                     std::filesystem::path output_root,
                     CookPlatform platform,
                     CookConfig   config)
    : input_root_(std::move(input_root))
    , output_root_(std::move(output_root))
    , platform_(platform)
    , config_(config)
{}

bool CookGraph::is_cookable(const std::filesystem::path& ext) {
    return kCookableExts.count(ext.string()) > 0;
}

namespace {

void collect_gltf_uri_strings(const std::string& json, std::vector<std::string>& out) {
    static constexpr char kKey[] = "\"uri\"";
    std::size_t             p     = 0;
    while ((p = json.find(kKey, p)) != std::string::npos) {
        p += sizeof(kKey) - 1u;
        while (p < json.size() &&
               (json[p] == ' ' || json[p] == '\t' || json[p] == '\n' || json[p] == '\r')) {
            ++p;
        }
        if (p >= json.size() || json[p] != ':') {
            continue;
        }
        ++p;
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) {
            ++p;
        }
        if (p >= json.size() || json[p] != '"') {
            continue;
        }
        ++p;
        const std::size_t e = json.find('"', p);
        if (e == std::string::npos) {
            break;
        }
        out.emplace_back(json.substr(p, e - p));
        p = e;
    }
}

}  // namespace

std::size_t CookGraph::ensure_source_node(const std::filesystem::path& source) {
    const std::string key = std::filesystem::weakly_canonical(source).string();
    if (const auto it = source_to_index_.find(key); it != source_to_index_.end()) {
        return it->second;
    }
    add_asset(source);
    return source_to_index_.at(key);
}

void CookGraph::link_gltf_texture_dependencies() {
    namespace fs = std::filesystem;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].source.extension() != ".gltf") {
            continue;
        }
        std::ifstream f(nodes_[i].source);
        if (!f) {
            continue;
        }
        const std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::vector<std::string> uris;
        collect_gltf_uri_strings(json, uris);
        const fs::path base = nodes_[i].source.parent_path();
        for (const auto& u : uris) {
            if (u.rfind("data:", 0) == 0) {
                continue;
            }
            const fs::path abs = fs::weakly_canonical(base / fs::path(u));
            std::error_code  ec;
            if (!fs::is_regular_file(abs, ec) || !is_cookable(abs.extension())) {
                continue;
            }
            const std::size_t dep = ensure_source_node(abs);
            auto&             deps = nodes_[i].deps;
            if (std::find(deps.begin(), deps.end(), dep) == deps.end()) {
                deps.push_back(dep);
            }
        }
    }
}

std::filesystem::path CookGraph::compute_output(
    const std::filesystem::path& source) const
{
    // Mirror the directory structure: content/meshes/foo.glb → assets/meshes/foo.gwmesh
    auto rel = std::filesystem::relative(source, input_root_);

    // Map extension to cooked extension.
    const auto ext = rel.extension().string();
    std::string out_ext;
    if (ext == ".glb" || ext == ".gltf")  out_ext = ".gwmesh";
    else                                   out_ext = ".gwtex";

    auto out = output_root_ / rel;
    out.replace_extension(out_ext);
    return out;
}

void CookGraph::discover() {
    if (!std::filesystem::exists(input_root_)) return;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(input_root_))
    {
        if (!entry.is_regular_file()) continue;
        if (is_cookable(entry.path().extension())) {
            add_asset(entry.path());
        }
    }
    link_gltf_texture_dependencies();
}

void CookGraph::add_asset(const std::filesystem::path& source) {
    const std::string key = std::filesystem::weakly_canonical(source).string();
    if (source_to_index_.count(key)) return;

    CookNode node{};
    node.source = source;
    node.output = compute_output(source);
    node.cooked_path = std::filesystem::relative(node.output, output_root_).string();

    const std::size_t idx = nodes_.size();
    source_to_index_[key] = idx;
    nodes_.push_back(std::move(node));
}

void CookGraph::resolve_cache(void* /*cache*/) {
    // Phase 6 full: query CookCache DB here.  For now, all nodes need cooking.
    for (auto& n : nodes_) n.needs_cook = true;
}

AssetResult<std::vector<std::size_t>> CookGraph::topological_order() const {
    // Kahn's algorithm. Cross-asset edges are populated by
    // `link_gltf_texture_dependencies()` for `.gltf` image URIs; other sources
    // remain isolated unless future cookers append deps.
    const std::size_t N = nodes_.size();
    std::vector<uint32_t> indegree(N, 0);
    std::vector<std::vector<std::size_t>> adj(N);

    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t dep : nodes_[i].deps) {
            adj[dep].push_back(i);
            ++indegree[i];
        }
    }

    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < N; ++i) {
        if (indegree[i] == 0) q.push(i);
    }

    std::vector<std::size_t> order;
    order.reserve(N);
    while (!q.empty()) {
        const std::size_t u = q.front(); q.pop();
        order.push_back(u);
        for (std::size_t v : adj[u]) {
            if (--indegree[v] == 0) q.push(v);
        }
    }

    if (order.size() != N) {
        return std::unexpected(AssetError{AssetErrorCode::InvalidArgument,
                                          "cook graph has a dependency cycle"});
    }
    return order;
}

} // namespace gw::tools::cook
