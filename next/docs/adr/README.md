# ADR — những quyết định không nên phải hỏi lại

Mỗi file là **một quyết định**: bối cảnh (vì sao phải quyết), quyết định, phương án đã cân nhắc,
hệ quả. Ngắn, một trang. Khi một quyết định bị thay, không xoá file cũ — thêm file mới và đánh dấu
file cũ là *thay thế bởi ADR-xxx*, để sau này còn đọc được vì sao từng làm như vậy.

| ADR | Nội dung | Trạng thái |
|---|---|---|
| [ADR-001](ADR-001-kien-truc.md) | Kiến trúc: lõi C++, dịch vụ Go, client Godot, script Lua 5.4 | đã chốt |
| [ADR-002](ADR-002-giao-thuc.md) | Protocol V2: khung tin, protobuf, đường truyền, phiên bản | đã chốt |
| [ADR-003](ADR-003-du-lieu-nhan-vat.md) | `RoleData`: id, `data_version`, chuỗi di trú | đã chốt |
| [ADR-004](ADR-004-tai-khoan-phien.md) | Tài khoản argon2id, một tài khoản một phiên, bảo vệ đường vào | đã chốt |
| [ADR-005](ADR-005-gameserver-nhieu-nhan.md) | GameServer nhiều nhân: chủ sở hữu, hàng lệnh, worker pool, scheduler | đã chốt |
| [ADR-006](ADR-006-lua54-khong-tuong-thich-lua4.md) | Script là Lua 5.4 thật, không có lớp tương thích Lua 4 | đã chốt |

Bàn giao và lịch trình tới khi hoàn thiện: [HANDOVER.md](../HANDOVER.md).

Tài liệu kỹ thuật đi kèm: [PROTOCOL.md](../PROTOCOL.md), [MAPS.md](../MAPS.md),
[NPCRES.md](../NPCRES.md), [SCRIPTS.md](../SCRIPTS.md), [RUNNING.md](../RUNNING.md),
[TESTING.md](../TESTING.md), [LOGGING.md](../LOGGING.md), [OLD-TO-NEW.md](../OLD-TO-NEW.md),
[REFERENCES.md](../REFERENCES.md).
