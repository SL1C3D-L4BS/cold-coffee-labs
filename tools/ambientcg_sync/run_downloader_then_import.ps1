# Runs ambientcg-downloader main.py then Greywater import_from_downloader.py (same shell, sequential).
# Long-running (hours). Logs to content/_download_cache/ambientcg_downloader/pull.log
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Clone = Join-Path $RepoRoot 'content\_download_cache\ambientcg_downloader'
$Log = Join-Path $Clone 'pull.log'
$Py = Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312\python.exe'
if (-not (Test-Path $Py)) {
  $Py = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
  if (-not $Py) { throw 'Python 3 not found' }
}

$env:PYTHONUNBUFFERED = '1'
$FixedMain = Join-Path $RepoRoot 'tools\ambientcg_sync\ambientcg_downloader_main.py'
Copy-Item -Force -Path $FixedMain -Destination (Join-Path $Clone 'main.py')
"$(Get-Date -Format o) Starting AmbientCG pull (see $Log)" | Tee-Object -FilePath $Log -Append
Push-Location $Clone
try {
  & $Py -u main.py 2>&1 | Tee-Object -FilePath $Log -Append
  if ($LASTEXITCODE -ne 0) { throw "main.py exited $LASTEXITCODE" }
} finally {
  Pop-Location
}

"$(Get-Date -Format o) Starting import into Greywater..." | Tee-Object -FilePath $Log -Append
$Import = Join-Path $RepoRoot 'tools\ambientcg_sync\import_from_downloader.py'
$Unzipped = Join-Path $Clone 'unzipped'
& $Py -u $Import --source $Unzipped --link-franchises 2>&1 | Tee-Object -FilePath $Log -Append
"$(Get-Date -Format o) Done." | Tee-Object -FilePath $Log -Append
