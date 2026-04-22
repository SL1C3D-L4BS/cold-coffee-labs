// engine/persist/cloud_save.cpp — dev-local cloud (ADR-0059).

#include "engine/persist/cloud_save.hpp"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

namespace gw::persist {

namespace {

class DevLocalCloud final : public ICloudSave {
public:
    explicit DevLocalCloud(CloudConfig cfg) : cfg_(std::move(cfg)) {
        root_ = std::filesystem::path{cfg_.root_dir} / cfg_.user_id_hash;
        std::filesystem::create_directories(root_);
        used_ = scan_used_();
    }

    CloudError put(SlotId slot, std::span<const std::byte> bytes, SlotStamp stamp) override {
        if (cfg_.sim_latency_ms > 0.0f) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(cfg_.sim_latency_ms)));
        }
        const auto q = quota();
        if (q.used_bytes + bytes.size() > q.quota_bytes) return CloudError::QuotaExceeded;

        const auto path = root_ / (std::string{"slot_"} + std::to_string(slot) + ".bin");
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) return CloudError::IoError;
        f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        // Sidecar stamp
        const auto sp = root_ / (std::string{"slot_"} + std::to_string(slot) + ".stamp");
        std::ofstream fs(sp, std::ios::binary | std::ios::trunc);
        fs.write(reinterpret_cast<const char*>(&stamp), sizeof(stamp));
        used_ = scan_used_();
        return CloudError::Ok;
    }

    CloudError get(SlotId slot, std::vector<std::byte>& out, SlotStamp& out_stamp) override {
        if (cfg_.sim_latency_ms > 0.0f) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(cfg_.sim_latency_ms)));
        }
        const auto path = root_ / (std::string{"slot_"} + std::to_string(slot) + ".bin");
        std::ifstream f(path, std::ios::binary);
        if (!f) return CloudError::NotFound;
        f.seekg(0, std::ios::end);
        const auto sz = f.tellg();
        f.seekg(0);
        out.resize(static_cast<std::size_t>(sz));
        f.read(reinterpret_cast<char*>(out.data()), sz);

        const auto sp = root_ / (std::string{"slot_"} + std::to_string(slot) + ".stamp");
        std::ifstream fs(sp, std::ios::binary);
        if (fs && fs.read(reinterpret_cast<char*>(&out_stamp), sizeof(out_stamp))) {
            // ok
        } else {
            out_stamp = {};
        }
        return CloudError::Ok;
    }

    CloudError list(std::vector<std::pair<SlotId, SlotStamp>>& out) override {
        out.clear();
        if (!std::filesystem::exists(root_)) return CloudError::Ok;
        for (const auto& e : std::filesystem::directory_iterator(root_)) {
            if (!e.is_regular_file()) continue;
            const auto name = e.path().filename().string();
            if (name.rfind("slot_", 0) != 0 || name.find(".bin") == std::string::npos) continue;
            const auto id_str = name.substr(5, name.size() - 5 - 4);
            try {
                const SlotId id = static_cast<SlotId>(std::stoi(id_str));
                SlotStamp st{};
                const auto sp = e.path().parent_path() / (std::string{"slot_"} + id_str + ".stamp");
                std::ifstream fs(sp, std::ios::binary);
                (void)fs.read(reinterpret_cast<char*>(&st), sizeof(st));
                out.emplace_back(id, st);
            } catch (...) {
            }
        }
        return CloudError::Ok;
    }

    CloudError remove(SlotId slot) override {
        std::error_code ec;
        std::filesystem::remove(root_ / (std::string{"slot_"} + std::to_string(slot) + ".bin"), ec);
        std::filesystem::remove(root_ / (std::string{"slot_"} + std::to_string(slot) + ".stamp"), ec);
        used_ = scan_used_();
        return CloudError::Ok;
    }

    [[nodiscard]] CloudQuota quota() const noexcept override {
        CloudQuota q{};
        q.quota_bytes = cfg_.quota_bytes;
        q.used_bytes  = used_;
        return q;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override { return "local"; }

private:
    std::uint64_t scan_used_() const {
        std::uint64_t n = 0;
        if (!std::filesystem::exists(root_)) return 0;
        for (const auto& e : std::filesystem::recursive_directory_iterator(root_)) {
            if (e.is_regular_file()) n += static_cast<std::uint64_t>(e.file_size());
        }
        return n;
    }

    CloudConfig               cfg_;
    std::filesystem::path     root_;
    std::uint64_t             used_{0};
};

} // namespace

std::unique_ptr<ICloudSave> make_dev_local_cloud(const CloudConfig& cfg) {
    return std::make_unique<DevLocalCloud>(cfg);
}

std::unique_ptr<ICloudSave> make_cloud_steam_backend(const CloudConfig& cfg);
std::unique_ptr<ICloudSave> make_cloud_eos_backend(const CloudConfig& cfg);
std::unique_ptr<ICloudSave> make_cloud_s3_backend(const CloudConfig& cfg);

std::unique_ptr<ICloudSave> make_cloud_aggregated(std::string_view backend, const CloudConfig& cfg) {
    if (backend == "local") return make_dev_local_cloud(cfg);
    if (backend == "steam") return make_cloud_steam_backend(cfg);
    if (backend == "eos") return make_cloud_eos_backend(cfg);
    if (backend == "s3") return make_cloud_s3_backend(cfg);
    return nullptr;
}

} // namespace gw::persist
