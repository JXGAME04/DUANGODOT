# Bảng đối chiếu mã cũ → JX NEXT

Quy ước: file mới **đặt tên theo file cũ cùng chức năng** (`KNpc`, `KSubWorld`, `UiLogin`…) để người
quen mã cũ tìm đúng chỗ. Khi một file cũ tách thành nhiều file mới, tên cũ giữ cho phần chính.

## Server (Heaven/Core cũ → zone C++ `server/`)

| Mã cũ | Mới | Ghi chú |
|---|---|---|
| `Core/Src/KNpc.h/.cpp` (mọi nhân vật: người chơi lẫn NPC) | `server/zone/include/jx/zone/KNpc.h` (`struct KNpc`, `KNpcKind`) | vị trí fixed-point, waypoints, hướng 0..7 |
| `Core/Src/KSubWorld.h/.cpp` | `server/zone/…/KSubWorld.h`, `src/KSubWorld.cpp` (`class KSubWorld`) | spawn/remove, di chuyển theo tick, AOI, chat, outbox |
| `Core/Src/KRegion.h/.cpp` (region 512×1024, danh sách npc) | `server/zone/…/KRegion.h/.cpp` (`class KRegionGrid`) | lưới AOI 512 ô, tầm nhìn 3×3 |
| `KSubWorld::LoadMap` + `KRegion::LoadServerObstacle` + `KNpcFindPath` | `server/zone/…/KMapData.h/.cpp` (`class KMapData`) | `map.json`/`obstacle.bin`, A* + làm mượt |
| `MultiServer/GameServer` (`KSwordOnLineSever`, Heaven) | `server/zone/…/KGameServer.h/.cpp`, `src/main.cpp` | nhận gateway, tick 20 Hz, PlayerSave |
| `MultiServer/Common/SocketServer.h`, `IOCompletionPort.h` | `server/net/include/jx/net/KSocket.h` (`Connection`, `Listener`, `connect`) | Asio, khung tin `jx::frame` |
| `Core/Src/KProtocol.h` (packet struct) | `proto/jx/*.proto` (`msg.proto` = bảng id) | protobuf 3, sinh cho C++/Go/GDScript |
| `Engine/Src/KDebug.h` (`g_DebugLog`) | `server/common/include/jx/log.hpp` (`jx::log::info(...)`) | JSON một dòng, theo `docs/LOGGING.md` |
| `Engine/Src/KIniFile.h` | `server/common/…/jx/config.hpp` | JSON + env + `--set` |
| `KPlayer.h`, `KPlayerSet` (tài khoản, RoleData) | `proto/jx/role.proto` + gateway Go (`persist`) | dữ liệu nhân vật là protobuf `RoleData` |

Sắp tới (chưa có, sẽ dùng đúng tên): `KItem`/`KItemSet`, `KSkill`/`KSkills`/`KMissle`, `KNpcAI`,
`KNpcTemplate`/`KNpcRes`, `KMission`/`KPlayerTask`, `KPlayerTeam`, `KPlayerTong`, `KScriptValueSet` (Lua 5.4).

## Gateway / auth / DB (Bishop, PaySys, Goddess cũ → Go `services/`)

| Mã cũ | Mới |
|---|---|
| `MultiServer/Bishop` (gateway, tạo/chọn nhân vật) | `services/internal/gateway` (`gateway.go` server, `session.go` phiên, `zone.go` cầu zone), `cmd/gateway` |
| `Sword3PaySys` (tài khoản) | `services/pkg/auth` (`Dev` cho dev) |
| `MultiServer/Goddess` (DB nhân vật) | `services/pkg/persist` (`FileStore`; PostgreSQL sau) |
| `MultiServer/Common/Buffer.h`, `IOBuffer.h` | `services/pkg/frame` |

## Tool đọc dữ liệu cũ (`services/pkg/jxold`, `cmd/jxassets`)

| Mã cũ | Mới |
|---|---|
| `Engine/Src/XPackFile.h/.cpp`, `KPakList` (hash `FileNameToId`) | `pkg/jxold/pak/XPackFile.go` |
| `Engine/Src/KSprite.h/.cpp`, `KDrawSprite.cpp` (RLE) | `pkg/jxold/spr/KSprite.go`, `XPackSprFrame.go` (spr nén theo frame), `KSpriteAtlas.go` (PNG) |
| `Core/Src/KSubWorld::LoadMap`, `Scene/KScenePlaceRegionC.cpp`, `SceneDataDef.h` | `pkg/jxold/wor/KSubWorld.go` |
| `Represent2/KRepresentShell2::CoordinateTransform` | `pkg/jxold/export/KSceneExport.go` (chiếu `y/2 − z·887/1024`) |
| `ucl/n2b_d.c` | `pkg/jxold/nrv2b` |
| Bảng TCVN3 (skill `vn_to_octal.py`) | `pkg/jxold/text/KTextTCVN3.go` |

## Client (S3Client cũ → Godot `client/`)

| Mã cũ | Mới | Singleton |
|---|---|---|
| `S3Client/NetConnect` (`KNetConnectAgent`) | `client/autoload/KSocketClient.gd` | `Net` |
| `Core/Src/KProtocolProcess.cpp` (xử lý packet client) | `client/autoload/KProtocolProcess.gd` | `Game` |
| `Engine/Src/KPakFile`, `KImageRes` (nạp tài nguyên) | `client/autoload/KPakFile.gd` | `Assets` |
| `Engine/Src/KDebug` | `client/autoload/KDebug.gd` | `Log` |
| `Core/Src/KProtocol.h` (khung tin) | `client/net/KProtocol.gd` + `client/proto/jx_pb.gd` (sinh) | |
| `Ui/UiCase/UiLogin` | `client/scenes/UiLogin.gd/.tscn` | |
| `Ui/UiCase/UiSelPlayer`, `UiNewPlayer` | `client/scenes/UiSelPlayer.gd/.tscn` | |
| `Ui/UiCase/UiGame` + `UiChatCentre` + `UiInformation` | `client/scenes/UiGame.gd/.tscn` | |
| `Core/Src/Scene/KScenePlaceC.cpp` (vẽ map theo region) | `client/scenes/KScenePlaceC.gd` | |
| `Core/Src/KNpc` (phía client: vẽ nhân vật) | `client/scenes/KNpc.gd` | |
