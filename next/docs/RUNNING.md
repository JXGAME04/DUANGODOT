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

Nguồn dữ liệu: `config/oldgame.local.json` (mẫu `config/oldgame.example.json`) trỏ tới **client VLTK 2.0** (thư mục có
`config.ini` + `data/*.pak`), client dự phòng (`bin/Client`, chỉ cấp file 2.0 thiếu) và **server Linux** (`D:\ServerLinux\server1`:
`Settings/npcs.txt`, `pak/maps.pak`, `lang/vn/replacename_npc.txt`); không có file này thì dùng `../bin/Client` + `../bin/Server`
(hoặc `JX_OLD_CLIENT`/`JX_OLD_SERVER`). Vai trò từng nguồn: [REFERENCES.md](REFERENCES.md). Zone chạy **level script Lua**
(`script/npclevelscript/*.lua`) của thư mục server đó qua `zone.script_root` — `dev.py start` tự đặt `JX_ZONE__SCRIPT_ROOT`;
không có thì máu/sát thương quái dùng số tạm (xem NPCRES.md mục 4). Zone chứa nhiều map: `zone.map_dir` (map mặc định) +
`zone.maps` = "3,7,99" trong `zone.maps_dir` — xuất trước bằng `python tools/dev.py assets 1 3 7 99` (cổng Phượng Tường dẫn
sang 3 Kiếm Các Tây Bắc, 7 Tần Lĩnh, 99 Vĩnh Lạc Trấn); map chưa xuất bị bỏ qua (log `map bundle skipped`).
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

## 3b. Tài khoản

Mật khẩu **luôn** được băm bằng argon2id (`services/pkg/auth/password.go`); file
`data/gateway/accounts.json` không còn chứa mật khẩu thường (bản dev cũ có thì được băm lại lúc
gateway khởi động). Hai chế độ, đặt bằng `gateway.auth_mode`:

| Chế độ | Nghĩa |
|---|---|
| `dev` (mặc định) | tài khoản mới tự tạo khi đăng nhập lần đầu — tiện cho test/bot, mật khẩu ngắn được chấp nhận |
| `strict` | chỉ tài khoản đã tạo bằng `jxaccount` mới vào được, mật khẩu ≥ 6 ký tự như `LOGIN_PASSWORD_MIN_LEN` cũ |

```bash
build/go/jxaccount -data data/gateway add thu1 matkhau123     # tạo tài khoản
build/go/jxaccount -data data/gateway passwd thu1 matkhaumoi
build/go/jxaccount -data data/gateway freeze thu1 "gian lận"  # khoá (E_ACCOUNT_FREEZE cũ)
build/go/jxaccount -data data/gateway unfreeze thu1
build/go/jxaccount -data data/gateway expire thu1 24          # còn 24 giờ chơi (0 = vô hạn)
build/go/jxaccount -data data/gateway list
```

Chạy `jxaccount` khi gateway đang tắt (cả hai cùng ghi `accounts.json`).

## 3c. Bảo vệ phiên

Gateway tự bảo vệ theo `config/gateway.json`: chờ `Hello` 10 s, im lặng ở lobby 300 s, im lặng
trong game 30 s (client ping 5 s), 40 gói/giây mỗi client (burst 100), 5 lần đăng nhập sai trên
một kết nối, 5 lần sai một tài khoản → khoá 60 s. **Một tài khoản chỉ một phiên**: đăng nhập lần
hai đá phiên cũ (`refuse_duplicate_login = true` để từ chối như server cũ). Khi tắt, gateway đá
mọi client rồi chờ zone lưu xong (`shutdown_wait_s`). Bảng đầy đủ: [PROTOCOL.md](PROTOCOL.md) mục 3b.

Mỗi 30 giây gateway ghi một dòng `cat=gw.stats`: số phiên, tài khoản online, gói/giây, KB/giây,
số lần đăng nhập sai, bị đá, quá hạn. Đây là chỗ để nối Prometheus sau (`Server.Snapshot()`).

## 4. Test tự động

```bash
python tools/dev.py test               # ctest (C++) + go test + Godot headless
python tools/dev.py smoke              # zone + gateway + 1 bot: đăng nhập, đi 100 đơn vị, chờ đến nơi; exit 0 = OK
```

## 5. Log ở đâu

| Tiến trình | File | Bật chi tiết |
|---|---|---|
| zone | `logs/zone.log` | `--set log.levels=zone.move=trace,net=debug` hoặc `JX_LOG__LEVELS=...` |
| gateway | `logs/gateway.log` | `-set log.levels=net.recv=trace,session=debug,gw.stats=info` |
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
- **"Tài khoản hoặc mật khẩu không đúng"** ở chế độ `dev`: tài khoản đã tồn tại với mật khẩu khác
  (`jxaccount list` để xem, `jxaccount passwd` để đổi).
- **"Đăng nhập sai quá nhiều lần"**: tài khoản bị khoá 60 giây (`gateway.max_fails`, `lock_s`).
- **"Tài khoản vừa được đăng nhập ở nơi khác"**: client thứ hai dùng cùng tài khoản đã đá client
  này; mở 2 client thì dùng 2 tài khoản.
- **`cmake --preset` báo lỗi vcpkg manifest**: `VCPKG_ROOT` đang trỏ tới bản zip không có git; dùng
  checkout git.
- **Godot không thấy addon godobuf**: mở project trong editor một lần để nó import; test headless
  không cần editor.
