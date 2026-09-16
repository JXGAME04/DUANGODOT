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
