# Stub completion matrix — Wave 0 (foundation)

**Program:** Cursor plan `stub_completion_program_1d2f4e69` (local plans folder).  
**Exit:** each row has a named automated test or review artifact.

| ID | Inventory / deliverable | Owner | Exit test / review |
|----|-------------------------|--------|-------------------|
| W0.0 | STUB matrix (this file) + cross-links to `graphify-out/GRAPH_REPORT.md` | Eng | 100% Wave 0 rows have exit |
| W0.2 | `gw::core::log` ring + `GW_LOG_FILE` + callbacks (`engine/core/log.*`) | Eng | Build + run editor; `GW_LOG_FILE` creates file |
| W0.3 | Console panel: filter, pause, copy, clear, severity colors (`editor/panels/console_panel.*`) | Eng | Visual + no “Phase 11” string |
| W0.4 | Profiler: sparkline, ECS count, Tracy host UI (`editor/panels/audit/profiler_panel.*`) | Eng | Manual in editor |
| W0.5 | Fuzz drivers wired to engine APIs (`tests/fuzz/fuzz_*.cpp`) | Eng | `ctest -R fuzz_` when `GW_FUZZ=ON` |
| W0.6 | Perf TODOs cleared: AI budgets use `engine/ai_runtime`; `GW_GPU_TESTS=1` runs `VulkanInstance`+`hal::VulkanDevice` smoke (full `MilestoneValidator` waits on render CMake wiring). | Eng | `ctest -L perf` + optional `GW_GPU_TESTS=1` |

| Wave 1+ (pointer only) | Key exit |
|------------------------|----------|
| W1 | `rg` empty `on_imgui_render` in `editor/panels/audit` = 0 (per plan) |
| W2 | BLD `StubHandler` → production (per plan) |
| W3–W10 | Per plan “Exit” rows |

*Graphify node reference: Community 4 (editor / `on_imgui_render` cluster) — see `graphify-out/GRAPH_REPORT.md`.*
