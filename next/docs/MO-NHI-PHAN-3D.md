# Lịch trình mổ nhị phân bản 3D (剑网江湖) cho client 3D của JX NEXT

Lập 2026-09-19 theo yêu cầu chủ dự án: *"lên danh sách lịch trình mổ nhị phân từ các bản 3D để làm… làm cho tới lúc hoàn
thiện theo bản 3D mổ nhị phân; chưa có UI, chưa có đầy đủ hình ảnh skill; full skill các phái, đánh quái được"*.

Tài liệu này là **danh sách việc mổ** (mỗi hạng mục: nguồn nhị phân, cách mổ, đầu ra trong repo, trạng thái) và **thứ tự
làm**. Lộ trình sản phẩm vẫn là [LO-TRINH-3D.md](LO-TRINH-3D.md); quy ước toạ độ [3D-QUY-UOC.md](3D-QUY-UOC.md); quyết định
[ADR-008](adr/ADR-008-client-3d.md): **hiển thị theo client 2.0, luật theo server Linux, hình 3D lấy từ bản tham khảo**.
Mọi con số ghi vào mã phải có nguồn `[TK]` (bản tham khảo), `[2.0]` (gamecl.exe), `[Linux]` (jx_linux_y) hoặc ghi rõ
**[tự chọn]**. Nhật ký từng phần vẫn ở [HANDOVER.md](HANDOVER.md) (phần "3D-nn").

## 0. Nguồn và công cụ

| Nguồn | Ở đâu | Cách đọc |
|---|---|---|
| Bản tham khảo PC V24.2 (Unity 2022.3, IL2CPP) | `D:\game3gTQ` (gói), `D:\game3gtQ_mo\pc\剑网江湖_Data` (đã giải nén) | 68 bundle `StreamingAssets\*.bdd` (tên = md5(tên + ".bdd")[:12], khoá giải mã trong `D:\game3gtQ_mo\khoa_bundle.txt` — **không đưa vào repo**); UnityPy qua `tools/scn3d/*.py` |
| Bản APK V24.2 (arm64) | `D:\game3gtQ_mo\apk`, `meta_apk.txt` | đối chiếu khi PC thiếu; cùng bảng, cùng script |
| IL2CPP metadata + mã máy | `D:\game3gtQ_mo\meta_pc.txt(.json)`, `meta_pc_res.txt` (134 945 dòng: lớp, hàm, RVA, giá trị tĩnh `fieldDefaultValues`), `cs_types.txt` (1 772 lớp), `strlit_pc.txt` (15 800 chuỗi) | `python D:\game3gtQ_mo\disasm.py "Class.Method"` hoặc `rva:0x…` (Capstone x64) |
| Script Lua uLua của bản tham khảo | bundle `script` (`fd1fb5b437c8.bdd`): **245 tệp `.lua` văn bản (2,6 MB)** → trích ra `D:\game3gtQ_mo\lua\` (ngoài repo) | đọc trực tiếp; đây là nơi logic UI + kỹ năng + cưỡi ngựa + nhặt đồ… của client tham khảo nằm |
| Bảng dữ liệu | bundle `excel` (`c96f69a275af.bdd`): **240 bảng** TextAsset UTF-16 (`<tên>_cmn` = dữ liệu, `<tên>` = schema) | `export_npc.Tables` |
| Client 2.0 | `gamecl.exe` + `reslst.dat` (ini UI, settings) — xem [CLIENT-2.0.md](CLIENT-2.0.md) | `tools/re` |
| Server Linux | `jx_linux_y` — xem [LINUX-SERVER.md](LINUX-SERVER.md) | `re_elf`, `re_calls` |

### 0.1 Bản đồ bundle của bản tham khảo (tên đã giải, 68 tệp)

| Tên gốc | Tệp | Cỡ | Nội dung | Đã dùng bởi |
|---|---|---:|---|---|
| `excel` | c96f69a275af | 2,0 MB | 240 bảng (501 TextAsset) | `export_npc.Tables`, `map_skills.py` |
| `scenes` | 3055752669ff | 117 MB | 410 texture dùng chung của scene | `export_scene.py` |
| `scenes_<tên>` ×46 | (theo `scn_list`) | 946 MB | 46 map 3D (mesh, lightmap, mark, navmesh tham chiếu) — xem `D:\game3gtQ_mo\BAO-CAO-MAP-3D.md` | `export_scene.py` (mới xuất `world_baling`) |
| `scenesnav` | f44380d04cd8 | 1,6 MB | 104 TextAsset: 47 navmesh AIS + 57 mark JSON | `make_map3d.py` |
| `expscnmesh` / `expscnmat` | 42f7752ddd5a / 6fbf37a4e80c | 102 / 0,1 MB | 1 410 mesh + 516 vật liệu thư viện scene | `export_scene.py` |
| `models` | 74ffea3407a5 | 49 MB | 236 texture nhân vật/vũ khí | `export_npc.py` |
| **nhân vật (prefab)** | 9e371715490e | 0,5 MB | 4 337 GameObject, 124 Animation, `HangItemMgr`, `SFXXWeaponAnim` | `export_npc.py` |
| **nhân vật (mesh / vật liệu / ref)** | d4b79c319044 / c8a4785ccffb / b78d0423da10 | 12 / 0 / 0 MB | 278 mesh, 233 vật liệu, 315 `ChaResourceRef` | `export_npc.py` |
| **animation** | 19b6b49a7124 | 18,7 MB | 716 AnimationClip | `export_npc.py` (nhóm 1–11, 20, 21) |
| **vũ khí (prefab)** | 8dd679aa97a2 | 2,3 MB | 519 GameObject, 173 mesh, `SFXMeshModify`, `SFXXWeaponAnchor` | `export_weapon.py` (71 glTF) |
| **hiệu ứng (particles)** | 1d472c44c423 | 9,3 MB | 1 455 GameObject, **315 ParticleSystem**, 449 MeshRenderer, `SFXBillboardHelper`, `SFXLineMesh`, `SFXMixerMesh`, `SFXMeshTrailDrag`, `PC2Anim` | `export_sfx.py` (273/558 prefab) |
| **UI prefab (NGUI)** | f799b063c668 | 0,8 MB | 3 085 GameObject, 4 340 MonoBehaviour (UIPanel/UISprite/UILabel…), 614 BoxCollider2D | chưa |
| **UI atlas + font** | 9431897d8969 | 146 MB | 117 texture, `UIAtlas`, `UIFont`, `ColorDefine`, 8 RenderTexture | chưa (UI theo 2.0) |
| `shader` + hậu kỳ | ba01262372a0 + c33674c0bad2 | 0,4 + 2,0 MB | 36 shader, `PostEffectBloom/RadialBlur/Distortion`, `GrabPassFeature` | đọc float vật liệu (nước, cỏ) |
| `audio` (nhạc nền) | 57c14efc7dfa | 181 MB | 75 AudioClip + AudioMixer | không dùng (âm thanh theo 2.0) |
| tiếng động | b709e92b70fc | 5,7 MB | 375 AudioClip (`sound_list_sound` 224 dòng) | không dùng (âm thanh theo 2.0) |
| `script` | fd1fb5b437c8 | 0,8 MB | 245 Lua văn bản | **mới trích** (2026-09-19) |
| `common` | 51516289e994 | 0 | rỗng | — |

## 1. Danh sách hạng mục mổ

Ký hiệu trạng thái: **xong** · **một phần** (ghi rõ còn gì) · **chưa** · *bỏ* (không cần vì hiển thị theo 2.0).
Cột "LT" = bước trong LO-TRINH-3D.

### A. Bảng dữ liệu hiển thị (bundle `excel`)

| # | Bảng (dòng) | Cột quan trọng | Dùng cho | Đầu ra | Trạng thái | LT |
|---|---|---|---|---|---|---|
| A1 | `cha_pic` (195) | xương, giới, scale, nhóm anim, skins, model | model nhân vật/NPC/ngựa | `assets3d/npc/npc_models.json` | **xong** (41 model đã xuất cho Ba Lăng; xuất thêm theo map) | 1.2 |
| A2 | `anim_group` (24), `animation_list` (60) | clip xx/zp/gjxx/gjpb/xdz/ss/sw/đánh/nội công theo nhóm | KNpc3DView chọn clip theo `doing` | trong `npc_models.json` (`groups`) | **xong** nhóm 1–11, 20 (cưỡi), 21 (ngựa) | 1.2 |
| A3 | `anim_effect` (17) | clip đánh → `sfx_object` (vệt vũ khí theo phẩm chất: 290 trắng, 291 xanh…) | vệt kiếm khi đánh thường | `Scn3DTrail.apply_params` + `TRAIL_BY_COLOUR` | **xong** (3D-49): phẩm chất theo màu tên 2.0; vệt NPC (dòng 10–14) **chưa** | 3.1 |
| A4 | `model_list` (63), `model_hang_list` (96) | điểm treo mặc định, loại vũ khí 1–10, nhóm anim khi cầm, `anim_effect` | vũ khí trên tay + đổi nhóm animation | `assets3d/weapon/weapons.json` | **xong** (71 vũ khí, `animgrp`) | 3.3 |
| A5 | `cha_model_view` (118) + `cha_pic` cột 5/14/15/18 (bán kính chân, khối chọn cầu/hộp, kiểu chết) | cỡ trụ chọn mục tiêu, cao thanh tên | `npc_models.json pick/death/foot_radius`, `sizeY` | **xong** khối chọn (3D-54); hạt gắn thân (`cha_model_view` cột 4) chưa | 3.1 |
| A6 | `skill_main` (342), `skill_section` (358), `skill_event` (828), `skill_childobj` (315), `sfx_object` (127) | tên Hán = tên JX1; sự kiện 8 (vật con) / 120 (sfx); vật con: đường dẫn, khung sống, độ cao, điểm treo, kiểu vị trí, đồng bộ (4 bay tới đích), tốc độ, hiệu ứng trúng | hiệu ứng ra chiêu / đạn / trúng / nội công | `assets3d/sfx/skill_map.json` | **xong đợt 1** (3D-47): 234 kỹ năng JX1 ghép (8 còn lại = bản • nâng cấp + 闪避), nhiều đoạn, vật con lồng nhau, AddState → hào quang; loại sự kiện đọc từ `eSkillEventType` | 3.1 |
| A7 | `state_list` (144), `state_effect` (73) | vật con/sfx của trạng thái (hào quang), màu thân (CCCC44 chậm), đổi cỡ, ẩn thân | hào quang, buff | `skill_map.json aura`, `skill_of_special` | **một phần**: hào quang xong; `state_effect` màu/ẩn *bỏ* (2.0 có `state_gfx`) | 3.1 |
| A8 | `skill_hit` (28) | kiểu cứng đờ (1 bị đánh, 2 đánh lui, 3 hút), sfx trúng, âm | phản ứng khi trúng | — | *bỏ* luật (theo Linux/2.0: `KNpc::OnHit`, đánh lui B4c-4); **chưa**: sfx trúng dự phòng khi vật con không ghi | 3.1 |
| A9 | `skill_warning` (15) | vòng cảnh báo AOE: trễ, thời gian, nháy, mờ | vòng cảnh báo trên đất | — | **chưa** (2.0 không có; [tự chọn] có làm hay không) | 3.1 |
| A10 | `still_list` (101) | vật rơi 1–300: model `drop_tongqian01`, anim rơi, âm, cỡ va chạm | vật rơi 3D thay billboard ObjData | — | **chưa** (hiện vật rơi = ảnh ObjData 2.0 billboard) | 1.4 |
| A11 | `cha_list` (214) | tên Hán → template JX1 (Hán-Việt trùng tên / từ điển loài / NPC dự phòng; loại boss theo `LifeParam`) | `map_npcs.py` → `cha_templates.json` | `make_map3d.py` | **xong đợt 1** (3D-54): 30 tay + 68 tự động; 62 nhân vật có tên riêng (boss, cốt truyện) chưa | 1.3 |
| A12 | `npc_fight_ai` (16), `npc_skill_ai` (54) | AI quái | — | *bỏ* (AI theo Linux) | — |
| A13 | `scn_list` (65), `scn_area_list` (93) | scene: đường dẫn bundle, camera `scn_list`; vùng: an toàn, nhạc `sound_group`, sương mù/ambient/đèn đổi theo vùng, camera theo vùng, bóng 45 m, tiếng bước chân | camera map, vùng an toàn, nhạc vùng (3.5), sương mù theo vùng | `map3d.json camera` | **một phần**: camera map xong; vùng (nhạc/sương/đèn) **chưa** | 1.1, 3.5 |
| A14 | `ui_map_view` (63) | ảnh minimap `world_baling_big.png`, tỉ lệ, toạ độ thế giới góc trái-dưới/phải-trên | minimap 3D | `make_map3d.export_minimap` → `map3d.json minimap` | **xong** (3D-50; ảnh chỉ trong assets3d) | 3.4 |
| A15 | `mark_list` (103), `trans` (166), `stop_flag` (54) | điểm đánh dấu, cổng dịch chuyển giữa scene | bẫy/cổng | `make_map3d.py traps` | **xong** (ExitPoint → bẫy) | 1.4 |
| A16 | `sound_group` (16), `sound_list_sound` (224), `sound_list_bgm` (74) | tiếng thi triển theo giới, nhạc nền | — | *bỏ* (âm thanh theo 2.0: `action_sounds.json`) | — |
| A17 | `pvp_camera_modify` (2) | camera PK: vị trí/góc/thời gian | — | *bỏ* | — |
| A18 | `define` (73) | hằng gameplay | — | *bỏ* (luật theo Linux) | — |
| A19 | `item_list` (1 355), `avatar_list` (73) | icon, thời trang/pháp bảo | — | *bỏ* (icon theo 2.0) | — |
| A20 | `sfx_ui_object` (7) | hiệu ứng UI (nâng cấp trang bị) | — | *bỏ* | — |

### B. Model, animation, điểm treo (bundle nhân vật / animation / vũ khí)

| # | Việc | Nguồn | Cách mổ | Đầu ra | Trạng thái |
|---|---|---|---|---|---|
| B1 | Model nhân vật + xương + skin + hang point | prefab 9e371715490e, mesh d4b79c319044, `ChaResourceRef` | UnityPy → glTF (đã) | `assets3d/npc/*.gltf` (41) | **xong** cho Ba Lăng; các map khác chạy lại `export_npc.py --map` |
| B2 | Animation 716 clip | 19b6b49a7124 | ghép theo `anim_group` | trong glTF | **xong** nhóm dùng; nhóm 12–19 (NPC đặc biệt/boss) khi cần |
| B3 | Vũ khí 71 + điểm treo `daojian/qianggun/ssdaochui/ssqt/sys_*`; `Player.UpdateModelLogic 0x4fbc60` + `GameNodePool.TryInstantiateNode 0x6eb770`: prefab vào bản lề với local 0/identity/1 → **bỏ transform gốc prefab** (mọi gốc −90° X, 15 gốc lệch −22 m) | 8dd679aa97a2 + IL2CPP | UnityPy | `assets3d/weapon` | **xong** (sửa 3D-57) |
| B4 | `XWeaponTrail` (MaxFrame 5 / Fps 30 / Granularity 15 / màu / ô atlas) | bố cục byte 156 của 16 prefab `dg_xw_*` | `export_sfx.read_xtrail` | `Scn3DTrail` | **xong** (3D-49); spline Granularity (làm mượt) [tự chọn] chưa |
| B5 | `HangItemMgr`, `ModelHangMgr`, `eEquipHangType` (treo mũ/áo/phi phong/vũ khí), `RideUnit.CreateRide 0x5bd930` (người vào bản lề `ma_qi1` với transform đơn vị), `AnimStator.UpdateAutoGroup 0x4d6210` (cưỡi → nhóm 20) | IL2CPP | disasm | `Scn3DNpc.hang_node/attach_weapon`, `KNpc3DView.set_weapon_group` | **một phần**: vũ khí (kể cả điểm treo trên xương) + ngựa đúng (3D-56); phi phong **chưa** (JX1 có phi phong) |
| B6 | `RideUnit`, `eRideType`, `lua_scnobj_ride.lua` | IL2CPP + Lua | đọc Lua (điểm `ma_qi1`, nhóm 20/21) | `KNpc3DView._on_riding_changed` | **xong** (cần soát lại theo Lua vừa trích) |
| B7 | `ShadowProjMgr` = `DynamicShadowProjector` (bóng hình thật, 45 m) | IL2CPP | đã đọc | mức low: đĩa mờ `_add_blob_shadow` | **xong** (3D-52) [tự chọn đĩa] |
| B8 | `TaskTweenDissolve` (tan xác khi chết) | IL2CPP + shader dissolve | disasm | shader tan | **chưa** ([2.0] xác nằm rồi mờ dần — làm theo 2.0: mờ) |

### C. Hiệu ứng kỹ năng (bundle particles + bảng A6/A7)

| # | Việc | Nguồn | Cách mổ | Đầu ra | Trạng thái |
|---|---|---|---|---|---|
| C1 | Xuất prefab hạt: ParticleSystem (module chính, shape, color/size over lifetime, texture sheet), MeshRenderer, Light, Animation, **tween NGUI** | 1d472c44c423 | UnityPy → JSON + glTF | `assets3d/sfx/*.json/.gltf` | **xong 343/343** prefab gốc (3D-47) |
| C2 | Ai gọi prefab nào: 280 từ bảng (childobj 152, sfx_object 122, state_list 63, anim_effect 15, sfx_ui 7, skill_hit 3), 63 từ mã/Lua (`cmn_select`, `cmn_droplight_*`, `cmn_rocker_*`, `cmn_chuansong`, `cmn_ma_tui_*`, `hg_*`) | như trên | `export_sfx.py --all` duyệt prefab gốc + đoán thư mục theo hash | `sfx_index.json` | **xong** (3D-47); dùng 63 prefab mã gọi: **chưa** (vòng chọn, vật rơi, bụi ngựa) |
| C3 | `SFXMixerMesh` (27 node / 18 prefab: hào quang phái, bẫy Đường Môn, dịch chuyển) | bố cục `MixLayer` + disasm .cctor/InitMeshData/FillMeshSquare/UpdateMixData + GLSL `sfx_mixer_mesh_rs` | `mixer_mesh.py`, `export_sfx.read_mixer` | `Scn3DSfx._add_mixer` + `scn3d_mixer.gdshaderinc` | **xong** (3D-48) |
| C4 | `SFXMeshTrailDrag` (3 prefab đao quang `dg_daoqing_*`), `SFXLineMesh` (1: 天机迅雷), ~~`Trail` (5: phi tiêu/phi đao)~~ **xong 3D-60** (`Scn3DPointTrail`), `SFXXWeaponAdapter` (10 hào quang vũ khí `hg_*`) | IL2CPP | disasm | `Scn3DSfx` | **một phần** (Trail xong; còn 14 prefab) ; mask-dissolve **không dùng** (`useMask` = 0 ở 418 `SFXMeshModify`, 3D-57) |
| C5 | `PC2Anim` (hoạt ảnh đỉnh `.pc2`: 1d472c44c423.bdd__model_zhuixinjian.pc2, 1 prefab 追心箭) | bundle + IL2CPP `PC2Anim.Update` | đọc định dạng pc2 (Point Cache 2) → glTF morph | `Scn3DSfx` | **chưa** |
| C6 | `SFXBillboardHelper` (128 node: 0 Billboard 83, 1 RotBillboardY 27, 2 NoRotPos 67, 3 Horizontal 1, 5 RotLocalBillboardZ 10; `Execute 0x6f5bf0`: `camRot × s_rot_180 × Euler(e)`, vị trí `PosTrans + camRot × (PosOffset + GlobalPosOffset)`) | IL2CPP (bố cục 128/128 khớp) | `export_sfx` → `billboard` trên node | `Scn3DSfx._process_billboards` (node bọc `bb_*`) | **xong** (3D-57); `FitOwnSizeBound`, cờ ngẫu nhiên chưa |
| C7 | `SFXMeshModify` đọc trọn (màu × adjust + emissive [0x6fdb70], ô atlas `f = uvGrow + uvOffset_X` đếm ngang rồi xuống hàng, `useSingleLerpGrow` cuộn u, `uiCurve` 26 mesh, `useMask` = 0), shader `blend_dst_*` + `blend_dst_zw_ver_rimlight` (viền sáng) | IL2CPP + shader APK | đã đọc hết | `Scn3DSfx` (`_uv_anims`, `scn3d_sfx_rim.gdshader`); màu đỉnh /255 | **xong** (3D-57) |
| C8 | 28 tên `skill_main` chưa ghép JX1 | bảng A6 + `skills.json` | `map_skills.MANUAL` (tên JX1 lệch âm Hán-Việt) | `skill_map.json` | **xong** (3D-47) |
| C9 | Kiểm mọi phái: 10 phái × kỹ năng có hình | `--auto3d --factions[=<phái>]` | thi triển tại chỗ (chỉ ảnh), đếm `fx.spawned` | `auto3d_fx_<phái>.png`, `AUTO3D_FACTIONS` | **xong** (3D-47): 170 kỹ năng phái, 133 có hình, 133/133 hiện |
| C10 | Sự kiện `skill_event` loại khác | `eSkillEventType` (metadata, `enum_values.py`) | đã liệt kê (`event_kinds`) | `skill_map.json` | **xong** liệt kê (3D-47); 111 Ghost **xong** (3D-51); 103 CameraShake không có dòng nào trong bảng; 26 tia nối (1 kỹ năng) **chưa** |

### D. Đánh quái trong 3D

| # | Việc | Nguồn | Cách mổ | Trạng thái |
|---|---|---|---|---|
| D1 | Vòng đánh quái: chọn mục tiêu (tia camera vào trụ SizeX/SizeY), đuổi, đánh, trúng, chết, rơi đồ, nhặt | 2.0 (luật) + A5 (trụ) | có sẵn | **xong** cơ bản (`AUTO_FIGHT` Heo trắng 80→58 trong 3D) |
| D2 | Hiệu ứng trúng đòn trên quái: `SkillHitNode` → sfx_object treo `sys_bd`, sync 2, góc 2; hàng `hit` vật con > hệ (`element_hit`, hệ người đánh khi đánh thường) | A6/A8 + IL2CPP | `KSkillFx3D.hit_on` khi `EntityLife` giảm máu và nguồn vừa thi triển | **xong** (3D-57): tia 金系击中 quanh heo khi đánh thường |
| D3 | Số sát thương bay lên: `FloatingText` 26 kiểu (prefab TopRoot), `TopRoot.ShowHpChg 0x5a3550` (10/1/0/8), `ShowSkillName 0x5a3f80` (3..7/13), `ShowHitMiss` (2) | UI bundle + IL2CPP | `export_floating.py` → `floating_text.json` | `KFloatingText3D.gd` | **xong** (3D-57) theo bản 3D (2.0 không có, chủ dự án yêu cầu); chí mạng/né chưa có trong gói zone |
| D4 | Thanh máu trên đầu quái, tên theo `KNpcGold` (vàng/xanh) | 2.0 | có sẵn `_draw_names` | **xong** |
| D5 | Quái đánh trả: clip đánh của nhóm anim quái + đạn quái (`KMissle3DView`) | A2/A6 | có sẵn | **xong** (soát lại từng nhóm quái Ba Lăng: heo, hươu, hổ, kỳ binh) |
| D6 | Chết: clip `sw` giữ khung cuối + mờ dần theo 2.0; xác biến mất theo zone | A2 + 2.0 | có sẵn `hold_last` | **xong** |
| D7 | Vật rơi 3D (`still_list`, `StillObject.OnStillReady 0x51b800`: cột sáng `cmn_droplight_*` theo phẩm chất `item_list` 2..6) | A10 + IL2CPP | gói 2.0 không mang phẩm chất vật rơi | *bỏ* theo ADR-008 (ảnh 2.0, tên trắng/tiền vàng); ghi 3D-59 |
| D8 | Kiểm tự động đánh quái trong 3D: `_auto_fight` + `AUTO3D_FLOATS added/hit_fx`, `auto_fight_hit.png`; `--skill=<id>:<phái> --series=<n>` bay 4 ảnh + `AUTO3D_MISSLE`; `--factions --fxshots` ảnh từng kỹ năng | UiGame | có | **xong** (3D-57) |

### E. UI trong client 3D

Theo ADR-008 **UI = bản 2.0** (32 màn đã xuất từ `reslst.dat`, 162 kiểm tra). UI của bản tham khảo (NGUI + 120 tệp `ui_*.lua`)
chỉ mổ để lấy **quy tắc 3D không có trong 2.0** (thanh tên đầu 3D, minimap trên map 3D, vòng chọn mục tiêu).

| # | Việc | Nguồn | Trạng thái |
|---|---|---|---|
| E1 | Các màn 2.0 còn thiếu trong game (main U6/U7, M15): bàn phím ảo, tuỳ chọn hệ thống, ghi hình, hội thoại NPC, cửa hàng, giao dịch… | `reslst.dat` ini + gamecl.exe | **một phần**: minimap xong (3D-50, `export-ui` 32 màn); còn lại làm ở main rồi gộp sang |
| E2 | Minimap: [2.0] `KUiMiniMap` 0x004C4BB0 + `KScenePlaceMapC` (ảnh `<map>24.jpg`, 32 px/region, chấm theo `Setting.ini [Map]`) cho map 2D; map 3D ảnh `ui_map_view` | gamecl.exe + SwordOnline + A14 | **xong** (3D-50): `UiMiniMap.gd`; còn: cờ/đường tới mục tiêu, bản duyệt lớn `ban-do-lon`, nút chuyển |
| E3 | Thanh tên/máu 3D: `HeadBarCrt/HeadBarStill/HeadBarGroup` (độ cao `sys_bar`, ẩn theo khoảng cách) + `ui_lifebar.lua` | IL2CPP + Lua | **một phần**: độ cao theo `sys_bar`; ẩn theo khoảng cách [tự chọn] |
| E4 | Vòng chọn mục tiêu, `ui_enemy.lua`/`ui_target_info.lua` (khung mục tiêu) | Lua | **một phần**: vòng có; khung mục tiêu 2.0 (`thanh-nhan-vat-thu-nho`?) **chưa** |
| E5 | Cần điều khiển ảo `ui_primary_rocker.lua` (di động) | Lua | **chưa** (M3D-6 Android) |
| E6 | Tuỳ chọn hiển thị `ui_cfg_display.lua`/`ui_setting.lua` (những mức nào bản tham khảo cho chỉnh) | Lua | **chưa** (6.2: đọc để đặt các nấc `settings3d.json`) |
| E7 | Màn nạp map `ui_loading.lua` | Lua | **chưa** (2.0 có màn nạp riêng → theo 2.0) |

### F. Camera, môi trường, scene

| # | Việc | Nguồn | Trạng thái |
|---|---|---|---|
| F1 | `cameraInit` (yaw/pitch/dist/min/max), `GameCamera`, `FreeCamera` | `scn_list` + IL2CPP | **xong** (KCamera3D) |
| F2 | `CameraBuildingFade` (0,25 / 10 / 0,15 / 0,3) | `fieldDefaultValues` + disasm 0x4a3ee0 | **xong** |
| F3 | Va chạm camera | `GameCamera.UpdateCameraParam` 0x4ae1d0 | **xong** (3D-52): tham khảo không va chạm; JX NEXT giữ `SpringArm3D` địa hình [tự chọn] |
| F4 | Rung camera `CameraAnim`, `TweenCamera`, `CameraSave` | IL2CPP | **chưa** (C10) |
| F5 | Sương mù/ambient/đèn đổi theo vùng (`scn_area_list` cột 10–12) | A13 | *bỏ* (4 vùng toàn game); nhạc vùng chờ 3.5 (3D-52) |
| F6 | Shader: lightmap 2 mặt, nước, cỏ đung đưa (float vật liệu), dissolve, distortion, bloom; mê cung `地形_迷宫_A高度` = Lambert đèn hướng + 4 splat (hang tối là do sương 4..12 m → 3D-55) | bundle shader (GLSL APK `shaders_apk`) | **một phần**: lightmap/nước/cỏ xong (công thức tự chọn); dissolve/bloom **chưa** |
| F6b | Hiệu ứng động của cảnh: `PrefabRef` (`Execute 0x87e890`, `SetPrefabData 0x87d6a0`) → 2 596 prefab `Cmn/*` (đuốc, đèn đá, đài phun, khói hương, bọt thác) trên 27 cảnh; nước chảy/cỏ lá đung đưa = shader | scene bundle + IL2CPP | `export_scene` → `scene.json.effects` | `KScenePlace3D._update_scene_effects` (60 m) | **xong** (3D-58); `GFX.levelMaxDists` theo prefab chưa |
| F7 | 45 map 3D: `batch_maps.py --all` (id 9052 + scene), mark → nhân vật (vai trò chung, xương, pinyin), nhân vật → template (`map_npcs.py`) | scenes_* | **xong** (3D-54/55): 44 map dựng, 17 604 vị trí, 6 map trống; hang sáng sau khi bỏ sương ngắn |

### G. Lua uLua của bản tham khảo (245 tệp, vừa trích)

| Nhóm | Tệp | Dùng để |
|---|---|---|
| Kỹ năng phái | `wudang emei shaolin wudu tangmen kunlun gaibang cuiyan tianwang tianren` (247 kỹ năng, id `skill_main`) + `npc_main` (23 kỹ năng quái/boss), `skill_cmn`, `lua_skill` | *luật* (tầm, nội lực, sát thương) → **không dùng** (luật theo Linux); dùng **đổi đơn vị**: "有效距离 cm = đơn vị JX × 1,5", "khung choáng = JX × 1,667 (18→30 Hz)", "tốc độ đạn dm/s = JX × 2,5" [TK] → ghi vào 3D-QUY-UOC §1 (bản tham khảo chọn 1 đơn vị JX = 1,5 cm; JX NEXT chọn 2 cm theo ảnh 2.0) |
| Cưỡi/nhặt/triệu hồi | `lua_scnobj_ride`, `item_loot`, `lua_scnobj_summon`, `lua_scnobj_flag`, `lua_setting_pick` | B6, D7 |
| UI | `ui_primary_*` (HUD chính: rocker, map, task, team, high, common, uilogic), `ui_lifebar`, `ui_enemy`, `ui_target_info`, `ui_deadui`, `ui_map`, `ui_setting`, `ui_cfg_display`, `ui_loading`, `ui_skill`, `ui_bag`, `ui_mainchat`, `ui_msgbox`, `ui_tips`, `ui_trait_fly` … | E2–E7 (chỉ quy tắc 3D) |
| Hệ thống | `lua_core*`, `lua_mb*` (đọc bảng), `lua_net*`, `lua_def`, `obj_def`, `flag_def`, `Layer.lua` (lớp render: Building, Terrain…) | tra tên cột/enum khi đọc bảng |

## 2. Thứ tự làm (ưu tiên chủ dự án 2026-09-19: đủ skill mọi phái → đánh quái → UI)

| Đợt | Hạng mục | Nghiệm thu |
|---|---|---|
| **1** | C8 ghép 28 tên còn lại; C2 xuất 285 prefab còn lại + ai gọi; C10 liệt kê loại sự kiện; C3 `SFXMixerMesh`; C4/C5/C6 theo số prefab dùng | `skill_map.json`: 100 % kỹ năng phái JX1 có hiệu ứng ra chiêu; `export_sfx --all` = 558; `--auto3d --faction=<10 phái>` mỗi kỹ năng `fx_spawned ≥ 1`, ảnh từng phái |
| **2** | D2 hiệu ứng trúng đòn thường (`anim_effect`), D3 số sát thương [2.0], D8 kiểm tự động | `AUTO_FIGHT` trong 3D: `hits ≥ 1`, `damage_texts ≥ 1`, quái chết, rơi đồ nhặt được |
| **3** | E2 minimap 3D, E3/E4 thanh tên + khung mục tiêu theo 2.0, E1 gộp các màn 2.0 từ main khi main làm xong | ảnh minimap + khung mục tiêu trong `--auto3d` |
| **4** | F3 va chạm camera, F5 vùng (sương/đèn/nhạc 3.5), A10/D7 vật rơi 3D, B7 bóng tròn, B8 tan xác | `auto3d` không chui đất; đổi vùng đổi sương |
| **5** | F7 các map còn lại theo lô (bảng ghép NPC tự động), B5 phi phong, E5/E6 (6.2) | mỗi map: zone nạp, client vào, `AUTO3D_OK` |

Mỗi hạng mục xong: ghi HANDOVER phần "3D-nn", cập nhật cột trạng thái ở đây và bảng trạng thái LO-TRINH-3D.

## H. Danh mục lớp của bản 3D (toàn bộ `Assembly-CSharp`, 2026-09-19) và những gì bản mình chưa có

Nguồn: `global-metadata.dat` → `D:\game3gtQ_mo\meta_pc_res.txt` (9 212 lớp; bỏ System/Unity/NGUI widget/Lua wrapper/enum/delegate → **639 lớp
ứng dụng**, liệt kê ở `build/ref_classes.txt` có số trường/hàm). Gom theo hệ; trạng thái: **có** (đã làm theo nhị phân), *một phần*, **chưa**,
*bỏ* (không cần theo ADR-008: UI/luật theo 2.0 + Linux, hoặc chỉ dành cho di động/SDK). Thứ tự làm tiếp = thứ tự bảng "Chưa có" ở cuối.

| Hệ | Lớp (bản 3D) | Ở bản mình | Trạng thái |
|---|---|---|---|
| Nhân vật & hoạt ảnh | `Creature`, `Player`, `EntityObj`, `AnimStator`, `ForcedlyAnimStator`, `ChaResourceRef`, `HangItem/HangItemMgr`, `EquipModel`, `RideUnit`, `AssetPool_Skin/_AnimationClip`, `GameNodePool_*` | `Scn3DNpc`, `KNpc3DView`, `export_npc.py` | **có** (nhóm anim 1–11/20/21, treo vũ khí/ngựa, `UpdateAutoGroup`) |
| Nhân vật: phụ | `bonechain`, `ClothStrings` (tóc/vải vật lý), `BlingBling/BlingBlingMgr` (lấp lánh), `TaskRimLight` (viền sáng khi chọn/trúng), `TaskTweenDissolve` (tan xác), `TaskStepSound` (tiếng bước theo mặt đất), `TaskGhost` (bóng mờ), `TaskAutoPick`, `Attract`, `LookTargetEvent`, `ChaAI*` (AI phụ client: nhìn theo, hành động rảnh) | bóng mờ có; còn lại chưa | *một phần* — **chưa**: bonechain/vải, RimLight, Dissolve (B8), StepSound, BlingBling |
| Kỹ năng | `ActionBase/Forcedly/Skill`, `SkillHelper`, `SkillMainWrap/Method`, `SkillEvent*`, `SkillHit/HitNode`, `SkillChildObjMethod`, `ChildObject`, `KinematicHelper`, `Orbiter`, `ConstForce`, `AlwaysForward`, `LineCOCtrl/LineData`, `StateInstance/StateMgrLogic/StateCD/CoolDownMgr`, `TaskStateMgr`, `PKRule`, `AfterEnemyState/LockEnemyState` | `KSkillFx3D`, `map_skills.py` (hình); luật ở zone (Linux) | **có** phần hình (3D-47/57); `LineCOCtrl` (tia nối, kind 26) **chưa**; `Orbiter` (bay vòng, sync 7) **chưa** |
| Hiệu ứng | `SFXObject`, `SFXUIObject`, `SFXMeshModify`, `SFXMixerMesh`, `SFXBillboardHelper`, `SFXMaterialModify`, `SFXMeshTrailDrag`, `SFXLineMesh`, `SFXXWeaponAdapter/Anchor/Anim`, `XftWeapon.XWeaponTrail/Spline/VertexPool`, `PocketRPGWeaponTrail`, `TronTrailSection`, `PC2Anim`, `ParticleKFAnimation`, `GFX` (LOD hạt theo khoảng cách/FPS), `Tween*` (17), `UITweener` | `Scn3DSfx`, `Scn3DTrail`, `export_sfx.py` | *một phần*: 343 prefab, tween, mixer, billboard, skinned, rim, uv-anim **có**; `SFXLineMesh` (3D-61), `Trail` (3D-60), `PC2Anim` (3D-63) **có**; *bỏ*: `SFXMeshTrailDrag` (boss riêng), `SFXXWeaponAdapter` (0 vũ khí dùng); **chưa**: `SFXMaterialModify`, `GFX` LOD, `TweenFOV/OrthoSize/Camera/Volume` |
| Cảnh | `Scene`, `SceneLoad`, `SceneBuildData/DataInfo/DataNode`, `Scene_Ref`, `PrefabRef`, `SceneConfiger`, `SceneRenderSetting`, `CullDistances`, `T4M*` (địa hình), `Grass/MeshGrass/UGrid2DGrass` (cỏ sinh), `MarkPoint/Area/Line`, `NavContext`, `PathFindContext.*`, `MapHandle/MapHelper`, `AreaData`, `TaskScnArea` | `KScenePlace3D`, `export_scene.py`, `batch_maps.py`, zone obstacle | **có** 45 map + hiệu ứng cảnh (3D-58); `Grass` sinh theo lưới **chưa** (cỏ có sẵn trong mesh cảnh?), `CullDistances` theo lớp **chưa** (dùng 45 m chung) |
| Vẽ / hậu kỳ | `ScnRenderPipeline`, `PostEffectBase/Bloom/Distortion/RadialBlur`, `DistortionMapCamera`, `DepthMapCamera`, `CustomDepthTexture`, `GrabPassFeature`, `FullScreenPassRendererFeature`, `MirrorReflection` (phản chiếu nước), `SkyboxCam/SkyBoxCollection` (bầu trời), `DynamicShadowProjector.*`, `ShadowProjMgr`, `SeqProjectorHelper`, `Decal/DecalBuilder/NavMeshDecal`, `RenderQueueModifier`, `GUIColorTransit`, `TaskTweenDirLight/Fog/SupColor` (đổi đèn/sương theo vùng) | shader lightmap/địa hình/nước/cỏ; bóng đĩa | *một phần* — **chưa**: bloom, distortion, radial blur, phản chiếu nước, skybox, decal (vòng cảnh báo A9), bóng chiếu thật (B7 dùng đĩa), đổi đèn/sương theo vùng |
| Camera | `GameCamera`, `CameraInitParam`, `CameraAnim`, `CameraFade`, `CameraBlur`, `CameraBuildingFade`, `CameraSave`, `CameraTargetMotifier`, `CameraModifyNode`, `CameraPostEffect`, `FreeCamera`, `FPSController`, `TweenCamera/FOV` | `KCamera3D` | *một phần*: khoảng cách/góc, che nhà **có**; rung (F4), mờ dần chuyển cảnh, `CameraAnim` (cốt truyện) **chưa** |
| Đầu nhân vật | `HeadBarBase/Crt/Still`, `HeadTop*` (11 lớp vẽ gộp), `TopRoot`, `FloatingText`, `HUDText`, `NumBoard`, `TargetSelectEffect`, `TouchEffect` | `_draw_names` (theo 2.0), `KFloatingText3D` | **có** tên/máu theo 2.0 + số bay theo bản 3D; `TargetSelectEffect` (vòng chọn `cmn_select`) và `TouchEffect` (dấu click đất) **chưa** (đang dùng vòng tự vẽ) |
| Vật rơi / tĩnh | `StillObject`, `mbtb_still_list`, `Cmn/cmn_droplight_*`, `cmn_drop_*` | ObjData 2.0 (billboard) | **chưa** (D7): cột sáng theo phẩm chất, hình vật rơi 3D |
| Âm thanh | `AudioSourceMgr`, `AreaSound`, `AudioListenerFollow`, `PitchShifter`, `TaskStepSound`, `mbtb_sound_list/group` | `KWavSound` (2.0) | *một phần*: tiếng thi triển/đạn/hành động theo 2.0 **có**; nhạc vùng (`AreaSound`, F5) và bước chân **chưa** |
| Cốt truyện | `StoryAnim*`, `mbtb_story_*` | — | *bỏ* (2.0 không có cắt cảnh) |
| Mạng / Lua / bảng | `UnityTcpClient.*`, `Net*`, `Msg*`, `Lua*` (120), `mbtb_*` (49 bảng), `TableInterface`, `mb_view_base` | zone/gateway/protocol riêng (ADR-008) | *bỏ* (chỉ đọc bảng để lấy dữ liệu hình) |
| UI | `UI*` (124 NGUI), `UIMgr`, `UIMapPointManager`, `JoystackCc`, `TouchUseSkill`, `UICoolDown`, `UIBag*` | UI 2.0 (`uicase`) | *bỏ* theo ADR-008 trừ: bản đồ (đã lấy ảnh `ui_map_view`), cần cân nhắc `JoystackCc` (Android, M3D-6) |
| Nền tảng / SDK | `AndroidTool`, `OpSdkWrapper`, `CApolloVoiceSys`, `GMTool.*`, `Debugger`, `HUDFPS`, `GameConfig`, `GameInitLoad/Startup`, `AssetPool*`, `BundleAsync`, `ResourceLoader`, `Localization*` | — | *bỏ* |

**Chưa có, làm theo thứ tự (mỗi mục một phần 3D-nn, số liệu mổ từ nhị phân):**
1. ~~Vòng chọn mục tiêu `TargetSelectEffect` (`cmn_select`)~~ **xong 3D-59**; `TouchEffect` = gợn chạm UI di động → *bỏ*.
2. ~~Vật rơi: `cmn_droplight_*`~~ *bỏ* (gói 2.0 không mang phẩm chất; 3D-59).
3. ~~Bầu trời~~: bản 3D **không có skybox** (camera `m_ClearFlags 2` = màu đặc theo cảnh, `SkyBoxCollection` không gắn ở cảnh nào, cubemap `skycube_tex_3` chỉ là `customReflection`) → xuất `render.camera_bg` (3D-61); ~~`MirrorReflection`~~ *bỏ* (0 bundle dùng; nước lấy `_ReflectTex` tĩnh).
4. ~~Hậu kỳ `PostEffectBloom`/distortion~~ *bỏ*: `PostEffectBase.IsEnable`/`PostEffectBloom.IsEnable` = stub `0x33e630` trả false → bản 3D không bật hậu kỳ.
5. Chết tan xác `TaskTweenDissolve` (B8) — giữ theo 2.0 (xác mờ dần, D6); ~~`TaskRimLight`~~ *bỏ* (không có nơi gọi trong Lua/sự kiện: kind 111 toàn là bóng mờ Ghost, kể cả 5 bóng né theo hệ `ghost_{jin,mu,shui,huo,tu}.mat`); tiếng bước `TaskStepSound` — 2.0 không có → *bỏ*.
6. Hiệu ứng còn lại: ~~`SFXMeshTrailDrag`~~ *bỏ* (3 prefab của boss 道清真人 riêng bản 3D), ~~`SFXLineMesh`/`LineCOCtrl`~~ **xong 3D-61** (2 sự kiện kind 26: 昆仑天际迅雷), ~~`Trail`~~ **xong 3D-60**, ~~`SFXXWeaponAdapter`~~ *bỏ* (71 vũ khí `sfx` = 0, không vũ khí nào có hào quang), ~~`PC2Anim`~~ **xong 3D-63** (1: 唐门追心箭, kỹ năng JX 50), `Orbiter` (sync 7: không dòng `skill_childobj`/`sfx_object` nào dùng → *bỏ* khi xác nhận ở 3D-64).
7. ~~Camera rung~~ *bỏ* (bảng `skill_event` **không có** sự kiện 103; enum có nhưng không dùng); nhạc/tiếng vùng `AreaSound` (F5, chờ nhạc 2.0 của main); đổi đèn/sương theo vùng (`TaskTweenDirLight/Fog`) — chưa thấy dữ liệu gọi.
8. `bonechain` vải/tóc, phi phong (B5); `GFX` LOD hạt; `CullDistances` theo lớp.
9. Còn của lịch trình cũ: 62 NPC tên riêng, 6 map trống, E-group UI 2.0, cài đặt, Android.
