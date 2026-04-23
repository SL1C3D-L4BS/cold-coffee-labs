@echo off
cd /d "%~dp0..\.."
set "PY=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if exist "%PY%" ("%PY%" -u "%~dp0ambientcg_pull_status.py" %* & exit /b %ERRORLEVEL%)
py -3 -u "%~dp0ambientcg_pull_status.py" %*
exit /b %ERRORLEVEL%
