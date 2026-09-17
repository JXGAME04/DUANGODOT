# ADR-001: Kiến trúc JX NEXT — lõi C++, dịch vụ Go, client Godot

- **Trạng thái**: đã chốt (2026-09-16), đang thực hiện
- **Liên quan**: [ADR-002](ADR-002-giao-thuc.md), [ADR-003](ADR-003-du-lieu-nhan-vat.md), [ADR-004](ADR-004-tai-khoan-phien.md)

## Bối cảnh

Bản cũ (SwordOnline/VLTK) là ~400 nghìn dòng C++ VC6, một cụm nhiều tiến trình Windows
(Heaven/Goddess/Bishop/Rainbow/PaySys) nối nhau bằng IOCP, script Lua 4.0, client DirectDraw 2D
32-bit. Nó chạy được và là **nguồn chân lý về luật chơi**, nhưng: chỉ Windows, chỉ 32-bit ở client,
struct gói tin đúc thẳng vào bộ nhớ, không test, không log có cấu trúc, không chạy nổi trên web/mobile.

Mục tiêu: dựng lại **cùng một game** trên nền hiện đại, 64-bit, đa nền tảng, có test và log từ dòng
code đầu tiên, giữ bản cũ làm oracle để đối chiếu.

## Quyết định

| Tầng | Chọn | Lý do |
|---|---|---|
| Lõi mô phỏng | **C++20** (`next/server/zone`), CMake + vcpkg, preset MSVC và GCC | port trực tiếp công thức/cấu trúc cũ (KNpc, KSubWorld, KNpcAI), hiệu năng, tick tất định |
| Dịch vụ xung quanh | **Go** (`next/services`): gateway, auth, persist, tool | TLS/WebSocket/DB/ops mạnh sẵn, ít C++ phải bảo trì, build tĩnh dễ triển khai |
| Script | **Lua 5.4** + lớp tương thích Lua 4 | script cũ chạy **nguyên văn**, không phải sửa 2.600 file |
| Client | **Godot 4.7**, GDScript | một mã nguồn ra Windows/Linux/Android/web, sprite/asset cũ nhập vào được |
| Giao thức | protobuf 3, sinh cho C++/Go/GDScript | xem ADR-002 |
| Log | JSON một dòng, có `sid`/`pid`/`tick` | truy một hành động xuyên mọi tiến trình bằng `grep` |
| Test | Catch2 (C++), `go test`, test headless Godot, e2e bằng `dev.py` | mọi phần có test ngay từ commit đầu |

**Biên giới**: zone không biết socket của client (chỉ biết `sid`); gateway không biết luật chơi;
client không tự quyết định gì (server authoritative).

## Phương án đã cân nhắc

- **Giữ nguyên C++ cho tất cả**: nhanh hơn về port, nhưng phải tự viết lại TLS/WebSocket/DB/ops và
  khó tuyển người bảo trì.
- **Viết lại toàn bộ bằng Go**: dịch vụ thì hợp, nhưng lõi mô phỏng phải dịch lại từng công thức từ
  C++ sang Go — mất chính lợi thế "1-1 với mã nguồn cũ" mà chủ dự án yêu cầu.
- **Unity/Unreal cho client**: nặng cho game 2D sprite, license phức tạp, không cần.

## Hệ quả

- Hai ngôn ngữ ở server: phải có một giao thức nội bộ rõ ràng giữa gateway và zone (ADR-002) và
  bảng đối chiếu tên file cũ → mới (`OLD-TO-NEW.md`) để người quen mã cũ tìm được chỗ.
- Mã sinh từ `.proto` cho ba ngôn ngữ **được commit**, để build không cần `protoc`.
- Mỗi phần mới đặt tên theo file/lớp cũ cùng chức năng (quy ước của chủ dự án).
- Bản cũ trong repo được giữ vĩnh viễn làm oracle; bộ ghi gói tin (`jxrecord`) thu bằng chứng từ nó.
