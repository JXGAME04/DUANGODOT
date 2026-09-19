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

## Cách nối vào dự án chính nếu được duyệt

Chỉ thay lớp "vẽ thế giới" của client (~2 300–2 500 dòng: `KScenePlaceC`, `KIpo*`, `KNpc/KNpcRes`, `KObj`, `KMissle*`,
phần map/camera/pick trong `UiGame`); zone, services, giao thức, UI giữ nguyên. Điều kiện: bản 2D tách lớp vẽ sau một giao diện
(xem HANDOVER 4b phần 33 và ghi chú trong cuộc trao đổi 2026-09-19).
