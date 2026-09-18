# Bảng đối chiếu mã cũ → JX NEXT

Quy ước: file mới **đặt tên theo file cũ cùng chức năng** (`KNpc`, `KSubWorld`, `UiLogin`…) để người
quen mã cũ tìm đúng chỗ. Khi một file cũ tách thành nhiều file mới, tên cũ giữ cho phần chính.

## Server (Heaven/Core cũ → zone C++ `server/`)

| Mã cũ | Mới | Ghi chú |
|---|---|---|
| `Core/Src/KNpc.h/.cpp` (mọi nhân vật: người chơi lẫn NPC) | `server/zone/include/jx/zone/KNpc.h` (`struct KNpc`, `KNpcKind`) | vị trí fixed-point, waypoints, hướng 0..63 |
| `Core/Src/KMath.h` (`g_GetDirIndex` 64 hướng) | `server/zone/include/jx/zone/KMath.h`, `client/scenes/KMath.gd` | bảng sin sinh lại (bản cũ đọc từ USB key) |
| `Core/Src/KSubWorld.h/.cpp` | `server/zone/…/KSubWorld.h`, `src/KSubWorld.cpp` (`class KSubWorld`) | spawn/remove, di chuyển theo tick, AOI, chat, outbox |
| `Core/Src/KRegion.h/.cpp` (region 512×1024, danh sách npc) | `server/zone/…/KRegion.h/.cpp` (`class KRegionGrid`) | lưới AOI ô 256, vùng nhìn chữ nhật theo màn hình; người chơi xếp riêng với NPC |
| `KRegion::BroadCast` + `MAX_BROADCAST_COUNT` (100 người nhận mỗi gói), client tự hỏi lại npc lạ (`c2s_requestnpc`) | `server/zone/src/KInterest.cpp` (`KViewer`, `KNpc::watchers`, `KSubWorld::look_around/entity_gone`) | giới hạn 100 nằm ở điều mỗi client **biết**; cập nhật và despawn đi tới đúng những client đã được báo spawn |
| `KSubWorld::LoadMap` + `KRegion::LoadServerObstacle` + `KNpcFindPath` | `server/zone/…/KMapData.h/.cpp` (`class KMapData`) | `map.json`/`obstacle.bin`, A* + làm mượt |
| `MultiServer/GameServer` (`KSwordOnLineSever`, Heaven) | `server/zone/…/KGameServer.h/.cpp`, `src/main.cpp` | nhận gateway, tick cố định, nhiều worker, PlayerSave |
| (mới — MASTER SPEC) runtime nhiều nhân | `server/core` (`ServerClock`, `FixedTick`, `Result`, `CommandQueue`/`EventQueue`, `ThreadPool`, `JobSystem`, `Metrics`) | nơi duy nhất tạo thread, đo mọi pha |
| (mới) `Npc[MAX_NPC]` + index thô | `server/entity` (`EntityHandle` index+generation, `EntityTable`) | handle cũ chết hẳn khi ô nhớ tái dùng |
| `KSubWorld` là một map chạy trong luồng chính | `server/zone/…/KMapInstance.h/.cpp` (+ `KWorldCommand.h`) | một map instance = một chủ sở hữu, có inbox lệnh và hàng sự kiện |
| (mới) phân map cho luồng | `server/zone/…/KWorldScheduler.h/.cpp` | đo chi phí tick, dời map khỏi worker nóng |
| `KNpcFindPath` / `KMapData::find_path` cấp phát mỗi lần | `server/zone/…/KPathFinder.h/.cpp` | dùng lại bộ đệm, generation stamp |
| `MultiServer/Common/SocketServer.h`, `IOCompletionPort.h` | `server/net/include/jx/net/KSocket.h` (`Connection`, `Listener`, `connect`) | Asio, khung tin `jx::frame` |
| `Core/Src/KProtocol.h` (packet struct) | `proto/jx/*.proto` (`msg.proto` = bảng id) | protobuf 3, sinh cho C++/Go/GDScript |
| `Engine/Src/KDebug.h` (`g_DebugLog`) | `server/common/include/jx/log.hpp` (`jx::log::info(...)`) | JSON một dòng, theo `docs/LOGGING.md` |
| `Engine/Src/KIniFile.h` | `server/common/…/jx/config.hpp` | JSON + env + `--set` |
| `KPlayer.h`, `KPlayerSet` (tài khoản, RoleData) | `proto/jx/role.proto` + gateway Go (`persist`) | dữ liệu nhân vật là protobuf `RoleData` |
| `KNpc.h` — hai khối `m_XXX` (gốc) / `m_CurrentXXX` (hiện tại), 0x15a8… / 0x1188… của `jx_linux_y` | `server/zone/…/KNpcAttrib.h` (`KNpcAttrib`, `KNpcCurrentAttrib`, cặp "yan"), `KNpc::base` / `KNpc::cur`, `KNpc::clear_attrib` = `KNpc::ClearAttrib 0x0807EE60` | mỗi trường ghi offset nhị phân; giá trị dùng thật = max(thường, yan) (`life_max_v()`…) |
| `Core/Src/KMagicAttrib.h` (`KMagicAttrib`, enum `MAGIC_ATTRIB`) | `server/zone/…/KMagicAttrib.h` + `KMagicAttribId.h` (sinh bởi `tools/gen_magic_ids.py` từ `docs/linux/jx_linux_magicattrib.tsv`, 335 tên của nhị phân) | `magic_lifemax_v = 85`…, `magic_attrib_name(id)` |
| `Core/Src/KNpcAttribModify.h/.cpp` (`ProcessFunc[]`) | `server/zone/…/KNpcAttribModify.h/.cpp` (`KNpcAttribModify::modify(npc, attrib, ctx)`) | 217 ô đọc từng thân hàm (`re_attribmod.py`), đúng phép toán (`_p` nhân giá trị **gốc**, cặp anti, kẹp 0..500, thời gian đóng băng…) |
| `Core/Src/KPlayer.cpp` (`LoadFrom`, `LevelUp`, `AddBaseStrength…`, `UpdataCurData`, `ReCalcEquip`, `SetNpcPhysicsDamage`) | `server/zone/…/KPlayer.h/.cpp` (`struct KPlayer` nằm trong `KNpc::player`) | công thức của `jx_linux_y` (`LINUX-SERVER.md` §10.4): chính xác = nhanh nhẹn×4−28, phòng thủ = nhanh nhẹn/4, tay không = sức mạnh/5+1… |
| `Core/Src/KPlayerSet.h` (`KLevelAdd`: `level_exp.txt`, `level_add.txt`; `stamina.ini`) | `server/zone/…/KPlayerSet.h/.cpp` (`class KPlayerSet`) đọc `client/assets/player.json` của `jxassets export-player` (`services/pkg/jxold/player/KPlayerSet.go`) | `GetLevelExp(level, trùng sinh)`, `GetStaminaBase`, kháng theo cấp kẹp 120 khi hệ số âm |
| `KNpc::m_DamageRecord` + `KDamageRecord::Add` (JX2, `KNpc+0x1724`) | `KNpc::damage_records` / `add_damage_record`, `KSubWorld::share_experience` | 3 ô, 1200 khung; kinh nghiệm chia theo sát thương |
| `KPlayer::AddExp`, `CalcExp`, `AddExpTeam` (JX2) | `KPlayer::add_exp` / `calc_exp` (`KSubWorld::share_experience` gọi) | chia đội chưa làm |
| `CURPLAYER_SYNC`, `PLAYER_ATTRIBUTE_SYNC`, `PLAYER_LEVEL_UP_SYNC`, `c2s_playeraddattribute` | `PlayerAttribSync` (`G2C_PLAYER_ATTRIB`), `AddPointReq` (`C2G_ADD_POINT`); client `Game.player_attrib`, `UiStatus` | một gói cho mọi số của nhân vật |
| `Core/Src/KSkills.h/.cpp` (`KSkill`: `GetInfoFromTabFile`, `LoadSkillLevelData`, `ParseString2MagicAttrib`), `KSkillManager.h/.cpp` (`m_SkillInfo`, `m_pOrdinSkill[id][level]`, `InstanceSkill`) — bản JX2 `jx_linux_y` (`LINUX-SERVER.md` §11) | `server/zone/…/KSkill.h/.cpp` (`KSkillRow`, `KSkill::add_attrib` = `AddMagicAttrib 0x080EDCC0`, `KSkillTable`, `KSkillManager`), `KSubWorld::skills()`; `pkg/jxold/skill/KSkills.go` + `jxassets export-skills` → `client/assets/skills.json` | số theo cấp chạy chính `script\skill\*.lua` (Lua 5.4 đã chuyển) qua `GetSkillLevelData`, như bản cũ |
| `Core/Src/KNpc.cpp` phần đánh nhau (`ReceiveDamage`, `CalcDamage`, `AppendSkillEffect`, `SetStateSkillEffect`, `SetImmediatelySkillEffect`, `RemoveStateSkillEffect`, `KnockBack`, `OnHurt`, `ProcessState`), `KNpcAttribModify::MixPoisonDamage` — bản JX2 `jx_linux_y` (`LINUX-SERVER.md` §12) | `server/zone/src/KNpc.cpp` (hàm thành viên `KSubWorld`: `receive_damage`, `calc_damage`, `calc_resist`, `append_skill_effect`, `set_state_skill_effect`, `set_immediately_skill_effect`, `remove_state_skill_effect`, `set_poison`, `knock_back`, `do_hurt`, `process_frame_state`, `per_second_attribs`), `KNpc.h` (`KStateNode`, `KStateModifier`, biểu tượng trạng thái), test `test_KNpcDamage.cpp` | mỗi số tính như nhị phân (chia cắt, FPU cắt); hai ô `+0x1400/+0x1418` chéo nhau giữ nguyên |
| `Core/Src/KSkills.cpp` (`KSkill::Cast`, `CastInitiativeSkill`, `CastPassivitySkill`, `CreateMissleMagicAttribsData`), `KMissle::ProcessDamage` phần lên một mục tiêu | `server/zone/src/KSkills.cpp` (`KSubWorld::skill_cast`, `cast_initiative_skill`, `cast_passivity_skill`, `skill_start_event`, `create_missle_magic_attribs_data`, `swing_skill`, `on_skill`), `KSkill::basic_attack` (dòng 2/3 `Skills.txt` khi không có bảng/script) | style 0/14 gọi sang `KMissle.cpp` (đạn thật) |
| `Core/Src/KMissle.h/.cpp` (`KMissle`: `Init`, `Activate`, `OnFly`, `CheckCollision`, `CheckBeyondRegion`, `PrePareFly`, `ProcessCollision`, `ProcessDamage`, `DoCollision`, `DoVanish`, `GetMpsPos`; `KMissleSet::Add/Remove/Activate`; `g_MisslesLib` ← `missles.txt`), `KSkills.cpp` phần `CastMissles`/`CastWall`/`CastLine`/`CastExtractiveLineMissle`/`CastSpread`/`CastCircle`/`CastZone`/`OnMissleEvent`, `TOrdinSkillParam` — bản JX2 `jx_linux_y` (`LINUX-SERVER.md` §13) | `server/zone/…/KMissle.h` (`KMissleTemplate`, `KMissleTable`, `struct KMissle` cùng tên trường), `src/KMissle.cpp` (`KSubWorld::cast_missles`, `cast_wall/line/extractive_line_missle/spread/circle/zone`, `cast_child_skill`, `cast_instant_missle`, `create_missle`, `missle_add/remove`, `activate_missles`, `missle_frame/activate/prepare_fly/on_fly/check_beyond_region/check_collision/process_collision/process_damage/do_collision/do_vanish/event`, `KOrdinSkillParam`); Go `services/pkg/jxold/missle/KMissle.go` + `jxassets export-missles` → `client/assets/missles.json` | ô 32 tuyệt đối + lệch 1/1024 thay vùng/ô; `KMissleTable::basic_attack` (mẫu 64/65) khi không có bảng |
| `KNpc` danh sách kỹ năng tự động (JX2: `+0x182c..+0x18dc`, `+0x18ec`; `0x08189000`, `0x08188BB0`, `0x080821C0`), `KNpc::GetFreeSpot`-like `0x08081B70`, `KRegion::GetBarrier 0x080E0A30` — bản JX2 `jx_linux_y` (`LINUX-SERVER.md` §14) | `KNpc.h` (`KAutoSkillList`, `KAutoSkillEntry`, `KNpc::auto_skills`, `on_cast_skills`), `KNpcAttribModify.cpp` (272/195/196/274), `KNpc.cpp` (`KSubWorld::trigger_auto_skills`, `knock_back_free_spot`), `KSkills.cpp` (`cast_on_cast_skills`), `KMissle.cpp` (`barrier_kind`) | `step_length` (+0x129c) thay bán kính; gói 0x85 và hồi chiêu `KSkillList` là B3/B4 |
| `MultiServer/Bishop/PlayerCreator.cpp` (`newplayerini%02d.ini` → `TRoleData` mới) | `services/pkg/persist/filestore.go` `NewRole` + `player.Set.NewPlayerFor(series, sex)` | chỉ số = `series*2 + sex`; thiếu tệp → giới kia cùng hệ |
| `Core/Src/KNpcTemplate.h/.cpp` (`npcs.txt`) | `server/zone/…/KNpcTemplate.h/.cpp` (`KNpcTemplateSet`) | đọc `npcres/npcs.json` xuất từ bảng server |
| `Core/Src/KNpcAI.h/.cpp` (phần `_SERVER`, `ProcessAIType01..06`) | `server/zone/…/KNpcAI.h`, `src/KNpcAI.cpp` (`class KNpcAI`, `g_GenOneRelation`) | quái đánh trả; `KNpcSet::GenOneRelation`/`GetRelation` = `KSubWorld::relation` |
| `Engine/Src/KLuaScript.h/.cpp` (Lua 4.0: `Load`, `CallFunction`, `Include`) | `server/zone/…/KLuaScript.h/.cpp` (Lua **5.4** thuần) | không còn lớp tương thích: [SCRIPTS.md](SCRIPTS.md) |
| `Sources/Library/LuaLib/src` (Lua 4.0 nhúng, đã sửa: `in` không phải từ khoá) | `services/pkg/jxlua` + `services/cmd/jxlua` | bộ chuyển Lua 4 → 5.4, chạy một lần |
| — | `server/zone/src/luacheck.cpp` (`jx_luacheck`) | nạp cả cây script bằng chính Lua 5.4 của zone |
| `Engine/Src/KScriptCache.h` (`g_GetScript`) | `server/zone/…/KScriptCache.h/.cpp` | một trạng thái Lua cho mỗi file, nạp một lần |
| `KNpcTemplate::InitNpcLevelData` (+ `g_pNpcTemplate[id][level]`) | `KNpcTemplateSet::level_data` + cache trong `KSubWorld` | máu/sát thương/kinh nghiệm qua level script |
| `Core/Src/ScriptFuns.cpp` (`GameScriptFuns[]`, `GetPlayerIndex`) | `server/zone/…/ScriptFuns.h/.cpp` (`RegisterGameScriptFuns`, `KScriptContext`) | API script cho trap: `NewWorld`, `SetPos`, `GetFightState`… |
| `KRegion::LoadServerTrap` + `KNpc::CheckTrap` + `KNpc::ChangeWorld` | `KMapData::trap_at`, `KSubWorld::check_trap/execute_script/change_world_request`, `KGameServer::process_world_changes` | bẫy, cổng, nhiều map trong một zone |
| `KSubWorldSet` (nhiều `SubWorld[]`) | `KGameServer::worlds_` (`world_of_map`, `world_of_session`) | một `KSubWorld` cho mỗi map |

Sắp tới (chưa có, sẽ dùng đúng tên): `KSkillList` (kỹ năng của từng nhân vật),
`KMission`/`KPlayerTask`, `KPlayerTeam`, `KPlayerTong`, `KScriptValueSet` (Lua 5.4).

## Gateway / auth / DB (Bishop, PaySys, Goddess cũ → Go `services/`)

| Mã cũ | Mới |
|---|---|
| `MultiServer/Bishop` (gateway, tạo/chọn nhân vật) | `services/internal/gateway` (`gateway.go` server, `session.go` phiên, `zone.go` cầu zone, `KGatewayStats.go` đếm), `cmd/gateway` |
| `Sword3PaySys/S3AccServer` (`S3PAccount::Login`, `Account_info`) | `services/pkg/auth` (`S3PAccount.go` máy chủ tài khoản, `password.go` argon2id, `auth.go` giao diện + Options), `cmd/jxaccount` |
| `MultiServer/testAccServer` (công cụ tài khoản console) | `services/cmd/jxaccount` (`add`/`passwd`/`freeze`/`expire`/`list`) |
| `Bishop/LoginDef.h` (`LOGIN_R_*`), `S3Client/Login/Login.cpp`, `Ui/UiCase/UiConnectInfo.cpp` (`CI_MI_*`) | `proto/jx/common.proto` (`Result`), `client/net/KLogin.gd` (`result_text`, `session_ends`) |
| `MultiServer/Goddess` (DB nhân vật), `TRoleData` | `services/pkg/persist` (`FileStore`, `TRoleData.go` phiên bản + di trú; PostgreSQL sau) |
| `MultiServer/Rainbow` (relay client ↔ server, chỗ bắt gói của bản cũ) | `services/cmd/jxrecord` + `pkg/jxrec` (proxy ghi `.jxrec`, đọc lại bằng `dump`) |
| `MultiServer/Common/Buffer.h`, `IOBuffer.h` | `services/pkg/frame` |
| (mới) hàng gửi cho mỗi client | `services/internal/gateway/KSendQueue.go` | bỏ gói vị trí lạc hậu thay vì ngắt người chơi |
| `MultiServer/Common/SocketServer` + `S3Client/NetConnect` (chỉ TCP thô + xáo trộn riêng) | `services/pkg/transport` (`KListener.go` mở cửa tcp/tls/ws/wss, `KWebSocket.go` RFC 6455) |

## Tool đọc dữ liệu cũ (`services/pkg/jxold`, `cmd/jxassets`)

| Mã cũ | Mới |
|---|---|
| `Engine/Src/XPackFile.h/.cpp`, `KPakList` (hash `FileNameToId`) | `pkg/jxold/pak/XPackFile.go` |
| `Engine/Src/KSprite.h/.cpp`, `KDrawSprite.cpp` (RLE) | `pkg/jxold/spr/KSprite.go`, `XPackSprFrame.go` (spr nén theo frame), `KSpriteAtlas.go` (PNG) |
| `Core/Src/KSubWorld::LoadMap`, `Scene/KScenePlaceRegionC.cpp`, `SceneDataDef.h` | `pkg/jxold/wor/KSubWorld.go` |
| `Core/Src/KSkillManager.cpp` (`skills.txt`) | `pkg/jxold/npcres/KSkillManager.go` (bán kính/kiểu kỹ năng cho AI) |
| `Core/Src/KItem.h/.cpp` (`KItem`, `KItemCommonAttrib`, `m_aryBaseAttrib/RequireAttrib/MagicAttrib`), `KInventory.h/.cpp` (lưới ô), `KItemList.h/.cpp` (`m_Room[room_num]`, `m_EquipItem[itempart_num]`, `GetEquipPlace`, `Fit`, `CanEquip/EnoughAttrib`), `KItemGenerator.cpp` (`Gen_Equipment` dòng = `particular*10+level-1`, `Gen_Medicine`, `Gen_Quest`, `Gen_GoldEquip`) | `server/zone/…/KItem.h/.cpp`: `KItemTemplateSet`/`KItemLibrary` (JSON theo phiên), `KItem`, `KInventory`, `KItemList`, `KItemGenerator`; `KSubWorld::items_of(sid)`, lưu/nạp qua `RoleData.items` (`ItemData`) | túi 6×10, kho 6×10, giao dịch 10×4, 3 ô nhanh, 15 ô trang bị như cũ; `Gen_MagicAttrib` + `magicattrib_limit` theo nhị phân Linux (`KItemGenerator::gen_magic_attrib`, chỉ mục `magic_candidates` = `m_CMAIT`) |
| `Core/Src/ScriptFuns.cpp` `LuaDelItem`, `LuaHaveItem`, `LuaGetTaskItemCount`… (bản JX2 `jx_linux_y`: `DelItem/DelItemEx/HaveItem/GetItemCount[Ex]/HaveCommonItem/DelCommonItem/AddStackItem`) | `server/zone/src/ScriptFuns.cpp` (`l_DelItem`…) | theo nhị phân Linux: nhận tên questkey.txt, xoá cả chồng, Ex = chỉ túi |
| `Engine/Src/Text.cpp` (`TEncodeText`, `s_ColorTable`, `TGetEncodedTextLineCountAE`), `engineFree.dll` `g_StrWrap` | `client/ui/KTextEncode.gd` | thẻ `<color=…>` với bảng màu của client 2.0; `str_wrap` = `g_StrWrap` 40 ký tự |
| `Ui/Elem/MouseHover.cpp` (`KMouseOver`) của client 2.0 | `client/ui/uicase/UiMouseHover.gd` | mọi dòng căn giữa, ngắt 64, rộng ≥ 26 ký tự, `ALW_GetWndPosition` |
| `KItem::GetDesc` của client 2.0 (`gamecl.exe 0x636460`), `KLibOfBPT::GetMagicRange` (`0x63c260`) | `client/ui/KUiItemView.gd::describe_text`, `client/ui/KMagicRange.gd` | chuỗi `G_ITEM_*` của `stringtable_core.txt` |
| `Engine/Src/KRandom.h/.cpp` (`g_Random`, `g_RandomSeed`, `g_GetRandomSeed`) | `server/zone/include/jx/zone/KRandom.h` (`KRandom`) | cùng LCG 3877/29573 (`0x08226AD0` của bản Linux); mỗi bộ quay giữ seed riêng thay vì một seed toàn cục |
| `Core/Src/KBasPropTbl.h/.cpp` (`KLibOfBPT`, `KBPT_Equipment/Medicine/Quest/TownPortal/MagicAttrib_TF/Equipment_Gold`, `VMA_*`: `settings\item\*.txt` đọc theo số cột) | `pkg/jxold/item/KBasPropTbl.go` (`item.Set`, `item.Load`), `jxassets export-items` → `client/assets/items/{base,v000..}.json` | mỗi bộ (thư mục phiên bản) một JSON; chuỗi TCVN3/GBK trộn giải bằng `text.DecodeMixed` |
| `Core/Src/KObj.h/.cpp` (`KObj::Activate` đếm `m_nLifeTime`/`m_nBelongTime`, `SetItemBelong` 600), `KObjSet.cpp` (`Add`, `AddMoneyObj`, `ObjData.txt`, `MoneyObj.txt`), `KNpc::OnDeath/LoseMoney/LoseSingleItem` (Linux `0x08088B60/0x08081950/0x08088840`, `GenRandomItem 0x08083BB0`), `KPlayer::ServerPickUpItem` (`PLAYER_PICKUP_SERVER_DISTANCE` 40000), `c2s_playerpickupitem`, `c2s_playerthrowawayitem` | `server/zone/…/KObj.h/.cpp` (`KObjDataSet`, `KGroundObject`), `KNpcDropRate.h`, `KSubWorld::drop_item/drop_money/object_tick/pick_up_request/lose_treasure/gen_random_item`, `item_drop_request`; `pkg/jxold/item/KNpcDropRate.go`, `pkg/jxold/export/KObjExport.go` (`export-objdata`); client `scenes/KObj.gd`, `UiGame._pick_up`; `C2G_PICK_UP`, `EntityInfo.count` | vật thể trên đất là thực thể `ENTITY_DROP` qua hệ AOI; ma pháp/bạch kim khi rơi và chia đội chưa port |
| `Core/Src/ScriptFuns.cpp` `LuaAddItem`, `LuaAddGoldItem` (Linux `0x08120D30`, `0x0811F210`), `Core/Src/KGMCommand.cpp` (`TextGMFilter`, `?gm ds/dw`), `Engine KLuaScript::LoadBuffer/ExecuteCode` | `server/zone/src/ScriptFuns.cpp` `l_AddItem`, `l_AddGoldItem`; `KSubWorld::gm_command`; `KLuaScript::do_string` | `zone.gm_chat` chỉ máy phát triển; `zone.item_version` = `g_SubWorldSet+0x34` |
| `Ui/Elem/WndObjContainer.cpp` (`KWndObjectMatrix`, `KWndObjectBox`, `l_BgColors`, `BorderColorMap`), `Ui/Elem/Wnds.cpp` `Wnd_SetDragObj`, `Ui/UiCase/UiItem.cpp` (`随身物品.ini`), `Ui/UiCase/UiStatus.cpp` (`玩家装备与人物状态*.ini`), `Ui/Elem/MouseHover.cpp` (`弹出说明文字.ini`), `Core/Src/KMagicDesc.cpp` (`GetDesc`, `MAGIC_ATTRIB_STRING`) | `client/ui/elem/KWndObjContainer.gd`, `client/ui/KUiDraggedObject.gd`, `client/ui/uicase/UiItem.gd`, `UiStatus.gd`, `UiMouseHover.gd`, `client/ui/KMagicDesc.gd`, `KUiItemView.gd`, `KUiGameWindows.gd`; `pkg/jxold/export/KItemImageExport.go` (`export-item-images`), `pkg/jxold/item/KMagicAttribNames.go` | bố cục và ảnh lấy từ client 2.0 qua `export-ui`; tên thuộc tính từ nhị phân Linux |
| `Core/Src/KItemList.cpp` `ExchangeItem` (tay cầm: nhấc ô → đặt → cái bị đè lên tay), `EatMecidine` (Linux `0x08204710`: cấm thuốc, đếm thuốc, `ApplyMagicAttribToNPC(npc, 3)`, stack), `KNpcAttribModify::LifePotionV` (`0x08097E70`), khối thuốc của `KNpc::ProcessState` (`0x0808B7BC`), `KProtocolProcess::PlayerMoveItem/PlayerEatItem`, `s2c_syncitem/removeitem/playermoveitem/syncmoney` | `KSubWorld::item_move_request` (`KItemList::exchange` gộp hai bước), `item_equip_request`, `item_unequip_request`, `item_use_request`, `process_potions`, `send_item_list`, `give_item/take_item`; `KNpc::life_state`, `life_replenish_percent`, `forbid_medicine`, `potion_counter`; `KMapInstance::apply_client_packet` (`C2G_ITEM_*`), `G2C_ITEM_LIST/ADD/REMOVE/MOVE/RESULT`, `G2C_MONEY`; client `KProtocolProcess.items` + `item_move/equip/unequip/use/drop` | luật uống thuốc và hồi máu theo nhị phân Linux (`docs/LINUX-SERVER.md` §9); `Check_ItemUsable`/`OnUseItem` (Lua) và ngồi/đứng chờ M11 E |
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
| `Ui/UiShell.cpp` (UiStart, luồng trước khi vào game) | `client/scenes/UiShell.gd/.tscn` | cảnh chính; `UiLoginPlain.gd` khi chưa xuất dữ liệu game |
| `Ui/Elem/WndWindow`, `WndImage`, `WndButton`, `WndLabeledButton`, `WndText`, `WndEdit`, `WndList`, `WndShowAnimate` | `client/ui/elem/KWnd*.gd` | mỗi lớp một tệp, `init_from(ini, section)` đọc đúng các khoá của `Init` cũ; `KWndList` gồm cả lớp danh sách có ảnh dòng của 2.0 |
| `Ui/Elem/UiImage` (`KUiImageRef`) | `client/ui/KUiImage.gd` | atlas + bảng khung hình |
| `KIniFile` đọc bố cục | `client/ui/KUiScheme.gd` | `get_integer/get_integer2/get_color/image` trên `bo-cuc.json` |
| `Represent/iRepresent/Font/KFont2`, `KFontData`; `Engine/Src/KDrawFont` | `services/pkg/jxold/font/KFontData.go`, `export/KFontExport.go`, `client/ui/KFont.gd` | font ASF → BMFont hai lớp (nét + viền) |
| `Engine/Src/Text.cpp` (`TEncodeText`, bảng màu) | `client/ui/KText.gd` | bảng màu lấy từ `enginefree.dll` 2.0 |
| `Ui/UiCase/UiInit`, `UiSelServer`, `UiLogin`, `UiLoginBg`, `UiConnectInfo`, `UiSelPlayer`, `UiSelNativePlace`, `UiNewPlayer` | `client/ui/uicase/<cùng tên>.gd` | |
| `Login/Login.cpp` `GetServerRegionList`, `GetServerList`, `m_Choices` | `client/net/KLoginServer.gd`, `client/config/serverlist.json` | |
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
