#pragma once

#include <string>

namespace gw::jobs {
class Scheduler;
}
namespace gw::events {
class CrossSubsystemBus;
}

namespace gw::persist {

struct PersistConfig {
    std::string save_dir{};
    std::string sqlite_path{};
    std::string cloud_backend{"local"}; // local | steam | eos | s3
    int         slots_max{16};
    int         compress_level{3};
    bool        compress_enabled{false};
    bool        verify_checksum_on_load{true};
    bool        migration_strict{true};
    /// Simulated latency for dev-local cloud (ms); 0 = none.
    float       cloud_sim_latency_ms{0.0f};
};

} // namespace gw::persist
