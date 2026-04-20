#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import sys


FORBIDDEN_PATTERNS = [
    re.compile(r'^\s*#\s*include\s*<windows\.h>', re.IGNORECASE),
    re.compile(r'^\s*#\s*include\s*<winuser\.h>', re.IGNORECASE),
    re.compile(r'^\s*#\s*include\s*<dlfcn\.h>', re.IGNORECASE),
    re.compile(r'^\s*#\s*include\s*<unistd\.h>', re.IGNORECASE),
    re.compile(r'^\s*#\s*include\s*<sys/[^>]+>', re.IGNORECASE),
]

SOURCE_DIRS = ("engine", "editor", "sandbox", "runtime", "gameplay", "tools")
ALLOWED_PREFIXES = ("engine/platform/",)
SKIP_PREFIXES = ("third_party/", "tests/fixtures/")
EXTENSIONS = (".hpp", ".cpp", ".h", ".c")


def posix(path: pathlib.Path) -> str:
    return path.as_posix()


def should_scan(rel: pathlib.Path) -> bool:
    rel_str = posix(rel)
    if not rel_str.startswith(SOURCE_DIRS):
        return False
    if rel.suffix.lower() not in EXTENSIONS:
        return False
    for prefix in SKIP_PREFIXES:
        if rel_str.startswith(prefix):
            return False
    return True


def allowed_path(rel: pathlib.Path) -> bool:
    rel_str = posix(rel)
    return any(rel_str.startswith(prefix) for prefix in ALLOWED_PREFIXES)


def detect_violations(repo_root: pathlib.Path) -> list[tuple[pathlib.Path, int, str]]:
    violations: list[tuple[pathlib.Path, int, str]] = []
    for path in repo_root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(repo_root)
        if not should_scan(rel) or allowed_path(rel):
            continue
        with path.open("r", encoding="utf-8", errors="ignore") as handle:
            for idx, line in enumerate(handle, start=1):
                if any(pattern.search(line) for pattern in FORBIDDEN_PATTERNS):
                    violations.append((rel, idx, line.strip()))
    return violations


def self_test(repo_root: pathlib.Path, fixture: pathlib.Path) -> int:
    target = (repo_root / fixture).resolve()
    if not target.exists():
        print(f"fixture not found: {fixture}", file=sys.stderr)
        return 2
    with target.open("r", encoding="utf-8", errors="ignore") as handle:
        hit = any(any(pattern.search(line) for pattern in FORBIDDEN_PATTERNS) for line in handle)
    if not hit:
        print(f"fixture did not trigger detector: {fixture}", file=sys.stderr)
        return 1
    print(f"fixture check passed: {fixture}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Enforce OS headers remain inside engine/platform boundaries."
    )
    parser.add_argument("--repo-root", default=".", help="Path to repository root.")
    parser.add_argument("--self-test", action="store_true", help="Run fixture detector self-test.")
    parser.add_argument(
        "--fixture",
        default="tests/fixtures/os_header_violation.cpp",
        help="Fixture path for --self-test.",
    )
    args = parser.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    if args.self_test:
        return self_test(repo_root, pathlib.Path(args.fixture))

    violations = detect_violations(repo_root)
    if not violations:
        print("OS-header boundary check passed.")
        return 0

    print("OS-header boundary violations detected:")
    for rel, line, text in violations:
        print(f"  {rel}:{line}: {text}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
