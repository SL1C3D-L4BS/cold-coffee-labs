# tools/prompts/render_pack.ps1 - PowerShell fallback renderer.
#
# Equivalent to tools/prompts/render_pack.py but implemented as a small
# line-by-line parser so contributors without Python installed can still
# render the pack locally. CI uses the Python renderer as authoritative.
#
# Usage:
#   powershell -File tools/prompts/render_pack.ps1
#   powershell -File tools/prompts/render_pack.ps1 -Check

[CmdletBinding()]
param(
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
$RepoRoot    = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$PackPath    = Join-Path $RepoRoot 'docs/prompts/pack.yml'
$RenderRoot  = Join-Path $RepoRoot 'docs/prompts'

if (-not (Test-Path $PackPath)) { throw "pack.yml not found: $PackPath" }

# Simple state machine:
#   OUTSIDE -> prefix "  - id:" opens a task (INSIDE_TASK)
#   INSIDE_TASK -> "    key: value" or "    key:" (list) or "    prompt: |"
#   INSIDE_LIST -> "      - item"
#   INSIDE_PROMPT -> "      ..." until a line with indent < 6 is hit.
function Parse-Pack {
    param([string]$Path)

    $raw_lines = Get-Content -Path $Path -Encoding UTF8
    $lines = @($raw_lines | ForEach-Object { $_ -replace "`r$", '' })

    $tasks = New-Object System.Collections.ArrayList
    $task  = $null
    $mode  = 'OUTSIDE'   # OUTSIDE | INSIDE_TASK | INSIDE_LIST | INSIDE_PROMPT
    $listKey = $null
    $promptLines = $null

    function Close-Task {
        param($Tasks, $T)
        if ($null -ne $T) { [void]$Tasks.Add($T) }
    }

    foreach ($line in $lines) {
        # Skip top-level comments / schema header lines outside tasks.
        if ($mode -ne 'INSIDE_PROMPT' -and $line -match '^#') { continue }
        if ($mode -ne 'INSIDE_PROMPT' -and $line -match '^\s*#') { continue }

        # --- End of prompt block? ----------------------------------------
        if ($mode -eq 'INSIDE_PROMPT') {
            if ($line -eq '') {
                [void]$promptLines.Add('')
                continue
            }
            # Lines that are part of the prompt must have >= 6-space indent.
            if ($line -match '^      ' -or $line -match '^\s{6}') {
                [void]$promptLines.Add($line.Substring(6))
                continue
            }
            # Anything with less indent ends the prompt block.
            $task.prompt = ($promptLines -join "`n").TrimEnd()
            $promptLines = $null
            $mode = 'INSIDE_TASK'
            # Do not continue -- reprocess this line in INSIDE_TASK mode.
        }

        # --- Task boundary ------------------------------------------------
        if ($line -match '^  - id:\s*(\S+)\s*$') {
            Close-Task -Tasks $tasks -T $task
            $task = [ordered]@{
                id            = $Matches[1]
                kind          = ''
                phase         = $null
                week          = $null
                tier          = $null
                title         = ''
                depends_on    = @()
                inputs        = @()
                writes        = @()
                exit_criteria = @()
                prompt        = ''
            }
            $mode = 'INSIDE_TASK'
            $listKey = $null
            continue
        }

        if ($mode -eq 'OUTSIDE') { continue }

        # --- Multiline list item ("      - x") ---------------------------
        if ($mode -eq 'INSIDE_LIST' -and $line -match '^      - (.+)$') {
            $item = $Matches[1].Trim().Trim('"', "'")
            [void]$task[$listKey].Add($item)
            continue
        }

        # --- 4-space scalar / list / prompt ------------------------------
        if ($line -match '^    (\w+):\s*\|\s*$') {
            # Block scalar (the prompt).
            if ($Matches[1] -eq 'prompt') {
                $promptLines = New-Object System.Collections.ArrayList
                $mode = 'INSIDE_PROMPT'
                continue
            }
        }

        if ($line -match '^    (\w+):\s*(.*)$') {
            $key = $Matches[1]
            $val = $Matches[2].TrimEnd()
            $mode = 'INSIDE_TASK'
            $listKey = $null

            if ($val -eq '') {
                $listKey = $key
                $task[$key] = New-Object System.Collections.ArrayList
                $mode = 'INSIDE_LIST'
                continue
            }
            if ($val -eq '[]') {
                $task[$key] = @()
                continue
            }
            if ($val -match '^\[(.*)\]$') {
                $inner = $Matches[1].Trim()
                if ($inner -eq '') { $task[$key] = @() }
                else {
                    $task[$key] = @($inner -split ',' | ForEach-Object { $_.Trim().Trim('"', "'") })
                }
                continue
            }

            $clean = $val.Trim().Trim('"', "'")
            if ($key -in @('phase','week')) {
                if ($clean -eq 'null' -or $clean -eq '') { $task[$key] = $null }
                else { $task[$key] = [int]$clean }
            } else {
                $task[$key] = $clean
            }
            continue
        }

        # Blank lines inside a task just continue.
    }

    if ($mode -eq 'INSIDE_PROMPT' -and $null -ne $task) {
        $task.prompt = ($promptLines -join "`n").TrimEnd()
    }
    Close-Task -Tasks $tasks -T $task

    return $tasks
}

function Get-BucketDir {
    param($Task)
    switch ($Task.kind) {
        'phase-week' {
            if ($null -eq $Task.phase) { throw "phase-week task $($Task.id) missing phase" }
            return Join-Path $RenderRoot ("phase{0:D2}" -f [int]$Task.phase)
        }
        'preflight'   { return Join-Path $RenderRoot 'preflight' }
        'ai-tooling'  { return Join-Path $RenderRoot 'ai_tooling' }
        'ci'          { return Join-Path $RenderRoot 'ci' }
        'docs'        { return Join-Path $RenderRoot 'docs' }
        default       { throw "unknown kind '$($Task.kind)' in task $($Task.id)" }
    }
}

function Format-Task {
    param($Task)

    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("# $($Task.id) - $($Task.title)")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("| Field | Value |")
    [void]$sb.AppendLine("|-------|-------|")
    [void]$sb.AppendLine("| Kind | ``$($Task.kind)`` |")
    if ($null -ne $Task.phase) { [void]$sb.AppendLine("| Phase | $($Task.phase) |") }
    if ($null -ne $Task.week)  { [void]$sb.AppendLine("| Week | $($Task.week) |") }
    if ($Task.tier) { [void]$sb.AppendLine("| Tier | $($Task.tier) |") }

    $dep = @($Task.depends_on)
    if ($dep.Count -gt 0) {
        $formatted = ($dep | ForEach-Object { "``$_``" }) -join ', '
        [void]$sb.AppendLine("| Depends on | $formatted |")
    } else {
        [void]$sb.AppendLine("| Depends on | *(none)* |")
    }
    [void]$sb.AppendLine("")

    $inputs = @($Task.inputs)
    if ($inputs.Count -gt 0) {
        [void]$sb.AppendLine("## Inputs")
        [void]$sb.AppendLine("")
        foreach ($p in $inputs) { [void]$sb.AppendLine("- ``$p``") }
        [void]$sb.AppendLine("")
    }

    $writes = @($Task.writes)
    if ($writes.Count -gt 0) {
        [void]$sb.AppendLine("## Writes")
        [void]$sb.AppendLine("")
        foreach ($p in $writes) { [void]$sb.AppendLine("- ``$p``") }
        [void]$sb.AppendLine("")
    }

    $exits = @($Task.exit_criteria)
    if ($exits.Count -gt 0) {
        [void]$sb.AppendLine("## Exit criteria")
        [void]$sb.AppendLine("")
        foreach ($e in $exits) { [void]$sb.AppendLine("- [ ] $e") }
        [void]$sb.AppendLine("")
    }

    [void]$sb.AppendLine("## Prompt")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine(($Task.prompt.TrimEnd()))
    [void]$sb.AppendLine("")

    return $sb.ToString()
}

# --- main ---------------------------------------------------------------
$tasks = Parse-Pack -Path $PackPath

# Validate DAG.
$ids = @{}
foreach ($t in $tasks) { $ids[$t.id] = $true }
foreach ($t in $tasks) {
    foreach ($d in @($t.depends_on)) {
        if (-not $ids.ContainsKey($d)) {
            throw "task $($t.id) depends on unknown id '$d'"
        }
    }
}

# Collect renderings.
$rendered = [ordered]@{}
foreach ($t in $tasks) {
    $dir  = Get-BucketDir -Task $t
    $path = Join-Path $dir "$($t.id).md"
    $rendered[$path] = Format-Task -Task $t
}

if ($Check) {
    $stale = @()
    foreach ($path in $rendered.Keys) {
        if (-not (Test-Path $path)) { $stale += $path; continue }
        $onDisk = (Get-Content -Raw -Encoding UTF8 -Path $path) -replace "`r`n", "`n"
        $want   = $rendered[$path] -replace "`r`n", "`n"
        if ($onDisk -ne $want) { $stale += $path }
    }
    if ($stale.Count -gt 0) {
        Write-Error ("Stale rendered prompt files. Run:`n  powershell -File tools/prompts/render_pack.ps1`n`nStale:`n  " + ($stale -join "`n  "))
        exit 1
    }
    Write-Host "$($rendered.Count) rendered prompt files are up-to-date."
    exit 0
}

# Clean managed buckets.
foreach ($item in Get-ChildItem $RenderRoot -Directory -ErrorAction SilentlyContinue) {
    if ($item.Name -in @('preflight','ai_tooling','ci','docs') -or $item.Name -like 'phase*') {
        Remove-Item -Recurse -Force $item.FullName
    }
}

foreach ($path in ($rendered.Keys | Sort-Object)) {
    $dir = Split-Path -Parent -Path $path
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    # Use UTF8 without BOM; ensure LF line endings for reproducibility.
    $text = $rendered[$path] -replace "`r`n", "`n"
    [System.IO.File]::WriteAllText($path, $text, (New-Object System.Text.UTF8Encoding $false))
}

Write-Host "Rendered $($rendered.Count) prompt files."
