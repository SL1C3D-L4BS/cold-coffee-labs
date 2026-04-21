#include "mount_point.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace gw::assets::vfs {

// ---------------------------------------------------------------------------
// NativeMountPoint — backs a VFS alias with an OS filesystem directory.
// ---------------------------------------------------------------------------
class NativeMountPoint final : public IMountPoint {
public:
    NativeMountPoint(std::string alias, std::filesystem::path root, int prio)
        : alias_(std::move(alias))
        , root_(std::move(root))
        , priority_(prio) {}

    [[nodiscard]] bool exists(std::string_view vpath) const override {
        return std::filesystem::exists(resolve(vpath));
    }

    [[nodiscard]] AssetOk read(std::string_view vpath,
                                std::vector<uint8_t>& out) override {
        const auto path = resolve(vpath);
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) {
            return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                              path.string().c_str()});
        }
        const auto size = static_cast<std::size_t>(f.tellg());
        f.seekg(0);
        out.resize(size);
        if (!f.read(reinterpret_cast<char*>(out.data()),
                    static_cast<std::streamsize>(size))) {
            return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                              "read failed"});
        }
        return asset_ok();
    }

    AssetOk watch(std::string_view /*vpath*/,
                  std::function<void(std::string_view)> /*cb*/) override {
        // Delegated to FSWatcher (registered separately).
        return asset_ok();
    }

    [[nodiscard]] std::string_view alias()    const noexcept override { return alias_; }
    [[nodiscard]] int              priority() const noexcept override { return priority_; }

private:
    [[nodiscard]] std::filesystem::path resolve(std::string_view vpath) const {
        // Strip the alias prefix before joining with root.
        std::string_view rel = vpath;
        if (rel.starts_with(alias_)) rel = rel.substr(alias_.size());
        return root_ / rel;
    }

    std::string           alias_;
    std::filesystem::path root_;
    int                   priority_;
};

// Factory used by VirtualFilesystem::mount().
std::unique_ptr<IMountPoint> make_native_mount(std::string_view alias,
                                                std::string_view host_path,
                                                int priority) {
    return std::make_unique<NativeMountPoint>(
        std::string(alias),
        std::filesystem::path(host_path),
        priority);
}

// ---------------------------------------------------------------------------
// MemoryMountPoint — backs a VFS alias with a std::unordered_map<string,bytes>.
// ---------------------------------------------------------------------------
class MemoryMountPoint final : public IMountPoint {
public:
    MemoryMountPoint(std::string alias,
                     std::unordered_map<std::string, std::vector<uint8_t>> files,
                     int prio)
        : alias_(std::move(alias))
        , files_(std::move(files))
        , priority_(prio) {}

    [[nodiscard]] bool exists(std::string_view vpath) const override {
        return files_.count(std::string(vpath)) != 0;
    }

    [[nodiscard]] AssetOk read(std::string_view vpath,
                                std::vector<uint8_t>& out) override {
        auto it = files_.find(std::string(vpath));
        if (it == files_.end()) {
            return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                              std::string(vpath).c_str()});
        }
        out = it->second;
        return asset_ok();
    }

    AssetOk watch(std::string_view /*vpath*/,
                  std::function<void(std::string_view)> /*cb*/) override {
        return asset_ok(); // memory mounts never change at runtime
    }

    [[nodiscard]] std::string_view alias()    const noexcept override { return alias_; }
    [[nodiscard]] int              priority() const noexcept override { return priority_; }

private:
    std::string alias_;
    std::unordered_map<std::string, std::vector<uint8_t>> files_;
    int priority_;
};

std::unique_ptr<IMountPoint> make_memory_mount(
    std::string_view alias,
    std::unordered_map<std::string, std::vector<uint8_t>> files,
    int priority)
{
    return std::make_unique<MemoryMountPoint>(
        std::string(alias), std::move(files), priority);
}

} // namespace gw::assets::vfs
