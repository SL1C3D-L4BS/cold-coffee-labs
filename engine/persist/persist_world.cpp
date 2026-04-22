// engine/persist/persist_world.cpp

#include "engine/persist/persist_world.hpp"

#include "engine/core/events/event_bus.hpp"
#include "engine/jobs/scheduler.hpp"

#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/fs_atomic.hpp"
#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/integrity.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/version.generated.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"
#include "engine/platform/fs.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace gw::persist {

namespace {

std::string expand_user_path(std::string s) {
    constexpr std::string_view kTok = "$user";
    const auto pos = s.find(kTok);
    if (pos == std::string::npos) return s;
    std::string base;
#ifdef _WIN32
    if (const char* p = std::getenv("LOCALAPPDATA")) base = p;
    else base = ".";
#else
    if (const char* p = std::getenv("XDG_DATA_HOME")) base = p;
    else if (const char* h = std::getenv("HOME")) base = std::string(h) + "/.local/share";
    else base = ".";
#endif
    base += "/CCL/Greywater";
    s.replace(pos, kTok.size(), base);
    return s;
}

std::string hex_digest_32(const std::array<std::byte, 32>& d) {
    static const char* hx = "0123456789abcdef";
    std::string        out(64, '0');
    for (int i = 0; i < 32; ++i) {
        const auto b = static_cast<unsigned char>(d[i]);
        out[static_cast<std::size_t>(i) * 2]     = hx[b >> 4];
        out[static_cast<std::size_t>(i) * 2 + 1] = hx[b & 0x0F];
    }
    return out;
}

std::filesystem::path slot_path_for(const PersistConfig& cfg, SlotId id) {
    return std::filesystem::path{expand_user_path(cfg.save_dir)}
         / (std::string{"slot_"} + std::to_string(id) + ".gwsave");
}

} // namespace

struct PersistWorld::Impl {
    PersistConfig                     cfg{};
    config::CVarRegistry*             cvars{nullptr};
    gw::events::CrossSubsystemBus*   bus{nullptr};
    gw::jobs::Scheduler*              sched{nullptr};
    std::unique_ptr<ILocalStore>    store{};
    std::unique_ptr<ICloudSave>     cloud{};
    bool                            inited{false};
    double                          autosave_accum{0};
};

PersistWorld::PersistWorld() : impl_(std::make_unique<Impl>()) {}
PersistWorld::~PersistWorld() { shutdown(); }

bool PersistWorld::initialize(PersistConfig cfg,
                              config::CVarRegistry* cvars,
                              events::CrossSubsystemBus* bus,
                              jobs::Scheduler*           scheduler) {
    shutdown();
    impl_->cfg    = std::move(cfg);
    impl_->cvars  = cvars;
    impl_->bus    = bus;
    impl_->sched  = scheduler;
    impl_->cfg.save_dir = expand_user_path(impl_->cfg.save_dir);
    if (impl_->cfg.sqlite_path.empty()) {
        impl_->cfg.sqlite_path = (std::filesystem::path{impl_->cfg.save_dir} / "greywater_persist.db").string();
    }
    std::filesystem::create_directories(impl_->cfg.save_dir);

    impl_->store = make_sqlite_local_store(impl_->cfg.sqlite_path);
    if (!impl_->store->open_or_create()) return false;

    CloudConfig cc{};
    cc.root_dir          = (std::filesystem::path{impl_->cfg.save_dir} / "cloud_local").string();
    cc.sim_latency_ms    = impl_->cfg.cloud_sim_latency_ms;
    cc.user_id_hash      = "sandbox";
    impl_->cloud         = make_cloud_aggregated(impl_->cfg.cloud_backend, cc);
    impl_->inited        = true;
    return true;
}

void PersistWorld::shutdown() {
    if (impl_->store) impl_->store->shutdown();
    impl_->store.reset();
    impl_->cloud.reset();
    impl_->inited = false;
}

bool PersistWorld::initialized() const noexcept { return impl_->inited; }

SaveError PersistWorld::save_slot(SlotId id, ecs::World& w, const SaveMeta& meta) {
    if (!impl_->inited) return SaveError::IoError;

    auto chunks = build_chunk_grid_demo(w);
    const auto centre = centre_chunk_payload(chunks);
    const auto det    = ecs_blob_determinism_hash(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(centre.data()), centre.size()));

    gwsave::HeaderPrefix hdr{};
    hdr.flags            = 0;
    hdr.schema_version   = meta.schema_version;
    hdr.engine_version =
        pack_engine_semver(GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH);
    hdr.world_seed       = meta.world_seed;
    hdr.session_seed     = meta.session_seed;
    hdr.created_unix_ms  = meta.created_unix_ms;
    hdr.anchor_x         = meta.anchor_pos.x;
    hdr.anchor_y         = meta.anchor_pos.y;
    hdr.anchor_z         = meta.anchor_pos.z;
    hdr.determinism_hash = det;
    hdr.chunk_count      = static_cast<std::uint32_t>(chunks.size());

    std::uint16_t wflags = hdr.flags;
#if GW_ENABLE_ZSTD
    if (impl_->cvars && impl_->cvars->get_bool_or("persist.compress.enabled", false)) {
        wflags = static_cast<std::uint16_t>(wflags | gwsave::kFlagZstd);
    }
#endif
    auto file_bytes = write_gwsave_container(wflags, hdr, chunks);

    const auto path = slot_path_for(impl_->cfg, id);
    if (!atomic_write_bytes(path, file_bytes)) return SaveError::IoError;

    const auto digest = blake3_digest_256(std::span<const std::byte>(file_bytes.data(), file_bytes.size()));
    SlotRow row{};
    row.slot_id       = id;
    row.display_name  = meta.display_name;
    row.unix_ms       = static_cast<std::int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    row.engine_ver    = static_cast<std::int32_t>(hdr.engine_version);
    row.schema_ver    = static_cast<std::int32_t>(hdr.schema_version);
    row.determinism_h = static_cast<std::int64_t>(hdr.determinism_hash);
    row.path          = path.string();
    row.size_bytes    = static_cast<std::int64_t>(file_bytes.size());
    row.blake3_hex    = hex_digest_32(digest);
    if (!impl_->store->upsert_slot(row)) return SaveError::IoError;

    if (impl_->bus) {
        // CrossSubsystemBus publish would need adapter types — omitted minimal.
    }
    return SaveError::Ok;
}

LoadError PersistWorld::load_slot(SlotId id, ecs::World& w) {
    if (!impl_->inited) return LoadError::IoError;
    const auto path = slot_path_for(impl_->cfg, id);
    const auto bytes_opt = gw::platform::FileSystem::read_bytes(path);
    if (!bytes_opt) return LoadError::IoError;

    std::vector<std::byte> file;
    file.resize(bytes_opt->size());
    std::memcpy(file.data(), bytes_opt->data(), bytes_opt->size());

    const bool verify = impl_->cvars ? impl_->cvars->get_bool_or("persist.checksum.verify_on_load", true) : true;
    const auto peek   = peek_gwsave(std::span<const std::byte>(file.data(), file.size()), verify);
    if (peek.err != LoadError::Ok) return peek.err;

    const bool strict = impl_->cvars ? impl_->cvars->get_bool_or("persist.migration.strict", true) : true;
    const std::uint32_t run_ver =
        pack_engine_semver(GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH);
    if (peek.hdr.engine_version > run_ver && strict) return LoadError::ForwardIncompatible;

    std::vector<gwsave::ChunkPayload> loaded;
    const LoadError lc = load_gwsave_all_chunks(std::span<const std::byte>(file.data(), file.size()), peek, loaded);
    if (lc != LoadError::Ok) return lc;

    const auto centre = centre_chunk_payload(loaded);
    if (centre.size() < sizeof(std::uint32_t)) return LoadError::TruncatedContainer;

    std::vector<std::uint8_t> blob(centre.size());
    std::memcpy(blob.data(), centre.data(), centre.size());

    gw::ecs::LoadOptions opts{};
    const auto lr = gw::ecs::load_world(w, blob, opts);
    if (!lr) return LoadError::SchemaMismatch;

    const auto det = ecs_blob_determinism_hash(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(centre.data()), centre.size()));
    if (det != peek.hdr.determinism_hash) return LoadError::DeterminismMismatch;

    return LoadError::Ok;
}

LoadError PersistWorld::validate_slot(SlotId id, ecs::World& w) {
    return load_slot(id, w);
}

SaveError PersistWorld::migrate_slot(SlotId id) {
    if (!impl_->inited) return SaveError::IoError;
    const auto path = slot_path_for(impl_->cfg, id);
    const auto bytes_opt = gw::platform::FileSystem::read_bytes(path);
    if (!bytes_opt) return SaveError::IoError;

    std::vector<std::byte> file(bytes_opt->size());
    std::memcpy(file.data(), bytes_opt->data(), bytes_opt->size());

    auto peek = peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    if (peek.err != LoadError::Ok) return SaveError::IoError;

    if (peek.hdr.schema_version >= kCurrentSaveSchemaVersion) return SaveError::Ok;

    std::vector<gwsave::ChunkPayload> loaded;
    if (load_gwsave_all_chunks(std::span<const std::byte>(file.data(), file.size()), peek, loaded) != LoadError::Ok) {
        return SaveError::IoError;
    }

    gwsave::HeaderPrefix hdr = peek.hdr;
    hdr.schema_version       = kCurrentSaveSchemaVersion;
    hdr.engine_version =
        pack_engine_semver(GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH);

    auto out = write_gwsave_container(0, hdr, loaded);
    if (!atomic_write_bytes(path, out)) return SaveError::IoError;

    if (impl_->store) {
        const auto row = impl_->store->get_slot(id);
        if (row) {
            SlotRow u = *row;
            u.schema_ver = static_cast<std::int32_t>(hdr.schema_version);
            u.engine_ver = static_cast<std::int32_t>(hdr.engine_version);
            u.size_bytes = static_cast<std::int64_t>(out.size());
            u.blake3_hex = hex_digest_32(blake3_digest_256(std::span<const std::byte>(out.data(), out.size())));
            (void)impl_->store->upsert_slot(u);
        }
    }
    return SaveError::Ok;
}

void PersistWorld::step(double dt_seconds) {
    if (!impl_->inited) return;
    (void)dt_seconds;
    // Autosave dispatch via jobs lands in Phase 15 follow-up; gate keeps main-thread budget.
}

ILocalStore* PersistWorld::local_store() noexcept { return impl_->store.get(); }

ICloudSave* PersistWorld::cloud() noexcept { return impl_->cloud.get(); }

const PersistConfig& PersistWorld::config() const noexcept { return impl_->cfg; }

} // namespace gw::persist
