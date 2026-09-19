@echo off
rem Mo scene thu nghiem 3D (map world_baling + NPC) cua ban swrod3-3d - nhap doi la chay. Khong can server.
rem Tai san phai co san: python tools\scn3d\export_scene.py world_baling  va  python tools\scn3d\export_npc.py --map world_baling
setlocal
set "HERE=%~dp0"
set "MAP=%~1"
if "%MAP%"=="" set "MAP=world_baling"
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
echo Mo scene 3D: "%GODOT%" --path "%HERE%client" scenes3d/Scn3D.tscn -- --map=%MAP%
start "JX NEXT 3D" "%GODOT%" --path "%HERE%client" scenes3d/Scn3D.tscn -- --map=%MAP%
