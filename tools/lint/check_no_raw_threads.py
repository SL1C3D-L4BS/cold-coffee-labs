#!/usr/bin/env python3
"""Enforce the OS-thread ownership boundary (ADR-0088).

`std::thread`, `std::async`, `std::packaged_task`, `std::promise`, and
`std::jthread` may appear only inside `engine/jobs/`. Platform thread APIs
(CreateThread, pthread_create) belong in `engine/platform/` if anywhere.

Mutexes, shared_mutex, atomics, condition_variable, latch, barrier are NOT
flagged — they are correctness primitives (see ADR-0088 §2).
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

SOURCE_ROOTS = ("engine", "editor", "runtime", "gameplay", "apps", "sandbox", "tests", "tools")
SKIP_PREFIXES = ("third_party/", "tests/fixtures/")
EXTENSIONS = (".hpp", ".cpp", ".h", ".c")

# Paths permitted to instantiate OS threads / std::async.
ALLOWED_PREFIXES = (
    "engine/jobs/",
    "engine/platform/",
    # tools/cook uses ReservedWorker, but cook_worker.cpp legitimately
    # spawns platform threads via the scheduler; audit allow-listed.
    "tools/cook/",
)

# Regexes guarded so comments / string literals are filtered first.
PATTERNS = (
    re.compile(r"\bstd::thread\s*[\(\{<\s]"),
    re.compile(r"\bstd::jthread\b"),
    re.compile(r"\bstd::async\s*\("),
    re.compile(r"\bstd::packaged_task\b"),
    re.compile(r"\bstd::promise\s*<"),
    re.compile(r"\bCreateThread\s*\("),
    re.compile(r"\bpthread_create\s*\("),
)

_LINE_COMMENT = re.compile(r"//.*$")
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_STRING = re.compile(r'"(?:\\.|[^"\\])*"')


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


def allowed(rel: pathlib.Path) -> bool:
    s = to_posix(rel)
    return any(s.startswith(p) for p in ALLOWED_PREFIXES)


def strip_noncode(text: str) -> str:
    text = _BLOCK_COMMENT.sub("", text)
    text = "\n".join(_LINE_COMMENT.sub("", line) for line in text.splitlines())
    text = _STRING.sub('""', text)
    return text


def detect(repo: pathlib.Path) -> list[tuple[str, int, str]]:
    hits: list[tuple[str, int, str]] = []
    for path in repo.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(repo)
        if not should_scan(rel):
            continue
        if allowed(rel):
            continue
        try:
            raw = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        cleaned = strip_noncode(raw)
        raw_lines = raw.splitlines()
        cleaned_lines = cleaned.splitlines()
        rel_posix = to_posix(rel)
        for idx, line in enumerate(cleaned_lines, start=1):
            for pat in PATTERNS:
                if pat.search(line):
                    original = raw_lines[idx - 1] if idx - 1 < len(raw_lines) else line
                    hits.append((rel_posix, idx, original.strip()))
                    break
    return hits


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        sample = "std::thread t([]{ });\n"
        cleaned = strip_noncode(sample)
        ok = any(p.search(cleaned) for p in PATTERNS)
        print("self-test:", "PASS" if ok else "FAIL")
        return 0 if ok else 1

    repo = pathlib.Path(args.repo_root).resolve()
    hits = detect(repo)
    if hits:
        print("OS-thread ownership violations (ADR-0088 \u00a71):")
        for rel, line, text in hits:
            print(f"  {rel}:{line}: {text}")
        print(f"\n{len(hits)} violation(s). Use engine/jobs/Scheduler or "
              "engine/jobs/ReservedWorker instead.")
        return 1
    print("Concurrency policy check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
