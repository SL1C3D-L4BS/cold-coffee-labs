// tests/unit/core/command_stack_test.cpp
// Phase 7 gate: CommandStack correctness — spec §17.
//
// <ostream> MUST be included before <doctest/doctest.h> on MSVC STL 14.44 +
// clang-cl: __msvc_string_view.hpp defines operator<<(ostream, string_view)
// inline and needs the COMPLETE basic_ostream type.  Doctest only
// forward-declares it in non-implement TUs, so we prime the definition here.
#include <ostream>
#include <doctest/doctest.h>
#include "engine/core/command.hpp"
#include <cstring>

using namespace gw::core;

// ---------------------------------------------------------------------------
// Test double
// ---------------------------------------------------------------------------
struct CountCmd final : ICommand {
    int&        counter;
    int         delta;
    const char* label;

    CountCmd(int& c, int d, const char* lbl = "count") noexcept
        : counter(c), delta(d), label(lbl) {}

    void execute()                    override { counter += delta; }
    void undo()                       override { counter -= delta; }
    [[nodiscard]] bool can_undo()
        const override               { return true; }
    [[nodiscard]] const char* name()
        const override               { return label; }
};

// Merges consecutive commands on the same target (same address).
struct MergeFloatCmd final : ICommand {
    float& target;
    float  before;
    float  after;

    MergeFloatCmd(float& t, float old_val, float new_val) noexcept
        : target(t), before(old_val), after(new_val) {}

    void execute()                    override { target = after; }
    void undo()                       override { target = before; }
    [[nodiscard]] bool can_undo()
        const override               { return true; }
    [[nodiscard]] const char* name()
        const override               { return "set_float"; }

    [[nodiscard]] bool try_merge(ICommand* newer) noexcept override {
        auto* n = dynamic_cast<MergeFloatCmd*>(newer);
        if (!n || &n->target != &target) return false;
        after = n->after;  // absorb the newer value
        target = after;    // apply immediately
        return true;
    }
};

// ---------------------------------------------------------------------------
TEST_CASE("CommandStack — push 5, undo 3, redo 2; cursor correct") {
    CommandStack cs;
    int val = 0;

    for (int i = 0; i < 5; ++i)
        cs.execute_and_push(std::make_unique<CountCmd>(val, 1));

    CHECK(val == 5);
    CHECK(cs.undo_depth() == 5);
    CHECK(cs.redo_depth() == 0);
    CHECK(cs.can_undo());
    CHECK_FALSE(cs.can_redo());

    CHECK(cs.undo()); val == 4;   // val is 4 now
    CHECK(cs.undo());             // val = 3
    CHECK(cs.undo());             // val = 2
    CHECK(val == 2);
    CHECK(cs.undo_depth() == 2);
    CHECK(cs.redo_depth() == 3);

    CHECK(cs.redo());             // val = 3
    CHECK(cs.redo());             // val = 4
    CHECK(val == 4);
    CHECK(cs.undo_depth() == 4);
    CHECK(cs.redo_depth() == 1);
}

TEST_CASE("CommandStack — dirty flag cleared by mark_saved") {
    CommandStack cs;
    int val = 0;
    CHECK_FALSE(cs.is_dirty());
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1));
    CHECK(cs.is_dirty());
    cs.mark_saved();
    CHECK_FALSE(cs.is_dirty());
    cs.undo();
    CHECK(cs.is_dirty());
}

TEST_CASE("CommandStack — merge: 10 consecutive drag commands coalesce to 1 undo step") {
    CommandStack cs;
    float x = 0.f;

    for (int i = 1; i <= 10; ++i)
        cs.execute_and_push(std::make_unique<MergeFloatCmd>(x,
            static_cast<float>(i - 1), static_cast<float>(i)));

    CHECK(x == doctest::Approx(10.f));
    CHECK(cs.undo_depth() == 1);   // all absorbed into first command

    cs.undo();
    CHECK(x == doctest::Approx(0.f));
    CHECK(cs.undo_depth() == 0);
}

TEST_CASE("CommandStack — transaction: begin/commit → single undo step") {
    CommandStack cs;
    int val = 0;

    cs.begin_transaction("multi_move");  // const char* — no string_view
    cs.execute_and_push(std::make_unique<CountCmd>(val, 3));
    cs.execute_and_push(std::make_unique<CountCmd>(val, 7));
    cs.commit_transaction();

    CHECK(val == 10);
    CHECK(cs.undo_depth() == 1);  // two commands wrapped into one step

    cs.undo();
    CHECK(val == 0);
    CHECK(cs.undo_depth() == 0);
}

TEST_CASE("CommandStack — transaction rollback reverts partial work") {
    CommandStack cs;
    int val = 0;

    cs.begin_transaction("partial");  // const char*
    cs.execute_and_push(std::make_unique<CountCmd>(val, 5));
    cs.execute_and_push(std::make_unique<CountCmd>(val, 3));
    cs.rollback_transaction();

    CHECK(val == 0);
    CHECK(cs.undo_depth() == 0);
    CHECK_FALSE(cs.in_transaction());
}

TEST_CASE("CommandStack — redo branch cleared on new command") {
    CommandStack cs;
    int val = 0;
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1)); // val=1
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1)); // val=2
    cs.undo();  // val=1, redo_depth=1
    CHECK(cs.redo_depth() == 1);
    cs.execute_and_push(std::make_unique<CountCmd>(val, 5)); // val=6, clears redo
    CHECK(cs.redo_depth() == 0);
    CHECK(val == 6);
}

TEST_CASE("CommandStack — max_undo evicts oldest") {
    CommandStack cs{3};  // max 3 undo steps
    int val = 0;
    for (int i = 0; i < 5; ++i)
        cs.execute_and_push(std::make_unique<CountCmd>(val, 1));

    CHECK(val == 5);
    CHECK(cs.undo_depth() == 3);  // oldest 2 evicted

    // Can only undo 3 times.
    CHECK(cs.undo()); CHECK(cs.undo()); CHECK(cs.undo());
    CHECK_FALSE(cs.undo());
    CHECK(val == 2);  // reversed 3 of 5
}

TEST_CASE("CommandStack — summary fills buffer with undo names") {
    CommandStack cs;
    int val = 0;
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1, "cmdA"));
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1, "cmdB"));
    cs.execute_and_push(std::make_unique<CountCmd>(val, 1, "cmdC"));

    char buf[256];
    cs.summary(buf, sizeof(buf), 10);
    // Most recent first: "cmdC|cmdB|cmdA"
    CHECK(strcmp(buf, "cmdC|cmdB|cmdA") == 0);
}
