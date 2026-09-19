@echo off
rem Mo client JX NEXT (Godot 4.7) va dich thang vao map 3D Ba Lang (9053) sau khi dang nhap (--gm=NewWorld...).
rem Server phai dang chay truoc:  python tools\dev.py start   (zone 19001, gateway 19100)
rem Map 3D khac: client3d.cmd 9078 171 421   (id = 9052 + id scene, xem: python tools\scn3d\batch_maps.py --list)
setlocal
set "HERE=%~dp0"
set "MAP=%~1"
set "X=%~2"
set "Y=%~3"
if not defined MAP set "MAP=9053"
if not defined X set "X=232"
if not defined Y set "Y=194"
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
set "RENDER="
if defined JX_RENDER set "RENDER=--rendering-method %JX_RENDER% --rendering-driver vulkan"
echo Mo client 3D: "%GODOT%" %RENDER% --path "%HERE%client" -- --gm=NewWorld(%MAP%,%X%,%Y%)
start "JX NEXT client 3D" "%GODOT%" %RENDER% --path "%HERE%client" -- "--gm=NewWorld(%MAP%,%X%,%Y%)"
