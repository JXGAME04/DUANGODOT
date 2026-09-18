@echo off
rem Mo client JX NEXT (Godot 4.7) - nhap doi (double-click) la chay.
rem Server phai dang chay truoc:  python tools\dev.py start   (zone 17001, gateway 17100)
setlocal
set "HERE=%~dp0"
set "GODOT="
if defined JX_GODOT if exist "%JX_GODOT%" set "GODOT=%JX_GODOT%"
if not defined GODOT for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\GodotEngine.GodotEngine_*") do (
  for %%G in ("%%~D\Godot_v*_win64.exe") do if not defined GODOT set "GODOT=%%~G"
)
if not defined GODOT for /f "delims=" %%G in ('where godot 2^>nul') do if not defined GODOT set "GODOT=%%G"
if not defined GODOT (
  echo Khong tim thay Godot 4.7 - cai bang: winget install GodotEngine.GodotEngine
  pause
  exit /b 1
)
echo Mo client: "%GODOT%" --path "%HERE%client"
start "JX NEXT client" "%GODOT%" --path "%HERE%client"
