# JX NEXT

Bản dựng lại đa nền tảng của Võ Lâm (JX) — mã nguồn cũ trong `../SwordOnline` chỉ là tài liệu tham
khảo. Trạng thái: **mốc M1 (lát cắt dọc)** chạy được: client Godot → gateway Go → zone C++,
đăng nhập, tạo nhân vật, vào map lưới, di chuyển đồng bộ, chat, NPC đi lại, lưu vị trí.

```text
next/
  proto/jx/*.proto        hợp đồng giao thức (protobuf 3) cho cả 3 ngôn ngữ      docs/PROTOCOL.md
  server/common           C++: log JSON, config, clock/tick cố định, id có kiểu, khung tin
  server/net              C++: Asio TCP + khung tin
  server/proto            C++: mã sinh từ .proto (CMake)
  server/zone             C++: jx_zone — AOI grid, di chuyển fixed-point, giao tiếp gateway
  services/               Go: pkg/{log,config,frame,jxpb,persist,auth}, internal/gateway,
                              cmd/gateway, cmd/jxbot
  client/                 Godot 4.7: autoload/{log,net,game}.gd, scenes/{login,char_select,world}
  config/                 zone.json, gateway.json
  tools/                  gen_proto.py (sinh mã), dev.py (build/start/stop/test/e2e)
  docs/                   LOGGING.md, TESTING.md, PROTOCOL.md, RUNNING.md, MAPS.md (map cũ → bundle),
                          NPCRES.md (sprite nhân vật/NPC), OLD-TO-NEW.md (bảng đối chiếu tên file cũ → mới)
```

Bắt đầu: đọc [docs/RUNNING.md](docs/RUNNING.md). Lộ trình: [../docs/ROADMAP-JXNEXT.md](../docs/ROADMAP-JXNEXT.md).

Quy tắc bất biến: mọi tiến trình log JSON một dòng/sự kiện theo `docs/LOGGING.md`; không có phần
nào "xong" nếu chưa có test chạy trên CI (`.github/workflows/next-ci.yml`: C++ Win/Linux, Go
Win/Linux, client Godot headless, e2e Linux).
