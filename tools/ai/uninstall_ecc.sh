#!/usr/bin/env bash
# tools/ai/uninstall_ecc.sh — reverse ECC user-global + project installs.
# Wraps third_party/everything-claude-code/scripts/uninstall.js (ADR-0119).
#
# DRY_RUN=1 ./tools/ai/uninstall_ecc.sh  — dry run only

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"
UNINSTALL_JS="$REPO_ROOT/third_party/everything-claude-code/scripts/uninstall.js"

if [[ ! -f "$UNINSTALL_JS" ]]; then
  echo "ERROR: missing $UNINSTALL_JS" >&2
  exit 1
fi

args=(--target claude --target cursor)
if [[ "${DRY_RUN:-}" == "1" ]]; then
  args+=(--dry-run)
  echo "[uninstall_ecc] DRY_RUN=1"
fi

echo "[uninstall_ecc] node ... uninstall.js ${args[*]}"
exec node "$UNINSTALL_JS" "${args[@]}"
