# ADR-002: Protocol V2 — khung tin, protobuf, đường truyền, phiên bản

- **Trạng thái**: đã chốt (2026-09-16), khung tin và transport đã xong (M1, M4d)
- **Chi tiết kỹ thuật**: [PROTOCOL.md](../PROTOCOL.md)

## Bối cảnh

Bản cũ gửi **thẳng struct C** qua socket: `memcpy` vào buffer rồi ép con trỏ ở đầu kia. Hậu quả:
thêm một trường là vỡ tương thích client↔server; một byte độ dài sai là đọc tràn bộ nhớ; client
32-bit và server 64-bit phải giữ layout giống hệt nhau; không có số phiên bản nên không biết ai đang
nói chuyện với ai.

## Quyết định

1. **Khung tin**: `u32 len | u16 msg | u16 flags | payload`, little-endian, `len` đếm từ sau chính
   nó. Giới hạn payload: client↔gateway 64 KiB, gateway↔zone 4 MiB. Vượt hoặc `len < 4` → đóng kết nối.
   Cùng một bộ vector kiểm chứng cho C++, Go, GDScript (`docs/PROTOCOL.md` mục 1).
2. **Nội dung là protobuf 3**, định nghĩa trong `proto/jx/*.proto`, sinh mã cho ba ngôn ngữ. **Không
   bao giờ** đổi số tag, không dùng lại số đã xoá; thêm trường mới là tương thích.
3. **Dải id rõ ràng**: 1000–1999 client→gateway, 2000–2999 gateway→client, 9000–9999 gateway↔zone.
   Message đánh dấu `(zone)` được gateway chuyển nguyên vẹn, zone trả lời theo danh sách `sid`.
4. **Đường truyền tách khỏi giao thức**: cùng dòng khung tin chạy trên TCP thô, TLS, WebSocket,
   WebSocket-trong-TLS (`services/pkg/transport`). Client chọn bằng địa chỉ (`tls://`, `ws://`).
5. **Phiên bản**: `PROTOCOL_VERSION` trong `msg.proto`, kiểm ở `Hello` (client) và `ZoneHello`
   (zone); lệch thì `Kick{VERSION_MISMATCH}` chứ không đoán.
6. **Nguyên tắc thiết kế message**: client chỉ gửi **ý định** (`MoveReq{target}`), server quyết định;
   mọi thứ client cần để vẽ nằm trong `EntityInfo`/`EntityMove`; chuỗi luôn UTF-8; không có message
   "chung chung" kiểu `Command{string}`.

## Phương án đã cân nhắc

- **Giữ struct nhị phân cũ**: tương thích được với client cũ, nhưng kéo theo mọi điểm yếu ở trên và
  chặn đường client web/mobile. Bản cũ vẫn dùng được làm oracle qua `.jxrec`, không cần tương thích dây.
- **JSON**: dễ đọc nhưng tốn 3–5 lần băng thông cho luồng vị trí 18 Hz.
- **FlatBuffers/Cap'n Proto**: nhanh hơn khi giải mã, nhưng hệ sinh thái cho GDScript không có.

## Hệ quả

- Mọi thay đổi giao thức phải sửa `.proto` rồi chạy `tools/gen_proto.py`; mã sinh được commit.
- Byte rác **không được phép** làm sập: có test fuzz ở cả ba bên (TESTING.md mục 3b).
- Thêm đường truyền mới (QUIC…) sau này chỉ là thêm một `Listener`, không đụng vào session.
- Gói tin ghi lại được bằng `jxrecord` và đọc lại bằng `dump -jx` vì khung tin tự phân định.
