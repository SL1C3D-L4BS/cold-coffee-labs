// editor/seq/seq_gwseq_rebuild.cpp

#include "editor/seq/seq_gwseq_rebuild.hpp"

#include "engine/core/result.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "engine/scene/seq/gwseq_format.hpp"

namespace gw::editor::seq {
namespace {

using gw::memory::ArenaAllocator;
using gw::seq::EntityHandle;
using gw::seq::GwseqKeyframe;
using gw::seq::GwseqQuat;
using gw::seq::GwseqReader;
using gw::seq::GwseqTrack;
using gw::seq::GwseqTrackType;
using gw::seq::GwseqTrackView;
using gw::seq::GwseqTransformPayloadKind;
using gw::seq::GwseqWriter;
using gw::seq::SeqOk;
using gw::seq::SeqResult;
using gw::seq::Vec3_f64;

}  // namespace

SeqResult append_empty_position_transform_track(GwseqReader&           reader,
                                                std::vector<std::uint8_t>& bytes,
                                                const std::uint32_t        new_track_id) noexcept {
    for (const auto& t : reader.tracks()) {
        if (t.track_id == new_track_id) {
            return gw::seq::SeqResult{gw::core::unexpected(gw::seq::SeqError::BadTrackId)};
        }
    }

    GwseqWriter w(bytes);
    const auto& hdr = reader.header();
    if (const auto h = w.write_header(hdr.frame_rate, hdr.duration_frames); !h.has_value()) return h;

    ArenaAllocator arena(64u * 1024u);

    for (const GwseqTrack& tr : reader.tracks()) {
        if (tr.keyframe_count == 0u) {
            if (tr.track_type == GwseqTrackType::Transform) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Transform,
                                               GwseqTransformPayloadKind::PositionVec3);
                    !b.has_value()) {
                    return b;
                }
            } else if (tr.track_type == GwseqTrackType::Float) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Float); !b.has_value()) return b;
            } else if (tr.track_type == GwseqTrackType::Bool) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Bool); !b.has_value()) return b;
            } else if (tr.track_type == GwseqTrackType::Event) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Event); !b.has_value()) return b;
            } else if (tr.track_type == GwseqTrackType::EntityRef) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::EntityRef); !b.has_value()) return b;
            }
            continue;
        }

        if (tr.track_type == GwseqTrackType::Transform) {
            GwseqTrackView<Vec3_f64> v3{};
            const auto               r3 = reader.read_track(tr.track_id, arena, v3);
            if (r3.has_value() && !v3.empty()) {
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Transform,
                                               GwseqTransformPayloadKind::PositionVec3);
                    !b.has_value()) {
                    return b;
                }
                for (const auto& k : v3.frames) {
                    if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
                }
            } else {
                arena.reset();
                GwseqTrackView<GwseqQuat> rq{};
                const auto                rq_r = reader.read_track(tr.track_id, arena, rq);
                if (!rq_r.has_value()) return rq_r;
                if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Transform,
                                               GwseqTransformPayloadKind::RotationQuat);
                    !b.has_value()) {
                    return b;
                }
                for (const auto& k : rq.frames) {
                    if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
                }
            }
        } else if (tr.track_type == GwseqTrackType::Float) {
            GwseqTrackView<float> fv{};
            const auto            fr = reader.read_track(tr.track_id, arena, fv);
            if (!fr.has_value()) return fr;
            if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Float); !b.has_value()) return b;
            for (const auto& k : fv.frames) {
                if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
            }
        } else if (tr.track_type == GwseqTrackType::Bool) {
            GwseqTrackView<bool> bv{};
            const auto           br = reader.read_track(tr.track_id, arena, bv);
            if (!br.has_value()) return br;
            if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Bool); !b.has_value()) return b;
            for (const auto& k : bv.frames) {
                if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
            }
        } else if (tr.track_type == GwseqTrackType::Event) {
            GwseqTrackView<bool> ev{};
            const auto           er = reader.read_track(tr.track_id, arena, ev);
            if (!er.has_value()) return er;
            if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::Event); !b.has_value()) return b;
            for (const auto& k : ev.frames) {
                if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
            }
        } else if (tr.track_type == GwseqTrackType::EntityRef) {
            GwseqTrackView<EntityHandle> rv{};
            const auto                   rr = reader.read_track(tr.track_id, arena, rv);
            if (!rr.has_value()) return rr;
            if (const auto b = w.begin_track(tr.track_id, GwseqTrackType::EntityRef); !b.has_value()) return b;
            for (const auto& k : rv.frames) {
                if (const auto wk = w.write_keyframe(k); !wk.has_value()) return wk;
            }
        }
        arena.reset();
    }

    if (const auto b = w.begin_track(new_track_id, GwseqTrackType::Transform,
                                   GwseqTransformPayloadKind::PositionVec3);
        !b.has_value()) {
        return b;
    }
    return w.finalise();
}

}  // namespace gw::editor::seq
