# ADR-006 — Script là Lua 5.4 thật, không có lớp tương thích Lua 4

**Trạng thái:** đã chốt (2026-09-17)

## Bối cảnh

Server cũ nhúng Lua 4.0 (`Sources/Library/LuaLib`, chuỗi phiên bản trong `lualibdll.dll` của bản
Linux xác nhận đúng 4.0). 7 663 file script được viết theo phương ngữ đó: `%upvalue`, `getn`,
`strfind`, `floor`, `mod`, `arg`, `for k,v in t`.

Cho tới nay zone chạy chúng nguyên trạng nhờ một đoạn mở đầu gán `getn = function(t) return #t end`,
`floor = math.floor`… vào biến toàn cục của Lua 5.4. Cách đó chạy được, nhưng:

- Người đọc file script thấy một phương ngữ **không tồn tại** ở đâu cả: không phải Lua 4 (vì máy
  chạy Lua 5.4), không phải Lua 5.4 (vì `getn` không có trong Lua 5.4).
- Lớp gán đó chỉ đúng gần đúng. `mod` của Lua 4 là `fmod` của C, `%` của Lua 5 là chia dư làm tròn
  xuống; hai cái khác nhau với số âm. `getn` không bằng `#` khi bảng có lỗ.
- Nó không cứu được những khác biệt về **cú pháp**: `%upvalue`, escape lạ, `in` làm tên biến,
  `1and`, `{a,; b}`. Những file đó im lặng hỏng.
- Người mới viết script sẽ bắt chước phương ngữ cũ, và lớp tương thích sẽ phải sống mãi.

## Quyết định

Chuyển một lần toàn bộ cây script sang **Lua 5.4 thật**, rồi **xoá** lớp tương thích.

- Bộ chuyển: `services/pkg/jxlua` + `services/cmd/jxlua`, chạy qua `python tools/dev.py lua`.
- Chuyển theo **byte**, không chuyển mã: cây script trộn GBK và TCVN3 trong cùng file, mọi phép
  chuyển mã đều hỏng một trong hai. Chỉ sửa ASCII nằm ngoài chuỗi và chú thích.
- Luật chuyển bám theo **mã nguồn Lua 4 của chính game** (`llex.c`, `lparser.c`), không theo tài
  liệu Lua 4 chung — bản nhúng này đã bị sửa (bỏ `in` khỏi từ dành riêng).
- Kết quả đi vào `data/script/` (đã gitignore). Cây gốc là tham chiếu, không sửa.
- `jx_luacheck` nạp mọi file bằng chính Lua 5.4 mà zone nhúng; đây là điều kiện nghiệm thu.

## Phương án đã cân nhắc

| Phương án | Vì sao không chọn |
|---|---|
| Giữ lớp tương thích | Giữ mãi một phương ngữ không có thật, và không cứu được khác biệt cú pháp |
| Nhúng lại Lua 4.0 | Đi ngược ADR-001; mất coroutine, `pcall` chuẩn, GC mới, 64-bit an toàn, và mọi công cụ Lua hiện đại |
| Chuyển lúc nạp (ngay trong zone) | Trả giá ở mỗi lần nạp, và lỗi chỉ lộ ra khi chạy thay vì lúc build |
| Chuyển tay | 34 418 chỗ phải sửa |

## Hệ quả

- Zone chỉ đăng ký `Include`, `IncludeLib`, `print` cộng các hàm gameplay; không còn biến toàn cục
  giả nào.
- Script **chưa chuyển** sẽ bị từ chối — có test khẳng định điều đó
  (`test_KLuaScript.cpp`, "an unconverted Lua 4 script is rejected").
- Phải chạy `python tools/dev.py lua` một lần trên mỗi máy, như `dev.py assets`.
- Chín file mở khối `/* */` chưa từng nạp được trên server cũ (Lua 4 không có `/* */`); nay khối đó
  thành chú thích nên phần còn lại của file chạy được. Đây là **thay đổi hành vi có chủ ý**, ghi rõ
  ở [SCRIPTS.md](../SCRIPTS.md) mục 5.
- Chuỗi sinh ra giống server cũ **từng byte**: escape lạ được bỏ dấu `\` đúng như `llex.c` đã làm.
