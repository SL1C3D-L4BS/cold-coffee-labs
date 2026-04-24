#!/usr/bin/env python3
"""Quick WCAG 2.1 contrast check for hex pairs (editor palette regression)."""
from __future__ import annotations

import sys


def srgb_to_lin(c: float) -> float:
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def rel_luminance(rgb: tuple[int, int, int]) -> float:
    r, g, b = (srgb_to_lin(x) for x in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(fg: tuple[int, int, int], bg: tuple[int, int, int]) -> float:
    l1 = rel_luminance(fg) + 0.05
    l2 = rel_luminance(bg) + 0.05
    return max(l1, l2) / min(l1, l2)


def parse_hex(s: str) -> tuple[int, int, int]:
    s = s.strip().lstrip("#")
    if len(s) == 6:
        return int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)
    raise ValueError(s)


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: wcag_relative_luminance.py #RRGGBB #RRGGBB", file=sys.stderr)
        return 2
    fg = parse_hex(sys.argv[1])
    bg = parse_hex(sys.argv[2])
    r = contrast_ratio(fg, bg)
    print(f"Contrast ratio: {r:.2f}:1")
    print("  AA body (4.5):", "pass" if r >= 4.5 else "fail")
    print("  AA large/UI (3.0):", "pass" if r >= 3.0 else "fail")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
