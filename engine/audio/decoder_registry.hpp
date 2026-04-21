#pragma once
// engine/audio/decoder_registry.hpp — per-codec IDecoder plug-ins.
//
// The registry is data-driven: each decoder registers a Codec it can open
// and returns a stream object. Built-in: RawF32 (always present). Vorbis /
// Opus / FLAC: gated by GW_AUDIO_VORBIS / GW_AUDIO_OPUS / GW_AUDIO_FLAC.

#include "engine/audio/kaudio_format.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace gw::audio {

// Full-file decoder — decodes all frames into a float buffer.
class IDecoder {
public:
    virtual ~IDecoder() = default;
    virtual bool decode_all(const kaudio::Header& h,
                            std::span<const uint8_t> body,
                            std::vector<float>& out) = 0;
    virtual kaudio::Codec codec() const noexcept = 0;
};

// Streaming decoder — used by MusicStreamer to pull pages.
class IStreamDecoder {
public:
    virtual ~IStreamDecoder() = default;

    // Total length in frames; 0 if unknown.
    virtual uint64_t total_frames() const noexcept = 0;
    virtual uint32_t sample_rate() const noexcept = 0;
    virtual uint8_t  channels() const noexcept = 0;
    virtual kaudio::Codec codec() const noexcept = 0;

    // Read up to `frame_count` frames into `out` (interleaved channels).
    // Returns the number of frames actually decoded. 0 = EOF (unless looping).
    virtual std::size_t pull(std::span<float> out, std::size_t frame_count) = 0;

    // Seek to a specific frame index. Returns true on success.
    virtual bool seek(uint64_t frame_index) = 0;

    // Rewind to the start. Convenience for looping.
    virtual void rewind() = 0;
};

class DecoderRegistry {
public:
    DecoderRegistry();
    ~DecoderRegistry() = default;

    // Look up a decoder for a given codec. Returns nullptr if unsupported.
    [[nodiscard]] IDecoder* find(kaudio::Codec c) const noexcept;

    // Register a decoder (takes ownership).
    void register_decoder(std::unique_ptr<IDecoder>);

    // Decode a whole file. Convenience wrapper.
    bool decode_file(std::span<const uint8_t> file_bytes,
                     kaudio::Header& header_out,
                     std::vector<float>& pcm_out);

    // Create a streaming decoder for the given file.
    // Returns null if the codec is not streamable in this build.
    std::unique_ptr<IStreamDecoder> open_stream(std::span<const uint8_t> file_bytes);

    [[nodiscard]] std::size_t size() const noexcept { return decoders_.size(); }

private:
    std::vector<std::unique_ptr<IDecoder>> decoders_;
};

// Built-in decoders.
[[nodiscard]] std::unique_ptr<IDecoder>       make_raw_f32_decoder();
[[nodiscard]] std::unique_ptr<IStreamDecoder> make_raw_f32_stream(std::span<const uint8_t> file_bytes);

} // namespace gw::audio
