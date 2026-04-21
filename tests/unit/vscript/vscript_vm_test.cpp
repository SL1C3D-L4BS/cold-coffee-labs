// tests/unit/vscript/vscript_vm_test.cpp
// Phase 8 week 046 — vscript bytecode compiler + VM golden tests.
//
// The VM must match the reference interpreter bit-for-bit on the corpus we
// care about. Every test here compiles a graph, runs it on both the
// interpreter and the VM, and checks that the observable outputs agree.

#include <doctest/doctest.h>

#include "engine/vscript/bytecode.hpp"
#include "engine/vscript/interpreter.hpp"
#include "engine/vscript/ir.hpp"
#include "engine/vscript/parser.hpp"
#include "engine/vscript/vm.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

using namespace gw::vscript;

namespace {

// Helper: compile + run VM, asserting success.
ExecResult compile_and_run(const Graph& g, const InputBindings& ins = {}) {
    auto prog = compile(g);
    REQUIRE_MESSAGE(prog.has_value(),
        "compile failed: ", (prog ? std::string{} : prog.error().message));
    auto r = run(*prog, ins);
    REQUIRE_MESSAGE(r.has_value(),
        "VM failed: ", (r ? std::string{} : r.error().message));
    return *r;
}

void check_same_outputs(const ExecResult& a, const ExecResult& b) {
    REQUIRE(a.outputs.size() == b.outputs.size());
    for (const auto& [k, v] : a.outputs) {
        REQUIRE(b.outputs.count(k) == 1);
        CHECK(v == b.outputs.at(k));
    }
}

} // namespace

TEST_CASE("vscript VM — compiles and executes the text adder") {
    constexpr std::string_view kText = R"(graph adder {
  input a : int = 0
  input b : int = 0
  output sum : int
  node 1 : get_input {
    in name : string = "a"
    out value : int
  }
  node 2 : get_input {
    in name : string = "b"
    out value : int
  }
  node 3 : add {
    in lhs : int
    in rhs : int
    out value : int
  }
  node 4 : set_output {
    in name : string = "sum"
    in value : int
  }
  edge 1.value -> 3.lhs
  edge 2.value -> 3.rhs
  edge 3.value -> 4.value
}
)";
    auto g = parse(kText);
    REQUIRE(g.has_value());

    InputBindings ins;
    ins["a"] = Value{std::int64_t{100}};
    ins["b"] = Value{std::int64_t{37}};

    const auto interp = execute(*g, ins);
    REQUIRE(interp.has_value());
    const auto vm = compile_and_run(*g, ins);
    check_same_outputs(*interp, vm);
    CHECK(std::get<std::int64_t>(vm.outputs.at("sum")) == 137);
}

TEST_CASE("vscript VM — select routes on bool condition (oracle match)") {
    constexpr std::string_view kText = R"(graph pick {
  input cond : bool = true
  output out : int
  node 1 : get_input {
    in name : string = "cond"
    out value : bool
  }
  node 2 : select {
    in cond : bool
    in a : int = 7
    in b : int = 13
    out value : int
  }
  node 3 : set_output {
    in name : string = "out"
    in value : int
  }
  edge 1.value -> 2.cond
  edge 2.value -> 3.value
}
)";
    auto g = parse(kText);
    REQUIRE(g.has_value());

    for (bool pick : {true, false}) {
        InputBindings ins;
        ins["cond"] = Value{pick};
        const auto interp = execute(*g, ins);
        REQUIRE(interp.has_value());
        const auto vm = compile_and_run(*g, ins);
        check_same_outputs(*interp, vm);
        CHECK(std::get<std::int64_t>(vm.outputs.at("out")) == (pick ? 7 : 13));
    }
}

TEST_CASE("vscript VM — float mul matches interpreter") {
    constexpr std::string_view kText = R"(graph area {
  input w : float = 0.0
  input h : float = 0.0
  output area : float
  node 1 : get_input {
    in name : string = "w"
    out value : float
  }
  node 2 : get_input {
    in name : string = "h"
    out value : float
  }
  node 3 : mul {
    in lhs : float
    in rhs : float
    out value : float
  }
  node 4 : set_output {
    in name : string = "area"
    in value : float
  }
  edge 1.value -> 3.lhs
  edge 2.value -> 3.rhs
  edge 3.value -> 4.value
}
)";
    auto g = parse(kText);
    REQUIRE(g.has_value());

    InputBindings ins;
    ins["w"] = Value{2.5};
    ins["h"] = Value{4.0};
    const auto interp = execute(*g, ins);
    REQUIRE(interp.has_value());
    const auto vm = compile_and_run(*g, ins);
    check_same_outputs(*interp, vm);
    CHECK(std::get<double>(vm.outputs.at("area")) == doctest::Approx(10.0));
}

TEST_CASE("vscript VM — missing driver is caught at compile time") {
    // Build a graph where an arithmetic op has no driver and no literal — the
    // interpreter and the VM should both reject it via validate(), and the
    // compiler must surface ValidationFailed.
    Graph g;
    g.name = "broken";
    g.outputs.push_back(IOPort{"n", Type::Int, Value{std::int64_t{0}}});

    Node add{};
    add.id = 1;
    add.kind = NodeKind::Add;
    add.inputs.push_back(Pin{"lhs", Type::Int, std::nullopt});
    add.inputs.push_back(Pin{"rhs", Type::Int, std::nullopt});
    add.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    g.nodes.push_back(std::move(add));

    Node out{};
    out.id = 2;
    out.kind = NodeKind::SetOutput;
    out.inputs.push_back(Pin{"name",  Type::String, Value{std::string{"n"}}});
    out.inputs.push_back(Pin{"value", Type::Int,    std::nullopt});
    g.nodes.push_back(std::move(out));
    g.edges.push_back(Edge{1, "value", 2, "value"});

    auto prog = compile(g);
    REQUIRE_FALSE(prog.has_value());
    CHECK(prog.error().code == CompileError::ValidationFailed);
}

TEST_CASE("vscript VM — halt terminates and fills unwritten outputs with defaults") {
    // Graph with an output but no set_output node — the VM must still return
    // a full output map populated with the declared defaults. Built in code
    // because the text grammar doesn't expose output-defaults.
    Graph g;
    g.name = "empty";
    g.outputs.push_back(IOPort{"n", Type::Int, Value{std::int64_t{42}}});

    const auto vm = compile_and_run(g);
    REQUIRE(vm.outputs.count("n") == 1);
    CHECK(std::get<std::int64_t>(vm.outputs.at("n")) == 42);
}
