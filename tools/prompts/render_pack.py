#!/usr/bin/env python3
# tools/prompts/render_pack.py — render docs/prompts/pack.yml into one
# markdown file per task.
#
# Usage:
#   python tools/prompts/render_pack.py            # render
#   python tools/prompts/render_pack.py --check    # fail if render is stale
#
# Buckets:
#   kind: preflight    -> docs/prompts/preflight/<id>.md
#   kind: phase-week   -> docs/prompts/phase<NN>/<id>.md (phase = task.phase)
#   kind: ai-tooling   -> docs/prompts/ai_tooling/<id>.md
#   kind: ci           -> docs/prompts/ci/<id>.md
#   kind: docs         -> docs/prompts/docs/<id>.md
#
# The rendered markdown contains a front-matter + prompt body so an agent
# (or human operator) can copy-paste it directly into Cursor.
from __future__ import annotations

import argparse
import hashlib
import pathlib
import shutil
import sys
from typing import Any

try:
    import yaml
except ImportError:  # pragma: no cover
    print("ERROR: PyYAML missing. Install with: pipx install pyyaml "
          "or pip install pyyaml", file=sys.stderr)
    sys.exit(2)

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
PACK_PATH = REPO_ROOT / "docs" / "prompts" / "pack.yml"
RENDER_ROOT = REPO_ROOT / "docs" / "prompts"

BUCKET_FOR_KIND = {
    "preflight": "preflight",
    "ai-tooling": "ai_tooling",
    "ci": "ci",
    "docs": "docs",
}


def bucket_for(task: dict[str, Any]) -> pathlib.Path:
    kind = task["kind"]
    if kind == "phase-week":
        phase = task.get("phase")
        if phase is None:
            raise ValueError(f"phase-week task {task['id']} missing phase")
        return RENDER_ROOT / f"phase{int(phase):02d}"
    if kind in BUCKET_FOR_KIND:
        return RENDER_ROOT / BUCKET_FOR_KIND[kind]
    raise ValueError(f"unknown kind {kind} in task {task['id']}")


def render_task(task: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append(f"# {task['id']} — {task.get('title', '')}")
    lines.append("")
    lines.append("<!-- Rendered from docs/prompts/pack.yml — do NOT edit by hand. -->")
    lines.append("")
    lines.append("| Field | Value |")
    lines.append("|-------|-------|")
    lines.append(f"| Kind | `{task['kind']}` |")
    if task.get("phase") is not None:
        lines.append(f"| Phase | {task['phase']} |")
    if task.get("week") is not None:
        lines.append(f"| Week | {task['week']} |")
    if task.get("tier"):
        lines.append(f"| Tier | {task['tier']} |")
    depends = task.get("depends_on") or []
    if depends:
        lines.append(f"| Depends on | {', '.join(f'`{d}`' for d in depends)} |")
    else:
        lines.append("| Depends on | *(none)* |")
    lines.append("")

    inputs = task.get("inputs") or []
    if inputs:
        lines.append("## Inputs")
        lines.append("")
        for p in inputs:
            lines.append(f"- `{p}`")
        lines.append("")

    writes = task.get("writes") or []
    if writes:
        lines.append("## Writes")
        lines.append("")
        for p in writes:
            lines.append(f"- `{p}`")
        lines.append("")

    exits = task.get("exit_criteria") or []
    if exits:
        lines.append("## Exit criteria")
        lines.append("")
        for e in exits:
            lines.append(f"- [ ] {e}")
        lines.append("")

    prompt = task.get("prompt", "").rstrip()
    lines.append("## Prompt")
    lines.append("")
    lines.append(prompt)
    lines.append("")
    return "\n".join(lines)


def load_pack() -> dict[str, Any]:
    if not PACK_PATH.exists():
        raise FileNotFoundError(f"pack not found at {PACK_PATH}")
    with PACK_PATH.open("r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    if not isinstance(data, dict) or "tasks" not in data:
        raise ValueError("pack.yml missing top-level 'tasks'")
    return data


def collect_renders(pack: dict[str, Any]) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}
    seen_ids: set[str] = set()
    for task in pack["tasks"]:
        tid = task["id"]
        if tid in seen_ids:
            raise ValueError(f"duplicate task id {tid}")
        seen_ids.add(tid)
        target_dir = bucket_for(task)
        out[target_dir / f"{tid}.md"] = render_task(task)
    return out


def validate_dag(pack: dict[str, Any]) -> None:
    ids = {t["id"] for t in pack["tasks"]}
    for t in pack["tasks"]:
        for d in t.get("depends_on") or []:
            if d not in ids:
                raise ValueError(
                    f"task {t['id']} depends on unknown id {d}"
                )


def digest(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def write_renders(renders: dict[pathlib.Path, str]) -> list[pathlib.Path]:
    # Clean bucket dirs we control (preflight / phase* / ai_tooling / ci / docs)
    managed = {"preflight", "ai_tooling", "ci", "docs"}
    for p in RENDER_ROOT.iterdir():
        if p.is_dir() and (p.name in managed or p.name.startswith("phase")):
            shutil.rmtree(p)

    written: list[pathlib.Path] = []
    for path, content in sorted(renders.items()):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
        written.append(path)
    return written


def check_renders(renders: dict[pathlib.Path, str]) -> int:
    stale: list[pathlib.Path] = []
    for path, content in renders.items():
        if not path.exists():
            stale.append(path)
            continue
        on_disk = path.read_text(encoding="utf-8")
        if digest(on_disk.replace("\r\n", "\n")) != digest(content):
            stale.append(path)
    if stale:
        print("Stale rendered prompt files (run `python tools/prompts/render_pack.py`):",
              file=sys.stderr)
        for p in stale:
            print(f"  {p.relative_to(REPO_ROOT)}", file=sys.stderr)
        return 1
    print(f"{len(renders)} rendered prompt files are up-to-date.")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Render pack.yml.")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail if the rendered markdown is stale.",
    )
    args = parser.parse_args(argv)

    pack = load_pack()
    validate_dag(pack)
    renders = collect_renders(pack)

    if args.check:
        return check_renders(renders)

    written = write_renders(renders)
    print(f"Rendered {len(written)} prompt files.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
