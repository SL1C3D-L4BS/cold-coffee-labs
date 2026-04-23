# tools/ai/install_ecc.ps1 - idempotent re-runner for the AI-tooling rail.
#
# Applies every step of the phase-21-22 finish-line plan (C1..C4) to a
# fresh checkout or a stale workstation:
#
#   1. Installs / upgrades graphifyy (pinned to tools/ai/requirements.txt).
#   2. Runs `graphify update .` + `graphify hook install`.
#   3. npm installs the vendored ECC tree (third_party/everything-claude-code).
#   4. Runs ECC install-apply.js with `--target claude --profile full`
#      (user-global) and `--target cursor --profile full` (project).
#   5. Runs `npx ccg-workflow init` in headless mode (skip MCP prompts).
#   6. Prints the ECC dashboard summary at the end.
#
# Safe to re-run. Writes nothing outside:
#   - $env:USERPROFILE\.local\bin (graphify.exe)
#   - $env:USERPROFILE\pipx\venvs\graphifyy (pipx venv)
#   - $env:USERPROFILE\.claude\*             (ECC user-global)
#   - $repoRoot\.cursor\* and vendored node_modules only.
#
# Companion uninstall: tools/ai/uninstall_ecc.ps1.
# Referenced by: docs/10_APPENDIX_ADRS_AND_REFERENCES.md ADR-0119.

[CmdletBinding()]
param(
    [switch]$SkipGraphify,
    [switch]$SkipECC,
    [switch]$SkipHooks,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ExpectedVendorVersion = '1.10.0'
$vendorRoot = Join-Path $repoRoot 'third_party/everything-claude-code'
$versionFile = Join-Path $vendorRoot 'VERSION'
if (-not (Test-Path $versionFile)) {
    throw "Missing $versionFile — ECC must be vendored (see ADR-0119 / C2)."
}
$haveV = (Get-Content $versionFile -Raw).Trim()
if ($haveV -ne $ExpectedVendorVersion) {
    throw "Vendor VERSION is '$haveV' (expected $ExpectedVendorVersion). Re-vendor or update the pin."
}

Push-Location $repoRoot
try {
    Write-Host "[install_ecc] repo root: $repoRoot"

    function Invoke-Step {
        param([string]$Label, [scriptblock]$Body)
        Write-Host "[install_ecc] $Label" -ForegroundColor Cyan
        if ($DryRun) {
            Write-Host "    (dry-run)"
            return
        }
        & $Body
    }

    if (-not $SkipGraphify) {
        Invoke-Step "1/4 graphify install (pipx)" {
            $pyExe = "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe"
            if (-not (Test-Path $pyExe)) {
                Write-Warning "Python 3.12 not found at $pyExe. Install via winget install Python.Python.3.12"
                return
            }
            & $pyExe -m pip install --user --quiet pipx
            & $pyExe -m pipx install --force "graphifyy==0.4.31"
        }

        Invoke-Step "2/4 graphify update + hook install" {
            $graphifyExe = "$env:USERPROFILE\.local\bin\graphify.exe"
            if (Test-Path $graphifyExe) {
                & $graphifyExe update . | Out-Host
                if (-not $SkipHooks) { & $graphifyExe hook install | Out-Host }
            } else {
                Write-Warning "graphify.exe not found after install."
            }
        }
    }

    if (-not $SkipECC) {
        Invoke-Step "3/5 ECC npm install" {
            Push-Location third_party/everything-claude-code
            try {
                & npm install --no-audit --no-fund --loglevel=error
            } finally { Pop-Location }
        }

        Invoke-Step "4/5 ECC install-apply.js (user + project scope)" {
            & node third_party/everything-claude-code/scripts/install-apply.js `
                --target claude --profile full | Out-Host
            & node third_party/everything-claude-code/scripts/install-apply.js `
                --target cursor --profile full | Out-Host
        }

        Invoke-Step "5/5 ccg-workflow init (headless)" {
            & npx -y ccg-workflow init --lang en --skip-prompt --skip-mcp | Out-Host
        }
    }

    Invoke-Step "dashboard" {
        $pyExe = "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe"
        if (Test-Path $pyExe) {
            & $pyExe tools/ai/ecc_dashboard.py
        } else {
            Write-Warning "Python not found; skipping dashboard."
        }
    }
} finally {
    Pop-Location
}
