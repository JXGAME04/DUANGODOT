@echo off
rem Mo client JX NEXT (Godot 4.7) va dich thang vao map 3D Ba Lang (9053) sau khi dang nhap (--gm=NewWorld...).
rem Server phai dang chay truoc:  python tools\dev.py start   (zone 19001, gateway 19100)
rem Map 3D khac: client3d.cmd 9078 171 421   (id = 9052 + id scene, xem: python tools\scn3d\batch_maps.py --list)
rem Vao thang bang tai khoan test, khong go gi: client3d.cmd play [ten tai khoan]  (tu dang nhap, nhan vat Cai Bang
rem   cap 90 co du ky nang phai, con trong tui, ngua Liet Bach Ma; mat khau "auto"; mac dinh tai khoan test3d)
rem set JX_RENDER=mobile  (hoac forward_plus) truoc khi chay: renderer Vulkan cho bloom rong nhu ban tham khao (3D-66);
rem mac dinh GL Compatibility theo ADR-008 (bloom chi vai pixel).
setlocal
set "HERE=%~dp0"
set "MODE=%~1"
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
if /i "%MODE%"=="play" goto play
echo Mo client 3D: "%GODOT%" %RENDER% --path "%HERE%client" -- --gm=NewWorld(%MAP%,%X%,%Y%)
start "JX NEXT client 3D" "%GODOT%" %RENDER% --path "%HERE%client" -- "--gm=NewWorld(%MAP%,%X%,%Y%)"
exit /b 0

:play
set "ACC=%~2"
if not defined ACC set "ACC=test3d"
echo Mo client 3D, tai khoan %ACC% (Cai Bang cap 90, ky nang phai, con, ngua), map Ba Lang 3D
start "JX NEXT client 3D" "%GODOT%" %RENDER% --path "%HERE%client" -- --play --account=%ACC% --password=auto --series=3 --server=127.0.0.1:19100 "--gm=NewWorld(9053,232,194)" "--gm=for i=1,89 do AddExp(100000000,0) end" "--gm=SetFaction(\"gaibang\")" "--gm=Include(\"\\\\script\\\\global\\\\skills_table.lua\") add_gb(90)" "--gm=AddItem(0,0,2,1,0,0)" "--gm=AddItem(0,10,2,1,0,0)"
exit /b 0
