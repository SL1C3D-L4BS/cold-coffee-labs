// engine/audio/music_streamer.cpp
#include "engine/audio/music_streamer.hpp"

#include <algorithm>
#include <cstring>

namespace gw::audio {

MusicStreamer::MusicStreamer(std::unique_ptr<IStreamDecoder> decoder,
                             MusicStreamConfig cfg)
    : decoder_(std::move(decoder)), cfg_(cfg) {
    if (!decoder_) {
        dead_ = true;
        return;
    }
    frame_stride_ = decoder_->channels() == 0 ? 1 : decoder_->channels();
    capacity_frames_ = cfg_.ring_capacity_frames;
    if (capacity_frames_ == 0) capacity_frames_ = 4800;
    ring_ = std::make_unique<float[]>(capacity_frames_ * frame_stride_);
}

std::size_t MusicStreamer::fill() {
    if (dead_) return 0;
    if (!decoder_) { dead_ = true; return 0; }

    const std::size_t free_frames = capacity_frames_ - queued_frames_;
    if (free_frames == 0) return 0;

    const std::size_t target = std::min<std::size_t>(cfg_.fill_page_frames, free_frames);

    // Write into the ring starting at write_head_, handling wrap.
    std::size_t filled = 0;
    std::size_t remaining = target;
    while (remaining > 0) {
        const std::size_t linear = std::min<std::size_t>(remaining, capacity_frames_ - write_head_);
        float* dst = ring_.get() + write_head_ * frame_stride_;
        const std::size_t got = decoder_->pull(std::span<float>(dst, linear * frame_stride_), linear);
        if (got == 0) {
            // EOF — either loop or mark eof.
            if (cfg_.loop) {
                decoder_->rewind();
                continue;
            }
            eof_ = true;
            break;
        }
        write_head_ = (write_head_ + got) % capacity_frames_;
        queued_frames_ += got;
        filled += got;
        remaining -= got;
    }
    stats_.fills += 1;
    stats_.queued_frames = static_cast<uint32_t>(queued_frames_);
    return filled;
}

std::size_t MusicStreamer::pull(std::span<float> out, std::size_t frame_count) {
    stats_.pulls += 1;
    if (dead_) {
        const std::size_t n = frame_count * frame_stride_;
        if (out.size() >= n) std::memset(out.data(), 0, n * sizeof(float));
        stats_.underruns += 1;
        return 0;
    }
    if (queued_frames_ < frame_count) {
        stats_.underruns += 1;
    }
    const std::size_t available = std::min<std::size_t>(frame_count, queued_frames_);
    std::size_t copied = 0;
    std::size_t remaining = available;
    while (remaining > 0) {
        const std::size_t linear = std::min<std::size_t>(remaining, capacity_frames_ - read_head_);
        const float* src = ring_.get() + read_head_ * frame_stride_;
        std::memcpy(out.data() + copied * frame_stride_, src, linear * frame_stride_ * sizeof(float));
        read_head_ = (read_head_ + linear) % capacity_frames_;
        queued_frames_ -= linear;
        copied += linear;
        remaining -= linear;
    }

    // Zero-fill underrun portion.
    if (copied < frame_count) {
        const std::size_t zeros = (frame_count - copied) * frame_stride_;
        std::memset(out.data() + copied * frame_stride_, 0, zeros * sizeof(float));
    }
    stats_.queued_frames = static_cast<uint32_t>(queued_frames_);
    return copied;
}

bool MusicStreamer::seek(uint64_t frame_index) {
    if (!decoder_) return false;
    if (!decoder_->seek(frame_index)) return false;
    read_head_ = 0;
    write_head_ = 0;
    queued_frames_ = 0;
    eof_ = false;
    return true;
}

void MusicStreamer::rewind() {
    if (!decoder_) return;
    decoder_->rewind();
    read_head_ = 0;
    write_head_ = 0;
    queued_frames_ = 0;
    eof_ = false;
}

} // namespace gw::audio
