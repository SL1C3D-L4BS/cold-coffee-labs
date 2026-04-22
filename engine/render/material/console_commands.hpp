#pragma once
// engine/render/material/console_commands.hpp — ADR-0074 §6, ADR-0075 §6.

namespace gw::console       { class ConsoleService; }
namespace gw::render::material {

class MaterialWorld;

void register_material_console_commands(console::ConsoleService&, MaterialWorld&);

} // namespace gw::render::material
