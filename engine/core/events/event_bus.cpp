// engine/core/events/event_bus.cpp — TU anchor for the event module.
// All templates are header-only; this TU exists so the link step has a
// file to compile when the module grows (Tracy hooks, stats, etc).

#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

namespace gw::events {

// Non-templated helpers can live here as the module evolves.

} // namespace gw::events
