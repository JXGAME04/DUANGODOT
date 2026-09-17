# Map và tài sản cũ → JX NEXT

Tool `jxassets` (Go, `services/cmd/jxassets`) đọc các `.pak` của client cũ và xuất ra **bundle**
mà cả zone C++ lẫn client Godot cùng dùng. Không sửa tay file sinh ra; sửa tool rồi xuất lại.

```bash
python tools/dev.py assets          # map 1 (Phượng Tường) -> client/assets/
python tools/dev.py assets 1 2 3    # nhiều map (id theo Settings/MapList.ini của client cũ)
```

## 1. Định dạng cũ (đã giải mã)

| Thứ | Ở đâu | Ghi chú |
|---|---|---|
| `.pak` | `bin/Client/Data/*.pak` theo `package.ini`, hoặc `data/*.pak` theo `config.ini` `[Package]` của client 2.0 (pak đầu thắng; `-client "a;b"` = chuỗi dự phòng, xem REFERENCES.md) | header `PACK`; index `{id, offset, size, flag}`; id = hash tên file (chữ thường, byte GBK tính như *signed char*); nén UCL NRV2B (0x01/0x20), zlib cho spr "zip", `0x10` = spr nén theo frame |
| `.spr` | trong pak | `SPRHEAD` 32 byte, bảng màu RGB 3 byte × Colors, bảng offset frame, mỗi frame `{w,h,ox,oy}` + RLE `[count][alpha][count chỉ số màu nếu alpha≠0]` |
| `.wor` | `\maps\<GBK>\<GBK>.wor` | INI: `[MAIN] rect=l,t,r,b` = chỉ số region |
| `Region_C.dat` | `\maps\<GBK>\<GBK>\v_YYY\XXX_Region_C.dat` | 6 phần: vật cản 16×32 int32, bẫy, NPC, obj, nền (`Ground.dat`: tile 16×16 ô + cover), vật thể (`BuildinObj.dat`) |
| Chuỗi Việt | `Settings/*.txt`, `.wor`, NPC | TCVN3 (bảng trong `pkg/jxold/text`) — tên thư mục/đường dẫn là GBK |

## 2. Hệ toạ độ

- **Scene** (đơn vị của zone, của game cũ): region = 512 × 1024; ô vật cản 32 × 32 → 16 × 32 ô/region.
- **Màn hình**: `sx = x`, `sy = y / 2` (`KRepresentShell2::CoordinateTransform`; vật thể có `z`:
  `sy = y/2 − z·887/1024`). Region trên màn hình là 512 × 512 px; tile nền 32 × 32 px.
- Gốc toạ độ bundle = region `(rect.left, rect.top)`; các vị trí trong file cũ tính từ region 0 đã được quy về gốc này.
- Hướng nhân vật `dir` 0..63 (`g_GetDirIndex` cũ, `KMath.h` zone / `KMath.gd` client): 0 = xuống, tăng theo chiều kim
  đồng hồ trên màn hình (16 = trái, 32 = lên, 48 = phải); sprite 8 hướng lấy `(dir + 4) / 8`.

## 3. Bundle

```text
client/assets/
  .gdignore                    editor Godot bỏ qua thư mục (nạp lúc chạy bằng Image.load)
  maps/<id>/map.json           kích thước, spawn, danh sách region có dữ liệu, NPC (tên UTF-8, toạ độ scene)
  maps/<id>/obstacle.bin       1 byte/ô, hàng trước cột, 0 = đi được          <- zone A*, client
  maps/<id>/rXXX_YYY.json      tiles {x,y,s,f} (px so với gốc region), objects {x,y,s,f,n,l,k,p1,p2,z1,ang,nod}
  sprites/<id8>.png/.json      atlas 1 sprite (id = hash pak), frames {x,y,w,h,ox,oy}, center, directions, interval
```

Object (`objects[]`, giữ đúng thứ tự trong file cũ):

- `x,y`: góc trên-trái trên màn hình (px, so với gốc bundle). Vật **tĩnh** đặt đúng tại ImgPos1 chiếu xuống
  (client cũ vẽ với cờ `RUIMAGE_RENDER_FLAG_FRAME_DRAW`: **không** cộng offset khung). Vật **động** (`n` > 1,
  tức `nAniSpeed > 0`) đặt tại oPos1 trừ tâm sprite (`REF_SPOT`, `KRepresentShell2::DrawScaleSprite`) và client
  cộng offset riêng của từng khung; đổi khung mỗi `interval` ms, tối thiểu 20 ms (`BuildinObjNextFrame`).
- `l`: `cover` (vẽ trên nền, dưới nhân vật), `object` (sắp xếp cùng nhân vật bằng cây KIpoTree),
  `above` (vẽ trên tất cả, theo `p1.y` rồi `z1` = oPos1.z).
- `k` (chỉ lớp `object`; suy từ header `BuildinObj.dat`: point, line, tree, above): `p` xếp theo điểm chân `p1`;
  `l` xếp theo đường đáy `p1 → p2`; `t` đường đáy chia cả cảnh (`KIpotBranch`), ảnh bị cắt tại chỗ giao.
  `ang`/`nod` = `fAngleXY`/`fNodicalY` để gộp các vật nằm cùng một đường. `p1`, `p2` tính bằng **đơn vị scene**
  (y chưa chia 2) so với gốc bundle.

### NPC trên map (`map.json` → `npcs[]`)

Hai nguồn, đều là tọa độ scene **tuyệt đối** (`KNpcSet::Add` đưa thẳng `nPositionX/Y` vào `Mps2Map`):

- **Server** (`<server>/pak/maps.pak` → `XXX_Region_S.dat`, mục Npc_S, `KRegion::LoadServerNpc`; server Linux hay
  `bin/Server` cho cùng vị trí): NPC thật (người trong thành `kind` 3, quái `kind` 0 có `level`). Tên = tên template
  trong `npcs.txt` **của server** (`<server>/Settings/npcs.txt`), hoặc tên thay thế nếu tên đặt trong map có trong
  `Settings/npc/replacename_npc.txt` (`gNpcNameMap`; server Linux: `lang/vn/replacename_npc.txt`). Hướng ban đầu 0 (`m_Dir = 0`).
- **Client** (`Region_C.dat`, mục Npc_C, `KRegion::LoadClientNpc`): thú/chim "client-only" (`client_only: true`),
  hướng đứng lấy từ khung `nCurFrame` (`GetNormalNpcStandDir` = 64·frame/tổng khung đứng).

Zone hiện sinh cả hai loại (`KSubWorld` ctor: `kind` 0 → quái, còn lại → NPC). `jxassets npcs <map> [x y]` liệt kê
để đối chiếu; `-server <thư mục>` (mặc định `../Server` cạnh client, hoặc `JX_OLD_SERVER`).

### Thứ tự vẽ (client)

Client port nguyên cây sắp xếp của game cũ: `KSceneMath.gd` (`SceneMath.cpp`), `KIpotLeaf.gd`, `KIpotBranch.gd`,
`KIpoTree.gd`. `KScenePlaceC.gd` dựng lại cây mỗi khi tập region thay đổi (đúng thứ tự cũ: vật `t` gộp theo
đường, đường dài trước; vật `l` dài trước; vật `p` theo region; rồi nhân vật), mỗi khung ghi thứ tự duyệt cây vào
`z_index` của các Sprite2D/nhân vật trong lớp `Objects`. Nhân vật là lá runtime (điểm chân `y + 6`), được rút ra /
cắm lại khi di chuyển như `KScenePlaceC::MoveObject`. Vật `t` bị cắt được vẽ bằng Sprite2D phụ với
`frame_part_texture`.

## 4. Zone dùng gì

`zone.map_dir` trong `config/zone.json` → `KMapData` (`server/zone/include/jx/zone/KMapData.h`): lưới đi được,
điểm spawn, NPC. `MoveReq` → điểm đến gần nhất đi được → **A\*** 8 hướng (không cắt góc) → làm mượt
theo tầm nhìn → `EntityMove.path` (waypoints). Client đi đúng các waypoint đó với cùng tốc độ.

## 5. Chưa có (làm ở mốc kế)

- Sprite nhân vật/quái (`npcres`: ghép đầu/thân/vũ khí theo hướng, hành động) — hiện vẽ chấm tròn.
- Ánh sáng, thời tiết, nhạc nền, minimap (`KLittleMap`), bẫy/cổng dịch chuyển (`Trap.dat` đã đọc, chưa dùng).
- Chỉ số NPC từ `Settings/npcs.txt` (mới có tên + template id từ map).
