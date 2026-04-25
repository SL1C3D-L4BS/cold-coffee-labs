# One-shot MCP stdio smoke: initialize + tools/call docs.search
# Usage: .\mcp_smoke_stdin.ps1 [-Exe path\to\gw_bld_server.exe] [-ProjectRoot path]
param(
    [string]$Exe = (Join-Path $PSScriptRoot "..\target\debug\gw_bld_server.exe" | Resolve-Path -ErrorAction Stop),
    [string]$ProjectRoot = (Join-Path $PSScriptRoot "..\.." | Resolve-Path -ErrorAction Stop)
)

function Send-Frame([System.IO.Stream] $stdin, [string] $json) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $hdr   = [System.Text.Encoding]::ASCII.GetBytes("Content-Length: $($bytes.Length)`r`n`r`n")
    $stdin.Write($hdr, 0, $hdr.Length)
    $stdin.Write($bytes, 0, $bytes.Length)
    $stdin.Flush()
}

$psi                 = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName        = $Exe
$psi.Arguments       = '--project-root "' + ($ProjectRoot -replace '"','\"') + '"'
$psi.UseShellExecute = $false
$psi.RedirectStandardInput  = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError  = $true
$psi.CreateNoWindow      = $true

$p = New-Object System.Diagnostics.Process
$p.StartInfo = $psi
[void]$p.Start()

$init = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","clientInfo":{"name":"mcp_smoke","version":"0"},"capabilities":{}}}'
$call = '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"docs.search","arguments":{"query":"Vulkan","k":3}}}'

Send-Frame $p.StandardInput.BaseStream $init
Send-Frame $p.StandardInput.BaseStream $call
$p.StandardInput.Close()

$out = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit(30000) | Out-Null

Write-Host "=== gw_bld_server exit $($p.ExitCode) ==="
if ($err) { Write-Host "=== stderr ===`n$err" }
Write-Host "=== stdout (first 2000 chars) ==="
Write-Host $out.Substring(0, [Math]::Min(2000, $out.Length))
