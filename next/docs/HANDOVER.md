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
| ~~U1~~ | ~~Bộ xuất bố cục UI từ `.ini` sang JSON~~ **xong** | vừa | Client JX1 có `\Ui\Ui3\登陆.ini`, `选游戏存档人物.ini`, `新建角色.ini` với toạ độ, cỡ chữ, màu, ảnh nền. |
| ~~U2~~ | ~~Màn đăng nhập theo bản 2.0~~ **xong** | vừa | Client 2.0 **không** có ba tệp đó; phải lấy bố cục JX1 rồi ghép ảnh 2.0, hoặc mổ tiếp `gamecl.exe` (đang bị nén). |
| ~~U3~~ | ~~Màn chọn nhân vật~~ **xong** | vừa | |
| ~~U4~~ | ~~Màn tạo nhân vật~~ **xong** | vừa | Có sẵn các ô: tên, nam/nữ, và ngũ hành Kim/Mộc/Thuỷ/Hoả/Thổ. |

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

## 4b. Nhật ký — cập nhật mỗi lần có việc xong

Ghi từ trên xuống, mới nhất ở trên. Mỗi dòng: **làm gì — đo được gì — commit nào**.

### 2026-09-17 — U2, U3, U4: ba màn của bản 2.0 dựng trong Godot

`client/scenes/KUiScheme.gd` đọc `assets/ui/<tên>.json` rồi dựng đúng cửa sổ của bản cũ: nền, nút
ba trạng thái (thường / nhấn / rê chuột), nút hai trạng thái (`Checkbox=1`), ô nhập (cỡ chữ, canh
lề, màu chữ, màu viền, `MaxLen`, ô mật khẩu), và ô chữ. Khả vẽ 800x600 được co **một hệ số cho cả
hai trục** rồi canh giữa — 1280x720 thành hệ số 1,2, không kéo méo.

Hai điều phải theo đúng bản cũ mới khớp:

- **Ảnh vẽ đúng cỡ thật, không kéo giãn theo ô `.ini`.** Bảng đăng nhập là 542x362 nằm trong cửa sổ
  800x600; kéo nó ra 800x600 thì mọi ô lệch khỏi nhãn của nó. Lần dựng đầu sai đúng chỗ này, ảnh
  chụp cho thấy ngay.
- **`LoginBg=` là nền phía sau.** Màn chọn nhân vật không có ảnh riêng, nó ghi `LoginBg=Login2`,
  tức lấy ảnh `login2` của cửa sổ `login_bg`. Thiếu bước này thì màn chọn hiện ra trống trơn.

**Ảnh nhân vật.** `KUiSelPlayer::GetRoleImageName` ghép tên `<prefix>_<ngũ hành>_<giới>_<n>.spr`:
`n=0` ảnh nhỏ, `n=1` người đứng trước, `n=2` người đứng sau. Màn tạo nhân vật đặt giới đang chọn ở
ảnh 1 và giới kia ở ảnh 2, đổi ngũ hành thì cả hai đổi theo — đúng `KUiNewPlayer::SetPlayerImage`.
Màn chọn nhân vật đặt mỗi nhân vật vào chỗ `Player2Pos_*` / `Player3Pos_*` của `.ini` cho.

| màn | tệp | ghi chú |
|---|---|---|
| Đăng nhập | `scenes/UiLogin.gd` | thêm một ô **Máy chủ** ở đáy — bản cũ đọc `ServerList.ini`, ta chưa có |
| Chọn nhân vật | `scenes/UiSelPlayer.gd` | bấm vào người để chọn; nút *Chuyển nhân vật* ẩn (dịch vụ ta không chạy) |
| Tạo nhân vật | `scenes/UiNewPlayer.gd` (mới) | tên, nam/nữ, Kim Mộc Thuỷ Hoả Thổ |

Mỗi màn vẫn có bản dự phòng bằng nút Godot thường, dùng khi chưa xuất `assets/ui` — client không
bao giờ hiện ra màn trắng.

**Xem thử và kiểm tra:**

```bash
python tools/dev.py client                       # mở client
godot --path client -- --shot                    # chụp màn đăng nhập rồi thoát
godot --headless --path client tests/UiCheck.tscn # 32 kiểm tra bố cục
```

`tests/UiCheck.tscn` phải là **một cảnh**, không chạy bằng `godot -s`, vì các màn này cần autoload
(`Assets`, `Log`, `Game`) mà `-s` không nạp. Nó cũng nằm trong `dev.py test`.

Đã chạy `dev.py e2e`: đăng nhập → tạo nhân vật → chọn → vào game → đi → đánh, cả TCP và WebSocket.

**Sửa thêm:** `dev.py start` chờ zone 90 giây thay vì 20 — nạp 980 map lúc đĩa nguội mất gần một
phút, trước đó bị giết oan.

### 2026-09-17 — U1: bộ xuất bố cục giao diện từ `.ini` sang JSON

`jxassets export-ui <scheme>` đọc một tệp `.ini` dưới `\Ui\<scheme>\` — đúng thứ mà
`KUiLogin::LoadScheme` đọc — rồi ghi ra `client/assets/ui/<tên>.json` kèm mọi ảnh nó gọi tên. Toạ độ
giữ nguyên như bản cũ: khả vẽ là 800x600, client tự co giãn.

| màn | lấy từ | ô |
|---|---|---:|
| `login` | `\Ui\Ui3\登陆.ini` | 7 |
| `login_bg` | `\Ui\Ui3\登陆过程背景窗口.ini` | 14 |
| `select_role` | `\Ui\Ui3\选游戏存档人物.ini` | 16 |
| `new_role` | `\Ui\Ui3\新建角色.ini` | 12 |

Giữ đủ những gì bản cũ dùng: khung chữ nhật, ảnh và số khung cho từng trạng thái nút
(`Up`/`Down`/`Over`), cỡ chữ, canh lề, màu chữ và màu viền, `MaxLen`, `Type=1` là ô mật khẩu. Mọi khoá
khác vào `extra` nên không mất gì.

Hai thứ phải làm thêm mới đủ:

- **Ảnh nền `.jpg`** (`ImgType=1`): không phải `.spr` nên bộ giải mã sprite từ chối. Giờ chép nguyên
  byte ra `client/assets/ui/images/<id>.jpg`.
- **Ảnh nhân vật**: bản cũ không ghi tên từng ảnh mà **ghép tên**
  (`KUiSelPlayer::GetRoleImageName`: `<PlayerImgPrefix>_<ngũ hành>_<giới>_<n>.spr`). Bộ xuất sinh đủ
  **30** tên — 5 ngũ hành x 2 giới x 3 góc — và ghi vào `portraits` của màn.

**Ảnh lấy từ bản 2.0.** Client 2.0 không có bốn tệp `.ini` này nhưng **có đủ ảnh**. Chạy với chuỗi dự
phòng thì chỉ bốn `.ini` rơi về client JX1, còn **cả 49 ảnh đều là ảnh 2.0**:

```bash
build/go/jxassets.exe export-ui Ui3 -client "<client 2.0>;<bin/Client>" -out client/assets
```

**Lỗi bắt được nhờ test**: `WriteAtlas` không tự tạo thư mục `sprites/`, nên xuất vào một thư mục
trống thì **mọi sprite đều hỏng lặng lẽ**. Đã sửa trong `KSpriteAtlas.go`.

Ba test mới trong `KUiExport_test.go` chốt ô tài khoản ở 351,238 156x18 `MaxLen=80`, ô mật khẩu
`Type=1`, nút Đăng nhập 3 khung 0/1/2, và đủ 30 ảnh nhân vật.

### 2026-09-17 — N1 và N2: chặn số người nhận, và vùng nhìn đúng hình màn hình

**N2 — vùng nhìn theo màn hình.** `KRegionGrid` nhận hai số ô riêng cho trục x và trục y thay vì một
hình vuông. `KSubWorldConfig` khai báo vùng nhìn bằng **đơn vị cảnh** (`view_width` 1280, `view_height`
1536) và luới tự suy ra số ô, **làm tròn lên** để vùng nhìn không bao giờ nhỏ hơn màn hình. Ô lưới
512 → 256. Đây là **sửa lỗi**: trước đó vùng nhìn chỉ bảo đảm 512 đơn vị trong khi màn hình cần 640
ngang và 768 dọc, nên có lúc entity hiện trên màn hình mà server chưa gửi.

**N1 — chặn số người nhận mỗi gói.** `KSubWorld::viewers_cached` giờ giữ khoảng cách của từng người
xem và cắt danh sách xuống `max_viewers` (mặc định **100**, đúng con số `MAX_BROADCAST_COUNT` của bản cũ ở
`Core/Src/KRegion.h:9`), **giữ người gần nhất**. Bản cũ giữ ai đến trước, cách này tốt hơn. Số lần cắt
đếm được qua `viewers_capped` trong dòng `zone.tick`.

**Đo được**, cùng kịch bản dồn vào một map:

| | Trước | Sau |
|---|---:|---:|
| người trên một map | 1 846 | **2 000** (vào được hết) |
| tick trung bình | 11,26 ms | **5,16 ms** |
| p95 | 33,55 | **12,58** |
| p99 | **100,66** | **29,36** |
| chi phí worker giữ map đó | 4,85 ms | **3,54 ms** |
| gateway đẩy ra | 1 058 242 gói/s | **197 805 gói/s** |
| băng thông | 56,3 MB/s | **15,0 MB/s** |

Số gói giảm **5,3 lần**, băng thông giảm **3,8 lần**, p99 tốt hơn **3,4 lần** — trong khi vùng nhìn mới
còn **rộng hơn** cũ (1792 đơn vị so với 1536). Đó là giá trị của N1.

**3 000 người trên một map** — trường hợp trước đây vỡ hẳn:

| | Trước | Sau |
|---|---:|---:|
| người vào được | 2 978 | **3 000** |
| chi phí worker giữ map | 28,38 ms | **6,15 ms** |
| tick lúc đông nhất | 41,45 ms | **9,08 ms** |
| **lúc ổn định**: tb / p95 / p99 | — | **7,80 / 10,49 / 14,68 ms** |
| tick bị rớt | 13 | **7** |

Số lần cắt danh sách người xem: **55 520** và vẫn tăng — cơ chế chặn chạy liên tục.

**M6 mới đạt một nửa.** Lúc ổn định thì p99 14,68 ms, thừa ngân sách. Nhưng **lúc người chơi ùa
vào** (100 người mỗi giây) thì p99 lên 268 ms và 7 tick bị rớt: mỗi người vào phải nhận toàn bộ
những ai đang thấy, và phải được báo cho tất cả. Đó đúng là **N4** — rải việc vào và ra ra nhiều tick,
chưa làm.

Thêm ba test: vùng nhìn mặc định phủ hết bốn góc màn hình của cả hai loại client; đám đông không làm
một hành động đến được tất cả; các test cơ chế qua biên ô giờ ghim `view_cells = 1` vì chúng nói về cơ
chế chứ không về độ rộng. **119 ctest xanh.**

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
