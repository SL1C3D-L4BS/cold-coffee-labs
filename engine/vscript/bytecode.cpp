// engine/vscript/bytecode.cpp
#include "bytecode.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <vector>

namespace gw::vscript {

namespace {

constexpr std::array<std::string_view, 12> kOpNames = {
    "halt", "push_i64", "push_f64", "push_bool", "push_str", "push_vec3",
    "get_input", "add", "sub", "mul", "select", "set_output",
};

} // namespace

std::string_view opcode_name(OpCode op) noexcept {
    const auto i = static_cast<std::size_t>(op);
    return i < kOpNames.size() ? kOpNames[i] : std::string_view{"?"};
}

std::int32_t Program::find_name(std::string_view s) const noexcept {
    for (std::size_t i = 0; i < strings.size(); ++i) {
        if (strings[i] == s) return static_cast<std::int32_t>(i);
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Compiler
// ---------------------------------------------------------------------------
namespace {

struct Compiler {
    const Graph& g;
    Program      out;
    // Interned pools — reverse lookup while building.
    std::unordered_map<std::string, std::uint32_t>  string_pool;
    // Track which (node, output-pin) pairs are already materialised on the
    // stack so shared sub-expressions aren't re-evaluated. The value is the
    // position from the bottom of the virtual stack at compile time; since
    // this is a linear schedule we only cache "materialised at topo-order
    // index N", not the absolute stack height.
    //
    // For Phase 8 we keep the simpler model: each output pin is an
    // independent evaluation, recomputed if needed. Memoisation lands with
    // the JIT in Phase 9.

    std::uint32_t intern_string(std::string s) {
        auto it = string_pool.find(s);
        if (it != string_pool.end()) return it->second;
        const auto idx = static_cast<std::uint32_t>(out.strings.size());
        out.strings.push_back(s);
        string_pool.emplace(std::move(s), idx);
        return idx;
    }

    std::uint32_t intern_vec3(const Vec3Value& v) {
        const auto idx = static_cast<std::uint32_t>(out.vec3s.size());
        out.vec3s.push_back(v);
        return idx;
    }

    void emit_value(const Value& v) {
        struct Visitor {
            Compiler& c;
            void operator()(std::int64_t x) const {
                Instr i{OpCode::PushI64}; i.i64 = x;   c.out.code.push_back(i);
            }
            void operator()(double x) const {
                Instr i{OpCode::PushF64}; i.f64 = x;   c.out.code.push_back(i);
            }
            void operator()(bool b) const {
                Instr i{OpCode::PushBool}; i.b = b;    c.out.code.push_back(i);
            }
            void operator()(const std::string& s) const {
                Instr i{OpCode::PushStr}; i.idx = c.intern_string(s);
                c.out.code.push_back(i);
            }
            void operator()(const Vec3Value& v) const {
                Instr i{OpCode::PushVec3}; i.idx = c.intern_vec3(v);
                c.out.code.push_back(i);
            }
            void operator()(std::monostate) const {
                Instr i{OpCode::PushI64}; i.i64 = 0;   c.out.code.push_back(i);
            }
        };
        std::visit(Visitor{*this}, v);
    }

    // Emit code that evaluates (node, pin) and leaves exactly one value on
    // the stack. Recursion mirrors the interpreter's demand-driven walk.
    // `pin` is the *input* pin of `user` whose driving value we want.
    std::expected<void, CompileFailure>
    emit_input(const Node& user, const Pin& pin) {
        // Locate the edge driving (user.id, pin.name); fall back to the
        // pin's literal. The edges vector is small — linear search beats a
        // hashmap for the graph sizes the editor produces.
        const Edge* edge = nullptr;
        for (const auto& e : g.edges) {
            if (e.dst_node == user.id && e.dst_pin == pin.name) {
                edge = &e; break;
            }
        }
        if (edge) {
            const Node* src = g.find_node(edge->src_node);
            if (!src) return std::unexpected(CompileFailure{
                CompileError::InternalError, "dangling edge"});
            return emit_node_output(*src, edge->src_pin);
        }
        if (pin.literal.has_value()) {
            emit_value(*pin.literal);
            return {};
        }
        return std::unexpected(CompileFailure{CompileError::ValidationFailed,
            "input pin has no driver — validate() should have caught this"});
    }

    std::expected<void, CompileFailure>
    emit_node_output(const Node& n, const std::string& pin_name) {
        switch (n.kind) {
            case NodeKind::Const: {
                for (const auto& p : n.outputs) {
                    if (p.name == pin_name) {
                        emit_value(p.literal.value_or(default_value(p.type)));
                        return {};
                    }
                }
                return std::unexpected(CompileFailure{
                    CompileError::InternalError, "const missing output pin"});
            }
            case NodeKind::GetInput: {
                if (n.inputs.size() != 1 || n.inputs[0].type != Type::String ||
                    !n.inputs[0].literal.has_value()) {
                    return std::unexpected(CompileFailure{
                        CompileError::ValidationFailed,
                        "get_input requires 1 string literal input"});
                }
                const auto& name = std::get<std::string>(*n.inputs[0].literal);
                Instr i{OpCode::GetInput};
                i.idx = intern_string(name);
                out.code.push_back(i);
                return {};
            }
            case NodeKind::Add:
            case NodeKind::Sub:
            case NodeKind::Mul: {
                if (n.inputs.size() < 2) {
                    return std::unexpected(CompileFailure{
                        CompileError::ValidationFailed,
                        "arithmetic node needs 2 inputs"});
                }
                if (auto r = emit_input(n, n.inputs[0]); !r) return r;
                if (auto r = emit_input(n, n.inputs[1]); !r) return r;
                Instr i{};
                i.op = (n.kind == NodeKind::Add ? OpCode::Add :
                        n.kind == NodeKind::Sub ? OpCode::Sub : OpCode::Mul);
                out.code.push_back(i);
                return {};
            }
            case NodeKind::Select: {
                if (n.inputs.size() < 3) {
                    return std::unexpected(CompileFailure{
                        CompileError::ValidationFailed,
                        "select needs 3 inputs"});
                }
                // Stack order: cond, a, b (VM pops b, a, cond).
                if (auto r = emit_input(n, n.inputs[0]); !r) return r;
                if (auto r = emit_input(n, n.inputs[1]); !r) return r;
                if (auto r = emit_input(n, n.inputs[2]); !r) return r;
                out.code.push_back(Instr{OpCode::Select});
                return {};
            }
            case NodeKind::SetOutput:
                return std::unexpected(CompileFailure{
                    CompileError::InternalError,
                    "set_output has no output pin — should not be queried as a producer"});
        }
        return std::unexpected(CompileFailure{
            CompileError::UnsupportedNodeKind, "unknown node kind"});
    }

    std::expected<void, CompileFailure> compile_all() {
        // Emit one "store output" sub-program per SetOutput node.
        for (const auto& n : g.nodes) {
            if (n.kind != NodeKind::SetOutput) continue;
            if (n.inputs.size() < 2 || n.inputs[0].type != Type::String ||
                !n.inputs[0].literal.has_value()) {
                return std::unexpected(CompileFailure{
                    CompileError::ValidationFailed,
                    "set_output requires (name:string, value:*)"});
            }
            if (auto r = emit_input(n, n.inputs[1]); !r) return r;
            Instr i{OpCode::SetOutput};
            i.idx = intern_string(std::get<std::string>(*n.inputs[0].literal));
            out.code.push_back(i);
        }
        out.code.push_back(Instr{OpCode::Halt});
        return {};
    }
};

} // namespace

std::expected<Program, CompileFailure> compile(const Graph& g) {
    auto err = validate(g);
    if (!err.empty()) {
        return std::unexpected(CompileFailure{CompileError::ValidationFailed, err});
    }
    Compiler c{g, {}, {}};
    // Copy I/O metadata directly — VMs need the typed port table.
    c.out.inputs  = g.inputs;
    c.out.outputs = g.outputs;
    if (auto r = c.compile_all(); !r) return std::unexpected(r.error());
    return std::move(c.out);
}

} // namespace gw::vscript
