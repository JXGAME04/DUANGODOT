# ADR-008: Client 3D — thế giới vẽ bằng 3D, luật chơi và server giữ nguyên

- **Trạng thái**: đã chốt (2026-09-19, chủ dự án: "làm từng bước đến khi xong"), đang thực hiện theo `docs/LO-TRINH-3D.md`
- **Thay thế**: phần *client* của [ADR-001](ADR-001-kien-truc.md) (client 2D sprite) và dòng "Client" của
  [ADR-007](ADR-007-tuoi-tho-cong-nghe.md); mọi phần khác của hai ADR đó giữ nguyên
- **Liên quan**: [ADR-002](ADR-002-giao-thuc.md) (giao thức không đổi), `docs/PHAN-TICH-3D-THIEU.md`, `docs/THU-NGHIEM-3D.md`,
  `docs/3D-QUY-UOC.md`

## Bối cảnh

Thử nghiệm `exp/3d-baling` (HANDOVER phần 33–35) chứng minh Godot 4.7 chạy được một map 3D có lightmap, camera quỹ đạo,
494 nhân vật có skin + animation, vũ khí treo tay, hiệu ứng hạt và đi lại theo navmesh ở 145 FPS (GL Compatibility).
Bản phân tích `PHAN-TICH-3D-THIEU.md` mục 5 liệt kê 12 mục còn thiếu để thành game 3D chuẩn; mục 1 là quyết định này.

Chủ dự án chốt hướng 3D và yêu cầu **mỗi dòng mã áp dụng phải có nguồn từ mổ nhị phân, không đoán mò**.

## Quyết định

| Tầng | Quyết định | Nguồn chân lý (nhị phân) |
|---|---|---|
| Server (zone C++, gateway/auth/persist Go), giao thức protobuf, Lua 5.4 | **Không đổi.** Zone vẫn tính bằng đơn vị scene 2D của bản cũ (ô 32, region 512 × 1024, hướng 0..63, 18 Hz); map 3D không có tầng đi được chồng nhau (cầu, lầu chỉ là hình) | server Linux `jx_linux_y` (LINUX-SERVER.md) |
| Client — luật hiển thị (khi nào đứng/đi/chạy/đánh, số khung, quay mặt, tên/thanh máu đặt ở đâu, chọn mục tiêu, kéo thả, UI) | **Giữ nguyên `KNpc`/`UiGame`/UI 2.0 đã port** — chỉ tách "lớp vẽ thế giới" ra sau một giao diện (`IWorldView`) có hai bản: `KWorldView2D` (cũ) và `KWorldView3D` (mới) | client 2.0 `gamecl.exe` (CLIENT-2.0.md) |
| Client — cách vẽ 3D (camera, hướng nhìn, tỉ lệ nhân vật, điểm treo vũ khí/tên, hiệu ứng theo khung, chuyển động mượt) | Theo **bản tham khảo 剑网江湖** (Unity 2022.3 IL2CPP): bảng `scn_list.cameraInit`, `cha_pic.sys_bar/sys_bd/cp_radius`, `hang_item`, `skill_childobj/sfx_object/anim_effect`, `animation_list`; số nào không đọc được từ bảng/nhị phân thì ghi rõ **"tự chọn"** trong mã và tài liệu | `D:\game3gTQ` (game3gtq-reference-client, BAO-CAO-MAP-3D.md) |
| Engine | Godot 4.7, GDScript, renderer **GL Compatibility** (đo 145 FPS map tham khảo; giữ xuất web/Android) — đổi sang Mobile chỉ khi đo thấy cần | — |
| Dữ liệu 3D | glTF 2.0 + PNG/KTX + JSON do ta xuất (`map3d.json`, `npc_models.json`, `weapons.json`, `sfx/*.json`), đơn vị **mét**, quy ước trong `docs/3D-QUY-UOC.md` | — |
| Tài sản | Model/map của bản tham khảo **chỉ để đo và thử nội bộ** (`client/assets3d/`, gitignore); bản phát hành dùng tài sản tự làm theo pipeline M3D-2. Không commit khóa giải mã | — |
| Đường lùi | Mọi map không có `map3d.json` chạy ở chế độ **2.5D** (nền tile 2D trải phẳng + sprite billboard) trong cùng thế giới 3D — 980 map luôn chơi được; `main` giữ bản 2D tới khi M3D-1 qua e2e | — |

## Phương án đã cân nhắc

- **Giữ 2D, chỉ thêm camera xoay**: không làm được — sprite 8 hướng không xoay được quá 30° (mục 2.1 của bản phân tích).
- **Viết client 3D mới từ đầu (Unity/Unreal)**: mất toàn bộ UI 2.0 đã port đúng điểm ảnh và 417 test client; license và tuổi thọ
  kém Godot (ADR-007).
- **Đổi zone sang toạ độ mét / 3D**: vô ích — luật chơi bản Linux là 2D (vật cản ô 32, tầm đánh theo đơn vị scene); đổi đơn vị
  là đổi luật. Đổi ở lớp vẽ rẻ hơn và kiểm chứng được.

## Hệ quả

- `UiGame` không được gọi thẳng `Camera2D`, `get_global_mouse_position`, `KScenePlaceC`: mọi thứ qua `IWorldView`
  (`pick`, `screen_to_scene`, `follow`, `add_entity`…). Test client phải xanh ở cả hai bản vẽ.
- Mỗi bước của `LO-TRINH-3D.md` là một commit trên `exp/3d-baling`, có ghi HANDOVER và có kiểm thử tự động (`--auto`, e2e).
- ADR-007 bảng tầng: dòng "Client" thêm "3D: glTF + JSON do ta xuất; không addon C++".
