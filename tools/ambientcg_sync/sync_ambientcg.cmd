@echo off
setlocal
rem Import from ambientcg-downloader "unzipped" folder into the repo, refresh index, franchise links.
rem Prereq: clone https://github.com/alvarognnzz/ambientcg-downloader , edit config.yaml, run main.py.
rem
rem   set AMBIENTCG_UNZIPPED=%USERPROFILE%\Desktop\ambientcg-downloader\unzipped
rem   tools\ambientcg_sync\sync_ambientcg.cmd
rem
rem Extra flags are forwarded (e.g. --copy, or --delete-repo-root "...\ambientcg-downloader" --yes).
rem To pass --source explicitly instead, run Python:
rem   python tools\ambientcg_sync\import_from_downloader.py --source "..." --link-franchises

cd /d "%~dp0..\.."

if "%AMBIENTCG_UNZIPPED%"=="" (
  echo Set AMBIENTCG_UNZIPPED to your ambientcg-downloader\unzipped folder, then run this again.
  echo Or: python tools\ambientcg_sync\import_from_downloader.py --source "PATH" --link-franchises
  exit /b 2
)

set "PY312=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
set "IM=%~dp0import_from_downloader.py"

if exist "%PY312%" (
  "%PY312%" -u "%IM%" --source "%AMBIENTCG_UNZIPPED%" --link-franchises %*
  exit /b %ERRORLEVEL%
)

where py >nul 2>&1
if %ERRORLEVEL%==0 (
  py -3 -u "%IM%" --source "%AMBIENTCG_UNZIPPED%" --link-franchises %*
  exit /b %ERRORLEVEL%
)

where python >nul 2>&1
if %ERRORLEVEL%==0 (
  python -u "%IM%" --source "%AMBIENTCG_UNZIPPED%" --link-franchises %*
  exit /b %ERRORLEVEL%
)

echo No Python found. Install Python 3.
exit /b 127
