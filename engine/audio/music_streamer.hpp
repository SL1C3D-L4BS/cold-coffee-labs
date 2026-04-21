#pragma once
// engine/audio/music_streamer.hpp — ring-buffered streaming music (ADR-0019).
//
// The streamer owns an IStreamDecoder and a fixed-capacity ring buffer. The
// main thread (or a jobs::Task in production) calls `fill` to advance
// decode; the audio thread calls `pull` to drain into the mixer. Pull is
// wait-free: underrun → silence + a stat counter bump.
//
// Looping is implemented in the streamer (not the decoder) so that any
// codec gets loop-seam handling for free.

#include "engine/audio/decoder_registry.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace gw::audio {

struct MusicStreamStats {
    uint32_t underruns{0};
    uint32_t pulls{0};
    uint32_t fills{0};
    uint32_t queued_frames{0};
};

struct MusicStreamConfig {
    uint32_t ring_capacity_frames{48'000};  // 1 s at 48 kHz
    uint32_t fill_page_frames{4'800};       // 100 ms
    bool     loop{false};
};

class MusicStreamer {
public:
    MusicStreamer(std::unique_ptr<IStreamDecoder> decoder,
                  MusicStreamConfig cfg = {});
    ~MusicStreamer() = default;

    MusicStreamer(const MusicStreamer&) = delete;
    MusicStreamer& operator=(const MusicStreamer&) = delete;

    // Main thread: advance decode by up to `fill_page_frames` frames if
    // there is space. Returns number of frames newly queued. May be called
    // zero or more times per frame.
    std::size_t fill();

    // Audio thread: drain up to `frame_count` interleaved frames.
    std::size_t pull(std::span<float> out, std::size_t frame_count);

    // Main thread: seek to frame. Clears the queue; next `fill` refills.
    bool seek(uint64_t frame_index);

    // Rewind + clear queue.
    void rewind();

    [[nodiscard]] const MusicStreamStats& stats() const noexcept { return stats_; }
    [[nodiscard]] uint32_t sample_rate() const noexcept { return decoder_ ? decoder_->sample_rate() : 0; }
    [[nodiscard]] uint8_t  channels()    const noexcept { return decoder_ ? decoder_->channels()    : 0; }
    [[nodiscard]] bool     is_dead()     const noexcept { return dead_; }
    [[nodiscard]] bool     is_eof()      const noexcept { return eof_ && queued_frames_ == 0; }

    // For tests: expose ring capacity.
    [[nodiscard]] uint32_t ring_capacity_frames() const noexcept { return cfg_.ring_capacity_frames; }

private:
    std::unique_ptr<IStreamDecoder> decoder_;
    MusicStreamConfig               cfg_;

    std::unique_ptr<float[]> ring_;      // channels * capacity_frames entries
    std::size_t              frame_stride_{1};  // channels
    std::size_t              capacity_frames_{0};
    std::size_t              read_head_{0};     // in frames
    std::size_t              write_head_{0};    // in frames
    std::size_t              queued_frames_{0};

    bool dead_{false};
    bool eof_{false};

    MusicStreamStats stats_{};
};

} // namespace gw::audio
