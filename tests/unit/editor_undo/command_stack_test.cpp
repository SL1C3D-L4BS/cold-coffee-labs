// tests/unit/editor_undo/command_stack_test.cpp
// Editor CommandStack — ADR-0005 contract tests.
//
// Covers: push/undo/redo round-trip, coalesce gating (same target + time window),
//         group semantics (begin/end + nesting), memory-budget FIFO trim,
//         redo-clear on fresh push, summary format.
//
// <ostream> MUST be included before <doctest/doctest.h> on MSVC STL 14.44 +
// clang-cl: __msvc_string_view.hpp defines operator<<(ostream, string_view)
// inline and needs the COMPLETE basic_ostream type.  Doctest only
// forward-declares it in non-implement TUs, so we prime the definition here.
#include <ostream>
#include <doctest/doctest.h>

#include "editor/undo/command_stack.hpp"

#include <cstring>
#include <string>
#include <thread>

using namespace gw::editor::undo;

namespace {

// -------------------------------------------------------------------------
// Test doubles
// -------------------------------------------------------------------------
struct CountCmd final : ICommand {
    int&        counter;
    int         delta;
    std::string lbl;

    CountCmd(int& c, int d, std::string l = "count") noexcept
        : counter(c), delta(d), lbl(std::move(l)) {}

    void do_()  override { counter += delta; }
    void undo() override { counter -= delta; }

    [[nodiscard]] std::string_view label()      const override { return lbl; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this) + lbl.capacity();
    }
};

// Coalescing command — merges on same target (address identity) within window.
struct DragCmd final : ICommand {
    float& target;
    float  before;
    float  after;

    DragCmd(float& t, float b, float a) noexcept
        : target(t), before(b), after(a) {}

    void do_()  override { target = after; }
    void undo() override { target = before; }

    [[nodiscard]] std::string_view label()      const override { return "drag"; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this);
    }

    [[nodiscard]] bool try_coalesce(
        const ICommand& other,
        std::chrono::steady_clock::time_point /*now*/) override {
        const auto* o = dynamic_cast<const DragCmd*>(&other);
        if (!o || &o->target != &target) return false;
        after  = o->after;
        target = after;
        return true;
    }
};

// Big-payload command for memory-budget tests.
struct FatCmd final : ICommand {
    std::string payload;

    explicit FatCmd(std::size_t bytes) : payload(bytes, 'x') {}

    void do_()  override {}
    void undo() override {}

    [[nodiscard]] std::string_view label()      const override { return "fat"; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this) + payload.capacity();
    }
};

}  // namespace

// ---------------------------------------------------------------------------
TEST_CASE("CommandStack — push 5, undo 3, redo 2") {
    CommandStack cs;
    int val = 0;

    for (int i = 0; i < 5; ++i)
        cs.push(std::make_unique<CountCmd>(val, 1));

    CHECK(val == 5);
    CHECK(cs.undo_depth() == 5);
    CHECK(cs.redo_depth() == 0);
    CHECK(cs.can_undo());
    CHECK_FALSE(cs.can_redo());

    cs.undo();
    cs.undo();
    cs.undo();
    CHECK(val == 2);
    CHECK(cs.undo_depth() == 2);
    CHECK(cs.redo_depth() == 3);

    cs.redo();
    cs.redo();
    CHECK(val == 4);
    CHECK(cs.undo_depth() == 4);
    CHECK(cs.redo_depth() == 1);
}

TEST_CASE("CommandStack — redo branch cleared on new push") {
    CommandStack cs;
    int val = 0;
    cs.push(std::make_unique<CountCmd>(val, 1));
    cs.push(std::make_unique<CountCmd>(val, 1));
    cs.undo();
    CHECK(cs.redo_depth() == 1);
    cs.push(std::make_unique<CountCmd>(val, 5));
    CHECK(cs.redo_depth() == 0);
    CHECK(val == 6);
}

TEST_CASE("CommandStack — coalesce: ten drags on same target → one undo step") {
    CommandStack cs;
    float x = 0.f;

    for (int i = 1; i <= 10; ++i)
        cs.push(std::make_unique<DragCmd>(x,
            static_cast<float>(i - 1), static_cast<float>(i)));

    CHECK(x == doctest::Approx(10.f));
    CHECK(cs.undo_depth() == 1);  // coalesced

    cs.undo();
    CHECK(x == doctest::Approx(0.f));
}

TEST_CASE("CommandStack — coalesce gated by time window") {
    CommandStack::Config cfg{};
    cfg.coalesce_window = std::chrono::milliseconds{5};
    CommandStack cs{cfg};

    float x = 0.f;
    cs.push(std::make_unique<DragCmd>(x, 0.f, 1.f));
    std::this_thread::sleep_for(std::chrono::milliseconds{15});
    cs.push(std::make_unique<DragCmd>(x, 1.f, 2.f));

    CHECK(cs.undo_depth() == 2);  // outside window → no coalesce
}

TEST_CASE("CommandStack — coalesce disabled when requested") {
    CommandStack cs;
    cs.set_coalescing_enabled(false);
    float x = 0.f;
    cs.push(std::make_unique<DragCmd>(x, 0.f, 1.f));
    cs.push(std::make_unique<DragCmd>(x, 1.f, 2.f));
    CHECK(cs.undo_depth() == 2);
}

TEST_CASE("CommandStack — group: begin/end collapses N commands into 1 undo step") {
    CommandStack cs;
    int val = 0;

    cs.begin_group("multi_move");
    cs.push(std::make_unique<CountCmd>(val, 3));
    cs.push(std::make_unique<CountCmd>(val, 7));
    cs.end_group();

    CHECK(val == 10);
    CHECK(cs.undo_depth() == 1);

    cs.undo();
    CHECK(val == 0);
}

TEST_CASE("CommandStack — nested groups become a single top-level step") {
    CommandStack cs;
    int val = 0;

    cs.begin_group("outer");
    cs.push(std::make_unique<CountCmd>(val, 1));
    cs.begin_group("inner");
    cs.push(std::make_unique<CountCmd>(val, 2));
    cs.push(std::make_unique<CountCmd>(val, 3));
    cs.end_group();
    cs.push(std::make_unique<CountCmd>(val, 4));
    cs.end_group();

    CHECK(val == 10);
    CHECK(cs.undo_depth() == 1);

    cs.undo();
    CHECK(val == 0);
}

TEST_CASE("CommandStack — empty group discarded") {
    CommandStack cs;
    cs.begin_group("empty");
    cs.end_group();
    CHECK(cs.undo_depth() == 0);
    CHECK_FALSE(cs.can_undo());
}

TEST_CASE("CommandStack — memory budget evicts oldest entries FIFO") {
    CommandStack::Config cfg{};
    cfg.memory_budget_bytes = 4 * 1024;  // 4 KiB cap
    CommandStack cs{cfg};

    // Each FatCmd is ~1200 bytes including string capacity.
    for (int i = 0; i < 10; ++i)
        cs.push(std::make_unique<FatCmd>(1024));

    CHECK(cs.memory_used() <= cfg.memory_budget_bytes);
    CHECK(cs.undo_depth() < 10);  // some evicted
    CHECK(cs.undo_depth() >= 1);
}

TEST_CASE("CommandStack — dirty flag cleared by mark_saved") {
    CommandStack cs;
    int val = 0;
    CHECK_FALSE(cs.is_dirty());
    cs.push(std::make_unique<CountCmd>(val, 1));
    CHECK(cs.is_dirty());
    cs.mark_saved();
    CHECK_FALSE(cs.is_dirty());
    cs.undo();
    CHECK(cs.is_dirty());
}

TEST_CASE("CommandStack — summary lists most-recent-first with '|' separator") {
    CommandStack cs;
    int val = 0;
    cs.push(std::make_unique<CountCmd>(val, 1, "cmdA"));
    cs.push(std::make_unique<CountCmd>(val, 1, "cmdB"));
    cs.push(std::make_unique<CountCmd>(val, 1, "cmdC"));

    char buf[256];
    cs.summary(buf, sizeof(buf), 10);
    CHECK(std::strcmp(buf, "cmdC|cmdB|cmdA") == 0);
}

TEST_CASE("CommandStack — next_undo/redo_label reflects the top") {
    CommandStack cs;
    int val = 0;
    CHECK(cs.next_undo_label().empty());

    cs.push(std::make_unique<CountCmd>(val, 1, "A"));
    cs.push(std::make_unique<CountCmd>(val, 1, "B"));

    CHECK(cs.next_undo_label() == "B");
    cs.undo();
    CHECK(cs.next_undo_label() == "A");
    CHECK(cs.next_redo_label() == "B");
}
