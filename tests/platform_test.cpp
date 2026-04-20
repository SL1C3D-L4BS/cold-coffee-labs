#include "engine/platform/dll.hpp"
#include "engine/platform/fs.hpp"
#include "engine/platform/time.hpp"

#include <doctest/doctest.h>

#include <array>
#include <filesystem>

TEST_CASE("platform/time monotonic clock advances") {
    const auto t0 = gw::platform::Clock::now_ns();
    const auto t1 = gw::platform::Clock::now_ns();
    CHECK(t1 >= t0);
}

TEST_CASE("platform/fs write and read bytes") {
    const auto temp_path = std::filesystem::temp_directory_path() / "gw_platform_fs_test.bin";
    const std::vector<std::byte> payload{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    REQUIRE(gw::platform::FileSystem::write_bytes(temp_path, payload));
    const auto read = gw::platform::FileSystem::read_bytes(temp_path);
    REQUIRE(read.has_value());
    CHECK(read->size() == payload.size());
    std::filesystem::remove(temp_path);
}

TEST_CASE("platform/dll gracefully fails on missing library") {
    gw::platform::DynamicLibrary dll{};
    CHECK_FALSE(dll.open("gw_missing_library_for_test"));
    CHECK_FALSE(dll.is_open());
}
