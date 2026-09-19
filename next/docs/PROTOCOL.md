# Protocol V2 (JX NEXT)

Một giao thức, ba ngôn ngữ: định nghĩa trong `proto/jx/*.proto` (protobuf 3), sinh mã cho
C++ (`jx::pb`, CMake `jx::proto`), Go (`services/pkg/jxpb`) và GDScript (`client/proto/*.gd`
qua godobuf). **Không sửa tay mã sinh; không đổi số tag; không dùng lại số đã xoá.**

## 1. Khung tin (frame) trên TCP

```text
+----------+----------+----------+------------------+
| u32 len  | u16 msg  | u16 flag | payload (len-4)  |   little-endian
+----------+----------+----------+------------------+
```

| Trường | Ý nghĩa |
|---|---|
| `len`  | số byte sau trường len = 4 (msg+flag) + kích thước payload |
| `msg`  | `jx.pb.MsgId` (`proto/jx/msg.proto`) |
| `flag` | bit0 = payload nén (dự phòng, phase 4), bit1 = mã hoá (dự phòng) |
| payload | protobuf của message tương ứng với `msg` |

Giới hạn payload: client→gateway **64 KiB**; gateway↔zone **4 MiB**. Vượt giới hạn hoặc `len < 4`
→ đóng kết nối và log `warn` với `cat=net`.

**Vector kiểm chứng** (cả ba bên phải cho đúng byte này):

```text
encode(msg=1001, flags=0, payload="hi")   -> 06 00 00 00 E9 03 00 00 68 69
encode(msg=0x1234, flags=3, payload="")   -> 04 00 00 00 34 12 03 00
```

Test: `server/common/tests/test_frame.cpp`, `services/pkg/frame/frame_test.go`, `client/tests/test_net.gd`.

## 1b. Đường truyền (transport)

Cùng một dòng khung tin ở mục 1, chạy trên bốn đường; mọi thứ phía trên (khung tin, protobuf,
trạng thái phiên) **giống hệt nhau**, chỉ khác cách mở kết nối:

| Địa chỉ client nhập | Đường | Dùng cho | Cổng dev |
|---|---|---|---|
| `host:port` hoặc `tcp://host:port` | TCP thô | client PC, bot, LAN | 19100 |
| `tls://host:port` | TCP trong TLS 1.2+ | server công khai | 19100 (khi bật chứng chỉ) |
| `ws://host:port/ws` | WebSocket (RFC 6455) | bản web, mobile, sau proxy công ty | 19102 |
| `wss://host:port/ws` | WebSocket trong TLS | như trên, công khai | 19102 (khi bật chứng chỉ) |

Server: `services/pkg/transport` (`KListener.go` mở cửa, `KWebSocket.go` bắt tay + khung RFC 6455,
chỉ message nhị phân, ping/pong, close; không extension/nén). Gateway mở cả hai cửa cùng lúc
(`gateway.listen`, `gateway.listen_ws`), TLS bật khi có `gateway.tls_cert` + `gateway.tls_key` và áp
cho **cả hai**. Mỗi message WebSocket mang đúng một khung tin. Client: `client/net/KNetAddress.gd`
tách địa chỉ, `KSocketClient.gd` chọn `StreamPeerTCP` / `StreamPeerTLS` / `WebSocketPeer`; HUD hiện
đường đang dùng. Bot: `jxbot -gateway ws://127.0.0.1:19102/ws`. Chứng chỉ tự ký khi thử: client chạy
với `--tls-insecure` (hoặc `JX_TLS_INSECURE=1`), bot tự bỏ kiểm tra cho `tls://`/`wss://`.

Cổng WebSocket còn trả lời `GET /healthz` (JSON: `zone_ready`, `sessions`, `online`, `stopping`;
`503` khi chưa sẵn sàng) cho cân bằng tải, giám sát và `tools/dev.py`.

Bản cũ chỉ có một cổng TCP thô với lớp xáo trộn riêng, nên client web/mobile của lộ trình không thể
kết nối; đây là lý do phần này làm sớm.

## 2. Các họ message và dải id

| Dải | Hướng | File |
|---|---|---|
| 1000–1999 | client → gateway (`C2G_*`) | `client.proto` |
| 2000–2999 | gateway → client (`G2C_*`) | `client.proto` |
| 9000–9999 | gateway ↔ zone (`GZ_*`, `ZG_*`) | `internal.proto` |

Message được đánh dấu **(zone)** trong `msg.proto` (`C2G_MOVE`, `C2G_CHAT`, …) được gateway
chuyển nguyên vẹn tới zone bằng `ClientPacket{sid,msg_id,payload}`; zone trả lời bằng
`ZonePacket{sids[],msg_id,payload}` và gateway fan-out tới từng client. Nhờ vậy:

- zone không biết socket của client, chỉ biết `sid`;
- một gói broadcast (EntityMove cho 200 người) đi qua đường gateway↔zone **một lần**.

## 3. Luồng chuẩn

```text
client            gateway                  zone
  |-- Hello -------->|
  |<-- HelloAck -----|                       (kiểm tra protocol_version)
  |-- LoginReq ----->|  auth (dev: mọi tài khoản)
  |<-- LoginRes -----|
  |-- CharListReq -->|  persist
  |<-- CharListRes --|
  |-- EnterWorldReq->|-- SessionOpen{sid,role} -->|
  |                  |<-- SessionOpenAck ---------|   spawn entity
  |<-- EnterWorldRes-|
  |                  |<-- ZonePacket(EntitySpawn) |   người/quái quanh mình
  |-- MoveReq ------>|-- ClientPacket ----------->|   mô phỏng theo tick
  |<-- EntityMove ---|<-- ZonePacket -------------|   tới mọi sid nhìn thấy
  |   (đóng socket)  |-- SessionClose ----------->|
  |                  |<-- PlayerSave{final} ------|   persist lưu RoleData
```

### Đánh nhau (M3b)

```text
C  → G → Z   C2G_ATTACK{target, seq}          click vào quái
Z  → G → C   G2C_ENTITY_MOVE                   (nếu ngoài tầm: zone tự đi tới mục tiêu)
Z  → G → C*  G2C_ENTITY_ACTION{ATTACK, frames, dir, target}   mọi người nhìn thấy, kể cả mình
Z  → G → C*  G2C_ENTITY_LIFE{life, life_max, delta, source}   lúc đòn trúng (60 % số khung)
Z  → G → C*  G2C_ENTITY_ACTION{HURT | DEATH, frames, pos}     mục tiêu giật / chết (giữ khung cuối)
Z  → G → C*  G2C_ENTITY_DESPAWN ... G2C_ENTITY_SPAWN           xác biến mất sau DeathFrame, hồi sinh sau ReviveFrame
```

`EntityInfo.life/life_max/doing/doing_frames` để người vào sau thấy đúng trạng thái. Nhịp khung và luật xem
`NPCRES.md` mục 4.

### Đổi map (bẫy `NewWorld`)

```text
Z  → G → C*  G2C_ENTITY_DESPAWN                     người xung quanh map cũ thấy biến mất
Z  → G → C   G2C_CHANGE_MAP{map_id, pos, scene_w, scene_h, entity_id}   client xoá mọi entity, nạp bundle map mới
Z  → G → C   G2C_ENTITY_SPAWN                       vùng nhìn mới (có cả chính mình)
```

`SessionOpenAck`/`RolePosition` mang `map_id` để người vào lại đúng map đã lưu (`EnterWorldRes.map_id` lấy từ ack khi ≠ 0).

## 3b. Vòng đời phiên và bảo vệ (gateway)

Trạng thái phiên: `hello → auth → lobby → entering → world` (`services/internal/gateway/session.go`).
Mỗi trạng thái chỉ nhận đúng nhóm message của nó; sai trạng thái = `Kick{RESULT_WRONG_STATE}`.

| Bảo vệ | Mặc định | Cấu hình | Vi phạm |
|---|---|---|---|
| Chờ `Hello` | 10 s | `gateway.hello_timeout_s` | đóng kết nối |
| Im lặng ở lobby | 300 s | `gateway.idle_timeout_s` | đóng kết nối |
| Im lặng trong game (heartbeat) | 30 s | `gateway.heartbeat_timeout_s` | `SessionClose{reason=1}` + đóng |
| Số gói/giây của một client | 40, burst 100 | `gateway.rate_msgs`, `rate_burst` | `Kick{RESULT_RATE_LIMITED}` |
| Đăng nhập sai trên một kết nối | 5 | `gateway.max_login_tries` | `Kick{RESULT_SERVER_BUSY}` |
| Đăng nhập sai của một tài khoản | 5 lần → khoá 60 s | `gateway.max_fails`, `lock_s` | `LoginRes{RESULT_SERVER_BUSY}` |
| Hàng gửi của client đầy | 256 khung | `OutQueue` | đóng kết nối (client quá chậm) |

`HelloAck` mang `auth_mode` ("dev"/"strict") và `heartbeat_s` để client tự biết nhịp ping
(client ping 5 s, tự ngắt nếu 15 s không có `Pong`).

**Một tài khoản = một phiên.** Đăng nhập lần hai mặc định *thắng*: phiên cũ nhận
`Kick{RESULT_REPLACED}`, zone nhận `SessionClose{reason=2}`. Đặt
`gateway.refuse_duplicate_login = true` để theo luật server cũ (`E_ACCOUNT_EXIST`): lần hai bị
từ chối bằng `LoginRes{RESULT_ACCOUNT_IN_USE}`.

`SessionClose.reason`: 0 client rời, 1 hết heartbeat, 2 bị đá/bị thay, 3 gateway tắt.
Khi tắt, gateway đá mọi client rồi **chờ `PlayerSave{final}` của zone** (`gateway.shutdown_wait_s`)
trước khi thoát, nên không mất tiến độ người chơi.

Mã kết quả đăng nhập giữ nguyên ý nghĩa của server cũ (`Bishop/LoginDef.h`, `S3PAccount::Login`):

| `Result` | Cũ | Client hiện (`client/net/KLogin.gd`) |
|---|---|---|
| `UNAUTHORIZED` | `LOGIN_R_ACCOUNT_OR_PASSWORD_ERROR` | Tài khoản hoặc mật khẩu không đúng |
| `ACCOUNT_IN_USE` | `LOGIN_R_ACCOUNT_EXIST` | Tài khoản đang được sử dụng ở nơi khác |
| `ACCOUNT_FROZEN` | `LOGIN_R_FREEZE` | Tài khoản đã bị khoá (+ lý do) |
| `NO_GAME_TIME` | `LOGIN_R_TIMEOUT` / `E_ACCOUNT_NODEPOSIT` | Hết thời gian chơi |
| `SERVER_BUSY` | `LOGIN_R_FAILED` | Sai quá nhiều lần / máy chủ bận |
| `VERSION_MISMATCH` | `LOGIN_R_INVALID_PROTOCOLVERSION` | Client cũ, cần cập nhật |
| `REPLACED` | `LOGIN_R_BEDISCONNECTED` | Vừa đăng nhập ở nơi khác |
| `SERVER_SHUTDOWN` | `LL_R_SERVER_SHUTDOWN` | Máy chủ đang bảo trì |
| `RATE_LIMITED`, `TIMEOUT` | (mới) | Gửi quá nhiều / mất kết nối |

## 4. Phiên bản

- `PROTOCOL_VERSION` (enum trong `msg.proto`) tăng khi thay đổi **không tương thích** (đổi khung
  tin, đổi ngữ nghĩa message). Thêm message hoặc thêm field mới **không** cần tăng.
- Gateway từ chối `Hello` sai phiên bản bằng `Kick{RESULT_VERSION_MISMATCH}`.
- Zone và gateway bắt tay bằng `ZoneHello/ZoneHelloAck`; lệch phiên bản → zone đóng kết nối và log `error`.

## 5. Quy tắc thiết kế message

1. Server authoritative: client chỉ gửi **ý định** (`MoveReq{target}`), zone quyết định vị trí.
2. Mọi thứ client cần để vẽ đều nằm trong `EntityInfo`/`EntityMove`; client không suy diễn từ dữ liệu cũ.
3. Chuỗi luôn là UTF-8 hợp lệ (gateway kiểm tra trước khi chuyển tới zone).
4. `sid` do gateway cấp, duy nhất trong đời tiến trình gateway; `player_id` do persist cấp, vĩnh viễn.
5. Không có message "chung chung" kiểu `Command{string}`; mỗi hành động là một message có tag rõ.

## 6. Sinh mã

```bash
python tools/gen_proto.py          # Go + GDScript (C++ sinh lúc build bằng CMake protobuf_generate)
```

`protoc` lấy từ vcpkg (`build/<preset>/vcpkg_installed/x64-windows/tools/protobuf/protoc.exe`),
`protoc-gen-go` từ `go install google.golang.org/protobuf/cmd/protoc-gen-go@latest`, GDScript bằng
`godot --headless -s addons/protobuf/protobuf_cmdln.gd`. Mã Go và GDScript sinh ra **được commit**
để `go build`/Godot không cần protoc.
