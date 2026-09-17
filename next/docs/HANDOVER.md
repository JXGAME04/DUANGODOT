# Bàn giao JX NEXT — trạng thái ngày 2026-09-17

Tài liệu này trả lời ba câu: **đang ở đâu**, **còn gì phải làm**, **làm theo thứ tự nào**. Người
tiếp nhận đọc xong là chạy được hệ thống và biết bước kế tiếp mà không phải hỏi ai.

Phạm vi đã chốt: **làm bản PC (Windows) trước**. Android và các nền tảng khác để sau, codebase giữ
đủ trừu tượng để thêm vào nhưng chưa làm.

## 1. Chạy được ngay trong mười phút

```bash
cd next
cmake --preset windows-msvc && cmake --build --preset windows-msvc-release
python tools/dev.py lua        # chuyển 7 663 script sang Lua 5.4 -> data/script  (~3 giây)
python tools/dev.py assets     # xuất map 1 từ client cũ          (~10 giây)
python tools/dev.py e2e        # zone + gateway + client Godot, qua cả TCP lẫn WebSocket
```

Muốn toàn bộ 980 map thì chạy `build/go/jxassets export-all -client "<client>" -server "<server>"
-out client/assets` (3 phút 49 giây, 2,4 GB) rồi để `zone.maps = "all"` trong `config/zone.json`.

Nguồn dữ liệu cũ khai báo ở `config/oldgame.local.json`; mẫu ở `config/oldgame.example.json`.
Thư mục `data/` và `client/assets/` **không commit** — chúng là dữ liệu sinh ra.

Tài liệu đi kèm: [ROADMAP](../../docs/ROADMAP-JXNEXT.md) · [RUNNING](RUNNING.md) ·
[TESTING](TESTING.md) · [SCRIPTS](SCRIPTS.md) · [MAPS](MAPS.md) · [PROTOCOL](PROTOCOL.md) ·
[OLD-TO-NEW](OLD-TO-NEW.md) · [ADR](adr/README.md).

## 2. Đang ở đâu

Xong và có test:

| Phần | Trạng thái | Bằng chứng |
|---|---|---|
| Lõi zone C++: tick cố định, entity, lưới không gian, tầm nhìn, A\* | xong | 117 ctest |
| Zone nhiều nhân: một map một chủ, worker pool, cân tải | xong | [ADR-005](adr/ADR-005-gameserver-nhieu-nhan.md) |
| Gateway Go: TCP, TLS, WebSocket, phiên, chống gói rác | xong | `go test ./...` |
| Giao thức V2 (protobuf) C++ ↔ Go ↔ Godot | xong | round-trip + fuzz |
| Tài khoản argon2id, một tài khoản một phiên | xong | [ADR-004](adr/ADR-004-tai-khoan-phien.md) |
| Script Lua 5.4 thật, **không còn lớp tương thích Lua 4** | xong | 7 663 tệp, 0 lỗi, [ADR-006](adr/ADR-006-lua54-khong-tuong-thich-lua4.md) |
| Toàn bộ 980 map trong một zone | xong | 819 MB, tick 1,6 ms lúc rỗng |
| Client Godot: đăng nhập → chọn nhân vật → vào map → đi → đánh | xong | e2e TCP + WS |
| Bẫy và cổng dịch chuyển giữa map | xong | test trap |

Đo được về tải, trên máy 24 luồng (i7-13700K, 32 GB), 18 Hz, ngân sách 55 ms một tick:

| Kịch bản | Người cùng lúc | tick tb | p99 | Rớt |
|---|---:|---:|---:|---:|
| 5 000 trên 40 map | 5 000 | 8,08 ms | 19,85 | 0 |
| 10 000 trên 80 map | 9 995 | 8,56 ms | 16,78 | 0 |
| 20 000 bot trên 120 map | 13 031 | 10,20 ms | 20,13 | 0 |
| **2 000 dồn vào một map** | 1 846 | 11,26 ms | 100,7 | 0 |
| **3 000 dồn vào một map** | 2 978 | 41,45 ms | **325,6** | 13 tick bị rớt |

Hai trần đã biết, và **cả hai đều nằm ở mạng chứ không ở tính toán**:

- **Một map chịu được khoảng 1 500–1 800 người.** Ở 1 846 người, mô phỏng chỉ tốn 4,85 ms trong khi
  gateway phải đẩy 1 058 242 gói mỗi giây (56,3 MB/giây).
- **20 000 người chưa đo được trên một máy**: Windows chỉ có 16 384 cổng động nên 3 665 bot không
  mở nổi kết nối. Phải bắn bot từ máy thứ hai, hoặc nới dải cổng.

## 3. Việc còn lại

Chia theo nhóm; cột **giá** là ước lượng công sức, không phải cam kết.

### 3a. Hiệu năng mạng — làm trước, vì đang có lỗi thật

| # | Việc | Giá | Ghi chú |
|---|---|---|---|
| N1 | **Giới hạn số người nhận mỗi gói** | nhỏ | Bản cũ dùng `MAX_BROADCAST_COUNT = 100` (`KRegion.h:9`); ta đang gửi cho **tất cả**. Ước giảm ~18 lần số gói ở 1 846 người. |
| N2 | **Vùng nhìn theo hình màn hình** | nhỏ | **Đây là lỗi**: ô 512 + `view_cells 1` chỉ bảo đảm 512 đơn vị, trong khi màn hình cần 640 ngang và 768 dọc. Có lúc entity hiện trên màn hình mà server chưa gửi. |
| N3 | Gửi thưa dần theo khoảng cách, gộp nhiều entity một gói | vừa | Người ở xa nhận vị trí mỗi vài tick. |
| N4 | Rải việc thoát ra nhiều tick | nhỏ | 10 000 người thoát cùng lúc làm p99 vọt lên 100 ms. |
| N5 | Bỏ bớt gói khi tắc, rộng hơn gói vị trí | vừa | Hiện chỉ bỏ vị trí nên ở 13 878 người còn tồn 356 MB trên đường truyền. |
| N6 | Chạy thật gateway thứ hai | nhỏ | Hạ tầng xong (`ZoneHelloAck.session_prefix`), chưa đo được vì máy test hết cổng. |
| N7 | **Chia vùng trong một map nóng** | lớn | Giai đoạn R. Chỉ làm **sau** N1–N3, vì nút thắt hiện ở mạng. |

### 3b. Giao diện bản 2.0 — việc bạn giao, chưa làm

| # | Việc | Giá | Ghi chú |
|---|---|---|---|
| U1 | Bộ xuất bố cục UI từ `.ini` sang JSON | vừa | Client JX1 có `\Ui\Ui3\登陆.ini`, `选游戏存档人物.ini`, `新建角色.ini` với toạ độ, cỡ chữ, màu, ảnh nền. |
| U2 | Màn đăng nhập theo bản 2.0 | vừa | Client 2.0 **không** có ba tệp đó; phải lấy bố cục JX1 rồi ghép ảnh 2.0, hoặc mổ tiếp `gamecl.exe` (đang bị nén). |
| U3 | Màn chọn nhân vật | vừa | |
| U4 | Màn tạo nhân vật | vừa | Có sẵn các ô: tên, nam/nữ, và ngũ hành Kim/Mộc/Thuỷ/Hoả/Thổ. |

### 3c. Gameplay — chủ dự án đã hoãn, chờ mổ nhị phân bản Linux

Vật phẩm và túi đồ · kỹ năng và công thức chiến đấu · bộ hàm script cho game (235 hàm của bản cũ) ·
nhiệm vụ · chat kênh, bạn bè, thư, bang hội · tổ đội, giao dịch, PK, bảng xếp hạng.

### 3d. Client PC

Hoạt ảnh đánh và chết · trang bị hiện lên người · minimap · âm thanh, thời tiết, ánh sáng.

### 3e. Vận hành và dữ liệu

| # | Việc | Giá | Ghi chú |
|---|---|---|---|
| O1 | **PostgreSQL thay kho tệp** | lớn | Mỗi nhân vật đang là một tệp JSON; 11 461 tệp từng làm gateway khởi động mất 31 giây. Không dùng thật được. |
| O2 | Số liệu Prometheus + bảng Grafana | vừa | Zone đã có `jx::Metrics` và `to_json`, chỉ thiếu đầu ra. |
| O3 | Công cụ quản trị | vừa | |
| O4 | Docker, sao lưu và khôi phục có test | vừa | |
| O5 | Nạp nóng dữ liệu và script | vừa | |
| D1 | 62 bảng Settings sang `data/` có schema + validator trong CI | lớn | |
| D2 | 74 map không có `.wor` trong cả hai client | nhỏ | Tìm nguồn khác hoặc chấp nhận bỏ. |
| D3 | Đọc `TRoleData` của Goddess cũ để có dữ liệu nhân vật thật | vừa | |

## 4. Lịch trình tới khi hoàn thiện

Mốc tính theo tuần làm việc, một người. Mỗi mốc có **điều kiện nghiệm thu đo được**, không nghiệm
thu bằng cảm tính.

| Mốc | Nội dung | Tuần | Nghiệm thu |
|---|---|---:|---|
| **M6** | N1, N2, N4 | 1 | 3 000 người **một map**: p99 < 55 ms, 0 tick bị rớt. Có test khẳng định vùng nhìn phủ hết màn hình. |
| **M7** | N3, N5, N6 | 1–2 | 20 000 người trên hai gateway: p99 < 55 ms, tồn đọng đường truyền < 4 MB. Cần bot từ máy thứ hai. |
| **M8** | U1–U4 | 2–3 | Đăng nhập, chọn và tạo nhân vật đúng bố cục bản 2.0; ảnh chụp màn hình đối chiếu. |
| **M9** | O1 (PostgreSQL) | 2 | 20 000 nhân vật, gateway khởi động < 3 giây; test crash giữa chừng không mất dữ liệu. |
| **M10** | Mổ nhị phân bản Linux: kỹ năng + hàm script | 3–4 | Danh sách đầy đủ 235 hàm và bảng kỹ năng, có tài liệu. |
| **M11** | Vật phẩm, túi đồ, trang bị, rơi đồ | 4 | Test tính chất: không âm, không nhân bản. |
| **M12** | Chiến đấu và kỹ năng theo công thức cũ | 6 | **Đối chiếu số với Core cũ**: cùng đầu vào, cùng kết quả. |
| **M13** | Nhiệm vụ trên Lua + bộ hàm script | 4 | Mỗi hàm binding có test; replay nhiệm vụ khớp. |
| **M14** | Xã hội: chat, bạn bè, thư, bang hội, tổ đội, giao dịch, PK | 5 | Test nhiều phiên; giao dịch nguyên tử. |
| **M15** | Client PC đầy đủ: hoạt ảnh, trang bị, minimap, âm thanh | 4 | Chơi được một vòng đầy đủ trên PC. |
| **M16** | N7 chia vùng map nóng | 3 | 5 000 người **một map**: p99 < 55 ms. |
| **M17** | Vận hành: O2–O5, D1–D3 | 4 | Bảng Grafana chạy; validator xanh 100 %. |
| **M18** | Beta kín | 4 | Người chơi thật; lỗi theo dõi từ log tập trung. |

Cộng lại khoảng **44 tuần cho một người**, tức gần một năm. Nếu tách hai người, một người làm
server và một người làm client, thì M8 và M15 chạy song song với phần server và rút xuống còn
khoảng **32 tuần**.

Thứ tự trên có một ràng buộc cứng: **M10 phải xong trước M11, M12, M13**, vì cả ba đều cần biết
chính xác bản cũ làm gì. Mọi thứ khác có thể đổi chỗ.

## 5. Điều người tiếp nhận cần biết để không vấp

- **Không sửa mã nguồn cũ bằng công cụ soạn thảo thường.** Cây `SwordOnline/` trộn GBK và TCVN3;
  đọc bằng latin-1 hoặc dùng script trong skill, nếu không sẽ hỏng toàn bộ tiếng Việt trong tệp.
- **Không commit dữ liệu game và cơ sở dữ liệu tài khoản.** `data/` và `client/assets/` đã nằm
  trong `.gitignore`; `account.db`, `role.db`, `swsql.txt` cũng vậy.
- **Dừng server trước khi build C++**, nếu không sẽ gặp `LNK1168`.
- **Test tải ăn rất nhiều bộ nhớ.** Bot, zone và gateway chạy cùng một máy: xem bộ nhớ trống trước
  khi chạy, và nhớ rằng từ 10 000 bot trở lên chính máy test là một phần của giới hạn.
- **Bản cũ là nguồn đối chiếu, không phải nguồn để sửa.** Mọi con số nghi ngờ đều tra được trong
  `SwordOnline/Sources` — `MAX_BROADCAST_COUNT` và `MAX_SYNC_RANGE` của mục N1, N2 tìm ra đúng theo
  cách đó.
- **Có nhánh dự phòng trên GitHub**: `safe/jxnext-2026-09-17` và tag `backup/jxnext-2026-09-17`.
  Nếu `main` hỏng thì lấy lại từ đó.
