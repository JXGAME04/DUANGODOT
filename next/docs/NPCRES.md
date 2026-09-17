# Sprite nhân vật / NPC (npcres)

Cách game cũ vẽ nhân vật (`KNpcRes`, `KNpcResNode`, `KNpcResList`, `KNpcTemplate`) được port nguyên: bảng dữ
liệu đọc từ `Settings/npcs.txt` + `Settings/npcres/*.txt` (trong pak `slistcache.pak`, với client 2.0 là `slistcl.pak`),
sprite từ `spr.pak`. Template ghép **dòng server** (mô phỏng) + **dòng client cùng id** (hình: `NpcResType`, trang bị,
`Stature`, khung đứng/đi/chạy) như client cũ tra bảng của nó — `export.MergeAppearance`, xem [REFERENCES.md](REFERENCES.md) mục 3.
Bảng nhân vật chính, `动作编号表.txt`, `BaseValue.ini`, `item/*Res.txt` client 2.0 không có → lấy từ client dự phòng (được log).

## 1. Xuất dữ liệu

```text
python tools/dev.py assets [map ids]         # export-map + export-npcres cho các map đó
jxassets export-npcres 1 -templates 1000,1001 -out client/assets
```

`export-npcres` xuất NPC được đặt trên các map (Npc_S.dat của pak server + Npc_C.dat của client → template id,
xem MAPS.md mục NPC) + hai nhân vật chính (`MainMan`, `MainLady`) + các template liệt kê ở `-templates`
(zone dùng 1000..1003 cho NPC test). Chỉ sprite của các
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

## 4. Đánh / bị đánh / chết (vòng combat tối giản, nhịp khung 1‑1 với `KNpc.cpp`)

Zone chạy **18 tick/giây** (`zone.tick_hz`, bằng vòng lặp logic cũ) nên các cột *Frame* của `npcs.txt` là số tick.
Zone đọc `npcres/npcs.json` (`KNpcTemplate.h/.cpp`: `attack_frame` = cột **AttackSpeed** như `KNpcTemplate::Init`,
`hurt_frame`, `death_frame`, `hit_recover`, `revive_frame`; người chơi lấy `BaseValue.ini` [Common] AttackFrame 18 /
HurtFrame 12).

- Client click vào quái → `C2G_ATTACK{target}`. Zone (`KSubWorld::attack_request`): quái còn sống và không phải NPC
  thành; nếu ngoài tầm (`kMeleeReach` 96 đơn vị, tạm cho tới khi vũ khí mang tầm đánh) thì **đi tới** trước
  (`approach`, tối đa 5 lần như client cũ đi rồi mới đánh), tới nơi thì vung.
- `start_attack` (`KNpc::DoAttack`): `frame_total = AttackFrame·100/(100+tốc độ đánh)`, quay mặt về mục tiêu
  (`g_GetDirIndex`), gửi `EntityAction{ATTACK, frames, dir}`. Client chọn ngẫu nhiên Attack1/Attack2 (50/50) như cũ.
- Mỗi tick (`KNpc::OnSpecial1`): hết khung → đứng và **tự vung tiếp** khi mục tiêu còn sống trong tầm; đúng khung
  `60 %` (`ATTACKACTION_EFFECT_PERCENT`) thì trúng đòn → `EntityLife{life, delta}`.
- Trúng đòn (`KNpc::DoHurt` phía server): HitRecover ≥ 100 → không giật; xác suất giật =
  `50 + HitRecover·50/100 %`; số khung giật = `HurtFrame·(100−HitRecover)/100` → `EntityAction{HURT, frames, pos}`,
  client dừng tại `pos` và chạy hoạt ảnh Wound; hết khung → đứng (`OnHurt`).
- Chết (`KNpc::DoDeath`): `EntityAction{DEATH, DeathFrame}`; client chạy Die rồi **giữ khung cuối** (xác);
  người chơi ngoài chế độ chiến đấu giữ 1 máu (luật cũ). Sau DeathFrame tick zone rút xác khỏi vùng nhìn
  (`DoRevive`, `EntityDespawn`), sau `ReviveFrame` tick (mặc định 2400 ≈ 133 s) hồi sinh tại chỗ đặt với đầy máu
  (`Revive` → `EntitySpawn`). `EntityInfo.doing/doing_frames` cho người vào sau thấy xác/đòn đang vung.
- Đi (`C2G_MOVE`) hủy đòn đang vung; đang giật/chết không đi được.

### Máu / sát thương / kinh nghiệm thật — level script qua Lua 5.4 (`KLuaScript`, `KScriptCache`)

Zone nhúng **Lua 5.4 chuẩn** (vcpkg `lua` 5.4.8). `KLuaScript` (tên theo `Engine/Src/KLuaScript`) = một trạng thái Lua cho
mỗi file script, có `Include`, `print` → log, và **lớp tương thích Lua 4.0** (`getn`, `floor`, `strfind`, `strsub`, `mod`… là
hàm toàn cục) nên script cũ chạy **nguyên văn**. `KScriptCache` (= `g_GetScript`) nạp một lần theo đường dẫn `\script\...`
dưới `zone.script_root` (thư mục server cũ; `dev.py start` đặt `JX_ZONE__SCRIPT_ROOT` từ `config/oldgame.local.json` →
`D:\ServerLinux\server1`).

`KNpcTemplateSet::level_data` = `KNpcTemplate::InitNpcLevelData` nguyên văn cho mỗi (template, cấp, ngũ hành), cache trong
`KSubWorld` như `g_pNpcTemplate[id][level]`: script = cột `LevelScript` (không có → `\script
pclevelscript
pclevelscript.lua`);
`Level1..4` → `GetNpcLevelData(series, level, "Level1", "a|b")`; `Exp/Life/AR/Defense/MinDamage/MaxDamage` =
`Param × GetNpcKeyData(series, level, tên, Param1, Param2, Param3) / 100` (chú ý tên `"AR"` chứ không phải `"AttackRating"`
→ đa số script rơi xuống công thức bậc hai `P1·L² + P2·L + P3`, đúng như bản cũ); `LifeReplenish`, `*Resist` qua
`GetNpcLevelData` với ô chuỗi; máu 0 → 100, AR 0 → 100. Không có `script_root` hoặc thiếu file → số tạm như trước (log cảnh báo).

Đòn trúng (`hit`): `KNpc::CheckHitTarget` — tỉ lệ = `AR·100/(AR + phòng thủ)`, kẹp [5, 95] %; trượt = "闪过攻击" không có gì
xảy ra. Rồi `CalcDamage(damage_physics)`: sát thương ngẫu nhiên `[min, max]` của người đánh, trừ kháng vật lý (tối đa 95 %);
`m_CurrentLife -= dmg`, **chết chỉ khi xuống dưới 0** (đòn để lại đúng 0 máu chưa chết — luật cũ); giật khi dmg > 0. Hồi máu
tự nhiên `KNpc::ProcessState` mỗi `GAME_UPDATE_TIME` = 10 khung cộng `LifeReplenish`.

**Còn tạm**: số của người chơi (10–20 sát thương, AR 100, phòng thủ 0, hồi máu `(cấp+5)/6`) chờ `KPlayer` thuộc tính/trang bị;
sát thương quái chưa cộng attrib của kỹ năng (`KSkill`/`KMissle`), chưa có khiên/phản đòn/nội lực/PK rate. Test:
`test_KLuaScript.cpp` (Lua 4 trên 5.4, Include, cache, `level_data` theo `InitNpcLevelData`, script thật của bản Linux),
`test_KSubWorld.cpp` "melee attack", client `--auto` tự đi đánh quái gần nhất (`AUTO_FIGHT`).

### Quái đánh trả — `KNpcAI` (port `KNpcAI.cpp` phần server, `server/zone/src/KNpcAI.cpp`)

Mỗi tick, NPC có `AIMode` (cột của `npcs.txt`, dữ liệu server Linux) chạy `KNpcAI::activate` **trước** phần cập nhật khung
(như `KNpc::Activate` → `NpcAI.Activate` khi `m_ProcessAI`), nhưng chỉ khi đang đứng/đi: đang vung, giật, chết thì AI tắt
(`DoSkill/DoHurt/DoDeath` xoá `m_ProcessAI`, `OnSkill/OnHurt` bật lại). Quyết định cách nhau `AIMaxTime` tick (`m_NextAITime`).

| Mode | Loại | Tham số `AIParam1..10` (= `m_AiParam[0..9]`; `[10]` = bán kính kỹ năng xa nhất², tính từ `skills.txt`) |
|---|---|---|
| 1 | chủ động | [0] tỉ lệ tuần tra khi không có địch; [1..4] tỉ lệ dùng kỹ năng 1..4; [5],[6] đứng/tuần tra khi địch xa |
| 2 | chủ động + hồi máu | [1] % máu, [2] tỉ lệ xử lý, [3] tỉ lệ hồi máu (kỹ năng 1, tối đa [9] lần) hoặc **bỏ chạy**; [4..6] kỹ năng 2..4; [7],[8] xa |
| 3 | chủ động + liều | như 2 nhưng [3] = tỉ lệ dùng kỹ năng 1 tấn công thay vì hồi máu |
| 4 | bị động | chỉ đánh kẻ đã đánh mình (`m_nPeopleIdx`, đặt trong `ReceiveDamage`); [1..4] kỹ năng, [5],[6] xa |
| 5 / 6 | bị động + hồi máu / liều | như 2 / 3 |

Các hàm con đúng tên cũ: `KeepActiveRange` (xa gốc quá `ActiveRadius` → về gốc, bán kính tạm giảm một nửa), `GetNearestNpc`
(quét ô 32 đơn vị trong `VisionRadius` theo đúng thứ tự cột/hàng/4 góc của bản cũ, quan hệ `relation_enemy` qua
`KNpcSet::GenOneRelation` = `g_GenOneRelation`: camp `begin` chỉ bị **thú** (`camp_animal`) đánh, `dialoger` không bao giờ,
cùng camp là đồng minh), `InEyeshot`, `CommonAction` (80 % về gốc, 20 % điểm ngẫu nhiên trong nửa `ActiveRadius`; dialoger đứng yên),
`FollowAttack` (≤ 32 → lùi ra `MINI_ATTACK_RANGE`; ≤ bán kính kỹ năng đang chọn và thấy → `do_skill`; còn lại đi tới địch),
`KeepAttackRange` (dùng `g_DirCos/g_DirSin` 64 hướng, bảng sinh lại trong `KMath.h`), `Flee` (đi tới `2·mình − địch`),
`SetActiveSkill` (ô Skill1..4 có cấp; `Level` "a|b" = a + b·cấp NPC như `GetData` của level script; bán kính từ `skills.txt`).
Lệnh `do_walk/do_stand/do_skill` = `KSubWorld::walk_to/do_stand/cast_skill`; kỹ năng cận chiến vung `AttackFrame`, kỹ năng khác
`CastFrame`; đòn trúng ở 60 % khung nếu mục tiêu còn trong tầm + 32 (trượt thì log `swing missed`). Tốc độ quái =
`WalkSpeed` đơn vị/khung (`ServeMove`) = `WalkSpeed × 18`/giây. Người chơi tạm ở camp `begin` (RoleData chưa mang `iteam`).

Chưa 1‑1: quái đi bằng A* của map thay cho `KPathFinder::GetDir` (lách vật cản tại chỗ); hồi máu = 1/5 máu tối đa (chờ công thức
kỹ năng); kỹ năng tầm xa vẫn trúng tức thì (chưa có `KMissle`); PK giữa người chơi chưa có (hai người chơi không bao giờ là địch).
Test: `test_KNpcAI.cpp` (6 ca: bảng quan hệ, chủ động đuổi–đánh, bị động chỉ đánh trả, camp justice tha người mới,
`KeepActiveRange`, bỏ chạy, thứ tự `GetNearestNpc`).

## 5. Chưa làm

Hoạt ảnh tấn công/bị đánh/chết/ngồi, trang bị theo `RoleData`, ngựa, phi phong, hiệu ứng trạng thái, đổi màu
(`ChangeColor`), NPC không có trong `人物类型.txt` (một số `critter0xx`) — hiện vẽ vòng tròn; tối ưu bộ nhớ
texture (mỗi sprite 120 khung ≈ 0.5–2 MB RGBA).
