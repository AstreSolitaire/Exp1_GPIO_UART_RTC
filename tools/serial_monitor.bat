@echo off
rem ============================================================
rem  Exp1 Serial Monitor launcher (double-click to run)
rem  Optional args:
rem     serial_monitor.bat -p COM11 -b 9600
rem  NOTE: keep this file ASCII-only; cmd parses batch files in
rem        the OEM codepage and UTF-8 Chinese comments break it.
rem ============================================================
title Exp1 Serial Monitor (STM32F407G-DISC1)
cd /d "%~dp0"
set PYTHONIOENCODING=utf-8

rem explicit path so PATH cannot hit the WindowsApps python stub
set "PYEXE=D:\python.exe"
if not exist "%PYEXE%" set "PYEXE=python"

"%PYEXE%" serial_monitor.py %*
echo.
pause
