@echo off
rem Wrapper: corre scripts\learn.ps1 sem depender da execution policy do sistema.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\learn.ps1" %*
exit /b %ERRORLEVEL%
