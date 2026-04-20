# Games (engine-vs-game separation)

**This directory is a placeholder.** There are no games in `docs/games/` at the time of writing (2026-04-19).

---

## Why this directory exists

Greywater_Engine (`greywater_core`) is the **product** Cold Coffee Labs is building. The engine is a library — titles are consumers. The engine must never import from a specific game, and game-vision documents must never leak into engine documentation.

When the studio begins design on its first title, **its vision documents live here**, numbered `12–` onward (continuing from the canonical set that ends at `11`). Each game lives in its own subdirectory with its own `README.md` explaining the engine/game relationship.

## Expected structure (when a title exists)

```
docs/games/
├── README.md                                  (this file)
└── <codename>/
    ├── README.md                              game-vision overview
    ├── 12_<CODENAME>_GAME_DESIGN.md           high-level design
    ├── 13_<CODENAME>_SYSTEMS.md               game-specific systems
    ├── 14_<CODENAME>_CONTENT_PIPELINE.md      content authoring for this title
    └── ... (additional game-specific docs)
```

## The rule

**Engine code does not reference game docs. Game docs reference engine docs.** If an engine subsystem is being designed around a specific title's needs, that is a violation — the engine must accommodate the title via public API, not by hardcoding the title.

See `00_SPECIFICATION_Claude_Opus_4.7.md` §7 for the formal engine-vs-game separation contract.

---

*Games ship on engines. Engines outlive games.*
