// tests/unit/runtime/progs_vm_test.cpp — ADR-0122 bytecode + verifier.

#include <doctest/doctest.h>

#include "engine/runtime/progs_vm.hpp"

#include <cstdint>
#include <vector>

namespace {

void append_u32_le(std::vector<std::byte>& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        b.push_back(static_cast<std::byte>(v & 0xFFu));
        v >>= 8;
    }
}

void append_u16_le(std::vector<std::byte>& b, std::uint16_t v) {
    b.push_back(static_cast<std::byte>(v & 0xFFu));
    b.push_back(static_cast<std::byte>((v >> 8) & 0xFFu));
}

[[nodiscard]] std::vector<std::byte> make_add7_3_halt() {
    std::vector<std::byte> out;
    append_u32_le(out, 0x47575047u);  // GWPG
    append_u16_le(out, 1);            // version
    append_u16_le(out, 0);            // reserved
    // 8 words: PushImm 7,0  PushImm 3,0  Add  Halt
    append_u32_le(out, 8);
    append_u16_le(out, static_cast<std::uint16_t>(gw::runtime::progs::Opcode::PushImm));
    append_u16_le(out, 7);
    append_u16_le(out, 0);
    append_u16_le(out, static_cast<std::uint16_t>(gw::runtime::progs::Opcode::PushImm));
    append_u16_le(out, 3);
    append_u16_le(out, 0);
    append_u16_le(out, static_cast<std::uint16_t>(gw::runtime::progs::Opcode::Add));
    append_u16_le(out, static_cast<std::uint16_t>(gw::runtime::progs::Opcode::Halt));
    return out;
}

}  // namespace

TEST_CASE("progs_vm — verify rejects bad magic") {
    auto b = make_add7_3_halt();
    b[0]   = std::byte{0};
    CHECK(gw::runtime::progs::verify_bytecode(b) == gw::runtime::progs::VerifyResult::BadMagic);
}

TEST_CASE("progs_vm — verify accepts add program") {
    const auto b = make_add7_3_halt();
    CHECK(gw::runtime::progs::verify_bytecode(b) == gw::runtime::progs::VerifyResult::Ok);
}

TEST_CASE("progs_vm — run 7+3") {
    const auto                b = make_add7_3_halt();
    const gw::runtime::progs::RunResult r =
        gw::runtime::progs::run_bytecode(b, 64);
    CHECK(r.ok);
    CHECK_FALSE(r.stack_empty);
    CHECK(r.stack_top == 10);
    CHECK(r.cycles_used == 4);
}

TEST_CASE("progs_vm — tick_vm uses active module") {
    const auto b = make_add7_3_halt();
    gw::runtime::progs::set_active_bytecode(b);
    const std::uint32_t c = gw::runtime::progs::tick_vm(64);
    CHECK(c == 4);
    gw::runtime::progs::set_active_bytecode({});
}
