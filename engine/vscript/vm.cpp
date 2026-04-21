// engine/vscript/vm.cpp
#include "vm.hpp"

#include <string>
#include <variant>
#include <vector>

namespace gw::vscript {

namespace {

template <typename T>
std::expected<T, ExecFailure> pop_as(std::vector<Value>& s, const char* where) {
    if (s.empty()) {
        return std::unexpected(ExecFailure{ExecError::InternalError,
            std::string{"VM stack underflow at "} + where});
    }
    auto v = std::move(s.back());
    s.pop_back();
    if (!std::holds_alternative<T>(v)) {
        return std::unexpected(ExecFailure{ExecError::TypeMismatch, where});
    }
    return std::get<T>(std::move(v));
}

} // namespace

std::expected<ExecResult, ExecFailure>
run(const Program& p, const InputBindings& inputs) {
    ExecResult         result;
    std::vector<Value> stack;
    stack.reserve(p.code.size());

    // Helpers to access pooled literals.
    auto pool_string = [&](std::uint32_t idx) -> const std::string& {
        return p.strings[idx];
    };
    auto pool_vec3 = [&](std::uint32_t idx) -> const Vec3Value& {
        return p.vec3s[idx];
    };

    std::size_t ip = 0;
    while (ip < p.code.size()) {
        const Instr& ins = p.code[ip++];
        switch (ins.op) {
            case OpCode::Halt:
                // Fill unset outputs with declared defaults before returning
                // so callers always observe a complete outputs map — mirrors
                // the interpreter's graph-output loop fallback behaviour.
                for (const auto& pt : p.outputs) {
                    result.outputs.try_emplace(pt.name, pt.value);
                }
                return result;
            case OpCode::PushI64:
                stack.emplace_back(Value{ins.i64});
                break;
            case OpCode::PushF64:
                stack.emplace_back(Value{ins.f64});
                break;
            case OpCode::PushBool:
                stack.emplace_back(Value{static_cast<bool>(ins.b)});
                break;
            case OpCode::PushStr:
                stack.emplace_back(Value{pool_string(ins.idx)});
                break;
            case OpCode::PushVec3:
                stack.emplace_back(Value{pool_vec3(ins.idx)});
                break;
            case OpCode::GetInput: {
                const auto& name = pool_string(ins.idx);
                auto it = inputs.find(name);
                if (it != inputs.end()) {
                    stack.emplace_back(it->second);
                    break;
                }
                // Fallback: scan compiled port defaults (cheap — tiny).
                const IOPort* port = nullptr;
                for (const auto& pt : p.inputs) {
                    if (pt.name == name) { port = &pt; break; }
                }
                if (!port) return std::unexpected(ExecFailure{
                    ExecError::UnknownInput,
                    "get_input: unknown input '" + name + "'"});
                stack.emplace_back(port->value);
                break;
            }
            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul: {
                if (stack.size() < 2) {
                    return std::unexpected(ExecFailure{ExecError::InternalError,
                        "arithmetic stack underflow"});
                }
                Value b = std::move(stack.back()); stack.pop_back();
                Value a = std::move(stack.back()); stack.pop_back();
                if (type_of(a) != type_of(b)) {
                    return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                        "arithmetic operands differ in type"});
                }
                auto apply = [&](auto op) -> std::expected<Value, ExecFailure> {
                    if (std::holds_alternative<std::int64_t>(a)) {
                        return Value{op(std::get<std::int64_t>(a),
                                         std::get<std::int64_t>(b))};
                    }
                    if (std::holds_alternative<double>(a)) {
                        return Value{op(std::get<double>(a),
                                         std::get<double>(b))};
                    }
                    return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                        "arithmetic on non-numeric type"});
                };
                std::expected<Value, ExecFailure> r;
                switch (ins.op) {
                    case OpCode::Add: r = apply([](auto x, auto y){ return x + y; }); break;
                    case OpCode::Sub: r = apply([](auto x, auto y){ return x - y; }); break;
                    case OpCode::Mul: r = apply([](auto x, auto y){ return x * y; }); break;
                    default: break;
                }
                if (!r) return std::unexpected(r.error());
                stack.push_back(std::move(*r));
                break;
            }
            case OpCode::Select: {
                if (stack.size() < 3) {
                    return std::unexpected(ExecFailure{ExecError::InternalError,
                        "select stack underflow"});
                }
                Value vb = std::move(stack.back()); stack.pop_back();
                Value va = std::move(stack.back()); stack.pop_back();
                Value vc = std::move(stack.back()); stack.pop_back();
                if (!std::holds_alternative<bool>(vc)) {
                    return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                        "select condition must be bool"});
                }
                stack.push_back(std::get<bool>(vc) ? std::move(va) : std::move(vb));
                break;
            }
            case OpCode::SetOutput: {
                if (stack.empty()) {
                    return std::unexpected(ExecFailure{ExecError::InternalError,
                        "set_output stack underflow"});
                }
                Value v = std::move(stack.back()); stack.pop_back();
                const auto& name = pool_string(ins.idx);
                // Type check against declared output.
                const IOPort* port = nullptr;
                for (const auto& pt : p.outputs) {
                    if (pt.name == name) { port = &pt; break; }
                }
                if (!port) return std::unexpected(ExecFailure{
                    ExecError::UnknownOutput,
                    "set_output references unknown output '" + name + "'"});
                if (type_of(v) != port->type) {
                    return std::unexpected(ExecFailure{ExecError::TypeMismatch,
                        "set_output '" + name + "' value type mismatch"});
                }
                result.outputs[name] = std::move(v);
                break;
            }
        }
    }
    // Fill in any unset outputs with their declared defaults so the caller
    // always sees a complete map.
    for (const auto& pt : p.outputs) {
        result.outputs.try_emplace(pt.name, pt.value);
    }
    return result;
}

} // namespace gw::vscript
