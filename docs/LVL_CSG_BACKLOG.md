# Phase-2 backlog — CSG / booleans

**ADR gate:** No CSG or mesh booleans in the blockout path until:

1. Phase 8 **Vulkan mesh draw** is stable for dynamic meshes.
2. **Collision** story for boolean results is defined (Jolt mesh vs compound).
3. **Undo** granularity for boolean ops is specified.

Until then, blockout remains **primitive union only** (no boolean CSG). See `docs/LVL_BLOCKOUT_RESEARCH.md` for Phase-1 scope.
