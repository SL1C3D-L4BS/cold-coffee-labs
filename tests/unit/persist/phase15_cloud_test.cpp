// Phase 15 — dev-local cloud + conflict policy (ADR-0059).

#include <doctest/doctest.h>

#include "engine/persist/cloud_conflict.hpp"
#include "engine/persist/cloud_save.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace {

std::string temp_cloud_root(std::string_view tag) {
    auto p = std::filesystem::temp_directory_path() / "gw_phase15_cloud" / tag;
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}

} // namespace

TEST_CASE("phase15 — cloud conflict PreferLocal") {
    gw::persist::SlotStamp L{200, 2, 1};
    gw::persist::SlotStamp C{300, 3, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreferLocal)
          == gw::persist::CloudResolveAction::TakeLocal);
}

TEST_CASE("phase15 — cloud conflict PreferCloud") {
    gw::persist::SlotStamp L{200, 2, 1};
    gw::persist::SlotStamp C{300, 3, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreferCloud)
          == gw::persist::CloudResolveAction::TakeCloud);
}

TEST_CASE("phase15 — cloud conflict PreferNewer agrees → TakeCloud") {
    gw::persist::SlotStamp L{100, 1, 1};
    gw::persist::SlotStamp C{200, 2, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreferNewer)
          == gw::persist::CloudResolveAction::TakeCloud);
}

TEST_CASE("phase15 — cloud conflict PreferNewer diagonal ms/clock → Prompt") {
    gw::persist::SlotStamp L{200, 1, 1};
    gw::persist::SlotStamp C{100, 2, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreferNewer)
          == gw::persist::CloudResolveAction::Prompt);
}

TEST_CASE("phase15 — cloud conflict PreferNewer both reverse → TakeLocal") {
    gw::persist::SlotStamp L{300, 3, 1};
    gw::persist::SlotStamp C{200, 2, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreferNewer)
          == gw::persist::CloudResolveAction::TakeLocal);
}

TEST_CASE("phase15 — cloud conflict default Prompt") {
    gw::persist::SlotStamp L{1, 1, 1};
    gw::persist::SlotStamp C{2, 2, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::Prompt)
          == gw::persist::CloudResolveAction::Prompt);
}

TEST_CASE("phase15 — cloud conflict PreserveBoth prompts") {
    gw::persist::SlotStamp L{5, 5, 1};
    gw::persist::SlotStamp C{6, 6, 1};
    CHECK(gw::persist::resolve_conflict(L, C, gw::persist::CloudConflictPolicy::PreserveBoth)
          == gw::persist::CloudResolveAction::Prompt);
}

TEST_CASE("phase15 — dev-local cloud put get list remove") {
    gw::persist::CloudConfig cfg{};
    cfg.root_dir     = temp_cloud_root("putget");
    cfg.sim_latency_ms = 0.0f;
    cfg.user_id_hash = "u1";
    auto cloud = gw::persist::make_dev_local_cloud(cfg);
    REQUIRE(cloud);
    const std::byte data[3] = {std::byte{0xAB}, std::byte{0xCD}, std::byte{0xEF}};
    gw::persist::SlotStamp st{1000, 1, 42};
    CHECK(cloud->put(1, data, st) == gw::persist::CloudError::Ok);
    std::vector<std::byte> out;
    gw::persist::SlotStamp got{};
    CHECK(cloud->get(1, out, got) == gw::persist::CloudError::Ok);
    REQUIRE(out.size() == 3u);
    CHECK(out[0] == std::byte{0xAB});
    std::vector<std::pair<gw::persist::SlotId, gw::persist::SlotStamp>> lst;
    CHECK(cloud->list(lst) == gw::persist::CloudError::Ok);
    CHECK_FALSE(lst.empty());
    CHECK(cloud->remove(1) == gw::persist::CloudError::Ok);
    CHECK(cloud->get(1, out, got) == gw::persist::CloudError::NotFound);
}

TEST_CASE("phase15 — dev-local cloud quota exceeded") {
    gw::persist::CloudConfig cfg{};
    cfg.root_dir      = temp_cloud_root("quota");
    cfg.user_id_hash  = "u2";
    cfg.quota_bytes   = 16;
    cfg.sim_latency_ms = 0.0f;
    auto cloud = gw::persist::make_dev_local_cloud(cfg);
    REQUIRE(cloud);
    std::vector<std::byte> big(32, std::byte{0x55});
    gw::persist::SlotStamp st{1, 1, 1};
    CHECK(cloud->put(9, big, st) == gw::persist::CloudError::QuotaExceeded);
}

TEST_CASE("phase15 — steam eos s3 factories null when gates off") {
    gw::persist::CloudConfig cfg{};
    cfg.root_dir = temp_cloud_root("stubs");
    CHECK(gw::persist::make_cloud_aggregated("steam", cfg) == nullptr);
    CHECK(gw::persist::make_cloud_aggregated("eos", cfg) == nullptr);
    CHECK(gw::persist::make_cloud_aggregated("s3", cfg) == nullptr);
}
