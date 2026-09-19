# Hướng dẫn dựng map / nhân vật 3D cho JX NEXT (M3D-2 bước 2.1)

Dành cho người dựng nội dung (Blender hoặc phần mềm xuất được glTF 2.0). Mọi quy ước ở đây là để `tools/scn3d/import_map3d.py`
nhập được vào zone + client bằng **một lệnh**, không sửa tay. Đơn vị và số đo lấy từ `docs/3D-QUY-UOC.md` (1 đơn vị scene của zone
= 2 cm; ô vật cản 0,64 m; nhân vật nam cao 1,73 m; tên trên đầu 1,94–2,15 m).

## 1. Map

**Tệp**: một `<ten>.gltf` (+ `.bin`, ảnh PNG/JPG cạnh tệp) và một `<ten>.map3d.json` mô tả (mẫu ở mục 4).

**Đơn vị**: mét, trục Y lên, **−Z là hướng bắc** (lên màn hình cổ điển), +X đông. Gốc (0,0,0) của tệp = góc **tây-bắc** của map
(zone tính x tăng về đông, y tăng về nam = +Z). Map thành thị JX1 cỡ 500 × 650 m (Phượng Tường: 25 600 × 33 792 đơn vị).

**Tên node** (tiền tố, không phân biệt hoa thường) — bộ nhập phân loại theo tên, không theo vật liệu:

| Tiền tố | Ý nghĩa | Bộ nhập làm gì |
|---|---|---|
| `terrain` | mặt đất (một hay nhiều mesh) | va chạm lấy cao độ, chiếu tia con trỏ |
| `walk` | **mesh vùng đi được** (phẳng, đè lên mặt đất, có thể nhiều mảnh) — đây là "navmesh" | rasterize → `obstacle.bin` cho zone (tâm ô 0,64 m nằm trong tam giác = đi được) |
| `building` | nhà, tường, cổng | mờ khi che camera (CameraBuildingFade) |
| `tree`, `grass` | cây, cỏ (vật liệu alpha cắt) | lá/cỏ đung đưa nếu vật liệu đặt `extras.sway` |
| `water` | mặt nước | shader nước |
| `stone`, `prop` | vật tĩnh khác | vẽ thường |
| `spawn` (empty) | điểm sinh / hồi sinh; nhiều điểm: `spawn`, `spawn_1`… | `map.json.spawn` (điểm đầu), `revive` |
| `npc_<template>_<n>` (empty) | NPC/quái JX1 theo `template` của `npcs.txt` (vd `npc_197_1` = Thợ rèn 1); hướng nhìn = −Z của empty | `map.json.npcs` (x, y, dir); model 3D theo `models.json` |
| `exit_<map>_<x>_<y>_<n>` (empty, có scale) | cổng sang map khác: `map` id đích, `x`/`y` **ô tuyệt đối** (Mps) trên map đích; scale.x/z = kích thước vùng bẫy (m) | `map.json.traps` 3×3 ô + Lua `NewWorld` |
| `light_sun` (light) | nắng (hướng), màu, cường độ | `render.light` |
| `cam` (empty) | `extras.dist/dist_min/dist_max/pitch/pitch_min/pitch_max` | `map3d.json.camera`; thiếu → 19*10*21*40*40*80 |

**Vật liệu**: PBR tiêu chuẩn glTF (albedo, alpha `MASK` cho lá/cỏ với cutoff, `BLEND` cho nước). Không cần lightmap cho map tự làm:
client dùng nắng thời gian thực + môi trường (`render`); lightmap chỉ là lựa chọn sau này (`LightmapGI` trong Godot editor).
`extras` trên vật liệu (Blender: Custom Properties): `sway: 1` (lá quay tròn) / `sway: 2` (cỏ theo gió), `sway_amount` (m), `sway_speed`.

**Kích thước texture**: 1024 hoặc 2048, PNG (KTX/BC nén ở bước tối ưu). Tam giác: thành thị ≤ 1,5 triệu (bản tham khảo 0,3–1,6 triệu).

**Kiểm nhanh**: `python tools/scn3d/import_map3d.py <ten>.gltf --id <id>` in ra: kích thước map (m và ô), số node từng loại,
diện tích đi được, NPC/cổng tìm thấy, lỗi (thiếu `spawn`, thiếu `walk`, node ngoài map, template không có).

## 2. Nhân vật, quái, NPC

- Một bộ xương chung cho người (nam/nữ), quái mỗi loài một bộ; glTF có skin + animation.
- Tỉ lệ: nam đứng 1,73 m, nữ 1,65 m; gốc tại chân, nhìn về **−Z**.
- Tên clip theo bảng hành động của bản tham khảo (`animation_list`): `xx01` đứng, `zp01` đi/chạy, `gjxx01` đứng chiến đấu, `gjpb01`
  chạy chiến đấu, `gj01/gj02` đánh (kiếm), `gjdj01/02` (đao), `gjqg01/02` (thương/côn), `ss01` bị đánh, `sw01` chết, `sf01` nội công,
  `xdz01` động tác nhỏ; trên ngựa: nhóm 20 (`xx85`…). 30 fps. Số khung của zone (`npcs.txt`) là thời lượng — client co giãn clip.
- Điểm treo (empty con của xương): `daojian` (kiếm/đao tay phải), `qianggun` (thương/côn), `ssdaochui_r/_l`, `ssqt_r/_l` (quyền),
  `sys_bd` (ngực: hiệu ứng ra chiêu), `sys_bar` (đỉnh đầu: tên), `sys_foot` (chân); ngựa: `ma_qi1` (yên).
- Vũ khí: glTF riêng, gốc tại điểm cầm, +Y là hướng lưỡi; hai empty `start`/`end` cho vệt đao.
- Ghi vào `client/assets3d/npc/npc_models.json` bằng `tools/scn3d/import_cha.py` (mục sau) với `template` JX1 ↔ tệp.

## 3. Bố cục theo map 2D cũ

`jxassets map <id>` in `Region_C.dat` (vật cản, NPC, cổng) → dựng blockout đúng chỗ: 1 ô 2D = 0,64 m, gốc tây-bắc. Cổng và NPC
lấy từ `client/assets/maps/<id>/map.json` (`traps`, `npcs`, toạ độ scene ÷ 50 = mét).

## 4. Mẫu `<ten>.map3d.json`

```json
{ "id": 9001, "name": "Thôn thử 3D", "exit_default_map": 1,
  "render": { "ambient_light": [0.55, 0.55, 0.6], "fog": true, "fog_mode": 1, "fog_start": 60, "fog_end": 220, "fog_color": [0.75, 0.82, 0.9] } }
```

Mọi trường đều có mặc định; `id` bắt buộc (9000–9999 = map thử, chưa chiếm id map JX1).

Tệp mẫu sinh bằng `python tools/scn3d/make_sample_map.py` (một thôn nhỏ: đất, đường đi được, hai nhà, cây, cổng về Phượng Tường) —
mở trong Blender để thấy đúng quy ước rồi thay bằng tài sản thật.
