@echo off
rem build.bat - thin wrapper around build.ps1
powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
