// tests/unit/vscript/vscript_ir_test.cpp
// Phase 8 week 045 — vscript IR + parser + interpreter.

#include <ostream>
#include <doctest/doctest.h>

#include "engine/vscript/interpreter.hpp"
#include "engine/vscript/ir.hpp"
#include "engine/vscript/parser.hpp"

#include <string>
#include <string_view>
#include <variant>

using namespace gw::vscript;

namespace {

// Build a small graph by hand so we can test the interpreter without
// depending on the parser (and vice-versa).
Graph build_adder_graph() {
    Graph g;
    g.name = "adder";
    g.inputs.push_back(IOPort{"a", Type::Int, Value{std::int64_t{0}}});
    g.inputs.push_back(IOPort{"b", Type::Int, Value{std::int64_t{0}}});
    g.outputs.push_back(IOPort{"sum", Type::Int, Value{std::int64_t{0}}});

    Node get_a{};
    get_a.id = 1;
    get_a.kind = NodeKind::GetInput;
    get_a.inputs.push_back(Pin{"name", Type::String, Value{std::string{"a"}}});
    get_a.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    g.nodes.push_back(std::move(get_a));

    Node get_b{};
    get_b.id = 2;
    get_b.kind = NodeKind::GetInput;
    get_b.inputs.push_back(Pin{"name", Type::String, Value{std::string{"b"}}});
    get_b.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    g.nodes.push_back(std::move(get_b));

    Node add{};
    add.id = 3;
    add.kind = NodeKind::Add;
    add.inputs.push_back(Pin{"lhs", Type::Int, std::nullopt});
    add.inputs.push_back(Pin{"rhs", Type::Int, std::nullopt});
    add.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    g.nodes.push_back(std::move(add));

    Node set_out{};
    set_out.id = 4;
    set_out.kind = NodeKind::SetOutput;
    set_out.inputs.push_back(Pin{"name",  Type::String, Value{std::string{"sum"}}});
    set_out.inputs.push_back(Pin{"value", Type::Int,    std::nullopt});
    g.nodes.push_back(std::move(set_out));

    g.edges.push_back(Edge{1, "value", 3, "lhs"});
    g.edges.push_back(Edge{2, "value", 3, "rhs"});
    g.edges.push_back(Edge{3, "value", 4, "value"});
    return g;
}

} // namespace

TEST_CASE("vscript ir — validate() accepts a well-formed DAG") {
    const auto g = build_adder_graph();
    const auto err = validate(g);
    CHECK(err.empty());
}

TEST_CASE("vscript ir — validate() rejects a disconnected input pin") {
    auto g = build_adder_graph();
    // Drop the edge driving the `lhs` input — `lhs` has no literal either.
    g.edges.erase(g.edges.begin());
    const auto err = validate(g);
    CHECK(err.find("'lhs'") != std::string::npos);
}

TEST_CASE("vscript ir — validate() rejects a cycle") {
    Graph g;
    g.name = "cycle";
    // Two nodes that drive each other's `in` pin: x depends on y, y on x.
    Node x{};
    x.id = 1;
    x.kind = NodeKind::Add;
    x.inputs.push_back(Pin{"lhs", Type::Int, Value{std::int64_t{0}}});
    x.inputs.push_back(Pin{"rhs", Type::Int, Value{std::int64_t{0}}});
    x.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    Node y = x;
    y.id = 2;
    g.nodes.push_back(std::move(x));
    g.nodes.push_back(std::move(y));
    g.edges.push_back(Edge{1, "value", 2, "lhs"});
    g.edges.push_back(Edge{2, "value", 1, "lhs"});
    const auto err = validate(g);
    CHECK(err.find("cycle") != std::string::npos);
}

TEST_CASE("vscript interpreter — adder runs end-to-end") {
    const auto g = build_adder_graph();
    InputBindings ins;
    ins["a"] = Value{std::int64_t{40}};
    ins["b"] = Value{std::int64_t{2}};
    auto r = execute(g, ins);
    REQUIRE(r.has_value());
    REQUIRE(r->outputs.count("sum") == 1);
    CHECK(std::get<std::int64_t>(r->outputs.at("sum")) == 42);
}

TEST_CASE("vscript interpreter — select routes on bool condition") {
    Graph g;
    g.name = "branch";
    g.inputs.push_back(IOPort{"pick", Type::Bool, Value{true}});
    g.outputs.push_back(IOPort{"out", Type::Int, Value{std::int64_t{0}}});

    Node get_cond{};
    get_cond.id = 1;
    get_cond.kind = NodeKind::GetInput;
    get_cond.inputs.push_back(Pin{"name", Type::String, Value{std::string{"pick"}}});
    get_cond.outputs.push_back(Pin{"value", Type::Bool, std::nullopt});
    g.nodes.push_back(std::move(get_cond));

    Node sel{};
    sel.id = 2;
    sel.kind = NodeKind::Select;
    sel.inputs.push_back(Pin{"cond", Type::Bool, std::nullopt});
    sel.inputs.push_back(Pin{"a",    Type::Int,  Value{std::int64_t{7}}});
    sel.inputs.push_back(Pin{"b",    Type::Int,  Value{std::int64_t{13}}});
    sel.outputs.push_back(Pin{"value", Type::Int, std::nullopt});
    g.nodes.push_back(std::move(sel));

    Node out{};
    out.id = 3;
    out.kind = NodeKind::SetOutput;
    out.inputs.push_back(Pin{"name",  Type::String, Value{std::string{"out"}}});
    out.inputs.push_back(Pin{"value", Type::Int,    std::nullopt});
    g.nodes.push_back(std::move(out));

    g.edges.push_back(Edge{1, "value", 2, "cond"});
    g.edges.push_back(Edge{2, "value", 3, "value"});

    InputBindings ins_true;   ins_true["pick"]  = Value{true};
    InputBindings ins_false;  ins_false["pick"] = Value{false};
    auto r_true  = execute(g, ins_true);
    auto r_false = execute(g, ins_false);
    REQUIRE(r_true.has_value());
    REQUIRE(r_false.has_value());
    CHECK(std::get<std::int64_t>(r_true->outputs.at("out"))  == 7);
    CHECK(std::get<std::int64_t>(r_false->outputs.at("out")) == 13);
}

TEST_CASE("vscript parser — serialize(parse(text)) is round-trip stable") {
    constexpr std::string_view kText = R"(graph demo {
  input a : int = 3
  input b : int = 4
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
    CHECK(validate(*g).empty());

    const auto s1 = serialize(*g);
    auto g2 = parse(s1);
    REQUIRE(g2.has_value());
    const auto s2 = serialize(*g2);
    CHECK(s1 == s2);

    // And it still runs.
    auto r = execute(*g);
    REQUIRE(r.has_value());
    CHECK(std::get<std::int64_t>(r->outputs.at("sum")) == 7);
}

TEST_CASE("vscript parser — unknown keyword produces ParseError with line #") {
    constexpr std::string_view kText = R"(graph oops {
  wat whatever : int
}
)";
    auto g = parse(kText);
    REQUIRE_FALSE(g.has_value());
    CHECK(g.error().line == 2);
    CHECK(g.error().message.find("wat") != std::string::npos);
}

TEST_CASE("vscript parser — type mismatch caught by validate()") {
    constexpr std::string_view kText = R"(graph bad {
  output n : int
  node 1 : const {
    out value : float = 1.5
  }
  node 2 : set_output {
    in name : string = "n"
    in value : int
  }
  edge 1.value -> 2.value
}
)";
    auto g = parse(kText);
    REQUIRE(g.has_value());
    const auto err = validate(*g);
    CHECK(err.find("type mismatch") != std::string::npos);
}
