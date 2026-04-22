// engine/scene/seq/gwseq_codec.cpp

#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstring>
#include <fstream>
#include <ios>
namespace gw::seq {

namespace {

[[nodiscard]] std::size_t align_up(std::size_t v, std::size_t a) noexcept {
    return (v + a - 1u) & ~(a - 1u);
}

}  // namespace

GwseqWriter::GwseqWriter(std::vector<std::uint8_t>& out) noexcept : out_(&out) {}

SeqResult GwseqWriter::write_header(const std::uint32_t frame_rate,
                                    const std::uint32_t duration_frames) {
    if (finalised_) return SeqResult{gw::core::unexpected(SeqError::WriterAlreadyFinalised)};
    header_.magic           = kGwseqMagic;
    header_.version         = kGwseqVersion;
    header_.track_count     = 0;
    header_.frame_rate      = frame_rate;
    header_.duration_frames = duration_frames;
    header_written_          = true;
    return SeqOk{};
}

SeqResult GwseqWriter::begin_track(const std::uint32_t track_id, const GwseqTrackType type,
                                   const GwseqTransformPayloadKind transform_payload) {
    if (!header_written_) return SeqResult{gw::core::unexpected(SeqError::WriterNotFinalised)};
    if (finalised_) return SeqResult{gw::core::unexpected(SeqError::WriterAlreadyFinalised)};

    for (const auto& t : tracks_) {
        if (t.meta.track_id == track_id) {
            return SeqResult{gw::core::unexpected(SeqError::BadTrackId)};
        }
    }

    PendingTrack pt{};
    pt.meta.track_id       = track_id;
    pt.meta.track_type     = type;
    pt.meta.keyframe_offset = 0;
    pt.meta.keyframe_count  = 0;
    pt.logical_type         = type;
    if (type == GwseqTrackType::Transform) {
        pt.is_vec3 = (transform_payload == GwseqTransformPayloadKind::PositionVec3);
    } else {
        pt.is_vec3 = true;
    }
    tracks_.push_back(std::move(pt));
    open_track_index_ = tracks_.size() - 1u;
    return SeqOk{};
}

SeqResult GwseqWriter::ensure_writable_track() {
    if (!header_written_) return SeqResult{gw::core::unexpected(SeqError::WriterNotFinalised)};
    if (finalised_) return SeqResult{gw::core::unexpected(SeqError::WriterAlreadyFinalised)};
    if (!open_track_index_.has_value()) {
        return SeqResult{gw::core::unexpected(SeqError::WriterNotFinalised)};
    }
    return SeqOk{};
}

SeqResult GwseqWriter::finalise() {
    if (!header_written_) return SeqResult{gw::core::unexpected(SeqError::WriterNotFinalised)};
    if (finalised_) return SeqResult{gw::core::unexpected(SeqError::WriterAlreadyFinalised)};

    open_track_index_.reset();

    out_->clear();

    header_.track_count = static_cast<std::uint32_t>(tracks_.size());

    const std::size_t header_sz = sizeof(GwseqHeader);
    const std::size_t table_sz  = sizeof(GwseqTrack) * tracks_.size();
    const std::size_t table_end = header_sz + table_sz;
    const std::size_t key_start = align_up(table_end, alignof(std::max_align_t));
    const std::size_t padding   = key_start - table_end;

    out_->resize(key_start);
    std::memcpy(out_->data(), &header_, sizeof(GwseqHeader));
    if (padding > 0u) {
        std::memset(out_->data() + table_end, 0, padding);
    }

    std::size_t cursor = key_start;
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        auto& t = tracks_[i];
        const std::size_t elem =
            (t.logical_type == GwseqTrackType::Transform)
                ? (t.is_vec3 ? sizeof(GwseqKeyframe<Vec3_f64>) : sizeof(GwseqKeyframe<GwseqQuat>))
                : (t.logical_type == GwseqTrackType::Float
                       ? sizeof(GwseqKeyframe<float>)
                       : (t.logical_type == GwseqTrackType::EntityRef
                              ? sizeof(GwseqKeyframe<EntityHandle>)
                              : sizeof(GwseqKeyframe<bool>)));
        if (elem == 0u) return SeqResult{gw::core::unexpected(SeqError::BadTrackPayload)};
        if (t.bytes.size() != static_cast<std::size_t>(t.meta.keyframe_count) * elem) {
            return SeqResult{gw::core::unexpected(SeqError::BadTrackPayload)};
        }

        t.meta.keyframe_offset = static_cast<std::uint64_t>(cursor);
        std::memcpy(out_->data() + sizeof(GwseqHeader) + i * sizeof(GwseqTrack), &t.meta,
                    sizeof(GwseqTrack));

        out_->insert(out_->end(), t.bytes.begin(), t.bytes.end());
        cursor += t.bytes.size();
    }

    std::memcpy(out_->data(), &header_, sizeof(GwseqHeader));

    finalised_ = true;
    tracks_.clear();
    return SeqOk{};
}

void GwseqReader::add_migrator(GwseqTrackMigrator fn) { migrators_.push_back(std::move(fn)); }

SeqResult GwseqReader::open(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return SeqResult{gw::core::unexpected(SeqError::IoOpenFailed)};
    const auto sz = static_cast<std::streamsize>(f.tellg());
    if (sz < 0) return SeqResult{gw::core::unexpected(SeqError::IoReadFailed)};
    f.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(sz));
    if (!f.read(reinterpret_cast<char*>(buf.data()), sz)) {
        return SeqResult{gw::core::unexpected(SeqError::IoReadFailed)};
    }
    return open_bytes(std::move(buf));
}

SeqResult GwseqReader::open_bytes(std::vector<std::uint8_t> bytes) {
    owned_       = std::move(bytes);
    track_table_.clear();

    if (owned_.size() < sizeof(GwseqHeader)) {
        return SeqResult{gw::core::unexpected(SeqError::Truncated)};
    }
    std::memcpy(&header_, owned_.data(), sizeof(GwseqHeader));
    if (header_.magic != kGwseqMagic) return SeqResult{gw::core::unexpected(SeqError::BadMagic)};
    if (header_.version > kGwseqVersion) {
        return SeqResult{gw::core::unexpected(SeqError::UnsupportedVersion)};
    }

    const std::size_t need =
        sizeof(GwseqHeader) + static_cast<std::size_t>(header_.track_count) * sizeof(GwseqTrack);
    if (owned_.size() < need) return SeqResult{gw::core::unexpected(SeqError::Truncated)};

    track_table_.reserve(header_.track_count);
    const auto* base = reinterpret_cast<const GwseqTrack*>(owned_.data() + sizeof(GwseqHeader));
    for (std::uint32_t i = 0; i < header_.track_count; ++i) {
        track_table_.push_back(base[i]);
    }
    return SeqOk{};
}

const GwseqTrack* GwseqReader::find_track_(const std::uint32_t track_id) const noexcept {
    for (const auto& t : track_table_) {
        if (t.track_id == track_id) return &t;
    }
    return nullptr;
}

void GwseqReader::refresh_header() noexcept {
    if (owned_.size() >= sizeof(GwseqHeader)) {
        std::memcpy(&header_, owned_.data(), sizeof(GwseqHeader));
    }
}

SeqResult GwseqReader::migrate_if_needed_(const GwseqTrack& tr,
                                          gw::memory::ArenaAllocator& arena,
                                          std::span<std::uint8_t>&     work) {
    if (header_.version >= kGwseqVersion) return SeqOk{};

    for (const auto& mig : migrators_) {
        const SeqResult r = mig(*this, tr.track_type, work, arena);
        if (!r.has_value()) return r;
        refresh_header();
        if (header_.version >= kGwseqVersion) {
            break;
        }
    }
    return SeqOk{};
}

}  // namespace gw::seq
