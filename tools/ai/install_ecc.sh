#!/usr/bin/env bash
# tools/ai/install_ecc.sh — idempotent AI-tooling install (graphify + ECC + ccg).
# See install_ecc.ps1 for the full contract; ADR-0119.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

SKIP_GRAPHIFY=0
SKIP_ECC=0
SKIP_HOOKS=0
DRY_RUN=0
for arg in "$@"; do
  case "$arg" in
    --skip-graphify) SKIP_GRAPHIFY=1 ;;
    --skip-ecc) SKIP_ECC=1 ;;
    --skip-hooks) SKIP_HOOKS=1 ;;
    --dry-run) DRY_RUN=1 ;;
  esac
done

EXPECTED_VERSION="1.10.0"
VENDOR="third_party/everything-claude-code"
VERSION_FILE="$VENDOR/VERSION"

die() { echo "[install_ecc] ERROR: $*" >&2; exit 1; }

echo "[install_ecc] repo root: $REPO_ROOT"

if [[ ! -f "$VERSION_FILE" ]]; then
  die "missing $VERSION_FILE — run from a full clone with ECC vendored (C2)."
fi
have=$(tr -d '\r\n' <"$VERSION_FILE")
if [[ "$have" != "$EXPECTED_VERSION" ]]; then
  die "vendor VERSION is '$have' (expected $EXPECTED_VERSION). Re-vendor ECC or fix ADR-0119 pin."
fi

run() {
  echo "[install_ecc] $1"
  shift
  if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "    (dry-run) $*"
    return 0
  fi
  "$@"
}

if [[ "$SKIP_GRAPHIFY" -eq 0 ]]; then
  if ! command -v python3 >/dev/null 2>&1; then
    echo "[install_ecc] WARN: python3 not on PATH; skipping graphify (install Python 3.12+)." >&2
  else
    run "pipx / graphify" bash -c 'python3 -m pip install --user --quiet pipx && python3 -m pipx ensurepath >/dev/null 2>&1 || true'
    run "pipx install graphifyy" python3 -m pipx install --force "graphifyy==0.4.31"
    export PATH="${HOME}/.local/bin:${PATH}"
    if command -v graphify >/dev/null 2>&1; then
      run "graphify update" graphify update .
      if [[ "$SKIP_HOOKS" -eq 0 ]]; then
        run "graphify hook install" graphify hook install
      fi
    else
      echo "[install_ecc] WARN: graphify not found after pipx install; check PATH (~/.local/bin)." >&2
    fi
  fi
fi

if [[ "$SKIP_ECC" -eq 0 ]]; then
  run "npm install (vendor)" bash -c "cd \"$VENDOR\" && npm install --no-audit --no-fund --loglevel=error"
  run "ECC install-apply claude" node "$VENDOR/scripts/install-apply.js" --target claude --profile full
  run "ECC install-apply cursor" node "$VENDOR/scripts/install-apply.js" --target cursor --profile full
  run "ccg-workflow init (headless)" npx -y ccg-workflow init --lang en --skip-prompt --skip-mcp
fi

if command -v python3 >/dev/null 2>&1; then
  run "dashboard" python3 tools/ai/ecc_dashboard.py
else
  echo "[install_ecc] WARN: python3 not found; skipping ecc_dashboard.py" >&2
fi
