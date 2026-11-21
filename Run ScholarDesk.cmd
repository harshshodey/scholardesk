@echo off
cd /d "%~dp0"
title ScholarDesk - Student Management System
if not exist "bin\ScholarDesk.exe" (
  echo Build the project first with build.ps1. See README.md.
  pause
  exit /b 1
)
"bin\ScholarDesk.exe"
if errorlevel 1 pause
