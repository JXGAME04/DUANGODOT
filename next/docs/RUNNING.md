# Chạy hệ thống test JX NEXT (mốc M1)

Ba tiến trình: **jx_zone** (C++, mô phỏng), **gateway** (Go, phiên/auth/persist), **client** (Godot)
— cộng **jxbot** (Go) để test tự động. Tất cả chạy trên một máy Windows hoặc Linux.

```text
Godot client ──TCP 17100──> gateway (Go) ──TCP 17001──> jx_zone (C++)
jxbot ×N     ──TCP 17100──>                              data/gateway/*.json (persist dev)
```

## 1. Yêu cầu

| Thứ | Phiên bản | Ghi chú |
|---|---|---|
| Visual Studio 2022 (Windows) hoặc GCC 13+ (Linux) | | C++20 |
| CMake ≥ 3.25 + Ninja | có sẵn trong VS | |
| vcpkg (git checkout) | commit trong `vcpkg.json` | `VCPKG_ROOT=C:\vcpkg` |
| Go | ≥ 1.25 | `go env -w GOPATH=D:\...` nếu ổ C đầy |
| Godot | 4.7 (không cần bản .NET) | `winget install GodotEngine.GodotEngine` |
| Python | 3.10+ | chỉ dùng stdlib (tools/) |

Trên Windows mở **x64 Native Tools Command Prompt for VS 2022** rồi `set VCPKG_ROOT=C:\vcpkg`.

## 2. Build mọi thứ

```bash
cd next
cmake --preset windows-msvc            # lần đầu: vcpkg build fmt/spdlog/protobuf/asio (~5-10 phút)
python tools/dev.py build              # sinh proto Go, build C++ Debug, build gateway.exe + jxbot.exe
```

Sinh lại mã protocol sau khi sửa `proto/jx/*.proto`: `python tools/gen_proto.py` (Go + GDScript;
C++ tự sinh khi build).

## 2b. Xuất map và sprite từ client cũ (một lần, ~10 giây)

```bash
python tools/dev.py assets             # map 1 (Phượng Tường) + 267 sprite -> client/assets (75 MB, không commit)
```

Cần thư mục client cũ có `package.ini` + `Data/*.pak` (mặc định `../bin/Client`, hoặc `JX_OLD_CLIENT=...`).
Zone đọc `client/assets/maps/1` theo `zone.map_dir` trong `config/zone.json`; chi tiết ở [MAPS.md](MAPS.md).
`dev.py assets` cũng xuất sprite nhân vật/NPC vào `client/assets/npcres` (xem [NPCRES.md](NPCRES.md)); thiếu thư mục
này client vẫn chạy nhưng vẽ nhân vật bằng vòng tròn.

## 3. Chạy

```bash
python tools/dev.py start              # mở 2 cửa sổ console: jx_zone và gateway
python tools/dev.py client             # mở Godot client
python tools/dev.py bots 5 60          # 5 bot đi lại 60 giây (nhìn thấy trong client)
python tools/dev.py stop
```

Client: nhập máy chủ `127.0.0.1:17100`, tài khoản bất kỳ (tự tạo lần đầu, mật khẩu phải giống
lần sau) → tạo nhân vật → **Vào game** → click chuột trái để đi, Enter để chat, cuộn chuột để zoom,
Esc để về màn chọn nhân vật. Mở nhiều client cùng lúc để thấy nhau.

## 4. Test tự động

```bash
python tools/dev.py test               # ctest (C++) + go test + Godot headless
python tools/dev.py smoke              # zone + gateway + 1 bot: đăng nhập, đi 100 đơn vị, chờ đến nơi; exit 0 = OK
```

## 5. Log ở đâu

| Tiến trình | File | Bật chi tiết |
|---|---|---|
| zone | `logs/zone.log` | `--set log.levels=zone.move=trace,net=debug` hoặc `JX_LOG__LEVELS=...` |
| gateway | `logs/gateway.log` | `-set log.levels=net.recv=trace,session=debug` |
| client | `%APPDATA%\Godot\app_userdata\JX NEXT\logs\client.log` | chạy với `-- --log-levels=net=trace` |
| bot | stdout | `-log-level trace` |

Mọi dòng là JSON theo `docs/LOGGING.md`; đối chiếu ba phía theo `sid` (gateway cấp, client thấy
trong HelloAck và HUD) và `pid`.

Ví dụ: xem một người chơi đi từ lúc click tới lúc zone xác nhận:

```bash
grep '"sid":3' logs/gateway.log logs/zone.log
```

## 6. Cổng và cấu hình

| Tiến trình | Cổng | File | Ghi đè |
|---|---|---|---|
| zone | 17001 | `config/zone.json` | `--set zone.port=…`, `JX_ZONE__PORT=…` |
| gateway | 17100 (client), nối zone 17001 | `config/gateway.json` | `-set gateway.listen=:17200` |

Chạy 2 zone/gateway song song để test trên cùng máy: đổi `zone.port`, `gateway.zone`, `gateway.listen`
và `gateway.data_dir`.

## 7. Dừng server đúng cách

`python tools/dev.py stop` gửi CTRL_BREAK (Windows) / SIGTERM (Linux): zone và gateway ghi nốt log,
gateway lưu nhân vật, rồi thoát; sau 5 giây không thoát mới bị giết cứng. Đóng cửa sổ console bằng
nút X cũng là dừng êm (zone bắt SIGBREAK). Log zone được flush mỗi giây, nên kể cả khi bị giết cứng
cũng chỉ mất tối đa 1 giây log cuối.

## 8. Sự cố thường gặp

- **Client báo "zone chưa sẵn sàng" (mã 9)**: gateway chưa nối được zone — xem `logs/gateway.log`
  dòng `zone link down`; zone chưa chạy hoặc sai cổng.
- **`cmake --preset` báo lỗi vcpkg manifest**: `VCPKG_ROOT` đang trỏ tới bản zip không có git; dùng
  checkout git.
- **Godot không thấy addon godobuf**: mở project trong editor một lần để nó import; test headless
  không cần editor.
