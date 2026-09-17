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
python tools/dev.py load 500 30 hot      # 500 client dồn vào một chỗ (Tống Kim), 30 giây
python tools/dev.py load 200 30 spread   # 200 client đi khắp map
```

Lệnh này bật server, chạy `jxbot`, rồi in **đúng những gì zone đo được**: `tick_ms_avg/p95/p99`,
tải từng worker, chi phí từng pha của map nặng nhất, số entity đang thức. Không tuyên bố con số
người chơi tối đa khi chưa đo (§55).

Đo được trên máy 24 luồng (Phượng Tường + 3 map, 18 Hz, ngân sách 55 ms):

| Kịch bản | tick avg | p95 | p99 | ghi chú |
|---|---|---|---|---|
| 1 người chơi, 3076 entity | 1,37 ms | 10,5 | 10,5 | 42 entity thức |
| 200 người cùng một chỗ | 3,35 ms | 5,2 | 7,3 | |
| 500 người cùng một chỗ | 14,05 ms | 25,2 | 29,0 | không ai rớt, 7 triệu gói hành động/29 s |

Đăng nhập: mật khẩu argon2id tốn ~16 ms CPU mỗi lần **có chủ ý**, nên ~80 lượt đăng nhập/giây trên
máy này; 500 client vào cùng lúc xếp hàng khoảng 6 giây (client thật chờ, không lỗi).

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
