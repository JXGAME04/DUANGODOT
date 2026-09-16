# JX NEXT — Lộ trình xây lại toàn bộ hệ thống (đa nền tảng)

Trạng thái: bản nháp 1 (2026-09-16). Mã cũ trong repo này là **tài liệu tham chiếu + oracle để đối chiếu**,
không phải nền để sửa tiếp. Mọi thứ mới nằm trong thư mục `next/` (monorepo) — xem mục 3.

Hai nguyên tắc bắt buộc, áp dụng cho **mọi bước** trong tài liệu này:

1. **Test từng phần** — một bước chỉ được coi là xong khi có test tự động chạy trong CI (unit, golden hoặc
   integration tuỳ bước) và test đó xanh trên cả Windows lẫn Linux.
2. **Log debug ngay từ đầu** — mọi module sinh ra phải có log có cấu trúc theo chuẩn ở mục 5 trước khi
   viết logic; không có "để sau thêm log".

---

## 1. Mục tiêu và phạm vi

- Client Godot 4.x: Windows, macOS, Linux, Android, iOS từ một codebase.
- Server C++20: build và chạy trên Windows (dev) và Linux (production, Docker).
- Giữ **luật chơi, công thức, dữ liệu, script và cảm giác 2.5D** của bản gốc; bỏ toàn bộ hạ tầng cũ
  (IOCP, DirectDraw, Berkeley DB, Lua 4.0, MSSQL/ADO, MD5, USBKey, mã hoá GBK/TCVN3).
- Kết quả cuối: hệ thống ngang bản cũ về gameplay, vận hành được như dịch vụ hiện đại
  (monitoring, hot-reload dữ liệu/script, CI/CD).

## 2. Quyết định kỹ thuật (đã chốt 2026-09-16)

| Hạng mục | Chọn | Lý do ngắn |
|---|---|---|
| Lõi mô phỏng | **C++20** — `server/zone` (tick, entity, combat, AOI, Lua), CMake + vcpkg, presets MSVC và GCC/Clang | tận dụng công thức/tham chiếu C++ cũ, hiệu năng, đa nền tảng |
| Dịch vụ xung quanh | **Go** — `services/gateway`, `auth`, `social`, `persist`, `admin`, phần lớn `tools/` | TLS/WebSocket/DB/ops mạnh, ít mã C++ phải bảo trì |
| Mạng | Go gateway: TLS + WebSocket (mobile/web) + TCP (PC); gateway↔zone: TCP nội bộ, protobuf | thay IOCP, chạy Linux |
| Giao thức | Protocol V2 = protobuf, length-prefixed, versioned, UTF-8; sinh mã C++ + Go + GDScript | không copy struct ra dây |
| Script | **Lua 5.4** + sol2 (không LuaJIT: iOS và kẹt 5.1) | di động, dễ debug |
| Dữ liệu | PostgreSQL (jsonb/bytea cho nhân vật, bảng quan hệ cho kinh tế) + Redis (phiên/cache) | bất đồng bộ, có schema |
| Log | spdlog (server), Godot `print_*` bọc lại thành `Log` autoload (client) | có cấu trúc, xoay file |
| Test | Catch2 (C++), GUT (GDScript), replay gói tin làm golden test | chạy trong CI |
| CI | GitHub Actions: matrix windows-latest + ubuntu-latest, self-hosted Windows cho build bản cũ | cổng chất lượng |
| Client | Godot 4.3+, GDScript cho UI/glue, GDExtension C++ cho codec/asset/pathfinding | đa nền tảng, hiệu năng chỗ cần |

## 2b. Thứ tự ưu tiên — làm trước những gì đắt nếu phải làm lại

Xếp theo "đổi sau sẽ tốn bao nhiêu"; làm đúng thứ tự này thì các phần sau chỉ *cộng thêm*, không phải sửa lại:

1. **Khung repo + build + CI hai nền tảng** — mọi thứ đứng trên nó; đổi sau = chạm mọi file.
2. **Chuẩn log + chuẩn test** — mọi module kế thừa; thêm sau = quay lại từng module.
3. **Mã hoá UTF-8 tại biên** — làm sai thì phải chuyển đổi lại toàn bộ dữ liệu và asset.
4. **ID và mô hình dữ liệu nhân vật (`RoleData.proto` có version)** — DB, giao thức, client đều phụ thuộc; đổi sau = migration.
5. **Khung Protocol V2** (header, version, lỗi, framing) — trước khi có bất kỳ message nội dung nào.
6. **Packet recorder trên bản cũ** — làm sớm để tích luỹ bằng chứng; không phụ thuộc mã mới.
7. **Định dạng asset/data mà Godot tiêu thụ** (atlas + JSON schema) — trước khi viết UI/map.
8. **Mô hình tick tất định của zone** — trước khi viết gameplay, vì test replay/đối chiếu dựa vào nó.
9. Sau đó mới đến vertical slice và từng hệ thống gameplay.

## 2c. Cách làm việc

- Mỗi phần: báo **"Đang làm: <phần>"** khi bắt đầu → làm xong báo **"Xong: <phần> — bạn kiểm tra"** kèm cách
  test; chủ dự án test/fix/duyệt rồi mới sang phần tiếp. Không làm chồng nhiều phần cùng lúc.
- Mọi phần đều có log debug theo mục 5 và test theo mục 6 ngay từ commit đầu.

## 3. Cấu trúc repo (monorepo trong `next/`)

```
next/
  proto/          IDL Protocol V2 (.proto) + sinh mã C++/Go/GDScript
  data/           game data data-driven (JSON/CSV + schema) xuất từ Settings cũ
  tools/          exporter asset (.spr/.pak/map), converter data, converter Lua4->5, packet recorder/replay (Go/Python)
  server/         C++20 (CMake + vcpkg)
    common/       log, config, clock, ids, metrics
    zone/         tick, entity/component, spatial grid + AOI, combat core, Lua 5.4
  services/       Go (một go.mod, nhiều cmd/)
    gateway/      TLS/WebSocket/TCP, phiên (sid), rate-limit, dịch V2 <-> (tạm) giao thức cũ
    auth/         tài khoản, argon2, token
    social/       chat, bang, bạn bè, mail
    persist/      ghi bất đồng bộ, schema có version
    admin/        API quản trị
    pkg/log       logger Go cùng định dạng JSON với server/common
  client-godot/   project Godot + gdextension/
  .github/workflows/next-ci.yml (ở gốc repo)  CI: matrix Windows + Ubuntu, Go, Godot export
  docs/           tài liệu (file này, LOGGING.md, TESTING.md, ADR)
```

## 4. Bắt đầu từ đâu — tuần 1 (theo đúng thứ tự)

1. Tạo `next/` với CMake presets (`windows-msvc`, `linux-gcc`), một target `common` chứa **Log** + **Config**
   + **Clock** và một test Catch2 "hello": log ra file JSON, test đọc lại được dòng log. → CI xanh 2 nền tảng.
2. Chuẩn log (mục 5) viết thành `docs/LOGGING.md` + code mẫu; chuẩn test (mục 6) thành `docs/TESTING.md`.
3. Project Godot rỗng có autoload `Log` (cùng định dạng dòng log với server) + 1 test GUT; export
   template Windows + Android build được trong CI.
4. **Packet recorder** cho bản cũ: chạy client/server cũ, ghi lại mọi gói tin (hex + thời gian + hướng)
   thành file `.jxrec` — đây là dữ liệu vàng để đối chiếu sau này. Bắt đầu từ Rainbow (client relay).
5. **Exporter asset** bước đầu: đọc `.pak` và `.spr` → PNG atlas + JSON; test bằng cách xuất 3 sprite và
   so CRC với bản cũ. Log số lượng file, thời gian, lỗi theo file.
6. Bảng chuyển mã TCVN3→Unicode, GBK→Unicode trong `tools/encoding`, có test với các chuỗi thật lấy từ
   `Settings` cũ. Mọi công cụ xuất dữ liệu bắt buộc đi qua đây; dữ liệu mới **chỉ** là UTF-8.
7. Sườn `proto/`: `common.proto` (header, version, error), `auth.proto`, và bộ sinh mã; test round-trip.
8. ADR-001 (kiến trúc), ADR-002 (giao thức), ADR-003 (dữ liệu nhân vật) — mỗi cái một trang.

Tiêu chí xong tuần 1: CI xanh trên 2 nền tảng, có 1 dòng log chuẩn từ cả server lẫn client, có 1 file
`.jxrec` thật, có 3 sprite xuất đúng.

## 5. Chuẩn log debug (áp dụng từ dòng code đầu tiên)

- Định dạng: một dòng JSON/dòng, các trường cố định:
  `ts` (UTC, µs), `lvl` (trace/debug/info/warn/error/fatal), `cat` (net, proto, auth, zone.tick, zone.combat,
  zone.ai, lua, db, asset, ui, …), `sid` (session id), `pid` (player id nếu có), `zone`, `msg`, và
  các trường bổ sung dạng key=value. Cùng định dạng ở server và client để ghép được theo `sid`.
- Mỗi gói tin ra/vào có log `trace` ở `cat=net` với `msg_id`, `len`, `seq`; bật/tắt theo category bằng
  config lúc chạy (hot-reload), không cần build lại.
- Mỗi tiến trình: log ra console + file xoay (spdlog rotating), ring buffer 10.000 dòng gần nhất được
  ghi kèm khi crash (crash handler + minidump trên Windows, core dump trên Linux).
- ID tương quan: gateway cấp `sid`; mọi service log `sid` khi xử lý gói của phiên đó; zone log thêm
  `tick`. Truy được một hành động của người chơi xuyên suốt gateway → zone → db bằng một `grep`.
- Client Godot: autoload `Log` cùng định dạng, ghi `user://logs/`, có overlay debug (F3) hiện tick,
  ping, số entity, msg/s; log gói tin dạng tóm tắt.
- Cấm `printf`/`print` trần trong code mới; review chặn.
- Lua: hàm `log.debug/info/warn/error` gắn `cat=lua`, có tên script + dòng.

## 6. Chuẩn test và cổng CI

| Loại | Ở đâu | Bắt buộc từ bước |
|---|---|---|
| Unit (Catch2/GUT) | mọi module: serializer, công thức, parser data, pathfinding, AOI | tuần 1 |
| Round-trip proto | `proto/`: encode→decode→equal cho mọi message | bước 1.1 |
| Golden/replay | phát lại `.jxrec` qua gateway mới, so kết quả với bản cũ (ID, thứ tự, giá trị) | bước 1.3 |
| Đối chiếu công thức | cùng input → sát thương/exp/drop của Core cũ (chạy DLL cũ trong harness x86) và của zone mới phải bằng nhau | bước 3.1 |
| Integration | vertical slice tự động: auth→gateway→zone→client headless, chạy trong CI bằng Docker compose | bước 2.5 |
| Tải | bot Godot headless/C++ mô phỏng 500–2000 kết nối vào 1 zone | bước 4.2 |
| Đa nền tảng | matrix Windows + Linux server; export Windows + Android client | tuần 1 |

Cổng CI: build 0 cảnh báo (`/W4` mới, `-Wall -Wextra`), test xanh, sanitizer (ASan/UBSan) trên Linux, coverage
không giảm. Không merge nếu đỏ.

Definition of Done cho mọi bước bên dưới: **code + log theo chuẩn + test tương ứng + tài liệu 5 dòng**.

## 7. Lộ trình theo giai đoạn

Thời lượng ước tính cho nhóm 2–3 người làm toàn thời gian; một người thì nhân ~3.

### Giai đoạn 0 — Nền (tuần 1–2)
Toàn bộ mục 4. Đầu ra: repo + CI + log + test + recorder + exporter sơ khai + proto sườn.

### Giai đoạn 1 — Dữ liệu và giao thức (tuần 3–6)
- 1.1 IDL đầy đủ: trích ngữ nghĩa 62 message + phần relay/tong từ `Headers/KProtocol*.h` và
  `KProtocolProcess.cpp` (86 handler) thành `.proto`, mỗi message có comment "tương đương s2c_/c2s_ nào".
  Test: round-trip + `static_assert`/bảng kích thước của struct cũ để chắc chắn không bỏ sót trường.
- 1.2 Exporter hoàn chỉnh: sprite (kèm palette/anim), map (region, obstacle, trap, ground layer), UI `.ini`
  → JSON layout, font, âm thanh. Log: từng file, lỗi, thống kê. Test: CRC/golden trên tập mẫu.
- 1.3 Converter data: 62 bảng Settings → `data/` có schema (JSON Schema) + validator chạy trong CI; mọi
  bảng có test "load được, đúng số dòng, không mất mã tiếng Việt".
- 1.4 Mô hình dữ liệu nhân vật (ADR-003): `RoleData.proto` có version; công cụ đọc `TRoleData` cũ
  (Goddess/BDB) → proto để có dữ liệu test thật. Test: 10 nhân vật thật chuyển đổi và kiểm tra trường.
- 1.5 Codec V2 trong GDExtension và server `common`; log `cat=proto`. Test: fuzz nhẹ (gói hỏng không crash).

### Giai đoạn 2 — Vertical slice (tuần 7–12)
- 2.1 `auth`: đăng ký/đăng nhập, argon2, token; PostgreSQL migration; log `cat=auth`; test integration DB.
- 2.2 `gateway`: TLS, phiên, `sid`, rate-limit, chuyển tiếp tới zone; test replay `.jxrec` phần login.
- 2.3 `zone` sườn: tick cố định 20Hz tất định, entity/component, spatial grid, AOI; 1 map từ exporter;
  di chuyển server-authoritative + NPC đứng. Log `cat=zone.tick` có thống kê ms/tick; test: tick tất định
  (cùng input → cùng state hash), AOI đúng (unit).
- 2.4 `persist` sườn: ghi/đọc `RoleData` bất đồng bộ; test crash giữa chừng không mất dữ liệu.
- 2.5 Client Godot: login → danh sách nhân vật → vào map → đi lại → thấy NPC; overlay debug; build
  Windows **và** Android chạy được. Test integration tự động (client headless) trong CI.
Tiêu chí: một người chơi đi trong map trên PC và điện thoại, log ghép được xuyên 4 tiến trình theo `sid`.

### Giai đoạn 3 — Hệ thống gameplay (tháng 4–9)
Thứ tự theo phụ thuộc; mỗi hệ thống = một checklist từ handler cũ + màn hình UI cũ tương ứng.
- 3.1 Combat core: thuộc tính, sát thương, skill, missile, trạng thái — viết lại theo công thức cũ, **test đối
  chiếu số** với Core cũ qua harness x86 (cùng input, cùng kết quả). Log `cat=zone.combat` theo `pid`.
- 3.2 Item/túi/trang bị/shop/drop; kinh tế trong bảng quan hệ; test tính chất (không âm, không nhân bản).
- 3.3 AI NPC/quái, spawn, patrol; test kịch bản.
- 3.4 Lua 5.4: thiết kế API từ 235 hàm cũ; sandbox, hot-reload, log `cat=lua`; test: mỗi hàm binding có test.
- 3.5 Quest/nhiệm vụ trên Lua: converter Lua4→5 + shim; chạy replay quest từ `.jxrec` làm golden.
- 3.6 `social`: chat kênh, bạn bè, mail, bang hội; test integration nhiều phiên.
- 3.7 Tổ đội, giao dịch, PK, bảng xếp hạng; test tính đúng đắn giao dịch (atomic).
- 3.8 Client: từng hệ thống một màn hình, UI mobile-first, overlay debug mở rộng.
Tiêu chí: checklist 86 handler + 49 màn hình cũ được tích hết, replay `.jxrec` đầy đủ khớp.

### Giai đoạn 4 — Vận hành (song song từ tháng 6)
- 4.1 Prometheus metrics + Grafana dashboard (tick ms, msg/s, người/zone, DB latency, lỗi/phút).
- 4.2 Test tải bot 500–2000 kết nối; tối ưu AOI/serialization theo số đo, không đoán.
- 4.3 Admin tool (Godot hoặc web): xem/khoá tài khoản, teleport, tặng vật phẩm, xem log theo `sid`.
- 4.4 Docker compose cho dev, Dockerfile production, backup/restore DB có test khôi phục.
- 4.5 Hot-reload data + script có kiểm tra schema trước khi áp dụng.

### Giai đoạn 5 — Chuyển nội dung (tháng 8–10)
- 5.1 Chuyển toàn bộ 2.604 script qua converter; phần shim giảm dần; mỗi script có smoke test "nạp không lỗi".
- 5.2 Chuyển toàn bộ dữ liệu bảng, bản đồ, sự kiện; validator xanh 100%.
- 5.3 Cân bằng: chạy bot theo kịch bản trên cả hai hệ thống, so bảng thống kê (exp/giờ, drop, tử vong).

### Giai đoạn 6 — Beta và mở rộng (tháng 10–12)
- 6.1 Beta kín với người chơi thật; crash/lỗi theo dõi từ log tập trung (Loki/ELK).
- 6.2 Nhiều zone/tiến trình, chuyển map giữa tiến trình, mở rộng theo chiều ngang.
- 6.3 Tắt bản cũ khi replay và số liệu cân bằng khớp; lưu bản cũ làm oracle vĩnh viễn (đã có trong repo).

## 8. Rủi ro chính và cách giảm

| Rủi ro | Giảm bằng |
|---|---|
| Sai công thức so với bản cũ | harness đối chiếu số (3.1) chạy trong CI; oracle là DLL cũ |
| Lua 4→5 vỡ script | converter + shim + smoke test từng script, chuyển dần theo nhiệm vụ |
| Mã hoá tiếng Việt | UTF-8 tại exporter, cấm chuỗi non-UTF-8 vào `data/` (validator) |
| Đa nền tảng vỡ muộn | CI 2 nền tảng từ tuần 1, Android build từ giai đoạn 2 |
| Khó truy lỗi khi có nhiều tiến trình | `sid`/`tick` trong mọi dòng log, log tập trung từ giai đoạn 4 |
| Phạm vi phình | mỗi hệ thống bám checklist handler/màn hình cũ, không thêm tính năng mới trước beta |

## 9. Bảng theo dõi

- [ ] Tuần 1 hoàn tất (mục 4)
- [ ] Giai đoạn 1: 1.1 · 1.2 · 1.3 · 1.4 · 1.5
- [ ] Giai đoạn 2: 2.1 · 2.2 · 2.3 · 2.4 · 2.5 — vertical slice trên PC + Android
- [ ] Giai đoạn 3: 3.1 · 3.2 · 3.3 · 3.4 · 3.5 · 3.6 · 3.7 · 3.8
- [ ] Giai đoạn 4: 4.1 · 4.2 · 4.3 · 4.4 · 4.5
- [ ] Giai đoạn 5: 5.1 · 5.2 · 5.3
- [ ] Giai đoạn 6: 6.1 · 6.2 · 6.3
