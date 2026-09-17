# ADR-004: Tài khoản và phiên — mật khẩu, một phiên một tài khoản, bảo vệ đường vào

- **Trạng thái**: đã chốt (2026-09-17, M4a)
- **Mã**: `services/pkg/auth`, `services/internal/gateway`, `services/cmd/jxaccount`
- **Chi tiết vận hành**: [RUNNING.md](../RUNNING.md) mục 3b/3c, [PROTOCOL.md](../PROTOCOL.md) mục 3b

## Bối cảnh

Bản cũ: `Sword3PaySys` giữ tài khoản trong SQL Server với mật khẩu dạng có thể so sánh trực tiếp,
`S3PAccount::Login` đánh dấu "đang online" bằng cột `iClientID` — client crash là tài khoản **kẹt
online** cho tới khi GM gỡ. Bishop không có giới hạn số gói, không có heartbeat: một kết nối chết
nằm lại cho tới khi TCP tự bỏ, một client lỗi có thể bơm gói vô hạn. Tắt server là mất tiến độ từ
lần lưu định kỳ gần nhất.

## Quyết định

1. **Mật khẩu chỉ lưu dạng băm argon2id** (19 MiB, 2 vòng, 1 luồng — tham số OWASP 2024), định dạng
   PHC. Mật khẩu thường của các bản dev đầu được băm lại và xoá ngay khi gateway khởi động.
2. **Cổng tài khoản là một interface** (`auth.Authenticator`) với hai chế độ:
   `dev` (đăng nhập lần đầu tự tạo tài khoản — cho test/bot) và `strict` (chỉ tài khoản tạo bằng
   `jxaccount`). Luật nghiệp vụ port từ `S3PAccount::Login`: sai mật khẩu, tài khoản bị khoá
   (`frozen`), hết thời gian chơi (`expires_at_ms`) — **cộng thêm** khoá tạm sau N lần sai liên tiếp.
3. **Mã kết quả giữ nguyên ngữ nghĩa cũ** (`LOGIN_R_*`): client dịch sang đúng câu thông báo của
   client cũ (`client/net/KLogin.gd`), nên người chơi thấy thông tin quen thuộc.
4. **Một tài khoản = một phiên sống**. Mặc định **lần đăng nhập mới thắng**: phiên cũ nhận
   `Kick{REPLACED}`, zone nhận `SessionClose{reason=2}` — điều này cũng tự gỡ tài khoản kẹt sau khi
   client crash. Ai muốn luật cũ thì bật `refuse_duplicate_login` (trả `ACCOUNT_IN_USE`).
5. **Phiên có thời hạn im lặng theo trạng thái**: chờ `Hello` 10 s, lobby 300 s, trong game 30 s
   (client ping 5 s và tự ngắt nếu 15 s không có `Pong`). Số gói mỗi giây có token bucket; vượt là đá.
6. **Tắt máy êm**: gateway đá mọi client, gửi `SessionClose{reason=3}`, rồi **chờ `PlayerSave` cuối
   của zone** (`shutdown_wait_s`) trước khi thoát.
7. **Đo được**: đếm phiên/đăng nhập/bị đá/gói/byte, một dòng `cat=gw.stats` mỗi 30 giây và
   `Server.Snapshot()` cho Prometheus sau này.

## Phương án đã cân nhắc

- **Giữ luật cũ "đang online thì cấm đăng nhập"** làm mặc định: đúng về mặt chống chia sẻ tài khoản,
  nhưng tài khoản kẹt sau crash là lỗi người chơi gặp thường xuyên nhất của bản cũ. Vẫn giữ lại làm
  tuỳ chọn.
- **bcrypt/scrypt**: chấp nhận được, nhưng argon2id là khuyến nghị hiện hành và có sẵn trong
  `golang.org/x/crypto`.
- **Token/OAuth ngay từ đầu**: chưa cần khi chưa có web đăng ký; `Authenticator` đã đủ chỗ để thêm.

## Hệ quả

- `persist.Store` phải có `CreateAccount`/`UpdateAccount`/`AccountByID`/`Accounts` (bỏ `EnsureAccount`).
- Mọi thao tác quản trị tài khoản đi qua `jxaccount` (add/passwd/freeze/unfreeze/expire/list), chạy
  khi gateway đang tắt vì cùng ghi `accounts.json`.
- Khi thêm hệ thống mới, message mới phải khai báo trạng thái phiên được phép gửi (bảng trong
  `session.go`), nếu không sẽ bị từ chối — đây là chủ ý.
