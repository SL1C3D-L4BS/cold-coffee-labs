#pragma once

#include <cstdint>
#include <string_view>

namespace gw::persist {

struct SaveStarted {
    std::int32_t slot_id{0};
};

struct SaveCompleted {
    std::int32_t slot_id{0};
    bool         ok{false};
};

struct LoadStarted {
    std::int32_t slot_id{0};
};

struct LoadCompleted {
    std::int32_t slot_id{0};
    bool         ok{false};
};

struct MigrationRan {
    std::uint32_t from_schema{0};
    std::uint32_t to_schema{0};
};

struct IntegrityFailed {
    std::string_view reason{};
};

struct QuotaWarning {
    std::uint64_t used_bytes{0};
    std::uint64_t quota_bytes{0};
};

} // namespace gw::persist
