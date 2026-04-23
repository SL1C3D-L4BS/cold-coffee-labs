#!/usr/bin/env python3
"""tools/ai/ecc_dashboard.py — ECC status dashboard.

Reads the pinned state files produced by graphify + ECC install and
prints a single-screen summary so an operator can confirm, at a glance,
that the AI-tooling rail is healthy.

Zero third-party deps (stdlib only). Run from repo root:

    python tools/ai/ecc_dashboard.py
    python tools/ai/ecc_dashboard.py --json

Plan reference: ADR-0119 §Dashboard, phase-21-22 finish-line Track 6.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]


def _read_json(path: Path) -> dict | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def _git(*args: str) -> str:
    try:
        return subprocess.check_output(
            ["git", *args], cwd=REPO, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def _exists(path: Path) -> bool:
    return path.exists()


def _status_graphify() -> dict:
    graph = REPO / "graphify-out" / "graph.json"
    report = REPO / "graphify-out" / "GRAPH_REPORT.md"
    hook = REPO / ".git" / "hooks" / "post-commit"
    data = _read_json(graph) or {}
    nodes = data.get("nodes")
    # NetworkX JSON graph uses "links" for undirected edges.
    edges = data.get("edges") or data.get("links")
    return {
        "graph_json": _exists(graph),
        "graph_report": _exists(report),
        "post_commit_hook": _exists(hook),
        "nodes": len(nodes) if isinstance(nodes, list) else None,
        "edges": len(edges) if isinstance(edges, list) else None,
        "graph_mtime": (
            _dt.datetime.fromtimestamp(graph.stat().st_mtime).isoformat(timespec="seconds")
            if _exists(graph)
            else None
        ),
    }


def _status_ecc() -> dict:
    vendor = REPO / "third_party" / "everything-claude-code"
    state_user = Path.home() / ".claude" / "ecc" / "install-state.json"
    state_proj = REPO / ".cursor" / "ecc-install-state.json"
    s_user = _read_json(state_user) or {}
    s_proj = _read_json(state_proj) or {}
    version_file = vendor / "VERSION"
    version = version_file.read_text(encoding="utf-8").strip() if _exists(version_file) else None
    return {
        "vendored": _exists(vendor),
        "version": version,
        "user_state": _exists(state_user),
        "user_profile": s_user.get("profileId") or s_user.get("profile"),
        "user_modules": len(s_user.get("selectedModuleIds", []) or s_user.get("modules", []) or []),
        "project_state": _exists(state_proj),
        "project_profile": s_proj.get("profileId") or s_proj.get("profile"),
        "project_modules": len(s_proj.get("selectedModuleIds", []) or s_proj.get("modules", []) or []),
        "harness_dirs": {
            name: _exists(REPO / name)
            for name in (".codex", ".opencode", ".gemini", ".kiro", ".trae", ".agents")
        },
    }


def _status_mcp() -> dict:
    mcp = _read_json(REPO / ".cursor" / "mcp.json") or {}
    servers = mcp.get("mcpServers", {}) or {}
    env_refs = {}
    for name, cfg in servers.items():
        refs = []
        for v in (cfg.get("env") or {}).values():
            if isinstance(v, str) and v.startswith("${env:"):
                refs.append(v[len("${env:") : -1])
        env_refs[name] = refs
    return {
        "servers": list(servers.keys()),
        "env_refs": env_refs,
    }


def _status_agentshield() -> dict:
    cfg = _read_json(REPO / ".cursor" / "agentshield.json") or {}
    workflow = REPO / ".github" / "workflows" / "agentshield.yml"
    cli = REPO / "tools" / "ai" / "agentshield_cli.js"
    return {
        "policy_version": cfg.get("policy_version"),
        "blocked_paths": cfg.get("blocked_paths", []),
        "exempt_paths": cfg.get("exempt_paths_for_volume_caps", []),
        "workflow_present": _exists(workflow),
        "local_cli_present": _exists(cli),
        "hard_fail": False if not _exists(workflow) else "advisory" not in workflow.read_text(encoding="utf-8")[:4096],
    }


def _status_git() -> dict:
    branch = _git("rev-parse", "--abbrev-ref", "HEAD")
    sha = _git("rev-parse", "--short", "HEAD")
    dirty = bool(_git("status", "--porcelain"))
    return {"branch": branch, "head": sha, "dirty": dirty}


def _emit_json(payload: dict) -> None:
    json.dump(payload, sys.stdout, indent=2, default=str)
    print()


def _emit_human(payload: dict) -> None:
    def hdr(txt: str) -> None:
        print(f"\n=== {txt} ===")

    print("ECC Dashboard (phase-21-22 finish line)")
    print(f"Repo:   {REPO}")
    print(f"Branch: {payload['git']['branch']} @ {payload['git']['head']} "
          f"{'[dirty]' if payload['git']['dirty'] else '[clean]'}")

    hdr("Graphify (ADR-0118)")
    g = payload["graphify"]
    print(f"  graph.json        : {'ok' if g['graph_json'] else 'MISSING'} "
          f"({g['nodes']} nodes, {g['edges']} edges)")
    print(f"  GRAPH_REPORT.md   : {'ok' if g['graph_report'] else 'MISSING'}")
    print(f"  .git/hooks/post-commit : {'ok' if g['post_commit_hook'] else 'MISSING'}")
    print(f"  last build        : {g['graph_mtime'] or '-'}")

    hdr("ECC (ADR-0119)")
    e = payload["ecc"]
    print(f"  vendor            : {'ok' if e['vendored'] else 'MISSING'} (v{e['version']})")
    print(f"  user state        : {'ok' if e['user_state'] else 'MISSING'} "
          f"profile={e['user_profile']} modules={e['user_modules']}")
    print(f"  project state     : {'ok' if e['project_state'] else 'MISSING'} "
          f"profile={e['project_profile']} modules={e['project_modules']}")
    dirs = " ".join(
        f"{name}={'+' if ok else '-'}" for name, ok in e["harness_dirs"].items()
    )
    print(f"  harness adapters  : {dirs}")

    hdr("MCP servers (.cursor/mcp.json)")
    m = payload["mcp"]
    print(f"  servers ({len(m['servers'])}): {', '.join(m['servers']) or '-'}")
    for name, refs in m["env_refs"].items():
        if refs:
            print(f"    {name}: needs env {', '.join(refs)}")

    hdr("AgentShield (ADR-0120)")
    a = payload["agentshield"]
    print(f"  policy_version    : {a['policy_version']}")
    print(f"  workflow          : {'ok' if a['workflow_present'] else 'MISSING'} "
          f"({'hard-fail' if a['hard_fail'] else 'advisory'})")
    print(f"  local CLI         : {'ok' if a['local_cli_present'] else 'MISSING'}")
    print(f"  blocked_paths     : {len(a['blocked_paths'])} globs")
    print(f"  exempt_paths      : {len(a['exempt_paths'])} globs")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()

    payload = {
        "git": _status_git(),
        "graphify": _status_graphify(),
        "ecc": _status_ecc(),
        "mcp": _status_mcp(),
        "agentshield": _status_agentshield(),
    }

    if args.json:
        _emit_json(payload)
    else:
        _emit_human(payload)
    return 0


if __name__ == "__main__":
    sys.exit(main())
