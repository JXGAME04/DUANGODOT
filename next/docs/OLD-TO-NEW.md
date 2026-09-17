# Bảng đối chiếu mã cũ → JX NEXT

Quy ước: file mới **đặt tên theo file cũ cùng chức năng** (`KNpc`, `KSubWorld`, `UiLogin`…) để người
quen mã cũ tìm đúng chỗ. Khi một file cũ tách thành nhiều file mới, tên cũ giữ cho phần chính.

## Server (Heaven/Core cũ → zone C++ `server/`)

| Mã cũ | Mới | Ghi chú |
|---|---|---|
| `Core/Src/KNpc.h/.cpp` (mọi nhân vật: người chơi lẫn NPC) | `server/zone/include/jx/zone/KNpc.h` (`struct KNpc`, `KNpcKind`) | vị trí fixed-point, waypoints, hướng 0..63 |
| `Core/Src/KMath.h` (`g_GetDirIndex` 64 hướng) | `server/zone/include/jx/zone/KMath.h`, `client/scenes/KMath.gd` | bảng sin sinh lại (bản cũ đọc từ USB key) |
| `Core/Src/KSubWorld.h/.cpp` | `server/zone/…/KSubWorld.h`, `src/KSubWorld.cpp` (`class KSubWorld`) | spawn/remove, di chuyển theo tick, AOI, chat, outbox |
| `Core/Src/KRegion.h/.cpp` (region 512×1024, danh sách npc) | `server/zone/…/KRegion.h/.cpp` (`class KRegionGrid`) | lưới AOI 512 ô, tầm nhìn 3×3 |
| `KSubWorld::LoadMap` + `KRegion::LoadServerObstacle` + `KNpcFindPath` | `server/zone/…/KMapData.h/.cpp` (`class KMapData`) | `map.json`/`obstacle.bin`, A* + làm mượt |
| `MultiServer/GameServer` (`KSwordOnLineSever`, Heaven) | `server/zone/…/KGameServer.h/.cpp`, `src/main.cpp` | nhận gateway, tick 20 Hz, PlayerSave |
| `MultiServer/Common/SocketServer.h`, `IOCompletionPort.h` | `server/net/include/jx/net/KSocket.h` (`Connection`, `Listener`, `connect`) | Asio, khung tin `jx::frame` |
| `Core/Src/KProtocol.h` (packet struct) | `proto/jx/*.proto` (`msg.proto` = bảng id) | protobuf 3, sinh cho C++/Go/GDScript |
| `Engine/Src/KDebug.h` (`g_DebugLog`) | `server/common/include/jx/log.hpp` (`jx::log::info(...)`) | JSON một dòng, theo `docs/LOGGING.md` |
| `Engine/Src/KIniFile.h` | `server/common/…/jx/config.hpp` | JSON + env + `--set` |
| `KPlayer.h`, `KPlayerSet` (tài khoản, RoleData) | `proto/jx/role.proto` + gateway Go (`persist`) | dữ liệu nhân vật là protobuf `RoleData` |
| `Core/Src/KNpcTemplate.h/.cpp` (`npcs.txt`) | `server/zone/…/KNpcTemplate.h/.cpp` (`KNpcTemplateSet`) | đọc `npcres/npcs.json` xuất từ bảng server |
| `Core/Src/KNpcAI.h/.cpp` (phần `_SERVER`, `ProcessAIType01..06`) | `server/zone/…/KNpcAI.h`, `src/KNpcAI.cpp` (`class KNpcAI`, `g_GenOneRelation`) | quái đánh trả; `KNpcSet::GenOneRelation`/`GetRelation` = `KSubWorld::relation` |
| `Engine/Src/KLuaScript.h/.cpp` (Lua 4.0: `Load`, `CallFunction`, `Include`) | `server/zone/…/KLuaScript.h/.cpp` (Lua **5.4** + lớp tương thích Lua 4) | script cũ chạy nguyên văn |
| `Engine/Src/KScriptCache.h` (`g_GetScript`) | `server/zone/…/KScriptCache.h/.cpp` | một trạng thái Lua cho mỗi file, nạp một lần |
| `KNpcTemplate::InitNpcLevelData` (+ `g_pNpcTemplate[id][level]`) | `KNpcTemplateSet::level_data` + cache trong `KSubWorld` | máu/sát thương/kinh nghiệm qua level script |
| `Core/Src/ScriptFuns.cpp` (`GameScriptFuns[]`, `GetPlayerIndex`) | `server/zone/…/ScriptFuns.h/.cpp` (`RegisterGameScriptFuns`, `KScriptContext`) | API script cho trap: `NewWorld`, `SetPos`, `GetFightState`… |
| `KRegion::LoadServerTrap` + `KNpc::CheckTrap` + `KNpc::ChangeWorld` | `KMapData::trap_at`, `KSubWorld::check_trap/execute_script/change_world_request`, `KGameServer::process_world_changes` | bẫy, cổng, nhiều map trong một zone |
| `KSubWorldSet` (nhiều `SubWorld[]`) | `KGameServer::worlds_` (`world_of_map`, `world_of_session`) | một `KSubWorld` cho mỗi map |

Sắp tới (chưa có, sẽ dùng đúng tên): `KItem`/`KItemSet`, `KSkill`/`KSkills`/`KMissle`, `KPathFinder`,
`KMission`/`KPlayerTask`, `KPlayerTeam`, `KPlayerTong`, `KScriptValueSet` (Lua 5.4).

## Gateway / auth / DB (Bishop, PaySys, Goddess cũ → Go `services/`)

| Mã cũ | Mới |
|---|---|
| `MultiServer/Bishop` (gateway, tạo/chọn nhân vật) | `services/internal/gateway` (`gateway.go` server, `session.go` phiên, `zone.go` cầu zone, `KGatewayStats.go` đếm), `cmd/gateway` |
| `Sword3PaySys/S3AccServer` (`S3PAccount::Login`, `Account_info`) | `services/pkg/auth` (`S3PAccount.go` máy chủ tài khoản, `password.go` argon2id, `auth.go` giao diện + Options), `cmd/jxaccount` |
| `MultiServer/testAccServer` (công cụ tài khoản console) | `services/cmd/jxaccount` (`add`/`passwd`/`freeze`/`expire`/`list`) |
| `Bishop/LoginDef.h` (`LOGIN_R_*`), `S3Client/Login/Login.cpp`, `Ui/UiCase/UiConnectInfo.cpp` (`CI_MI_*`) | `proto/jx/common.proto` (`Result`), `client/net/KLogin.gd` (`result_text`, `session_ends`) |
| `MultiServer/Goddess` (DB nhân vật) | `services/pkg/persist` (`FileStore`; PostgreSQL sau) |
| `MultiServer/Common/Buffer.h`, `IOBuffer.h` | `services/pkg/frame` |
| `MultiServer/Common/SocketServer` + `S3Client/NetConnect` (chỉ TCP thô + xáo trộn riêng) | `services/pkg/transport` (`KListener.go` mở cửa tcp/tls/ws/wss, `KWebSocket.go` RFC 6455) |

## Tool đọc dữ liệu cũ (`services/pkg/jxold`, `cmd/jxassets`)

| Mã cũ | Mới |
|---|---|
| `Engine/Src/XPackFile.h/.cpp`, `KPakList` (hash `FileNameToId`) | `pkg/jxold/pak/XPackFile.go` |
| `Engine/Src/KSprite.h/.cpp`, `KDrawSprite.cpp` (RLE) | `pkg/jxold/spr/KSprite.go`, `XPackSprFrame.go` (spr nén theo frame), `KSpriteAtlas.go` (PNG) |
| `Core/Src/KSubWorld::LoadMap`, `Scene/KScenePlaceRegionC.cpp`, `SceneDataDef.h` | `pkg/jxold/wor/KSubWorld.go` |
| `Core/Src/KSkillManager.cpp` (`skills.txt`) | `pkg/jxold/npcres/KSkillManager.go` (bán kính/kiểu kỹ năng cho AI) |
| `Represent2/KRepresentShell2::CoordinateTransform`, `DrawScaleSprite` (FRAME_DRAW / REF_SPOT), `KIpotLeaf.cpp::PaintABuildinObject` | `pkg/jxold/export/KSceneExport.go` (chiếu `y/2 − z·887/1024`, vị trí vật tĩnh/động, kiểu sắp xếp `k`) |
| `KScenePlaceRegionC::GetBuildinObjs` (header đếm point/line/tree/above) | `pkg/jxold/wor/KSubWorld.go` (`BuildinKind`); `jxassets objects <map> <x> <y>` in bản ghi thô để đối chiếu |
| `KRegion::LoadServerNpc` (`Region_S.dat` trong pak server), `KRegion::LoadClientNpc` | `pkg/jxold/wor/KSubWorld.go` (`LoadServerRegion`, `LoadRegion`); `jxassets npcs <map>` |
| `KNpcSet.cpp` (`gNpcNameMap` ← `replacename_npc.txt`, `KNpcSet::Add(int, KSPNpc*)`), `KNpcRes::GetNormalNpcStandDir` | `pkg/jxold/npcres/KNpcSet.go` (`ParseReplaceNames`, `PlacementName`, `StandDir`) |
| `ucl/n2b_d.c` | `pkg/jxold/nrv2b` |
| `Engine/Src/KTabFile.h/.cpp` (bảng tab, hàng/cột 1-based) | `pkg/jxold/npcres/KTabFile.go` |
| `Core/Src/KNpcTemplate.cpp` (`npcs.txt`) | `pkg/jxold/npcres/KNpcTemplate.go` |
| `Core/Src/KNpcResList.cpp`, `KNpcResNode.cpp` (`人物类型.txt`, bảng bộ phận/关联表/贴图顺序表, `CSortTable`) | `pkg/jxold/npcres/KNpcResNode.go` (`List`, `Node`, `SortTable`) |
| `KNpcRes::Init/SetAction` (chọn file spr theo trang bị + action) | `pkg/jxold/export/KNpcResExport.go` → `client/assets/npcres/{npcs.json,res/<tên>.json}`; `jxassets export-npcres` |
| Bảng TCVN3 (skill `vn_to_octal.py`) | `pkg/jxold/text/KTextTCVN3.go` |

## Client (S3Client cũ → Godot `client/`)

| Mã cũ | Mới | Singleton |
|---|---|---|
| `S3Client/NetConnect` (`KNetConnectAgent`) | `client/autoload/KSocketClient.gd` (tcp/tls/ws/wss), `client/net/KNetAddress.gd` | `Net` |
| `Core/Src/KProtocolProcess.cpp` (xử lý packet client) | `client/autoload/KProtocolProcess.gd` | `Game` |
| `Engine/Src/KPakFile`, `KImageRes` (nạp tài nguyên) | `client/autoload/KPakFile.gd` | `Assets` |
| `Engine/Src/KDebug` | `client/autoload/KDebug.gd` | `Log` |
| `Core/Src/KProtocol.h` (khung tin) | `client/net/KProtocol.gd` + `client/proto/jx_pb.gd` (sinh) | |
| `S3Client/Login/Login.cpp` (`KLogin`, `ProcessAccountLoginResponse`) | `client/net/KLogin.gd` (bảng thông báo kết quả đăng nhập/bị đá) | |
| `Ui/UiCase/UiLogin` | `client/scenes/UiLogin.gd/.tscn` | |
| `Ui/UiCase/UiSelPlayer`, `UiNewPlayer` | `client/scenes/UiSelPlayer.gd/.tscn` | |
| `Ui/UiCase/UiGame` + `UiChatCentre` + `UiInformation` | `client/scenes/UiGame.gd/.tscn` | |
| `Core/Src/Scene/KScenePlaceC.cpp` (vẽ map theo region, `AddObject/MoveObject` nhân vật vào cây) | `client/scenes/KScenePlaceC.gd` | |
| `Core/Src/Scene/KIpoTree.h/.cpp` (cây sắp xếp vật thể ↔ nhân vật, bỏ phần ánh sáng) | `client/scenes/KIpoTree.gd` | |
| `Core/Src/Scene/KIpotBranch.h/.cpp` (nhánh = đường đáy chia cảnh, danh sách lá) | `client/scenes/KIpotBranch.gd` | |
| `Core/Src/Scene/KIpotLeaf.h/.cpp` (lá: vật point/line, nhân vật runtime, `Clone` cắt ảnh) | `client/scenes/KIpotLeaf.gd` | |
| `Core/Src/Scene/SceneMath.h/.cpp` (`SM_Relation_PointLine`, `SM_Relation_LineLine_CheckCut`) | `client/scenes/KSceneMath.gd` | |
| `Core/Src/KNpc` (phía client: `Activate`/`Paint`, `m_Frames`, `m_ResDir` quay dần) | `client/scenes/KNpc.gd` | |
| `Core/Src/KNpcRes.h/.cpp` (`Draw`: ghép bộ phận theo hướng/khung, bóng, thứ tự vẽ) | `client/scenes/KNpcRes.gd` | |
| `Core/Src/KNpcResNode.cpp` (`GetActNo`, `CSortTable::GetSort`) | `client/scenes/KNpcResNode.gd` (hàm tĩnh, test được) | |
| `Core/Src/KNpcResList.cpp` (`g_NpcResList`) | `client/autoload/KNpcResList.gd` | `NpcResList` |
