#pragma once
// engine/vscript/ir.hpp
// Phase 8 week 045 — Visual Scripting typed IR.
//
// CLAUDE.md non-negotiable #14: the text IR is the ground-truth. Every node
// graph the editor shows is a projection of a Graph instance authored here.
// Authoring is the same whether the user types text or drags nodes — both
// paths converge on this data model before any interpreter / cook sees it.
//
// Design goals:
//   * Strongly typed pins — each connection carries a Type enum; the parser
//     rejects mismatched edges and the interpreter never has to coerce.
//   * Deterministic ordering — nodes and edges are stored in an insertion-
//     stable vector; golden-test files can rely on round-trip byte equality.
//   * No heap-thrashing in the hot path — the interpreter only reads from
//     vectors and a side-table of evaluated values.
//
// Non-goals (for Phase 8): generics, closures, module imports, cycles,
// concurrency. Graph is a DAG; cycles are rejected at parse time.

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace gw::vscript {

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
enum class Type : std::uint8_t {
    Int,
    Float,
    Bool,
    String,
    Vec3,
    Void,    // control-only edges carry Void
};

[[nodiscard]] std::string_view type_name(Type t) noexcept;

// Parse a type name ("int", "float", "bool", "string", "vec3", "void").
// Returns std::nullopt on unknown input.
[[nodiscard]] std::optional<Type> parse_type(std::string_view s) noexcept;

// ---------------------------------------------------------------------------
// Value — tagged union matching Type.
// ---------------------------------------------------------------------------
struct Vec3Value {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    friend bool operator==(const Vec3Value&, const Vec3Value&) = default;
};

using Value = std::variant<
    std::int64_t,  // Int
    double,        // Float
    bool,          // Bool
    std::string,   // String
    Vec3Value,     // Vec3
    std::monostate // Void
>;

[[nodiscard]] Type type_of(const Value& v) noexcept;
[[nodiscard]] Value default_value(Type t);

// ---------------------------------------------------------------------------
// Pin ids
// ---------------------------------------------------------------------------
// Pin identifiers are stable names scoped to their owning node. We use
// strings instead of integers because human-authored IR must survive
// refactors without renumbering.
using PinName = std::string;

// A connection: (source node id, source pin name) → (destination node, pin).
// Source = producer, destination = consumer.
struct Edge {
    std::uint32_t src_node = 0;
    PinName       src_pin;
    std::uint32_t dst_node = 0;
    PinName       dst_pin;
};

// ---------------------------------------------------------------------------
// Nodes
// ---------------------------------------------------------------------------
enum class NodeKind : std::uint8_t {
    // Data producers
    Const,       // `const` — emits a literal
    GetInput,    // reads a graph-level input
    // Data transforms — arithmetic.
    Add,
    Sub,
    Mul,
    // Control
    Select,      // ternary: out = cond ? a : b
    // Sinks
    SetOutput,   // writes to a graph-level output
};

[[nodiscard]] std::string_view node_kind_name(NodeKind k) noexcept;
[[nodiscard]] std::optional<NodeKind> parse_node_kind(std::string_view s) noexcept;

// A pin descriptor — name + type + whether it is input or output.
struct Pin {
    PinName name;
    Type    type = Type::Void;
    // Optional literal attached to an input pin. When present the node
    // consumes this value unless an Edge arrives at the same pin. Literals
    // are how the parser represents `in x = 3` with no upstream producer.
    std::optional<Value> literal;
};

struct Node {
    std::uint32_t id = 0;   // stable within a Graph; matches vector index
    NodeKind      kind = NodeKind::Const;
    std::string   debug_name;   // optional human label
    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
};

// ---------------------------------------------------------------------------
// Graph
// ---------------------------------------------------------------------------
struct IOPort {
    PinName name;
    Type    type = Type::Void;
    // For graph inputs, this is the default value observed when no caller
    // provides one. For graph outputs, this is the evaluated value after an
    // interpreter run.
    Value   value = std::monostate{};
};

struct Graph {
    std::string        name;
    std::vector<IOPort> inputs;    // graph-level parameters
    std::vector<IOPort> outputs;   // graph-level results
    std::vector<Node>   nodes;
    std::vector<Edge>   edges;

    // Debug-friendly lookup helpers.
    [[nodiscard]] const Node* find_node(std::uint32_t id) const noexcept;
    [[nodiscard]] Node*       find_node(std::uint32_t id)       noexcept;

    [[nodiscard]] const IOPort* find_input(std::string_view name) const noexcept;
    [[nodiscard]] const IOPort* find_output(std::string_view name) const noexcept;
    [[nodiscard]] IOPort*       find_output(std::string_view name)       noexcept;
};

// Validate graph shape. Reports the first error encountered:
//   * disconnected non-literal input pin
//   * edge with unknown src_node or dst_node
//   * edge whose pin types disagree
//   * cycle between nodes
// Returns empty string on success; human-readable error otherwise.
[[nodiscard]] std::string validate(const Graph& g);

} // namespace gw::vscript
