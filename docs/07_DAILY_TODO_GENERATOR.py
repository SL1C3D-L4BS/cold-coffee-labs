#!/usr/bin/env python3
"""
07_DAILY_TODO_GENERATOR.py — Greywater

Generates one dated Markdown file per working day across the 37-month
buildout under `docs/daily/`. Run once at project start; rerun only if
the project start date or horizon changes. Rerunning is non-destructive:
existing files are skipped.

Usage:
    python docs/07_DAILY_TODO_GENERATOR.py

Configuration: edit START_DATE and HORIZON_DAYS below.
"""

from __future__ import annotations

import datetime as dt
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
START_DATE = dt.date(2026, 4, 20)    # Phase 1, week 01, day 01
HORIZON_DAYS = 1150                   # 162 weeks × 7 days + buffer for LTS kickoff
OUT_DIR = Path(__file__).resolve().parent / "daily"

# Phase boundaries in weeks (from 05_ROADMAP_AND_MILESTONES.md)
PHASES = [
    ("Phase 1 — Scaffolding & CI/CD",                           1,   4,  "First Light"),
    ("Phase 2 — Platform & Core Utilities",                      5,   9,  "—"),
    ("Phase 3 — Math, ECS, Jobs & Reflection",                  10,  16,  "Foundations Set"),
    ("Phase 4 — Renderer HAL & Vulkan Bootstrap",               17,  24,  "—"),
    ("Phase 5 — Frame Graph & Reference Renderer",              25,  32,  "Foundation Renderer"),
    ("Phase 6 — Asset Pipeline & Content Cook",                 33,  36,  "—"),
    ("Phase 7 — Editor Foundation",                             37,  42,  "Editor v0.1"),
    ("Phase 8 — Scene Serialization & Visual Scripting",        43,  47,  "—"),
    ("Phase 9 — BLD (Brewed Logic Directive, Rust)",            48,  59,  "Brewed Logic"),
    ("Phase 10 — Runtime I: Audio & Input",                     60,  65,  "—"),
    ("Phase 11 — Runtime II: UI, Events, Config, Console",      66,  71,  "Playable Runtime"),
    ("Phase 12 — Physics",                                      72,  77,  "—"),
    ("Phase 13 — Animation & Game AI Framework",                78,  83,  "Living Scene"),
    ("Phase 14 — Networking & Multiplayer",                     84,  95,  "Two Client Night"),
    ("Phase 15 — Persistence & Telemetry",                      96, 101,  "—"),
    ("Phase 16 — Platform Services, i18n & a11y",              102, 107,  "Ship Ready"),
    ("Phase 17 — Shader Pipeline Maturity, Materials & VFX",   108, 113,  "—"),
    ("Phase 18 — Cinematics & Mods",                           114, 118,  "Studio Ready"),
    ("Phase 19 — Greywater Universe Generation",               119, 126,  "Infinite Seed"),
    ("Phase 20 — Greywater Planetary Rendering",               127, 134,  "First Planet"),
    ("Phase 21 — Greywater Atmosphere & Space",                135, 142,  "First Launch"),
    ("Phase 22 — Greywater Survival Systems",                  143, 148,  "Gather & Build"),
    ("Phase 23 — Greywater Ecosystem & Faction AI",            149, 154,  "Living World"),
    ("Phase 24 — Hardening & Release",                         155, 162,  "Release Candidate"),
    ("Phase 25 — LTS Sustenance",                              163, 9999, "Quarterly LTS cuts"),
]


def phase_for_week(week: int) -> tuple[str, str]:
    """Return (phase_label, milestone_name) for a given week number."""
    for label, start, end, milestone in PHASES:
        if start <= week <= end:
            return label, milestone
    return ("Post-v1", "—")


def is_weekend(d: dt.date) -> bool:
    return d.weekday() >= 5  # 5=Saturday, 6=Sunday


def template(d: dt.date, week: int, phase_label: str, milestone: str) -> str:
    weekday = d.strftime("%A")
    sunday_note = ""
    if d.weekday() == 6:  # Sunday
        sunday_note = (
            "\n> **Sunday:** run the 30-minute weekly retro today. "
            "No coding during the retro. See `06_KANBAN_BOARD.md` §Weekly ritual.\n"
        )
    return f"""# {d.isoformat()} · {weekday}

**Week:** {week} · **Phase:** {phase_label} · **Milestone:** *{milestone}*
{sunday_note}
---

## 🎯 Today's Focus
<!-- Paste the Kanban card ID + Context field here before starting the block. -->

## 📋 Tasks Completed
- [ ]

## 🚧 Blockers
<!-- None, or describe what stopped progress. Link to card / doc / ADR. -->

## 📝 Notes / Log
<!-- Citations, surprises, design decisions, research snippets. -->

## 🔮 Tomorrow's Intention
<!-- One sentence. What will I pull first? -->
"""


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    generated = 0
    skipped = 0
    current = START_DATE

    for offset in range(HORIZON_DAYS):
        d = START_DATE + dt.timedelta(days=offset)
        if is_weekend(d) and d.weekday() == 5:
            # Saturday — rest day, no log generated
            skipped += 1
            continue

        week = (d - START_DATE).days // 7 + 1
        phase_label, milestone = phase_for_week(week)

        out_path = OUT_DIR / f"{d.isoformat()}.md"
        if out_path.exists():
            skipped += 1
            continue

        out_path.write_text(template(d, week, phase_label, milestone), encoding="utf-8")
        generated += 1

    print(f"Generated: {generated}")
    print(f"Skipped:   {skipped}")
    print(f"Output:    {OUT_DIR}")


if __name__ == "__main__":
    main()
