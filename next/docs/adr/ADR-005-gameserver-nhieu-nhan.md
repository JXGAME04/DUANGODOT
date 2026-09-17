# ADR-005: GameServer 64-bit nhiều nhân — chủ sở hữu, hàng lệnh, worker pool

- **Trạng thái**: đã chốt và đang chạy (2026-09-17)
- **Nguồn**: MASTER SPEC "GAMESERVER C++ 64-BIT MULTI-CORE ARCHITECTURE" của chủ dự án
- **Mã**: `server/core`, `server/entity`, `server/zone/{KMapInstance,KWorldScheduler,KWorldCommand,KPathFinder}`,
  `services/internal/gateway/KSendQueue.go`

## Bối cảnh

Bản cũ: mỗi tiến trình một vòng lặp, mọi thứ trong một luồng; muốn thêm người chơi thì chỉ còn cách
mua CPU nhanh hơn. Máy hiện tại có 24 luồng mà server chỉ dùng được một. Đồng thời có những kiểu
sai không được phép lặp lại: một luồng cho mỗi người chơi, một mutex khoá cả thế giới, luồng mạng
sửa thẳng trạng thái game, truy vấn CSDL chặn tick.

## Quyết định

**1. Thread chỉ sinh ra ở một chỗ.** `core::ThreadPool` là nơi duy nhất tạo luồng; số lượng lấy từ
cấu hình hoặc `hardware_concurrency`, không ghim core, `stop()` join sạch, ngoại lệ trong task không
giết tiến trình. Không có luồng cho mỗi người chơi / NPC / map (§2, §14–16, §87, §88).

**2. Một thực thể sửa được = một chủ sở hữu.** Một `KMapInstance` (một bản chạy của map) tại mỗi
thời điểm do **đúng một** worker tick. Ai muốn tác động phải đẩy lệnh vào inbox của nó; thứ gì world
cần từ bên ngoài thì đi ra bằng sự kiện. Vì vậy **không có mutex khoá thế giới** (§6, §19, §73).

**3. Luồng mạng không sửa world.** `KGameServer` giải mã gói rồi đẩy `KCmdSpawnPlayer` /
`KCmdClientPacket` / `KCmdRemovePlayer` / `KCmdSaveRequest`; kết quả về bằng `KEvSessionOpened` /
`KEvPlayerSave` / `KEvWorldChange`. Một tick: chạy song song các map → gom sự kiện → gửi gói tin,
đúng thứ tự đó nên client luôn nhận `ChangeMap`/ack trước gói spawn của map mới (§22, §23, §30).

**4. Scheduler xếp map cho worker, không ghim.** `KWorldScheduler` đo chi phí tick thật của từng map
và dời map khỏi worker nóng, có ngưỡng chống dao động (§8, §9).

**5. Handle có generation.** `EntityId` = `index(32) | generation(32)`; ô nhớ tái dùng thì generation
tăng nên handle cũ chết hẳn. `EntityTable` là nơi duy nhất tạo/xoá entity (§5, §36).

**6. Job system quyết định *chạy ở đâu*, không quyết định *ai được sửa*.** `parallel_for`/`run_all`
kết thúc bằng barrier; job đọc dữ liệu và trả kết quả hoặc command buffer, không ghi thẳng vào
world (§17, §18, §23).

**7. Mọi thứ đều đo được từ đầu.** `core::Metrics` (counter/gauge/timing có histogram → avg/P95/P99),
`TickProfile` đo 15 pha của §22, log `zone.tick` in tải từng worker và chi phí từng pha của map nặng
nhất (§51–53, §96).

**8. Tối ưu chỉ sau khi profiler chỉ chỗ** (§R). Ba thứ đã sửa theo đúng số đo, không theo cảm giác:
NPC không ai nhìn thấy thì không nghĩ (§44/§45), tìm đường dùng lại bộ đệm thay vì cấp phát mảng cỡ
bản đồ mỗi truy vấn (§47/§48), hàng gửi bỏ gói vị trí đã lạc hậu thay vì ngắt người chơi (§70).

## Số đo (máy 24 luồng, Phượng Tường + 3 map, 18 Hz, ngân sách 55 ms/tick)

| Bước | tick trung bình | ghi chú |
|---|---|---|
| Một luồng (trước) | 25,1 ms | 3076 entity, 1 người chơi |
| 4 worker | 16,0 ms | mỗi map một worker |
| + entity ngủ | **1,37 ms** | chỉ 42/3076 entity thức |
| 200 người cùng một chỗ | 11,9 ms → **3,35 ms** | sau khi sửa tìm đường |
| 500 người cùng một chỗ | **14,05 ms** (p95 25,2, p99 29,0) | không ai rớt, 7 triệu gói hành động / 29 s |

## Phương án đã cân nhắc

- **Một luồng cho mỗi map, ghim cứng**: đơn giản nhưng map đông sẽ kẹt vĩnh viễn ở một core và không
  cân bằng lại được khi người chơi dồn về một chỗ.
- **Khoá toàn world bằng một mutex rồi chạy nhiều luồng**: đúng về mặt an toàn, vô nghĩa về hiệu năng.
- **ECS toàn phần**: chưa cần; `EntityTable` + dữ liệu liền kề đã cho phần lớn lợi ích mà không phải
  viết lại toàn bộ gameplay (§78).

## Hệ quả

- Thêm hệ thống mới phải theo luật: đọc world ở pha song song, **ghi** chỉ trong pha commit của chủ
  sở hữu; muốn tác động sang map khác thì gửi lệnh.
- Lua: mỗi map instance một `KScriptCache` riêng, `g_ScriptContext` là `thread_local` (§42).
- Còn lại (đã đo là chưa cần, nhưng kiến trúc đã chuẩn bị): **chia region cho một map cực đông**
  (§11–13, §P) để nhiều worker cùng xử lý một map, và mức độ quan tâm theo khoảng cách (§26, §27).
