// engine/vscript/interpreter.cpp
#include "interpreter.hpp"

#include <unordered_map>
#include <variant>

namespace gw::vscript {

std::string_view to_string(ExecError e) noexcept {
    switch (e) {
        case ExecError::ValidationFailed:    return "validation failed";
        case ExecError::UnknownInput:        return "unknown graph input";
        case ExecError::UnknownOutput:       return "unknown graph output";
        case ExecError::MissingDriver:       return "input pin has no driver";
        case ExecError::TypeMismatch:        return "type mismatch";
        case ExecError::UnsupportedNodeKind: return "unsupported node kind";
        case ExecError::InternalError:       return "internal error";
    }
    return "?";
}

namespace {

// Key for memoisation: (node_id, pin_name). Using a flat unordered_map with
// a string key keeps the implementation readable; evaluation sets are tiny
// so this isn't perf-critical.
struct PinKey {
    std::uint32_t node = 0;
    std::string   pin;
    friend bool operator==(const PinKey&, const PinKey&) = default;
};
struct PinKeyHash {
    std::size_t operator()(const PinKey& k) const noexcept {
        return std::hash<std::string>{}(k.pin) ^ (std::uint64_t{k.node} * 0x9E3779B97F4A7C15ULL);
    }
};

struct Env {
    const Graph&                            g;
    const InputBindings&                    inputs;
    std::unordered_map<PinKey, Value, PinKeyHash> cache;
    // Edges indexed by destination (node, pin) for O(1) lookup during
    // eval. Built once per execute() call.
    std::unordered_map<PinKey, const Edge*, PinKeyHash> by_dst;

    Env(const Graph& gr, const InputBindings& ins) : g(gr), inputs(ins) {
        for (const auto& e : gr.edges) {
            by_dst[{e.dst_node, e.dst_pin}] = &e;
        }
    }
};

std::expected<Value, ExecFailure> eval_node(Env& env, std::uint32_t nid,
                                             const std::string& pin);

// Evaluate whatever drives `(node, pin)` on the input side: either an
// incoming edge's upstream output, or the pin's literal fallback.
std::expected<Value, ExecFailure> eval_input_pin(Env& env,
                                                   const Node& n,
                                                   const Pin& p) {
    auto it = env.by_dst.find({n.id, p.name});
    if (it != env.by_dst.end()) {
        return eval_node(env, it->second->src_node, it->second->src_pin);
    }
    if (p.literal.has_value()) return *p.literal;
    return std::unexpected(ExecFailure{ExecError::MissingDriver,
        "node " + std::to_string(n.id) + " pin '" + p.name +
        "' has no driver and no literal"});
}

template <typename T>
std::expected<T, ExecFailure> expect_type(const Value& v, const char* where) {
    if (!std::holds_alternative<T>(v)) {
        return std::unexpected(ExecFailure{ExecError::TypeMismatch, where});
    }
    return std::get<T>(v);
}

std::expected<Value, ExecFailure> eval_node(Env& env, std::uint32_t nid,
                                             const std::string& pin) {
    const PinKey key{nid, pin};
    if (auto cached = env.cache.find(key); cached != env.cache.end()) {
        return cached->second;
    }
    const Node* n = env.g.find_node(nid);
    if (!n) return std::unexpected(ExecFailure{ExecError::InternalError,
        "dangling node id " + std::to_string(nid)});

    auto finish = [&](Value v) -> std::expected<Value, ExecFailure> {
        env.cache[key] = v;
        return v;
    };

    switch (n->kind) {
        case NodeKind::Const: {
            // Output pin either has a literal already (const nodes encode the
            // value on the `value` output pin literal) or falls back to the
            // pin type's default.
            for (const auto& p : n->outputs) {
                if (p.name == pin) {
                    return finish(p.literal.value_or(default_value(p.type)));
                }
            }
            return std::unexpected(ExecFailure{ExecError::InternalError,
                "const node missing requested output pin"});
        }
        case NodeKind::GetInput: {
            // Pull the value from InputBindings (preferred) or the port
            // default. Node expects one input pin `name` with a literal
            // string naming the graph input to read.
            if (n->inputs.size() != 1 || n->inputs[0].type != Type::String ||
                !n->inputs[0].literal.has_value()) {
                return std::unexpected(ExecFailure{ExecError::ValidationFailed,
                    "get_input requires 1 string literal input"});
            }
            const auto& name = std::get<std::string>(*n->inputs[0].literal);
            if (auto it = env.inputs.find(name); it != env.inputs.end()) {
                return finish(it->second);
            }
            const auto* port = env.g.find_input(name);
            if (!port) return std::unexpected(ExecFailure{ExecError::UnknownInput,
                "get_input references unknown graph input '" + name + "'"});
            return finish(port->value);
        }
        case NodeKind::Add:
        case NodeKind::Sub:
        case NodeKind::Mul: {
            if (n->inputs.size() < 2) {
                return std::unexpected(ExecFailure{ExecError::ValidationFailed,
                    "arithmetic node needs two inputs"});
            }
            auto va = eval_input_pin(env, *n, n->inputs[0]);
            if (!va) return std::unexpected(va.error());
            auto vb = eval_input_pin(env, *n, n->inputs[1]);
            if (!vb) return std::unexpected(vb.error());
            if (type_of(*va) != type_of(*vb)) {
                return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                    "arithmetic operands differ in type"});
            }
            auto apply = [&](auto op) -> std::expected<Value, ExecFailure> {
                if (std::holds_alternative<std::int64_t>(*va)) {
                    return Value{op(std::get<std::int64_t>(*va),
                                    std::get<std::int64_t>(*vb))};
                }
                if (std::holds_alternative<double>(*va)) {
                    return Value{op(std::get<double>(*va),
                                    std::get<double>(*vb))};
                }
                return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                    "arithmetic on non-numeric type"});
            };
            std::expected<Value, ExecFailure> r;
            if (n->kind == NodeKind::Add)      r = apply([](auto a, auto b){ return a + b; });
            else if (n->kind == NodeKind::Sub) r = apply([](auto a, auto b){ return a - b; });
            else                                r = apply([](auto a, auto b){ return a * b; });
            if (!r) return std::unexpected(r.error());
            return finish(*r);
        }
        case NodeKind::Select: {
            // Pins: in cond:bool, in a:T, in b:T | out value:T
            if (n->inputs.size() < 3) {
                return std::unexpected(ExecFailure{ExecError::ValidationFailed,
                    "select needs 3 inputs (cond, a, b)"});
            }
            auto vc = eval_input_pin(env, *n, n->inputs[0]);
            if (!vc) return std::unexpected(vc.error());
            auto bc = expect_type<bool>(*vc, "select condition must be bool");
            if (!bc) return std::unexpected(bc.error());
            const auto& take = *bc ? n->inputs[1] : n->inputs[2];
            auto vv = eval_input_pin(env, *n, take);
            if (!vv) return std::unexpected(vv.error());
            return finish(*vv);
        }
        case NodeKind::SetOutput: {
            // Writes the graph output whose name matches inputs[0].literal
            // (string). Returns the forwarded value on an `out` pin named
            // "value" so chains remain composable.
            if (n->inputs.size() < 2 || n->inputs[0].type != Type::String ||
                !n->inputs[0].literal.has_value()) {
                return std::unexpected(ExecFailure{ExecError::ValidationFailed,
                    "set_output requires (name:string, value:*)"});
            }
            auto vv = eval_input_pin(env, *n, n->inputs[1]);
            if (!vv) return std::unexpected(vv.error());
            return finish(*vv);
        }
    }
    return std::unexpected(ExecFailure{ExecError::UnsupportedNodeKind,
        "node kind " + std::to_string(static_cast<int>(n->kind))});
}

} // namespace

std::expected<ExecResult, ExecFailure>
execute(const Graph& g, const InputBindings& inputs) {
    // Belt-and-braces: validate first so bad graphs can't reach the
    // evaluator.
    auto err = validate(g);
    if (!err.empty()) {
        return std::unexpected(ExecFailure{ExecError::ValidationFailed, err});
    }

    Env env{g, inputs};
    ExecResult result;

    // For every graph output port, find the node+pin that drives it (the
    // edge whose dst is a SetOutput node wired to this output name). If no
    // driver exists, fall back to the port's default value.
    // SetOutput nodes reference the output name in inputs[0].literal (string).
    for (const auto& out : g.outputs) {
        // Locate a SetOutput node wired to this output name.
        const Node* sink = nullptr;
        for (const auto& n : g.nodes) {
            if (n.kind != NodeKind::SetOutput) continue;
            if (n.inputs.empty()) continue;
            if (n.inputs[0].type != Type::String) continue;
            if (!n.inputs[0].literal.has_value()) continue;
            if (std::get<std::string>(*n.inputs[0].literal) == out.name) {
                sink = &n; break;
            }
        }
        if (!sink) {
            result.outputs.emplace(out.name, out.value);
            continue;
        }
        auto vv = eval_input_pin(env, *sink, sink->inputs[1]);
        if (!vv) return std::unexpected(vv.error());
        if (type_of(*vv) != out.type) {
            return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                "graph output '" + out.name + "' type mismatch"});
        }
        result.outputs.emplace(out.name, *vv);
    }
    return result;
}

} // namespace gw::vscript
