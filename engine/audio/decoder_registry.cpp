// engine/audio/decoder_registry.cpp
#include "engine/audio/decoder_registry.hpp"

#include <cstring>

namespace gw::audio {

namespace {

class RawF32Decoder final : public IDecoder {
public:
    bool decode_all(const kaudio::Header& h, std::span<const uint8_t> body,
                    std::vector<float>& out) override {
        return kaudio::decode_raw_f32(h, body, out);
    }
    kaudio::Codec codec() const noexcept override { return kaudio::Codec::RawF32; }
};

class RawF32StreamDecoder final : public IStreamDecoder {
public:
    RawF32StreamDecoder(kaudio::Header h, std::vector<uint8_t> owned)
        : header_(h), owned_(std::move(owned)) {
        body_ptr_ = owned_.data() + header_.body_offset;
        frame_bytes_ = static_cast<std::size_t>(header_.channels) * sizeof(float);
    }

    uint64_t total_frames() const noexcept override { return header_.num_frames; }
    uint32_t sample_rate() const noexcept override { return header_.sample_rate; }
    uint8_t  channels()    const noexcept override { return header_.channels; }
    kaudio::Codec codec()  const noexcept override { return kaudio::Codec::RawF32; }

    std::size_t pull(std::span<float> out, std::size_t frame_count) override {
        if (header_.num_frames == 0) return 0;
        const std::size_t remaining = static_cast<std::size_t>(header_.num_frames - cursor_);
        const std::size_t n = frame_count < remaining ? frame_count : remaining;
        if (n == 0) return 0;
        std::memcpy(out.data(), body_ptr_ + cursor_ * frame_bytes_, n * frame_bytes_);
        cursor_ += n;
        return n;
    }

    bool seek(uint64_t frame_index) override {
        if (frame_index > header_.num_frames) return false;
        cursor_ = frame_index;
        return true;
    }

    void rewind() override { cursor_ = 0; }

private:
    kaudio::Header       header_{};
    std::vector<uint8_t> owned_;
    const uint8_t*       body_ptr_{nullptr};
    std::size_t          frame_bytes_{0};
    uint64_t             cursor_{0};
};

} // namespace

DecoderRegistry::DecoderRegistry() {
    register_decoder(make_raw_f32_decoder());
}

IDecoder* DecoderRegistry::find(kaudio::Codec c) const noexcept {
    for (auto& d : decoders_) {
        if (d->codec() == c) return d.get();
    }
    return nullptr;
}

void DecoderRegistry::register_decoder(std::unique_ptr<IDecoder> d) {
    decoders_.push_back(std::move(d));
}

bool DecoderRegistry::decode_file(std::span<const uint8_t> file_bytes,
                                  kaudio::Header& header_out,
                                  std::vector<float>& pcm_out) {
    kaudio::Error err{};
    if (!kaudio::read_header(file_bytes, header_out, err)) return false;
    if (file_bytes.size() < header_out.body_offset + header_out.body_bytes) return false;

    const auto body = file_bytes.subspan(
        static_cast<std::size_t>(header_out.body_offset),
        static_cast<std::size_t>(header_out.body_bytes));

    if (auto* dec = find(header_out.codec); dec) {
        return dec->decode_all(header_out, body, pcm_out);
    }
    return false;
}

std::unique_ptr<IStreamDecoder> DecoderRegistry::open_stream(std::span<const uint8_t> file_bytes) {
    kaudio::Header header{};
    kaudio::Error err{};
    if (!kaudio::read_header(file_bytes, header, err)) return nullptr;
    if (file_bytes.size() < header.body_offset + header.body_bytes) return nullptr;

    // Copy the bytes into owned storage because the stream decoder must
    // out-live any borrowed buffer.
    std::vector<uint8_t> owned(file_bytes.begin(), file_bytes.end());

    if (header.codec == kaudio::Codec::RawF32) {
        return std::make_unique<RawF32StreamDecoder>(header, std::move(owned));
    }
    return nullptr;
}

std::unique_ptr<IDecoder> make_raw_f32_decoder() {
    return std::make_unique<RawF32Decoder>();
}

std::unique_ptr<IStreamDecoder> make_raw_f32_stream(std::span<const uint8_t> file_bytes) {
    kaudio::Header header{};
    kaudio::Error err{};
    if (!kaudio::read_header(file_bytes, header, err)) return nullptr;
    if (file_bytes.size() < header.body_offset + header.body_bytes) return nullptr;
    std::vector<uint8_t> owned(file_bytes.begin(), file_bytes.end());
    if (header.codec == kaudio::Codec::RawF32) {
        return std::make_unique<RawF32StreamDecoder>(header, std::move(owned));
    }
    return nullptr;
}

} // namespace gw::audio
