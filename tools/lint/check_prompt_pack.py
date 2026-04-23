#!/usr/bin/env python3
# tools/lint/check_prompt_pack.py - CI lint asserting docs/prompts/ is
# consistent with docs/prompts/pack.yml.
#
# Thin wrapper around tools/prompts/render_pack.py --check so the lint
# path exactly matches what a contributor would run locally.
from __future__ import annotations

import pathlib
import subprocess
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
RENDERER  = REPO_ROOT / "tools" / "prompts" / "render_pack.py"


def main() -> int:
    if not RENDERER.exists():
        print(f"ERROR: renderer missing at {RENDERER}", file=sys.stderr)
        return 2
    return subprocess.call([sys.executable, str(RENDERER), "--check"])


if __name__ == "__main__":
    sys.exit(main())
