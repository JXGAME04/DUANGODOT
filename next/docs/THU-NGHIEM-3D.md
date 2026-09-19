# Thử nghiệm 3D — nhánh riêng `exp/3d-baling` (không phải dự án chính)

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

Thêm `--auto` để chụp 5 góc vào `user://logs/scn3d_world_baling_auto*.png`, đo FPS 2 giây rồi thoát (in `SCN3D_OK ...`).

## Đo được (2026-09-19, RTX 3080, GL Compatibility, 1280×720)

`world_baling` (Ba Lăng Huyện): 1714 node, 300 mesh, 51 vật liệu, 62 texture, buffer 32 MB — nạp 0,8 s, **145 FPS**,
591 draw call, 465 k tam giác/khung, VRAM 156 MB. Xoay 0/90/180/270 và nghiêng 75° đúng; bóng đổ + lightmap đúng chỗ.

## Bước sau (theo thứ tự chủ dự án nêu)

1. Hình nhân vật & NPC: sprite 8 hướng JX1 làm billboard, chọn hướng theo góc tương đối với camera, khóa pitch ≈ 30°
   (phép chiếu cũ `sy = y/2`); hoặc model 3D nếu có.
2. Nước / lá cây đung đưa (đang là vật liệu tĩnh), che mờ nhà chắn camera.
3. Nối với zone: `X = x`, `Z = y` (đơn vị scene của zone), cao độ chỉ để vẽ; map 3D không có tầng đi được chồng nhau.

## Cách nối vào dự án chính nếu được duyệt

Chỉ thay lớp "vẽ thế giới" của client (~2 300–2 500 dòng: `KScenePlaceC`, `KIpo*`, `KNpc/KNpcRes`, `KObj`, `KMissle*`,
phần map/camera/pick trong `UiGame`); zone, services, giao thức, UI giữ nguyên. Điều kiện: bản 2D tách lớp vẽ sau một giao diện
(xem HANDOVER 4b phần 33 và ghi chú trong cuộc trao đổi 2026-09-19).
