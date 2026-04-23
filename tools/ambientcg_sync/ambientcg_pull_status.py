#!/usr/bin/env python3
"""
Verifiable AmbientCG pull state: no trust required.

Reads the same CSV filter as ambientcg_downloader_main.py, compares to on-disk zips
and unzipped dirs, reports pull.log freshness, and counts python.exe processes.

Run anytime from repo root:
  python tools/ambientcg_sync/ambientcg_pull_status.py
"""
from __future__ import annotations

import argparse
import csv
import io
import subprocess
import sys
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


def matching_rows_from_csv(resolution: str, file_type: str) -> list[dict]:
    target_attr = f"{resolution}-{file_type}"
    url = "https://ambientcg.com/api/v2/downloads_csv"
    req = urllib.request.Request(url, headers={"User-Agent": "Greywater/ambientcg_pull_status"})
    with urllib.request.urlopen(req, timeout=120) as resp:
        text = io.TextIOWrapper(resp, encoding="utf-8")
        rows = []
        for row in csv.DictReader(text):
            attr = row["downloadAttribute"]
            if attr != target_attr and not (file_type == "JPG" and attr == resolution):
                continue
            rows.append(row)
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description="Verify AmbientCG pull progress with measurable facts.")
    ap.add_argument("--content-root", type=Path, default=None)
    args = ap.parse_args()
    here = Path(__file__).resolve().parent
    root = args.content_root or (here / "../..").resolve()
    clone = root / "content" / "_download_cache" / "ambientcg_downloader"
    cfg_path = clone / "config.yaml"
    downloads = clone / "downloads"
    unzipped = clone / "unzipped"
    log_path = clone / "pull.log"

    print("=== AmbientCG pull - verification report ===")
    print(f"time_utc: {datetime.now(timezone.utc).isoformat()}")
    print(f"clone_dir: {clone}")
    print(f"clone_dir_exists: {clone.is_dir()}")

    if not cfg_path.is_file():
        print("config.yaml: MISSING (clone not set up?)")
        return 1

    cfg_text = cfg_path.read_text(encoding="utf-8")
    try:
        import yaml  # type: ignore

        cfg = yaml.safe_load(cfg_text)
        resolution = str(cfg.get("resolution", ""))
        file_type = str(cfg.get("file_type", ""))
    except Exception:
        import re

        rm = re.search(r'resolution:\s*"([^"]+)"', cfg_text)
        fm = re.search(r'file_type:\s*"([^"]+)"', cfg_text)
        resolution = rm.group(1) if rm else ""
        file_type = fm.group(1) if fm else ""
    print(f"config: resolution={resolution} file_type={file_type}")

    try:
        expected = matching_rows_from_csv(resolution, file_type)
    except Exception as e:
        print(f"csv_fetch: FAILED {e}")
        return 1

    n_exp = len(expected)
    expected_names = set()
    for row in expected:
        link = row["downloadLink"]
        name = link.split("file=")[-1]
        expected_names.add(name)

    zip_paths = list(downloads.glob("*.zip")) if downloads.is_dir() else []
    zip_names = {p.name for p in zip_paths}
    n_zip = len(zip_names)
    n_unz = len([p for p in unzipped.iterdir() if p.is_dir()]) if unzipped.is_dir() else 0

    on_disk_in_scope = zip_names & expected_names
    n_match = len(on_disk_in_scope)
    orphan_zips = zip_names - expected_names

    print(f"csv_rows_matching_filter: {n_exp}")
    print(f"downloads_zip_count: {n_zip}")
    print(f"downloads_zip_in_csv_scope: {n_match}")
    if orphan_zips:
        sample = sorted(orphan_zips)[:5]
        print(f"downloads_zip_out_of_scope_sample: {sample}")

    print(f"unzipped_dir_count: {n_unz}")

    if n_exp:
        pct = 100.0 * n_match / n_exp
        print(f"download_phase_progress_by_name: {n_match}/{n_exp} ({pct:.1f}% of expected zip names present)")

    if log_path.is_file():
        st = log_path.stat()
        age_s = max(0, int(datetime.now(timezone.utc).timestamp() - st.st_mtime))
        print(f"pull_log: {log_path}")
        print(f"pull_log_bytes: {st.st_size}")
        print(f"pull_log_mtime_utc: {datetime.fromtimestamp(st.st_mtime, tz=timezone.utc).isoformat()}")
        print(f"pull_log_age_seconds: {age_s}")
        tail = log_path.read_text(encoding="utf-8", errors="replace").strip().splitlines()[-5:]
        print("pull_log_last_lines:")
        for line in tail:
            print(f"  | {line}")
    else:
        print("pull_log: MISSING")

    # Observable process count (Windows-friendly).
    try:
        if sys.platform == "win32":
            r = subprocess.run(
                ["tasklist", "/FI", "IMAGENAME eq python.exe", "/FO", "CSV", "/NH"],
                capture_output=True,
                text=True,
                timeout=30,
            )
            lines = [ln for ln in r.stdout.strip().splitlines() if "python.exe" in ln.lower()]
            print(f"tasklist_python_exe_rows: {len(lines)}")
        else:
            r = subprocess.run(["pgrep", "-c", "python"], capture_output=True, text=True, timeout=10)
            print(f"pgrep_python_count_stdout: {r.stdout.strip()}")
    except Exception as e:
        print(f"process_scan: skipped ({e})")

    print("=== end ===")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
