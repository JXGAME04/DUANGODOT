# Chuẩn test JX NEXT

Nguyên tắc: **không có phần nào được coi là xong nếu chưa có test tự chạy được trên CI**
(Windows + Linux). Test viết ngay cùng lúc với code, không để "làm sau".

## 1. Các tầng test

| Tầng | Công cụ | Chạy khi | Mục tiêu |
|---|---|---|---|
| Unit | Catch2 v3 (C++), `go test` (Go), GUT (Godot) | mỗi lần build | hàm/lớp riêng lẻ, < 1 s toàn bộ |
| Proto round-trip | Catch2 + Go test dùng cùng file `.bin` mẫu | mỗi lần build | encode ở C++ → decode ở Go và ngược lại, byte-exact |
| Golden replay | `.jxrec` ghi từ hệ thống cũ, chạy lại qua zone core | mỗi PR | cùng input → cùng trạng thái (checksum theo tick) |
| Formula compare | bảng số liệu xuất từ Core cũ (`.csv`) | khi động vào combat/skill/item | công thức mới ra đúng số của bản cũ |
| Integration | docker-compose: gateway + zone + postgres + redis | nightly / trước release | login → vào map → đánh quái → lưu DB |
| Fuzz codec | `go test -fuzz`, test ngẫu nhiên có seed cố định (C++/GDScript) | mỗi PR (seed corpus) | byte rác không làm sập, không sinh gói quá hạn |
| Load | tool Go bắn N client giả | trước release | 2 000 CCU/zone, p99 tick < 50 ms |
| Multi-platform | GitHub Actions matrix | mỗi PR | Windows MSVC + Linux GCC (client: export Windows/Linux/Android) |

## 2. Chạy tại máy

```bash
cd next
cmake --preset windows-msvc            # hoặc linux-gcc   (cần VCPKG_ROOT, chạy trong VS developer prompt trên Windows)
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug      # thêm -R log để chạy riêng nhóm [log]
```

Chạy trực tiếp binary để thấy output chi tiết: `build/windows-msvc/server/common/tests/Debug/jx_common_tests.exe "[clock]" -s`.

## 3. Quy ước viết test

- Mỗi module `X` có `tests/test_X.cpp`; tên `TEST_CASE` là **câu khẳng định hành vi**, tag `[X]`.
- Test phải **xác định** (deterministic): không phụ thuộc giờ máy, thứ tự thread, mạng thật.
  Thời gian → `ManualClock`/`FixedStep`; id → `IdGenerator(1)`; file → `temp_directory_path()/jxnext-test-<module>` và xoá sau.
- Test đọc log qua `jx::log::ring_snapshot()` khi cần khẳng định "đã log đúng field".
- Không test private; nếu phải test, thiết kế lại API.
- Test chạy được **đồng thời** trên CI: không dùng port cố định, không dùng file cố định.
- Fail phải nói rõ: dùng `CHECK` cho nhiều điều kiện độc lập, `REQUIRE` chỉ khi bước sau vô nghĩa nếu fail.

## 3b. Test gói tin hỏng (fuzz)

Server cũ đọc gói bằng cách ép con trỏ buffer sang struct: một byte độ dài sai là đọc tràn. Ở bản
mới, **byte rác chỉ được phép** cho ra khung tin hợp lệ, "cần thêm dữ liệu", hoặc lỗi — không bao
giờ sập, không bao giờ sinh payload vượt giới hạn. Ba chỗ kiểm cùng một hợp đồng:

| Nơi | Test | Chạy |
|---|---|---|
| Go | `pkg/frame/frame_fuzz_test.go` (`FuzzParser`, `FuzzReader`) | seed chạy trong `go test`; engine: `go test ./pkg/frame -run XXX -fuzz FuzzParser -fuzztime 30s` |
| C++ | `test_frame.cpp` `[fuzz]` (mt19937 seed 20260917, 500 vòng) | `ctest` |
| GDScript | `tests/run.gd` `test_frame_fuzz` (300 vòng) | test client headless |
| Gateway | `internal/gateway/KGarbage_test.go` | 40 kết nối gửi id/payload ngẫu nhiên; phiên đang chơi **không** bị ảnh hưởng |

Seed cố định nên lỗi tái hiện được. Khi fuzzer tìm ra input lỗi, Go ghi vào
`services/testdata/fuzz/...` — **commit file đó** để thành test hồi quy.

## 3c. Ghi gói tin của hệ thống cũ (`.jxrec`)

`jxrecord` đứng giữa client và server như một proxy TCP, ghi **mọi byte của cả hai chiều** kèm mốc
thời gian vào file `.jxrec`. Nó không hiểu nội dung nên ghi được cả bản cũ (luồng riêng của
Rainbow/Bishop) lẫn Protocol V2 — đây là **bằng chứng** để đối chiếu server mới về sau (golden
replay, mục 1 bảng trên).

```bash
# bản cũ: trỏ client vào 127.0.0.1:7100 thay vì server thật
build/go/jxrecord proxy -listen 127.0.0.1:7100 -to 203.0.113.9:5600 -out logs/old-login.jxrec -note "dang nhap"

# xem lại (thêm -jx để giải mã khung tin mới, -hex để xem byte)
build/go/jxrecord dump -jx logs/old-login.jxrec
```

Định dạng (`services/pkg/jxrec`): `"JXREC1\n"` + một dòng JSON header + các bản ghi
`dir(1) | ms(4) | len(4) | bytes`. File bị cắt giữa chừng vẫn đọc được đến bản ghi cuối còn nguyên
(recorder flush mỗi giây). Một file cho mỗi kết nối.

## 3d. Test tải (MASTER SPEC 55–58)

```bash
python tools/dev.py load 500  30  hot                 # 500 client dồn vào một chỗ (Tống Kim)
python tools/dev.py load 5000 60  spread 40           # 5000 client trên 40 map đông nhất
python tools/dev.py load 10000 120 spread 80 2        # 10 000 client, 80 map, 2 gateway trước một zone
```

Lệnh này **tự chuẩn bị dữ liệu**: `jxaccount seed` tạo N tài khoản, mỗi tài khoản một nhân vật, rải đều
trên danh sách map đông nhất (map của nhân vật nằm trong vị trí đã lưu nên zone tự đặt người chơi vào
đúng map đó). Bot **vào dần** (`-ramp`, khoảng 100 lượt đăng nhập/giây vì argon2 cố tình đắt) và **rời
cùng một mốc**, nếu không thì người đầu đã thoát khi người cuối mới vào và đỉnh không bao giờ đạt.
Tất cả bot đều **đánh nhau** (`-attack`). Vật phẩm chưa có nên chưa nằm trong phép đo.

Cuối mỗi lượt, lệnh in **đúng những gì zone và gateway đo được** tại ba thời điểm: lúc đông nhất, lúc
p99 xấu nhất, và lúc kết thúc. Không tuyên bố con số người chơi tối đa khi chưa đo (§55).

### Đo được trên máy 24 luồng (i7-13700K, 32 GB, 18 Hz, ngân sách 55 ms)

| Kịch bản | người chơi | tick tb | p95 | p99 | RSS zone | rớt |
|---|---:|---:|---:|---:|---:|---:|
| 1 người, 3076 entity, 4 map | 1 | 1,37 ms | 10,5 | 10,5 | | 0 |
| 200 cùng một chỗ | 200 | 3,35 ms | 5,2 | 7,3 | | 0 |
| 500 cùng một chỗ | 500 | 14,05 ms | 25,2 | 29,0 | | 0 |
| **5000 trên 40 map, 1 gateway** | **5000** | **8,08 ms** | **14,68** | **19,85** | 1353 MB | **0** |
| **10 000 trên 80 map, 1 gateway** | **9 995** | **8,56 ms** | **16,78** | **16,78** | 1374 MB | **0** |
| **20 000 bot, 120 map, 1 gateway** | **13 031** | **10,20 ms** | **14,68** | **20,13** | 1956 MB | **0** |

Ở lượt 5000 người: 980 map và 115 734 entity trong zone, gateway đẩy 670 738 gói/giây (22,6 MB/giây),
**không ai rớt, không ai bị đá, không gói nào bị bỏ, không lần đăng nhập nào hỏng**.

Ở lượt **10 000 người trên một zone và một gateway**: cả 10 000 đều vào được thế giới (0 hỏng),
98 351 entity, gateway đẩy **1 526 124 gói/giây (53,2 MB/giây)** qua 240 506 lần ghi socket; bot nhận
93 triệu gói hành động và 36 triệu gói di chuyển trong 220 giây. **0 bị đá, 0 timeout, 2 khung bị bỏ.**
Vào thế giới trung bình 1,9 giây (trước khi sửa ba chỗ nghẽn dưới đây là 8,0 giây).

Một điểm phải nói rõ: **lúc 10 000 người thoát cùng một lúc**, tick vọt lên 22,6 ms trung bình và
p99 100,7 ms — vượt ngân sách 55 ms trong vài tick. Đó là kịch bản test (mọi bot rời cùng một mốc),
không phải của người chơi thật, nhưng rời thế giới hàng loạt vẫn là việc cần rải ra nhiều tick — chưa làm.

### Ba chỗ nghẽn tìm được bằng đo, không phải bằng đoán

1. **Mỗi gói một syscall ghi socket.** pprof chỉ ra 69 % CPU của gateway nằm trong
   `internal/poll.(*FD).Write`. Gom lại mỗi vòng một lần ghi: 726 000 gói/giây còn ~100 000 lần ghi,
   đúng một lần mỗi phiên mỗi tick. CPU gateway **950 → 442 giây**. Bật pprof:
   `"gateway": { "pprof": "127.0.0.1:17199" }`.
2. **Tra phiên bằng mutex chung.** Zone gửi một gói kèm danh sách phiên nhận; mỗi người nhận là một
   lượt khoá — 46 triệu lượt trong hai phút. `KSessionTable` tra không khoá (mảng chia khối theo id).
3. **Đường truyền zone → gateway tắc đầu hàng.** Đo được **49 MB** tồn đọng ở 4000 người chơi, và
   **vào thế giới mất trung bình 8 giây**. Ba sửa: `Connection` ghi gộp nhiều khung một lần
   (scatter/gather), gói điều khiển có hàng ưu tiên riêng (`send_urgent`), và khi một đường truyền
   đã tồn đọng quá 4 MiB thì **bỏ gói vị trí** (chỉ vị trí, không bao giờ bỏ spawn/sát thương/chat).
   Tồn đọng 49 MB → ~150 KB, vào thế giới 8,0 → 2,8 giây.

### Một map đông người: trần là ~1 800, và chi phí tăng theo bình phương

Dồn tất cả vào **một map** (`dev.py load N 60 hot 1 1`, Phượng Tường):

| Người trên một map | Worker giữ map đó | tick tb | p95 | p99 | tick bị rớt | gateway đẩy ra |
|---:|---:|---:|---:|---:|---:|---:|
| 500 | | 14,05 ms | 25,2 | 29,0 | 0 | |
| 1 846 | 4,85 ms | 11,26 ms | 33,6 | 100,7 | 0 | 1 058 242 gói/s, 56,3 MB/s |
| **2 978** | **28,38 ms** | **41,45 ms** | **201,3** | **325,6** | **13** | 616 698 gói/s, 37,9 MB/s |

Ba điều ba lượt này nói ra:

1. **Thêm nhân không cứu được.** Cả ba lượt, đúng một worker gánh map đó còn worker kia nằm không
   (`w1:28.38ms/1maps/2978p  w0:0.01ms/1maps/0p`). Đó là hệ quả trực tiếp của luật một map một chủ
   (SPEC 6): đổi lại là trong tick không cần một khoá nào.
2. **Chi phí tăng nhanh hơn số người rất nhiều.** Từ 1 846 lên 2 978 người (1,6 lần) thì chi phí
   của map đi từ 4,85 lên 28,38 ms (**5,9 lần**). Lý do: trong đám đông dày, gần như ai cũng nhìn thấy
   ai, nên số cặp "người thấy người" tăng theo bình phương.
3. **Ở 2 978 người, server bắt đầu bỏ tick**: 13 tick bị rớt hẳn. Đây là ngưỡng hỏng thật sự, không
   phải chỉ là chậm.

**Trần thực tế của một map trên phần cứng này: khoảng 1 500–1 800 người.** Muốn đông hơn thì cần hai
việc, và việc thứ hai mới là việc chính:

- **Chia vùng trong một map** (giai đoạn R): cắt map nóng thành nhiều mảnh, mỗi mảnh một worker, chỉ
  đồng bộ ở đường biên.
- **Giảm số gói theo tầm nhìn**: người ở xa thì gửi thưa hơn và gộp nhiều entity vào một gói, thay vì
  mỗi hành động một gói tới mọi người trong tầm. Không làm việc này thì chia vùng cũng vô ích: ở 1 846
  người một chỗ, mô phỏng chỉ tốn 4,85 ms trong khi gateway phải đẩy 56,3 MB mỗi giây.

### 20 000 bot: chỗ chặn là **máy test hết cổng TCP**, không phải server

Bắn 20 000 bot thì **13 031** vào được cùng lúc, zone vẫn thảnh thơi: tick trung bình 10,20 ms, p99
20,13 ms trong ngân sách 55 ms, không ai bị đá, không timeout, không gói nào bị bỏ. 6 969 bot hỏng, và lý do
đọc được ngay trong log của bot:

| Lý do | Số bot |
|---|---:|
| `Only one usage of each socket address ... is normally permitted` (hết cổng động) | 3 665 |
| hết giờ chờ `G2C_ENTER_WORLD_RES` | 3 304 |

Windows mặc định chỉ có **16 384 cổng động** (49152–65535) cho mọi kết nối đi ra, đo lúc đó còn
10 540 cổng đang ở TIME_WAIT:

```
netsh int ipv4 show dynamicport tcp     ->  Start 49152, Number of Ports 16384
```

20 000 kết nối đồng thời từ **một máy** là không thể, dù server có rảnh đến đâu. Muốn đo thật 20 000
thì phải chọn một trong hai:

- nới dải cổng động của máy test:
  `netsh int ipv4 set dynamicport tcp start=10000 num=55000` (đổi cấu hình hệ điều hành, cần quyền admin);
- hoặc bắn bot từ **máy thứ hai** — cách đúng đắn hơn, vì bot và server đang tranh nhau cùng 24 luồng.

Còn 3 304 bot hết giờ chờ vào thế giới là vấn đề thật của server: **một gateway không rút kịp**. Đo được
**356 MB** tồn đọng trên đường truyền zone → gateway ở 13 878 người: cơ chế bỏ bớt hiện chỉ bỏ gói
vị trí, còn gói đánh nhau và gói máu vẫn dồn lại. Từ ~10 000 người trở lên cần **nhiều gateway**, và
đó chính là lý do có `ZoneHelloAck.session_prefix`.

### Hai lỗi đúng thật chỉ lộ ra ở mức này

- **Hai gateway đánh trùng số phiên.** Mỗi gateway đánh số phiên từ 1, nên zone thấy phiên 1 của
  gateway thứ hai là phiên 1 của gateway thứ nhất và **ghi đè im lặng**; đo với 10 000 client thì một nửa
  không bao giờ vào được thế giới. Zone giờ cấp cho mỗi đường truyền một tiền tố riêng
  (`ZoneHelloAck.session_prefix`).
- **Bảng phiên cấp phát 1 TiB.** Sau khi id phiên mang tiền tố ở bit cao (~2⁴⁹), `KSessionTable` vẫn
  dùng id làm chỉ số mảng: Go xin 1 TiB và gateway chết ngay ở client đầu tiên. Giờ bảng đánh chỉ số
  theo 48 bit thấp và đối chiếu lại id đầy đủ. Cả hai đều có test.

Lần đầu gateway chết **không để lại gì** vì `dev.py` không hứng stderr của server. Giờ mọi server
ghi stderr vào `logs/<tên>.console.log`, nên một cú panic hay một lần hệ điều hành từ chối cấp phát
đều có dấu vết.

### Còn lại

- **Trên 5000 người chưa đo được trên máy này.** Bot, zone và gateway chạy cùng một máy, bên cạnh
  `GameServer` của bản cũ đang giữ 8,6 GB: ở 10 000 client, Windows từ chối tạo tiến trình mới
  ("the paging file is too small"). Đây là giới hạn của **máy test**, không phải của server: ở 5000
  người, gateway mới dùng ~4 nhân trong 24 và zone ~0,7 nhân. Muốn đo 10 000–20 000 thì phải bắn
  bot từ máy khác.
- Một map = một worker: **một map đông** vẫn bị giới hạn ở một nhân (500 người cùng chỗ = 14 ms).
  Chia vùng cho map nóng là giai đoạn R của MASTER SPEC, chưa làm.
- Vật phẩm chưa có nên chưa nằm trong phép đo.

## 3e. Script Lua 5.4

Không còn lớp tương thích Lua 4: script phải là Lua 5.4 thật ([SCRIPTS.md](SCRIPTS.md),
[ADR-006](adr/ADR-006-lua54-khong-tuong-thich-lua4.md)). Ba lớp kiểm:

```bash
python tools/dev.py lua                    # chuyển + nạp thử toàn bộ cây bằng Lua 5.4
build/go/jxlua check -in data/script/server1/script    # chạy lại phải ra 0 thay đổi
```

| Nơi | Test | Khẳng định |
|---|---|---|
| Go | `pkg/jxlua/KLuaConvert_test.go` | 39 test, mỗi phép biến đổi một test, ví dụ lấy từ dòng thật của script gốc |
| C++ | `test_KLuaScript.cpp` `[lua]` | script đã chuyển chạy; script Lua 4 **bị từ chối**; `getn`/`strfind`/`floor` không còn |
| Cây thật | `jx_luacheck` | 7 663 file, 22,6 MB, **0 file lỗi cú pháp** |

## 4. Definition of Done cho một phần việc

1. Code + log theo `docs/LOGGING.md`.
2. Test unit xanh trên **cả hai** job CI.
3. Nếu là thay đổi giao thức: test round-trip C++ ↔ Go và file mẫu `.bin` được cập nhật.
4. Nếu là logic gameplay: golden replay hoặc formula-compare có case cho phần đó.
5. 5 dòng ghi chú trong `docs/` (mục đích, API chính, cách cấu hình, cách test, điều chưa làm).
6. Báo "Xong: <phần> — bạn kiểm tra" kèm lệnh chạy.

## 5. CI

`.github/workflows/next-ci.yml` chạy khi có thay đổi trong `next/**`: configure → build Debug →
ctest → build Release → ctest, trên `windows-latest` (MSVC) và `ubuntu-24.04` (GCC). vcpkg dùng
đúng commit `builtin-baseline` trong `vcpkg.json`; binary cache lưu bằng `actions/cache` nên lần
chạy sau chỉ mất vài phút. Khi fail, log configure/ctest được đính kèm ở mục *Artifacts* của run.
