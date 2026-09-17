# ADR-003: Dữ liệu nhân vật — `RoleData`, id, phiên bản và di trú

- **Trạng thái**: đã chốt (2026-09-17, M4b)
- **Mã**: `proto/jx/role.proto`, `services/pkg/persist/TRoleData.go`

## Bối cảnh

Bản cũ lưu nhân vật bằng `TRoleData` — một struct C đổ thẳng xuống Goddess/BDB, **không có số phiên
bản**. Thêm một trường vào struct là mọi nhân vật đã lưu bị hiểu sai từ chỗ đó trở đi; một server cũ
ghi đè bản ghi do server mới tạo sẽ xoá sạch trường mới. Đây là nguồn mất dữ liệu kinh điển, và theo
thứ tự ưu tiên của lộ trình (mục 2b số 4) phải làm đúng **trước** khi có nhiều hệ thống gameplay.

## Quyết định

1. **Một mô hình duy nhất**: `jx.pb.RoleData` (protobuf). Persist lưu nó, zone nạp/lưu nó, client
   nhận bản chiếu của nó (`CharSummary`, `EntityInfo`). Không có struct nhân vật thứ hai.
2. **Hai id, hai vai trò**: `player_id` do persist cấp, **vĩnh viễn**; `entity_id` do zone cấp, chỉ
   sống trong một phiên; `sid` do gateway cấp, chỉ sống trong đời tiến trình gateway.
3. **`data_version` bắt buộc**: `persist.CurrentRoleVersion` là dạng bản ghi mà server đang chạy ghi ra.
   - bản ghi **cũ hơn** → `MigrateRole` nâng cấp một lần lúc mở store, rồi **ghi lại** ngay;
   - bản ghi **mới hơn** → `ErrNewerData`, từ chối nạp và từ chối ghi đè (không bao giờ hạ cấp);
   - thêm trường protobuf tuỳ chọn thì **không** cần tăng version; đổi ý nghĩa một trường, hoặc sửa
     dữ liệu đã lưu, thì phải thêm một bước di trú.
4. **Bước di trú là code, không phải script tay**: mỗi bước là một entry trong `roleMigrations`
   `{version, what, apply}`, chạy tuần tự, có test riêng.
5. **Lưu trữ hiện tại**: `FileStore` — `accounts.json` + `chars/<player_id>.json` (protojson, đọc và
   so sánh được bằng mắt), ghi nguyên tử (file tạm + rename). PostgreSQL sẽ cài cùng interface
   `persist.Store`, không đụng mô hình.

## Phương án đã cân nhắc

- **Không đánh version, dựa vào "protobuf tự tương thích"**: đúng cho việc *thêm* trường, nhưng không
  cứu được khi phải *sửa* dữ liệu đã lưu, và không chặn được server cũ ghi đè bản mới.
- **Di trú bằng script chạy tay khi nâng cấp**: dễ quên, và không chạy được trong test.
- **Bản ghi chỉ ghi thêm (event sourcing)**: mạnh nhưng nặng hơn nhiều so với nhu cầu hiện tại.

## Hệ quả

- Mọi lần đổi ý nghĩa dữ liệu nhân vật: tăng `CurrentRoleVersion`, thêm một bước di trú **và** một test.
- Khi chạy lùi về bản server cũ, nhân vật đã nâng cấp sẽ **không** nạp được — đúng như mong muốn: mất
  dữ liệu im lặng tệ hơn là dừng lại và báo lỗi.
- Zone gửi `PlayerSave` với đúng `data_version` mà nó nhận; gateway kiểm tra lần nữa trước khi ghi.
