// tests/unit/assets/vfs_test.cpp
// Phase 6 spec §13.1: VirtualFilesystem tests.
// No Vulkan, no GPU — pure VFS logic.
#include <doctest/doctest.h>
#include "engine/assets/vfs/virtual_fs.hpp"
#include <string>
#include <vector>
#include <unordered_map>

using namespace gw::assets::vfs;
using namespace gw::assets;

static std::unordered_map<std::string, std::vector<uint8_t>>
make_files(std::initializer_list<std::pair<const char*, const char*>> entries) {
    std::unordered_map<std::string, std::vector<uint8_t>> m;
    for (auto& [k, v] : entries) {
        m[k] = std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(v),
                                     reinterpret_cast<const uint8_t*>(v) + std::strlen(v));
    }
    return m;
}

TEST_CASE("VFS: memory mount basic read") {
    VirtualFilesystem vfs;
    vfs.mount_memory("assets/", {{"assets/test.txt", {'h','e','l','l','o'}}}, 0);

    std::vector<uint8_t> buf;
    auto res = vfs.read("assets/test.txt", buf);
    REQUIRE(res.has_value());
    CHECK(buf.size() == 5);
    CHECK(buf[0] == 'h');
}

TEST_CASE("VFS: missing path returns FileNotFound") {
    VirtualFilesystem vfs;
    vfs.mount_memory("assets/", {}, 0);

    std::vector<uint8_t> buf;
    auto res = vfs.read("assets/nonexistent.txt", buf);
    CHECK(!res.has_value());
    CHECK(res.error().code == AssetErrorCode::FileNotFound);
}

TEST_CASE("VFS: higher-priority mount shadows lower-priority for same vpath") {
    VirtualFilesystem vfs;
    // Priority 0: original file
    vfs.mount_memory("assets/",
        {{"assets/data.bin", {0x01, 0x02, 0x03}}},
        0);
    // Priority 100: override
    vfs.mount_memory("assets/",
        {{"assets/data.bin", {0xAA, 0xBB}}},
        100);

    std::vector<uint8_t> buf;
    auto res = vfs.read("assets/data.bin", buf);
    REQUIRE(res.has_value());
    REQUIRE(buf.size() == 2);
    CHECK(buf[0] == 0xAAu); // high-priority mount wins
}

TEST_CASE("VFS: exists() returns true for mounted files") {
    VirtualFilesystem vfs;
    vfs.mount_memory("assets/", {{"assets/foo.txt", {'x'}}}, 0);
    CHECK(vfs.exists("assets/foo.txt"));
    CHECK(!vfs.exists("assets/bar.txt"));
}

TEST_CASE("VFS: unmount removes access") {
    VirtualFilesystem vfs;
    vfs.mount_memory("data/", {{"data/file.bin", {1}}}, 5);
    CHECK(vfs.exists("data/file.bin"));
    vfs.unmount("data/");
    CHECK(!vfs.exists("data/file.bin"));
}
