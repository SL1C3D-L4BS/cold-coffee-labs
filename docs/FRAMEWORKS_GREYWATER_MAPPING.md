# Framework mapping — Unity / Unreal / Godot → Greywater

Short parity table for asset and level authoring (editor plan).

| Concept | Unity | Unreal | Godot | Greywater |
|--------|-------|--------|-------|-----------|
| Asset identity | GUID + `.meta` | Object path + Asset Registry | `res://` + `.import` | Cook key + VFS path (`engine/assets/asset_types.hpp`) |
| Import / cook | Importers, AssetDatabase | AssetImportManager | Import dock | `tools/cook`, texture/mesh cookers |
| Content browser | Project + packages | Content Browser | FileSystem dock | **Asset Browser** + **Sacrilege Library** |
| Thumbnails | Asset preview cache | Thumbnail pool | EditorResourcePreview | **ImGuiTextureCache** (Vulkan) |
| Materials | ShaderGraph / URP | Materials | ShaderMaterial | `.gwmat` + templates (`engine/render/material/`) |
| Level blockout | ProBuilder | Modeling / brushes | CSG | **Phase 1:** primitives + ECS (`BlockoutPrimitiveComponent`); **Phase 2:** CSG ADR (`docs/LVL_CSG_BACKLOG.md`) |
| Deterministic levels | Addressables + hash | Streaming + seed | PackedScene + seed | **Hell Seed** + Blacklake GPTM (`docs/08_BLACKLAKE.md`) |

## Editor indexing

- **Unity AssetDatabase.Refresh** analogue: rebuild **`sacrilege_library.tsv`** / JSON via `tools/build_sacrilege_library.py` after AmbientCG sync + cook.
