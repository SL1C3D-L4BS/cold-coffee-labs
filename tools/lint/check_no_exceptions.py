#!/usr/bin/env python3
"""Enforce the constitutional "no exceptions" policy (docs/01 §2.2, §3).

The long-term goal (ADR-0086 in docs/10) is zero try/throw/catch in engine,
runtime, editor, and gameplay code. Today the codebase carries historical
debt inherited from early-phase HAL / persist / parser work. This lint:

* Scans every C++ TU under the production roots.
* Fails the build on ANY new try/throw/catch outside the explicit
  ALLOW_LIST below.
* Is intended to RATCHET: the ALLOW_LIST should shrink phase by phase,
  never grow. Adding to it requires an ADR revision.

Run locally:  python tools/lint/check_no_exceptions.py
Self-test:    python tools/lint/check_no_exceptions.py --self-test
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys


SOURCE_ROOTS = ("engine", "editor", "runtime", "gameplay", "apps", "sandbox")
SKIP_PREFIXES = ("third_party/", "tests/fixtures/")
EXTENSIONS = (".hpp", ".cpp", ".h", ".c")

# Matches bare keywords that look like real C++ exception machinery.
# Deliberately conservative: we require the keyword followed by whitespace
# and either an identifier-or-literal (throw), an opening paren (catch), or
# an opening brace (try). Comments/strings are filtered in detect_offenses.
PATTERNS = (
    re.compile(r"\bthrow\s+[A-Za-z_0-9:\"']"),
    re.compile(r"\btry\s*\{"),
    re.compile(r"\bcatch\s*\("),
)

# Baseline of historical offenders. Shrink this list, never grow it.
# Every entry is technical debt tracked by ADR-0086 in docs/10.
ALLOW_LIST = frozenset({
    # engine/core — Result.value()/error() misuse would be the worst
    # offender and is fixed; no entries needed here.

    # engine/ecs — migration + archetype allocation debt (Phase 3/8)
    "engine/ecs/world.cpp",
    "engine/ecs/world.hpp",
    "engine/ecs/archetype.cpp",
    "engine/ecs/component_registry.cpp",

    # engine/memory — allocator out-of-memory paths (Phase 2)
    "engine/memory/pool_allocator.cpp",
    "engine/memory/arena_allocator.cpp",
    "engine/memory/freelist_allocator.cpp",

    # engine/render/hal — Vulkan HAL error paths (Phase 4)
    "engine/render/hal/vulkan_device.cpp",
    "engine/render/hal/vulkan_instance.cpp",
    "engine/render/hal/vulkan_buffer.cpp",
    "engine/render/hal/vulkan_command_buffer.cpp",
    "engine/render/hal/vulkan_descriptor.cpp",
    "engine/render/hal/vulkan_queue.cpp",
    "engine/render/hal/vulkan_swapchain.cpp",

    # engine/persist — SQLite + cloud backend (Phase 15)
    "engine/persist/impl/sqlite_store.cpp",
    "engine/persist/impl/local_fs_backend.cpp",
    "engine/persist/console_commands.cpp",
    "engine/persist/cloud_save.cpp",

    # engine/telemetry — DSAR export (Phase 15)
    "engine/telemetry/dsar_exporter.cpp",

    # engine/net — session + cvars + console (Phase 14)
    "engine/net/console_commands.cpp",
    "engine/net/net_cvars.cpp",
    "engine/net/network_world.cpp",

    # engine/physics — cvars + console (Phase 12)
    "engine/physics/physics_world.cpp",
    "engine/physics/physics_cvars.cpp",
    "engine/physics/console_commands.cpp",

    # engine/audio — mixer (Phase 10)
    "engine/audio/audio_mixer_system.cpp",

    # engine/input — TOML parser narrowing casts (Phase 10)
    "engine/input/action_map_toml.cpp",

    # engine/i18n — XLIFF parser (Phase 16)
    "engine/i18n/xliff.cpp",

    # engine/anim + gameai — console commands (Phase 13)
    "engine/anim/console_commands.cpp",
    "engine/gameai/console_commands.cpp",

    # engine/vscript — IR parser (Phase 8)
    "engine/vscript/parser.cpp",

    # Top-level entry points — catch-all at process boundary is OK
    "runtime/main.cpp",
    "editor/vscript/vscript_panel.cpp",
})


def to_posix(p: pathlib.Path) -> str:
    return p.as_posix()


def should_scan(rel: pathlib.Path) -> bool:
    s = to_posix(rel)
    if not s.startswith(SOURCE_ROOTS):
        return False
    if rel.suffix.lower() not in EXTENSIONS:
        return False
    for skip in SKIP_PREFIXES:
        if s.startswith(skip):
            return False
    return True


_LINE_COMMENT = re.compile(r"//.*$")
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_STRING = re.compile(r'"(?:\\.|[^"\\])*"')


def strip_noncode(text: str) -> str:
    text = _BLOCK_COMMENT.sub("", text)
    text = "\n".join(_LINE_COMMENT.sub("", line) for line in text.splitlines())
    text = _STRING.sub('""', text)
    return text


def detect_offenses(repo: pathlib.Path) -> list[tuple[str, int, str]]:
    hits: list[tuple[str, int, str]] = []
    for path in repo.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(repo)
        if not should_scan(rel):
            continue
        rel_posix = to_posix(rel)
        if rel_posix in ALLOW_LIST:
            continue
        try:
            raw = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        cleaned = strip_noncode(raw)
        raw_lines = raw.splitlines()
        cleaned_lines = cleaned.splitlines()
        for idx, line in enumerate(cleaned_lines, start=1):
            for pat in PATTERNS:
                if pat.search(line):
                    # Report the ORIGINAL line for readability.
                    original = raw_lines[idx - 1] if idx - 1 < len(raw_lines) else line
                    hits.append((rel_posix, idx, original.strip()))
                    break
    return hits


def verify_allow_list(repo: pathlib.Path) -> list[str]:
    """Warn if an allow-list entry no longer contains offenses (can be removed)."""
    stale: list[str] = []
    for rel_posix in sorted(ALLOW_LIST):
        p = repo / rel_posix
        if not p.exists():
            stale.append(f"  {rel_posix}  (file missing)")
            continue
        text = strip_noncode(p.read_text(encoding="utf-8", errors="ignore"))
        if not any(pat.search(text) for pat in PATTERNS):
            stale.append(f"  {rel_posix}  (no throws/try/catch remain)")
    return stale


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--show-stale", action="store_true",
                        help="Report allow-list entries that can now be removed.")
    args = parser.parse_args()

    repo = pathlib.Path(args.repo_root).resolve()

    if args.self_test:
        sample = "throw std::runtime_error(\"x\");\n"
        cleaned = strip_noncode(sample)
        ok = any(p.search(cleaned) for p in PATTERNS)
        print("self-test:", "PASS" if ok else "FAIL")
        return 0 if ok else 1

    hits = detect_offenses(repo)
    if hits:
        print("Exception-policy violations detected "
              "(docs/01 \u00a72.2, ADR-0086):")
        for rel, line, text in hits:
            print(f"  {rel}:{line}: {text}")
        print(f"\n{len(hits)} violation(s). See ADR-0086 for the allow-list "
              "and the burn-down plan.")
        return 1

    if args.show_stale:
        stale = verify_allow_list(repo)
        if stale:
            print("Allow-list entries that appear clean and can be removed:")
            for s in stale:
                print(s)

    print("Exception-policy check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
