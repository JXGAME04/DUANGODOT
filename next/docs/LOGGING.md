# Chuẩn log JX NEXT

Mọi tiến trình của JX NEXT (zone core C++, các service Go, client Godot, tool) ghi log theo **cùng
một định dạng**: mỗi sự kiện là **một dòng JSON**. Lý do: gom log từ nhiều tiến trình trên nhiều
máy về một chỗ, lọc bằng `jq`/Loki/Grafana, và khi replay lỗi có thể đối chiếu client ↔ gateway ↔
zone theo `sid`/`pid`/`tick` mà không phải đoán.

## 1. Định dạng dòng

```json
{"ts":"2026-09-16T08:00:00.123456Z","lvl":"info","cat":"net","proc":"zone","sid":42,"pid":7,"zone":3,"tick":99,"tid":4132,"msg":"connected","addr":"127.0.0.1","port":"15622"}
```

| Trường | Kiểu | Bắt buộc | Ý nghĩa |
|---|---|---|---|
| `ts`   | string | ✔ | UTC ISO-8601, micro giây, hậu tố `Z`. Luôn UTC — không dùng giờ máy. |
| `lvl`  | string | ✔ | `trace` `debug` `info` `warn` `error` `fatal` |
| `cat`  | string | ✔ | Category dạng chấm: `net`, `net.recv`, `zone.tick`, `zone.ai`, `db`, `lua`, `auth`… |
| `proc` | string | ✔ | Tên tiến trình: `zone`, `gateway`, `auth`, `client`, `tool.pakx`… |
| `sid`  | number | khi có | Session id do gateway cấp (một kết nối client) |
| `pid`  | number | khi có | Player id (nhân vật, id trong DB) |
| `zone` | number | khi có | Zone id |
| `tick` | number | khi có | Tick mô phỏng của zone tại thời điểm ghi |
| `tid`  | number | – | Thread id rút gọn (chỉ để debug đa luồng) |
| `msg`  | string | ✔ | Câu ngắn, **tiếng Anh, không dấu, cố định** (không nhét biến vào msg) |
| khác   | string | – | Các field bổ sung `key=value`; luôn là chuỗi để mọi ngôn ngữ giống nhau |

Thứ tự trường cố định như trên (C++ dùng `ordered_json`) để đọc raw bằng mắt vẫn dễ.

## 1b. Màn hình console — cho người, không phải cho máy

Dòng JSON ở trên đi vào **tệp log** (`log.file`). Còn **console** của `jx_zone` và gateway in cho
người đang ngồi trước máy: mỗi sự kiện một câu, tiếng Việt, mức log có màu.

```text
14:32:05.118 THÔNG TIN    [khởi động]   jx_zone bắt đầu khởi động · phiên bản=0.4.0 · tệp cấu hình=config/zone.json
14:32:05.119 THÔNG TIN    [cấu hình]    Thiết lập · khoá=zone.port · giá trị=19001
14:32:05.640 THÔNG TIN    [bản đồ]      Đã nạp map · map=1 · tên=Phượng Tường · số ô=262144 · nạp (ms)=212
14:32:05.702 THÔNG TIN    [khởi động]   Zone đã mở cổng, chờ gateway kết nối · cổng=19001
14:32:07.330 THÔNG TIN    [tài khoản]   Đăng nhập thành công · tài khoản=test1 · mã tài khoản=12 · phiên=562949953421313
14:32:09.004 CẢNH BÁO     [tài khoản]   Sai mật khẩu · tài khoản=test1 · số lần thử=2
```

Cách hoạt động: mã nguồn vẫn ghi `msg` tiếng Anh cố định (để tệp log và công cụ không đổi); console
tra câu đó, tên category và tên trường trong **`config/log.vi.json`**. Câu nào bảng chưa có thì in
nguyên tiếng Anh — không bao giờ mất dòng. C++ (`jx::log`) và Go (`pkg/log`) dùng chung một bảng và
một định dạng, nên hai cửa sổ đặt cạnh nhau đọc như nhau.

| Khoá cấu hình | Mặc định | Nghĩa |
|---|---|---|
| `log.console` | `true` | có in ra console hay không |
| `log.console_style` | `"text"` | `"text"`: câu cho người đọc; `"json"`: đúng dòng JSON của tệp (khi nối console vào công cụ) |
| `log.lang` | `"vi"` | `"vi"` đọc `config/log.vi.json`; `"en"` in như trong mã nguồn |
| `log.catalog` | tự tìm | đường dẫn bảng dịch; bỏ trống thì tìm `config/log.<lang>.json` từ thư mục chạy trở lên |

Lúc khởi động, mỗi thiết lập đang dùng được in **một dòng** (`[cấu hình] Thiết lập · khoá=… · giá trị=…`),
sau khi đã gộp tệp cấu hình, biến môi trường `JX_*` và tham số dòng lệnh — nhìn là biết server đang
chạy với số nào. Khoá có chữ `password`, `secret`, `token` thì giá trị hiện `***`.

Thêm một dòng log mới thì thêm câu tiếng Việt của nó: `python tools/check_log_catalog.py` liệt kê
câu và category còn thiếu (thoát mã 1), CI chạy lệnh này. Tên trường thiếu chỉ được nhắc, không chặn.

Trên Windows, tiến trình tự đặt console sang UTF‑8 (`SetConsoleOutputCP(65001)`) và bật màu ANSI;
khi console bị chuyển hướng vào tệp thì không chèn mã màu.

## 2. Mức log — dùng khi nào

| Mức | Dùng cho | Bật ở production? |
|---|---|---|
| `trace` | từng packet, từng bước pathfinding | không |
| `debug` | trạng thái nội bộ theo tick, giá trị formula | không (bật theo category khi cần) |
| `info`  | sự kiện vòng đời: start/stop, login, vào map, nạp data | có |
| `warn`  | bất thường tự phục hồi: packet sai, timeout, thiếu file thay bằng mặc định | có |
| `error` | thao tác thất bại, người chơi bị ảnh hưởng | có |
| `fatal` | tiến trình không thể tiếp tục; ghi xong sẽ **dump ring buffer** rồi thoát | có |

## 3. Quy tắc viết log

1. **Không `printf`/`std::cout`/`print()`** trong code sản phẩm. Chỉ dùng `jx::log`.
2. `msg` là hằng số; biến đi vào field: `info("net","connected",{kv("addr",a),kv("port",p)})` — không
   viết `"connected to " + addr`. (Lọc/gom nhóm theo `msg` mới được.)
3. **Không log bí mật**: mật khẩu, token, khoá session, số thẻ. Che bằng `***` nếu cần.
4. Category đặt theo module, tối đa 3 cấp: `zone.ai.path`. Category mới → thêm vào bảng ở cuối file.
5. Mọi thao tác trên một người chơi phải chạy trong `ScopedContext` (sid/pid/zone/tick) — để không phải
   tự nhét `pid` vào từng dòng.
6. Log lỗi phải có **cách tái hiện**: ít nhất `pid` + `tick` + tham số vào.
7. Một sự kiện = một dòng. Không log nhiều dòng cho một việc, không log trong vòng lặp nóng ở `info`.

## 4. Cấu hình mức log lúc chạy

Thứ tự ưu tiên: file cấu hình < biến môi trường < dòng lệnh.

```text
zone.json:        { "log": { "level": "info", "levels": "net=warn,zone.tick=debug", "file": "logs/zone.log" } }
biến môi trường:  JX_LOG__LEVEL=debug   JX_LOG__LEVELS="net=trace,zone.ai=debug,=warn"
dòng lệnh:        --set log.level=trace
```

Cú pháp `levels`: danh sách `category=level` cách nhau bằng dấu phẩy; `=warn` (không có category)
đặt mức mặc định. Mức có hiệu lực của một category là mức của **tiền tố dài nhất** đã cấu hình
(`zone.tick.ai` → `zone.tick` → `zone` → mặc định).

## 5. Ring buffer và crash dump

`jx::log` giữ 10 000 dòng gần nhất trong RAM (chỉ những dòng đã qua bộ lọc mức, để `trace` tắt
không tốn CPU). Khi gọi `fatal()` hoặc khi handler crash gọi `dump_ring()`, toàn bộ ring được ghi ra
`<file>.crash.log` cạnh file log. Đây là thứ đầu tiên cần đính kèm khi báo lỗi.

## 6. API theo ngôn ngữ

### C++ (`jx/log.hpp`)

```cpp
jx::log::Options o;
o.process = "zone";
o.file = "logs/zone.log";
jx::log::init(o);
jx::log::set_levels(cfg.get_string("log.levels", ""));

jx::log::info("boot", "listening", {jx::log::kv("port", port)});
{
    jx::log::ScopedContext ctx({.sid = sid, .pid = pid, .zone = zone, .tick = tick});
    jx::log::debug("zone.tick", "npc respawn", {jx::log::kv("npc", npc_id)});
}
jx::log::fatal("db", "cannot open", {jx::log::kv("path", p)});   // dump ring + flush
```

### Go (`pkg/log`, tuần 1)

`log.Info(ctx, "net", "connected", log.KV("addr", addr))` — `ctx` mang `sid/pid` qua `context.Context`;
xuất JSON bằng `log/slog` với handler tuỳ biến cùng thứ tự trường.

### Godot (`autoload Log`, tuần 1)

`Log.info("ui", "login pressed", {"server": name})` — ghi `user://logs/client.log` và console; cùng
schema, `proc = "client"`.

## 7. Bảng category

| Category | Tiến trình | Nội dung |
|---|---|---|
| `boot` | tất cả | khởi động, nạp cấu hình, dừng |
| `cfg` | tất cả | giá trị cấu hình có hiệu lực |
| `net`, `net.recv`, `net.send` | gateway, zone, client | kết nối, packet |
| `auth` | gateway, auth | đăng nhập, token |
| `zone.tick` | zone | vòng tick, tick trễ, tick bị bỏ |
| `zone.ai`, `zone.ai.path` | zone | AI, tìm đường |
| `zone.combat` | zone | công thức sát thương, kỹ năng |
| `lua` | zone | nạp script, lỗi script |
| `db` | persist, zone | truy vấn, lỗi DB |
| `asset` | client, tool | nạp tài nguyên |
| `test` | test | chỉ dùng trong test |
