@echo off
setlocal
cd /d "%~dp0"
if not exist scripts mkdir scripts
cl /nologo /utf-8 /O2 /W4 /LD src\MW_WeatherControl.c /Foscripts\MW_WeatherControl.obj /link /MACHINE:X86 /OUT:scripts\MW_WeatherControl.asi kernel32.lib user32.lib advapi32.lib gdi32.lib
if errorlevel 1 exit /b 1
del /q scripts\MW_WeatherControl.obj 2>nul
