# tools/ai/uninstall_ecc.ps1 — reverse ECC user-global + project installs.
# Wraps third_party/everything-claude-code/scripts/uninstall.js (ADR-0119).
#
# Usage:
#   pwsh tools/ai/uninstall_ecc.ps1
#   pwsh tools/ai/uninstall_ecc.ps1 -DryRun

[CmdletBinding()]
param(
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$uninstall = Join-Path $repoRoot 'third_party/everything-claude-code/scripts/uninstall.js'

if (-not (Test-Path $uninstall)) {
    throw "Missing $uninstall — vendor ECC must be present."
}

Push-Location $repoRoot
try {
    if ($DryRun) {
        Write-Host '[uninstall_ecc] node ... uninstall.js --target claude --target cursor --dry-run' -ForegroundColor Cyan
        & node $uninstall --target claude --target cursor --dry-run
    } else {
        Write-Host '[uninstall_ecc] node ... uninstall.js --target claude --target cursor' -ForegroundColor Cyan
        & node $uninstall --target claude --target cursor
    }
} finally {
    Pop-Location
}
