#pragma once
// engine/render/post/post_world.hpp — ADR-0079/0081 Wave 17F.

#include "engine/render/post/bloom.hpp"
#include "engine/render/post/chromatic.hpp"
#include "engine/render/post/dof.hpp"
#include "engine/render/post/grain.hpp"
#include "engine/render/post/motion_blur.hpp"
#include "engine/render/post/post_types.hpp"
#include "engine/render/post/taa.hpp"
#include "engine/render/post/tonemap.hpp"

#include <cstdint>
#include <memory>

namespace gw::config { class CVarRegistry; }
namespace gw::events { class CrossSubsystemBus; }

namespace gw::render::post {

struct PostConfig {
    BloomConfig       bloom{};
    TonemapConfig     tonemap{};
    TaaConfig         taa{};
    MotionBlurConfig  motion_blur{};
    DofConfig         dof{};
    ChromaticConfig   chromatic{};
    GrainConfig       grain{};
    std::uint32_t     width{1920};
    std::uint32_t     height{1080};
};

class PostWorld {
public:
    PostWorld();
    ~PostWorld();
    PostWorld(const PostWorld&)            = delete;
    PostWorld& operator=(const PostWorld&) = delete;

    [[nodiscard]] bool initialize(PostConfig cfg,
                                   config::CVarRegistry* cvars = nullptr,
                                   events::CrossSubsystemBus* bus = nullptr);
    void               shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    // Frozen pipeline order (ADR-0079 §2):
    //   bloom → taa → motion_blur → dof → chromatic → tonemap → grain
    void step(double dt_seconds);

    // Pulls CVars into live config (invoked on step).
    void pull_cvars();

    [[nodiscard]] const PostConfig& config() const noexcept;
    [[nodiscard]] PostStats         stats()  const noexcept;

    // Sub-system accessors used by sandbox + tests (read-only).
    [[nodiscard]] const Bloom& bloom() const noexcept;
    [[nodiscard]] const Taa&   taa()   const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::render::post
