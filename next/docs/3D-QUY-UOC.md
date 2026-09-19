# Quy ước toạ độ, đơn vị và dữ liệu 3D (M3D-0 bước 0.2 / 0.4)

Mọi con số dưới đây có nguồn ghi bên cạnh: **[2.0]** = client 2.0 `gamecl.exe` / bảng của nó, **[Linux]** = server Linux,
**[TK]** = bản tham khảo 剑网江湖 (bảng excel / bundle), **[tự chọn]** = ta chọn, có lý do. Mã: `client/scenes3d/KScene3DMath.gd`
(test `test_scene3d_math` trong `client/tests/run.gd`).

## 1. Đơn vị scene của zone (không đổi)

- Ô vật cản 32 × 32 đơn vị, region 512 × 1024 đơn vị (16 × 32 ô) **[2.0, Linux]** (`KRegion`, MAPS.md §2).
- Phép chiếu 2D `sx = x`, `sy = y/2 − z·887/1024` (`KRepresentShell2::CoordinateTransform`) **[2.0]**:
  `1/2 = sin 30°`, `887/1024 = 0,866 = cos 30°` → bản 2D là một góc nhìn **trực giao, ngẩng 30° so với mặt đất**.
  x và y (mặt đất) cùng một đơn vị (tốc độ đi tính theo độ dài đường đi, ô vuông 32 × 32).
- Hướng `dir` 0..63: 0 = xuống màn hình (+y), tăng theo chiều kim đồng hồ; 16 = trái (−x), 32 = lên (−y), 48 = phải (+x)
  (`g_GetDirIndex`, `KMath.gd`) **[2.0]**.
- Tốc độ: `WalkSpeed`/`RunSpeed` của `npcs.txt` là đơn vị mỗi khung 18 Hz **[Linux]** (`KSubWorld.cpp` ServeMove);
  người chơi mặc định 200 đơn vị/giây (`zone.default_speed`).

## 2. Đổi sang mét (Godot)

| | Giá trị | Nguồn |
|---|---|---|
| Tỉ lệ **`UNIT = 0,02 m`** (1 đơn vị scene = 2 cm; ô 32 = 0,64 m; region = 10,24 × 20,48 m) | | [tự chọn], suy từ hai dòng dưới |
| Chiều cao nhân vật nam đứng: sprite thân `MA_BD_019_ST01` đáy 198, đầu `MA_HR_019_ST01` đỉnh 123 (REF_SPOT 160,192) → **75 px** màn hình → 75 / cos 30° = **86,6 đơn vị** | 86,6 × 0,02 = **1,73 m** | [2.0] sprite trong `client/assets/sprites` |
| Tên trên đầu người chơi: `m_nStature` + 84 px (`GetNpcPate`, `0x005EC13D`) → 84 / cos 30° = 97 đơn vị | **1,94 m**; bản tham khảo `sys_bar` người 2,03–2,15 m, chủ tiệm 1,75 m | [2.0], [TK] `cha_pic.sys_bar` |
| Tốc độ chạy người chơi 200 đơn vị/s | **4,0 m/s** (đi bộ 5 đơn vị/khung = 90/s = 1,8 m/s) | [Linux] |
| Bản tham khảo đổi đơn vị JX trong script kỹ năng (`wudang.lua` đầu tệp): tầm hiệu lực **cm = đơn vị JX × 1,5** (1 đơn vị = 1,5 cm), khung choáng × 1,667 (18 → 30 Hz), tốc độ đạn dm/s = JX × 2,5 (×1,6 với một số chiêu) | 480 đơn vị (Nộ Lôi Chỉ) = 7,2 m ở bản tham khảo, **9,6 m** ở JX NEXT | [TK] `D:\game3gtQ_mo\lua\wudang.lua`; JX NEXT giữ 2 cm để nhân vật/ô đúng tỉ lệ ảnh 2.0 (ở 1,5 cm nhân vật 1,73 m = 115 đơn vị, không khớp sprite 86,6) |
| Ba Lăng Huyện JX1 rộng 16 896 đơn vị = **338 m**; Ba Lăng bản tham khảo navmesh rộng **337 m** (sâu 210 m so với 395 m — bố cục khác) | | [2.0] map 53, [TK] `world_baling` |

Toạ độ Godot (Y lên, −Z là "trước" của node):

```
X = x · UNIT          Z = y · UNIT          Y = z · UNIT (cao độ vẽ; zone không có z, lấy từ địa hình map 3D)
x = X / UNIT          y = Z / UNIT
```

Màn hình 2D "xuống" (+y) = về phía camera cổ điển = **+Z** Godot; camera cổ điển đứng ở +Z nhìn về −Z, ngẩng 30°.

## 3. Hướng quay (yaw)

Model glTF do ta xuất nhìn về **−Z** ở yaw 0 (model Unity nhìn +Z được xoay 180° khi xuất, `export_npc.py`) → 

```
yaw_deg(dir) = (32 − dir) · 360 / 64 = (32 − dir) · 5,625
dir(yaw)     = posmod(32 − round(yaw_deg / 5,625), 64)
```

Kiểm: dir 32 (lên, −y = −Z) → yaw 0; dir 0 (+Z) → 180°; dir 16 (−x) → +90° (quay +90° quanh Y đưa −Z về −X); dir 48 (+x) → −90°.
Điểm đứng của mark bản tham khảo: `angle` lưu = −(góc Unity), yaw Godot = 180 + angle (`Scn3D.gd`) **[TK]**.

## 4. Camera

| Chế độ | Thông số | Nguồn |
|---|---|---|
| Theo map (mặc định 3D) | `scn_list.cameraInit` = `dist*dist_min*dist_max*yaw*pitch*pitch_min*pitch_max`, Ba Lăng `19*10*21*0*40*40*80` | [TK] |
| Cổ điển (giống 2D) | trực giao, pitch 30°, yaw 0 (nhìn về −Z), khung nhìn 1280 × 1536 đơn vị = 25,6 × 30,7 m (`zone.view_width/height`) | [2.0] phép chiếu, [Linux] AOI |
| Xoay/zoom bằng chuột | chuột phải kéo = yaw/pitch, con lăn = dist trong [dist_min, dist_max]; tốc độ kéo 0,25°/px, zoom 1 m/nấc | [tự chọn] (hằng số của bản tham khảo nằm trong IL2CPP, chưa đọc) |

## 5. Nhân vật, tên, chọn mục tiêu

- Tên/thanh máu: nhãn 2D tại `unproject(pos + (0, bar_y, 0))`; `bar_y` = `sys_bar` của model **[TK]**, model JX1 tự làm ghi
  `bar_y` trong `npc_models.json`; thiếu thì `pate_px / cos 30° · UNIT` (= luật `GetNpcPate` 2.0 đổi sang mét) **[2.0]**.
- Chọn mục tiêu: hình trụ bán kính `cp_radius` (m) cao `bar_y` quanh chân **[TK]** (`cha_pic.cp_radius`); model chưa có số thì
  bán kính 0,5 m **[tự chọn]**. Thứ tự ưu tiên khi trùng: gần camera hơn thắng (2D: `z_index` cao hơn thắng, `UiGame._entity_at`).
- Quay mặt mượt: `res_dir` quay nửa đường mỗi khung 18 Hz về `dir` (`KNpc::Paint`) **[2.0]** — 3D nội suy thêm theo khung
  hình thật (quy tắc FPS của chủ dự án, `jxnext-fps-interpolation`).
- Hoạt ảnh: `doing` (đứng/đi/chạy/đánh/bị đánh/chết) và số khung do zone/`npcs.txt` quyết định **[Linux, 2.0]**; clip 3D chọn theo
  `anim_group` của vũ khí (`animation_list`) **[TK]**, phát với `speed_scale = clip_len / (frames / 18)` để khớp số khung.

## 6. Dữ liệu map 3D: `client/assets3d/maps/<id>/`

```
map3d.json      { "id", "name", "scene": "<thư mục glTF>/<map>.gltf", "unit": 0.02,
                  "origin": [X0, Z0]   -> gốc scene (0,0) của zone nằm ở đâu trong glTF (m)
                  "scale": 1.0          -> hệ số thêm nếu map 3D không đúng tỉ lệ UNIT (1.0 = đúng)
                  "camera": {dist, dist_min, dist_max, yaw, pitch, pitch_min, pitch_max},
                  "ground_y": 0.0, "nav": "scene.json#marks.nav" }
map.json        bản 2D tối thiểu cho zone (id, cells_x/y, cell_size 32, spawn, traps, npcs) - cùng schema MAPS.md §3
obstacle.bin    1 byte/ô, hàng trước cột, 0 = đi được - rasterize từ navmesh (tools/scn3d/navmesh_to_obstacle.py)
```

Quy tắc: `scene (x, y) → glTF (X, Z) = (origin.X + x·UNIT·scale, origin.Z + y·UNIT·scale)`; ô (cx, cy) đi được khi tâm ô
`((cx + 0,5)·32, (cy + 0,5)·32)` nằm trong một tam giác của navmesh (chiếu xuống mặt XZ). Zone đọc thư mục này qua
`zone.maps_dir_extra`.
