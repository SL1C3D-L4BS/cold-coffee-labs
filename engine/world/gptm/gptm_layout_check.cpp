// engine/world/gptm/gptm_layout_check.cpp — compile-time GPTM layout checks (kept out of headers for IDE/clangd).

#include "engine/world/gptm/gptm_types.hpp"

static_assert(sizeof(gw::world::gptm::GptmVertex) == 32, "GptmVertex must remain 32-byte for GPU cache behaviour");
static_assert(sizeof(gw::world::gptm::GptmTileInstance) == 32, "GptmTileInstance should stay 32-byte for tight SSBO packing");
