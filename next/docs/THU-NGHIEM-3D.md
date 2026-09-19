# Thử nghiệm 3D — nhánh riêng `exp/3d-baling` (không phải dự án chính)

**Bản sao riêng trên đĩa: `D:\JXWIN-NANGGODOT\JXN\JXN\swrod3-3d`** (nhân bản từ kho, luôn ở nhánh `exp/3d-baling`). Mọi việc 3D làm trong thư mục này; thư mục `swrod3` gốc chỉ dùng cho 2D (`main`). Cùng kho GitHub `JXGAME04/DUANGODOT`, khác nhánh, không merge vào `main`.

Ghi chú riêng để không lẫn với JX NEXT 2D trên `main`. Dự án chính vẫn là client 2D isometric theo bản 2.0
(ADR-001, ADR-007, MAPS.md). Nhánh này chỉ để trả lời câu hỏi: *"map 3D + camera xoay chuẩn game 3D trong Godot
trông và chạy thế nào?"* — chưa có quyết định đổi hướng; muốn đổi thì viết ADR mới.

## Có gì trên nhánh

| Thứ | Ở đâu | Ghi chú |
|---|---|---|
| Tool xuất map | `tools/scn3d/export_scene.py` | Bundle Unity của bộ tham khảo 剑网江湖 → `client/assets3d/<map>/` (glTF + .bin + `tex/*.png` + `scene.json`) |
| Scene Godot | `client/scenes3d/Scn3D.tscn` + `Scn3D.gd` | Nạp glTF lúc chạy (`GLTFDocument`), gắn shader lightmap / địa hình splat, đèn + sương từ `scene.json`, va chạm địa hình |
| Camera | `client/scenes3d/Scn3DCamera.gd` | Quỹ đạo: chuột phải xoay, con lăn/chuột giữa zoom, Q/E, PgUp/PgDn; kẹp theo `cameraInit` của bảng `scn_list` |
| Nhân vật tạm | `client/scenes3d/Scn3DPlayer.gd` | Hình trụ; chuột trái đi tới điểm bấm, WASD theo camera; cao độ raycast |
| Shader | `client/scenes3d/*.gdshader` | `scn3d_lm` (albedo × lightmap, có bản 2 mặt), `scn3d_terrain` (control + 4 splat + lightmap) |
| Mổ nhị phân | `D:\game3gTQ_mo\BAO-CAO-MAP-3D.md` (ngoài repo) | 46 map 3D / 65 scene id, khóa bundle, `Scene_Ref`, `MaterialRef`, định dạng navmesh AIS |

**Tài sản không vào git**: là bản quyền game gốc → `client/assets3d/` đã `.gitignore` + `.gdignore`; khóa giải mã đọc từ
`D:\game3gTQ_mo\khoa_bundle.txt` (`--key-file`), không commit.

## Chạy

```bash
python tools/scn3d/export_scene.py world_baling
```

```bash
godot --path client scenes3d/Scn3D.tscn -- --map=world_baling
```

Lần đầu ở máy mới chạy `godot --path client --headless --import` một lần (tạo cache `.godot/`). Thêm `--auto` để chụp 5 góc vào `user://logs/scn3d_world_baling_auto*.png`, đo FPS 2 giây rồi thoát (in `SCN3D_OK ...`).

## Đo được (2026-09-19, RTX 3080, GL Compatibility, 1280×720)

`world_baling` (Ba Lăng Huyện): 1714 node, 300 mesh, 51 vật liệu, 62 texture, buffer 32 MB — nạp 0,8 s, **145 FPS**,
591 draw call, 465 k tam giác/khung, VRAM 156 MB. Xoay 0/90/180/270 và nghiêng 75° đúng; bóng đổ + lightmap đúng chỗ.

## Nhân vật / NPC 3D (2026-09-19, bước 2)

- **Tool** `tools/scn3d/export_npc.py --map world_baling`: `cha_pic` (xương, các phần da body*head*shoes, scale, nhóm animation) →
  xương `exprolesbone bone/<x>.prefab`, da `exprolesskin skinpart/<x>/<phần>.asset` (`ChaResourceRef{meshUrl, matUrl, bones[]}`), mesh da
  `exprolesmesh` (trọng số xương kênh 12/13 + `m_BindPose`), vật liệu `exprolesmat` → texture `models.bdd`, clip
  `exprolesanim animation/<x>/<clip>.asset` (legacy, đường cong quaternion/pos/scale theo đường dẫn xương) → glTF **có skin + animation**
  (`assets3d/npc/cha_<id>_<xương>.gltf`, animation đặt tên theo nhóm: `xx` nghỉ, `zp` đi, `gjxx`, `xdz`, `ss`, `sw`).
- **Đặt NPC**: bảng refresh của server không có trong client, nên ghép điểm đứng trong mark JSON (`n_tiejiang`, `n_yaodian`, …, `baizhu`,
  `meihualu`, `jinmao`) với `cha_pic` theo tên (`MARK_TO_CHA` trong tool) → `assets3d/<map>/npcs.json` (494 vị trí ở Ba Lăng, phần lớn là
  điểm quái có nhiều toạ độ). Nhân vật chính = `cha_pic 1` (标准男), đi phát `zp`, đứng phát `xx`.
- **Client**: `Scn3DNpc.gd` (nạp glTF qua `GLTFDocument` có cache, `AnimationPlayer` lặp `xx`, `Label3D` tên; con `Model` quay 180° vì model
  Unity nhìn +Z), `Scn3D._setup_npcs`, `Scn3DPlayer.set_model`.
- **Đo**: 31 model, 494 NPC có skin + animation trên map: nạp 2,4 s, **80 FPS**, 794 draw call, VRAM 219 MB.
- Chưa làm: vũ khí/vật treo (`model_hang_list`, `smodels.bdd`), cloth, shader nhân vật riêng (đang dùng StandardMaterial3D), chọn NPC bằng chuột,
  nối với zone (spawn thật từ server thay cho bảng ghép tay).

## Vũ khí, đòn đánh theo vũ khí, nhãn tên, ánh sáng (2026-09-19, bước 3)

- **Vũ khí** `tools/scn3d/export_weapon.py`: `model_hang_list` (96 mục; 71 vũ khí/áo choàng có prefab, còn lại là hiệu ứng thú cưỡi) → prefab
  trong `smodels.bdd` còn nguyên mesh/vật liệu/texture → `assets3d/weapon/wp_<id>_<res>.gltf` + `weapons.json` (loại, điểm treo, nhóm animation,
  điểm `start/end` cho vệt đao). Điểm treo từ `HangItemMgr` trên xương (`daojian` → `hang@wq_r`, `qianggun` → `hang@qianggun`, song đao/chùy
  hai tay, `ssqt_r/l` cẳng tay) với lệch vị trí/góc; trong Godot node đổi tên `hang@…` → `hang_…` (Godot thay `@`).
- **Nhân vật chính** xuất đủ 30 clip của 11 nhóm animation theo vũ khí (`anim_group` 1 tay không, 2 kiếm, 3 đao, 4 thương, 5 côn, 6 song đao,
  7 song chùy, 8 quyền, 9–11 ám khí): nghỉ `xx01`, nghỉ chiến đấu `xxzd*`, đi `zp*`, đòn `gj*01/02` (xác suất 65/35), nội công `sf*`, bị đánh
  `ss*`, chết `sw*`. `Scn3DNpc`: `set_group`, `attack()`, `act("magic"/"ss"/"sw")`, `attach_weapon`. Phím: 1–7 loại vũ khí, 0 tay không,
  Tab đổi mẫu cùng loại, Space đòn đánh, F nội công, G bị đánh, H chết.
- **Nhãn tên** chuyển từ `Label3D` (mờ khi xa) sang `Label` 2D vẽ đè theo `Camera3D.unproject_position` (như game gốc), cao độ `sys_bar`.
- **Ánh sáng — lý do đất đen**: lightmap của bộ tham khảo (Bakery `comp_light`, BC6H) chỉ chứa ánh sáng gián tiếp/bầu trời (Unity Mixed – Baked
  Indirect); nắng mặt trời là `DirectionalLight3D` thời gian thực. Shader `unshaded` cũ chỉ lấy lightmap nên tối. Nay: đèn mặt trời + bóng thời gian
  thực chiếu lên `ALBEDO`, lightmap cộng qua `EMISSION = albedo × lm` (không lật V, `ambient_light_disabled`), năng lượng đèn 1,35 → đất sáng
  màu cát như game gốc.
- **Hiệu năng**: NPC cách nhân vật > 45 m dừng `AnimationPlayer` → 494 NPC vẫn ~145 FPS.
- Tuỳ chọn `--at=<điểm>` (vd `--at=n_yaodian`) để đứng tại điểm đánh dấu so ảnh với game gốc.

## Hiệu ứng kỹ năng (2026-09-19, bước 4 — bản đầu)

- Dữ liệu: `skill_main` (344 kỹ năng, 10 phái trùng JX1) → `skill_section` → sự kiện → `skill_childobj` (317 vật thể hiệu ứng, đường dẫn
  `Skill/<tên>` tương đối `Particles/`) và `sfx_object` (129: hiệu ứng thi triển theo ngũ hành `shifa_*`, vệt đao 290–293…), `anim_effect`
  (vệt đao theo clip đánh). Prefab trong `particles.bdd` (558): `ParticleSystem` (module đọc được), mesh + `SFXMeshModify` (ô atlas, màu),
  `Light`, animation biến đổi; vật liệu shader `剑网江湖/particle/*` với `_BlendDst` (1 cộng / 10 alpha), atlas 1024 (`sprite_skill_*`).
- **Tool** `tools/scn3d/export_sfx.py <Skill/x> | --childobj id | --sfx id | --all` → `assets3d/sfx/<tên>.json` (mô tả node: particle, vật liệu,
  đèn) + `.gltf` (cây node, mesh, animation) + `tex/`.
- **Client** `Scn3DSfx.gd`: dựng lại — `CPUParticles3D` (tuổi thọ, tốc độ, kích thước + đường cong, màu theo thời gian, hình phát, sprite
  sheet `particles_anim`, mesh particle), `MeshInstance3D` vật liệu unshaded cộng/alpha, `OmniLight3D`; tự huỷ theo thời gian. `Scn3DTrail.gd`:
  vệt đao giữa `start/end` của vũ khí khi đang đánh. Phím thử Z/X/C = 3 kỹ năng Võ Đang (怒雷指 Nộ Lôi Chỉ, 无我无剑 Vô Ngã Vô Kiếm,
  剑飞惊天 Kiếm Phi Kinh Thiên) + hiệu ứng thi triển hệ Hỏa.
- **Chưa đúng/chưa làm**: nhiều hiệu ứng cần chỉnh tay (hướng mặt phẳng, blend "mul", kích thước mesh particle, UV cuộn của
  `SFXMeshModify`, stretched billboard, trail module); ghép kỹ năng JX1 ↔ hiệu ứng bộ tham khảo theo phái chưa làm (cần bảng ánh xạ);
  `--all` để xuất toàn bộ 558 prefab khi cần.

## Đi lại theo navmesh (2026-09-19, bước 5)

- Không đi xuyên nhà/tường/nước: dựng `NavigationRegion3D` từ navmesh AIS của game gốc (`scene.json marks.nav`, 2 967 đỉnh / 4 404 tam giác
  ở Ba Lăng; đảo chiều tam giác vì đã đảo trục X). WASD: vị trí mong muốn được kẹp về điểm gần nhất trên navmesh (trượt dọc mép, không xuyên);
  chuột trái: `NavigationServer3D.map_get_path` → đi theo waypoint. Kiểm thử tự động (`--auto`, `SCN3D_NAVTEST`): đi 3 s về phía lò rèn bị chặn ở
  tường, lệch khỏi navmesh ≤ 0,006 m.
- Đây cũng là cách nối với zone sau này: navmesh → rasterize thành lưới vật cản 32×32 đơn vị scene (`obstacle.bin`) cho A* của `KMapData`.

## Cổng riêng của bản 3D (không trùng bản 2D)

| | bản 2D (`swrod3`) | bản 3D (`swrod3-3d`) |
|---|---|---|
| zone | 17001 | **19001** |
| gateway TCP / WebSocket | 17100 / 17102 | **19100 / 19102** |
| e2e (`JX_PORT_OFFSET=1000`) | 18001 / 18100 | 20001 / 20100 |
| pprof gateway | 17199 | 19199 |

Đã đổi mặc định trong `config/zone.json`, `config/gateway.json`, client (`KNetAddress.gd`, `KProtocolProcess.gd`,
`serverlist.json`, màn đăng nhập), `tools/dev.py`, Go/C++ và test/tài liệu tương ứng.

## Bước sau (theo thứ tự chủ dự án nêu)

1. Hình nhân vật & NPC: sprite 8 hướng JX1 làm billboard, chọn hướng theo góc tương đối với camera, khóa pitch ≈ 30°
   (phép chiếu cũ `sy = y/2`); hoặc model 3D nếu có.
2. Nước / lá cây đung đưa (đang là vật liệu tĩnh), che mờ nhà chắn camera.
3. Nối với zone: `X = x`, `Z = y` (đơn vị scene của zone), cao độ chỉ để vẽ; map 3D không có tầng đi được chồng nhau.

## Chạy thế giới 3D với zone thật (M3D-1, từ 2026-09-19)

1. Tài sản: `python tools/scn3d/export_scene.py world_baling`, `python tools/scn3d/export_npc.py --map world_baling`,
   `python tools/scn3d/export_weapon.py`, rồi `python tools/scn3d/make_map3d.py world_baling --id 9053` → `client/assets3d/maps/9053/`
   **Mọi map tham khảo một lượt** (3D-53/54): `python tools/scn3d/map_npcs.py` (bảng ghép quái → template JX1) rồi
   `python tools/scn3d/batch_maps.py --all` (45 scene có bundle, id = 9052 + id scene của `scn_list`: 9053 Ba Lăng, 9054 Vĩnh Lạc
   Trấn, 9055 Đạo Hương Thôn, 9062 Thành Đô, 9064 Biện Kinh, 9078 Thanh Thành Sơn…; `--list` in bảng; `--maps-only` chỉ chạy lại
   `make_map3d`); vào bằng `?gm ds NewWorld(<id>, <ô x>, <ô y>)` — ô sinh xem `spawn` trong `client/assets3d/maps/<id>/map.json`.
   (`map.json` + `obstacle.bin` cho zone, `map3d.json` + `models.json` cho client, bẫy ra Phượng Tường + Lua trong `data/script/3d`).
   Zone đọc thư mục này qua `zone.maps_dir_extra` (config/zone.json).
2. `client/assets` của bản 3D là thư mục thật: junction `maps`/`sprites`/`npcres` sang `swrod3/next/client/assets`, còn `ui`/`items`/
   `sounds`/`text`/`missles`/`*.json` là bản sao bộ xuất đầy đủ (từ worktree `handover-doc-review`). Thiếu bộ này thì không có thanh 2.0.
3. Build bản 3D: `builduild_zone.cmd all` (VS 2022, không commit) và `go build -o ../build/go/ ./cmd/...` trong `services/`;
   `JX_CONFIG=Release python tools/dev.py start` → zone 19001 (981 map, map 9053 có 494 NPC), gateway 19100/19102.
4. Renderer: mặc định GL Compatibility (ADR-008); `set JX_RENDER=mobile` (hoặc `forward_plus`) trước `client3d.cmd` → Vulkan, bloom toả rộng như bản tham khảo (3D-66).
   Kiểm tay cầm 71 vũ khí: `godot --path client scenes3d/Scn3D.tscn -- --auto --map=copy_baling --weapons` rồi `python tools/scn3d/weapon_sheet.py --crop 330,100,950,700 --cols 8` (3D-67).
4b. Client: `godot --path client -- --auto --auto3d --gm=NewWorld(9053,300,150) --server=127.0.0.1:19100 --account=x --password=auto`
   chạy tự động (vào map 1 → NewWorld → bản vẽ đổi sang `KWorldView3D` → chụp 3 góc → đi → kiếm trên tay → đánh heo → bẫy về map 1 →
   `AUTO3D_OK`); chơi tay: `client.cmd` rồi gõ `?gm ds NewWorld(9053,232,194)` trong chat (zone dev có `gm_chat`).
   Bản vẽ chọn tự động theo `client/assets3d/maps/<id>/map3d.json` (`Game.want_3d`), `--3d`/`--2d` ép.
5. Điều khiển 3D: chuột trái đi/chọn/đánh như 2.0, kéo chuột phải xoay camera (click phải không kéo = kỹ năng chuột phải),
   con lăn zoom trong [dist_min, dist_max] của `cameraInit`.
6. Map không có bộ 3D (979 map còn lại) chạy **2.5D** tự động (M3D-4): nền tile + nhà/cây là bảng đứng, nhân vật là sprite 2.0
   trên bảng quay theo camera, camera trực giao 30° (yaw ±25°, zoom = kích thước khung). `--2d` giữ client 2D cũ.

## Lộ trình

Lịch từng bước để lên 3D chuẩn (mốc M3D-0 … M3D-6, mã và nội dung chạy song song, bản 2.5D dự phòng cho 980 map):
`docs/LO-TRINH-3D.md`. Phân tích thiếu gì: `docs/PHAN-TICH-3D-THIEU.md` (mục 5).

## Cách nối vào dự án chính nếu được duyệt

Chỉ thay lớp "vẽ thế giới" của client (~2 300–2 500 dòng: `KScenePlaceC`, `KIpo*`, `KNpc/KNpcRes`, `KObj`, `KMissle*`,
phần map/camera/pick trong `UiGame`); zone, services, giao thức, UI giữ nguyên. Điều kiện: bản 2D tách lớp vẽ sau một giao diện
(xem HANDOVER 4b phần 33 và ghi chú trong cuộc trao đổi 2026-09-19).
