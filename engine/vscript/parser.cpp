// engine/vscript/parser.cpp
#include "parser.hpp"

#include <cctype>
#include <charconv>
#include <cstdio>
#include <sstream>

namespace gw::vscript {

std::string ParseError::to_string() const {
    std::string out;
    out.reserve(32 + message.size());
    out += "parse error (line ";
    out += std::to_string(line);
    out += "): ";
    out += message;
    return out;
}

// ---------------------------------------------------------------------------
// Tokenizer — hand-rolled to avoid pulling in a parser generator and to keep
// the error surface aligned with the grammar above.
// ---------------------------------------------------------------------------
namespace {

enum class Tok : std::uint8_t {
    End,
    Ident,       // also holds keywords; the parser matches them by string
    Int,
    Float,
    String,      // payload = decoded contents, no surrounding quotes
    LBrace,      // {
    RBrace,      // }
    LParen,      // (
    RParen,      // )
    Colon,       // :
    Comma,       // ,
    Eq,          // =
    Arrow,       // ->
    Dot,         // .
};

struct Token {
    Tok           kind = Tok::End;
    std::string   text;          // raw text (or decoded string)
    double        fvalue = 0.0;  // populated for Float
    std::int64_t  ivalue = 0;    // populated for Int
    std::uint32_t line  = 0;
};

class Lexer {
public:
    explicit Lexer(std::string_view s) : src_(s) {
        // Strip a leading UTF-8 BOM (EF BB BF) if present. Users pasting text
        // out of a GUI editor or a Windows Notepad save will commonly carry
        // a BOM, and we do not want the first lex step to fail on it.
        if (src_.size() >= 3 &&
            static_cast<unsigned char>(src_[0]) == 0xEFu &&
            static_cast<unsigned char>(src_[1]) == 0xBBu &&
            static_cast<unsigned char>(src_[2]) == 0xBFu) {
            pos_ = 3;
        }
    }

    std::expected<Token, ParseError> next() {
        skip_ws_and_comments();
        if (pos_ >= src_.size()) return Token{Tok::End, {}, 0.0, 0, line_};

        const char c = src_[pos_];
        const auto ln = line_;

        if (c == '{') { ++pos_; return Token{Tok::LBrace, "{", 0, 0, ln}; }
        if (c == '}') { ++pos_; return Token{Tok::RBrace, "}", 0, 0, ln}; }
        if (c == '(') { ++pos_; return Token{Tok::LParen, "(", 0, 0, ln}; }
        if (c == ')') { ++pos_; return Token{Tok::RParen, ")", 0, 0, ln}; }
        if (c == ':') { ++pos_; return Token{Tok::Colon,  ":", 0, 0, ln}; }
        if (c == ',') { ++pos_; return Token{Tok::Comma,  ",", 0, 0, ln}; }
        if (c == '=') { ++pos_; return Token{Tok::Eq,     "=", 0, 0, ln}; }
        if (c == '.') { ++pos_; return Token{Tok::Dot,    ".", 0, 0, ln}; }
        if (c == '-' && pos_ + 1 < src_.size() && src_[pos_+1] == '>') {
            pos_ += 2; return Token{Tok::Arrow, "->", 0, 0, ln};
        }

        if (c == '"') return lex_string();
        if (c == '-' || c == '+' || std::isdigit(static_cast<unsigned char>(c))) {
            return lex_number();
        }
        if (c == '_' || std::isalpha(static_cast<unsigned char>(c))) {
            return lex_ident();
        }
        // Build a diagnostic that prints both the glyph (if printable) and
        // the raw byte value. This turns "unexpected character 'X'" into
        // something actionable when X is a stray control byte, a BOM frag,
        // or a high-ASCII character pasted from rich-text.
        const auto uc = static_cast<unsigned char>(c);
        char hex[8];
        std::snprintf(hex, sizeof(hex), "0x%02X", uc);
        std::string msg = "unexpected character ";
        if (std::isprint(uc)) {
            msg.push_back('\'');
            msg.push_back(c);
            msg += "' (";
            msg += hex;
            msg.push_back(')');
        } else {
            msg += hex;
        }
        return std::unexpected(ParseError{ln, std::move(msg)});
    }

private:
    void skip_ws_and_comments() {
        while (pos_ < src_.size()) {
            const char c = src_[pos_];
            if (c == '\n') { ++line_; ++pos_; continue; }
            if (std::isspace(static_cast<unsigned char>(c))) { ++pos_; continue; }
            // Tolerate zero-width / non-breaking spaces that commonly sneak
            // in via clipboard paste (U+200B, U+200C, U+200D, U+FEFF,
            // U+00A0 as UTF-8 C2 A0). These are not useful as tokens.
            const auto uc = static_cast<unsigned char>(c);
            if (uc == 0xC2u && pos_ + 1 < src_.size() &&
                static_cast<unsigned char>(src_[pos_ + 1]) == 0xA0u) {
                pos_ += 2; continue; // U+00A0 NBSP
            }
            if (uc == 0xE2u && pos_ + 2 < src_.size() &&
                static_cast<unsigned char>(src_[pos_ + 1]) == 0x80u) {
                const auto b2 = static_cast<unsigned char>(src_[pos_ + 2]);
                if (b2 == 0x8Bu || b2 == 0x8Cu || b2 == 0x8Du) {
                    pos_ += 3; continue; // U+200B/C/D ZWSP/ZWNJ/ZWJ
                }
            }
            if (uc == 0xEFu && pos_ + 2 < src_.size() &&
                static_cast<unsigned char>(src_[pos_ + 1]) == 0xBBu &&
                static_cast<unsigned char>(src_[pos_ + 2]) == 0xBFu) {
                pos_ += 3; continue; // stray UTF-8 BOM mid-stream
            }
            if (c == '#') {
                while (pos_ < src_.size() && src_[pos_] != '\n') ++pos_;
                continue;
            }
            break;
        }
    }

    std::expected<Token, ParseError> lex_ident() {
        const auto ln = line_;
        const auto begin = pos_;
        while (pos_ < src_.size()) {
            const char c = src_[pos_];
            if (c == '_' || std::isalnum(static_cast<unsigned char>(c))) ++pos_;
            else break;
        }
        return Token{Tok::Ident, std::string{src_.substr(begin, pos_ - begin)},
                      0.0, 0, ln};
    }

    std::expected<Token, ParseError> lex_number() {
        const auto ln = line_;
        const auto begin = pos_;
        // Allow leading sign.
        if (src_[pos_] == '+' || src_[pos_] == '-') ++pos_;
        bool is_float = false;
        // Integer part.
        while (pos_ < src_.size() &&
               std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
        // Fractional part — only consume `.` if a digit follows. `3.lhs`
        // must tokenize as Int(3), Dot, Ident(lhs) so `edge 3.lhs` parses.
        if (pos_ < src_.size() && src_[pos_] == '.' &&
            pos_ + 1 < src_.size() &&
            std::isdigit(static_cast<unsigned char>(src_[pos_ + 1]))) {
            is_float = true;
            ++pos_;
            while (pos_ < src_.size() &&
                   std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }
        // Exponent — `e` / `E` followed by optional sign + digits.
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            auto save = pos_;
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            if (pos_ < src_.size() &&
                std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                is_float = true;
                while (pos_ < src_.size() &&
                       std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                    ++pos_;
                }
            } else {
                // Not an exponent — roll back.
                pos_ = save;
            }
        }
        const auto text = src_.substr(begin, pos_ - begin);
        Token t{is_float ? Tok::Float : Tok::Int, std::string{text}, 0.0, 0, ln};
        if (is_float) {
            try { t.fvalue = std::stod(std::string{text}); }
            catch (...) {
                return std::unexpected(ParseError{ln,
                    "invalid floating-point literal '" + std::string{text} + "'"});
            }
        } else {
            std::int64_t v = 0;
            auto [p, ec] = std::from_chars(text.data(),
                                            text.data() + text.size(), v);
            if (ec != std::errc{}) {
                return std::unexpected(ParseError{ln,
                    "invalid integer literal '" + std::string{text} + "'"});
            }
            t.ivalue = v;
        }
        return t;
    }

    std::expected<Token, ParseError> lex_string() {
        const auto ln = line_;
        ++pos_;  // opening "
        std::string buf;
        while (pos_ < src_.size()) {
            const char c = src_[pos_++];
            if (c == '"') return Token{Tok::String, std::move(buf), 0.0, 0, ln};
            if (c == '\\' && pos_ < src_.size()) {
                const char n = src_[pos_++];
                switch (n) {
                    case 'n': buf.push_back('\n'); break;
                    case 't': buf.push_back('\t'); break;
                    case '\\': buf.push_back('\\'); break;
                    case '"': buf.push_back('"');  break;
                    default:  buf.push_back(n);    break;
                }
                continue;
            }
            if (c == '\n') ++line_;
            buf.push_back(c);
        }
        return std::unexpected(ParseError{ln, "unterminated string literal"});
    }

    std::string_view src_;
    std::size_t      pos_  = 0;
    std::uint32_t    line_ = 1;
};

// ---------------------------------------------------------------------------
// Recursive-descent parser.
// ---------------------------------------------------------------------------
class Parser {
public:
    explicit Parser(Lexer& lx) : lx_(lx) {}

    std::expected<Graph, ParseError> parse_graph() {
        Graph g;
        auto t = advance();
        if (!t) return std::unexpected(t.error());
        if (!(cur_.kind == Tok::Ident && cur_.text == "graph")) {
            return std::unexpected(err("expected 'graph'"));
        }
        if (auto e = expect_ident(); !e) return std::unexpected(e.error());
        g.name = std::move(cur_.text);

        if (auto e = expect(Tok::LBrace); !e) return std::unexpected(e.error());
        while (true) {
            auto n = advance();
            if (!n) return std::unexpected(n.error());
            if (cur_.kind == Tok::RBrace) break;
            if (cur_.kind != Tok::Ident) {
                return std::unexpected(err("expected keyword or '}'"));
            }
            if (cur_.text == "input")       { if (auto r = parse_input(g);  !r) return std::unexpected(r.error()); }
            else if (cur_.text == "output") { if (auto r = parse_output(g); !r) return std::unexpected(r.error()); }
            else if (cur_.text == "node")   { if (auto r = parse_node(g);   !r) return std::unexpected(r.error()); }
            else if (cur_.text == "edge")   { if (auto r = parse_edge(g);   !r) return std::unexpected(r.error()); }
            else return std::unexpected(err("unknown keyword '" + cur_.text + "'"));
        }
        return g;
    }

private:
    std::expected<void, ParseError> advance() {
        auto t = next_token();
        if (!t) return std::unexpected(t.error());
        cur_ = std::move(*t);
        return {};
    }

    ParseError err(std::string msg) {
        return ParseError{cur_.line, std::move(msg)};
    }

    std::expected<void, ParseError> expect(Tok k) {
        if (auto e = advance(); !e) return std::unexpected(e.error());
        if (cur_.kind != k) return std::unexpected(err("unexpected token"));
        return {};
    }
    std::expected<void, ParseError> expect_ident() {
        if (auto e = advance(); !e) return std::unexpected(e.error());
        if (cur_.kind != Tok::Ident)
            return std::unexpected(err("expected identifier"));
        return {};
    }

    std::expected<Type, ParseError> parse_type_name() {
        if (auto e = expect_ident(); !e) return std::unexpected(e.error());
        const auto t = ::gw::vscript::parse_type(cur_.text);
        if (!t) return std::unexpected(err("unknown type '" + cur_.text + "'"));
        return *t;
    }

    std::expected<Value, ParseError> parse_literal(Type want) {
        if (auto e = advance(); !e) return std::unexpected(e.error());
        switch (want) {
            case Type::Int:
                if (cur_.kind != Tok::Int)
                    return std::unexpected(err("expected integer literal"));
                return Value{cur_.ivalue};
            case Type::Float:
                if (cur_.kind == Tok::Float) return Value{cur_.fvalue};
                if (cur_.kind == Tok::Int)   return Value{static_cast<double>(cur_.ivalue)};
                return std::unexpected(err("expected float literal"));
            case Type::Bool:
                if (cur_.kind != Tok::Ident ||
                    (cur_.text != "true" && cur_.text != "false"))
                    return std::unexpected(err("expected 'true' or 'false'"));
                return Value{cur_.text == "true"};
            case Type::String:
                if (cur_.kind != Tok::String)
                    return std::unexpected(err("expected string literal"));
                return Value{cur_.text};
            case Type::Vec3: {
                // Expect "vec3 ( <f> , <f> , <f> )" but we already consumed
                // the "vec3" identifier above (cur_ is sitting on it). If a
                // user wrote "= vec3(...)" we see the ident here.
                if (cur_.kind != Tok::Ident || cur_.text != "vec3")
                    return std::unexpected(err("expected 'vec3(...)' literal"));
                if (auto e = expect(Tok::LParen); !e) return std::unexpected(e.error());
                auto rx = parse_literal(Type::Float);
                if (!rx) return std::unexpected(rx.error());
                if (auto e = expect(Tok::Comma); !e) return std::unexpected(e.error());
                auto ry = parse_literal(Type::Float);
                if (!ry) return std::unexpected(ry.error());
                if (auto e = expect(Tok::Comma); !e) return std::unexpected(e.error());
                auto rz = parse_literal(Type::Float);
                if (!rz) return std::unexpected(rz.error());
                if (auto e = expect(Tok::RParen); !e) return std::unexpected(e.error());
                return Value{Vec3Value{
                    static_cast<float>(std::get<double>(*rx)),
                    static_cast<float>(std::get<double>(*ry)),
                    static_cast<float>(std::get<double>(*rz))}};
            }
            case Type::Void:
                return std::unexpected(err("void has no literal form"));
        }
        return std::unexpected(err("unreachable literal type"));
    }

    std::expected<void, ParseError> parse_input(Graph& g) {
        if (auto e = expect_ident(); !e) return std::unexpected(e.error());
        IOPort p;
        p.name = std::move(cur_.text);
        if (auto e = expect(Tok::Colon); !e) return std::unexpected(e.error());
        auto ty = parse_type_name();
        if (!ty) return std::unexpected(ty.error());
        p.type = *ty;
        p.value = default_value(*ty);
        // Optional default literal.
        auto peek = lx_.next();
        if (!peek) return std::unexpected(peek.error());
        if (peek->kind == Tok::Eq) {
            // consume =, then value
            cur_ = std::move(*peek);
            auto lit = parse_literal(*ty);
            if (!lit) return std::unexpected(lit.error());
            p.value = std::move(*lit);
        } else {
            // Put token back — the lexer has no ungetc, so we stash it.
            pending_ = std::move(*peek);
            has_pending_ = true;
        }
        g.inputs.push_back(std::move(p));
        return {};
    }

    std::expected<void, ParseError> parse_output(Graph& g) {
        if (auto e = expect_ident(); !e) return std::unexpected(e.error());
        IOPort p;
        p.name = std::move(cur_.text);
        if (auto e = expect(Tok::Colon); !e) return std::unexpected(e.error());
        auto ty = parse_type_name();
        if (!ty) return std::unexpected(ty.error());
        p.type = *ty;
        p.value = default_value(*ty);
        g.outputs.push_back(std::move(p));
        return {};
    }

    std::expected<void, ParseError> parse_node(Graph& g) {
        if (auto e = advance(); !e) return std::unexpected(e.error());
        if (cur_.kind != Tok::Int)
            return std::unexpected(err("expected node id (integer)"));
        Node n;
        n.id = static_cast<std::uint32_t>(cur_.ivalue);
        if (auto e = expect(Tok::Colon); !e) return std::unexpected(e.error());
        if (auto e = expect_ident(); !e) return std::unexpected(e.error());
        auto kind = parse_node_kind(cur_.text);
        if (!kind) return std::unexpected(err("unknown node kind '" + cur_.text + "'"));
        n.kind = *kind;

        // Optional debug name.
        auto peek = next_token();
        if (!peek) return std::unexpected(peek.error());
        if (peek->kind == Tok::Ident) {
            n.debug_name = std::move(peek->text);
            peek = next_token();
            if (!peek) return std::unexpected(peek.error());
        }
        if (peek->kind != Tok::LBrace)
            return std::unexpected(ParseError{peek->line, "expected '{' in node body"});
        cur_ = std::move(*peek);

        while (true) {
            auto tk = next_token();
            if (!tk) return std::unexpected(tk.error());
            if (tk->kind == Tok::RBrace) break;
            if (tk->kind != Tok::Ident ||
                (tk->text != "in" && tk->text != "out")) {
                return std::unexpected(ParseError{tk->line, "expected 'in' or 'out'"});
            }
            const bool is_in = (tk->text == "in");
            cur_ = std::move(*tk);
            if (auto e = expect_ident(); !e) return std::unexpected(e.error());
            Pin p;
            p.name = std::move(cur_.text);
            if (auto e = expect(Tok::Colon); !e) return std::unexpected(e.error());
            auto ty = parse_type_name();
            if (!ty) return std::unexpected(ty.error());
            p.type = *ty;

            auto maybe_eq = next_token();
            if (!maybe_eq) return std::unexpected(maybe_eq.error());
            if (maybe_eq->kind == Tok::Eq) {
                cur_ = std::move(*maybe_eq);
                auto lit = parse_literal(*ty);
                if (!lit) return std::unexpected(lit.error());
                p.literal = std::move(*lit);
            } else {
                pending_ = std::move(*maybe_eq);
                has_pending_ = true;
            }
            (is_in ? n.inputs : n.outputs).push_back(std::move(p));
        }
        g.nodes.push_back(std::move(n));
        return {};
    }

    std::expected<void, ParseError> parse_edge(Graph& g) {
        Edge e{};
        if (auto r = advance(); !r) return std::unexpected(r.error());
        if (cur_.kind != Tok::Int) return std::unexpected(err("expected source node id"));
        e.src_node = static_cast<std::uint32_t>(cur_.ivalue);
        if (auto r = expect(Tok::Dot); !r) return std::unexpected(r.error());
        if (auto r = expect_ident(); !r) return std::unexpected(r.error());
        e.src_pin = std::move(cur_.text);
        if (auto r = expect(Tok::Arrow); !r) return std::unexpected(r.error());
        if (auto r = advance(); !r) return std::unexpected(r.error());
        if (cur_.kind != Tok::Int) return std::unexpected(err("expected dest node id"));
        e.dst_node = static_cast<std::uint32_t>(cur_.ivalue);
        if (auto r = expect(Tok::Dot); !r) return std::unexpected(r.error());
        if (auto r = expect_ident(); !r) return std::unexpected(r.error());
        e.dst_pin = std::move(cur_.text);
        g.edges.push_back(std::move(e));
        return {};
    }

    std::expected<Token, ParseError> next_token() {
        if (has_pending_) {
            has_pending_ = false;
            return std::move(pending_);
        }
        return lx_.next();
    }

    Lexer& lx_;
    Token  cur_{};
    Token  pending_{};
    bool   has_pending_ = false;
};

} // namespace

std::expected<Graph, ParseError> parse(std::string_view text) {
    Lexer  lx{text};
    Parser p{lx};
    return p.parse_graph();
}

// ---------------------------------------------------------------------------
// Serializer.
// ---------------------------------------------------------------------------
namespace {

void emit_value(std::string& out, const Value& v) {
    struct Visitor {
        std::string& out;
        void operator()(std::int64_t x) const {
            out += std::to_string(x);
        }
        void operator()(double x) const {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.17g", x);
            std::string s{buf};
            // Ensure a '.' so the re-read lands on Tok::Float, not Tok::Int,
            // even for values like 1e6 that already parse as float.
            bool has_dot_or_exp = false;
            for (char c : s) if (c == '.' || c == 'e' || c == 'E') { has_dot_or_exp = true; break; }
            if (!has_dot_or_exp) s += ".0";
            out += s;
        }
        void operator()(bool b) const { out += b ? "true" : "false"; }
        void operator()(const std::string& s) const {
            out.push_back('"');
            for (char c : s) {
                switch (c) {
                    case '"':  out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n";  break;
                    case '\t': out += "\\t";  break;
                    default:   out.push_back(c); break;
                }
            }
            out.push_back('"');
        }
        void operator()(const Vec3Value& v) const {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "vec3(%.9g, %.9g, %.9g)",
                          static_cast<double>(v.x),
                          static_cast<double>(v.y),
                          static_cast<double>(v.z));
            out += buf;
        }
        void operator()(std::monostate) const { /* void: nothing */ }
    };
    std::visit(Visitor{out}, v);
}

} // namespace

std::string serialize(const Graph& g) {
    std::string out;
    out.reserve(256);
    out += "graph ";
    out += g.name;
    out += " {\n";

    for (const auto& p : g.inputs) {
        out += "  input ";
        out += p.name;
        out += " : ";
        out += type_name(p.type);
        if (type_of(p.value) == p.type && !std::holds_alternative<std::monostate>(p.value)) {
            out += " = ";
            emit_value(out, p.value);
        }
        out += '\n';
    }
    for (const auto& p : g.outputs) {
        out += "  output ";
        out += p.name;
        out += " : ";
        out += type_name(p.type);
        out += '\n';
    }
    for (const auto& n : g.nodes) {
        out += "  node ";
        out += std::to_string(n.id);
        out += " : ";
        out += node_kind_name(n.kind);
        if (!n.debug_name.empty()) { out += ' '; out += n.debug_name; }
        out += " {\n";
        for (const auto& p : n.inputs) {
            out += "    in ";
            out += p.name;
            out += " : ";
            out += type_name(p.type);
            if (p.literal) {
                out += " = ";
                emit_value(out, *p.literal);
            }
            out += '\n';
        }
        for (const auto& p : n.outputs) {
            out += "    out ";
            out += p.name;
            out += " : ";
            out += type_name(p.type);
            out += '\n';
        }
        out += "  }\n";
    }
    for (const auto& e : g.edges) {
        out += "  edge ";
        out += std::to_string(e.src_node);
        out += '.';
        out += e.src_pin;
        out += " -> ";
        out += std::to_string(e.dst_node);
        out += '.';
        out += e.dst_pin;
        out += '\n';
    }
    out += "}\n";
    return out;
}

} // namespace gw::vscript
