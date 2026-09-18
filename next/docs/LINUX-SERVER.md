# Máy chủ Linux `D:\ServerLinux` — mổ nhị phân (M10)

Tài liệu này là kết quả **mổ nhị phân** bản server Linux (JX2/Kiếm Thế bản VNG) mà chủ dự án giao,
để lấy **toàn bộ hệ thống settings và bộ hàm script** cho JX NEXT làm được như bản Linux. Mọi con
số ở đây đọc thẳng từ tệp nhị phân bằng bộ công cụ `next/tools/re/` (§2), không đoán; chỗ nào có
thể là suy luận đều ghi rõ "suy luận" và ghi cách kiểm.

> Đây là **bản đồ để làm M11–M13** (vật phẩm, chiến đấu/kỹ năng, nhiệm vụ + bộ hàm script):
> mỗi hàm script có chữ ký (§5), mỗi tệp settings có danh sách cột/khoá mà mã đọc (§4, §6).

## 1. Ba tệp thực thi

| Tệp | Vai trò | Kích thước | Ghi chú |
|---|---|---:|---|
| `server1/jx_linux_y` | **GameServer** — mô phỏng thế giới, chạy script | 8,9 MB | ELF 32-bit i386, **không có section header** |
| `gateway/s3relay/s3relay_y` | Relay/gateway | | ELF 32-bit i386 |
| `server1/libheaven.so`, `librainbow.so` | Thư viện engine (KGLua, mạng) | 183 KB, 84 KB | **có symbol**, dễ đối chiếu |
| `server1/KG_Angel.so`, `GameExtConnect.so`, `GameExtContent.so` | Chống sửa / mở rộng | 4 MB | |

`jx_linux_y` **không dùng UPX**. Nhưng có **một segment `rwx` 5,7 MB entropy ~8.0** (mã hoá, giải
mã lúc chạy — lớp bảo vệ của `KG_Angel`), và `.fini` trỏ vào đó. **Code thật của game, các bảng đăng
ký Lua và mọi chuỗi nằm trong segment `r-x` đầu (`0x08048000`–`0x082DA338`), ở dạng rõ** — đó là
chỗ mổ. Lưu ý: `.rodata` **nằm chung segment với `.text`**, nên "một địa chỉ trong segment code" chưa
chắc là code; chuỗi là "cái gì đọc được như chuỗi". Bảng chương trình:

```
LOAD va 08048000..082DA338 file 0+292338 r-x   <- code + .rodata (chuỗi, bảng Lua) — RÕ
LOAD va 082DB000..097B5ED4                rw-   <- .got (082DB45C..) + dữ liệu ghi
LOAD va 09B6C190..0A0E720E                rwx   <- 5,7 MB entropy 8.0 (lớp bảo vệ, bỏ qua)
entry 0804B7C0 -> __libc_start_main(main=0x0804B880)
```

Cần: `libdl libuuid libpthread libstdc++ libm libgcc_s libc`. Đoạn `DYNAMIC` đã bị lớp bảo vệ **viết
lại** (SYMTAB/STRTAB/JMPREL trỏ vào segment `rwx`): 312 symbol, 248 stub PLT của lớp bảo vệ ở
`0x09B6C290`. Nhưng **`.rel.plt` gốc vẫn còn ở `0x0804A654` (183 mục `R_386_JMP_SLOT`)** và chỉ số
symbol trong đó **vẫn khớp** bảng symbol hiện tại — kiểm bằng 5 stub đã biết chức năng từ thân hàm
gọi chúng (`strtol` sau `GetKeyValue`, `sprintf`, `strncpy`, `strtod` trước `fstp`, `__cxa_atexit` ngay
sau constructor của đối tượng toàn cục): cả 5 đúng. Vậy **183 stub PLT gốc `0x0804Axxx` đều có tên**
(`re_elf.py imports`, tổng 431 stub) và `dis` chú thích `call 0x804b30c ; strtol@plt`.

## 2. Bộ công cụ (`next/tools/re/`)

| Công cụ | Việc | Lệnh |
|---|---|---|
| `re_elf.py` | Đọc ELF không section header qua program header + DYNAMIC (như loader); segment, import (cả PLT gốc), chuỗi GBK, xref, disassemble có chú thích, bảng đăng ký Lua | `info`, `imports`, `strings <regex>`, `xrefstr <text>`, `dis <va> [n]`, `func <va>`, `luamap` |
| `re_calls.py` | **Đồ thị gọi hàm** theo từng hàm (không quét tuyến tính — quét tuyến tính lạc nhịp ở dữ liệu giữa các hàm). Theo dõi hằng trong thanh ghi/ô stack/`[esp+N]` để biết **mỗi lời gọi truyền chuỗi, đối tượng nào**; hiểu `this` = đối số 1, thành viên `this+off`, đối tượng trên stack, tail-call (`jmp` tới prologue), khối sau `ret` sớm (trạng thái lấy từ nhánh nhảy tới đó), thanh ghi cất/khôi phục qua `[ebp-N]`. Cache `<elf>.calls.json` (~10 s, 6027 hàm / 35 074 lời gọi) | `build`, `callers <va>`, `strargs <va>`, `byarg <text>` |
| `re_luasig.py` | **Chữ ký** từng hàm script (§5): đối số đọc bằng API Lua 4.0 nào, có xem `lua_gettop` không, có cần nhân vật không, đẩy gì, trả mấy giá trị; theo cả hàm bọc (tail-jump và gọi thường với `L`), cả khối nằm sau epilogue | `callees`, `sig <tên>`, `all <out.tsv>` |
| `re_tables.py` | **Bảng settings ↔ cột/khoá** (§6): nối `KTabFile::Load(obj, path)` với mọi `Get*(obj, row, "Cột")` qua đồ thị gọi hàm, kể cả bảng truyền qua đối số, đường dẫn ghép lúc chạy, đọc theo chỉ số cột, ghi/`Save` | `objects`, `columns`, `report <out.md>` |
| `re_settings.py` | Bản thô hơn: hàm nào nhắc tới đường dẫn + các hằng chuỗi hàm đó dùng (giữ để tra nhanh) | `list`, `file <text>`, `all` |
| `re_tabdesc.py <elf> <reader-va>…` | mảng mô tả cột `{kiểu, đích, mặc định}` của một `KBPT_*::ReadRow` (bộ đọc chung `0x081ECF40`) |
| `re_upx.py <exe>` | mở `gamecl.exe` (UPX 3.03) của client 2.0 thành ảnh bộ nhớ cho các công cụ này ([CLIENT-2.0.md](CLIENT-2.0.md)) |
| `re_scan.py` | Ba phép quét mọi hàm (§9): **ai đọc/ghi thành viên ở offset** (`disp 1f8`), **hàm nào đổ bảng con trỏ hàm thành viên** (`pmf` — tìm ctor của `KNpcAttribModify`, `KProtocolProcess`), **lệnh có hình dạng** (`ins <regex>`) | `disp <hex>[,…] [mnemonic]`, `pmf [min]`, `ins <regex> [max]` |

Chạy: `python re_<tool>.py D:/ServerLinux/server1/jx_linux_y <lệnh>` (cần `pip install capstone`).

## 3. Bộ hàm script — 1506 (game) + 438 (gateway)

`jx_linux_y` đăng ký **1506 hàm** cho Lua, `s3relay_y` **438 hàm**. Cách tìm: quét mọi cặp dword
liên tiếp `{con trỏ tên C hợp lệ, con trỏ vào .text}` thành dãy ≥ 3 cặp (đó là bảng đăng ký), rồi
**lọc theo prologue**: hàm thật bắt đầu bằng `55` (`push ebp`) — bỏ hết cặp mà "con trỏ hàm" thực ra
trỏ vào chuỗi (đó là nhiễu). Đối chiếu với bản đồ của chủ dự án (`ReverseTools/*.luamap.full.txt`):
trùng ~1550 tên, phần lệch hai bên đều là từ khoá Lua / rác — bộ lọc prologue của ta sạch hơn.

- Danh sách đầy đủ + địa chỉ: [`linux/jx_linux_luaapi.txt`](linux/jx_linux_luaapi.txt) (1506),
  [`linux/s3relay_luaapi.txt`](linux/s3relay_luaapi.txt) (438).
- Nhóm theo miền: [`linux/jx_linux_luaapi_nhom.txt`](linux/jx_linux_luaapi_nhom.txt).
- **Chữ ký từng hàm**: [`linux/jx_linux_luasig.tsv`](linux/jx_linux_luasig.tsv) (§5).

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

## 4. Hệ thống settings — 109 tệp, 736 cột/khoá đã nối

`jx_linux_y` nhắc tới **101 đường dẫn `\settings\`** (`re_settings.py list`; thêm vài đường dẫn viết `settings/`
và 62 đường dẫn `\script\`) —
[`linux/jx_settings_files.txt`](linux/jx_settings_files.txt). Đi từ mã ra (mọi `KTabFile::Load` /
`KIniFile::Load`, §6) thấy **109 tệp được nạp thật** (thêm `servercfg.ini`, `gamesetting.ini` qua
hàm bọc, các tệp `\lang\%s\...` ghép lúc chạy) và **104 tệp nối được với cột/khoá mã đọc**:
[`linux/jx_settings_cot.md`](linux/jx_settings_cot.md) — mỗi tệp: đối tượng bảng, hàm nạp, từng
cột/khoá và hàm đọc nó. Vài tệp cột mốc cho M11–M13:

| Tệp | Cột/khoá đọc | Cho hệ |
|---|---:|---|
| `settings\Skills.txt` (đối tượng `0x0830AE00`, đọc ở `0x080E9200`) | 60 | Kỹ năng (M12) |
| `settings\NpcS.txt` (`0x0830AEC0`, đọc ở `0x080A2160`/`0x080A4410`) | 94 | NPC (M12) |
| `settings\gamesetting.ini` (`0x0830CF5C`, đọc ở `0x08061650`) | 116 | Hằng số toàn cục |
| `settings\item\AbradeRate.ini` | 75 | Hao mòn trang bị (M11) |
| `settings\item\{autohang,zijingao}droprate.ini` | 27 + 27 | Tỷ lệ rơi (M11) |
| `settings\tong\tong_setting.ini`, `citywar.ini` | 30, 15 | Bang hội, công thành |
| `settings\partner\*` (11 tệp) | 95 | Đồng hành |
| `settings\logset.ini` | 19 | Ghi log |
| `settings\npc\player\{level_exp,level_add,level_lead_exp}.txt` | theo chỉ số cột | Kinh nghiệm theo cấp |
| `settings\item\*Res.txt`, `goods.txt`, `buysell.txt`, `StationPrice.txt`… | theo chỉ số cột | Vật phẩm, xe |

"Theo chỉ số cột" = mã gọi `GetInteger(nRow, nColumn, …)`: tệp đọc từng cột theo **số thứ tự**, tên
cột không có trong mã — lấy từ dòng tiêu đề của chính tệp trong `D:\ServerLinux\server1\settings\`.
Còn **5 tệp** chưa nối được gì (`HorseRes.txt`, `MeleeRes.txt`, `lottery.txt`, `npcres\人物类型.txt`,
`ui\ui3\摆摊广告条.ini`: đọc qua con trỏ chưa theo dõi được) và **6 lượt đọc** ở 5 hàm chưa nối tên tệp
(liệt kê cuối báo cáo). Kiểm chứng đã làm: `logset.ini` thật chỉ có `[LogSet]` — báo cáo cũng chỉ
gán `[LogSet]` cho nó, còn `[ENCHASER]`, `[Coin]`, `[DiceGame]`… về đúng `gamesetting.ini` (tệp thật
có đúng các section ấy).

## 5. Quy ước gọi hàm script (đã kiểm — sửa lại bản trước)

> Bản trước của mục này viết "các hàm không nhận `lua_State*`, nhận đối số nguyên trực tiếp" — **sai**.
> Đọc kỹ thân hàm và các hàm chúng gọi thì đúng là API Lua 4.0 chuẩn, giống `Script.cpp` của JX1.

Mỗi hàm là `int f(lua_State* L)`; `L` ở `[ebp+8]`. Lua 4.0 **liên kết tĩnh** trong `jx_linux_y`
(`lua_State.top` +0, `Cbase` +0x10; `TObject` 12 byte; `ttype` 1 nil, 2 số, 3 chuỗi, 4 bảng, 7 userdata):

| Địa chỉ | Hàm | Cách nhận ra |
|---|---|---|
| `0x8232490` | `lua_gettop` | `(top − Cbase) / 12` |
| `0x82338B0` / `0x82339B0` | `lua_tonumber` / bản trả `int` (`fistp`) | `o = Cbase + (i−1)·12; ttype==2 → value.n` |
| `0x8233850` | `lua_tostring` | |
| `0x8232590` / `0x8232690` | `lua_type` / `lua_touserdata` | |
| `0x8232D40` / `0x82337A0` / `0x8233730` | `lua_pushnumber` / `lua_pushstring` / `lua_pushlstring` | `top->ttype = 2; top += 12` |
| `0x8232E70` / `0x8232BE0` / `0x8232F20` | `lua_pushnil` / `lua_newtable` / `lua_pushvalue` | |
| `0x8233C10` / `0x8233360` / `0x8233620` | `lua_settop` / `lua_rawseti` / `lua_getglobal` | |
| `0x8245AF0` `0x8245BB0` `0x8245C90` `0x8245CE0` `0x8245B60` | KGLua: `check_number`, `check_string`, `check_type`, `has_arg`, `opt_number` | bọc các hàm trên |
| `0x8107860` (và `0x8107910` nhảy tới) | **`GetPlayerIndex(L)`** | `lua_getglobal(L,"PlayerIndex")`; nil → −1; else `lua_tonumber` |
| `0x8106A40` | `GetSubWorldIndex(L)` | global `"SubWorld"` |

Ví dụ `GetLevel @ 0x081111E0` (đọc bằng `re_elf.py dis`):

```
mov  ebx, [ebp+8]            ; L
mov  [esp], ebx ; call 0x8107860        ; GetPlayerIndex(L): global "PlayerIndex" -> i
lea  edx,[eax-1] ; cmp edx,0x4ae ; ja lỗi   ; 1 <= i <= 0x4AE (MAX_PLAYER)
imul eax, eax, 0x8788        ; sizeof(KPlayer) = 0x8788
mov  edx, [0x8baee60]        ; g_pPlayer
mov  eax, [eax+edx+0x3f4]    ; KPlayer::m_nIndex (chỉ số Npc)
imul eax, eax, 0x1a4c        ; sizeof(KNpc) = 0x1A4C
mov  edx, [0x836eae0]        ; Npc[]
fild dword [eax+edx+0x20]    ; KNpc::m_Level
fstp qword [esp+4] ; mov [esp], ebx ; call 0x8232d40   ; lua_pushnumber(L, level)
mov  eax, 1 ; ret            ; trả 1 giá trị
```

Nghĩa cho JX NEXT: đăng ký **cùng tên, cùng đối số Lua** (JX NEXT dùng Lua 5.4 nhưng ngữ nghĩa
`PlayerIndex`/`SubWorld` là global do engine đặt trước khi chạy script — giữ nguyên). Chữ ký của cả
1506 hàm ở [`linux/jx_linux_luasig.tsv`](linux/jx_linux_luasig.tsv) (cột: tên, địa chỉ, `args`
"i:kiểu" đọc ở chỉ số i, `gettop` = có xem số đối số (đối số tuỳ chọn), `player` = có gọi
`GetPlayerIndex`, `pushes`, `returns`, kích thước). Thống kê: **1149** hàm đọc đối số ở chỉ số cố định,
894 xem `lua_gettop` (đối số tuỳ chọn hoặc nhiều dạng), 711 cần nhân vật, 1496/1506 biết số giá trị
trả về. Ví dụ đọc ra: `SetPos(x, y)` (+tuỳ chọn), `GetPos() → 3` (map, x, y), `GetTask(id) → số|nil`,
`SetTask(id, giá trị)`, `Msg2Player(chuỗi)`. Tệp `dac_ta_17_ham_hoatdong_phuong.json` của chủ dự án
(`D:\GAMEDEVNEW\ReverseTools`) đặc tả sẵn 17 hàm — dùng đối chiếu.

## 6. Hai lớp đọc tệp: `KTabFile` và `KIniFile` (tên theo header cũ, địa chỉ đã kiểm từng thân hàm)

Header gốc: `SwordOnline/Sources/Engine/Src/KTabFile.h`, `KITabFile.h`, `KIniFile.h`. `KTabFile` kích
thước **0x20** (`vtable`, `m_Width` +4, `m_Height` +8, `m_Memory` +0xC, `m_OffsetTable` +0x14) — khớp
`lea eax,[ebx+0x20]` khi một lớp chứa 5 bảng liền nhau (`MeleeRes` +0, `RangeRes` +0x20, `ArmorRes`
+0x40, `HelmRes` +0x60, `HorseRes` +0x80 ở `0x08068D90`). Thứ tự vtable theo `KITabFile`: `+0x10 FindRow`,
`+0x14 FindColumn`, `+0x18 GetWidth`, `+0x1C GetHeight`, `+0x20…` các `Get*`.

| Địa chỉ | `KTabFile::` | Đối số (`[esp]` = this) | Cách nhận ra |
|---|---|---|---|
| `0x082284E0` | `Load(path)` | `[esp+4]` đường dẫn | mọi đường dẫn `.txt` đi qua đây |
| `0x08228170` | `GetInteger(nRow, szColumn, nDefault, int*, bColumnLab)` | cột `[esp+8]` | `FindColumn` → `GetValue` → `strtol` |
| `0x082280C0` | `GetFloat(nRow, szColumn, …)` | cột `[esp+8]` | … → `strtod` → `fstp dword` |
| `0x08228220` | `GetString(nRow, szColumn, lpDefault, buf, size, bColumnLab)` | cột `[esp+8]`, mặc định `[esp+12]` | |
| `0x08227E90` | `GetInteger(szRow, szColumn, …)` | dòng theo **tên** `[esp+4]` | gọi vtable `+0x10 FindRow`, `+0x14 FindColumn` |
| `0x08227FA0` | `GetString(szRow, szColumn, …)` | | |
| `0x08227E10` | `GetInteger(nRow, nColumn, …)` | **theo chỉ số**, không tên cột | `GetValue(row−1, col−1)` thẳng |
| `0x08227F40` | `GetString(nRow, nColumn, …)` | theo chỉ số | |
| `0x08227AD0` | `FindColumn(szColumn)` | | quét dòng 0 (tiêu đề), `strcmp`, trả `i+1` |
| `0x08227B50` | `FindRow(szRow, nStartRow)` | | quét cột 0 từ dòng `nStartRow` |
| `0x08227A00` | `GetValue(nRow, nColumn, buf, size)` (private) | | |
| `0x08228030` | `Str2Col("AB")` | | cột theo **chữ cái** khi `bColumnLab = 0` |
| `0x082279D0` / `0x08228630` / `0x082285A0` | `Clear()` / constructor / destructor | | ctor đặt vtable rồi `KMemClass()`, sau đó `__cxa_atexit` |

| Địa chỉ | `KIniFile::` | Đối số | Cách nhận ra |
|---|---|---|---|
| `0x08220140` | `Load(path)` | `[esp+4]` | |
| `0x0821F2C0` | `GetInteger(section, key, nDefault, int*)` | `[esp+4]`, `[esp+8]` | `GetKeyValue` (0x0821ED00) → `strtol` |
| `0x0821F340` | `GetString(section, key, default, buf, size)` | | |
| `0x0821F1F0` | `GetInteger2(section, key, int*, int*)` | | tách bằng `,` |
| `0x0821EF40` | đọc nhiều giá trị `x,y,…` (`GetRect`/`GetStruct`) | | |
| `0x0821F7C0` / `0x0821F810` | `WriteInteger` / `WriteString` | | `sprintf("%d")` → `SetKeyValue` (0x0821F3A0) — **ghi**, không đọc |
| `0x0821FE80` | `Save(path)` | | ghi từng dòng `"%s\r\n"` |
| `0x08220280` / `0x08220240` | constructor / destructor | | |

Tệp được **ghi** (không chỉ đọc): `AntiBotConfig.ini` (đọc 11, ghi 7 khoá), `rolenamechangehis.ini`
(ghi 4), `SysAlarmConfig.ini` (3/3), `ServerCfg.ini` (ghi 1) — xem báo cáo.

## 7. Ba cây nguồn — đừng mở nhầm (theo `HUONGDAN_DICHNGUOC_TINHNANG_LINUX.md` của chủ dự án)

`D:\ServerLinux` có **ba** gốc con: `server1` (kịch bản + bảng), `gateway` (lịch chạy ở
`relaysetting`), `Patch\settings` (34 bảng `server1` không có). Tệp Linux **trộn hai bảng mã trong
một dòng** (tiếng Việt VNI/TCVN + tiếng Trung GBK); dùng `ReverseTools\port_3hd\dec2.py::decline2`
để giải, `gbktool`/`iconv` giải sai.

## 8. Bước kế tiếp

M10 xong: **liệt kê + phân loại + định vị + chữ ký + cột**. M11–M13 làm từng hệ: lấy các cột của hệ
từ `jx_settings_cot.md`, chữ ký các hàm Lua của hệ từ `jx_linux_luasig.tsv`, hiện thực trong zone mới
với **cùng tên, cùng số**, đối chiếu kết quả với server Linux đang chạy. Khi cần đọc sâu một hàm:
`re_elf.py func <va>` (đã có tên PLT), `re_calls.py callers <va>` để biết ai gọi.

## 9. Đã tìm thấy khi làm M11 (vật phẩm) — địa chỉ đã kiểm từng thân hàm

Cách tìm khi không có tên hàm: (1) hàm đổ **bảng con trỏ hàm thành viên** lộ ra bằng `re_scan.py pmf`
(nhiều `mov [reg+disp], imm(mã)` liên tiếp; phần tử 8 byte {ptr, adj}); (2) từ một hàm đã biết, lần
theo `re_calls.py callers` và theo **offset thành viên** bằng `re_scan.py disp`; (3) chuỗi RTTI
`NKBPT_…` → typeinfo → vtable (4 ô: dtor, đọc dòng, tìm dòng, nạp) cho các lớp bảng vật phẩm.

| Địa chỉ | Là gì | Bằng chứng |
|---|---|---|
| `0x08099600` | ctor `KNpcAttribModify` (217 ô `ProcessFunc[]`, đối tượng toàn cục `0x08BAC120`, bảng từ +4, phần tử 8 byte: chỉ số = (disp − 4) / 8) | `pmf`; static init `0x08099E90` |
| `0x08095B40` | `KNpcAttribModify::ModifyAttrib(this, nIdx, pNpc, pMagic, extra)` — `type ≤ 0x12d`, gọi `ProcessFunc[type]` | đọc `[ebx + type*8 + 4]` rồi `call ecx` |
| `0x0807D210` | `KNpc::ModifyAttrib(this, nIdx, pData, extra)` → hàm trên | 29 nơi gọi |
| `0x08097E70` | `ProcessFunc[153]` = **LifePotionV**: `time = max(t1,t2)`, `value = (x1·t1 + x2·t2)/time`; `m_LifeState` = `KNpc+0x1f0` (value), `+0x1f8` (time) | giống nguồn Windows từng dòng |
| `0x08097DE0` | `ProcessFunc[154]` = ManaPotionV: `+0x200` / `+0x208` | |
| `0x0809A2F0` | `ProcessFunc[190]` = `lifereplenish_p`: cộng vào `KNpc+0x1194` (phần trăm hồi máu, log `"(Percent)"`) | |
| `0x0808B610` | **`KNpc::ProcessState`**: `+0x1184` m_RegionIndex, `+0x1904` m_LoopFrames (% 10), `+0x224` m_Doing (8 = do_sit), `+0x1190` hồi máu tự nhiên × `+0x1194`/100 (`"AddLife: %d * %d%% = %d"`), `+0x118c` m_CurrentLife chặn ở max(`+0x1a14`, `+0x1a18`) và ≥ 0; `+0x11a0`/`+0x11a4` nội lực; rồi các trạng thái mỗi frame: `+0x1c8` độc, `+0x1d8` đóng băng, `+0x1e8` choáng, **`+0x1f8` thuốc máu** (`0x0808B7BC`: `time--`, `% 10 == 0` → `+0x1f0 × percent/100`, chỉ chặn trên), `+0x208` thuốc nội | `disp 1f8`; chuỗi `"AddLifeState: %d * %d%% = %d"` |
| `0x080DA560` | ctor `KProtocolProcess` (106 ô `ProcessFunc[c2s_…]`) | `pmf` |
| `0x08204710` | **`KItemList::EatMecidine(this, nIdx)`**: `Player[m_PlayerIdx]` (0x8788 byte, `g_pPlayer` 0x8BAEE60), `Item[]` 0x830D300 (0x368 byte); chết (`m_Doing == 10`) → không; `Check_ItemUsable` trong `\script\item\forbiditem.lua`; genre 1 thuốc → `0x08204858`: cờ `KNpc+0x147a` (forbit_takemedicine), đếm `Player+0x86a4/+0x86a8`, `ApplyMagicAttribToNPC(npc, 3)` (`0x08068560`), `events.lua OnUseItem`, stack (`Item+0x14` xếp chồng, `+0x308` nStack, `+0x30c` max) → `SetItemStack(n−1)` (`0x08200D30`, gói s2c 168) hoặc `Remove` (`0x082006B0`) + `ItemSet.Remove` (`0x0806DB90`); ngồi (8) → đứng (`0x08078AA0`); genre 5 (`0x08204B00`) phù, 6 (`0x082049B8`) kịch bản | chuỗi `"Check_ItemUsable"`, `"OnUseItem"` |
| `0x08068560` | `KItem::ApplyMagicAttribToNPC(npc, nActive)` = `0x080669B0` (7 thuộc tính cơ bản, `Item+0xe4`) + `0x08066890` (6 ma pháp `Item+0x154`, hậu tố lẻ đếm `nActive`); `0x08068440` gỡ (giá trị đảo dấu) | |
| `0x081FD880` | `KItemList::UnEquip` bản JX2 (ô trang bị `this + place*8 + 0xc`, gỡ thuộc tính, tính lại bộ 15 ô — bộ đồ) | |
| `0x081ED430` | **`KBPT_Medicine` đọc dòng** qua bộ đọc chung `0x081ECF40` (mảng mô tả 12 byte {kiểu, đích, mặc định}; kiểu 0 số → `GetInteger(row+2, col)`, 1 chuỗi, 2 **bỏ cột**): 19 cột = tên(+0, 0x50), genre +0x50, detail +0x54, particular +0x58, [ảnh bỏ], obj +0x5c, rộng +0x60, cao +0x64, [giới thiệu bỏ], hệ +0x68, giá +0x6c, cấp +0x70, **是否叠放 +0x74**, 2 thuộc tính (+0xa4…+0xb8). Dòng 0xbc byte; `FindRecord(detail, level)` = `0x081ED5D0` | RTTI `13KBPT_Medicine`, vtable `0x0826A758` |
| `0x08206110` | **`KItemList::ExchangeItem`** (6,8 KB): mô hình tay cầm như Windows; phòng ở `this+0x4ca8/+0x4cc4/+0x4d18`; `FindItem` `0x081F8850`, `PickUpItem` `0x081F8720`, `PlaceItem` `0x081F8520`, `CheckSameDetailType` `0x08065A70`; gói cho client `0x080A8400` | chuỗi `"%s exchange item error"` |
| `0x080A3B80` | **bộ nạp bảng rơi đồ** (`KItemDropRate::Load`): `KIniFile::GetInteger` từng khoá với mặc định — `[Main] Count, RandRange, MagicRate, MoneyRate=20, MoneyScale=50, MinItemLevelScale=20, MaxItemLevelScale=10, MaxItemLevel=10, MinItemLevel=1, Series=−1, EnchasableRate=0, MinSocket=1, MaxSocket=1, IsTeamShare=0, TeamShareRate=0`; `[i] Genre, Quality, Detail, Particular, RandRate, MinItemLevel=−1, MaxItemLevel=−1, Series=−1, EnchasableRate=−1, MinSocket=−1, MaxSocket=−1, MagicLevel1..6` (0x44 byte/dòng, mảng ở +0x3c) | `ini_reads.py` (scratch) |
| `0x080A4410` → `0x080A41E0` | đọc `NpcS.txt` (`Treasure`, `DropRateFile`) và nạp/cache bảng rơi đồ theo đường dẫn | |
| `0x08088B60` | **`KNpc::OnDeath` phần rơi đồ**: `m_CurrentTreasure` (+0x1378) lần: `g_Random(100) < MoneyRate(+0xc)` → `LoseMoney 0x08081950`, không thì `LoseSingleItem 0x08088840`; `SubWorld+0x4f298` ≠ 0 → không rơi; `IsTeamShare` → `0x08089188`; rồi script `OnGlobalNpcDeath` | |
| `0x08081950` | `KNpc::LoseMoney`: `m_CurrentExperience(+0x1188) × MoneyScale(+0x10) / 100 × g_ServerConfig.MoneyRate(0x0830C7E4, `[ServerConfig] MoneyRate` mặc định 100) / 100` | |
| `0x08083BB0` | **`GenRandomItem(player, pDropRate, npcLevel, flag)`**: dòng theo `RandRate` cộng dồn trên `g_Random(RandRange)`; series/enchasable/socket −1 → của bảng; `genre 0 && quality ∈ {1,4}` → `detail −= 1`; may mắn = `Player+0x5958` (+ thưởng qua `0x080CC620`); cấp: `lo=(L−1)/MaxScale+1, hi=(L−1)/MinScale+1`, kẹp [MinItemLevel, MaxItemLevel] rồi `1..10`; ma pháp: `k = g_Random(4)+3`, ô `i ≤ k` = cấp (quality 0 và `g_Random(100) ≥ EnchasableRate`), quality 2 / khảm → ô = −1 (lỗ), quality := 2; `MagicLevel1..6` của dòng ghi đè; `KItemSet::Add(genre, quality, series, level, luck, detail, particular, magic[], version, 0, "GenRandomItem"…)` | chuỗi `"GenRandomItem"` |
| `0x08088840` | `KNpc::LoseSingleItem`: `0x080CC320` kiểm; `GenRandomItem`; `GetFreeObjPos 0x080F0950`; `KObjSet::Add 0x080A6A70` (`Item+0x10` = dòng ObjData); `SetItemBelong 0x080A4D90` = `{belong, belong ≥ 0 ? 600 : 0}` | |
| `0x080B8210` | `KPlayer::ServerPickUpItem` bản JX2: khoảng cách² so `0x9C40` (40000) | |
| `0x080B5180` / `0x08205830` | `KPlayer::AddItem` / `KItemList::Add` (đích của Lua `AddItem` `0x08120D30` → `0x08120B30` → `0x0811F230` đọc 18 đối số Lua → `0x0806E110` sinh vật phẩm) | `re_calls` |
| `0x0806E110` → `0x0806DD30` → `0x0806BA10` | **`KItemSet::Add`** → sinh vật phẩm: `version = −1` → `g_nItemVersion` (`0x9777F34`), phải ≤ nó; `Item+0x1fc` = version, `+0x1e0` = seed; rồi **bảng nhảy theo genre** (`0x08252D28`): genre 0 → theo quality: 0 `Gen_Equipment 0x0806B3A0`, 1 `Gen_GoldEquip 0x0806A150`, 2 `0x0806B6C0` (bạch kim: gọi Gen_Equipment rồi đổi ô −1 thành lỗ), 4 `0x0806A9D0`; genre 1 `0x0806BBA0`, 4 `0x0806BB28`, 5 `0x0806BB10`, 6 `0x0806BAD8`, 7 `0x0806BA98`, 8 `0x0806BA50`, 9 `0x0806BB40`, 10 `0x0806BB70`; genre 2, 3 → không | `re_calls`, đọc bảng nhảy bằng `Elf.u32` |
| `0x0806B3A0` | **`KItemGenerator::Gen_Equipment(this, detail, particular, series, level, pnaryMALevel, luck, pItem, bNew)`**: bNew → chép `*pItem` vào bản tạm, `+0x1e0 = g_GetRandomSeed()`, `+0x1fc = this+0x5a4c` (phiên bản bộ bảng), `+0x200 = luck`; `bMagic = pnaryMALevel && detail ≠ 11` (mặt nạ); `+0x1e4..` = 6 cấp ma pháp; dòng bảng `0x080690D0(detail, particular, level)`; `SetAttrib_CBR 0x08068100`; `+0x28 = series`; có ma pháp → `Gen_MagicAttrib`, rồi bNew → `CheckNewItemAttrib 0x08069D10`; rớt → `g_Random(100)`, **làm lại tối đa 0x15 = 21 lần** rồi trả 0; xong `SetAttrib_MA 0x08065710`, `+0x354 = time(0)`, chép về `*pItem`; khôi phục seed | chuỗi `"KItemGenerator::Gen_Equipment(%s) Error"` |
| `0x0806AF70` | **`KItemGenerator::Gen_MagicAttrib(this, type, pnaryMALevel, series, luck, pnaryMA, version)`**: với mỗi ô i khi `level[i] ≠ 0`: `pos = 1 − (i & 1)`; `GetCMIT(pos, type, series, level) 0x08070B50` (NULL → log + dừng); **`nDecide`**: `version > 3` → `g_Random(1000000)·100/(10·luck+100)`; `≤ 1` → `g_Random(100)/(luck/10+1)`; 2–3 → `g_Random(1000000)/(luck/10+1)`; duyệt ứng viên: bỏ `m_nUseFlag` (dòng+0x1a8), bỏ `DropRate[type]` (dòng+0x178+type·4) `≤ nDecide`, bỏ trùng `nPropKind` (+0x15c) với ô trước; không còn → dừng; chọn `Get(g_Random(n))`; `type = kind`, `value[k] = min_k + g_Random(max_k − min_k + 1)` (+0x160..+0x174); xoá `UseFlag` cuối hàm. Dòng `KMAGICATTRIB_TABFILE` 0x1ac byte: +0 pos, +4 tên, +0x54 class, +0x58 level, +0x15c kind, +0x160 3 khoảng, +0x178 `DropRate[12]`, +0x1a8 UseFlag | chuỗi `"[GenMagicAttrib]"` |
| `0x08070B50` / `0x08070D00` / `0x08070BD0` | `KLibOfBPT::GetCMIT` (pos ≤ 1, type ≤ 11, series ≤ 4, level 1..10 → `this+0x1f98 + pos·0x1c20 + type·0x258 + series·0x78 + (level−1)·12`, ô `{ptr, cap, count}`); bộ dựng `m_CMAIT` (mỗi dòng: `UseFlag = 0`; mọi type có `DropRate ≠ 0`; `class −1` → hệ 0..4; **cấp từ cấp dòng tới 10**); `InitMALib` (`m_CMAT[2][12]` ở `+0x57d8/+0x5868`) | |
| `0x0806BE00` / `0x08069D10` | bộ nạp **`magicattrib_limit.txt`** (`\settings\item\%03d\`; cột `MagicType, Min1, Max1, Min2, Max2, Min3, Max3`, mặc định −1, `type` 1..0x153, `std::map` ở `this+0x5a54`, nút `+0x10 type, +0x14 Max1..3, +0x20 Min1..3`; trùng type → `"mapCheckNewItemAttrib Have Same Check Data"`) / **`CheckNewItemAttrib(this, sMA)`**: mỗi thuộc tính có dòng giới hạn, mỗi k có `Max_k ≠ −1`: **rớt khi `value ≤ Min_k` hoặc `Max_k ≤ value`** (bảng 004: `139 0 0…` cấm hẳn `allskill_v`) | chuỗi `"CheckNewItemMagicAttrib"` |
| `0x08065710` | `KItem::SetAttrib_MA`: chép 6×16 byte vào `Item+0x154`; kind `0x2b` (43 `indestructible_b`) → `Item+0x308` (độ bền) = −1 | |
| `0x0806A150` | `KItemGenerator::Gen_GoldEquip(this, row, pItem, bNew)`: `row < this+0x170c` (số dòng), dòng 0x16c byte tại `this+0x1708`; `Item+0x80 = row`, `+4 = 1` (hoàng kim), nhóm bộ `+0x270/+0x274/+0x2a4..`; `SetAttrib_CBR`; 6 ma pháp từ `magicattrib_ge` (dòng+0x154.., bảng `this+0x1a44`) | chuỗi `"Gen_GoldEquipment Error"` |
| `0x0806C2E0` / `0x0805EA10` | `KItemGenerator::Init(this, version)` (`this+0x5a4c = version`, `KLibOfBPT::Init 0x080719C0` đọc thư mục `/settings/item/%03d`); `g_ItemGenerator = 0x0830D408` (new 0x5a68 byte) được Init với **`g_nItemVersion 0x9777F34`** | |
| `0x080F6A40` | **ctor `KSubWorldSet`** (`g_SubWorldSet 0x9777F00`): `+0x34 = 4` — **phiên bản vật phẩm hiện hành của bản này là 4** (thư mục `004`), không đọc từ tệp; Lua `ITEM_GetLatestItemVersion 0x081542C0` trả nó | `re_scan disp 34 mov` |
| `0x08227E10` / `0x08227A00` | **`KTabFile::GetInteger(row, col, default, &out)`** / `GetValue`: ô **trống** (độ dài 0) hoặc ngoài bảng → **mặc định**, còn lại `strtol` — khác nguồn Windows (`atoi("")` = 0). Bộ đọc dòng của từng bảng đưa mặc định theo cột: `tools/re/re_tabdesc.py` (`0x081ED830` trang bị: hệ 0, giá 0, cấp 1, khác −1; `0x081EEE30` magicattrib: mọi số −1; `0x081ED430` thuốc) | `re_tabdesc` |
| `0x08226AD0` / `0x08226B00` / `0x08226AC0` | **`g_Random(n)`** = `seed = seed·0xF25 + 0x7385; seed % n` (unsigned; `n = 0` → 0) — đúng `Engine/Src/KRandom.cpp` (IA 3877, IC 29573); `g_GetRandomSeed` / `g_RandomSeed` (`0x082E76C4`) | |

Bảng tên thuộc tính ma pháp của bản JX2 (`MAGIC_ATTRIB_STRING`, ctor `KMagicDesc` `0x080724B0` đổ
`mov [0x0830E640 + id*4], "tên"`, 0x155 ô, 335 tên, id tới 340) — lưu ở
[`linux/jx_linux_magicattrib.tsv`](linux/jx_linux_magicattrib.tsv) và trong Go
`pkg/jxold/item/KMagicAttribNames.go` (client dùng làm khoá tra `\settings\magicdesc.ini`). Khớp
`ProcessFunc`: 153 `lifepotion_v`, 154 `manapotion_v`, 155 `physicsresmax_p`, 190 `lifereplenish_p`
(hệ số hồi máu ở `KNpc+0x1194`).
