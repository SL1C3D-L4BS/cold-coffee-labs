#pragma once

namespace gw::telemetry {

struct TelemetryFlushed {
    std::uint32_t batches{0};
};

struct CrashBufferedOffline {};

struct ConsentChanged {
    int tier{0};
};

struct TelemetryCompiledOut {};

} // namespace gw::telemetry
