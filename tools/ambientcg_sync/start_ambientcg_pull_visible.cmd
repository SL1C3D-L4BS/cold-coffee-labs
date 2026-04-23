@echo off
rem Opens a separate PowerShell window (stays open) for the full AmbientCG download, unzip, and Greywater import.
rem Log also: content\_download_cache\ambientcg_downloader\pull.log
cd /d "%~dp0..\.."
start "Greywater AmbientCG — pull + import" powershell.exe -NoProfile -ExecutionPolicy Bypass -NoExit -File "%~dp0run_downloader_then_import.ps1"
