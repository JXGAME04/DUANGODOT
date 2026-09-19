# Script Lua của JX NEXT

Server cũ chạy **Lua 4.0** (bản nhúng trong `Sources/Library/LuaLib`). JX NEXT chạy **Lua 5.4
chuẩn**, và **không có lớp tương thích Lua 4 nào cả**: chính các file script đã được chuyển sang
Lua 5.4 một lần bằng `services/pkg/jxlua`. Mở một file trong `data/script/` ra, bạn đọc được đúng
thứ máy chủ chạy, và thư viện chuẩn hành xử đúng như tài liệu Lua 5.4.

Quyết định: [ADR-006](adr/ADR-006-lua54-khong-tuong-thich-lua4.md).

## 1. Chạy

```bash
python tools/dev.py lua
```

Lệnh này build `jxlua`, đọc từng thư mục tham chiếu trong `config/oldgame.local.json`
(`server` rồi `server_fallback`), ghi bản đã chuyển vào `data/script/<tên>/script/…`, rồi bắt
`jx_luacheck` nạp **toàn bộ** file bằng chính Lua 5.4 mà zone nhúng. `data/` nằm trong
`.gitignore` — script đã chuyển là dữ liệu sinh ra, như asset.

`dev.py start` tự trỏ `JX_ZONE__SCRIPT_ROOT` vào các thư mục đó, đúng thứ tự ưu tiên cũ, nên
`KScriptCache` vẫn tìm ở thư mục thứ hai khi thư mục thứ nhất thiếu file (server Linux không có
trap script theo từng map, chúng nằm ở `bin/Server`).

Chạy riêng từng bước:

```bash
build/go/jxlua convert -in D:\ServerLinux\server1\script -out data\script\server1\script -report r.json
build/go/jxlua check   -in data\script\server1\script
build/windows-msvc/bin/Release/jx_luacheck data/script/server1/script data/script/binserver/script
```

`check` chạy y hệt `convert` nhưng không ghi gì — chạy trên cây **đã chuyển** phải ra **0 thay
đổi**. Đó là phép thử tự kiểm: chuyển hai lần cho ra cùng một kết quả.

## 2. Kết quả đo được

| | server1 (Linux) | bin/Server | cộng |
|---|---:|---:|---:|
| file `.lua` | 5 059 | 2 604 | **7 663** |
| lượt sửa | 31 204 | 3 214 | **34 418** |
| file không nạp được bằng Lua 5.4 | 0 | 0 | **0** |

Dung lượng 22,6 MB. Sau khi chuyển, zone nạp 555 script lúc khởi động, không một lỗi nào.

## 3. Những gì được sửa, và vì sao

Bộ chuyển **không đụng vào byte nội dung**: cây script trộn cả tiếng Trung GBK lẫn tiếng Việt
TCVN3 trong cùng một file (đo được 480 364 đoạn cao 1 byte bên cạnh 33 673 đoạn dài từ 9 byte), nên
mọi phép chuyển mã đều làm hỏng một trong hai. Cú pháp Lua thì thuần ASCII, và bộ chuyển chỉ sửa
ASCII **ngoài chuỗi và chú thích**: nó dựng một bản sao "che" chuỗi/chú thích bằng byte trống nhưng
giữ nguyên độ dài, dò trên bản che, ghép trên bản thật.

| Sửa | Số lượt | Vì sao |
|---|---:|---|
| `floor/ceil/abs/sqrt/min/max/random/sin/cos/exp` → `math.*` | 9 037 | Lua 5 dời vào thư viện |
| escape lạ bỏ dấu `\` | 5 457 | Lua 4 `\d` = `d` (llex.c:267), Lua 5.4 báo lỗi |
| `format/strfind/strsub/strlen/gsub/strbyte` → `string.*` | 5 234 | như trên |
| `getn(t)` → `#(t)` | 4 560 | `getn` không còn |
| `%upvalue` → `upvalue` | 4 044 | Lua 5 bắt biến ngoài tự động |
| `tinsert/tremove/sort/unpack` → `table.*` | 3 392 | như trên |
| `date/clock/time` → `os.*` | 849 | như trên |
| `for k,v in t` → `for k,v in pairs(t)` | 741 | Lua 5 cần hàm lặp |
| `mod(a,b)` → `math.fmod(a,b)` | 696 | `mod` của Lua 4 là `fmod` của C, **không** phải `%` của Lua 5 (khác dấu với số âm) |
| `function f(...)` dùng `arg` → thêm `local arg = table.pack(...)` | 106 | Lua 4 tự cho bảng `arg` |
| `write/read/openfile` → `io.*`, `closefile(f)` → `(f):close()` | 174 | như trên |
| `call(f, t)` → `f(table.unpack(t))` | 62 | `call` không còn |
| `dostring(s)` → `load(s)()` | 22 | như trên |
| `getglobal/setglobal` → `_G[...]` / `rawset(_G, …)` | 23 | như trên |
| khối `/* */` → chú thích từng dòng | 14 | xem mục 4 |
| `pow(a,b)` → `((a)^(b))` | 2 | `math.pow` bị bỏ ở 5.4 |
| `in` làm tên biến → `_in` | 2 | xem mục 4 |
| `1and` → `1 and` | 1 | xem mục 4 |
| `{a,; b}` → `{a, b}` | 1 | xem mục 4 |
| `--[[` → `-- [[` | 1 | xem mục 4 |

## 4. Năm điểm khác biệt chỉ đọc mã nguồn Lua 4 cũ mới biết

Cả năm điểm dưới đây đều **kiểm chứng trong `Sources/Library/LuaLib/src`**, không suy đoán.

1. **Lua 4 không có chú thích dài.** `llex.c:297-301`: gặp `--` là chạy thẳng tới cuối dòng, bất kể
   phía sau là gì. Nghĩa là `--[[` chỉ mở một chú thích **một dòng**, và các dòng bên dưới nó là mã
   sống. Lua 5.4 thì nuốt tới `]]` gần nhất. Bộ chuyển tách `--[[` thành `-- [[`, và khi dò nó
   cũng dùng đúng luật Lua 4, nếu không thì mã dưới `--[[` sẽ bị bỏ sót.
2. **Lua 4 không có toán tử `%`.** Chia dư là hàm `mod()`. Nên trong mã Lua 4, mọi `%` đứng trước
   một tên đều là dấu upvalue — kể cả sau `)` như `function(nItemIdx) %tbLog:Write(…)`. Hình dạng
   duy nhất có thể là chia dư về sau là `a%b` dính liền: bộ chuyển giữ nguyên và ghi chú lại.
3. **`in` không phải từ khoá.** Danh sách từ dành riêng ở `llex.c:31-34` không có `in`; trình phân
   tích nhận ra vòng `for` tổng quát bằng cách so tên với chuỗi `"in"` (`lparser.c:875`). Vì vậy
   `global/jingli.lua` đặt tên tham số là `in` và vẫn chạy. Lua 5.4 cấm, nên nó thành `_in`.
   `true`, `false`, `goto` cũng không phải từ khoá Lua 4 — đã rà, không file nào dùng làm tên.
4. **Escape lạ bị bỏ dấu `\`.** `llex.c:267` — nhánh mặc định lưu chính ký tự đó. Nên
   `"\\script\\\dailogsys\\\dailogsay.lua"` cho ra `\script\dailogsys\dailogsay.lua`. Lua 5.4 từ
   chối biên dịch `\d`. Bộ chuyển bỏ đúng dấu `\` mà Lua 4 đã bỏ, nên **chuỗi sinh ra giống hệt
   từng byte** so với server cũ. `\x`, `\z`, `\u` cũng vậy: Lua 4 đọc là chữ cái thường.
5. **Bảng Lua 4 là `{ phần mảng ; phần khoá }`.** `{182, 630,; }` hợp lệ. Ở Lua 5 hai phần nhập
   làm một, `,` và `;` là hai dấu ngăn như nhau, nên `,;` là một ô rỗng và là lỗi cú pháp.

Ngoài ra: Lua 4 đọc số dừng ở ký tự đầu tiên không nối tiếp được, nên `1and` là số `1` rồi từ
`and`; Lua 5.4 đọc `1a` và báo "malformed number". Và chuỗi dài `[[ ]]` của Lua 4 **lồng nhau
được** (`llex.c:200-210`) còn của Lua 5.4 thì không — bộ chuyển báo khi gặp, cây tham chiếu không
có trường hợp nào.

## 5. Chín file chưa từng nạp được

Chín file mở khối `/* */` kiểu C. Trình phân tích từ vựng Lua 4 **không có nhánh nào cho `/`**
(`llex.c:297-360`), nên chúng chưa bao giờ biên dịch được — kể cả trên server cũ, kể cả
`lib/basic.lua`, `lib/string.lua`, `lib/say.lua`. Ý định của tác giả rõ ràng là tắt khối đó, nên bộ
chuyển thêm `--` vào đầu từng dòng của khối: khối vẫn tắt, phần còn lại của file cuối cùng mới
chạy được. Chỉ làm vậy khi `/*` đứng đầu dòng; nằm giữa dòng thì để nguyên và ghi chú.

Danh sách: `item/card/card_shitu.lua`, `item/hecheng/{amulet,pendant,ring,shoushihecheng}.lua`,
`lib/{basic,say,string}.lua`, `shitu/shitu.lua`.

## 6. Hàm engine mà script gọi

Sau khi bỏ lớp tương thích, `KLuaScript::init` chỉ đăng ký ba hàm của engine:

| Hàm | Làm gì |
|---|---|
| `Include("\\script\\…")` | nạp file vào cùng một state (`KLuaScript::include`) |
| `IncludeLib(name)` | thư viện C của engine cũ; đã nằm sẵn trong zone, nên không làm gì |
| `print(...)` | ghi vào log có cấu trúc, category `lua`, kèm tên file |

Cộng thêm các hàm gameplay trong `ScriptFuns.cpp` (mỗi hàm chép từ `jx_linux_y`, đọc tới `ret`;
địa chỉ ghi trong chú thích và `docs/LINUX-SERVER.md`).

**Bao nhiêu hàm còn thiếu để chạy trọn bộ script Linux** — xem `docs/SCRIPT-API.md`, sinh bởi

```bash
python tools/script_api_coverage.py --skip PARTNER_ --out docs/SCRIPT-API.md
```

Công cụ đối chiếu ba danh sách: hàm C mà `jx_linux_y` đăng ký cho Lua (`tools/re/jx_linux_y.luamap.txt`,
1577 hàm + 174 hàm thư viện Lua 4 tĩnh ở địa chỉ ≥ 0x08230000), hàm zone đăng ký (`ScriptFuns.cpp` +
`KLuaScript.cpp`), và mọi lời gọi hàm toàn cục `Tên(` trong `data/script`. Kết quả 2026-09-19: zone đã có 161
tên (63 806 lượt gọi), **còn thiếu 866 tên (29 056 lượt)** — làm từ trên xuống theo số lượt gọi (quyết định
chủ dự án 2026-09-19: ưu tiên việc này, bỏ qua PARTNER_* — 22 tên, 436 lượt). 155 tên script gọi mà nhị phân
không có (`OB_SaveShareData`, `GetSysCurrentTime`, `WriteStringToFile`, `GlobalExecute`…: script gốc cũng lỗi ở đó).

## 7. Kiểm thử

| Nơi | Test |
|---|---|
| Go | `services/pkg/jxlua/KLuaConvert_test.go` — 39 test, mỗi phép biến đổi một test, dùng đúng dòng thật của script gốc làm ví dụ |
| C++ | `server/zone/tests/test_KLuaScript.cpp` — script đã chuyển chạy được; script Lua 4 **bị từ chối**; `getn`/`strfind`/`floor` không còn là biến toàn cục |
| Cây thật | `jx_luacheck` nạp cả 7 663 file bằng Lua 5.4 |
| Bất biến | `jxlua check` trên cây đã chuyển ra 0 thay đổi |
