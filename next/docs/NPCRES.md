# Sprite nhân vật / NPC (npcres)

Cách game cũ vẽ nhân vật (`KNpcRes`, `KNpcResNode`, `KNpcResList`, `KNpcTemplate`) được port nguyên: bảng dữ
liệu đọc từ `Settings/npcs.txt` + `Settings/npcres/*.txt` (trong pak `slistcache.pak`), sprite từ `spr.pak`.

## 1. Xuất dữ liệu

```text
python tools/dev.py assets [map ids]         # export-map + export-npcres cho các map đó
jxassets export-npcres 1 -templates 1000,1001 -out client/assets
```

`export-npcres` xuất NPC được đặt trên các map (Npc_C.dat → template id) + hai nhân vật chính (`MainMan`,
`MainLady`) + các template liệt kê ở `-templates` (zone dùng 1000..1003 cho NPC test). Chỉ sprite của các
"doing" đứng/đứng 2/đi/chạy được xuất ở giai đoạn này (`s` rỗng = chưa xuất; client vẽ vòng tròn thay).

```text
client/assets/npcres/npcs.json        templates {id → name (UTF-8), res, stand_frame, stand_frame1, walk_frame,
                                      run_frame, death_frame, helm/armor/weapon/horse, ride}, player {male/female:
                                      stand/walk/run_frame từ BaseValue.ini}, npc_actions[14], actions[75]
client/assets/npcres/res/<tên>.json   một KNpcResNode (xem dưới)
client/assets/sprites/<id8>.png/.json atlas dùng chung với map
```

## 2. Hai loại nhân vật (`人物类型.txt`, cột CharacterType)

- **NormalNpc** (NPC, quái, thú): một ảnh cho mỗi *doing* (`npc动作表.txt`, thứ tự = enum `CLIENTACTION` của
  `KNpc.h`: FightStand, NormalStand1, NormalStand2, FightWalk, NormalWalk, FightRun, NormalRun, Wound, Die,
  Attack1, Attack2, Magic, SitDown, JunpFly). File lấy từ `普通npc资源.txt`, khung/hướng/interval từ
  `普通npc资源信息.txt` ("48,8,200"), bóng = `<tên>b.spr`. `res/<tên>.json`: `actions[14] {s, frames, dirs,
  interval, shadow}`.
- **SpecialNpc** (`MainMan`/`MainLady`): ghép từ **bộ phận** (`部件列表.txt`: Head 0, Hair 1, Shoulder 4, Body 5,
  LeftHand 6, RightHand 7, LeftWeapon 8, RightWeapon 9, HorseFront/Middle/Back 12–14, Mantle 16; chỉ số =
  nhóm×4 + vị trí). Mỗi bộ phận có bảng `<bộ phận>.txt` (hàng = số trang bị, cột = 75 action của
  `动作编号表.txt`) và `<bộ phận>信息.txt`. Doing → action qua `未骑马关联表.txt` (hàng = loại vũ khí, cột =
  14 doing) hoặc `骑马关联表.txt` khi cưỡi ngựa. `res/<tên>.json`: `parts[] {index, name, equips {"<trang bị>":
  [75 × {s, frames, dirs, interval, color}]}}`, `no_horse[][]`, `on_horse[][]`, `sort`, `shadow[75]`, `equips`
  (bộ trang bị đã xuất, xem dưới).

Số hàng trang bị của mỗi nhóm bộ phận **không** phải 0 khi không mặc gì: game cũ lấy từ `g_ItemChangeRes`
(`KItemChangeRes.cpp`, bảng `Settings/item/{HelmRes,ArmorRes,MeleeRes,RangeRes,HorseRes}.txt`):
`GetHelmRes(0,0)` = ô (2,2) của HelmRes − 2 = **18**, `GetArmorRes(0,0)` = **18**, `GetWeaponRes(0,0,0)` = **0**
(tay không), ngựa −1, phi phong −1 (`KItemList.cpp:1054`, `KPlayerDBFuns.cpp:402`). Port: `KItemChangeRes.go`,
`DefaultEquips()`; khi mặc đồ sẽ tính từ item như `KItemList.cpp:733` (chưa làm). Tên nhân vật đặt cao
`m_nStature` (+84 với người chơi) trên chân (`KNpc::GetNpcPate`).

## 3. Vẽ (client `KNpcRes.gd`, `KNpc.gd`, `KNpcResNode.gd`, `KMath.gd`)

- Vị trí: mọi ảnh vẽ theo **điểm tham chiếu** = tâm sprite (`CenterX/Y` trong header spr; nếu 0 và rộng >160 thì
  (160,192)) đặt tại chân nhân vật (`REF_SPOT` như `KRepresentShell2`), cộng offset của khung.
- Hướng: 64 hướng (`g_GetDirIndex`: 0 = xuống, theo chiều kim đồng hồ; bảng sin sinh lại vì bản cũ đọc từ USB
  key). Hướng vẽ `res_dir` quay dần về hướng thật mỗi khung (`KNpc::Paint`). Sprite `dirs` hướng lấy
  `(dir + 32/dirs) / (64/dirs)`.
- Khung: đếm `cur_frame` mỗi tick 18 Hz tới `total_frame` (StandFrame/WalkFrame/RunFrame của template; người
  chơi lấy `BaseValue.ini`), rồi `frame = dir_no × (frames/dirs) + (frames/dirs) × cur_frame / total_frame`
  (`KNpcRes::Draw`). Các bộ phận khác dùng đúng khung của bộ phận đầu tiên. Đứng xong một chu kỳ có 1/6 xác
  suất chuyển sang NormalStand2 (`KNpc::OnStand`).
- Thứ tự vẽ bộ phận: `贴图顺序表.txt` (`CSortTable`): `[DEFAULT] Dir1..16` + section riêng từng action (Dir hoặc
  Line "<khung>,<bộ phận>...") → `sort.default`, `sort.acts`. Bóng vẽ trước; bộ phận 10/11 (hiệu ứng vũ khí)
  bỏ qua như code cũ. Thứ tự con của node = thứ tự vẽ.
- Nhân vật là lá runtime của cây KIpoTree (điểm chân + 6) nên xếp đúng trước/sau nhà cửa.

## 4. Chưa làm

Hoạt ảnh tấn công/bị đánh/chết/ngồi, trang bị theo `RoleData`, ngựa, phi phong, hiệu ứng trạng thái, đổi màu
(`ChangeColor`), NPC không có trong `人物类型.txt` (một số `critter0xx`) — hiện vẽ vòng tròn; tối ưu bộ nhớ
texture (mỗi sprite 120 khung ≈ 0.5–2 MB RGBA).
