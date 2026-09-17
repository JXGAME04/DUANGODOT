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
| Client Godot: luồng đăng nhập **bản 2.0** (7 cửa sổ) → vào map → đi → đánh | xong | e2e TCP + WS, 95 kiểm tra giao diện, khớp điểm ảnh client thật |
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
| ~~N1~~ | ~~Giới hạn số người nhận mỗi gói~~ **xong, và đã sửa lại** | nhỏ | Bản đầu cắt mỗi gói ở 100 phiên gần nhất → client giữ "bóng ma". Nay giới hạn nằm ở **điều mỗi client biết** (`KInterest.cpp`), xem nhật ký. |
| ~~N2~~ | ~~Vùng nhìn theo hình màn hình~~ **xong** | nhỏ | |
| ~~N3~~ | ~~Gửi thưa dần theo khoảng cách, gộp nhiều entity một gói~~ **xong** | vừa | Trong `near_radius` (320) báo ngay; xa hơn gom vào `EntityMoves` mỗi 6 tick. Kèm luật hoán đổi mới và gác theo ô: 3 000 người một map p99 325 → **21 ms**, xem nhật ký. |
| ~~N4~~ | ~~Rải việc vào ra nhiều tick~~ **xong** | nhỏ | `spawn_budget` 48 entity mỗi lần nhìn quanh: p99 lúc người ùa vào 268 ms → 16,78 ms. |
| ~~N5~~ | ~~Bỏ bớt gói khi tắc, rộng hơn gói vị trí~~ **xong** | vừa | Hàng đợi gateway gộp cả gói máu (giá trị tuyệt đối) và đánh/bị đánh theo entity, bỏ theo thứ tự vị trí → chiến đấu, không bao giờ bỏ chết/hồi sinh/spawn/chat. Đường zone→gateway bỏ cả gói gộp xa khi tồn > 4 MiB. Chưa đo lại ở 13 878 người (cần máy bot thứ hai). |
| ~~N6~~ | ~~Chạy thật gateway thứ hai~~ **xong ở 3 000** | nhỏ | 2 gateway × 1 500 người: cả hai online đủ, 0 hỏng, zone p99 12,5 ms. 20 000 vẫn cần máy bot thứ hai. |
| N7 | **Chia vùng trong một map nóng** | lớn | Giai đoạn R. Chỉ làm **sau** N1–N3, vì nút thắt hiện ở mạng. |

### 3b. Giao diện bản 2.0 — luồng đăng nhập đã xong, khớp client thật

| # | Việc | Trạng thái |
|---|---|---|
| ~~U1~~ | Bộ xuất bố cục UI: `.ini` của 2.0 (đọc theo đúng tên game gọi) → JSON + atlas + font bitmap + câu thông báo | **xong** |
| ~~U2–U5~~ | Bảy cửa sổ của luồng đăng nhập 2.0 trong Godot (`client/ui/uicase`), phần tử theo `Ui/Elem` cũ (`client/ui/elem`) | **xong**, hai màn đối chiếu được với client thật: 99,98 % và 99,88 % |
| U6 | Bàn phím ảo, "Tùy Chọn Hệ Thống", "Xem Ghi Hình", âm thanh nút | chưa |
| U7 | Đối chiếu điểm ảnh các màn sau đăng nhập | cần ảnh chụp client thật (phải có tài khoản) |

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
| ~~**M6**~~ **đạt 2026‑09‑17** | N1, N2, N4 | 1 | 3 000 người **một map**: p99 < 55 ms, 0 tick bị rớt → đo được **p99 16,78 ms, 0 tick rớt**. Có test khẳng định vùng nhìn phủ hết màn hình và client không bao giờ giữ bóng ma. |
| ~~**M7**~~ **đạt 2026‑09‑17 (ở 3 000)** | N3, N5, N6 | 1–2 | 3 000 người **một map**: p99 **20,97 ms** một gateway / **12,52 ms** hai gateway, 0 tick rớt, gateway 337 k gói/s (trước 617 k). 20 000 người trên hai gateway chưa đo được: máy test hết cổng, cần bot từ máy thứ hai. |
| ~~**M8**~~ **đạt 2026‑09‑17** | U1–U5 | 2–3 | Đăng nhập, chọn và tạo nhân vật đúng bố cục bản 2.0; ảnh chụp màn hình đối chiếu → **99,98 % / 99,88 %** điểm ảnh trên hai màn chụp được từ client thật, 95 kiểm tra giao diện. |
| **M9** | O1 (PostgreSQL) | 2 | 20 000 nhân vật, gateway khởi động < 3 giây; test crash giữa chừng không mất dữ liệu. |
| ~~**M10**~~ **đạt 2026‑09‑17** | Mổ nhị phân bản Linux: kỹ năng + hàm script | 3–4 | 1506 hàm script (game) + 438 (gateway), **chữ ký đọc bằng máy cho cả 1506** (1149 đối số cố định, 1496 biết số trả về); **109 tệp settings, 104 nối được cột/khoá mã đọc (736)**; hai lớp `KTabFile`/`KIniFile` đặt tên từng phương thức; 431 stub PLT có tên. Công cụ `re_elf/re_calls/re_luasig/re_tables`, [LINUX-SERVER.md]. |
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

### 2026-09-17 (đêm) — M7: một map 3 000 người từ p99 325 ms xuống 21 ms; N3, N5, N6

Đo trước khi làm (1 500 bot một chỗ) chỉ ra chỗ nghẽn thật của một map đông **không phải mạng** mà là pha
**interest** (mỗi client nhìn quanh): 20–27 ms mỗi tick. Benchmark thuần `[.bench]` (3 000 người một chỗ)
với bộ đếm mới (`look_stats`) chỉ đích danh: **luật hoán đổi "xa nhường gần"** của client đã đầy — trong
đám đông dày lúc nào cũng có người "gần bằng nửa", nên mỗi client đổi 4 người mỗi 16 tick mãi mãi: ~700
cặp despawn+spawn mỗi tick, 85 % pha interest, gấp đôi số gói. Chi tiết và bảng đo: [TESTING.md](TESTING.md)
§3d "đo lại sau M7".

- **Luật hoán đổi mới**: chỉ đổi khi người lạ bước vào trong `near_radius` (320 = 1/4 màn hình) và người
  bị thay đang ở ngoài; tìm theo ô gần trước, dừng khi đủ (`KRegionGrid::for_each_player_near`). Benchmark:
  interest **11,9 → 2,1 ms**.
- **N3**: `EntityMove` ngay cho watcher gần, `EntityMoves` gộp mỗi `far_period` (6 tick) cho watcher xa
  (`KSubWorld::emit_move` / `flush_far`, proto `G2C_ENTITY_MOVES`, client Godot + bot đã hiểu). Cấu hình
  `zone.near_radius`, `zone.far_period`. Gói/tick trong benchmark 19 434 → 12 692.
- **Gác theo ô**: `KRegionGrid` đếm phiên bản mỗi ô; `KViewer` nhớ 30 phiên bản trong tầm nhìn, chỉ quét ứng
  viên ở ô đã đổi (và quét hết khi đổi ô, quên ai đó, hay còn chỗ hơn lần trước). Đo thật: 7–8 ứng viên
  mỗi lượt nhìn thay vì ~300 NPC.
- **N5**: `KSendQueue` của gateway phân lớp khung (`classify`): vị trí và gói gộp xa bỏ trước, gói máu /
  đánh / bị đánh gộp theo entity và bỏ sau, chết / hồi sinh / spawn / despawn / chat / ack không bao giờ.
  Đường zone→gateway bỏ cả `EntityMoves` khi tồn > 4 MiB. Có test.
- **N6**: 3 000 bot trên **2 gateway** (`dev.py load 3000 60 hot 1 2`): 1 500 + 1 500 online, 0 hỏng,
  zone p99 12,52 ms.
- **Đo thật (Release)**: 1 500 một chỗ: tick 3,15 / p99 6,15 ms; **3 000 một map: 5,82 / p99 20,97 ms, 0
  rớt** (bảng cũ 2 978: 41,45 / 325,6, 13 rớt); gateway 337 k gói/s, 15,4 MB/s (cũ 617 k / 37,9).
- Bài học ghi lại: `dev.py` mặc định zone **Debug**; 4 lượt đo đầu của đợt là Debug (42 → 26 ms, so sánh
  nội bộ vẫn đúng). Giờ dòng tổng kết in `zone Debug/Release` và nhắc.
- Dòng `zone.tick` có thêm `looks` (số lượt nhìn quanh, nhàn rỗi, ứng viên/lượt, học/quên) — con số để
  chẩn đoán pha interest mà không cần benchmark.

Việc còn lại của 3a: N7 (chia vùng trong một map) — chưa cần: 3 000 người một map đã dưới ngân sách.

### 2026-09-17 (khuya) — CI: hai lỗi thật đầu tiên của lần chạy đủ (5c66f1c) — đã sửa

Lần CI đầu tiên chạy hết (không bị push sau huỷ) báo hai job đỏ. Log GitHub giờ **bắt buộc đăng nhập**
mới xem được, nên đọc qua annotation (công khai qua API) — và sửa để lần sau cũng đọc được như thế.

- **MSVC Test (Release)**: `test_ThreadPool.cpp` "tasks run on the workers" — `CHECK(threads.size() > 1)`
  hỏng vì trên runner ít nhân, một worker kịp làm hết 200 tác vụ rỗng trước khi worker khác thức. Test
  giờ giữ worker đầu lại đến khi worker thứ hai xuất hiện (condition_variable, chờ tối đa 5 s), và
  đếm lỗi bằng atomic rồi `CHECK` trên luồng test (assert Catch2 không an toàn đa luồng). Chạy 3×2
  cấu hình + ghim tiến trình vào **1 nhân**: đều đạt.
- **End to end (Linux)**: zone **fatal** vì không có `client/assets/maps/1/map.json` — CI không có
  dữ liệu game (không bao giờ có), nên job này chưa từng đạt. `dev.py start` giờ thấy thiếu bản đồ thì
  chạy zone với `--set zone.map_dir=` (thế giới phẳng thử nghiệm) và nói rõ. Mô phỏng đúng CI tại chỗ
  (giấu cả `client/assets` lẫn `data/`): BOT OK, CLIENT OK, CLIENT WS OK.
- Bước e2e trong workflow ghi output ra `build/e2e.log`; khi hỏng, `ci_annotate.py --tail` in đuôi của
  e2e.log / zone.log / gateway.log (dòng error/fatal trước) thành annotation.

### 2026-09-17 (khuya, đợt 2) — M10 xong: chữ ký 1506 hàm script, cột/khoá của 109 tệp settings

Chủ dự án nhắc "làm thật kỹ". Đợt 1 mới liệt kê; đợt này đi tới **từng dòng**: chữ ký từng hàm script và
từng cột/khoá mà mã đọc của từng tệp settings — bằng máy, kiểm lại bằng tay, sửa cả một chỗ đợt 1 viết sai.
Chi tiết ở [LINUX-SERVER.md](LINUX-SERVER.md) §5–§6; công cụ `next/tools/re/`.

- **Sửa sai đợt 1**: §5 cũ nói "hàm script không nhận `lua_State*`, nhận số trực tiếp" — **sai**. Đọc thân
  hàm: `int f(lua_State* L)` với Lua 4.0 liên kết tĩnh (`lua_gettop 0x8232490`, `lua_tonumber 0x82338B0`,
  `lua_pushnumber 0x8232D40`…; `GetPlayerIndex 0x8107860` đọc global `"PlayerIndex"`). Đúng như `Script.cpp`
  của JX1. Ví dụ `GetLevel` giải mã lại từng lệnh trong tài liệu.
- **`re_luasig.py`** → [`linux/jx_linux_luasig.tsv`](linux/jx_linux_luasig.tsv): 1506 hàm, mỗi hàm: đối
  số đọc ở chỉ số nào/kiểu gì, có xem `lua_gettop` (đối số tuỳ chọn), có cần nhân vật, đẩy gì, trả mấy giá
  trị. **1149** hàm đối số cố định, 894 có `gettop`, 711 cần nhân vật, **1496/1506** biết số trả về. Theo
  được hàm bọc (tail-jump, gọi thường với `L`) và khối GCC đặt sau epilogue (`SetPos(x,y)`, `GetPos()→3`,
  `GetTask(id)→số|nil`).
- **`re_calls.py`**: đồ thị gọi hàm đi theo từng hàm (quét tuyến tính lạc nhịp ở dữ liệu), theo dõi hằng
  qua thanh ghi / stack / `[esp+N]`, hiểu `this`, thành viên `this+off`, bảng trên stack, tail-call, khối sau
  `ret` sớm, cất/khôi phục thanh ghi. 6027 hàm, 35 074 lời gọi, cache 10 giây.
- **`re_tables.py`** → [`linux/jx_settings_cot.md`](linux/jx_settings_cot.md): **109 tệp nạp thật, 104 nối
  được với 736 cột/khoá mã đọc**, còn 6 lượt đọc ở 5 hàm chưa nối (từ 1094 lúc đầu). Ghi rõ tệp đọc theo
  chỉ số cột (không có tên cột trong mã) và tệp được **ghi**. Kiểm chứng với tệp thật: `logset.ini` chỉ có
  `[LogSet]` — báo cáo cũng vậy; `[ENCHASER]`, `[Coin]`… về đúng `gamesetting.ini` (116 khoá). `Skills.txt`
  60 cột, `NpcS.txt` 94, `AbradeRate.ini` 75.
- **Hai lớp tệp đặt tên đủ** (đối chiếu header cũ `KTabFile.h`/`KITabFile.h`/`KIniFile.h`, đọc từng thân
  hàm): `KTabFile` 0x20 byte, `Load/GetInteger/GetFloat/GetString` theo tên cột, theo tên dòng, theo chỉ số,
  `FindRow/FindColumn/Str2Col`, ctor/dtor; `KIniFile` `Load/GetInteger/GetString/GetInteger2/WriteInteger/
  WriteString/Save`. Đợt 1 tưởng `0x0821F7C0` là `GetFloat` — thực ra là `WriteInteger`.
- **PLT gốc có tên**: lớp bảo vệ viết lại DYNAMIC, nhưng `.rel.plt` gốc còn ở `0x0804A654` (183 mục) và chỉ
  số symbol vẫn khớp — kiểm bằng 5 stub đã biết chức năng (`strtol`, `sprintf`, `strncpy`, `strtod`,
  `__cxa_atexit`): cả 5 đúng. `dis` giờ chú thích `strtol@plt`.

Cách kiểm nhanh một dòng bất kỳ: `python tools/re/re_luasig.py D:/ServerLinux/server1/jx_linux_y sig <tên>`
rồi `re_elf.py dis <va>` đọc đối chiếu.

### 2026-09-17 (khuya) — M10 đợt 1: mổ nhị phân server Linux — bộ hàm script + hệ settings

Chủ dự án giao: "mổ nhị phân `D:\ServerLinux` lấy toàn bộ settings và script, chính xác từng dòng".
Đợt này làm phần **liệt kê + phân loại + định vị** (bản đồ để M11–M13 hiện thực từng hệ). Tất cả đọc
thẳng từ nhị phân, không đoán. Chi tiết: [LINUX-SERVER.md](LINUX-SERVER.md).

- Công cụ mới `next/tools/re/re_elf.py`: đọc ELF **không có section header** qua program header +
  DYNAMIC (info/imports/exports/strings/xref/xrefstr/dis/func/luamap).
- `jx_linux_y` **không nén UPX**; code game + bảng Lua + chuỗi settings nằm **rõ** trong segment r-x
  đầu. Có một segment rwx 5,7 MB entropy 8.0 (lớp bảo vệ KG_Angel) — bỏ qua, không cần.
- **Bộ hàm script: 1506 hàm** (`jx_linux_y`) + **438** (`s3relay_y`), lọc theo prologue `push ebp`
  nên sạch hơn bản đồ cũ (1561, lẫn từ khoá Lua). Nhóm theo miền: Bang hội/công thành 194, Vật phẩm
  123, NPC 84, Nhiệm vụ 61, Kỹ năng+chiêu 53, Cấp/exp 42, Thú cưng 35, Nhân vật 36... JX NEXT hiện
  mới đăng ký **3** hàm — khoảng cách đó là M11–M13.
- **Hệ settings: 102 tệp** trong `\settings\` (+ 62 đường dẫn script). Đã ghi ra tệp, đã chỉ tệp nào
  cho hệ nào. Tìm nơi đọc + cột đọc bằng `xrefstr`/`func` (ví dụ `gamesetting.ini` @0x805EA59).
- ~~**Quy ước gọi**: hàm nhận đối số nguyên/thực trực tiếp~~ — **sai, đã sửa ở đợt 2** (là `int f(lua_State*)`,
  API Lua 4.0).

Dữ liệu kèm theo (text, ~100 KB): `docs/linux/jx_linux_luaapi.txt`, `s3relay_luaapi.txt`,
`jx_linux_luaapi_nhom.txt`, `jx_settings_files.txt`.

### 2026-09-17 (khuya) — console của `jx_zone` và gateway: từng dòng, tiếng Việt, có màu

Trước: console in đúng dòng JSON của tệp log — máy đọc thì tốt, người ngồi trước cửa sổ thì không.
Giờ tách đôi, theo đúng yêu cầu "thông báo phải rõ ràng từng dòng":

- **Tệp log** giữ nguyên JSON tiếng Anh (công cụ, `dev.py`, Loki… không đổi gì).
- **Console** in mỗi sự kiện một câu: `giờ  MỨC  [mảng]  câu · trường=giá trị · …`, mức log có màu
  (xanh / vàng / đỏ), Windows tự chuyển sang UTF‑8. Ví dụ thật:
  `14:45:22.734 THÔNG TIN [tài khoản] Đăng nhập thành công · tài khoản=smoke1 · mã tài khoản=3001 · phiên=…`
- Mã nguồn vẫn ghi `msg` tiếng Anh cố định; console tra **`config/log.vi.json`** — 147 câu, 12 mảng,
  159 tên trường. C++ (`jx::log`) và Go (`pkg/log`) dùng **chung một bảng, một định dạng**.
  Thiếu câu nào thì in tiếng Anh chứ không mất dòng. `log.lang = "en"` để xem tiếng Anh,
  `log.console_style = "json"` khi cần nối console vào công cụ.
- Lúc khởi động, **mỗi thiết lập đang dùng in một dòng** (sau khi gộp tệp, `JX_*`, tham số dòng
  lệnh) thay cho một khối JSON dài; khoá có `password/secret/token` hiện `***`.
- `python tools/check_log_catalog.py`: liệt kê câu / mảng chưa có tiếng Việt, thoát mã 1 — CI chạy
  lệnh này, nên thêm dòng log mới là phải thêm câu của nó.

Test: C++ 124/124 (Debug + Release, thêm test console), Go `pkg/log` + `pkg/config`, smoke thật trên
cổng lệch. Tài liệu: [LOGGING.md](LOGGING.md) §1b.

**CI lần đầu chạy thật** (commit `43a7d8c`): Linux GCC hỏng vì `Result.h` thiếu `<cstdint>` — MSVC
kéo header hộ nên Windows không thấy. `tools/check_includes.py` tìm loại lỗi này trong 1 giây (theo
chuỗi include của chính dự án), có `--fix`; nó tìm ra **48 chỗ**, đã thêm đủ, CI chạy nó trước khi
build. Job Windows "Test (Release)" chỉ nói "exit code 8": `tools/ci_annotate.py` chạy lại test hỏng
và biến từng `REQUIRE` hỏng thành annotation đọc được công khai.

### 2026-09-17 (tối) — M8 làm lại: luồng đăng nhập bản 2.0, khớp client thật 99,9 % điểm ảnh

Phiên trước dựng ba màn từ bố cục **đoán** (nó không tìm thấy các `.ini` của 2.0). Giờ bố cục thật
đã đọc được (kho lồng `\reslst.dat`), nên toàn bộ luồng được **viết lại từ mã nguồn cũ + mã máy
`gamecl.exe` 2.0**, không đoán chỗ nào:

| Việc | Kết quả đo được |
|---|---|
| Đối chiếu điểm ảnh với client 2.0 thật, 1024x768 | **Chọn Máy Chủ: 99,98 %** điểm ảnh lệch ≤ 11/255 (danh sách cụm 100 %, danh sách máy chủ 99,99 %, tiêu đề 100 %); **bảng chọn đầu (`KUiInit`): 99,88 %** (bốn nút 100 %, dải bản quyền 100 %). Phần lệch còn lại là lá rơi đang chuyển động và màu 16‑bit của bản cũ. |
| Bảy cửa sổ, đúng thứ tự của bản 2.0 | `KUiInit` → `KUiSelServer` → `KUiLogin` → `KUiConnectInfo` → `KUiSelPlayer` → `KUiSelNativePlace` → `KUiNewPlayer`, nền `KUiLoginBackGround` (lá rơi, logo, biển 18+). Ai mở ai, nút nào quay về đâu: theo `UiCase/*.cpp`. |
| Chữ | Font bitmap **của chính game** (`\font\vn\gbk_fs12/14/16.fnt`, định dạng ASF): mỗi ký tự cách nhau đúng `cỡ/2` px, viền chữ nằm sẵn trong glyph. Xuất thành BMFont (`chu-14.fnt` + `chu-14-vien.fnt`). |
| Câu thông báo | 89 câu `[InfoString]` của `\Ui\Setting.ini` ("Hiện đang kết nối với máy chủ"…), 752 chuỗi `stringtable_client.txt`, 8 tân thủ thôn, mô tả ngũ hành — đều lấy từ game, không tự viết. |
| Ảnh | Mỗi sprite một atlas + bảng khung hình (x, y, w, h, ox, oy). Cách cũ (mỗi khung một PNG 800x528) tốn 65 MB và ~760 MB VRAM cho 900 khung nhân vật; giờ **34 MB**, dáng nhân vật dùng chung một thư mục. |
| Test | Go 31 test (`jxold/...`, có fuzz font), Godot **95 kiểm tra giao diện** + 262 test thuần, e2e TCP + WebSocket qua luồng mới: **đạt**. Không có dữ liệu game (CI, máy mới clone) thì client tự lùi về `UiLoginPlain` và vẫn đăng nhập, tạo nhân vật, vào game được. |

**Những điều chỉ mã máy mới cho biết** (chi tiết ở [VLTK20-CLIENT.md](VLTK20-CLIENT.md) §6–§9):

- `KUiSelServer`: `LeftList` + `RightList` là **một** danh sách CỤM chia hai cột, 14 cụm mỗi cột;
  `IpList` mới là danh sách máy chủ của cụm đang chọn; `NameBigger` là tên cụm. Danh sách có ảnh
  nền từng dòng (`SprImg`), các dòng cách nhau **18 px** (hằng số trong mã, không phải `Font+1`),
  chữ bắt đầu ở `TextXStart` (mặc định 7). Máy chủ có chữ "(Đầy)" tô đỏ, "(Đề cử)" tô xanh.
- Danh sách máy chủ của 2.0 là `\UserDataCl\serverlist.ini`, XOR 0x32, cùng cấu trúc
  `[List]/[Region_n]` như mã nguồn cũ. Của ta: `client/config/serverlist.json` cùng hình dạng.
- Bảng màu của thẻ `<color=...>` trong `enginefree.dll` có 21 tên (Gold, Orange, Violet… mà JX1
  không có).
- Ảnh tên hệ ở màn tạo nhân vật ghép trong mã: `<PropertyBgImgPrefix>\<金|木|水|火|土>vn.spr`.
  Cấp nhân vật in theo `"LV:%d"`. Kim chỉ nam, Thủy chỉ nữ (giống mã nguồn cũ).
- Client thật mở ở `KUiInit` (bốn nút), không phải vào thẳng Chọn Máy Chủ như ghi chú trước.

**Chưa đối chiếu được bằng ảnh**: từ màn Đăng nhập trở đi. Client thật không nhận phím/chuột ảo
(`PostMessage`), còn các màn sau đăng nhập thì phải có tài khoản thật mới tới được — tôi không được
phép đăng nhập thay ai. Các màn đó dựng bằng **cùng bộ phần tử đã kiểm chứng** trên hai màn kia.
Nếu muốn chắc từng điểm ảnh: chụp giúp bốn màn đó ở 1024x768, tôi so bằng `tools/re` trong vài phút.

**Sửa lỗi của phiên trước gặp trên đường**: test Go đọc dữ liệu thật vỡ khi `JX_OLD_CLIENT` trỏ vào
client 2.0 (chúng mặc định client nào cũng có `package.ini`). Giờ mỗi test gọi
`oldgame.ClientJX1()` / `ClientVLTK20()` / `ServerJX1()` — nói rõ loại thư mục mình cần, tự tìm qua
biến môi trường, `config/oldgame.local.json`, `bin/`, và bỏ qua nếu không có.

**Phía server của luồng tạo nhân vật** (commit kế tiếp): `CharCreateReq.native_place` +
`RoleData.native_place` (mã map của tân thủ thôn đã chọn); kho dữ liệu nhận một giá trị
`persist.NewCharacter` và **tự kiểm tra lựa chọn như game gốc**: 5 hệ, 2 giới, Kim chỉ nam, Thủy chỉ
nữ — cửa sổ có chặn thì server vẫn phải chặn, vì client viết lại được. Tên nhân vật **không được có
khoảng trắng** ở bất kỳ đâu (đúng `KUiNewPlayer::GetInputInfo`, câu thông báo 17). `jxbot` tạo nhân
vật theo đúng luật đó.

Xem ảnh: `python tools/dev.py assets` rồi
`godot --path client --resolution 1024x768 res://scenes/UiShell.tscn -- --shot=chon-may-chu`
(các tên khác: `bat-dau`, `dang-nhap`, `thong-bao-ket-noi`, `chon-nhan-vat`, `chon-tan-thu-thon`,
`tao-nhan-vat`) → `user://logs/ui_<tên>.png`.

Còn lại của phần giao diện đăng nhập: bàn phím ảo (`虚拟键盘.ini` đã xuất, chưa dựng), hai bảng
"Tùy Chọn Hệ Thống" và "Xem Ghi Hình" của `KUiInit`, âm thanh nút bấm (`UiSoundPlay`).

### 2026-09-17 (chiều) — rà lại việc của phiên trước: ba lỗi thật, sửa xong cả ba

Chủ dự án yêu cầu **kiểm tra lại mã đã đẩy lên trước khi làm tiếp**. Tìm ra ba lỗi, cả ba đều có
bằng chứng đo được, không phải nhận xét về phong cách.

**Lỗi 1 — CI chưa từng chạy.** `.github/workflows/next-ci.yml` dòng 131 viết
`run: "$JX_GODOT" --headless ...`: một chuỗi trong nháy kép mà còn chữ đằng sau thì **không phải
YAML**. Cả tệp workflow vô hiệu, nên từ M5d tới U5 (17 commit) GitHub báo mọi lần đẩy là *failed*
mà **không chạy job nào** — build Linux/GCC, `go test -race`, fuzz, e2e đều chưa hề được kiểm.
Các dòng "CI xanh" trong nhật ký cũ là sai. Đã sửa (block scalar), thêm bước `UiCheck`, và cho CI
chạy cả trên nhánh `claude/**`. `gateway.go` cũng chưa `gofmt` — CI sẽ chặn đúng chỗ đó.

**Lỗi 2 — N1 để lại "bóng ma".** Bản N1 cắt **mỗi gói** xuống 100 phiên gần nhất. Hậu quả, tái hiện
bằng test mô phỏng đúng những gì client nhận (`tests/test_KInterest.cpp`), chạy trên mã cũ:

```text
người mới vào được gửi MỌI THỨ trong tầm nhìn, nhưng tin "có người mới" chỉ tới 100 phiên
một người rời đám đông 40 người (giới hạn 8)  -> 20 client vẫn giữ bóng ma của họ
400 tick hỗn loạn (đi, đánh, vào, ra)          -> 1 865 entity "ma" còn nằm trong các client
gói chết / máu / chat cũng bị cắt              -> quái đứng sống mãi trên máy người ở xa
```

Bản cũ của Kingsoft sống được với luật này vì client tự hỏi lại thứ nó không biết và tự xoá thứ im
lặng. Ta không có hai cơ chế đó, nên sửa tận gốc: **giới hạn nằm ở điều mỗi client BIẾT**.
`KViewer` (mỗi phiên một cái) giữ danh sách entity client đã được báo spawn; mỗi entity giữ
`watchers` = các phiên biết nó. Mọi cập nhật và gói despawn đi tới **đúng** `watchers`. Tổng lưu
lượng vẫn bị chặn y như cũ (tổng các danh sách ≤ số người × 100) nhưng không gì có thể lệch.
Thêm `max_known_npcs` 300, `interest_period` 4 tick, `view_slack` 1 ô (đi dọc mép ô không còn
hiện‑mất liên tục), `spawn_budget` 48 — chính cái cuối cùng là **N4**: người bước vào chỗ đông được
báo dần qua vài tick, gần nhất trước.

**Lỗi 3 — bộ đọc `.pak` bỏ sót 10 267 tệp của client 2.0**, xem mục dưới.

**Đo lại, 3 000 bot trên MỘT map, bản Release, cùng máy:**

| | Trước (N1 bản đầu) | Sau |
|---|---:|---:|
| tick trung bình lúc đông nhất | 9,08 ms | **7,32 ms** |
| p99 tệ nhất (kể cả lúc người ùa vào) | **268 ms** | **16,78 ms** |
| tick dài nhất | 325 ms | **17,31 ms** |
| tick bị rớt | 7 | **0** |
| đăng nhập lỗi / bị đá / khung bị bỏ | 0 / 0 / 0 | 0 / 0 / 0 |

Trước khi tối ưu, pha *interest* tốn 37 ms mỗi tick ở 3 000 người (benchmark ẩn `[.bench]` trong
`test_KInterest.cpp`, có in chi phí từng pha). Ba thay đổi theo đúng số đo đưa nó về 11,9 ms:
lưới `KRegionGrid` xếp người chơi riêng với NPC và xoá O(1) theo ô nhớ; bước "ai đã ra khỏi tầm"
tính ô từ toạ độ thay vì tra bảng băm; client đã đầy chỉ tìm người gần hơn để đổi chỗ mỗi ~0,9 giây
và chỉ trong bán kính cần thiết.

**`dev.py` chạy song song được.** `JX_PORT_OFFSET=1000` dời mọi cổng (zone 18001, gateway
18100/18102): hai checkout trên một máy không còn giành cổng 17001, và `start` báo rõ khi cổng đã
bị chiếm thay vì "zone did not open port".

**Test:** 123 ctest (4 test interest mới + 1 test chat viết lại), `go vet` + `go test`, Godot 262,
e2e TCP + WebSocket — tất cả xanh.

### 2026-09-17 (trưa) — kho lồng `\reslst.dat`: client 2.0 có đủ bố cục đăng nhập

**Kết luận "ba màn còn lại là Flash" của mục U5 bên dưới là SAI.** `update.swf` chỉ là cửa sổ
launcher. Màn đăng nhập của 2.0 là cửa sổ C++ như JX1; tệp bố cục của nó nằm trong một kho mà bộ
đọc của ta không biết.

Cách tìm ra: không đoán nữa mà **theo dõi chính game đang chạy**. `tools/re/filetrace.py` chạy
`gamecl.exe` dưới debug API, đặt breakpoint trong `KPakFile::Open` của `engineFree.dll` và ghi lại
tên từng tệp game xin mở cùng nơi nó được tìm thấy:

```text
game mở 13 tệp .pak (kho #0..#12) nhưng tìm thấy 232 tệp ở "kho #13"
14  pak#11 size=13477978  \reslst.dat          <- nằm TRONG font.pak
    pak#13 elem=3875       \Ui\ui3_1024\UiNewLogin\开始.ini
```

`\reslst.dat` là một tệp `PACK` hoàn chỉnh (10 267 mục: 6 738 script Lua, 1 161 `.ini`, 840 bảng
tab) lưu thành 7 mảnh 2 MB. Cờ `0x10000000` mà mã JX1 gọi là "chỉ dùng cho sprite" thì engine 2.0
dùng cho mọi tệp lớn. Sửa ở `pkg/jxold/pak/XPackFile.go`; `jxassets check-trace` đối chiếu với game
thật: trước **225/490 tệp game thấy mà ta không thấy**, sau **khớp 503, thiếu 0, thừa 0**.

Hệ quả ngay: client dự phòng (`bin/Client`) **không còn phải cấp tệp nào** — bảng nhân vật chính,
bảng động tác, `BaseValue.ini`, bảng `*Res.txt` giờ đều lấy từ 2.0 thật. Chi tiết, địa chỉ trong
nhị phân, 14 bố cục đăng nhập và nghĩa của `PositionType`: [VLTK20-CLIENT.md](VLTK20-CLIENT.md).

### 2026-09-17 — U5: mổ client 2.0 thật, và bỏ cách tra theo tên tệp

**Việc trước làm sai.** Bốn màn hôm nay lấy bố cục JX1 rồi dán ảnh 2.0 — đó không phải giao diện
2.0. Chủ dự án gửi ảnh chụp client 2.0 thật và nó khác hẳn.

**Mổ nhị phân.** `gamecl.exe` và `katgame.dll` bị **UPX nén** (section tên `UPX0`/`UPX1`, entropy
7,91; PackHeader `UPX!` ở 0x3e0: method 8 = NRV2B_LE16, 1.285.044 → 34.588.706 byte). Đó là lý do
quét chuỗi thẳng ra rỗng. `rainbow.dll` còn sót `D:\newBuilder\projects\jxvn20\code\product\win32\server\rainbow.pdb`.

**Nhưng chìa khoá không ở exe.** Kho `.pak` **không lưu tên tệp, chỉ lưu mã băm**
(`KPakList::FileNameToId`), nên không cách nào liệt kê. Thêm `pak.Set.ScanText`: giải nén *mọi* mục
rồi giữ mục nào là văn bản.

```
scan-text: 4130 tep van ban trong 13 kho (client 2.0), 2031 duong dan duoc goi ten
```

Ra được:

- **Giao diện 2.0 là `Ui4`**, không phải `Ui3`: `\Spr\Ui4\主界面\登入界面\...`, tên có đuôi `vn`.
- **Màn tạo nhân vật 2.0**, khung **1024x768**: 5 thẻ dọc 103x38 ở x=0, y=78/116/154/192/230
  (`金选项vn.spr`…), nam ở −38, nữ ở +262, ô tên 436,670 `MaxLen=16`, Xác Định 562,666 62x29,
  Huỷ 642,666, mô tả 314,30 270x38. Nền là `JX20登录1024.spr`.
- **Bảng chữ tiếng Việt** (TCVN3) nằm trong pak: `G_STR_CANCEL → Hủy bỏ`, `G_MSG_EXCHANGE_MAINTAIN
  → Server đang bảo trì...`.

**Ba màn còn lại của bản 2.0 không phải giao diện C++ — chúng là Flash.** `update.swf` (CWS, zlib,
giải nén 1.919.352 byte) chạy qua `flash.ocx` + `stmocx.dll`, chứa **43 ảnh (1,8 MB)**, 29 ô nhập
chữ, 67 sprite, và đúng các ký hiệu ActionScript của ảnh chụp: `winSelectServerMain`,
`WinChooseServer`, `btnChooseServer1/2/3`, `btnMoveServerUp/Down`, `Act_selectServer`, `Startgame`.
Kèm `ServerListUrl=http://jx1-auto.xoyocdn.com/serverlist/jxvn20/`. Vì vậy **không tồn tại** `.ini`
cho màn menu, chọn máy chủ và ô đăng nhập của bản 2.0.

**Bỏ hẳn cách tra theo tên tệp.** Chủ dự án chê `{"login", "\xb5\xc7\xc2\xbd.ini"}` khó đọc — và
hoá ra còn sai hướng, vì pak không có tên. Giờ mỗi màn được nhận ra bằng **chữ ký các section**:

```go
{"tao-nhan-vat", "Tạo nhân vật", []string{"newplayer", "name", "male", "female",
                                          "gold", "wood", "water", "fire", "earth"}},
```

Client nào có hai bản cùng một màn (2.0 có bản 800x600 và 1024x768) thì **khung lớn hơn thắng**,
vì đó là bản game thật chạy. Không còn byte escape nào trong mã nguồn.

**Ảnh xuất ra tên tiếng Việt**, đặt theo ô nó thuộc về, không phải mã băm:

```
client/assets/ui/tao-nhan-vat/bo-cuc.json
                             /the-kim.png  the-kim-nhan.png
                             /nut-xac-dinh.png  nut-xac-dinh-nhan.png  nut-xac-dinh-re-chuot.png
                             /vai-kim-nam-1.png  vai-thuy-nu-2.png ...
```

`files[]` trong JSON giữ bảng tra ngược `đường dẫn game cũ → tệp mới`, nên vẫn lần lại được nguồn.

**Kiểm tra:** `tests/UiCheck.tscn` 41 kiểm tra (chốt khung 1024x768, 5 thẻ 103x38 đúng chỗ,
nam −38 / nữ +262, 30 ảnh nhân vật); `KUiExport_test.go` chốt lại phía Go. Chạy thật `dev.py start`
+ client: màn tạo nhân vật lên đúng như ảnh chụp bản 2.0.

**Còn lại:** ba màn Flash. Ảnh nằm trong `update.swf`, phải rút 43 ảnh đó ra rồi dựng lại bố cục
trong Godot — bản 2.0 không có tệp bố cục cho chúng.

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
