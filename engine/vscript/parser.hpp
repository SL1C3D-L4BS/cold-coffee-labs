#pragma once
// engine/vscript/parser.hpp
// Phase 8 week 045 — Text-IR parser/serializer for Visual Scripting.
//
// The text IR is the ground-truth (CLAUDE.md non-negotiable #14). Node
// graphs in the editor are a projection; authoring tools round-trip through
// this parser so the text file is always the canonical source of truth on
// disk and in version control.
//
// Grammar (EBNF-ish):
//
//   graph      := "graph" IDENT "{" { entry } "}"
//   entry      := input | output | node | edge
//   input      := "input"  IDENT ":" type [ "=" literal ]
//   output     := "output" IDENT ":" type
//   node       := "node"   UINT ":" kind [ IDENT ] "{" { pin } "}"
//   pin        := ( "in" | "out" ) IDENT ":" type [ "=" literal ]
//   edge       := "edge" UINT "." IDENT "->" UINT "." IDENT
//   literal    := INT | FLOAT | BOOL | STRING | "vec3(" FLOAT "," FLOAT "," FLOAT ")"
//
// Whitespace is insignificant; `#` to end-of-line is a comment.
//
// Errors contain a 1-based line number and a short diagnostic string.

#include "ir.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace gw::vscript {

struct ParseError {
    std::uint32_t line = 0;
    std::string   message;

    [[nodiscard]] std::string to_string() const;
};

// Parse a text IR document into a Graph. The returned graph has *not* yet
// been validated; call `validate(g)` to run the semantic checks.
[[nodiscard]] std::expected<Graph, ParseError>
parse(std::string_view text);

// Serialize a Graph back to canonical text IR. The result is deterministic:
// identical graphs produce byte-identical text (round-trip golden tests in
// tests/unit/vscript lean on this).
[[nodiscard]] std::string serialize(const Graph& g);

} // namespace gw::vscript
