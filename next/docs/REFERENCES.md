# Ba nguồn tham chiếu (bản chuẩn để đối chiếu 1‑1)

JX NEXT không có dữ liệu riêng: mọi map, sprite, bảng số liệu đều **xuất từ game cũ** và mọi luật đều
**port từ mã cũ**. Từ 2026‑09‑17 có ba cây tham chiếu, mỗi cây "chuẩn" cho một việc:

| Cây | Ở đâu (máy này) | Chuẩn cho | Ghi chú |
|---|---|---|---|
| **Mã nguồn** JX1 | `SwordOnline/Sources` (repo này) + `D:\GAMEDEVNEW\Sources\LINUX.zip` | **thuật toán**: KNpc, KNpcAI, KSubWorld, KScenePlaceC, KNpcRes… | `LINUX.zip` = bản Kingsoft port GameServer sang Linux (2003, `readme.txt`), cùng `Core/` với swrod3 (`KNpcAI.cpp` giống hệt), Lua 4.0 |
| **Server Linux** (VNG chạy thật) | `D:\ServerLinux\server1` | **dữ liệu + luật phía server**: `settings/npcs.txt`, `settings/*`, `script/**/*.lua` (Lua 4.0), `pak/maps.pak` (Region_S: NPC thật), `lang/vn/replacename_npc.txt` | chỉ có nhị phân (`libheaven.so`, `librainbow.so`) → không đọc mã được; dùng script + bảng + disasm (`D:\GAMEDEVNEW\ReverseTools\linuxbc`) |
| **Client VLTK 2.0** | `C:\Users\<you>\Level Up Games\Vo Lam Truyen Ky 2.0` | **hình ảnh**: `data/*.pak` (map.pak, spr.pak, resource.pak, slistcl.pak…) | `gamecl.exe` nén UPX, `Represent=3`; danh sách pak nằm trong `config.ini` mục `[Package]` (không có `package.ini`); `settings\maplist.ini`, `settings\npcs.txt`, `settings\npcres\*` nằm trong `slistcl.pak` |

Tài liệu bạn đã viết về hai bản này: `D:\GAMEDEVNEW\HUONGDAN_DICHNGUOC_TINHNANG_LINUX.md` (quy trình dịch
ngược tính năng từ bản Linux, 13 bẫy), `BANGIAO_CANH_VLTK2_1009.md` (client 2.0 vẽ cảnh thế nào),
`ReverseTools\pak_vltk\vltk2\pak_tim.py` (tra tên trong pak 2.0).

## 1. Trỏ công cụ vào hai bản

`config/oldgame.local.json` (gitignored; mẫu `config/oldgame.example.json`):

```json
{
  "client": "C:\\Users\\<you>\\Level Up Games\\Vo Lam Truyen Ky 2.0",
  "client_fallback": "D:\\<repo>\\bin\\Client",
  "server": "D:\\ServerLinux\\server1"
}
```

- `python tools/dev.py assets` đọc file này → `jxassets -client "<client>;<client_fallback>" -server <server>`.
- `jxassets` nhận thư mục client có `package.ini` **hoặc** `config.ini` (2.0); `-client "a;b"` là **chuỗi dự phòng**:
  `a` là bản chuẩn, `b` chỉ phục vụ file mà `a` **không có**, và cuối mỗi lệnh xuất log
  `files served by the fallback client folder(s)` liệt kê đúng những file đó (`-log-level debug` in đủ).
- Không có file cấu hình → như cũ: `../bin/Client`, `../bin/Server` (hoặc `JX_OLD_CLIENT`, `JX_OLD_SERVER`).

## 2. Những gì client 2.0 **không có** (lấy từ client dự án qua chuỗi dự phòng)

Đo bằng `python tools/dev.py assets` (map 1):

| Nhóm | File | Hệ quả |
|---|---|---|
| Bảng bộ phận nhân vật chính | `settings\npcres\男主角*.txt`, `女主角*.txt` (部件列表, 头部, 躯体, 贴图顺序表, 未骑马关联表…) — 2.0 không có dưới bất kỳ đường dẫn nào đã thử | bảng lấy từ client dự án, **sprite** `spr\npcres\man|woman\*.spr` vẫn lấy từ `spr.pak` của 2.0 khi có |
| Bảng động tác | `动作编号表.txt` (75 action), `npc动作表.txt` (14 doing), `主角动作阴影对应表.txt` | như trên (bảng của engine, không đổi giữa các bản) |
| Số liệu người chơi / trang bị mặc định | `settings\npc\player\BaseValue.ini`, `settings\item\{Helm,Armor,Melee,Range}Res.txt` | như trên |
| Sprite map 1 | `水波_01/33/34.spr`, `flower_1028_01.spr`, `灌木丛.spr` | 2.0 thiếu 5 ảnh này (client 2.0 cũng không vẽ chúng); lấy từ client dự án để không mất bụi/hoa |

Tổng: 7 file cho map, 68 file cho npcres (đều được log). Muốn "đúng 2.0 tuyệt đối" thì bỏ `client_fallback`
→ `export-npcres` sẽ dừng vì thiếu `动作编号表.txt`.

## 3. Bảng template NPC: server quyết luật, client quyết hình

`npcs.txt` của ba bản khác nhau (2.0: 3458 dòng · Linux: 2353 · dự án: 2631). `jxassets` ghép như client cũ khi
nối vào server thật (`export.MergeAppearance`):

- **Dòng của server** (`<server>/Settings/npcs.txt`, Linux) cho mô phỏng: tên (qua `replacename`), `Kind`, `Series`,
  `AttackSpeed/HurtFrame/DeathFrame/ReviveFrame/HitRecover`, `LifeParam`, sát thương, `AIMode/AIParam`, `LevelScript`.
- **Dòng cùng id trong pak client** (2.0) cho hình: `NpcResType`, `Helm/Armor/Weapon/HorseType`, `RideHorse`, `Stature`,
  `StandFrame/StandFrame1/WalkFrame/RunFrame` — vì client cũ tra bảng **của nó** theo id (`KNpc::Init` phía client).
  Map 1: 128 template có `NpcResType` khác giữa hai bảng (vd 481 `enemy072` → `enemy068`).
- Tên NPC: server Linux giữ bảng thay tên ở `lang\vn\replacename_npc.txt` (884 dòng, giống bản dự án trừ 1) →
  `npcres.ReplaceNameFileLang`.
- `MapList.ini` của Linux/2.0 ghi `西北南区\凤翔` (không có `\maps\`) → `mapDirPath` thêm vào; 2.0 giữ maplist trong pak
  (`mapList` đọc từ pak khi không có file thường).

Vị trí NPC map 1 của `maps.pak` Linux **trùng hệt** bản dự án (1409 NPC server + 63 client-only).

## 4. Dùng bản Linux làm gì tiếp

- `script/npclevelscript/*.lua` (`GetNpcKeyData`: `Life = 4·GetLife`, `Exp = 1.5·GetExp`, `property.lua`
  `Quadratic/Linear`) → **đã dùng**: zone nhúng Lua 5.4 và chạy đúng các script này (`KLuaScript`, `NPCRES.md` mục 4).
- `settings/npcs.txt` cột `AIMode/AIParam1..9/AIMaxTime/VisionRadius/ActiveRadius` → dữ liệu cho port `KNpcAI`.
- `script/ai/fighter.lua` (AI người chơi tự động, `SetAITime/SetVisionRadius/SetActiveRange`) → tham khảo API AI.
- Mọi số liệu hoạt động khác (task, drop, skill) — đọc `HUONGDAN_DICHNGUOC_TINHNANG_LINUX.md` trước khi port.
