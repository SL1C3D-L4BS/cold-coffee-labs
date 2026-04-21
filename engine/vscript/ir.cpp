// engine/vscript/ir.cpp
#include "ir.hpp"

#include <array>
#include <string>
#include <unordered_set>

namespace gw::vscript {

namespace {

constexpr std::array<std::string_view, 6> kTypeNames = {
    "int", "float", "bool", "string", "vec3", "void",
};

constexpr std::array<std::string_view, 7> kKindNames = {
    "const", "get_input", "add", "sub", "mul", "select", "set_output",
};

} // namespace

std::string_view type_name(Type t) noexcept {
    const auto i = static_cast<std::size_t>(t);
    return i < kTypeNames.size() ? kTypeNames[i] : std::string_view{"?"};
}

std::optional<Type> parse_type(std::string_view s) noexcept {
    for (std::size_t i = 0; i < kTypeNames.size(); ++i) {
        if (kTypeNames[i] == s) return static_cast<Type>(i);
    }
    return std::nullopt;
}

std::string_view node_kind_name(NodeKind k) noexcept {
    const auto i = static_cast<std::size_t>(k);
    return i < kKindNames.size() ? kKindNames[i] : std::string_view{"?"};
}

std::optional<NodeKind> parse_node_kind(std::string_view s) noexcept {
    for (std::size_t i = 0; i < kKindNames.size(); ++i) {
        if (kKindNames[i] == s) return static_cast<NodeKind>(i);
    }
    return std::nullopt;
}

Type type_of(const Value& v) noexcept {
    struct Visitor {
        Type operator()(std::int64_t) const noexcept    { return Type::Int;    }
        Type operator()(double)       const noexcept    { return Type::Float;  }
        Type operator()(bool)         const noexcept    { return Type::Bool;   }
        Type operator()(const std::string&) const noexcept { return Type::String; }
        Type operator()(const Vec3Value&)   const noexcept { return Type::Vec3;   }
        Type operator()(std::monostate)     const noexcept { return Type::Void;   }
    };
    return std::visit(Visitor{}, v);
}

Value default_value(Type t) {
    switch (t) {
        case Type::Int:    return Value{std::int64_t{0}};
        case Type::Float:  return Value{0.0};
        case Type::Bool:   return Value{false};
        case Type::String: return Value{std::string{}};
        case Type::Vec3:   return Value{Vec3Value{}};
        case Type::Void:   return Value{std::monostate{}};
    }
    return Value{std::monostate{}};
}

// ---------------------------------------------------------------------------

const Node* Graph::find_node(std::uint32_t id) const noexcept {
    for (const auto& n : nodes) if (n.id == id) return &n;
    return nullptr;
}
Node* Graph::find_node(std::uint32_t id) noexcept {
    for (auto& n : nodes) if (n.id == id) return &n;
    return nullptr;
}

const IOPort* Graph::find_input(std::string_view name) const noexcept {
    for (const auto& p : inputs) if (p.name == name) return &p;
    return nullptr;
}
const IOPort* Graph::find_output(std::string_view name) const noexcept {
    for (const auto& p : outputs) if (p.name == name) return &p;
    return nullptr;
}
IOPort* Graph::find_output(std::string_view name) noexcept {
    for (auto& p : outputs) if (p.name == name) return &p;
    return nullptr;
}

// ---------------------------------------------------------------------------

namespace {

// DFS cycle detection using the classic white/grey/black colouring. Returns
// true if a cycle is reachable from `root`.
bool has_cycle_dfs(const Graph& /*g*/,
                   std::uint32_t root,
                   std::unordered_map<std::uint32_t, std::uint8_t>& colour,
                   const std::unordered_map<std::uint32_t, std::vector<std::uint32_t>>& adj) {
    // Iterative DFS. 1 = on-stack, 2 = done.
    std::vector<std::pair<std::uint32_t, std::size_t>> stack;
    stack.push_back({root, 0});
    colour[root] = 1;
    while (!stack.empty()) {
        auto& [node, idx] = stack.back();
        auto it = adj.find(node);
        const auto* succ = (it == adj.end()) ? nullptr : &it->second;
        if (succ && idx < succ->size()) {
            const auto next = (*succ)[idx++];
            const auto c = colour[next];
            if (c == 1) return true;
            if (c == 0) {
                colour[next] = 1;
                stack.push_back({next, 0});
            }
        } else {
            colour[node] = 2;
            stack.pop_back();
        }
    }
    return false;
}

} // namespace

std::string validate(const Graph& g) {
    // 1. Node ids unique.
    std::unordered_set<std::uint32_t> seen;
    for (const auto& n : g.nodes) {
        if (!seen.insert(n.id).second) {
            return "duplicate node id " + std::to_string(n.id);
        }
    }

    // 2. Every edge's endpoints exist with compatible types.
    for (const auto& e : g.edges) {
        const auto* src = g.find_node(e.src_node);
        const auto* dst = g.find_node(e.dst_node);
        if (!src) return "edge references unknown source node " +
                          std::to_string(e.src_node);
        if (!dst) return "edge references unknown dest node " +
                          std::to_string(e.dst_node);
        const Pin* src_pin = nullptr;
        for (const auto& p : src->outputs)
            if (p.name == e.src_pin) { src_pin = &p; break; }
        if (!src_pin) {
            return "edge from node " + std::to_string(e.src_node) +
                   " references unknown output pin '" + e.src_pin + "'";
        }
        const Pin* dst_pin = nullptr;
        for (const auto& p : dst->inputs)
            if (p.name == e.dst_pin) { dst_pin = &p; break; }
        if (!dst_pin) {
            return "edge to node " + std::to_string(e.dst_node) +
                   " references unknown input pin '" + e.dst_pin + "'";
        }
        if (src_pin->type != dst_pin->type) {
            return "edge type mismatch: node " +
                   std::to_string(e.src_node) + "." + e.src_pin + " (" +
                   std::string{type_name(src_pin->type)} + ") -> node " +
                   std::to_string(e.dst_node) + "." + e.dst_pin + " (" +
                   std::string{type_name(dst_pin->type)} + ")";
        }
    }

    // 3. Every input pin is either driven by an edge or has a literal.
    std::unordered_map<std::uint64_t, const Edge*> driven;
    for (const auto& e : g.edges) {
        const std::uint64_t key = (std::uint64_t{e.dst_node} << 32) |
                                  std::hash<std::string>{}(e.dst_pin);
        driven[key] = &e;
    }
    for (const auto& n : g.nodes) {
        for (const auto& p : n.inputs) {
            const std::uint64_t key = (std::uint64_t{n.id} << 32) |
                                      std::hash<std::string>{}(p.name);
            if (!p.literal.has_value() && !driven.count(key)) {
                return "input pin '" + p.name + "' on node " +
                       std::to_string(n.id) +
                       " has neither literal nor incoming edge";
            }
        }
    }

    // 4. No cycles. Build src→dst adjacency; DFS from every node.
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> adj;
    for (const auto& e : g.edges) adj[e.src_node].push_back(e.dst_node);
    std::unordered_map<std::uint32_t, std::uint8_t> colour;
    for (const auto& n : g.nodes) {
        if (colour[n.id] == 0 && has_cycle_dfs(g, n.id, colour, adj)) {
            return "graph contains a cycle through node " +
                   std::to_string(n.id);
        }
    }
    return {};
}

} // namespace gw::vscript
