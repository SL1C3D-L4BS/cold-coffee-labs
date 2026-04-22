// engine/persist/impl/cloud_stubs.cpp — Steam / EOS / S3 factories (Phase 16 hooks).

#include "engine/persist/cloud_save.hpp"

namespace gw::persist {

std::unique_ptr<ICloudSave> make_cloud_steam_backend(const CloudConfig&) {
#if GW_ENABLE_STEAMWORKS
    // Phase 16: ISteamRemoteStorage wrapper in cloud_steam.cpp
#endif
    return nullptr;
}

std::unique_ptr<ICloudSave> make_cloud_eos_backend(const CloudConfig&) {
#if GW_ENABLE_EOS
    // Phase 16: EOS_PlayerDataStorage
#endif
    return nullptr;
}

std::unique_ptr<ICloudSave> make_cloud_s3_backend(const CloudConfig&) {
#if GW_ENABLE_CPR
    // Phase 16: SigV4 PUT via cpr
#endif
    return nullptr;
}

} // namespace gw::persist
