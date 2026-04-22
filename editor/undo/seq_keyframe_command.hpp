#pragma once
// editor/undo/seq_keyframe_command.hpp — .gwseq keyframe frame index edits (Phase 18-B).

#include "editor/undo/command_stack.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gw::editor::undo {

/// Patches the first `sizeof(std::uint32_t)` bytes at `frame_field_offset` in a
/// mutable `.gwseq` blob (little-endian frame number for all keyframe types).
class SeqKeyframeFramePatchCommand final : public ICommand {
public:
    SeqKeyframeFramePatchCommand(std::string label, std::vector<std::uint8_t>* blob,
                                 std::size_t frame_field_offset, std::uint32_t before,
                                 std::uint32_t after) noexcept
        : label_(std::move(label))
        , blob_(blob)
        , off_(frame_field_offset)
        , before_(before)
        , after_(after) {}

    void do_() override { apply(after_); }
    void undo() override { apply(before_); }

    [[nodiscard]] std::string_view label() const override { return label_; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this) + label_.capacity();
    }

private:
    void apply(std::uint32_t f) noexcept {
        if (blob_ == nullptr || off_ + sizeof(std::uint32_t) > blob_->size()) return;
        std::uint8_t* p = blob_->data() + off_;
        p[0] = static_cast<std::uint8_t>(f & 0xFFu);
        p[1] = static_cast<std::uint8_t>((f >> 8u) & 0xFFu);
        p[2] = static_cast<std::uint8_t>((f >> 16u) & 0xFFu);
        p[3] = static_cast<std::uint8_t>((f >> 24u) & 0xFFu);
    }

    std::string              label_{};
    std::vector<std::uint8_t>* blob_{};
    std::size_t                off_{};
    std::uint32_t              before_{};
    std::uint32_t              after_{};
};

}  // namespace gw::editor::undo
