// engine/i18n/xliff.cpp — deterministic XLIFF 2.1 round-trip.

#include "engine/i18n/xliff.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace gw::i18n::xliff {

namespace {

void xml_escape_into(std::string_view in, std::string& out) {
    out.reserve(out.size() + in.size());
    for (char c : in) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out.push_back(c); break;
        }
    }
}

std::string xml_unescape(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size();) {
        if (in[i] == '&') {
            auto semi = in.find(';', i + 1);
            if (semi != std::string_view::npos) {
                auto ent = in.substr(i + 1, semi - i - 1);
                if      (ent == "amp")  { out.push_back('&');  i = semi + 1; continue; }
                else if (ent == "lt")   { out.push_back('<');  i = semi + 1; continue; }
                else if (ent == "gt")   { out.push_back('>');  i = semi + 1; continue; }
                else if (ent == "quot") { out.push_back('"');  i = semi + 1; continue; }
                else if (ent == "apos") { out.push_back('\''); i = semi + 1; continue; }
                if (!ent.empty() && ent[0] == '#') {
                    long v = 0;
                    try {
                        v = (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X'))
                            ? std::stol(std::string(ent.substr(2)), nullptr, 16)
                            : std::stol(std::string(ent.substr(1)), nullptr, 10);
                    } catch (...) { v = '?'; }
                    if (v < 0x80) {
                        out.push_back(static_cast<char>(v));
                    } else {
                        out.push_back('?'); // non-ASCII numeric refs ignored in shim
                    }
                    i = semi + 1;
                    continue;
                }
            }
        }
        out.push_back(in[i]);
        ++i;
    }
    return out;
}

// Read attribute value `name="..."` from tag content (between '<' and '>').
std::string attr_value(std::string_view open_tag, std::string_view name) {
    auto pos = open_tag.find(name);
    while (pos != std::string_view::npos) {
        std::size_t end = pos + name.size();
        if (end < open_tag.size() && (open_tag[end] == '=' || open_tag[end] == ' ')) {
            std::size_t eq = open_tag.find('=', end);
            if (eq == std::string_view::npos) return {};
            std::size_t q = eq + 1;
            while (q < open_tag.size() && (open_tag[q] == ' ' || open_tag[q] == '\t')) ++q;
            if (q >= open_tag.size() || (open_tag[q] != '"' && open_tag[q] != '\'')) return {};
            const char qc = open_tag[q++];
            std::size_t endq = open_tag.find(qc, q);
            if (endq == std::string_view::npos) return {};
            return xml_unescape(open_tag.substr(q, endq - q));
        }
        pos = open_tag.find(name, pos + name.size());
    }
    return {};
}

// Extract text between the first `<elem>` close-angle and matching `</elem>`.
// Returns true on success.
bool extract_element_text(std::string_view haystack,
                            std::string_view elem,
                            std::size_t      start,
                            std::string&     out,
                            std::size_t&     end_after) {
    std::string open_needle("<");
    open_needle += elem;
    auto a = haystack.find(open_needle, start);
    if (a == std::string_view::npos) return false;
    auto gt = haystack.find('>', a);
    if (gt == std::string_view::npos) return false;
    std::string close_needle("</");
    close_needle += elem;
    close_needle += ">";
    auto c = haystack.find(close_needle, gt);
    if (c == std::string_view::npos) return false;
    out = xml_unescape(haystack.substr(gt + 1, c - gt - 1));
    end_after = c + close_needle.size();
    return true;
}

} // namespace

XliffError parse(std::string_view xml, std::vector<Unit>& out) {
    auto file_pos = xml.find("<file");
    if (file_pos == std::string_view::npos) return XliffError::MissingFile;

    std::size_t pos = file_pos;
    while (true) {
        auto u = xml.find("<unit", pos);
        if (u == std::string_view::npos) break;
        auto gt = xml.find('>', u);
        if (gt == std::string_view::npos) break;
        std::string_view open_tag = xml.substr(u, gt - u + 1);
        auto close = xml.find("</unit>", gt);
        if (close == std::string_view::npos) break;
        std::string_view body = xml.substr(gt + 1, close - gt - 1);

        Unit unit{};
        unit.id = attr_value(open_tag, "id");
        std::size_t after = 0;
        std::string text;
        if (extract_element_text(body, "source", 0, text, after)) unit.source_utf8 = std::move(text);
        if (extract_element_text(body, "target", 0, text, after)) unit.target_utf8 = std::move(text);
        if (extract_element_text(body, "note",   0, text, after)) unit.notes       = std::move(text);

        if (unit.id.empty()) return XliffError::UnitParseError;
        out.push_back(std::move(unit));

        pos = close + std::strlen("</unit>");
    }
    return XliffError::Ok;
}

std::string write(std::string_view src_lang,
                   std::string_view tgt_lang,
                   const std::vector<Unit>& units) {
    std::string out;
    out.reserve(256 + units.size() * 128);
    out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<xliff xmlns=\"urn:oasis:names:tc:xliff:document:2.1\" version=\"2.1\" srcLang=\"";
    xml_escape_into(src_lang, out);
    out += "\" trgLang=\"";
    xml_escape_into(tgt_lang, out);
    out += "\">\n  <file id=\"f1\">\n";

    auto sorted = units;
    std::sort(sorted.begin(), sorted.end(), [](const Unit& a, const Unit& b) {
        return a.id < b.id;
    });
    for (const auto& u : sorted) {
        out += "    <unit id=\"";
        xml_escape_into(u.id, out);
        out += "\">\n      <segment>\n        <source>";
        xml_escape_into(u.source_utf8, out);
        out += "</source>\n        <target>";
        xml_escape_into(u.target_utf8, out);
        out += "</target>\n      </segment>\n";
        if (!u.notes.empty()) {
            out += "      <notes><note>";
            xml_escape_into(u.notes, out);
            out += "</note></notes>\n";
        }
        out += "    </unit>\n";
    }
    out += "  </file>\n</xliff>\n";
    return out;
}

std::vector<std::pair<std::string, std::string>> to_kv(const std::vector<Unit>& units) {
    std::vector<std::pair<std::string, std::string>> kv;
    kv.reserve(units.size());
    for (const auto& u : units) {
        if (!u.target_utf8.empty()) kv.emplace_back(u.id, u.target_utf8);
    }
    return kv;
}

} // namespace gw::i18n::xliff
