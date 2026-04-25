// tests/fuzz/fuzz_gwscene.cpp — LibFuzzer harness for `.gwscene` / decode_scene.

#include "engine/ecs/world.hpp"
#include "engine/scene/scene_file.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    gw::ecs::World w;
    (void)gw::scene::decode_scene(std::span<const std::uint8_t>(data, size), w, {});
    return 0;
}
