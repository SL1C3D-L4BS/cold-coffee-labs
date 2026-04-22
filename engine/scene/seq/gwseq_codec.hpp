#pragma once
// engine/scene/seq/gwseq_codec.hpp
// Binary codec for `.gwseq` — Phase 18-A.

#include "engine/core/result.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "engine/scene/seq/gwseq_format.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace gw::seq {

enum class SeqError : std::uint8_t {
    Ok = 0,
    IoOpenFailed,
    IoReadFailed,
    IoWriteFailed,
    BadMagic,
    UnsupportedVersion,
    Truncated,
    BadTrackId,
    BadTrackPayload,
    TypeMismatch,
    WriterNotFinalised,
    WriterAlreadyFinalised,
    MigrationFailed,
};

/// Success token for `gw::core::Result` (variant cannot hold `void`).
using SeqOk = std::monostate;

using SeqResult = gw::core::Result<SeqOk, SeqError>;

class GwseqReader;

/// Migrates keyframe bytes in-place when possible; otherwise allocates a new
/// buffer from `arena` and returns bytes there. Runs when the on-disk
/// `GwseqHeader::version` is below `kGwseqVersion` before views are handed out.
using GwseqTrackMigrator = std::function<SeqResult(
    GwseqReader& self, GwseqTrackType type, std::span<std::uint8_t> keyframe_bytes,
    gw::memory::ArenaAllocator& arena)>;

/// Random-access view into a single track's keyframes (borrowed bytes).
template <typename T>
struct GwseqTrackView {
    std::span<const GwseqKeyframe<T>> frames{};

    /// Returns true when the view carries at least one keyframe.
    [[nodiscard]] bool empty() const noexcept { return frames.empty(); }
};

/// Selects which payload layout is used for `GwseqTrackType::Transform` tracks.
enum class GwseqTransformPayloadKind : std::uint8_t {
    PositionVec3 = 0,
    RotationQuat = 1,
};

// ---------------------------------------------------------------------------
// GwseqWriter — builds a `.gwseq` blob in a caller-owned byte vector.
// ---------------------------------------------------------------------------
class GwseqWriter {
public:
    /// Wraps `out`; the vector must outlive `finalise()`.
    explicit GwseqWriter(std::vector<std::uint8_t>& out) noexcept;

    GwseqWriter(const GwseqWriter&)            = delete;
    GwseqWriter& operator=(const GwseqWriter&) = delete;

    /// Writes the header skeleton (track_count patched in `finalise()`).
    [[nodiscard]] SeqResult write_header(std::uint32_t frame_rate,
                                         std::uint32_t duration_frames);

    /// Opens a new track; subsequent `write_keyframe` calls append here.
    [[nodiscard]] SeqResult begin_track(std::uint32_t track_id, GwseqTrackType type,
                                        GwseqTransformPayloadKind transform_payload =
                                            GwseqTransformPayloadKind::PositionVec3);

    /// Appends one keyframe to the active track opened by `begin_track`.
    template <typename T>
    [[nodiscard]] SeqResult write_keyframe(const GwseqKeyframe<T>& kf);

    /// Seals the file: patches header track_count and validates layout.
    [[nodiscard]] SeqResult finalise();

private:
    struct PendingTrack {
        GwseqTrack                 meta{};
        GwseqTrackType             logical_type = GwseqTrackType::Transform;
        bool                       is_vec3      = true;
        std::vector<std::uint8_t>  bytes{};
    };

    [[nodiscard]] SeqResult ensure_writable_track();

    std::vector<std::uint8_t>* out_{nullptr};
    GwseqHeader                 header_{};
    bool                        header_written_ = false;
    bool                        finalised_      = false;
    std::vector<PendingTrack>   tracks_{};
    std::optional<std::size_t>   open_track_index_{};
};

// ---------------------------------------------------------------------------
// GwseqReader — reader over an owned byte buffer with optional per-track
// migrators executed before returning a typed view.
// ---------------------------------------------------------------------------
class GwseqReader {
public:
    GwseqReader() noexcept = default;

    GwseqReader(const GwseqReader&)            = delete;
    GwseqReader& operator=(const GwseqReader&) = delete;

    GwseqReader(GwseqReader&&) noexcept            = default;
    GwseqReader& operator=(GwseqReader&&) noexcept = default;

    /// Returns a mutable view over the loaded file bytes (migrators may patch
    /// the `GwseqHeader` and keyframe payloads in place).
    [[nodiscard]] std::span<std::uint8_t> mutable_owned_bytes() noexcept {
        return {owned_.data(), owned_.size()};
    }

    /// Reloads the cached `GwseqHeader` from the start of `owned_`.
    void refresh_header() noexcept;

    /// Registers a migrator invoked when `header().version < kGwseqVersion`.
    void add_migrator(GwseqTrackMigrator fn);

    /// Reads an entire file into an internal buffer (cold path allocation).
    [[nodiscard]] SeqResult open(const std::filesystem::path& path);

    /// Parses an in-memory blob.
    [[nodiscard]] SeqResult open_bytes(std::vector<std::uint8_t> bytes);

    /// Header view after a successful `open`.
    [[nodiscard]] const GwseqHeader& header() const noexcept { return header_; }

    /// Parsed track table (same order as on disk). Empty before `open`.
    [[nodiscard]] std::span<const GwseqTrack> tracks() const noexcept {
        return {track_table_.data(), track_table_.size()};
    }

    /// Returns a typed span for `track_id`, running migrators when needed.
    /// The returned `frames` pointer may reference `arena` memory; do not reset
    /// `arena` until the span is discarded.
    template <typename T>
    [[nodiscard]] SeqResult read_track(std::uint32_t track_id,
                                       gw::memory::ArenaAllocator& arena,
                                       GwseqTrackView<T>&           out_view);

private:
    [[nodiscard]] const GwseqTrack* find_track_(std::uint32_t track_id) const noexcept;

    [[nodiscard]] SeqResult migrate_if_needed_(const GwseqTrack& tr,
                                               gw::memory::ArenaAllocator& arena,
                                               std::span<std::uint8_t>&     work);

    GwseqHeader                     header_{};
    std::vector<GwseqTrack>         track_table_{};
    std::vector<std::uint8_t>     owned_{};
    std::vector<GwseqTrackMigrator> migrators_{};
};

// ---------------------------------------------------------------------------
template <typename T>
SeqResult GwseqWriter::write_keyframe(const GwseqKeyframe<T>& kf) {
    const auto r = ensure_writable_track();
    if (!r.has_value()) return r;

    auto& tr = tracks_[*open_track_index_];
    if (tr.logical_type == GwseqTrackType::Transform) {
        if constexpr (std::is_same_v<T, Vec3_f64>) {
            if (!tr.is_vec3) return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        } else if constexpr (std::is_same_v<T, GwseqQuat>) {
            if (tr.is_vec3) return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        } else {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr.logical_type == GwseqTrackType::Float) {
        if constexpr (!std::is_same_v<T, float>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr.logical_type == GwseqTrackType::Bool ||
               tr.logical_type == GwseqTrackType::Event) {
        if constexpr (!std::is_same_v<T, bool>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr.logical_type == GwseqTrackType::EntityRef) {
        if constexpr (!std::is_same_v<T, EntityHandle>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else {
        return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
    }

    const auto* b = reinterpret_cast<const std::uint8_t*>(&kf);
    tr.bytes.insert(tr.bytes.end(), b, b + sizeof(GwseqKeyframe<T>));
    ++tr.meta.keyframe_count;
    return SeqOk{};
}

template <typename T>
SeqResult GwseqReader::read_track(std::uint32_t track_id,
                                  gw::memory::ArenaAllocator& arena,
                                  GwseqTrackView<T>&           out_view) {
    const GwseqTrack* tr = find_track_(track_id);
    if (!tr) return SeqResult{gw::core::unexpected(SeqError::BadTrackId)};

    if (tr->track_type == GwseqTrackType::Transform) {
        if constexpr (!(std::is_same_v<T, Vec3_f64> || std::is_same_v<T, GwseqQuat>)) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr->track_type == GwseqTrackType::Float) {
        if constexpr (!std::is_same_v<T, float>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr->track_type == GwseqTrackType::Bool || tr->track_type == GwseqTrackType::Event) {
        if constexpr (!std::is_same_v<T, bool>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else if (tr->track_type == GwseqTrackType::EntityRef) {
        if constexpr (!std::is_same_v<T, EntityHandle>) {
            return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
        }
    } else {
        return SeqResult{gw::core::unexpected(SeqError::TypeMismatch)};
    }

    const std::size_t base_off = static_cast<std::size_t>(tr->keyframe_offset);
    if (base_off > owned_.size()) {
        return SeqResult{gw::core::unexpected(SeqError::Truncated)};
    }

    std::uint64_t next_off_u64 = static_cast<std::uint64_t>(owned_.size());
    for (const auto& t : track_table_) {
        if (t.keyframe_offset > tr->keyframe_offset && t.keyframe_offset < next_off_u64) {
            next_off_u64 = t.keyframe_offset;
        }
    }
    const std::size_t region_end = static_cast<std::size_t>(next_off_u64);
    if (region_end < base_off) {
        return SeqResult{gw::core::unexpected(SeqError::Truncated)};
    }

    const std::size_t byte_len = region_end - base_off;
    if (tr->keyframe_count == 0) {
        out_view.frames = {};
        return SeqOk{};
    }

    const std::size_t expect =
        static_cast<std::size_t>(tr->keyframe_count) * sizeof(GwseqKeyframe<T>);
    if (byte_len < expect) {
        return SeqResult{gw::core::unexpected(SeqError::BadTrackPayload)};
    }

    std::span<std::uint8_t> work{owned_.data() + base_off, expect};

    if (const SeqResult m = migrate_if_needed_(*tr, arena, work); !m.has_value()) {
        return m;
    }

    if (work.size_bytes() != expect) {
        return SeqResult{gw::core::unexpected(SeqError::BadTrackPayload)};
    }

    out_view.frames = std::span<const GwseqKeyframe<T>>(
        reinterpret_cast<const GwseqKeyframe<T>*>(work.data()),
        tr->keyframe_count);
    return SeqOk{};
}

}  // namespace gw::seq
