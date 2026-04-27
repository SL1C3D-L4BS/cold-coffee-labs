#pragma once

/// Central TU for gameplay DLL symbols that must survive link-time pruning
/// until ECS registrars own them (ADR-0121 program — gameplay-link-hygiene).
void gw_gameplay_touch_dll_linkage() noexcept;
