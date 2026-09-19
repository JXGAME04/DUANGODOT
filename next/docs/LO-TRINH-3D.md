# Lộ trình từng bước: JX NEXT lên 3D

Ngày 2026-09-19. Dựa trên `PHAN-TICH-3D-THIEU.md` (mục 5) và thử nghiệm đã chạy ở `exp/3d-baling`. Ước lượng cho **một người
lập trình (làm cùng Claude) + một nguồn làm nội dung 3D** (họa sĩ / thuê / chủ dự án). Tuần = tuần làm việc. Nội dung 3D là
đường găng: mã xong sớm hơn nội dung nhiều, nên lộ trình tách **mã** và **nội dung** chạy song song và có **bản 2.5D dự phòng** để
game luôn chơi được đủ 980 map trong lúc dựng map 3D dần.

Quy tắc chung: mọi việc 3D làm trên `swrod3-3d` (`exp/3d-baling`) cho tới khi qua mốc M3D-1; mỗi bước commit + push + ghi
HANDOVER; mỗi mốc có kiểm thử tự động (`--auto` chụp ảnh + FPS, e2e). Tài sản của 剑网江湖 chỉ dùng thử nội bộ, không vào
sản phẩm. Bản 2D trên `main` giữ nguyên và là đường lùi.

## Trạng thái (cập nhật 2026-09-19, sau một phiên "làm từng bước")

| Mốc | Trạng thái | Ghi chú |
|---|---|---|
| M3D-0 | **xong** | ADR-008, `3D-QUY-UOC.md`, `KScene3DMath`, `KWorldView`/`KWorldView2D`, `make_map3d.py`, `zone.maps_dir_extra` |
| M3D-1 | **xong** | `KWorldView3D` + zone thật trên map 9053 (Ba Lăng 3D): đi, đánh, chết, vũ khí theo vật phẩm, khối tên 2.0, bẫy 3D↔2D, `--auto3d` |
| M3D-2 | **mã xong, nội dung chờ họa sĩ** | 2.1 `3D-HOA-SI.md`, 2.2 `import_map3d.py` + map mẫu (thôn 9001 chạy với zone); 2.3–2.5 cần người dựng |
| M3D-3 | 3.1 / 3.2 / 3.3 / 3.6 xong đợt 1; 3.4 / 3.5 hoãn | kỹ năng 205/288 ghép theo tên Hán; mờ nhà; nước + lá cỏ; ngựa; minimap & nhạc vùng chờ phần 2.0 tương ứng |
| M3D-4 | **xong** | 2.5D cho 980 map 2D, bản 3D/2.5D là mặc định, e2e cả hai bản vẽ |
| M3D-5 | chưa (nội dung) | theo họa sĩ |
| M3D-6 | 6.1 một phần | mức chất lượng, đo FPS thật; còn texture nén, Android, patcher, CI GPU |

## Tổng quan

| Mốc | Nội dung | Tuần | Điều kiện đạt |
|---|---|---|---|
| M3D-0 | Quyết định + nền | 1 | ADR-008 duyệt; quy ước toạ độ/đơn vị; giao diện "lớp vẽ thế giới" trong client |
| M3D-1 | Thế giới 3D nối zone thật | 2–4 | Đăng nhập 2.0 → vào Ba Lăng 3D (map thử) → đi, đánh quái thật, chat, chuyển map; e2e xanh |
| M3D-2 | Pipeline nội dung + map/nhân vật thí điểm | 3–8 (song song) | Phượng Tường 3D + 1 nam/1 nữ + 5 quái do ta làm, chơi được với zone |
| M3D-3 | Hệ thống hiển thị 3D đầy đủ | 9–12 | trang bị lên người, kỹ năng 10 phái có hiệu ứng, camera/ môi trường/ tương tác/ minimap |
| M3D-4 | Bản 2.5D dự phòng cho 980 map | 6–9 (song song) | mọi map 2D chạy trong thế giới 3D (nền + billboard), chuyển liền mạch với map 3D thật |
| M3D-5 | Mở rộng nội dung theo lộ trình người chơi | 13+ (liên tục) | mỗi map/nhân vật 3D thay dần bản 2.5D theo thứ tự cấp |
| M3D-6 | Hiệu năng, thiết bị, phát hành | 13–14 | 60 FPS máy yếu, Android, patcher, cài đặt |

## Đối chiếu với 12 mục thiếu (PHAN-TICH-3D-THIEU.md mục 5)

| Mục thiếu | Bước trong lộ trình |
|---|---|
| 1 Quyết định hướng | 0.1 |
| 2 Tích hợp thế giới 3D vào client thật | 0.3, 1.1–1.6 |
| 3 Map 3D cho thế giới JX1 | 0.4, 2.1–2.3, M3D-4 (2.5D dự phòng), M3D-5 |
| 4 Model nhân vật/quái 3D | 2.4, M3D-5 |
| 5 Trang bị lên người, thú cưỡi | 2.5, 3.6 |
| 6 Hiệu ứng kỹ năng 3D | 1.3, 3.1 |
| 7 Camera & hiển thị | 3.2 |
| 8 Vật liệu & môi trường | 3.3 |
| 9 Tương tác trong thế giới | 1.4, 3.4 |
| 10 Hiệu năng & tải map | 6.1 |
| 11 Âm thanh 3D | 3.5 |
| 12 Công cụ & quy trình | 2.1, 2.2, 6.3 |

## M3D-0 — Quyết định và nền (tuần 1)

| Bước | Việc | Kết quả / kiểm |
|---|---|---|
| 0.1 | **ADR-008 "Client 3D"**: thay ADR-001/007 phần client (Godot 4.7, 3D, renderer Mobile hay Compatibility?), giữ zone/gateway/giao thức/Lua; chính sách tài sản (tự làm, tham khảo không phát hành) | ADR có chữ ký chủ dự án |
| 0.2 | **Quy ước toạ độ/đơn vị**: zone giữ đơn vị scene (ô 32); client 3D: `X = x·k, Z = y·k`, chọn `k` để nhân vật cao 1,8 m (đề xuất 1 ô = 1 m → k = 1/32); hướng: +Z tới (Unity/glTF), yaw = 180 + góc; cao độ chỉ để vẽ | ghi `docs/3D-QUY-UOC.md`; test đơn vị đổi qua lại |
| 0.3 | **Tách lớp vẽ thế giới** trong client 2D (`UiGame.gd`, `KNpc.gd`): giao diện `IWorldView` (load_map, add/remove_entity, move(path), play_action, camera_follow, pick, world_to_screen); `KNpc` = trạng thái, `KNpcView2D` = hình | test client 417 vẫn xanh; 2D chạy như cũ |
| 0.4 | Định dạng **map 3D** cho zone + client: `maps/<id>/map3d.json` (glTF, lightmap, nav, spawn, cổng, vùng, nhạc), tool `navmesh → obstacle.bin` | `jxassets validate-3d` |

## M3D-1 — Thế giới 3D nối zone thật (tuần 2–4)

| Bước | Việc | Kết quả / kiểm |
|---|---|---|
| 1.1 | `KWorldView3D` (từ `Scn3D`): thực thi `IWorldView` — nạp map3d, camera quỹ đạo, navmesh, tên/máu 2D | mở map thử bằng client thật |
| 1.2 | Thực thể từ zone: `EntitySpawn/Move/Action/Life/Despawn` → `Scn3DNpc` (vị trí đổi đơn vị, đi theo waypoint của zone, hướng), nhân vật chính = zone điều khiển (không tự đi) | 2 client cùng thấy nhau đi |
| 1.3 | Hành động: `EntityAction` (đánh thường/kỹ năng/bị đánh/chết/hồi sinh) → clip theo nhóm vũ khí + hiệu ứng đúng thời điểm; số sát thương nổi; `G2C_MISSLE` → đạn 3D | đánh quái thật, quái chết, rơi đồ |
| 1.4 | Tương tác: bấm chọn NPC/quái (Area3D theo bán kính), khung mục tiêu, hội thoại NPC (stub M13), nhặt đồ, cổng chuyển map (`G2C_CHANGE_MAP` → nạp map3d hoặc 2.5D) | e2e `--auto`: login → đi → đánh → chuyển map |
| 1.5 | UI 2.0 hiện có đè lên thế giới 3D (thanh dưới, túi, kỹ năng, chat, trạng thái) — không sửa UI, chỉ nối `world_to_screen` | 95 kiểm tra UI vẫn xanh |
| 1.6 | Map thử: Ba Lăng 3D (tài sản tham khảo, chỉ nội bộ) + NPC/quái JX1 đặt theo bảng của zone (ánh xạ tên) | mốc M3D-1 đạt |

## M3D-2 — Pipeline nội dung và thí điểm (tuần 3–8, song song với M3D-1)

| Bước | Việc | Ai | Kết quả / kiểm |
|---|---|---|---|
| 2.1 | **Tài liệu cho họa sĩ**: đơn vị mét, +Z tới, tỉ lệ nhân vật 1,8 m, texture 1024/2048 BC7, UV2 cho lightmap, đặt tên node (terrain/water/building/tree/grass), điểm `start/end` vũ khí, điểm treo `hang_*`, `sys_bar/sys_bd/sys_foot` | dev | `docs/3D-HOA-SI.md` + Blender mẫu |
| 2.2 | **Bộ nhập**: glTF → kiểm tra (đơn vị, tên, vật liệu, số tam giác), nướng lightmap (`LightmapGI`, Godot editor headless), nướng navmesh (`NavigationMesh` từ hình học) → `obstacle.bin`, xuất `map3d.json` | dev | `jxassets import-3d <map>` chạy một lệnh |
| 2.3 | **Map thí điểm Phượng Tường**: blockout theo `Region_C.dat` (nền = texture tile cũ trải phẳng làm nháp, nhà/tường/cây = khối) → art → bake | họa sĩ | đi được, NPC/cổng đúng chỗ như 2D |
| 2.4 | **Nhân vật thí điểm**: xương chung, 1 nam + 1 nữ, bộ animation theo bảng hành động 2.0 (đứng/đi/chạy/đánh theo vũ khí/nội công/bị đánh/chết/ngồi/cưỡi), 5 quái tân thủ | họa sĩ | thay nhân vật tham khảo trong client |
| 2.5 | **Trang bị lên người**: phần da theo `item_list` (mũ/áo/giày/vũ khí) như `npcres` 2.0 ghép bộ phận | dev + họa sĩ | đổi đồ thấy trên người |

## M3D-3 — Hệ thống hiển thị 3D đầy đủ (tuần 9–12)

| Bước | Việc | Kết quả / kiểm |
|---|---|---|
| 3.1 | Hiệu ứng kỹ năng: bộ soạn hiệu ứng trong Godot (particle + mesh + đèn + trail), bảng `skill_id → hiệu ứng theo khung` (thi triển / bay / trúng / trạng thái) cho 10 phái, vệt đao theo vũ khí, hiệu ứng buff | mỗi phái ≥ 5 kỹ năng có hiệu ứng |
| 3.2 | Camera: làm mờ/ẩn nhà chắn, va chạm mềm, pitch/zoom theo map (`cameraInit`), rung khi trúng | ảnh so sánh |
| 3.3 | Môi trường: nước, lá/cỏ đung đưa, ngày/đêm, sương theo vùng, thời tiết mưa/tuyết | 3 map mẫu |
| 3.4 | Minimap: chụp từ trên xuống lúc nhập map → `*_big.png` + khung toạ độ; radar; dịch chuyển nhanh | như `ui_map_view` |
| 3.5 | Âm thanh 3D: đòn/đạn theo vị trí, nhạc nền theo vùng, tiếng môi trường | |
| 3.6 | Thú cưỡi: animation cưỡi + điểm treo `ma_qi1`, tốc độ từ zone | |

## M3D-4 — Bản 2.5D dự phòng cho toàn bộ 980 map (tuần 6–9, song song)

Mục đích: game chơi được **hết** thế giới JX1 trong thế giới 3D ngay cả khi map 3D chưa dựng.

| Bước | Việc | Kết quả / kiểm |
|---|---|---|
| 4.1 | Nền map 2D (`Ground.dat` tile) → texture lớn trải lên mặt phẳng 3D (kéo giãn dọc ×2 để bỏ nén 30°), vật cản = `obstacle.bin` | 980 map nạp tự động |
| 4.2 | Vật thể 2D (`BuildinObj`) → billboard tại điểm chân, thứ tự vẽ theo độ sâu; nhân vật/quái = sprite 8 hướng 2.0 billboard, chọn hướng theo yaw camera | camera khoá pitch 30°, yaw tự do |
| 4.3 | Chuyển liền mạch: map có `map3d.json` → 3D thật, không có → 2.5D; camera tự đổi giới hạn | đi từ Phượng Tường 3D sang map 2.5D không lỗi |

## M3D-5 — Mở rộng nội dung (tuần 13+, liên tục)

Thứ tự theo lộ trình người chơi JX1: tân thủ (thôn khởi đầu 10 thôn) → 10 thành thị → dã ngoại theo cấp (10–90) → động/mê cung theo
cấp → phó bản/chiến trường (Tống Kim, Phong Lăng Độ, Bạch Hổ Đường). Giá mỗi map (đo theo bộ tham khảo: 5–70 MB, 0,3–1,6 triệu tam giác):

| Loại | Blockout | Art | Bake/nav/NPC | Kiểm |
|---|---|---|---|---|
| Thôn/động nhỏ (≈ 150×150 m) | 1 ngày | 3–4 ngày | 0,5 ngày | 0,5 ngày |
| Thành thị (≈ 400×400 m) | 2 ngày | 8–12 ngày | 1 ngày | 0,5 ngày |
| Dã ngoại lớn (≈ 500×500 m) | 2 ngày | 6–8 ngày | 1 ngày | 0,5 ngày |

Nhân vật/quái: mỗi quái 2–3 ngày (model + rig + 6 animation), nhân vật chính mỗi giới 2 tuần (rig + bộ animation 30 clip + phần da theo
trang bị). Số lượng gợi ý cho bản chơi thử: 10 thôn + 3 thành + 8 dã ngoại + 6 động ≈ **27 map**, 2 nhân vật chính, 40 quái, 30 NPC
→ khoảng **6–8 tháng** cho 1 họa sĩ, 3–4 tháng cho 2 họa sĩ. Phần còn lại chạy 2.5D.

## M3D-6 — Hiệu năng, thiết bị, phát hành (tuần 13–14)

| Bước | Việc | Kiểm |
|---|---|---|
| 6.1 | Tải/giải phóng map khi chuyển, texture nén, LOD/`visibility_range`, occlusion, gộp draw call | 60 FPS máy văn phòng (GPU tích hợp), 30 FPS Android tầm trung |
| 6.2 | Cài đặt hiển thị (chất lượng, bóng, khoảng nhìn), hàng đợi đăng nhập, đăng ký, patcher tải map theo yêu cầu | |
| 6.3 | Kiểm thử: `--auto` mỗi map (chụp 5 góc + FPS + navtest), e2e với zone, CI chạy trên máy có GPU | |

## Phụ thuộc và rủi ro

- **Nội dung là đường găng**: không có họa sĩ thì chỉ tới M3D-1 + M3D-4 (2.5D) — vẫn là "3D camera" nhưng hình vẫn 2D.
- **Renderer**: GL Compatibility đủ cho phong cách lightmap + mesh tĩnh (đã đo 145 FPS); nếu chọn Mobile thì bỏ xuất web.
- **Zone giữ 2D**: mọi map 3D phải không có tầng đi được chồng nhau; cầu/lầu chỉ là hình.
- **Bản quyền**: mọi tài sản 剑网江湖 xoá khỏi bản phát hành; chỉ để so sánh trong `assets3d/` (đã gitignore).

## Việc làm ngay tuần này (nếu chốt hướng)

1. Viết ADR-008 + quy ước toạ độ (0.1–0.2).
2. Tách `IWorldView` trong client 2D (0.3) — test 417 xanh.
3. Định dạng `map3d.json` + tool `navmesh → obstacle.bin`, chạy với Ba Lăng thử (0.4).
4. Bắt đầu `KWorldView3D` nối `EntitySpawn/Move` (1.1–1.2) — mốc đầu: 2 client thấy nhau đi trong Ba Lăng 3D.
5. Song song: tài liệu họa sĩ + Blender mẫu (2.1), chọn người dựng Phượng Tường.
