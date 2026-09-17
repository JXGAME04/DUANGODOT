# Máy chủ Linux `D:\ServerLinux` — mổ nhị phân (M10)

Tài liệu này là kết quả **mổ nhị phân** bản server Linux (JX2/Kiếm Thế bản VNG) mà chủ dự án giao,
để lấy **toàn bộ hệ thống settings và bộ hàm script** cho JX NEXT làm được như bản Linux. Mọi con
số ở đây đọc thẳng từ tệp nhị phân bằng `next/tools/re/re_elf.py`, không đoán.

> Đây là **bản đồ để làm M11–M13** (vật phẩm, chiến đấu/kỹ năng, nhiệm vụ + bộ hàm script). Nó liệt
> kê **cái gì có** và **ở đâu**; chữ ký từng hàm dựng dần khi làm từng hệ, theo mẫu ở §5.

## 1. Ba tệp thực thi

| Tệp | Vai trò | Kích thước | Ghi chú |
|---|---|---:|---|
| `server1/jx_linux_y` | **GameServer** — mô phỏng thế giới, chạy script | 8,9 MB | ELF 32-bit i386, **không có section header** |
| `gateway/s3relay/s3relay_y` | Relay/gateway | | ELF 32-bit i386 |
| `server1/libheaven.so`, `librainbow.so` | Thư viện engine (KGLua, mạng) | 183 KB, 84 KB | **có symbol**, dễ đối chiếu |
| `server1/KG_Angel.so`, `GameExtConnect.so`, `GameExtContent.so` | Chống sửa / mở rộng | 4 MB | |

`jx_linux_y` **không dùng UPX**. Nhưng có **một segment `rwx` 5,7 MB entropy ~8.0** (mã hoá, giải
mã lúc chạy — có lẽ là lớp bảo vệ của `KG_Angel`), và `.fini` trỏ vào đó. **Code thật của game và
các bảng đăng ký Lua nằm trong segment `r-x` đầu (`0x08048000`–`0x082DA338`), ở dạng rõ** — đó là
chỗ mổ. Bảng chương trình:

```
LOAD va 08048000..082DA338 file 0+292338 r-x   <- code + bảng Lua + chuỗi settings (RÕ)
LOAD va 082DB000..097B5ED4                rw-   <- dữ liệu ghi
LOAD va 09B6C190..0A0E720E                rwx   <- 5,7 MB entropy 8.0 (lớp bảo vệ, bỏ qua)
entry 0804B7C0 -> __libc_start_main(main=0x0804B880)
```

Cần: `libdl libuuid libpthread libstdc++ libm libgcc_s libc`. 312 symbol động, 248 stub PLT.

## 2. Bộ công cụ (`next/tools/re/re_elf.py`)

Phân tích ELF không section header **qua program header + DYNAMIC**, y như loader:

```bash
python re_elf.py <elf> info                 # segment, thư viện cần, entry
python re_elf.py <elf> imports [lọc]        # hàm ngoài + stub PLT gọi nó
python re_elf.py <elf> strings <regex>      # chuỗi (giải GBK/latin-1)
python re_elf.py <elf> xrefstr <text>       # code nào nhắc tới chuỗi này (tìm nơi đọc 1 tệp settings)
python re_elf.py <elf> dis <va> [n]         # disassemble, chú thích call@plt và chuỗi
python re_elf.py <elf> func <va>            # cả một hàm
python re_elf.py <elf> luamap               # bảng đăng ký {tên, hàm} cho Lua
```

## 3. Bộ hàm script — 1506 (game) + 438 (gateway)

`jx_linux_y` đăng ký **1506 hàm** cho Lua, `s3relay_y` **438 hàm**. Cách tìm: quét mọi cặp dword
liên tiếp `{con trỏ tên C hợp lệ, con trỏ vào .text}` thành dãy ≥ 3 cặp (đó là bảng đăng ký), rồi
**lọc theo prologue**: hàm thật bắt đầu bằng `55` (`push ebp`) — bỏ hết cặp mà "con trỏ hàm" thực ra
trỏ vào chuỗi (đó là nhiễu). Đối chiếu với bản đồ của chủ dự án (`ReverseTools/*.luamap.full.txt`):
trùng ~1550 tên, phần lệch hai bên đều là từ khoá Lua / rác — bộ lọc prologue của ta sạch hơn.

- Danh sách đầy đủ + địa chỉ: [`linux/jx_linux_luaapi.txt`](linux/jx_linux_luaapi.txt) (1506),
  [`linux/s3relay_luaapi.txt`](linux/s3relay_luaapi.txt) (438).
- Nhóm theo miền: [`linux/jx_linux_luaapi_nhom.txt`](linux/jx_linux_luaapi_nhom.txt).

Số hàm theo miền (tên có chứa từ khoá):

| Miền | Số hàm | | Miền | Số hàm |
|---|---:|---|---|---:|
| Bang hội / công thành (`Tong*`) | 194 | | PK / sát khí | 10 |
| Vật phẩm, túi đồ, trang bị | 123 | | Tổ đội | 10 |
| NPC | 84 | | Bang xếp hạng, thẻ thần thú | 4 |
| Nhiệm vụ | 61 | | Danh hiệu | 9 |
| Cấp / kinh nghiệm | 42 | | Trận doanh / phe | 9 |
| Kỹ năng + chiêu thức | 53 | | Xe / thuyền / bến | 7 |
| Nhân vật người chơi | 36 | | Chiến trường | 6 |
| Thú cưng / đồng hành | 35 | | Tiền tệ | 4 |
| Bản đồ / cảnh / vùng | 25 | | Thành tựu | 4 |
| Giao dịch / chợ / đấu giá | 15 | | **Khác** (chưa phân loại) | 775 |

**JX NEXT hiện đăng ký 3 hàm** (`Include`, `print`, `IncludeLib` trong `server/zone/src/KLuaScript.cpp`)
cộng với các hàm mà npclevelscript cần. Khoảng cách tới 1506 chính là phần việc M11–M13.

## 4. Hệ thống settings — 102 tệp

`jx_linux_y` đọc **102 tệp trong `\settings\`** (thêm 62 đường dẫn `\script\`). Danh sách đầy đủ:
[`linux/jx_settings_files.txt`](linux/jx_settings_files.txt). Vài tệp cột mốc cho M11–M13:

| Tệp | Cho hệ |
|---|---|
| `settings\Skills.txt`, `Missles.txt`, `magicdesc.ini`, `THIEFSKILL.TXT` | Kỹ năng, chiêu thức (M12) |
| `settings\item\{Clothes,Platina,Gold}EquipRes.txt`, `Melee/Range/Armor/Helm/HorseRes.txt`, `goods.txt`, `AbradeRate.ini` | Vật phẩm, trang bị (M11) |
| `settings\task\missions.txt`, `timertask.txt`, `systemtimetask.txt` | Nhiệm vụ, hẹn giờ (M13) |
| `settings\npc\player\{level_exp,level_add,BaseValue,NewPlayerBaseAttribute}.*` | Chỉ số theo cấp |
| `settings\NpcS.txt`, `npcres\人物类型.txt`, `npc\NpcGoldTemplate.txt`, `PKRate.ini` | NPC, rơi tiền |
| `settings\maplist.ini`, `revivepos.ini`, `Weather\Weather.ini` | Bản đồ, hồi sinh, thời tiết |
| `settings\tong\*`, `citywar.ini` | Bang hội, công thành (194 hàm!) |
| `settings\gamesetting.ini`, `attribconstdata.ini`, `taxrates.ini` | Hằng số toàn cục |

Tìm **nơi đọc** một tệp và **các cột nó đọc**: `re_elf.py <elf> xrefstr "<tên tệp>"` → địa chỉ hàm
đọc, rồi `func <va>`. Ví dụ `gamesetting.ini` đọc ở `0x0805EA59` và `0x0805F3CD`; `NpcS.txt` ở
`0x0805F005`.

## 5. Quy ước gọi hàm script (mẫu cho M11–M13)

Các hàm này **không** nhận `lua_State*` kiểu chuẩn. Chúng nhận **đối số nguyên/thực trực tiếp**
(engine KGLua bọc lại), y như bindings Lua 4 của bản JX1 (`Sources\...\Script.cpp`). Ví dụ đọc từ
`re_elf.py dis`:

```
GetLevel @ 081111E0:
  mov  ebx, [ebp+8]        ; đối số 1 = chỉ số nhân vật (đẩy sẵn trên khung)
  call 0x8107860           ; -> chỉ mục thật trong bảng nhân vật
  imul eax, eax, 0x8788    ; sải bảng player = 0x8788 byte
  mov  eax, [eax+edx+0x3f4]; đọc trường ở offset 0x3f4
  ...                      ; trả về (fild -> fstp qword): Lua nhận số thực
```

Nghĩa cho JX NEXT: đăng ký **cùng tên, cùng quy ước đối số nguyên**; thân hàm ánh xạ sang
`KNpc`/`KPlayer` của zone mới. `dac_ta_17_ham_hoatdong_phuong.json` của chủ dự án
(`D:\GAMEDEVNEW\ReverseTools`) đã đặc tả sẵn 17 hàm hoạt động theo cách này — dùng làm mẫu.

## 6. Ba cây nguồn — đừng mở nhầm (theo `HUONGDAN_DICHNGUOC_TINHNANG_LINUX.md` của chủ dự án)

`D:\ServerLinux` có **ba** gốc con: `server1` (kịch bản + bảng), `gateway` (lịch chạy ở
`relaysetting`), `Patch\settings` (34 bảng `server1` không có). Tệp Linux **trộn hai bảng mã trong
một dòng** (tiếng Việt VNI/TCVN + tiếng Trung GBK); dùng `ReverseTools\port_3hd\dec2.py::decline2`
để giải, `gbktool`/`iconv` giải sai.

## 7. Bước kế tiếp

M10 xong phần **liệt kê + phân loại + định vị**. M11–M13 làm từng hệ: với mỗi hệ, `xrefstr` các tệp
settings của nó để biết cột nào được đọc, `func` từng hàm Lua của hệ để lấy chữ ký, rồi hiện thực
trong zone mới với **cùng tên, cùng số**, đối chiếu kết quả với server Linux đang chạy.
