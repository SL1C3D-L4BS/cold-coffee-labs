// engine/persist/persist_types.cpp

#include "engine/persist/persist_types.hpp"

namespace gw::persist {

std::string_view to_string(LoadError e) noexcept {
    switch (e) {
    case LoadError::Ok: return "Ok";
    case LoadError::BadMagic: return "BadMagic";
    case LoadError::TruncatedContainer: return "TruncatedContainer";
    case LoadError::UnsupportedVersion: return "UnsupportedVersion";
    case LoadError::UnsupportedCompression: return "UnsupportedCompression";
    case LoadError::ForwardIncompatible: return "ForwardIncompatible";
    case LoadError::IntegrityMismatch: return "IntegrityMismatch";
    case LoadError::DeterminismMismatch: return "DeterminismMismatch";
    case LoadError::MigrationGap: return "MigrationGap";
    case LoadError::SchemaMismatch: return "SchemaMismatch";
    case LoadError::IoError: return "IoError";
    }
    return "LoadError?";
}

std::string_view to_string(SaveError e) noexcept {
    switch (e) {
    case SaveError::Ok: return "Ok";
    case SaveError::IoError: return "IoError";
    case SaveError::BadPath: return "BadPath";
    case SaveError::SlotMissing: return "SlotMissing";
    case SaveError::CloudError: return "CloudError";
    }
    return "SaveError?";
}

std::uint32_t pack_engine_semver(int major, int minor, int patch) noexcept {
    return (static_cast<std::uint32_t>(major) << 16)
         | (static_cast<std::uint32_t>(minor) << 8)
         | static_cast<std::uint32_t>(patch & 0xFF);
}

} // namespace gw::persist
