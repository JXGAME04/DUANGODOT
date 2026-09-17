# ADR-007: Tuổi thọ công nghệ — mã nguồn dùng được 5–10 năm không phải đập đi

- **Trạng thái**: đã chốt (2026-09-17), có bảng việc kèm theo
- **Liên quan**: [ADR-001](ADR-001-kien-truc.md), [ADR-002](ADR-002-giao-thuc.md), [ADR-006](ADR-006-lua54-khong-tuong-thich-lua4.md)

## Bối cảnh

Chủ dự án yêu cầu: "nâng cấp công nghệ tốt nhất để có một mã nguồn có thể dùng từ 5–10 năm sau không
cần nâng cấp nữa". Bản cũ là bài học ngược: VC6 + DirectDraw + struct gói tin đúc thẳng vào bộ nhớ +
Lua 4.0 — mỗi mảnh đều là thứ đã chết trước 2010, nên tới 2026 muốn sửa một dòng cũng phải dựng lại
cả môi trường 1999.

"Không cần nâng cấp nữa" không có nghĩa là chọn được phần mềm bất tử — không có phần mềm nào như thế.
Nghĩa thật là ba điều đo được:

1. mọi thứ ta phụ thuộc còn **được bảo trì và tương thích ngược** trong khoảng ấy;
2. **phiên bản được ghim** và build lại được y hệt sau nhiều năm, không phụ thuộc "bản mới nhất" của
   mạng;
3. **mã của ta không dính vào** thứ gì có hạn dùng: API riêng của một hệ điều hành, một trình biên
   dịch, một phiên bản engine, một cách xếp byte trong bộ nhớ.

## Quyết định — từng tầng, và vì sao tin được

| Tầng | Đang dùng (ghim) | Vì sao sống được 10 năm | Điều ta tự ràng buộc |
|---|---|---|---|
| Ngôn ngữ lõi | **C++20** (`CMAKE_CXX_STANDARD 20`, `REQUIRED ON`) | Chuẩn ISO; C++11 (2011) vẫn biên dịch nguyên vẹn năm 2026 trên mọi trình biên dịch. C++20 đã đủ chín (MSVC 2022, GCC 12+, Clang 15+). | Không dùng phần mở rộng riêng của MSVC hay GCC; **CI biên dịch cả hai** (MSVC + GCC, `-Werror`), nên mã không thể lệ thuộc một bên. Chỉ 64-bit. |
| Build C++ | **CMake ≥ 3.25** + preset; **vcpkg ghim commit** (`builtin-baseline 3855d4d6…`), Lua ghim `5.4.8` | CMake là chuẩn de facto từ 2010; vcpkg baseline = nguồn của mọi thư viện được đóng băng theo commit, build lại 5 năm sau ra đúng bản ấy. | Không thêm thư viện ngoài vcpkg; mỗi lần đổi baseline là một commit riêng, chạy đủ test. |
| Thư viện C++ | fmt, spdlog, nlohmann-json, asio (không Boost), protobuf, Catch2 | Đều là thư viện chuẩn de facto, header-only hoặc tự chứa, không kéo theo framework; fmt/format và asio/networking là hình mẫu của chuẩn C++. | Ranh giới mỏng: log qua `jx::log`, mạng qua `net::`, JSON qua `Config` — đổi thư viện bên dưới không chạm tới game. |
| Dịch vụ | **Go 1.26** (`go.mod`), phụ thuộc: protobuf, x/crypto, x/text | Go có **lời hứa tương thích ngược** chính thức (Go 1 compatibility promise, 2012 → nay không gãy); build tĩnh, một tệp thực thi, không runtime rời. | `go.mod` ghim mọi phiên bản; không dùng framework web; chỉ thư viện chuẩn + `golang.org/x`. |
| Giao thức | **protobuf 3** (`proto/jx/*.proto`), khung `len/msg/flags/payload` little-endian | Định dạng dây ổn định từ 2008, thêm trường không gãy client cũ; có trình sinh cho mọi ngôn ngữ, kể cả GDScript (godobuf). | Không bao giờ đúc struct vào bộ nhớ; mọi thay đổi giao thức là **thêm trường**, không đổi số thứ tự (ADR-002). |
| Script | **Lua 5.4** (ghim 5.4.8), không lớp tương thích Lua 4 (ADR-006) | Lua 5.4 ra 2020, dòng 5.x ổn định 20 năm, nhúng bằng C API không đổi. | Script cũ được **chuyển đổi một lần** thành Lua 5.4 thật (`dev.py lua`), không kéo dài tuổi thọ Lua 4. |
| Client | **Godot 4.7** GDScript, renderer "GL Compatibility" | Mã nguồn mở MIT, không phụ thuộc nhà cung cấp, ra Windows/Linux/Android/web từ một mã. Nhánh 4.x có LTS. | Chỉ dùng API Godot ổn định (không addon C++), asset là JSON + PNG + `.fnt` do ta xuất — engine đổi thì dữ liệu vẫn nguyên. |
| Dữ liệu | JSON có schema (map, sprite, UI), protobuf cho nhân vật, PostgreSQL (M9) | Định dạng văn bản, mở, đọc được bằng mắt; DB quan hệ chuẩn SQL, 30 năm. | Không dùng định dạng nhị phân riêng cho dữ liệu lâu dài; mọi bảng có `schema` và validator trong CI (D1). |
| Bảng mã | **UTF-8 mọi nơi** (log, config, tên nhân vật, console Windows chuyển `CP_UTF8`) | Bản cũ chết vì GBK/TCVN3 trộn lẫn. | Chuyển mã chỉ ở một chỗ: `pkg/jxold/text` khi đọc dữ liệu cũ. |
| Hệ điều hành | Windows + Linux, cùng một mã (CI cả hai) | API riêng của nền tảng chỉ ở 6 tệp có `#ifdef _WIN32` (môi trường tiến trình, console UTF-8, tín hiệu dừng, `process_usage`, mở tệp theo đường dẫn Unicode, `_WIN32_WINNT` cho asio). | Mỗi `#ifdef` phải có bản Linux tương đương và test chạy trên cả hai. |

## Những gì cố ý **không** chọn

- Framework mạng/game "hot" nào đó (ENet fork, Unreal/Unity cho server): vòng đời ngắn hơn game.
- Boost: to, thay đổi chậm nhưng kéo theo cả kho; asio độc lập là đủ.
- ORM và thư viện DB riêng: SQL thuần qua driver chuẩn (`pgx` khi làm M9).
- Định dạng nhị phân tự chế cho map/sprite: đã có `.pak` + `.spr` cũ làm bài học — mọi thứ xuất ra JSON/PNG.
- Lớp tương thích Lua 4 mãi mãi: chuyển đổi một lần rồi bỏ (ADR-006).

## Chính sách nâng cấp (để "không cần nâng cấp" thành "nâng cấp là chuyện nhỏ")

1. **Ghim rồi mới dùng.** Không có phụ thuộc nào trỏ vào "latest": vcpkg baseline, `go.mod`,
   `GODOT_VERSION` trong CI, Lua override.
2. **Mỗi năm một lần** (hoặc khi có lỗ hổng bảo mật): dời baseline vcpkg, `go get -u`, Godot nhánh
   LTS mới — trong **một commit riêng**, CI xanh cả Windows lẫn Linux, load test `dev.py load` không xấu
   đi. Việc này tốn một ngày vì mọi test đã có.
3. **Không bao giờ** làm hai việc trong một commit nâng cấp (đổi thư viện + đổi luật chơi).
4. Trình biên dịch mới cảnh báo gì thì sửa ngay (`-Werror` đang bật): mã luôn sạch với chuẩn mới nhất.
5. Ghi lại lý do mỗi lần ghim/nâng ở ADR này (bảng dưới).

## Bảng việc (trạng thái 2026-09-17)

| # | Việc | Trạng thái |
|---|---|---|
| T1 | C++20, `-Werror` / `/WX` (`jx_warnings`), CI MSVC + GCC | **xong** (CI đủ hai bên từ 43a7d8c) |
| T2 | vcpkg baseline ghim, Lua ghim 5.4.8 | **xong** |
| T3 | `go.mod` ghim, không framework | **xong** |
| T4 | protobuf 3, chỉ thêm trường (ADR-002) | **xong**, đã qua 3 lần thêm trường (native_place, EntityMoves…) không gãy client cũ |
| T5 | Lua 5.4 thuần, chuyển đổi một lần | **xong** (ADR-006) |
| T6 | Godot 4.7, asset JSON/PNG/`.fnt` do ta xuất | **xong** (jxassets export-ui) |
| T7 | UTF-8 mọi nơi, chuyển mã một chỗ | **xong** |
| T8 | `#ifdef` nền tảng có bản Linux + test | **xong** (e2e Linux trong CI) |
| T9 | Dữ liệu lâu dài trong PostgreSQL thay kho tệp | **chưa** — M9 (O1) |
| T10 | Schema + validator cho 62 bảng settings trong CI | **chưa** — D1 |
| T11 | Lịch nâng cấp hằng năm ghi trong HANDOVER | **chưa** — mục "vận hành" |
| T12 | Docker + sao lưu/khôi phục có test | **chưa** — O4 |

## Hệ quả

Cái giá của chính sách này là kỷ luật: không kéo thư viện tiện tay, không "nhanh cho xong" bằng API
riêng của Windows, và mỗi năm mất một ngày nâng phiên bản có kiểm chứng. Đổi lại, năm 2031 hay 2036,
người kế tiếp `git clone`, `cmake --preset`, `go build`, mở Godot — và mọi thứ còn biên dịch, còn chạy,
còn có test nói cho họ biết điều gì hỏng.
