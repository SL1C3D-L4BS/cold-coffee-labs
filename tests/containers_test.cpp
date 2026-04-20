#include "engine/core/array.hpp"
#include "engine/core/hash_map.hpp"
#include "engine/core/ring_buffer.hpp"
#include "engine/core/span.hpp"
#include "engine/core/vector.hpp"

#include <doctest/doctest.h>

TEST_CASE("core/array and span views work") {
    gw::core::Array<int, 3> values{1, 2, 3};
    gw::core::Span<int> span{values.data(), values.size()};
    CHECK(span.size() == 3);
    CHECK(span[1] == 2);
}

TEST_CASE("core/vector and ring_buffer push/pop") {
    gw::core::Vector<int> values{};
    values.push_back(10);
    values.push_back(20);
    CHECK(values.size() == 2);

    gw::core::RingBuffer<int> ring{2};
    CHECK(ring.push(1));
    CHECK(ring.push(2));
    CHECK_FALSE(ring.push(3));
    REQUIRE(ring.pop().has_value());
    CHECK(*ring.pop() == 2);
}

TEST_CASE("core/hash_map supports heterogeneous lookup") {
    gw::core::StringHashMap<int> map{};
    map.emplace("alpha", 1);
    const auto it = map.find("alpha");
    REQUIRE(it != map.end());
    CHECK(it->second == 1);
}
