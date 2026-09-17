# JX NEXT

Bản dựng lại đa nền tảng của Võ Lâm (JX) — mã nguồn cũ trong `../SwordOnline` chỉ là tài liệu tham
khảo. Trạng thái: client Godot → gateway Go → zone C++ chạy được trên map thật của game cũ (Phượng
Tường + 3 map nối bằng cổng), NPC/quái có AI và công thức máu/sát thương từ script Lua cũ, tài khoản
argon2id, đường truyền TCP/TLS/WebSocket.

```text
next/
  proto/jx/*.proto        hợp đồng giao thức (protobuf 3) cho cả 3 ngôn ngữ      docs/PROTOCOL.md
  server/common           C++: log JSON, config, clock/tick cố định, id có kiểu, khung tin
  server/net              C++: Asio TCP + khung tin
  server/proto            C++: mã sinh từ .proto (CMake)
  server/zone             C++: jx_zone — AOI grid, di chuyển fixed-point, giao tiếp gateway
  services/               Go: pkg/{log,config,frame,jxpb,persist,auth,transport,jxrec,jxold}, internal/gateway,
                              cmd/{gateway,jxbot,jxaccount,jxassets,jxrecord}
  client/                 Godot 4.7: autoload/{log,net,game}.gd, scenes/{login,char_select,world}
  config/                 zone.json, gateway.json
  tools/                  gen_proto.py (sinh mã), dev.py (build/start/stop/test/e2e)
  docs/                   LOGGING.md, TESTING.md, PROTOCOL.md, RUNNING.md, MAPS.md (map cũ → bundle),
                          NPCRES.md (sprite nhân vật/NPC), OLD-TO-NEW.md (bảng đối chiếu tên file cũ → mới),
                          adr/ (quyết định kiến trúc, giao thức, dữ liệu nhân vật, tài khoản)
```

Bắt đầu: đọc [docs/RUNNING.md](docs/RUNNING.md). Vì sao làm như vậy: [docs/adr/](docs/adr/README.md).
Lộ trình: [../docs/ROADMAP-JXNEXT.md](../docs/ROADMAP-JXNEXT.md).

Quy tắc bất biến: mọi tiến trình log JSON một dòng/sự kiện theo `docs/LOGGING.md`; không có phần
nào "xong" nếu chưa có test chạy trên CI (`.github/workflows/next-ci.yml`: C++ Win/Linux, Go
Win/Linux, client Godot headless, e2e Linux).
