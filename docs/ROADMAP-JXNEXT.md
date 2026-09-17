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

- [x] Bước 1 (2026-09-16): khung `next/`, `jx::common` (log/config/clock/ids), CMake 2 nền tảng, CI.
- [x] **M1 — lát cắt dọc chạy được (2026-09-17)**: Protocol V2 (`proto/jx`, sinh C++/Go/GDScript),
      khung tin + Asio (`server/net`), zone C++ (AOI grid, di chuyển fixed-point xác định, NPC đi lại,
      PlayerSave), gateway Go (phiên, auth dev, persist file, cầu zone, tự nối lại), `jxbot`, client Godot
      (đăng nhập → nhân vật → map lưới, click đi, chat, HUD), `tools/dev.py` (build/start/e2e/screenshot),
      CI: C++ Win+Linux, Go Win+Linux, client headless, e2e Linux. Chi tiết: `next/docs/RUNNING.md`.
- [x] **M2 — tài sản cũ (2026-09-17)**: `jxassets` đọc `.pak` (hash tên, UCL NRV2B, spr nén frame), `.spr` → PNG atlas,
      `.wor`/`Region_C.dat` → bundle `client/assets/maps/<id>` (map.json, obstacle.bin, region JSON); bảng TCVN3/GBK→UTF-8;
      zone nạp lưới vật cản, A* + làm mượt, NPC từ map (63 NPC Phượng Tường); client vẽ map thật theo region,
      đi theo waypoints. Chi tiết: `next/docs/MAPS.md`.
      - 2026-09-17 (sau khi so với client cũ ở lò rèn Phượng Tường): vật tĩnh đặt theo FRAME_DRAW (không cộng offset khung),
        vật động theo REF_SPOT (oPos1 − tâm sprite), thứ tự vẽ port nguyên **cây KIpoTree/KIpotBranch/KIpotLeaf/SceneMath**
        của game cũ thay cho y-sort; `jxassets objects` in bản ghi thô để đối chiếu.
- [x] **M3a — sprite nhân vật/NPC (2026-09-17)**: `jxassets export-npcres` đọc `npcs.txt` + `Settings/npcres` (KTabFile,
      KNpcTemplate, KNpcResNode/KNpcResList port sang Go) → `client/assets/npcres`; client ghép bộ phận nhân vật chính
      (Head/Body/tay/vũ khí + bóng, thứ tự vẽ theo `贴图顺序表`), NPC một ảnh, 64 hướng (`KMath`), khung theo tick 18 Hz
      như `KNpcRes::Draw`/`KNpc::Paint`; zone gửi `dir` 0..63; tên NPC trên map lấy từ `npcs.txt`. Chi tiết `next/docs/NPCRES.md`.
      Còn lại: đánh/bị đánh/chết/ngồi, trang bị theo RoleData, ngựa, hiệu ứng, texture memory.
      - 2026-09-17 **NPC thật của thành**: đọc `Region_S.dat` từ `bin/Server/pak/maps.pak` (1409 NPC server map 1: người
        trong thành + quái có level) + 63 NPC client-only, tọa độ tuyệt đối, tên qua `replacename_npc.txt`; zone sinh theo
        `kind` (0 = quái); `jxassets npcs`. NPC không trang bị: `KItemChangeRes` (helm 18/armor 18/weapon 0).
- [x] **M3b — đánh/bị đánh/chết (2026-09-17)**: zone 18 tick/s, `KNpcTemplate` đọc npcs.json, `C2G_ATTACK` →
      `EntityAction`/`EntityLife`; nhịp khung như `KNpc::DoAttack/OnSpecial1(60 %)/DoHurt/DoDeath/DoRevive`, tự đi tới mục tiêu,
      tự vung tiếp; client hoạt ảnh Attack1/2, Wound, Die (giữ khung cuối), thanh máu, chọn mục tiêu bằng click; `--auto` tự
      đánh quái (`AUTO_FIGHT`). Sát thương/máu tạm (chờ Lua), quái chưa đánh trả. Chi tiết `next/docs/NPCRES.md` mục 4.
- [x] **M3e — bẫy/cổng, nhiều map trong một zone (2026-09-17)**: `Region_S.dat` trap → `map.json traps` (id = `g_FileName2Id`
      của script, tra ngược qua `script/` của chuỗi server), `KNpc::CheckTrap` mỗi tick, script trap chạy qua Lua 5.4 với
      `ScriptFuns` (`GetFightState/SetFightState/SetPos/NewWorld/GetPos/Msg2Player`), `KGameServer` chứa nhiều `KSubWorld`
      (`zone.maps`), `G2C_CHANGE_MAP` + `RolePosition.map_id`; client nạp lại bundle. Toạ độ script là Mps tuyệt đối →
      `KSubWorld::to_local/to_absolute` qua `origin` của `map.json`. Sửa lỗi **id template lệch 1** (đánh số từ
      0) làm tên/sprite NPC lệch. Map 1 ↔ 3/7/99 đã xuất. Test `test_KTrap.cpp`. Chưa: `Say/Talk` (hộp thoại), `AddStation`,
      sang server khác, chết/hồi sinh người chơi trong chế độ chiến đấu.
- [x] **M3d — máu/sát thương/kinh nghiệm thật qua Lua 5.4 (2026-09-17)**: `KLuaScript`/`KScriptCache` (Lua 5.4.8 vcpkg,
      lớp tương thích Lua 4 nên `npclevelscript/*.lua` của server Linux chạy nguyên văn), `KNpcTemplateSet::level_data` =
      `InitNpcLevelData` (Exp/Life/AR/Defense/Min-MaxDamage/LifeReplenish/Resist/Level1..4, cache theo cấp), `CheckHitTarget`,
      `CalcDamage(physics)`, hồi máu `ProcessState`; `zone.script_root` ← `oldgame.local.json`. Còn tạm: số của người chơi,
      attrib kỹ năng. Test `test_KLuaScript.cpp`.
- [x] **M3c — quái đánh trả (2026-09-17)**: `KNpcAI` port 1‑1 sáu chế độ `AIMode` (chủ động/bị động, hồi máu, bỏ chạy),
      `KeepActiveRange/GetNearestNpc/CommonAction/FollowAttack/KeepAttackRange/Flee`, quan hệ camp `GenOneRelation`, nhịp
      `AIMaxTime`, tốc độ `WalkSpeed`/khung, kỹ năng Skill1..4 + bán kính từ `skills.txt` (`KSkillManager.go`), AI tắt khi đang
      vung/giật/chết; 6 test `test_KNpcAI.cpp`. Chưa: `KPathFinder`, `KMissle`, công thức hồi máu, PK. `NPCRES.md` mục 4.
- [x] **Hai bản tham chiếu (2026-09-17, theo yêu cầu "làm theo bản Linux và VLTK 2.0")**: `jxassets`/`dev.py assets` đọc
      client VLTK 2.0 (`config.ini` [Package], maplist/npcs.txt trong pak) + server Linux VNG (`Settings/npcs.txt`,
      `pak/maps.pak`, `lang/vn/replacename_npc.txt`) qua `config/oldgame.local.json`; chuỗi client dự phòng `-client "a;b"`
      (2.0 thiếu bảng nhân vật chính/động tác/BaseValue/item res + 5 sprite map, đều được log); template = dòng server +
      hình từ dòng client (`export.MergeAppearance`, 128 template khác res). `client/assets` xuất lại từ hai bản này.
      Chi tiết `next/docs/REFERENCES.md`. Bản Linux còn cho: `npclevelscript/*.lua` (công thức máu/sát thương thật), cột AI.
- [x] Quy ước đặt tên (2026-09-17, theo yêu cầu): file/lớp mới **đặt theo tên mã cũ cùng chức năng** (`KNpc`, `KSubWorld`,
      `KRegion`, `KMapData`, `KGameServer`, `KSocket`, `UiLogin`, `UiSelPlayer`, `UiGame`, `KScenePlaceC`, `XPackFile`, `KSprite`…);
      bảng đối chiếu `next/docs/OLD-TO-NEW.md`.
- [x] **M4a — nền mạng: tài khoản + phiên (2026-09-17)**: mật khẩu argon2id (`auth/password.go`), máy chủ tài khoản
      `S3PAccount` port từ `S3PAccount::Login` (sai mật khẩu/khoá/hết giờ chơi/khoá tạm sau 5 lần sai), hai chế độ
      `dev`/`strict`, công cụ `jxaccount` (add/passwd/freeze/expire/list); `persist.Account` có hash + trạng thái, bỏ mật khẩu
      thường (tự băm lại khi khởi động). Gateway: **một tài khoản một phiên** (đá phiên cũ hoặc từ chối như `E_ACCOUNT_EXIST`),
      timeout `Hello`/lobby/heartbeat, giới hạn gói/giây (token bucket), giới hạn lần đăng nhập sai, `SessionClose.reason`,
      tắt êm **chờ zone lưu xong**, thống kê `cat=gw.stats` + `Server.Snapshot()` cho Prometheus sau. Client: `KLogin.gd`
      dịch mã kết quả theo `LOGIN_R_*`/`CI_MI_*` cũ, watchdog heartbeat, bị đá thì về màn đăng nhập kèm lý do.
      Test: `pkg/auth` (6), `internal/gateway` (7 mới). Chi tiết `next/docs/PROTOCOL.md` mục 3b, `RUNNING.md` mục 3b/3c.
- [x] **M4b — mô hình dữ liệu nhân vật có phiên bản (2026-09-17, ưu tiên 2b mục 4)**: `RoleData.data_version` +
      `persist.CurrentRoleVersion` (2) + chuỗi di trú `MigrateRole` (`TRoleData.go`): bản ghi cũ được nâng cấp **một lần**
      lúc mở store rồi ghi lại, bản ghi **mới hơn bị từ chối** (`ErrNewerData`) thay vì để server cũ ghi đè mất trường
      (lỗi kinh điển của `TRoleData` cũ trong Goddess/BDB). Bước 1: điền trường của bản chưa có version; bước 2: sửa
      chỉ số hỏng (max > 0, hp/mp trong khoảng, move_speed). Thêm `RoleData.fight_mode` (trạng thái chiến đấu giữ qua
      lần đăng nhập). Test: 4 test `TRoleData_test.go` + `test_KTrap.cpp` (lưu/khôi phục fight_mode). ADR-003.
- [x] **M4c — codec chịu được gói hỏng (2026-09-17, giai đoạn 1 mục 1.5)**: fuzz khung tin ở cả ba bên —
      Go `FuzzParser`/`FuzzReader` (21 triệu lượt không lỗi, CI chạy 30 s mỗi PR), C++ `[fuzz]` 500 vòng seed cố định,
      GDScript `test_frame_fuzz` 300 vòng; `KGarbage_test.go`: 40 kết nối bắn id/payload ngẫu nhiên vào gateway,
      phiên đang chơi vẫn ping/đi lại bình thường và mọi kết nối rác bị đóng. `docs/TESTING.md` mục 3b.
- [x] **M4d — đường truyền TLS + WebSocket (2026-09-17, quyết định mục 2 "Mạng")**: `services/pkg/transport`
      (`KListener.go` mở nhiều cửa, `KWebSocket.go` tự cài RFC 6455 — bắt tay, message nhị phân, phân mảnh,
      ping/pong, close, **không phụ thuộc thư viện ngoài**); gateway mở đồng thời TCP 17100 và WebSocket 17102,
      TLS chung cho cả hai qua `gateway.tls_cert/tls_key`. Client Godot: `KNetAddress.gd` + `KSocketClient.gd` chọn
      `StreamPeerTCP`/`StreamPeerTLS`/`WebSocketPeer` theo địa chỉ (`tls://`, `ws://`, `wss://`), HUD hiện đường đang
      dùng; `jxbot -gateway ws://...`. Test: 8 test transport (bắt tay, phân mảnh, ping, frame không mask, quá cỡ,
      TLS/wss tự ký), 1 test gateway qua WebSocket, 10 test địa chỉ ở client; `dev.py e2e` chạy client qua **cả hai**
      đường. `docs/PROTOCOL.md` mục 1b, `RUNNING.md` mục 3a.
- [x] **M4e — bộ ghi gói tin `.jxrec` (2026-09-17, tuần 1 mục 4.3, ưu tiên 2b mục 6)**: `services/pkg/jxrec`
      (định dạng `JXREC1` + header JSON + bản ghi `dir|ms|len|bytes`, đọc được cả file bị cắt giữa chừng) và
      `cmd/jxrecord` (`proxy` đứng giữa client ↔ server ghi cả hai chiều, `dump` xem lại kèm `-jx` giải mã
      msg id, `-hex`). Không hiểu nội dung nên ghi được **cả bản cũ** (luồng riêng của Rainbow/Bishop) lẫn
      Protocol V2. Đã ghi thật một phiên bot: 19 bản ghi, 1230 byte, dump ra đúng chuỗi Hello → Login →
      CharCreate → EnterWorld → Move. Test: 4 test `KPacketRecord_test.go`. `docs/TESTING.md` mục 3c.
- [x] **M4f — ADR (2026-09-17, tuần 1 mục 4.8)**: `next/docs/adr/` — ADR-001 kiến trúc (C++ zone / Go services /
      Godot client / Lua 5.4), ADR-002 Protocol V2 (khung tin, protobuf, đường truyền, phiên bản),
      ADR-003 dữ liệu nhân vật (`RoleData`, id, `data_version`, di trú), ADR-004 tài khoản và phiên
      (argon2id, một tài khoản một phiên, timeout/giới hạn gói, tắt êm). Mỗi ADR: bối cảnh, quyết định,
      phương án đã cân nhắc, hệ quả.
- [x] **M4g — `/healthz` (2026-09-17)**: cổng WebSocket trả lời `GET /healthz` bằng JSON
      (`zone_ready`, `sessions`, `online`, `stopping`; `503` khi chưa sẵn sàng hoặc đang tắt) —
      `dev.py start` chờ tín hiệu này nên `e2e` hết lỗi chập chờn "zone chưa sẵn sàng" (3 lần chạy liên tiếp đều xanh);
      cũng là chỗ để cân bằng tải và giám sát cắm vào (giai đoạn 4.1). Test `KHealth_test.go`.
- [x] Tuần 1 (mục 4) đã xong toàn bộ.
- [x] **M5a — MASTER SPEC Phase A: nền runtime nhiều nhân (2026-09-17)**: thư viện mới `server/core` (`jx::core`) —
      `ServerClock`/`ServerTick`/`TickRate` (mọi thứ trong gameplay tính bằng tick, §21/§59/§61), `FixedTick`
      (chạy đúng số tick từ thời gian thật, chống spiral, enum 15 phase của §22 + `TickProfile` đo từng phase),
      `Result`/`Status`/`ErrorCode` (lỗi là giá trị, không ném ngoại lệ qua biên thread, §79/§88),
      `CommandQueue`/`EventQueue` (nhiều người ghi, một chủ sở hữu đọc — cách duy nhất để thread khác tác động
      vào state, §19/§30/§73), `ThreadPool` (nơi **duy nhất** tạo thread, số luồng từ cấu hình/`hardware_concurrency`,
      không pin core, join sạch, nuốt exception — §2/§14/§15/§16/§87/§88), `JobSystem` (`parallel_for`/`run_all`,
      mỗi lệnh là một barrier §23, luồng gọi cũng làm việc, pool dừng thì chạy tại chỗ thay vì treo),
      `Metrics` (counter/gauge/timing histogram → avg/P50/P95/P99/max, JSON, §51/§52/§53/§96). Log chuyển sang
      **bất đồng bộ** (§49): worker chỉ đẩy dòng vào hàng đợi, một luồng ghi file, tắt máy thì xả hết hàng đợi.
      28 test mới, ctest 96/96. Chưa đụng gameplay đa luồng (đúng Phase A).
- [x] **M5b — MASTER SPEC Phase B: Entity handle an toàn (2026-09-17)**: `server/entity` (`jx::entity`) —
      `EntityHandle` đóng gói `index(32) | generation(32)` vào chính `EntityId` 64-bit đang chạy trên dây
      (§5), `EntityTable` là EntityManager của §36: sparse-dense nên **handle bền**, **bộ nhớ liên tục** cho
      vòng lặp nóng (§47/§77) và **không có mảng cố định `Npc[MAX_NPC]`**. Ô nhớ được tái dùng thì generation
      tăng, mọi handle cũ thành vô hiệu ngay (bug kinh điển của server cũ: id cũ trỏ sang con quái mới).
      `KSubWorld` đã chuyển hẳn sang `EntityTable` (bỏ `unordered_map` + `IdGenerator`). 13 test riêng
      (handle, tái dùng ô, handle lạ, iterate khi đang xoá, 100k entity, kiểu move-only); ctest 109/109, e2e OK.
- [x] **M5c — MASTER SPEC Phase C/H/I/J: map instance có chủ sở hữu, worker pool, scheduler (2026-09-17)**:
      `KMapInstance` (một bản chạy của map, có inbox lệnh + hàng sự kiện + chủ sở hữu + chi phí tick),
      `KWorldCommand`/`KWorldEvent` (SpawnPlayer/RemovePlayer/ClientPacket/SaveRequest ↔ SessionOpened/
      PlayerSave/WorldChange), `KWorldScheduler` (xếp map cho worker theo chi phí đo được, dời map khỏi
      worker nóng, có ngưỡng chống dao động). `KGameServer` viết lại theo mô hình: **luồng mạng không
      còn sửa world** (§30) mà đẩy lệnh; mỗi tick chạy song song các map theo worker (§6/§7/§23), rồi
      luồng server gom sự kiện (ack/save/đổi map) và gói tin gửi đi. Lua: mỗi map instance có `KScriptCache`
      riêng và `g_ScriptContext` là `thread_local` (§42). Chạy thật: 4 map / 4 worker, tick trung bình
      **25,1 ms → 16,0 ms**, log `zone.tick` có p95/p99 và tải từng worker. 5 test scheduler; ctest 114/114, e2e OK.
- [x] **M5d — MASTER SPEC 44/45: ngân sách AI và entity ngủ (2026-09-17)**: lưới không gian đếm người chơi theo
      ô (`KRegionGrid::for_each_player_cell`), mỗi tick `KSubWorld` dựng tập ô "thức" = ô có người chơi nở rộng
      theo tầm nhìn AI lớn nhất của map. NPC ngoài vùng đó **không chạy AI, không đi lang thang** và chỉ hồi máu
      thưa hơn 8 lần; NPC đang bận (đang đi, đang đánh, bị thương, chưa về nhà) luôn thức nên không có hành động
      nào bị đóng băng giữa chừng. Đo thật trên 4 map / 3076 entity / 1 người chơi:
      **tick trung bình 15,98 ms → 1,37 ms**, max 626 ms → 18,4 ms, tick bị rớt 24 → 0, chỉ 42 entity thức.
      Log `zone.tick` thêm trường `awake`. Test: npc xa người chơi ngủ, lại gần thì thức.
- [ ] Giai đoạn 1: 1.1 · 1.2 · 1.3 · 1.4 · 1.5 (kế tiếp: hoạt ảnh đánh/chết + trang bị, minimap, bẫy/cổng, NPC từ script)
- [ ] Giai đoạn 2: 2.1 · 2.2 · 2.3 · 2.4 · 2.5 — vertical slice trên PC + Android
- [ ] Giai đoạn 3: 3.1 · 3.2 · 3.3 · 3.4 · 3.5 · 3.6 · 3.7 · 3.8
- [ ] Giai đoạn 4: 4.1 · 4.2 · 4.3 · 4.4 · 4.5
- [ ] Giai đoạn 5: 5.1 · 5.2 · 5.3
- [ ] Giai đoạn 6: 6.1 · 6.2 · 6.3
