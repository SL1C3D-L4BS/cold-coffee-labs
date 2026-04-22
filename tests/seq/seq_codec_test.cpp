// tests/seq/seq_codec_test.cpp — Phase 18-A `.gwseq` codec + migration tests.

#include <doctest/doctest.h>

#include "engine/memory/arena_allocator.hpp"
#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

using gw::memory::ArenaAllocator;
using gw::seq::EntityHandle;
using gw::seq::GwseqInterpolation;
using gw::seq::GwseqKeyframe;
using gw::seq::GwseqQuat;
using gw::seq::GwseqReader;
using gw::seq::GwseqTrackType;
using gw::seq::GwseqTrackView;
using gw::seq::GwseqTransformPayloadKind;
using gw::seq::GwseqWriter;
using gw::seq::Vec3_f64;

namespace {

[[nodiscard]] float sample_linear_mid(const GwseqKeyframe<float>* k, std::size_t n,
                                      double frame) noexcept {
    if (n < 2u) return 0.f;
    const double fa = static_cast<double>(k[0].frame);
    const double fb = static_cast<double>(k[1].frame);
    const double t  = (frame - fa) / (fb - fa);
    return static_cast<float>(static_cast<double>(k[0].value) * (1.0 - t) +
                               static_cast<double>(k[1].value) * t);
}

void migrate_v1_float_double_all_tracks(gw::seq::GwseqReader& self) {
    const auto bytes = self.mutable_owned_bytes();
    if (bytes.size() < sizeof(gw::seq::GwseqHeader)) return;
    auto* hdr = reinterpret_cast<gw::seq::GwseqHeader*>(bytes.data());
    if (hdr->version != 1u) return;

    const auto* tracks = reinterpret_cast<const gw::seq::GwseqTrack*>(bytes.data() +
                                                                       sizeof(gw::seq::GwseqHeader));
    for (std::uint32_t ti = 0; ti < hdr->track_count; ++ti) {
        const gw::seq::GwseqTrack& tr = tracks[ti];
        if (tr.track_type != GwseqTrackType::Float || tr.keyframe_count == 0u) continue;
        const std::size_t off = static_cast<std::size_t>(tr.keyframe_offset);
        if (off + static_cast<std::size_t>(tr.keyframe_count) * sizeof(GwseqKeyframe<float>) >
            bytes.size()) {
            return;
        }
        auto* kf =
            reinterpret_cast<GwseqKeyframe<float>*>(bytes.data() + off);
        for (std::uint32_t ki = 0; ki < tr.keyframe_count; ++ki) {
            kf[ki].value *= 2.f;
        }
    }
    hdr->version = gw::seq::kGwseqVersion;
}

}  // namespace

TEST_CASE("gwseq_format — keyframe structs are trivially copyable") {
    static_assert(std::is_trivially_copyable_v<GwseqKeyframe<Vec3_f64>>);
    static_assert(std::is_trivially_copyable_v<GwseqKeyframe<GwseqQuat>>);
    static_assert(std::is_trivially_copyable_v<GwseqKeyframe<float>>);
    static_assert(std::is_trivially_copyable_v<GwseqKeyframe<bool>>);
    static_assert(std::is_trivially_copyable_v<GwseqKeyframe<EntityHandle>>);
}

TEST_CASE("gwseq_codec — round-trip Transform (Vec3), Float, Bool, Event, EntityRef, Quat") {
    std::vector<std::uint8_t> blob;
    GwseqWriter               w(blob);
    REQUIRE(w.write_header(30u, 120u).has_value());

    REQUIRE(w.begin_track(1u, GwseqTrackType::Transform, GwseqTransformPayloadKind::PositionVec3)
                .has_value());
    GwseqKeyframe<Vec3_f64> k0{};
    k0.frame         = 0u;
    k0.value         = Vec3_f64{0.0, 0.0, 0.0};
    k0.interpolation = GwseqInterpolation::Linear;
    GwseqKeyframe<Vec3_f64> k1 = k0;
    k1.frame = 30u;
    k1.value = Vec3_f64{3.0, 0.0, 0.0};
    REQUIRE(w.write_keyframe(k0).has_value());
    REQUIRE(w.write_keyframe(k1).has_value());

    REQUIRE(w.begin_track(2u, GwseqTrackType::Transform, GwseqTransformPayloadKind::RotationQuat)
                .has_value());
    GwseqKeyframe<GwseqQuat> q0{};
    q0.frame         = 0u;
    q0.value         = GwseqQuat{1.0, 0.0, 0.0, 0.0};
    q0.interpolation = GwseqInterpolation::Linear;
    GwseqKeyframe<GwseqQuat> q1 = q0;
    q1.frame = 30u;
    q1.value = GwseqQuat{0.7071067811865476, 0.0, 0.7071067811865476, 0.0};
    REQUIRE(w.write_keyframe(q0).has_value());
    REQUIRE(w.write_keyframe(q1).has_value());

    REQUIRE(w.begin_track(3u, GwseqTrackType::Float).has_value());
    GwseqKeyframe<float> f0{};
    f0.frame         = 0u;
    f0.value         = 1.f;
    f0.interpolation = GwseqInterpolation::Linear;
    GwseqKeyframe<float> f1 = f0;
    f1.frame = 10u;
    f1.value = 3.f;
    REQUIRE(w.write_keyframe(f0).has_value());
    REQUIRE(w.write_keyframe(f1).has_value());

    REQUIRE(w.begin_track(4u, GwseqTrackType::Bool).has_value());
    GwseqKeyframe<bool> b0{};
    b0.frame         = 0u;
    b0.value         = false;
    b0.interpolation = GwseqInterpolation::Linear;
    GwseqKeyframe<bool> b1 = b0;
    b1.frame = 10u;
    b1.value = true;
    REQUIRE(w.write_keyframe(b0).has_value());
    REQUIRE(w.write_keyframe(b1).has_value());

    REQUIRE(w.begin_track(5u, GwseqTrackType::Event).has_value());
    GwseqKeyframe<bool> e0{};
    e0.frame         = 5u;
    e0.value         = true;
    e0.interpolation = GwseqInterpolation::Step;
    REQUIRE(w.write_keyframe(e0).has_value());

    REQUIRE(w.begin_track(6u, GwseqTrackType::EntityRef).has_value());
    GwseqKeyframe<EntityHandle> r0{};
    r0.frame         = 0u;
    r0.value         = EntityHandle::from_parts(3u, 2u);
    r0.interpolation = GwseqInterpolation::Step;
    REQUIRE(w.write_keyframe(r0).has_value());

    REQUIRE(w.finalise().has_value());
    REQUIRE(blob.size() > sizeof(gw::seq::GwseqHeader));

    GwseqReader r;
    REQUIRE(r.open_bytes(std::move(blob)).has_value());

    ArenaAllocator arena(64u * 1024u);
    GwseqTrackView<Vec3_f64> tv{};
    REQUIRE(r.read_track(1u, arena, tv).has_value());
    REQUIRE(tv.frames.size() == 2u);
    CHECK(tv.frames[0].value.x == doctest::Approx(0.0));
    CHECK(tv.frames[1].value.x == doctest::Approx(3.0));

    arena.reset();
    GwseqTrackView<GwseqQuat> qv{};
    REQUIRE(r.read_track(2u, arena, qv).has_value());
    REQUIRE(qv.frames.size() == 2u);
    CHECK(qv.frames[0].value.w == doctest::Approx(1.0));

    arena.reset();
    GwseqTrackView<float> fv{};
    REQUIRE(r.read_track(3u, arena, fv).has_value());
    REQUIRE(fv.frames.size() == 2u);
    constexpr double mid_frame = 5.0;
    const float      mid_v     = sample_linear_mid(fv.frames.data(), fv.frames.size(), mid_frame);
    CHECK(mid_v == doctest::Approx(2.f));

    arena.reset();
    GwseqTrackView<bool> bv{};
    REQUIRE(r.read_track(4u, arena, bv).has_value());
    CHECK_FALSE(bv.frames[0].value);
    CHECK(bv.frames[1].value);

    arena.reset();
    GwseqTrackView<bool> ev{};
    REQUIRE(r.read_track(5u, arena, ev).has_value());
    CHECK(ev.frames[0].value);

    arena.reset();
    GwseqTrackView<EntityHandle> rv{};
    REQUIRE(r.read_track(6u, arena, rv).has_value());
    CHECK(rv.frames[0].value.index() == 3u);
    CHECK(rv.frames[0].value.generation() == 2u);
}

TEST_CASE("gwseq_codec — migration hook runs on version mismatch") {
    std::vector<std::uint8_t> blob;
    GwseqWriter               w(blob);
    REQUIRE(w.write_header(60u, 10u).has_value());
    REQUIRE(w.begin_track(9u, GwseqTrackType::Float).has_value());
    GwseqKeyframe<float> k{};
    k.frame         = 0u;
    k.value         = 2.5f;
    k.interpolation = GwseqInterpolation::Linear;
    REQUIRE(w.write_keyframe(k).has_value());
    REQUIRE(w.finalise().has_value());

    auto* hdr = reinterpret_cast<gw::seq::GwseqHeader*>(blob.data());
    hdr->version = 1u;

    GwseqReader r;
    r.add_migrator([](gw::seq::GwseqReader& self, gw::seq::GwseqTrackType,
                      std::span<std::uint8_t>, ArenaAllocator&) -> gw::seq::SeqResult {
        migrate_v1_float_double_all_tracks(self);
        return gw::seq::SeqOk{};
    });
    REQUIRE(r.open_bytes(std::move(blob)).has_value());

    ArenaAllocator         arena(64u * 1024u);
    const std::size_t      blocks_before = arena.block_count();
    GwseqTrackView<float> fv{};
    REQUIRE(r.read_track(9u, arena, fv).has_value());
    REQUIRE(fv.frames.size() == 1u);
    CHECK(fv.frames[0].value == doctest::Approx(5.0f));
    CHECK(r.header().version == gw::seq::kGwseqVersion);
    CHECK(arena.block_count() == blocks_before);
}

TEST_CASE("gwseq_codec — arena allocator stays single-block without migrator allocations") {
    std::vector<std::uint8_t> blob;
    GwseqWriter               w(blob);
    REQUIRE(w.write_header(24u, 48u).has_value());
    REQUIRE(w.begin_track(1u, GwseqTrackType::Float).has_value());
    GwseqKeyframe<float> k{};
    k.frame = 0u;
    k.value = 1.f;
    REQUIRE(w.write_keyframe(k).has_value());
    REQUIRE(w.finalise().has_value());

    GwseqReader      r;
    ArenaAllocator   arena(4096u);
    const std::size_t bc0 = arena.block_count();
    REQUIRE(r.open_bytes(std::move(blob)).has_value());
    GwseqTrackView<float> fv{};
    REQUIRE(r.read_track(1u, arena, fv).has_value());
    REQUIRE(r.read_track(1u, arena, fv).has_value());
    CHECK(arena.block_count() == bc0);
}
