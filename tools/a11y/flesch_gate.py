#!/usr/bin/env python3
"""docs/a11y_phase15_selfcheck.md — rough Flesch Reading Ease gate (English prose).

Usage: python tools/a11y/flesch_gate.py path/to/strings.txt [--min 60]
"""

from __future__ import annotations

import argparse
import re
import sys


def count_syllables(word: str) -> int:
    w = re.sub(r"[^a-z]", "", word.lower())
    if not w:
        return 0
    vowels = len(re.findall(r"[aeiouy]+", w))
    return max(1, vowels)


def flesch_reading_ease(text: str) -> float:
    words = re.findall(r"[A-Za-z']+", text)
    sentences = re.split(r"[.!?]+", text)
    sentences = [s for s in sentences if s.strip()]
    if not words or not sentences:
        return 0.0
    syllables = sum(count_syllables(w) for w in words)
    w = len(words)
    s = len(sentences)
    return 206.835 - 1.015 * (w / s) - 84.6 * (syllables / w)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("path")
    p.add_argument("--min", type=float, default=60.0)
    args = p.parse_args()
    try:
        text = open(args.path, encoding="utf-8").read()
    except OSError as e:
        print(e, file=sys.stderr)
        return 2
    score = flesch_reading_ease(text)
    print(f"Flesch Reading Ease: {score:.1f} (min {args.min})")
    return 0 if score >= args.min else 1


if __name__ == "__main__":
    raise SystemExit(main())
