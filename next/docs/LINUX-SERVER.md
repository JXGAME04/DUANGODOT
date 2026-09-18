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
| `re_attribmod.py <elf>` | bản đồ `KNpcAttribModify` (§10): mỗi id ma pháp → ô `KNpc` mà `ProcessFunc` của nó cộng/ghi, dừng ở tail-jump cuối hàm → `docs/linux/jx_linux_attribmod.tsv` |
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
| `0x081FD2C0` | **`KItemList::GetEquipEnhance(this, place, flag)`**: `m_PlayerIdx ≤ 0` → 0; `!flag && this+0x4c7c ≠ −1` → 3; place 10..14 → 3; > 14 → 0; `n = g_IsAccrue(hệ Npc+0x28, hệ Item+0x28)`; hai ô kích hoạt `[0x082E7460 + place·8]` có đồ → `+= g_IsAccrue(hệ đồ đó, hệ món)` | |
| `0x08074190` / `0x080741D0` | `g_IsAccrue(bảng, src, des)` = `bảng[src] == des` (src ≤ 4); bảng ngũ hành: `0x0830ED18` sinh {2,3,1,4,0}, `0x0830ED2C` khắc {1,4,3,0,2}, `0x0830ED40` được sinh bởi {4,2,0,1,3}, `0x0830ED54` bị khắc bởi {3,0,4,2,1} | `re_scan disp` |
| `0x0811D5D0` → `0x08204560` | Lua **`DelItem(detail \| tên)`**: chuỗi → `KTabFile::GetInteger(dòng theo tên, "DetailType")` trên `\settings\item\questkey.txt` (`0x9780D20`, nạp ở `0x0805D580`); `KItemList::RemoveTaskItem`: duyệt danh sách mục (`this+0x4c6c`, `+0x4c74` số mục, mỗi mục 20 byte: `+0x94` idx, `+0x98` vị trí) — mục đầu genre 4 cùng detail → `Remove 0x082006B0` + `KItemSet::Remove` (cả chồng) | chuỗi `"[TASK] Can Not Del Item"` |
| `0x0811D3B0` → `0x08204350` | `DelItemEx`: như trên nhưng chỉ mục có vị trí **3 = pos_equiproom** (túi) | |
| `0x0811D250` → `0x081F9FA0` | `HaveItem(detail \| tên)` → 1/0: mục genre 4 cùng detail ở bất kỳ phòng | |
| `0x0811D6E0` → `0x081FA010` | `GetItemCount(detail \| tên)`: **đếm số mục** (chồng = 1) genre 4 cùng detail | |
| `0x0811D830` → `0x081FC550` | `GetItemCountEx`: như trên, chỉ túi (vị trí 3) | |
| `0x0811D140` → `0x081FA080` | `HaveCommonItem(genre, detail, particular)`: −1 = bất kỳ | |
| `0x0811D4C0` → `0x08204470` | `DelCommonItem(genre, detail, particular)`: xoá mục đầu khớp | |
| `0x0811FA10` | **`AddStackItem([tag,] count, genre, detail, particular, level, series, luck[, magic…])`**: chuỗi đầu bị `lua_remove`; `Lua_NewItem 0x0811F230`; `Item+0x14` chồng được và `0 < count ≤ max(Item+0x30c,1)` → `Item+0x308 = min(count, 0xffff)`; `KPlayer::AddItem(player, idx, 1, 1, 0)` → trả idx | |
| `0x080B5180` → `0x08205830` | `KPlayer::AddItem(idx, room?, bStack, ..)` → `KItemList::Add(room, idx, bStack)`: bStack và chồng được → tìm chồng cùng loại `0x081FC2C0`, gộp `0x08202980` tới khi hết (`+0x308 < 0`); không thì đặt vào chỗ trống | |
| `0x08227E10` / `0x08227A00` | **`KTabFile::GetInteger(row, col, default, &out)`** / `GetValue`: ô **trống** (độ dài 0) hoặc ngoài bảng → **mặc định**, còn lại `strtol` — khác nguồn Windows (`atoi("")` = 0). Bộ đọc dòng của từng bảng đưa mặc định theo cột: `tools/re/re_tabdesc.py` (`0x081ED830` trang bị: hệ 0, giá 0, cấp 1, khác −1; `0x081EEE30` magicattrib: mọi số −1; `0x081ED430` thuốc) | `re_tabdesc` |
| `0x08226AD0` / `0x08226B00` / `0x08226AC0` | **`g_Random(n)`** = `seed = seed·0xF25 + 0x7385; seed % n` (unsigned; `n = 0` → 0) — đúng `Engine/Src/KRandom.cpp` (IA 3877, IC 29573); `g_GetRandomSeed` / `g_RandomSeed` (`0x082E76C4`) | |

Bảng tên thuộc tính ma pháp của bản JX2 (`MAGIC_ATTRIB_STRING`, ctor `KMagicDesc` `0x080724B0` đổ
`mov [0x0830E640 + id*4], "tên"`, 0x155 ô, 335 tên, id tới 340) — lưu ở
[`linux/jx_linux_magicattrib.tsv`](linux/jx_linux_magicattrib.tsv) và trong Go
`pkg/jxold/item/KMagicAttribNames.go` (client dùng làm khoá tra `\settings\magicdesc.ini`). Khớp
`ProcessFunc`: 153 `lifepotion_v`, 154 `manapotion_v`, 155 `physicsresmax_p`, 190 `lifereplenish_p`
(hệ số hồi máu ở `KNpc+0x1194`).

## 10. Cấu trúc `KNpc` / `KPlayer` và thuộc tính nhân vật (M12) — đã kiểm từng dòng

Cách tìm: `tools/re/re_attribmod.py` duyệt 217 `ProcessFunc` của `KNpcAttribModify` (mỗi hàm chạm
npc qua thanh ghi nạp từ `[ebp+0x10]`, đọc `pMagic->nValue[k]` ở `[ebp+0x14] + 4 + 4k`) → mỗi id ma
pháp cho biết **ô nào của KNpc** nó cộng/ghi (kết quả:
[`linux/jx_linux_attribmod.tsv`](linux/jx_linux_attribmod.tsv)); rồi đọc tay `KNpc::ClearAttrib`,
`KNpc::Init`, `KNpc::SetTemplate`, `KPlayer::LoadFrom`, `KPlayer::LevelUp`, `KPlayer::UpdataCurData`
để có chiều "gốc → hiện tại". Tên đặt theo `Core/Src/KNpc.h` của nguồn cũ (`m_LifeMax` gốc,
`m_CurrentLifeMax` hiện tại).

**Kích thước và mảng**: `sizeof(KNpc) = 0x1A4C`, mảng `g_pNpc` ở `[0x836EAE0]`, số ô `[0x830CA58]`;
`sizeof(KPlayer) = 0x8788`, `g_pPlayer` ở `[0x8BAEE60]`; `Player+0x3f4 = m_nIndex` (npc của người
chơi), `Npc+0x1908 = m_nPlayerIdx`; `Npc+0x4 = m_Index` (đối số đầu của log `0x08096A30`).
`g_PlayerSet = 0x8BACAC0` (`KPlayerSet`), `g_PlayerSet.m_cLevelAdd = 0x8BAF378` (`KLevelAdd`).

### 10.1 `KNpc` — ô gốc (`m_XXX`, +0x15a8…) và ô hiện tại (`m_CurrentXXX`)

Hai bản "hiện tại" cho máu/nội/kháng/tốc độ: bản thường và bản **`_yan_`** (id 228…245 của bảng
ma pháp — `lifemax_yan_v`…); giá trị dùng thật = **max(thường, yan)** (`ProcessState` chặn máu ở
`max(+0x1a14, +0x1a18)`, `LevelUp` đổ đầy máu bằng cùng max, `OnHurt` lấy `max(+0x1a44, +0x1a48)`).

| Gốc | Hiện tại | Yan | Tên | Từ đâu |
|---|---|---|---|---|
| `+0x20` | | | `m_Level` | người chơi: WORD `TRoleData+0xc3`; quái: mẫu `+0x1138` |
| `+0x24` | | | `m_Kind` (1 = người chơi) | mẫu `+0x20` |
| `+0x28` | | | `m_Series` | `TRoleData+0xbb` / mẫu `+0x28` |
| `+0x21c` | `+0x220` | | `m_Camp` / `m_CurrentCamp` (`changecamp_b` ghi `+0x220` khi 1..6, khác → về `+0x21c`) | `TRoleData+0xbf` / mẫu `+0x24` |
| | `+0x224` | | `m_Doing` (8 ngồi, 9 bị đánh, 10 chết, 0x12, 0x15…) | |
| | `+0x22c` / `+0x230` | | `m_Frames.nTotalFrame` / `nCurrentFrame` | |
| | `+0x248` | | `m_SkillList` (`KSkillList`: ô i = 1..79 ở `+0x250 + 0x30·i`: `+0` id, `+4` cấp, `+0x18` cấp hiện tại) | mẫu `+0x108` (0xF00 byte) |
| | `+0x1150` | | danh sách ma pháp mẫu (`+0x1150/54/58` từ mẫu `+0x1008..`) — `ClearAttrib` xoá | |
| | `+0x1180` / `+0x1184` | | `m_SubWorldIndex` (SubWorld 0x63FC8 byte, `[0x8FC81E0]`) / `m_RegionIndex` (0xCC4 byte) | |
| `+0x15a8` | `+0x1188` | | `m_Experience` / `m_CurrentExperience` = mẫu `+0xc0` × `[ServerConfig] ExpRate` (`[0x830C7E0]`) / 100 | `SetTemplate` |
| `+0x15ac` | `+0x1a14` | `+0x1a18` | `m_LifeMax` / `m_CurrentLifeMax` | `TRoleData+0xeb` / mẫu `+0x60` |
| | `+0x118c` | | `m_CurrentLife` | `TRoleData+0xf7` |
| `+0x15b0` | `+0x1190` | | `m_LifeReplenish` / `m_CurrentLifeReplenish` (người chơi gốc = 0) | mẫu `+0xc4` |
| | `+0x1194` | | `m_nLifeReplenishPercent` = 100 (`lifereplenish_p` cộng) | |
| `+0x15b4` | `+0x1a1c` | `+0x1a20` | `m_ManaMax` / `m_CurrentManaMax` | `TRoleData+0xf3` |
| | `+0x11a0` | | `m_CurrentMana` (`TRoleData+0xff`); `+0x119c` `m_nManaReplenishPercent` = 100 | |
| `+0x15b8` | `+0x11a4` | | `m_ManaReplenish` / `m_CurrentManaReplenish` (người chơi gốc = 0) | |
| `+0x15bc` | `+0x11ac` | | `m_StaminaMax` / `m_CurrentStaminaMax`; `+0x11a8` `m_CurrentStamina` (`TRoleData+0xfb`) | `GetStaminaBase(series, sex, level)` |
| | `+0x11b0` | | `m_CurrentStaminaSitAdd` = max(1, `m_CurrentStaminaMax × SitAdd / 1000`) — tính lại mỗi khi StaminaMax đổi | `stamina.ini SitAdd` |
| `+0x15c0` | `+0x11b4` | | `m_StaminaGain` / `m_CurrentStaminaGain` (người chơi = `NormalAdd`) | `stamina.ini` |
| | `+0x11b8` | | `m_PhysicsDamage` {`+0x11b8` type, `+0x11bc` min, `+0x11c0`, `+0x11c4` max} | `SetNpcPhysicsDamage` / mẫu `+0xd0..+0xdc` |
| | `+0x11c8`/`+0x11d8`/`+0x11e8`/`+0x11f8` | | `m_CurrentFireDamage` / Cold / Light / Poison (`KMagicAttrib` 16 byte; `addXdamage_v` ghi `nValue`) | |
| | `+0x1208`/`+0x1218`/`+0x1228`/`+0x1238`/`+0x1248` | | ma pháp vật lý/băng/lôi/hoả/độc (`addXmagic_v` 168–172) | |
| `+0x15c4` | `+0x1258` | | `m_AttackRating` / `m_CurrentAttackRating` | người chơi: `dexterity × 4 − 28`; mẫu `+0xc8` |
| `+0x15c8` | `+0x125c` | | `m_Defend` / `m_CurrentDefend` (`armordefense_v` 30, `adddefense_v` 150) | người chơi: `dexterity / 4`; mẫu `+0xcc` |
| | `+0x1260` / `+0x1264` / `+0x1268` | | `returnres_p` 205 / `melee_returnres_p` 299 / `range_returnres_p` 300 | |
| `+0x1608..+0x1618` | `+0x126c..+0x127c` | | `m_XResistMax` / hiện tại (thứ tự hoả, băng, độc, lôi, vật lý); người chơi từ `TRoleData+0x164..0x168` (0 → 75) | mẫu `+0x98..+0xa8` (hoả, băng, lôi, độc, vật lý) |
| | `+0x1280` / `+0x1284` | | `skill_enhance` 243 / `magicdamage_p` 244 | |
| `+0x161c` / `+0x1620` | `+0x1288` / `+0x128c` | `+0x1a2c` / `+0x1a30` (% `fastwalkrun_p` / yan) | `m_WalkSpeed` / `m_RunSpeed` (người chơi 5 / 10; mẫu `+0x50` / `+0x5c`); `0x08098A50`: bỏ phần `gốc × max(p, yan) / 100` cũ, `p += v0`, cộng lại `gốc × max(p, yan) / 100` | |
| | `+0x1294` / `+0x1298` | | `nomovespeed` 182 | |
| `+0x1624` | `+0x129c` | | `m_AttackRadius` / `m_CurrentAttackRadius` | |
| `+0x1630` | `+0x12a4` | | `m_VisionRadius` / `m_CurrentVisionRadius` (người chơi 120; `visionradius_p` 112) | mẫu `+0xb0` |
| `+0x1638` | `+0x12ac` | | `m_ActiveRadius` / `m_CurrentActiveRadius` | mẫu `+0xac` |
| | `+0x12b0`… | | các cặp `anti_*` (2 dword mỗi cặp): `+0x12b0` anti_hitrecover, `+0x12b8` anti_stuntimereduce, `+0x12c0/c8/d0/d8/e0` anti_{physics,fire,cold,lighting,poison}res, `+0x12e8..+0x1308` bản yan, `+0x1310` anti_sorbdamage_yan, `+0x1318` anti_block_rate, `+0x1320` anti_enhancehit_rate, `+0x1328` anti_do_stun, `+0x1330` do_stun, `+0x1338` anti_do_hurt, `+0x1340` do_hurt, `+0x1348` anti_poisontimereduce; `0x08078F10` xoá tất cả | |
| `+0x1640` | `+0x1378` | | `m_Treasure` / `m_CurrentTreasure` | mẫu `+0x68` |
| | `+0x137c` / `+0x1380` / `+0x1384` / `+0x1388,+0x138c` / `+0x1394` / `+0x1398,+0x13a0` | | `meleedamagereturnmana_p` / `rangedamagereturnmana_p` / `clearallcd` / `addblockrate` / `walkrunshadow` / `manatoskill_enhance` | |
| | `+0x13a4` / `+0x13a8` / `+0x13ac` / `+0x13b0` / `+0x13b4` / `+0x13b8` | | `meleedamagereturn_p` / `_v`, `rangedamagereturn_p` / `_v`, `poisondamagereturn_p` / `_v` | |
| | `+0x13bc` / `+0x13c0` / `+0x13cc` / `+0x13d0` / `+0x13d4` | | `slowmissle_b` / `statusimmunity_b` / `damage2addmana_p` / `poison2decmana_p` / `manashield_p` | |
| | `+0x13dc` / `+0x13e0` / `+0x13e4` | | `steallife_p` / `stealmana_p` / `stealstamina_p` (ghi đè, không cộng; 136–138 `*enhance_p` cùng ô) | |
| | `+0x13e8` / `+0x13ec` / `+0x13f0` / `+0x13f4` | | `seriesres_p` / `seriesenhance_p` / `five_elements_enhance_v` / `five_elements_resist_v` | |
| | `+0x13f8` / `+0x13fc` / `+0x1400` / `+0x1404` / `+0x1408` / `+0x140c` / `+0x1410` / `+0x1414` (=100) / `+0x1418` | | `knockback_p` / `deadlystrike_p` / `stun_p` / `fatallystrikeres_p` / `block_rate` / `enhancehit_rate` / `enhancehiteffect_rate` / `add_damage_p` / `fatallystrike_p` | |
| | `+0x1420` / `+0x1424` / `+0x1428` | | `freezetimereduce_p` / `poisontimereduce_p` / `stuntimereduce_p` | |
| | `+0x142c` / `+0x1430` / `+0x1434` / `+0x1438` / `+0x143c` | | `fireenhance_p` / `coldenhance_p` / `poisonenhance_p` / `lightingenhance_p` / `addphysicsdamage_v` | |
| | `+0x1464` / `+0x1468` / `+0x146c` / `+0x1470` / `+0x1474` | | `dynamicmagicshield_v` / `staticmagicshield_v,_p` / `ignoreskill_p` / `returnskill_p` / `ignorenegativestate_p` | |
| | `+0x1478` / `+0x1479` / `+0x147a` / `+0x147b` (byte) | | `forbit_attack` / `frozen_action` / `forbit_takemedicine` / `invincibility` (`sete` sau `cmp nValue[0], 1`: cờ = 1 **khi giá trị đúng bằng 1**) | |
| | `+0x14c4` | | `randmove` 199 | |
| | `+0x1505` (32 byte) / `+0x1528` | | `m_szName` / độ dài tên | `TRoleData+0x4` / tên mẫu |
| | `+0x152c` | | `m_nSex` | `TRoleData+0x24` |
| | `+0x15cc..+0x15dc` / `+0x15e0..+0x15f0` | | `me2{metal,wood,water,fire,earth}damage_p` 276… / `{…}2medamage_p` 277… (5 hệ) | |
| `+0x15f4` / `+0x15f8` / `+0x15fc` / `+0x1600` / `+0x1604` | `+0x19ec` / `+0x19f4` / `+0x19fc` / `+0x1a04` / `+0x1a0c` | `+0x19f0` / `+0x19f8` / `+0x1a00` / `+0x1a08` / `+0x1a10` | `m_FireResist` / Cold / Poison / Light / Physics (người chơi từ `level_add`; mẫu `+0xec/+0xf0/+0xf8/+0xf4/+0xfc`) | |
| `+0x1628` / `+0x162c` | `+0x1a34` / `+0x1a3c` | `+0x1a38` / `+0x1a40` | `m_AttackSpeed` / `m_CastSpeed` (người chơi 0 / 0) | |
| `+0x163c` | `+0x1a44` | `+0x1a48` | `m_HitRecover` (người chơi 0; mẫu `+0xb8`) | |
| | `+0x1a24` / `+0x1a28` | | `sorbdamage_p` / `sorbdamage_yan_p` | |
| | `+0x1658` (10 int) / `+0x1680` / `+0x1720` | | `m_AiParam[10]` / bán kính kỹ năng lớn nhất² / `m_AiSkillRadiusLoadFlag` | mẫu `+0x70..` |
| | `+0x1750` | | tên kịch bản (`strcpy` từ mẫu `+0x103c`) | |
| | `+0x1904` | | `m_LoopFrames` | |
| | `+0x190c`, `+0x1910`, `+0x1914` (`m_HurtFrame`), `+0x1918`, `+0x191c`, `+0x1920`, `+0x1924`, `+0x1928`, `+0x192c` (`m_DeathFrame`) | | số khung | mẫu `+0x38, +0x40, +0x4c, +0x54, +0x58, +0x3c, +0x48, +0x44, +0xbc` |
| | `+0x19e8` | | cờ "đang tính lại trang bị/trạng thái" | |
| | `+0x1f0/+0x1f8`, `+0x200/+0x208` | | `m_LifeState`, `m_ManaState` (thuốc) | §9 |
| | `+0x234` | | đầu danh sách trang trạng thái: mỗi trang `+4` next, 20 `KMagicAttrib` từ `+0x24` | |

### 10.2 `KPlayer` (0x8788 byte)

`+0x3f4 m_nIndex`; `+0x3fc m_ItemList` (ô trang bị `+0xc + place·8`: `+0x408 + place·8` idx, `+0x40c`
cờ); `+0x5924 m_nAttributePoint`; `+0x5928 m_nSkillPoint`; `+0x5930 m_nStrength`, `+0x5934
m_nDexterity`, `+0x5938 m_nVitality`, `+0x593c m_nEngergy`, `+0x5940 m_nLucky`; `+0x5948..+0x5954
m_nCurStrength/Dexterity/Vitality/Engergy`, `+0x5958 m_nCurLucky`; `+0x595c/+0x5960 m_nExp` (64 bit),
`+0x5964/+0x5968 m_nNextLevelExp`; `+0x5994` danh sách kỹ năng người chơi; `+0x86b8` số lần **trùng
sinh** (byte `TRoleData+0x162`, ≤ 7) — đổi bảng kinh nghiệm và chặn dưới kháng; `+0x86a4/+0x86a8`
đếm thuốc (§9).

### 10.3 `KLevelAdd` (`0x8BAF378`, nạp ở `0x080C4FD0`)

`level_exp.txt` 200 dòng (cấp 1..200, dòng i+2): cột 2 → `this[i]` (1..2·10⁹, sai → 2·10⁹); cột 3 →
`this+0x320+4i` (≤ 2·10⁹); cột 4..10 (trùng sinh 1..7) → int64 `this + 0x640 + 4·0x640·(k−1)... ` đúng
hơn: `this + 8·(200·k + i)` = cột × 10 000. **`GetLevelExp(level, reborn)`** (`0x080C3FF0`) = `cột2[level]
+ (reborn == 0 ? 10 000 × cột3[level] : bảng64[reborn][level])`; ngoài 1..200 hoặc reborn > 7 → −1.
`level_add.txt` 5 dòng (hệ 0..4): cột 2 `LifePerLevel` (+0x3200), 3 `StaminaMalePerLevel` (+0x323c),
4 `StaminaFemalePerLevel` (+0x3250), 5 `ManaPerLevel` (+0x3264), 6 `LifePerVitality` (+0x3278), 7
`StaminaPerVitality` (+0x328c), 8 `ManaPerEnergy` (+0x32a0), 9 `LeadExpShare` (+0x32b4), 10
`FireResPerLevel` (+0x32c8), 11 Cold (+0x32dc), 12 Poison (+0x32f0), 13 Lighting (+0x3304), 14 Physics
(+0x3318), 15 `StaminaMaleBase` (+0x3214), 16 `StaminaFemaleBase` (+0x3228); mọi ô trống → 0.
**`GetXResist(series, level, reborn≠0)`** (`0x080C4220` hoả, `0x080C42A0` băng, `0x080C4320` độc,
`0x080C43A0` lôi, `0x080C4420` vật lý): series > 4 hoặc cấp ∉ 1..200 → 0; `n = level`, nhưng cấp > 120
và hệ số **âm** → `n = 120`; `res = XResPerLevel[series] × n / 100`; trùng sinh → `max(res,
[0x830CA08])`. **`GetStaminaBase(series, sex, level)`** (`0x080C4120`) = `(level − 1) ×
StaminaPerLevel[sex][series] + StaminaBase[sex][series]` (sex 0 = nam). `GetLifePerLevel`
`0x080C40A0`, `GetStaminaPerLevel(series, sex)` `0x080C40C0`, `GetManaPerLevel` `0x080C4180`.
`stamina.ini` → `KPlayerSet`: `NormalAdd` +0x14bc (1), `ExerciseRunSub` +0x14c0 (6), `FightRunSub`
+0x14c4 (6), `KillRunSub` +0x14c8 (6), `SitAdd` +0x14cc (3) — mặc định trong ngoặc khi thiếu tệp.

### 10.4 Các hàm (địa chỉ đã kiểm từng thân hàm)

| Địa chỉ | Là gì |
|---|---|
| `0x0807DBD0` | `KNpc::Init` (từ ctor `0x0807E5D0`): gốc máu/nội/thể/chính xác = 100, phòng thủ = 10 |
| `0x0807EE60` | **`KNpc::ClearAttrib(this, bClearState)`**: ô hiện tại = ô gốc (bảng 10.1), `+0x1194/+0x119c = 100`, `+0x1414 = 100`, xoá mọi ô ma pháp, `0x08078F10` xoá `anti_*`; xoá danh sách `+0x1150` (từng nút: `0x082248C0` rồi `vtbl[1]`); **79 ô kỹ năng**: `cấp hiện tại (+0x18) > cấp (+4)` → `0x080E4CA0(skilllist, i, cur, base)` rồi `cur = base`; người chơi: `Player+0x86f8/+0x86fc/+0x8700 = 0`; xoá 5 danh sách `+0x1854/+0x1878/+0x189c/+0x18c0/+0x1830` (`0x0808C140`) và `+0x18e8` (`0x0805F760`); `+0x15cc..+0x15f0 = 0`; `bClearState` → xoá `+0x1bc/+0x1cc/+0x1dc/+0x1ec/+0x1fc` (các trạng thái độc/băng/choáng/thuốc) |
| `0x08082680` | **`KNpc::Init/ResetCurData`** (sau `SetTemplate`, `LoadFrom`, Lua `0x08165750`, `0x08085E70`): `m_CurrentCamp = m_Camp`; người chơi: `0x081621B0(Player+0x8078, level, level)`; `0x080823B0(this, 1, 0, 0, 0)`; máu = máu max, nội = nội max, thể = thể max, chép mọi ô gốc → hiện tại như `ClearAttrib`, `0x08079030`; cuối: nhảy `0x08079C30` |
| `0x08082E20` | **`KNpc::SetTemplate(this, nTemplate, nLevelOff, nSeries)`**: mẫu = `[0x836EB00 + 4·(nTemplate·0xB4 + (nSeries < 5 ? nSeries+1 : 1)·0x1E + nLevelOff)]` (mỗi mẫu 6 hệ × 30 cấp con trỏ); `0x08078F10`; tên (`+0x1505`, 32 byte); ô theo bảng 10.1; kinh nghiệm × `ExpRate`; `m_AiParam[10]` + bán kính² lớn nhất của 4 kỹ năng (vtable `+0x48` = `GetAttackRadius`, `0x08BC99E0` kho kỹ năng, `0x080E6E10` nạp) — chỉ lần đầu (`+0x1720`); kết: gọi `0x08082680`, nhảy `0x080A1C20` |
| `0x080C16D0` | **`KPlayer::LoadFrom(this, TRoleData*, bFlag)`**: `0x080A8550` (xoá), `+0x86b8` trùng sinh, `+0x8600`; `0x080F6D20` map; `npc = g_PlayerSet.AddNpc`(`0x0809FB10`); kháng max từ `+0x164..` (0 → 75); `m_Kind = 1`; `0x08078BE0(npc, playerIdx)`; `m_Level` = WORD `+0xc3`; tên; 5 điểm gốc `+0xd7..+0xe7` → cả gốc lẫn hiện tại; **`SetNpcPhysicsDamage 0x080A7EC0`**, **`SetNpcAttackRating 0x080A7F90`**, **`SetNpcDefence 0x080A7FC0`**; kinh nghiệm 64 bit `= +0xc7 + WORD +0xc5 × 2·10⁹`; `m_nNextLevelExp = GetLevelExp`; `m_Series` `+0xbb`, `m_Camp` `+0xbf`, `m_nSex` `+0x24`, **`m_LifeMax = TRoleData+0xeb`**, `m_StaminaMax = GetStaminaBase`, `m_LifeReplenish = m_ManaReplenish = 0`, `m_ManaMax = +0xf3`, `m_StaminaGain = NormalAdd`; **`SetNpcResist 0x080AB7C0`** (5 kháng gốc từ `level_add`); `0x080A7FF0` (đi 5, chạy 10, tốc đánh/thi triển 0, tầm nhìn 120, hồi đòn 0); `0x08078F10`; **`0x08082680`**; máu/nội/thể hiện tại từ `+0xf7/+0xff/+0xfb`; … |
| `0x080A7EC0` | `KPlayer::SetNpcPhysicsDamage` (gốc): `m_PhysicsDamage = {min = max = m_nCurStrength / 5 + 1, nValue[1] = 0}`, xoá hoả/băng/lôi/độc |
| `0x080A7F90` / `0x080A7FC0` | `SetNpcAttackRating`: `m_AttackRating = m_nDexterity × 4 − 28`; `SetNpcDefence`: `m_Defend = m_nDexterity / 4` |
| `0x080AF740` | **`KPlayer::SetNpcPhysicsDamage` (hiện tại)**: `GetWeaponDamage 0x081F9310` (`min = (Item+0x88 + Σ weapondamagemin_v) × (100 + Σ weapondamageenhance_p) / 100`, max với `+0x98`; tay không: `m_nCurStrength / 5 + 1`); `GetWeaponType 0x081F92E0` (= `Item+0x8` detail, −1 khi trống): **0 (cận chiến) → cả hai `+= m_nCurStrength / 5`; 1 (xa) → `+= m_nCurDexterity / 5`**; khác → giữ nguyên; `KNpc::SetPhysicsDamage 0x08078E20` ghi `+0x11bc/+0x11c4` |
| `0x080B0B40` / `0x080B0AF0` | `ChangeCurStrength(n)`: `+0x5948 += n` rồi hàm trên; `ChangeCurDexterity(n)`: `+0x594c += n`, **`m_CurrentAttackRating += 4n`, `m_CurrentDefend += n/4`**, rồi hàm trên (ProcessFunc 97/98 `strength_v`/`dexterity_v` gọi hai hàm này, chỉ người chơi) |
| `0x080B0B60` | `AddBaseDexterity(n, bCheck)`: `bCheck && n > điểm` → không; gốc và hiện tại `+= n`; âm → lỗi; điểm `−= n`; `0x080AF700`/`0x080AF6C0` (chính xác/phòng thủ gốc), `UpdataCurData(0)`, `SetNpcPhysicsDamage`; gói s2c **0x5D** {attr 1, gốc, hiện tại, điểm} 14 byte |
| `0x080AF250` / `0x080AF1A0` / `0x080AF120` | `LevelAddBaseLifeMax(sign)`: `m_LifeMax += LifePerLevel(series) × sign`, cả hai max hiện tại = gốc; `LevelAddBaseStaminaMax(sign)`: `+= StaminaPerLevel(series, sex) × sign`, max hiện tại = gốc, tính lại `+0x11b0`; `LevelAddBaseManaMax(sign)` tương tự |
| `0x080AF800` | **`KPlayer::LevelUp(this, bUp)`**: `m_nExp = 0`; lên: cấp ≤ 199 → `++`, **điểm thuộc tính += 5, điểm kỹ năng += 1**; xuống (`bUp = 0`): cấp > 1 → `−−`, `−5`, `−1`; `m_nNextLevelExp = GetLevelExp(cấp mới)`; ba hàm trên với dấu; 5 kháng gốc = `GetXResist` và hiện tại = gốc, 5 kháng max hiện tại = gốc; giữ `m_CurrentCamp`, **`UpdataCurData(1)`**, `SetNpcPhysicsDamage`, trả `m_CurrentCamp`; **máu = max(+0x1a14,+0x1a18), thể = max thể, nội = max(+0x1a1c,+0x1a20)**; `0x080A86B0` (đồng bộ), `0x081E7FD0`; báo đội (7 ô `0x08BB86EC`); bang; lên cấp → `\script\global\server_playerlevelup.lua` `main(player, level)`; trùng sinh → `0x080E4A00(skilllist)` |
| `0x080AF550` | **`KPlayer::UpdataCurData(this, bClearState)`**: `ClearAttrib(npc, bClearState)`; 5 điểm hiện tại = gốc; `Player+0xc8/+0xcc/+0xd0/+0xd4 = 0`; `KNpc::ReCalcStateEffect 0x0807D270` (duyệt trang trạng thái `+0x234`, mỗi ô ≠ 0 → `ModifyAttrib(npc, idx, {type, −v0, −v1, −v2}, 0)`); **`ReCalcEquip 0x080AF3E0`** (`+0x19e8 = 1`; `0x081FD1B0`; 15 ô trang bị có đồ: `KItem::ApplyBaseAttrib 0x080669B0`, `nActive = GetEquipEnhance(place, 0)`, `ApplyMagicAttrib 0x08066890`, `0x080684A0`, `0x08068280`; `+0x19e8 = 0`); `0x080AF390` (`0x081E5F20(0x97AC300, player)`); `0x081D5800(Player+0x8704)`; đuôi `0x080CBE40(Player+0x5994, idx)` (rỗng) |
| `0x0807F780` | `KNpc::OnHurt(this, nAntiHitRecover)`: `m_RegionIndex ≥ 0`, `m_Doing ∉ {9, 10}`; log `"m_CurrentHitRecover:%d - AntiHitRecover:%d = %d"`; `hr = max(+0x1a44, +0x1a48) − anti`; `hr > 99` → không bị đánh; `g_Random(100) ≤ 49` → không; `ignorenegativestate_p` `+0x1474` > `g_Random(100)` → log `"IgnoreNegState(Hurt):%d%%, Hit!"` và không; `m_Doing = 9`, `+0x194c = 0`, `+0x230 = 0`, **`+0x22c = max(1, (100 − hr) × m_HurtFrame(+0x1914) / 100)`**; báo vùng `0x080EF710`, gói `0x0807A970(this, 9, …)` |

### 10.5 Kinh nghiệm (đã kiểm từng thân hàm)

| Địa chỉ | Là gì |
|---|---|
| `0x0809BC70` | **`KDamageRecord::Add(rec, playerIdx, damage)`**: `rec = KNpc+0x1724` (3 ô × 12 byte từ `+4`: `+0` playerIdx, `+4` sát thương, `+8` = 0x4B0 = 1200 khung); chỉ khi npc **không** phải người chơi (`+0x24 == 0`?? — `m_Kind` của npc bị đánh ≠ 1) và playerIdx hợp lệ; người chơi có đội (`Player+0x5994`) → ghi theo **đội trưởng** (`0x08BB86E8 + team·0x30`); ô của người đó có sẵn → cộng, không thì ô trống đầu, hết ô → bỏ. `0x0809BD80` xoá 3 ô; `0x0809BDA0` đặt lại |
| `0x0809BDD0` | **chia kinh nghiệm khi chết** (gọi từ `KNpc::DoDeath 0x080892E0` sau `OnDeath`): mỗi ô có sát thương: người chơi có đội → chọn thành viên **gần nhất** trong 0x100000 = 1024² đơn vị; `exp = (int)((double)m_Experience(+0x15a8) × sátThương / max(+0x1a14, +0x1a18))` → `KPlayer::AddExpTeam 0x080B03E0(player, exp, cấp npc)`; trả về ô sát thương lớn nhất (chủ đồ rơi). Đòn kết liễu ghi **nguyên** sát thương nên tổng có thể vượt 100 % |
| `0x080B03E0` | `KPlayer::AddExpTeam(this, exp, npcLevel, …)`: không đội → `AddExp`; có đội → đếm thành viên cùng map trong 1024 đơn vị, thưởng `√n × hệ số (float 0x825528C)` + `100 + n`… (làm ở lát đội, M13/M14) |
| `0x080B00C0` | **`KPlayer::AddExp(this, exp, npcLevel)`**: `exp ≤ 0`, `m_Doing ∈ {10, 0x15}`, `KNpc+0x168c == 0` → không; `exp = CalcExp(exp, cấp mình, cấp npc)`; `> 5 000 000` → `exp/100 × (100 + Player+0xd4)` không thì `(100 + +0xd4) × exp / 100`; `0x080A9B50` (kinh nghiệm thống lĩnh); đồng hành (`+0x8088`) nhận phần riêng qua `0x080A7D30`; sự kiện `[0x9781158]` + `+0x8650/+0x8654` → chia 5; `0x080AF640(exp, +0xd0, &+0xc8)` = `exp × (100 + p) / 100 + lo + g_Random(hi − lo)` (trên 999 999 chia 100 trước); cờ map `SubWorld+0x63f8c & 4` không có → `0x08176160(+0x7d00)` 1 → chia đôi, 2 → 0; rồi lõi `0x080AFEA0` |
| `0x080A7C80` | **`CalcExp(exp, cấp mình, cấp npc)`**: cấp mình ≥ 100: npc ≤ 89 → 1, không thì exp; `d = mình − npc`: `d < −54`: `d < −69` → exp, không thì `exp × (−19d − 1030) / 300` (−55 → 5 %, −69 → 93 %); `|d| ≤ 5` → exp; `≤ 15` → `exp × (25 − |d|) / 20`; hơn → `exp / 2`; kết quả ≥ 1 |
| `0x080AFEA0` | **lõi `AddExp(this, int64)`**: cấp 200 (`> 0xC7`) không cộng; `exp = min(exp + n, m_nNextLevelExp)`; `≥ next` → `LevelUp(1)` (đặt exp = 0 — phần dư **mất**, như nguồn cũ), không thì gói s2c **0xC6** {int64 exp} |
| `0x08099F40` / `0x08099F00` / `0x08099EC0` | ProcessFunc 175 `expenhance_v` → `Player+0xc8 += v0, +0xcc += v2`; 176 `expenhance_p` → `+0xd0 += v0`; 206 `add120skillexpenhance_p` → `+0xd4 += v0` (chỉ người chơi; `UpdataCurData` xoá cả bốn) |
| `0x08088B60` | `KNpc::OnDeath` phần người chơi chết: mất kinh nghiệm `GetLevelExp(cấp)/100·2` … × `(7 − PK)/7`, trần 0x1FBD0; quái chết → **không cộng kinh nghiệm trong C++** mà gọi script toàn cục `OnGlobalNpcDeath(npc, kẻ giết)` (`\script\activitysys\g_npcdeath.lua`: nhiệm vụ/sự kiện) — kinh nghiệm đi đường `DoDeath → 0x0809BDD0` ở trên |
| `0x0809A100` … | `strength_v`/`dexterity_v`/`vitality_v`/`energy_v` (97–100) → `KPlayer::ChangeCurStrength 0x080B0B40` (`+0x5948 += n`, `SetNpcPhysicsDamage`), `ChangeCurDexterity 0x080B0AF0` (`+0x594c += n`, `m_CurrentAttackRating += 4n`, `m_CurrentDefend += n/4`, `SetNpcPhysicsDamage`), `ChangeCurVitality 0x080B0E60` (`+0x5950 += n`, `KNpc::AddCurLifeMax 0x08078C30` = cả `+0x1a14` lẫn `+0x1a18` `+= LifePerVitality × n`, `AddCurStaminaMax 0x08078CA0` `+= StaminaPerVitality × n` + tính lại `+0x11b0`), `ChangeCurEnergy 0x080B0DF0` (`+0x5954 += n`, `AddCurManaMax 0x08078D10` cả hai ô `+= ManaPerEnergy × n`) |

Trạng thái `KNpc` lúc bị đánh/chết/ngồi… và công thức sát thương (`KNpc::Attack`, `ReceiveDamage`,
`CalcDamage`) → lát tiếp của M12.
