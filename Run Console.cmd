@echo off
cd /d "%~dp0"
title ScholarDesk - C++ Console Edition
"bin\ScholarDeskConsole.exe"
if errorlevel 1 pause
