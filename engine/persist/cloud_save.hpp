#pragma once

#include "engine/persist/persist_types.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::persist {

struct SlotStamp {
    std::uint64_t unix_ms{0};
    std::uint32_t vector_clock{0};
    std::uint32_t machine_salt{0};
};

enum class CloudConflictPolicy : std::uint8_t {
    Prompt = 0,
    PreferLocal,
    PreferCloud,
    PreferNewer,
    PreserveBoth,
};

enum class CloudError : std::uint8_t {
    Ok = 0,
    QuotaExceeded,
    NotFound,
    BackendDisabled,
    IoError,
    Conflict,
};

struct CloudQuota {
    std::uint64_t quota_bytes{1ull << 30};
    std::uint64_t used_bytes{0};
};

class ICloudSave {
public:
    virtual ~ICloudSave() = default;

    virtual CloudError put(SlotId slot, std::span<const std::byte> bytes, SlotStamp stamp) = 0;
    virtual CloudError get(SlotId slot, std::vector<std::byte>& out, SlotStamp& out_stamp) = 0;
    virtual CloudError list(std::vector<std::pair<SlotId, SlotStamp>>& out) = 0;
    virtual CloudError remove(SlotId slot) = 0;

    [[nodiscard]] virtual CloudQuota quota() const noexcept = 0;
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

struct CloudConfig {
    std::string   root_dir{};
    float         sim_latency_ms{0};
    std::string   user_id_hash{"local"}; // subdir for dev-local
    std::uint64_t quota_bytes{1ull << 30};
};

[[nodiscard]] std::unique_ptr<ICloudSave> make_dev_local_cloud(const CloudConfig& cfg);

[[nodiscard]] std::unique_ptr<ICloudSave> make_cloud_aggregated(std::string_view backend,
                                                                const CloudConfig& cfg);

} // namespace gw::persist
