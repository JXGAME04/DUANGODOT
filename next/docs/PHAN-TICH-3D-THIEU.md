# Phân tích bản 3D tham khảo (剑网江湖 V24.x) — JX NEXT hiện thiếu gì

Ngày 2026-09-19. Nguồn: mổ nhị phân 3 gói trong `D:\game3gTQ` (PC V24.2, APK V24.2, PC64 V24.1 — cùng một bộ dữ liệu):
**304 bảng excel** (`excel.bdd`), **245 script Lua** (`script.bdd`), 68 bundle (map, nhân vật, vũ khí, hiệu ứng, UI, âm thanh),
chuỗi/lớp IL2CPP (`global-metadata.dat`). Đối chiếu với JX NEXT theo `HANDOVER.md` §0.3/§0.4/§2/§3 và `MAPS.md` (bản `main` 2026-09-19).

Lưu ý khung nhìn: bản tham khảo là **MMO di động thương mại** dựng lại thế giới JX1 (10 phái, tên map/NPC/quái trùng JX1) —
nhiều hệ thống của nó là kiếm tiền (VIP, thương thành, sự kiện) chứ không phải gameplay JX1; phần "thiếu" dưới đây phân
biệt **thiếu so với JX1 gốc** (phải làm) và **có ở bản 3D nhưng JX1 không có** (tuỳ chọn).

## 1. Kết luận ngắn

| Nhóm | JX NEXT đã có | Thiếu rõ nhất (theo mức ưu tiên) |
|---|---|---|
| Lõi server, mạng, dữ liệu | mạnh nhất: zone C++ 980 map, đạn/sát thương/trạng thái/kỹ năng đúng từng lệnh, gateway Go, protobuf, Lua 5.4, PostgreSQL | — (bản tham khảo không có server để so) |
| Thế giới / map | 980 map 2D theo client 2.0, vật cản, cổng, NPC | **thế giới 3D** chỉ mới thử nghiệm 1 map; **minimap**, **vùng (an toàn/chiến đấu) + tên khu**, thời tiết/ánh sáng/sương, lời tải map |
| Nhân vật / hình ảnh | sprite 2.0 (npcres đang làm), hào quang, hiệu ứng trạng thái, đạn 2D | **trang bị hiện lên người**, **thú cưỡi**, **thời trang**, hoạt ảnh đánh/chết đầy đủ, vệt đao, hiệu ứng kỹ năng theo phái |
| Chiến đấu / kỹ năng | logic gần đủ (sổ kỹ năng, thi triển, hồi chiêu, tự động, đánh lùi, ẩn thân, tiếng) | **vùng cảnh báo kỹ năng** (skill_warning), **camera PVP**, mục tiêu/khung địch, hiển thị sát thương bay |
| Vật phẩm / trang bị | túi, chú thích 2.0, rơi/nhặt, ma pháp | kho đồ, giao dịch, **cường hoá/khảm/tinh luyện** (JX1 có), sạp hàng, đấu giá (3D có) |
| NPC / nhiệm vụ | trap/cổng, NPC đứng, hàm script đang port (M13) | **hội thoại NPC có cây chọn**, **nhiệm vụ chính/phụ + tự tìm đường tới mục tiêu**, hướng dẫn tân thủ, cửa hàng NPC |
| Xã hội | tài khoản, phiên, chat cơ bản | **chat kênh + biểu tượng**, **bạn bè, thư, tổ đội, bang hội** (M14), danh hiệu, xếp hạng, sư đồ, PK/đấu trường |
| Client kỹ thuật | Godot GL Compatibility, UI 2.0 (7 cửa sổ + cửa sổ game), e2e | **trình cập nhật (patcher)**, **hàng đợi đăng nhập**, đăng ký, **cài đặt hiển thị/game**, GM console, **âm thanh nền theo map**, bàn phím ảo, localization/font map |

## 2. Đối chiếu chi tiết theo hệ thống

Cột "3D có" ghi bằng chứng (bảng / script / bundle). Cột "giá" ước lượng công sức: nhỏ (≤ 2 ngày), vừa (≤ 1 tuần), lớn (> 1 tuần).

### 2.1 Thế giới, map, di chuyển

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Map | 46 map 3D (6 thành, 8 dã ngoại, 24 động, 8 phó bản), 65 scene id (`scn_list`), phó bản/VIP dùng lại map | 980 map 2D (`jxassets`), toàn bộ nạp trong 1 zone | 3D: chỉ thử nghiệm (`exp/3d-baling`); muốn 3D thật phải dựng map theo bố cục JX1 (46 map kia là 5 % thế giới) | lớn (nội dung) |
| Vùng trong map | `scn_area_list` 93 vùng: an toàn/chiến đấu/cổng, ưu tiên, nhạc nền, sương, hồi sinh | vật cản + trap; chưa có khái niệm vùng an toàn hiển thị | tên khu vực + đổi trạng thái chiến đấu khi qua vùng (JX1 có "an toàn/chiến đấu" theo map) | nhỏ |
| Đi lại | navmesh AIS (`pathfind.dll`), điểm đánh dấu (spawn, cổng, NPC) | A\* lưới 32×32 trên `obstacle.bin`, waypoint | tương đương; 3D đã dùng navmesh (`Scn3D`) | — |
| Minimap / bản đồ | `ui_map_view` (ảnh `*_big.png` + khung toạ độ), `ui_map.lua`, `ui_primary_map.lua` (radar nhỏ), `travel_mark` (dịch chuyển nhanh) | chưa (`KLittleMap` ghi ở MAPS.md §5) | minimap + bản đồ lớn + dịch chuyển nhanh | vừa |
| Ánh sáng / thời tiết / sương | `SceneRenderSetting` mỗi map (ambient, fog), `sl_fog_type` mê cung, lightmap | chưa (3d) | với 2D: lớp phủ ngày/đêm đơn giản; với 3D: đã có ở thử nghiệm | nhỏ (2D) |
| Tải map | `loading_tips`, loading image mỗi map (`loadingMap`) | có màn chuyển map? (chưa thấy trong docs) | màn loading + mẹo | nhỏ |
| Thú cưỡi | `cha_pic` 1500–1567 (ngựa, rồng), `anim_group` 20/21, `lua_scnobj_ride.lua` | chưa (JX1 có ngựa: `SetHorse` đã port server, client chưa) | client vẽ cưỡi ngựa (sprite 2.0 có sẵn khung cưỡi) | vừa |

### 2.2 Nhân vật, hình ảnh, trang bị lên người

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Model nhân vật | 197 `cha_pic` (xương + da body/head/shoes), 315 da, 716 clip; NPC đặt theo mark | sprite 2.0 (`npcres`: ghép đầu/thân/vũ khí theo hướng — đang làm, M15) | **trang bị hiện lên người** (2.0: ghép bộ phận theo item) — 3D làm bằng `model_list.skins` + `hangs` | vừa |
| Vũ khí trên tay | 71 vũ khí, điểm treo `HangItemMgr`, vệt đao (`anim_effect` → `sfx 290–293`) | chưa (2.0: sprite vũ khí trong `npcres`) | vệt đao khi đánh (2.0 có hiệu ứng đao quang riêng) | nhỏ |
| Thời trang / ngoại trang | `avatar_list` (thời trang, tường vân, pháp bảo, vũ khí ngoại trang) | không có trong JX1 | tuỳ chọn | — |
| Hoạt ảnh | nhóm theo vũ khí (nghỉ, đi, đánh 2 đòn, nội công, bị đánh, chết, động tác nhỏ), cưỡi | 2.0 có bộ khung đứng/đi/đánh; HANDOVER 3d: "hoạt ảnh đánh và chết" chưa xong | hoàn thiện bảng hành động → khung sprite, chết/hồi sinh | vừa |
| Danh hiệu, tên | `title_list/group/condition`, `player_name` (đặt tên ngẫu nhiên), `name_dictionary` (lọc tên) | tên nhân vật; chưa danh hiệu | danh hiệu (JX1 có), lọc tên xấu | nhỏ |

### 2.3 Chiến đấu, kỹ năng

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Dữ liệu kỹ năng | `skill_main` 344 (10 phái), `skill_section`/`skill_event` (sự kiện theo khung), `skill_hit`, `skill_levelup`, `skill_tree`, `cooldown_*`, `passive_skill_trigger` | `Skills.txt` 114 cột → `skills.json`, sổ kỹ năng, cây kỹ năng 2.0, hồi chiêu, tự động | logic đủ hơn bản 3D | — |
| Hiệu ứng kỹ năng | `skill_childobj` 317 vật thể (đạn/vòng/hào quang), `sfx_object` 129, `particles.bdd` 558 prefab | đạn 2D + hiệu ứng trạng thái từ sprite 2.0 | 3D: bộ chuyển đã có bản đầu (`export_sfx.py`, `Scn3DSfx`), cần chỉnh từng hiệu ứng + ánh xạ kỹ năng JX1 ↔ hiệu ứng | lớn |
| Cảnh báo vùng kỹ năng | `skill_warning` (vòng/quạt dưới chân trước khi trúng) | không có trong JX1 | tuỳ chọn (hay cho boss) | nhỏ |
| Trạng thái (buff) | `state_list`/`state_effect` (biểu tượng, hiệu ứng, sắp xếp `sys_state@buf`) | `ProcessState`, hiệu ứng trạng thái trên npc (B4e), biểu tượng hào quang | thanh buff trên UI (2.0 có `KUiSkillState` — đã làm B4b-4) | — |
| Mục tiêu / khung địch | `ui_enemy`, `ui_enemy_info`, `ui_target_info`, `ui_lifebar`, `boss_view` | thanh máu npc? (chưa rõ), chọn mục tiêu bằng chuột | khung mục tiêu + máu boss (2.0 có) | nhỏ |
| Camera PVP | `pvp_camera_modify` | không áp dụng 2D | — | — |
| Sát thương bay / số | (chuỗi UI) | `ui_showmsg`? chưa thấy số sát thương nổi | số sát thương/chí mạng nổi trên đầu (2.0 có) | nhỏ |
| AI NPC | `npc_fight_ai`, `npc_skill_ai`, `ai_group`, `cha_ai_event` | AI đuổi/đánh cơ bản (`KNpcAI`), auto skills đúng nhị phân | so được; thiếu AI theo nhóm/sự kiện (JX1 dùng script) | vừa |

### 2.4 Vật phẩm, trang bị, kinh tế

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Vật phẩm | `item_list` + 11 bảng (loại, nhóm, nguồn, kế thừa, ngẫu nhiên) | M11: bảng đúng cột, túi, chú thích 2.0, rơi/nhặt/vứt, ma pháp | so được | — |
| Trang bị nâng cấp | `equip_levelup/*` (theo màu), `equip_magicattrib`, `equip_potential`, `equip_suit`, `gem_list`, `refine_list`, `melt_*`, `enforce_*`, `recipe_*`, `combine_list` | chưa (M11 dồn: bạch kim/lỗ khảm, `AddItemEx`) | **JX1 có**: cường hoá, khảm ngọc, tinh luyện, hợp thành — theo bản Linux | lớn |
| Kho đồ, giao dịch, sạp | `ui_storage`, `ui_exchange`, `ui_stall`, `auction_*`, `ui_item_sell` | chưa (kho cần NPC) | kho, giao dịch trực tiếp (JX1); sạp/đấu giá (JX1 có sạp) | vừa |
| Cửa hàng NPC / thương thành | `npc_business`, `shop_list`, `shopitem_list`, `ui_shop`, `ui_spshop`, `rmb_buy`, `vip.lua` | chưa cửa hàng NPC | cửa hàng NPC (JX1); thương thành/VIP không thuộc JX1 | nhỏ |
| Tiêu hao / dùng vật phẩm | `use_list`, `use_consume` | `OnUseItem` chưa móc | dùng thuốc/vật phẩm → script (đang port) | nhỏ |

### 2.5 NPC, hội thoại, nhiệm vụ, hướng dẫn

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Hội thoại NPC | `talk_list`, `npc_talkstory_*` (cây thoại, chat ngẫu nhiên), `ui_talk`, `ui_select_dialog` | `Say/Talk` là stub (MAPS.md) | **hội thoại có lựa chọn** = `Say/Talk/AddOption` của script JX1 (M13) | vừa |
| Nhiệm vụ | `task_main/child`, `quest_*`, `ui_primary_task`, `ui_award_task`, `target_guide` (bấm → tự chạy tới NPC/map) | M13 chưa | nhật ký nhiệm vụ 2.0, **tự tìm đường** (2.0 không có nhưng rất đáng thêm) | lớn |
| Tân thủ / hướng dẫn | `newbie_list`, `ui_newbie_target/pass`, `story_*` | chưa | tuỳ chọn | vừa |
| Vật thể tĩnh tương tác | `still_list` (rương, cờ, bù nhìn) | vật thể map 2D tĩnh | rương/điểm thu thập có script (JX1 có) | nhỏ |
| Hoạt động / phó bản | `activity`, `activity_copy`, `copy_list`, `clonebattle_*`, `ladder_*` (đấu trường), `zyt_*`, `xianjie_*`, `dance_*` | chưa (JX1: Tống Kim, Phong Lăng Độ, Bạch Hổ Đường, Thiên Lâm) | các hoạt động JX1 theo script bản Linux | lớn |

### 2.6 Xã hội

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Chat | kênh (`chat_item_zone`), biểu tượng cảm xúc (`chat_icon`), `ui_mainchat`, `ui_chat_large`, gửi vật phẩm vào chat | chat cơ bản trong `UiGame` | kênh (thế giới/bang/đội/mật), biểu tượng, link vật phẩm | vừa |
| Bạn bè / thư | `lua_friend`, `ui_friend_*`, `ui_mail` | chưa (M14) | bạn bè, thư (JX1 có thư nhắn) | vừa |
| Tổ đội | `team_*` 5 bảng, `ui_primary_team`, chia kinh nghiệm đội | server chưa (`AddExpTeam` đã đọc), client chưa | tổ đội (M14) | vừa |
| Bang hội | 13 bảng (hoạt động, boss bang, quyên góp, cây tiền), `ui_guild*` | chưa | bang hội JX1 (bản Linux có `guild` script) | lớn |
| Sư đồ, đồng hành | `master_*` 10 bảng, `accompany_list` | không có trong JX1 (JX1 có sư đồ đơn giản) | tuỳ chọn | vừa |
| Xếp hạng / thành tích | `rank_data`, `record_*`, `ui_datarank`, `kill_honor`, `warcraft_honor` | chưa | bảng xếp hạng (JX1 có) | nhỏ |
| PK / đấu trường | `activity_pvp`, `ladder_*`, `qiecuo.lua` (khiêu chiến), `ui_qiecuo_duzhu` (cược) | PK mode server có (`pkmode`), client chưa | luật PK/đỏ tên JX1 (server), thách đấu | vừa |

### 2.7 Client kỹ thuật, vận hành

| Hạng mục | 3D có | JX NEXT | Thiếu / ghi chú | Giá |
|---|---|---|---|---|
| Cập nhật client | `ui_updater*` (tải bundle theo version, `url_list`) | chưa | **patcher** (bắt buộc khi phát hành) | vừa |
| Đăng nhập | `ui_login`, `ui_login_regist` (đăng ký), `ui_login_queue` (hàng đợi), chọn máy chủ theo khu (`serverlist`) | luồng 7 cửa sổ 2.0, chọn máy chủ | đăng ký trong game, hàng đợi khi đầy | nhỏ |
| Cài đặt | `ui_cfg_display/game/main`, `ui_setting`, `mDeviceList` (chất lượng theo máy) | chưa (U6) | cài đặt hiển thị/âm thanh/game | nhỏ |
| Âm thanh | `audio.bdd` 181 MB, `sound_list_bgm` theo map/vùng, `sound_group`, tiếng UI | tiếng thi triển/đạn/hành động (B4f) | **nhạc nền theo map**, tiếng UI, tiếng môi trường | nhỏ |
| Giao diện | NGUI (`gui.bdd` 11 410 đối tượng, atlas 146 MB), tween, bàn phím số ảo, rocker (di động) | Godot Control, bố cục 2.0 xuất từ `.ini` | so được (khác nền tảng) | — |
| GM | `ui_gm*`, `gm_type` | `?gm ds <lua>` trong chat | bảng lệnh GM có giao diện (O3) | nhỏ |
| Đa ngôn ngữ | `localization`, `text_list*`, `string_unity_auto`, `font_map` | chuỗi TCVN3 → UTF-8 của 2.0 | bảng chuỗi tập trung (D1 có phần) | vừa |
| Thiết bị | Android APK 1,6 GB (cùng bundle), `快捷键.txt` PC | Godot xuất Windows/Linux/Android/web (ADR-007) | rocker/điều khiển cảm ứng cho di động | vừa |
| Chống gian lận / bảo vệ | bundle mã hoá (ArchiveStorage + khoá suy từ IL2CPP), tên tệp/asset băm md5 | chống gói rác ở gateway | bảo vệ dữ liệu client (tuỳ mức) | nhỏ |

## 3. Cái bản 3D **không** có mà JX NEXT có (giữ, không phải thiếu)

- Server thật: zone 980 map, 110 730 NPC, tick 1,6 ms; gateway TLS/WS; đo tải tới 13 000 người; PostgreSQL; kiểm thử 222 ctest + 417 test client + e2e.
- Logic chiến đấu/kỹ năng/vật phẩm đúng từng lệnh của bản Linux (bản 3D là client, không biết server nó thế nào).
- Bố cục UI đúng điểm ảnh client 2.0 (bản 3D là UI di động).

## 4. Đề xuất thứ tự (theo giá trị / công sức)

1. **Client cơ bản còn thiếu, giá nhỏ–vừa**: minimap + bản đồ, khung mục tiêu/máu boss, số sát thương nổi, nhạc nền theo map, cài đặt, hàng đợi/đăng ký, patcher.
2. **Gameplay JX1 theo bản Linux (M13–M14)**: hội thoại NPC + nhiệm vụ (thêm tự tìm đường), cửa hàng/kho/giao dịch, tổ đội, chat kênh, bạn bè/thư, bang hội, xếp hạng, cường hoá/khảm.
3. **Hình ảnh nhân vật 2.0**: npcres ghép trang bị lên người, hoạt ảnh đánh/chết, cưỡi ngựa, vệt đao.
4. **Hướng 3D** (nếu chủ dự án chốt bằng ADR): pipeline đã có ở `exp/3d-baling` (map, NPC, vũ khí, kỹ năng, navmesh, tên 2D);
   việc còn lại là **nội dung** (dựng map 3D theo bố cục JX1, model nhân vật) và bước tích hợp UI + zone vào thế giới 3D.

Bảng dữ liệu của bản tham khảo đáng "mượn cách tổ chức" (không mượn dữ liệu): `scn_list`/`scn_area_list`/mark JSON (map),
`skill_childobj`/`sfx_object`/`anim_effect` (hiệu ứng theo sự kiện khung), `target_guide` (tự tìm đường), `loading_tips`,
`sound_list_bgm` theo vùng.

## 5. Để JX NEXT thành game **3D chuẩn** còn thiếu gì (đối chiếu với bản tham khảo)

Trạng thái thử nghiệm `exp/3d-baling` (đã chạy được): 1 map 3D + lightmap + nắng thời gian thực, camera quỹ đạo, 494 NPC/quái 3D có
skin + animation + tên Việt, nhân vật chính 3D với 71 vũ khí treo tay và 30 clip theo vũ khí, vệt đao, 3 hiệu ứng kỹ năng mẫu, đi lại theo
navmesh (không xuyên nhà). **Chưa nối server, chưa có UI game, chưa có nội dung ngoài 1 map.** Còn thiếu, theo thứ tự nên làm:

| # | Thiếu | Bản tham khảo làm thế nào | Việc cụ thể cho JX NEXT | Giá |
|---|---|---|---|---|
| 1 | **Quyết định hướng** | — | ADR mới thay ADR-001/007 (client 2D → 3D), giữ zone/gateway/giao thức | nhỏ |
| 2 | **Tích hợp thế giới 3D vào client thật** | Unity scene `Game` + Lua UI | `UiGame3D`: đăng nhập 2.0 → nhận `EntitySpawn/Move/Action` của zone → nhân vật/NPC 3D (`Scn3DNpc`), toạ độ `X = x, Z = y`, cao độ từ địa hình; lớp "vẽ thế giới" sau một giao diện (đã đề xuất) | lớn |
| 3 | **Map 3D cho thế giới JX1** | 46 map dựng tay trong Unity + Bakery lightmap + navmesh | dựng map 3D theo bố cục 980 map 2D (ưu tiên Phượng Tường và lộ trình tân thủ), pipeline Blender → glTF → nướng lightmap; navmesh → rasterize `obstacle.bin` cho zone | **rất lớn (nội dung)** |
| 4 | **Model nhân vật/quái 3D của JX1** | 197 bộ xương + 315 da, 716 clip | model 5 phái × nam/nữ, ~200 quái/NPC JX1, animation theo vũ khí (dùng bảng hành động cũ làm chuẩn); tạm thời có thể dùng sprite 2.0 billboard 8 hướng (khoá pitch 30°) | rất lớn (nội dung) |
| 5 | **Trang bị lên người 3D** | `model_list.skins` (body*head*shoes) + `hangs` | phần da theo trang bị (ghép theo `item_list` như npcres 2.0), vũ khí treo tay (đã có), thú cưỡi (`anim_group` 20/21, `ma_qi1`) | lớn |
| 6 | **Hiệu ứng kỹ năng 3D** | 558 prefab particle + `skill_childobj/sfx_object/anim_effect` + `SFXXWeaponAnim` | hoàn thiện bộ chuyển (stretched billboard, trail module, UV cuộn, blend mul), bảng ánh xạ 10 phái × kỹ năng JX1 → hiệu ứng, sự kiện theo khung từ zone (`EntityAction.skill_id` → hiệu ứng đúng thời điểm) | lớn |
| 7 | **Camera & hiển thị** | `cameraInit` mỗi map, `CameraBuildingFade`, `pvp_camera_modify` | làm mờ/ẩn nhà chắn camera, va chạm camera với nhà, tuỳ chọn pitch, FOV theo map | vừa |
| 8 | **Vật liệu & môi trường** | shader riêng (17 loại: nước 2 sóng, lá/cỏ đung đưa, địa hình splat, alpha test, rim) | shader nước, lá cỏ đung đưa, thời tiết/ngày đêm, sương theo `scn_area`, `LightmapGI` khi map do ta dựng | vừa |
| 9 | **Tương tác trong thế giới** | pick bằng ray, `sys_bd` hit point, `cha_model_view` kích thước | bấm chọn NPC/quái (Area3D theo `cp_radius`), thanh máu + tên 2D (đã có tên), số sát thương nổi, vật phẩm rơi 3D (`still_list`), cổng/điểm dịch chuyển hiện hình | vừa |
| 10 | **Hiệu năng & tải map** | bundle theo map, texture dùng chung, LOD | tải/giải phóng map khi chuyển, LOD/`visibility_range`, occlusion, texture nén (KTX/BC), giới hạn draw call; đo lại với GL Compatibility trên máy yếu + Android | vừa |
| 11 | **Âm thanh 3D** | `AudioSource` theo vị trí, BGM theo vùng | `AudioStreamPlayer3D` cho đòn/đạn (đã có tiếng 2D), nhạc nền theo `scn_area` | nhỏ |
| 12 | **Công cụ & quy trình** | Unity Editor + bộ xuất "SceneMakeRes" | quy ước glTF (đơn vị mét, +Z tới), kiểm tra tự động (`--auto` chụp + FPS), tài liệu dựng map cho họa sĩ, validator dữ liệu 3D trong CI | vừa |

Tóm lại: **kỹ thuật** để chạy 3D trong Godot đã chứng minh xong ở thử nghiệm (mục 2, 6, 7, 9 là việc lập trình vừa/lớn);
cái quyết định là **nội dung** (3, 4, 5) — map và model 3D cho toàn bộ thế giới JX1 — và **quyết định hướng** (1).
Bộ tham khảo chỉ dùng được làm chuẩn kích thước/cách tổ chức, không dùng được làm tài sản phát hành.
