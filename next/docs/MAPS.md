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
| `.pak` | `bin/Client/Data/*.pak`, thứ tự trong `package.ini` (pak đầu thắng) | header `PACK`; index `{id, offset, size, flag}`; id = hash tên file (chữ thường, byte GBK tính như *signed char*); nén UCL NRV2B (0x01/0x20), zlib cho spr "zip", `0x10` = spr nén theo frame |
| `.spr` | trong pak | `SPRHEAD` 32 byte, bảng màu RGB 3 byte × Colors, bảng offset frame, mỗi frame `{w,h,ox,oy}` + RLE `[count][alpha][count chỉ số màu nếu alpha≠0]` |
| `.wor` | `\maps\<GBK>\<GBK>.wor` | INI: `[MAIN] rect=l,t,r,b` = chỉ số region |
| `Region_C.dat` | `\maps\<GBK>\<GBK>\v_YYY\XXX_Region_C.dat` | 6 phần: vật cản 16×32 int32, bẫy, NPC, obj, nền (`Ground.dat`: tile 16×16 ô + cover), vật thể (`BuildinObj.dat`) |
| Chuỗi Việt | `Settings/*.txt`, `.wor`, NPC | TCVN3 (bảng trong `pkg/jxold/text`) — tên thư mục/đường dẫn là GBK |

## 2. Hệ toạ độ

- **Scene** (đơn vị của zone, của game cũ): region = 512 × 1024; ô vật cản 32 × 32 → 16 × 32 ô/region.
- **Màn hình**: `sx = x`, `sy = y / 2` (`KRepresentShell2::CoordinateTransform`; vật thể có `z`:
  `sy = y/2 − z·887/1024`). Region trên màn hình là 512 × 512 px; tile nền 32 × 32 px.
- Gốc toạ độ bundle = region `(rect.left, rect.top)`; các vị trí trong file cũ tính từ region 0 đã được quy về gốc này.
- Hướng nhân vật `dir` 0..7: 0 = xuống, ngược chiều kim đồng hồ.

## 3. Bundle

```text
client/assets/
  .gdignore                    editor Godot bỏ qua thư mục (nạp lúc chạy bằng Image.load)
  maps/<id>/map.json           kích thước, spawn, danh sách region có dữ liệu, NPC (tên UTF-8, toạ độ scene)
  maps/<id>/obstacle.bin       1 byte/ô, hàng trước cột, 0 = đi được          <- zone A*, client
  maps/<id>/rXXX_YYY.json      tiles {x,y,s,f} (px so với gốc region), objects {x,y,sy,s,f,n,l}
  sprites/<id8>.png/.json      atlas 1 sprite (id = hash pak), frames {x,y,w,h,ox,oy}, center, directions, interval
```

`l` của object: `cover` (vẽ trên nền, dưới nhân vật), `object` (sắp xếp theo `sy` cùng nhân vật), `above` (trên tất cả).
`n` > 1: vật thể có hoạt ảnh, đổi frame mỗi `interval` tick cũ (18 tick/giây).

## 4. Zone dùng gì

`zone.map_dir` trong `config/zone.json` → `KMapData` (`server/zone/include/jx/zone/KMapData.h`): lưới đi được,
điểm spawn, NPC. `MoveReq` → điểm đến gần nhất đi được → **A\*** 8 hướng (không cắt góc) → làm mượt
theo tầm nhìn → `EntityMove.path` (waypoints). Client đi đúng các waypoint đó với cùng tốc độ.

## 5. Chưa có (làm ở mốc kế)

- Sprite nhân vật/quái (`npcres`: ghép đầu/thân/vũ khí theo hướng, hành động) — hiện vẽ chấm tròn.
- Ánh sáng, thời tiết, nhạc nền, minimap (`KLittleMap`), bẫy/cổng dịch chuyển (`Trap.dat` đã đọc, chưa dùng).
- Chỉ số NPC từ `Settings/npcs.txt` (mới có tên + template id từ map).
