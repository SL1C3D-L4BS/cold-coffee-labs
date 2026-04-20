#include "engine/memory/arena_allocator.hpp"
#include "engine/memory/frame_allocator.hpp"
#include "engine/memory/freelist_allocator.hpp"
#include "engine/memory/pool_allocator.hpp"

#include <doctest/doctest.h>

TEST_CASE("memory/frame_allocator allocates and resets") {
    gw::memory::FrameAllocator alloc{128};
    void* a = alloc.allocate(16, 8);
    void* b = alloc.allocate(16, 8);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(alloc.used() >= 32);
    alloc.reset();
    CHECK(alloc.used() == 0);
}

TEST_CASE("memory/arena_allocator grows blocks") {
    gw::memory::ArenaAllocator alloc{32};
    void* a = alloc.allocate(24, 8);
    void* b = alloc.allocate(24, 8);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(alloc.block_count() >= 2);
}

TEST_CASE("memory/pool_allocator capacity boundaries") {
    gw::memory::PoolAllocator alloc{16, 2};
    void* a = alloc.allocate();
    void* b = alloc.allocate();
    void* c = alloc.allocate();
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(c == nullptr);
    alloc.deallocate(a);
    CHECK(alloc.free_count() == 1);
}

TEST_CASE("memory/freelist_allocator reuses deallocated block") {
    gw::memory::FreelistAllocator alloc{128};
    void* a = alloc.allocate(32);
    REQUIRE(a != nullptr);
    alloc.deallocate(a, 32);
    void* b = alloc.allocate(32);
    CHECK(b == a);
}
