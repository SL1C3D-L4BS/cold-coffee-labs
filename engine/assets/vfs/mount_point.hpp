#pragma once
// engine/assets/vfs/mount_point.hpp
// IMountPoint — pure interface for all VFS backing stores.
// Phase 6 spec §8.3.

#include "../asset_error.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gw::assets::vfs {

class IMountPoint {
public:
    virtual ~IMountPoint() = default;

    // Returns true if the virtual path exists in this mount.
    [[nodiscard]] virtual bool exists(std::string_view vpath) const = 0;

    // Read the full contents of vpath into out.  Returns an AssetError on
    // failure.  Thread-safe (implementations must lock internally).
    [[nodiscard]] virtual AssetOk
    read(std::string_view vpath, std::vector<uint8_t>& out) = 0;

    // Register a change-notification callback for vpath.  The callback
    // receives the changed vpath when the underlying file changes.
    // Not all mount types support watching; those return asset_ok() and ignore
    // the callback.
    virtual AssetOk watch(std::string_view vpath,
                          std::function<void(std::string_view)> cb) = 0;

    // The prefix this mount is registered under (e.g. "assets/").
    [[nodiscard]] virtual std::string_view alias()    const noexcept = 0;
    // Higher priority wins on vpath conflict.
    [[nodiscard]] virtual int              priority() const noexcept = 0;
};

} // namespace gw::assets::vfs
