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
