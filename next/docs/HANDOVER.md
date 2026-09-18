# Bàn giao JX NEXT — trạng thái ngày 2026-09-18 (đồng hồ máy; các mục ghi 2026-09-19 phía dưới cùng một đợt làm việc)

Tài liệu này trả lời ba câu: **đang ở đâu**, **còn gì phải làm**, **làm theo thứ tự nào**. Người
tiếp nhận đọc xong là chạy được hệ thống và biết bước kế tiếp mà không phải hỏi ai.

Phạm vi đã chốt: **làm bản PC (Windows) trước**. Android và các nền tảng khác để sau, codebase giữ
đủ trừu tượng để thêm vào nhưng chưa làm.

## 0. Hướng dẫn cho phiên sau — đọc trước tiên (cập nhật 2026-09-19)

Mục này gom **toàn bộ** những gì một phiên mới cần để tiếp tục mà không phải hỏi lại: quy tắc chủ dự
án đã đặt, hai nguồn nhị phân và cách mổ, việc đã làm / chưa làm và cách làm, lỗi đã mắc, cách làm
đang tốt. Các mục 1–5 phía dưới vẫn đúng và chi tiết hơn ở từng phần.

### 0.1 Quy tắc làm việc (chủ dự án đã nói — bắt buộc)

1. **Đọc mã nguồn, không đoán mò.** Mọi luật game phải lấy từ **nhị phân Linux** (`D:\ServerLinux\server1\jx_linux_y`)
   đọc **từng dòng**; mọi hình ảnh / bố cục / câu chữ giao diện phải lấy từ **client 2.0**
   (`C:\Users\nguye\Level Up Games\Vo Lam Truyen Ky 2.0`). Nguồn cũ VC6 (`SwordOnline/Sources`, bản JX1)
   chỉ dùng để **đặt tên** và hiểu ý đồ, không dùng làm luật (bản 2.0 là JX2, khác số).
2. **Xong phần nào → ghi `docs/HANDOVER.md` (mục 4b) + commit + push ngay**, rồi dời nhánh
   `safe/jxnext-2026-09-17` và tag `backup/jxnext-2026-09-17` lên commit mới nhất (`git branch -f`,
   `git tag -f`, `git push -f origin <cả hai>`). Chủ dự án xem GitHub Desktop, nên không gom nhiều việc
   vào một commit. Sau khi xong một cụm việc thì **fast-forward `main`** lên nhánh làm việc
   (`git push origin HEAD:main`) cho đồng bộ.
3. **Làm liên tục đến khi chủ dự án bảo dừng.** Không dừng giữa chừng để hỏi; gặp trở ngại thường
   (lỗi build, test đỏ, thiếu dữ liệu) thì tự xử lý; chỉ dừng khi cần một quyết định thật sự của chủ dự án.
   Mỗi lúc bắt đầu / xong một phần thì báo ngắn: **"Đang làm: …"** / **"Xong: …"**, có ảnh chụp thì gửi.
4. Log của `jx_zone` và `jx_gateway` phải **rõ từng dòng, tiếng Việt hoặc tiếng Anh chi tiết**: mọi
   thông báo / trường mới phải có trong `config/log.vi.json` (`python tools/check_log_catalog.py` phải sạch;
   CI chạy nó trước mọi thứ).
5. **Đặt tên tệp / lớp theo nguồn cũ** (`KNpc`, `KPlayer`, `KItemList`, `UiStatus`…), bảng đối chiếu ở
   `docs/OLD-TO-NEW.md`; mỗi tệp mới thêm một dòng vào đó.
6. **PostgreSQL lưu theo từng nhân vật, rải đều, không dồn dập** (mục tiêu 20 000 người một server) —
   đã làm ở M9 (`PgStore`, hàng đợi lưu riêng ở gateway); không được quay lại kiểu lưu cả cụm.
7. **PC trước**; nền tảng khác để sau nhưng không được viết mã chặn đường.
8. Nâng cấp công nghệ để mã dùng được 5–10 năm: C++20 + CMake + vcpkg, Go 1.26, Godot 4.7, protobuf;
   không thêm lớp tương thích tạm, không giữ Lua 4.
9. Không cài phần mềm lên máy chủ dự án khi chưa hỏi (không có PostgreSQL / Docker / GCC tại chỗ —
   PostgreSQL thật chạy trong CI). Không dùng token GitHub của chủ dự án; không đăng nhập game thật;
   không nhập mật khẩu.
10. Trước khi build C++ phải **tắt server đang chạy** (LNK1168). **Không bao giờ giết tiến trình của chủ
    dự án** (server ở checkout chính, cổng 17001/17100/17102); chỉ tắt tiến trình test do mình mở.
    Khi cần chạy song song thì `JX_PORT_OFFSET=1000`.
11. Không commit dữ liệu game (`data/`, `client/assets/`, `config/*.local.json`, DB tài khoản, `build/`).
    Không sửa nguồn cũ GBK/TCVN3 bằng công cụ soạn thảo thường (đọc qua `python … decode('gbk')`).
12. Mọi câu hỏi thiết kế trả lời bằng **số đo và bằng chứng** (địa chỉ hàm, dòng nguồn cũ, ảnh chụp).

### 0.2 Hai nguồn nhị phân — đường dẫn, công cụ, cách mổ

| | Bản Linux (luật game) | Client 2.0 (hình ảnh, chữ, bố cục) |
|---|---|---|
| Tệp | `D:\ServerLinux\server1\jx_linux_y` (ELF 32-bit không section header), `gateway\s3relay_y`, `bishop_y`; bảng: `D:\ServerLinux\server1\settings\`, script: `…\script\` | `C:\Users\nguye\Level Up Games\Vo Lam Truyen Ky 2.0\gamecl.exe` (UPX 3.03), `enginefree.dll`, kho `Data\*.pak`, `\reslst.dat` |
| Mở | `python tools/re/re_calls.py D:/ServerLinux/server1/jx_linux_y build` (một lần, cache `jx_linux_y.calls.json`) | `python tools/re/re_upx.py "<gamecl.exe>"` → `build/re/gamecl.exe.unpacked.img` + `.json`; các công cụ dưới đọc `.img` như ELF |
| Dịch ngược | `re_elf.py <bin> dis <va> [n]`, `func <va>`, `xref <va>`, `xrefstr <chuỗi>`, `strings <regex>`, `luamap` | như bên trái (`function_start` biết prologue MSVC) + `re_pe.py`, `re_dll.py` cho DLL, `imgstr.py` |
| Ai gọi ai | `re_calls.py <bin> callers <va>`, `strargs`, `byarg <chuỗi>` (xem chuỗi/đối tượng truyền vào từng lời gọi) | cùng công cụ trên `.img` |
| Thành viên / bảng con trỏ hàm | `re_scan.py <bin> disp <hex>[,…] [mnemonic]` (ai đọc/ghi `[reg+off]`), `pmf` (ctor đổ `ProcessFunc[]`), `ins <regex>` | cùng |
| Bảng settings ↔ cột | `re_tables.py`, `re_tabdesc.py <bin> <reader-va>` (mảng mô tả cột `{kiểu, đích, mặc định}` của `KBPT_*::ReadRow`) | bảng của client nằm trong pak: `jxassets export-items` tự rút `\settings\item\NNN\*.txt` |
| Chữ ký hàm Lua | `re_luasig.py` → `docs/linux/jx_linux_luasig.tsv` (1506 hàm) | — |
| Ma pháp ↔ ô KNpc | `re_attribmod.py <bin>` → `docs/linux/jx_linux_attribmod.tsv` | — |
| Kết quả ghi ở | `docs/LINUX-SERVER.md` (§9 vật phẩm, §10 KNpc/KPlayer, §10.5 kinh nghiệm), `docs/linux/*.tsv` | `docs/CLIENT-2.0.md` (`KItem::GetDesc`, `KMouseOver`, bảng chuỗi `stringtable_core`, màu engine) |

Cách tìm một hàm không có tên (đã dùng suốt M11–M12, luôn hiệu quả):
1. **Chuỗi**: `re_elf.py … strings "<regex>"` rồi `xrefstr` → hàm chứa chuỗi log/lỗi (ví dụ
   `"m_CurrentHitRecover:%d"` → `KNpc::OnHurt`).
2. **Bảng con trỏ hàm**: `re_scan.py … pmf` → ctor `KNpcAttribModify` (0x08099600), `KProtocolProcess` (0x080DA560).
3. **Offset thành viên đã biết**: `re_scan.py … disp 15ac,15b4` → mọi hàm chạm `m_LifeMax` → ra
   `ClearAttrib`, `LoadFrom`, `LevelUp`… rồi `re_calls callers` để biết ai gọi.
4. **Hàm Lua**: `luamap` / `jx_linux_luasig.tsv` cho địa chỉ hàm C++ đứng sau tên script (`AddExp` →
   `0x0811A140` → `KPlayer::AddExp 0x080B00C0`).
5. **Đọc gọn nhiều hàm nhỏ**: bản dump rút gọn (bỏ prologue/epilogue, chỉ giữ lệnh chạm `[npc+off]`) —
   mẫu ở scratchpad phiên trước (`pfdump.py`) — viết lại trong 20 dòng bằng `Elf.dis` + lọc.
6. Mỗi hằng số kỳ lạ đều là phép chia của trình biên dịch: `0x51EB851F, sar 5` = /100;
   `0x10624DD3, sar 6` = /1000; `0x66666667, sar 1` = /5, `sar 2` = /10, `sar 3` = /20;
   `0x1B4E81B5, sar 5` = /300; `0x92492493` = /7. **`sub edx, ecx`** là chia thường, **`sub ecx, edx`**
   là **âm** của thương (đã gặp ở `nomovespeed`).
7. Nguồn cũ để đặt tên: `SwordOnline/Sources/Core/Src/*.h` — đọc bằng script decode GBK (mẫu `oldsrc.py`
   ở scratchpad: `open(p,'rb').read().decode('gbk','replace')`), **không** mở bằng editor.

Mọi số đưa vào mã phải kèm địa chỉ hàm trong chú thích (`// 0x080A7F90`), và test khẳng định đúng số
đó với bảng thật của server Linux (`test_KPlayer.cpp` là mẫu).

### 0.3 Đã làm (tóm tắt; chi tiết ở mục 2 và nhật ký 4b)

- **M6–M8** hiệu năng mạng + luồng đăng nhập 2.0 (7 cửa sổ, khớp điểm ảnh 99,9 %).
- **M9** kho PostgreSQL (`PgStore`, lưu theo nhân vật, CI có PostgreSQL thật) — còn số đo 20 000.
- **M10** mổ nhị phân Linux: 1506 hàm script có chữ ký, 109 bảng settings nối cột, công cụ `tools/re`.
- **M11** vật phẩm: bảng đọc đúng cột (mọi phiên bản 000–004), `KItem/KInventory/KItemList/KItemGenerator`,
  túi đồ + cửa sổ nhân vật + chú thích đúng client 2.0 từng dòng, rơi đồ / nhặt / vứt, ma pháp tiền
  tố–hậu tố + hoàng kim, `GetEquipEnhance`, hàm script vật phẩm nhiệm vụ.
- **M12 lát A (xong 2026-09-19)**: bản đồ `KNpc`/`KPlayer` (§10), `KNpcAttrib.h` (gốc/hiện tại/yan),
  `KMagicAttribId.h` (335 tên sinh tự động), `KNpcAttribModify` (217 ProcessFunc đọc từng hàm),
  `KPlayer` (LoadFrom, công thức điểm → chỉ số, UpdataCurData/ReCalcEquip, LevelUp, cộng điểm,
  AddExp + CalcExp, bản ghi sát thương chia kinh nghiệm), `KPlayerSet` + `jxassets export-player`
  (`player.json`), gateway tạo nhân vật mới theo `newplayerini%02d`, gói `G2C_PLAYER_ATTRIB` /
  `C2G_ADD_POINT`, client `UiStatus` trang thuộc tính + nút cộng điểm.
- **M12 lát B1 (xong 2026-09-18)**: bảng `Skills.txt` và **số theo cấp từ chính script cấp** như bản Linux
  (`KSkill.h/.cpp`: `KSkillRow`, `KSkill::add_attrib` = `AddMagicAttrib`, `KSkillTable`, `KSkillManager`;
  `jxassets export-skills` → `skills.json`), `LINUX-SERVER.md` §11, test với `shaolin_gunfa` thật cấp 1/10/20.
- **M12 lát B2a + lát C (xong 2026-09-18)**: **lõi sát thương và trạng thái đúng từng lệnh** của `jx_linux_y`
  (`LINUX-SERVER.md` §12): `KNpc.cpp` (`ReceiveDamage`, `CalcDamage` + kháng theo kiểu, `AppendSkillEffect` + 5 hàm
  nguyên tố, `MixPoisonDamage`, đặt/gộp độc, `SetStateSkillEffect`/`Remove`/`Immediately`, `OnHurt`, `KnockBack`,
  `ProcessState` mỗi khung: độc, băng, choáng, thuốc, trạng thái hết hạn), `KSkills.cpp` (`Cast` style 2/3,
  `CreateMissleMagicAttribsData`, `StartEvent`, tải đạn lên mục tiêu), `attribconstdata.ini` → `skills.json`
  (`attrib_data`), `addphysicsdamage_p` đủ 11 loại; đánh thường = kỹ năng 1/2 (dòng thật của `Skills.txt`) qua
  cùng đường; test `test_KNpcDamage.cpp` 11 ca / 206 kiểm, mọi số tính tay từ lệnh máy.
- **M12 lát B2b (xong 2026-09-18)**: **hệ đạn đúng từng lệnh** của `jx_linux_y` (`LINUX-SERVER.md` §13): `missles.txt` →
  `missles.json` (`jxassets export-missles`, Go `missle.Parse`), `KMissle.h/.cpp` (`CreateMissle`, `KMissleSet::Add`, khung đạn,
  `PrePareFly`, `Activate` với bước 10 đơn vị và chu kỳ `DmgInterval`, `OnFly` 7 kiểu bay (đứng/thẳng/vòng/xoắn/bám/đích/quay lại),
  `TestBarrier`, `CheckCollision` (tầm, bắn chính xác), `ProcessCollision`/`ProcessDamage` (tỉ lệ trượt, số lần trúng),
  `DoCollision`/`DoVanish`, sự kiện bay/chạm/mất, `CastMissles` 8 dạng + 6 hàm sinh, độ trễ `MslsGenerate`, thi triển con, đạn
  tức thời style 14, thi triển từ đạn); `g_GetDirIndex` + bảng sin/cos thay bằng **bản JX2** (phải = 48, bảng cắt 979);
  đánh thường là đạn thật (mẫu 64/65 có sẵn khi không có bảng); quái test không mẫu là `camp_animal`; test
  `test_KMissle.cpp` 14 ca / 319 kiểm, ctest 183/183, Go 5/5.
- **M12 lát B2c (xong 2026-09-18)**: **kỹ năng tự động + ô trống khi đánh lùi đúng từng lệnh** (`LINUX-SERVER.md` §14):
  5 danh sách `KNpc::auto_skills` (mỗi khung, bị đánh, đánh trúng, máu tụt, chết) nạp bởi `autocastskill`/`autoreplyskill`/
  `autoattackskill` (`0x08189000`, khoá `id<<8|cấp`, % byte thấp, khung chờ theo mục tiêu), duyệt `0x08188BB0` (mỗi khung trước
  `ProcessState`, trong `ReceiveDamage` với đối số thật: danh sách đánh trúng bắn vào nạn nhân), bản đồ `oncastskill`
  (`0x080821C0` cuối `StartEvent`), `knock_back_free_spot` (`0x08081B70`: bước 12 = `step_length` `+0x129c` — **đính chính**
  `+0x129c` không phải bán kính kỹ năng), `barrier_kind` (`0x080E0A30`: nửa ô chéo 2..5, nhị phân đạn dùng chung); đã đọc và
  ghi (chưa port): `CanCastSkill 0x080E8AE0`, kỹ năng tạo npc `0x080E8770`; test `test_KAutoSkill.cpp` 5 ca / 84 kiểm, ctest 188/188.
- **M12 lát B3a (xong 2026-09-18)**: **sổ kỹ năng `KSkillList` đúng từng lệnh** (`LINUX-SERVER.md` §15): 80 ô × 12 trường
  (`KNpc+0x248`), `Add/Remove/IncreaseLevel/ChangeCurrentLevel` (`0x080E5420/52D0/5010/56A0`), nút cộng cấp `KSkillLevelIncNode`
  (`allskill_v` 139 → `0x080E5BF0`), bản đồ tăng sát thương (`addskilldamage`, = `skill_enhance` cũ), hồi chiêu (`CanCast 0x080E4540`,
  `0x080847B0`, `reduceskillcd` 288..290), cấm (`ForbitSkill`), kinh nghiệm kỹ năng (`0x080E5D90` từ đòn đánh 6/10 + Lua),
  điểm kỹ năng (`KPlayer::AddSkillPoint 0x080BD460` đủ 11 bước kiểm, script `LevelUpScript`), nạp/lưu `RoleData.skills`,
  đồng bộ client (`G2C_SKILL_LIST/LEVEL/FORBID`, `C2G_ADD_SKILL_POINT`), 17 hàm Lua; đã đọc (chưa port, B3b): đường lệnh thi
  triển `SendCommand 0x0809B750` → `0x0809B840` → `CastSkill 0x08088350` → `DoSkill 0x08088150`; test `test_KSkillList.cpp`
  8 ca / 260 kiểm, ctest 196/196.
- **M12 lát B3b (xong 2026-09-18)**: **thi triển theo lệnh đúng đường của nhị phân** (`LINUX-SERVER.md` §16): gói client
  (`NpcSkillCommand 0x080DD130` → `C2G_CAST_SKILL`) → vòng 5 lệnh `SendCommand 0x0809B750` (phải có trong sổ) → mỗi khung
  `ProcessCommand 0x0809B9E0` + kiểm `0x0809B840` (bận, `PeaceCanUse` khi chưa chiến đấu, `SetActiveSkill` đặt `+0x12a8`, `CanCast`,
  `CanCastSkill`, kiểm phí; mục tiêu ngoài tầm của kỹ năng có kiểm tầm bị bỏ, đi tới +300 chỉ với kỹ năng không kiểm tầm)
  → `CastSkill 0x08088350` (trừ phí `0x08078B10`, gói 0x5a → `EntityAction.skill_id`) → `DoSkill 0x08088150` (do_magic 6 /
  do_attack 7, khung = CastFrame/AttackFrame·100/(tốc độ+100), `clearallcd`) → khung 60 % `0x08085020` `Cast` + `SetSkillCoolTime`;
  `CanCastSkill 0x080E8AE0` port trọn (`weapon_physics_skill 0x08079A90`, `EqtLimit 0x080E8C05`, tầm theo style); bảng
  `武器物理攻击对照表.txt` (`0x0805F18D`) → `KWeaponSkillTable`/`weapon_skill.json` (**tệp thiếu trên máy** → đánh thường 1/2);
  kỹ năng khởi đầu `[FSKILLS]` (53, 1, 2, 229..232) vào `RoleData.skills` khi tạo nhân vật; AI npc đi thẳng `cast_skill`;
  test `test_KNpcCommand.cpp` 6 ca / 113 kiểm.
- **M12 lát B3c-0 (xong 2026-09-18)**: **ẩn thân `[hide]` 200 đúng từng lệnh** (`LINUX-SERVER.md` §16.1): `ProcessFunc 200
  0x08097860` → `KNpc::SetHide 0x0807FF80` (0→n: gói 0x4f cho người xung quanh trừ chính mình → zone `entity_gone(keep_self)`;
  n→0: đồng bộ lại → `wake_viewers_near`), `IsInvisibleTo 0x08079200` (ẩn: chỉ chính mình thấy; hỏi trong `look_around`), ẩn thân
  vỡ `0x0807D4C0` (`[hide]` `Data1..4` = 713/1235/1258/1267 gỡ; `Data0` = 70 độ trong suốt) trong `CastSkill 0x080884A3` và
  `DoDeath 0x08089359`; 187 `addstealfeatureskill` → `add_level_inc(v1, ±1)`; Lua `SetHide`; `EntityInfo.hide`; sửa kèm: người
  chơi không tự lặp buff lên mình; test `test_KNpcCommand.cpp` 8 ca / 155 kiểm, ctest 204/204. Đã đọc, chưa port: 186 mượn
  dáng `0x08099170`, ngựa `0x0807D520`, gói 0x85/aura `0x080873B0`, `NpcSetHide` (chờ quy ước chỉ số npc).
- **M12 lát B3c-1 (xong 2026-09-18)**: **thân pháp style 1 đúng từng lệnh** (`LINUX-SERVER.md` §16.2): `0x08087F70` bảng
  `0x08254AC0` dạng 8..13 — 8 đòn con (`0x08084930`, doing 14, con ở 60 %), 9 nhảy (`0x08087CF0` đường ≤ 40 bước×12, > 20; `0x0807B320`
  gói 0x54; mỗi khung `0x080817E0` độ cao `((5(b−1) − 5f)·f)/8`), 10 nhảy đánh (`0x080807E0`, pha 0/1, con style 0 ở 60 % rồi đứng),
  11 chạy đánh (`0x08084A10` `+0x128c += Param1`; `0x080853B0` con khi tới hay quá `0x080E8650(sk,0)`), 12 thi triển nhiều lần
  (`0x08084B40/0x08086E50`, `ChildSkillNum` lần, trễ đạn thứ i), 13 thuấn di (`0x08084C90` `Param1` tầm, −1 GM; `0x08080760`
  SetPos sau `Param2` khung); zone `KDoing` +6, `KNpc::cast_kept*/jump_*/phase/run_*/height`, `KSubWorld::do_special_skill` +
  các hàm bắt đầu/mỗi khung (`KSkills.cpp`), `stop_action`/`end_run`, `ACTION_JUMP` (client chạy tới điểm rơi); test
  `test_KNpcCommand.cpp` 11 ca / 257 kiểm, ctest 207/207.
- **M12 lát B3c-2 (xong 2026-09-18)**: **mòn đồ đúng từng lệnh** (`LINUX-SERVER.md` §16.3): `KItemSet::Init 0x0806E250` đọc
  `settings/item/AbradeRate.ini` (3 chế độ × 15 ô mặc + AdvPlatina cho ngọc bội cấp > 5, `0x0806D560`) → Go `KAbradeRate.go` +
  `jxassets export-abrade-rate` → `abrade_rate.json` → `KAbradeRate`; `KItemList 0x08201940` (mỗi ô mặc trừ Mask, `KItem::Abrade
  0x08066570` một phần N; mòn → gói 0x9b = `G2C_ITEM_ADD`; hết → thông báo `G_STR_ITEM_ABRADETOZERO`, thành phế phẩm
  (`0x08067540`, bảng phế phẩm chưa đọc → chỉ `genre = broken`), vào túi (`0x082006B0` + `0x080B5180`) hay huỷ (`0x0806DB90`));
  gọi từ `CastSkill` (đánh), `ReceiveDamage 0x0808B148` (mất máu), mỗi bước đi `0x0807C2F0`, Lua `AbradeEquipments`; `0x08201D90`
  mất n % (chết/PK) → `abrade_equipments_percent`; test `test_KItem.cpp` +1 ca / 30 kiểm, ctest 208/208, Go test bảng.

### 0.4 Chưa làm — và làm như thế nào

| Việc | Cách làm (đã biết địa chỉ / nguồn) |
|---|---|
| **M12 lát B — kỹ năng**: **B1 xong** (bảng 114 cột → `skills.json`, `KSkill`/`KSkillManager`, số theo cấp chạy chính script; §11); **B2a xong 2026-09-18** (lõi sát thương/trạng thái + `Cast` style 2/3; §12); **B2b xong 2026-09-18** (hệ đạn: `missles.txt`, `CastMissles` 8 dạng, bay, va chạm, sự kiện; §13); **B2c xong 2026-09-18** (kỹ năng tự động 5 danh sách + bản đồ thi triển kèm + ô trống khi đánh lùi; §14) | **Còn của B2**: port `CanCastSkill 0x080E8AE0` (đã đọc §14; cần bảng `武器物理攻击对照表.txt` → kỹ năng đánh theo vũ khí, thay `swing_skill` tạm) và kỹ năng tạo npc `0x080E8770` (đã đọc §14; cần sổ npc tạo của `KPlayer +0x7d34..` và đếm ngược thời gian — chưa đọc), `0x081FEE60` (đồ mặc ghi danh sách bị đánh/đánh trúng), gói `0x85` ra client, kỹ năng tự động `0x08188BB0` (4 danh sách `+0x182c/+0x1850/+0x1874/+0x1898`, `{kỹ năng, tỉ lệ}`) + bản đồ `0x080821C0` (`+0x18EC`) — móc `trigger_auto_skills` đã sẵn; `0x08081B70` ô trống khi đánh lùi; `CanCastSkill 0x080E8AE0` (tiêu hao/hồi chiêu); kỹ năng tạo npc `0x080E8770`; `Player+0x5a50` (đối tượng PK: đạn của người chơi bị bỏ khi nó đổi — zone so với 0); client `KMath.gd` vẫn `g_GetDirIndex` JX1 (`63−k`, bảng làm tròn) — chỉ để vẽ, đồng bộ ở B4. **B3a xong 2026-09-18** (sổ `KSkillList` + điểm/kinh nghiệm kỹ năng + Lua; §15). **B3b xong 2026-09-18** (đường lệnh thi triển + `CanCastSkill` + bảng vũ khí + kỹ năng khởi đầu; §16). **B3c-0 xong 2026-09-18** (ẩn thân `[hide]` 200: `SetHide 0x0807FF80`, `IsInvisibleTo 0x08079200`, vỡ `0x0807D4C0`; §16.1). **B3c-1 xong 2026-09-18** (thân pháp style 1 sáu dạng; §16.2). **B3c-2 xong 2026-09-18** (mòn đồ `0x08201940` + `AbradeRate.ini`; §16.3; còn: bảng phế phẩm `g_ItemGenerator+0x1e88` của `0x08067540`, cấp ngọc bội `+0x344` trong bộ sinh). **B3c**: ngựa `0x0807D520` (`+0x199c`; lên ngựa làm vỡ ẩn thân), mượn dáng 186 `0x08099170`, gói 0x85/aura `0x080873B0`, `NpcSetHide` (chờ quy ước chỉ số npc của API script), `0x080B12C0`, kỹ năng tạo npc `0x080E8770` + đếm `Player+0x7db0`, đồng hành (loại 2, `npc+0x1698`), chết của người chơi `0x080B2790` + PK (`[0x8BADF50]`, `Player+0x5a50`), `Player+0x5994` (chưa rõ), bảng chuyển sinh `0x0830CA14`, hàm `0x080AEBC0`/`0x081D0C00`. **Dữ liệu cần xin chủ dự án**: `settings/武器物理攻击对照表.txt` (vũ khí → kỹ năng vật lý; máy này chỉ có `clientweaponskill.txt`). **B4** client: ô kỹ năng / phím tắt theo bố cục 2.0, gói trạng thái 0x87 (biểu tượng `+0x54`, `+0x4c`), đánh lùi (`KDoing::knock_back` đang gửi như choáng). **Dữ liệu**: 156/285 script cấp thiếu trên máy — kể cả đánh thường; zone tạm cho số 0 (`skill level script missing`) → hỏi chủ dự án lấy từ server thật. |
| **M12 lát C — công thức sát thương**: **xong trong B2a** (`ReceiveDamage 0x0808A4A0`, `CalcDamage 0x08089C90`, kháng `0x0807BCD0/0x0807BB20/0x08078910`, `AppendSkillEffect 0x0807CE70`, `OnHurt 0x0807F780`, §12) | còn thuộc B2b/B3: `KnockBack` cần `0x08081B70` (ô trống trên đường); bộ nạp hằng PK `[0x8BADF50]`; `[0x830D234]`/`[0x830D248]`/`[0x830D24C]` (cap đóng băng/độc) đang 0 như nhị phân; ngồi (`m_Doing 8`) và chạy (`0x12`, thưởng `+0x14b0`, thể lực `[0x8BADF80..]`) chưa có trạng thái trong zone. |
| Trạng thái (độc / băng / choáng / thuốc) | `KNpc::ProcessState 0x0808B610` (§9), trang trạng thái `KNpc+0x234` (20 ô × 16 byte), `ReCalcStateEffect 0x0807D270` (áp lại với dấu âm), `+0x1bc..+0x1fc` các bộ đếm. Zone mới có `life_state/mana_state` và ô giữ chỗ `poison/freeze/stun_state`. |
| Chia kinh nghiệm theo **đội** | `KPlayer::AddExpTeam 0x080B03E0` (đếm thành viên cùng map trong 1024 đơn vị, `√n × float 0x0825528C`, `100 + n`); `KDamageRecord::Add` ghi theo đội trưởng `0x08BB86E8 + team·0x30`. Cần hệ đội (M14). |
| Hình phạt chết của người chơi | `KNpc::OnDeath 0x08088B60` phần đầu (mất kinh nghiệm `GetLevelExp/100·2 × (7−PK)/7`, trần 0x1FBD0; `0x080B9FA0` mất tiền/đồ theo PK). |
| Ngồi hồi thể lực / chạy trừ thể lực | `stamina.ini` đã có trong `KPlayerSet` (`NormalAdd`, `SitAdd` ‰ ở `+0x11b0`, `ExerciseRunSub/FightRunSub/KillRunSub`); tìm nơi dùng `+0x11b0` và `KPlayerSet+0x14c0..` (`re_scan disp 11b0`). |
| `m_nLucky` vào rơi đồ | `GenRandomItem` đọc `Player+0x5958` (`KPlayer::cur_lucky` đã có) — nối vào `KSubWorld::gen_random_item` (đang truyền 0). |
| M11 dồn lại | bạch kim / lỗ khảm (quality 2 `0x0806B6C0`), `AddItemEx`, móc `Check_ItemUsable`/`OnUseItem`, kho đồ (cần NPC), giao dịch, `bAllActived` (`+0x4c7c`), dòng khoá/ràng buộc trong chú thích. |
| M13 nhiệm vụ / hàm script, M14 xã hội, M15 client (hoạt ảnh đánh/chết, trang bị lên người, minimap, âm thanh), M16 chia vùng, M17 vận hành (O2–O5, D1–D3), U6/U7 | theo mục 3 và 4. `spawn_npc` trong tick cần hoãn (nguy cơ `EntityTable` cấp phát lại) — chip task đã tạo. |
| Đo 20 000 nhân vật PostgreSQL (M9) | cần PostgreSQL / Docker tại chỗ — chờ chủ dự án cấp. |
| CI | sau mỗi push xem `https://github.com/JXGAME04/DUANGODOT/actions?query=branch%3Aclaude%2Flogin-system-upgrade-95794b` (trình duyệt tích hợp, không đăng nhập); push dồn làm các run trước bị **cancelled** (bình thường); run đỏ nhanh (~1 phút) thường là `gofmt`, `check_includes`, `check_log_catalog`. |

### 0.5 Lỗi đã mắc — để phiên sau tránh

- **Công cụ Bash ở máy này rút `\\` thành `\` và ăn `\N`, `\x`, `\u` trong heredoc** → patch Python bị
  `AssertionError` hoặc ghi sai chuỗi (`"\\n"` thành xuống dòng thật trong `.gd`). Cách đúng: viết tệp
  patch bằng công cụ **Write** rồi `python patch.py`; console cp1252 → `sys.stdout.reconfigure(encoding="utf-8")`.
- **Đoán tên hàm từ địa chỉ gần đúng**: từng nhận nhầm `0x632710` là `GetDesc` (thật `0x00636460`),
  `0x784A58` là `strcat` (thật `g_StrWrap`), `0x8BACAC0` là `g_NpcSet` (thật `g_PlayerSet`). Luôn kiểm bằng
  chuỗi log trong hàm, số đối số, và một hàm gọi nó.
- **`re_attribmod.py` bản đầu chạy lố sang hàm kế** (các ProcessFunc kết thúc bằng `jmp` tail-call, không
  `ret`) → gán thừa ô. Mọi bộ duyệt hàm phải chặn ở địa chỉ hàm kế và ở `jmp` ra ngoài.
- **Viết lại công cụ đã có** (`re_upx.py` trùng `upx_unpack.py`/`re_pe.py`) vì không đọc `tools/re/README.md`
  trước. Đọc README + `LINUX-SERVER.md §2` trước khi viết công cụ mới.
- **CI đỏ vì `-Werror=sign-conversion` của GCC** (MSVC không báo): đã bật `/w44365` để bắt tại chỗ; chỉ số
  `std::array` phải là `size_t`, `return ec ? std::uint16_t{0} : port`.
- **CI đỏ vì `gofmt`** (tệp Go mới không format) và **catalogue log thiếu** thông báo mới của gateway →
  chạy `gofmt -l .`, `go vet ./...`, `python tools/check_log_catalog.py`, `python tools/check_includes.py`
  **trước mỗi commit**.
- **Godot treo khi script lỗi cú pháp** → luôn chạy Godot qua `Start-Process` + `WaitForExit(240000)`,
  đọc `*.err`; `var x := a.duplicate(true)` không suy được kiểu, `static func _set` đụng `Object._set`.
- **Test giả định số cũ** (máu 100, chính xác 100 vs phòng thủ 0 → trúng 100 %): khi đưa công thức thật vào,
  tỉ lệ trúng thành 54 % và test ngẫu nhiên; cách sửa là cho vai test điểm cao (nhanh nhẹn 100 → 95 %) chứ
  không nới công thức.
- Bộ đọc bảng Go: `atoi("")` = 0 khác `KTabFile::GetInteger` (ô trống = mặc định) — đã sửa bằng `cell()`;
  nhớ luật này khi đọc bảng mới. Chuỗi TCVN3 có `/` bị tưởng là đường dẫn — decode thuần TCVN3 cho chuỗi UI.
- **`re_elf.py func` dừng ở `ret` đầu tiên**, còn GCC đặt nhiều khối SAU `ret` (`0x080EE380`, `0x080EDCC0`,
  `0x080E6E10` đều thế): phải `dis` tiếp từ mọi nhãn nhảy tới khi không còn nhánh nào chưa đọc.
- **Ghi đối số theo phỏng đoán**: phiên trước ghi "số cấp = đối số 3" của `0x080EE4B0`; đọc chỗ dùng
  (`[ebp+0x10]` là `nRow` của `GetString`) thấy ngược. Mỗi đối số phải chỉ ra lệnh dùng nó.
- **In dòng bảng có lọc `= 0`** rồi tưởng ô trống → kỳ vọng test sai (`AttackRadius` kỹ năng 4 ghi rõ "0").
  In thô ô cần kiểm trước khi viết kỳ vọng.
- **Thư mục script tên GBK trên đĩa Windows** là mojibake cp1252 nhưng vài byte (0x81, 0x8D, 0x8F, 0x90, 0x9D)
  không mã hoá ngược được bằng `str.encode("cp1252")` → phải mã hoá từng ký tự, rớt thì lấy `ord(c)`.
- **Tên hàm của zone đánh lừa**: `g_IsAccrue` của `KMath.h` là bảng **tương sinh** `0x830ED18`, nhưng `ReceiveDamage`
  gọi `0x08074190` với bảng **tương khắc** `0x830ED2C` — phải đọc hàm nạp bảng (`0x080741D0`) chứ không suy từ tên;
  test sai một lần vì thế.
- **Kỳ vọng test tính tay thiếu một lệnh**: kháng vượt `max` (kể cả `max = 0`) bị "mềm" ở `0x08078910`; `nEnhance` áp
  cả ô băng; khiên nội chỉ bỏ giảm đòn khi nội **âm** (0 thì vẫn giảm); `Cast` trả 1 dù `CastInitiativeSkill` từ chối.
  Trước khi ghi số kỳ vọng, đi lại từng lệnh của đường đó, đừng nhớ đại ý.
- **Đổi chuỗi ngẫu nhiên làm test cũ đỏ**: lõi mới gọi `g_Random` thêm (DoHurt, OnHurt) → test "20 tick giết quái máu 1"
  trượt 5 % ở seed cố định. Sửa test cho đủ đòn (200 tick), không nới `MAX_HIT_PERCENT`.
- **Heredoc Bash với `\settings\`, backtick, dấu nháy** hỏng (`unexpected EOF`) → nội dung dài ghi bằng **Write** ra scratch
  rồi `python patch.py` nối vào; đã ghi ở lỗi đầu, vẫn mắc lại.
- **Con trỏ vào bảng thực thể chết khi spawn thêm** (`EntityTable` dồn vector): test lấy `KNpc*` rồi `spawn_npc` thêm → ghi vào
  vùng đã giải phóng, đạn không sinh (launcher rác) và SIGSEGV. Sau mỗi spawn phải `mutable_entity` lại.
- **Tin bảng số của zone thay vì nhị phân**: bảng sin/cos `KMath.h` "sinh lại" làm tròn (980) còn nhị phân **cắt** (979) ở 28 ô;
  `g_GetDirIndex` của zone là bản JX1 (`63−k`, bảng nửa bước) còn `0x080EEEC0`/`KnockBack` JX2 là `64−k` làm tròn gần nhất
  (phải = 48, lên = 32). Mọi bảng hằng phải `vtab` từ nhị phân, kể cả khi "chỉ là lượng giác".
- **Đạn chỉ chạm được mục tiêu đúng quan hệ**: quái test không mẫu mang `camp_free` = đồng minh của tân thủ (`g_GenOneRelation`)
  → đánh thường bằng đạn không trúng, 3 test cũ đỏ dù logic đúng. Sửa dữ liệu test (`camp_animal` cho quái không mẫu), không nới bộ lọc.
- **Sự kiện đạn tra kỹ năng theo id/cấp qua `g_SkillManager`** (`0x08076CE0`): kỹ năng dựng tay trong test không có trong bảng → không
  sự kiện; và `load_skill_level_data` chỉ áp "0" khi **có tên** script cấp (không tên → bỏ, không tải) — test phải cho `LvlSetScript`.
- **Lệnh Bash `cmd.exe /c "next_env.cmd cmake ..."` không chạy gì** (cmd không nhận `%*`); phải gọi từ PowerShell
  `& cmd.exe /c "$S\next_env.cmd" cmake --build ...`. Kiểm bằng dòng cuối log (`Linking`), đừng tin "không lỗi".
- **CI Linux đỏ vì hàm tĩnh không dùng** (`apply_state_modifier` trong `KNpc.cpp`, `-Werror=unused-function`; MSVC không báo): mỗi
  hàm trong `namespace {}` phải có nơi gọi; và **log CI đọc được không cần token**: mở trang run trên GitHub bằng trình duyệt tích
  hợp (`Annotations` liệt kê đúng dòng lỗi) — đừng đoán mò nguyên nhân.
- **Nhãn offset sai kéo theo luật sai**: §10.1 ghi `+0x129c` = bán kính kỹ năng; `re_scan disp` cho thấy chỉ `Init` (=12) và `ClearAttrib`
  ghi nó, `0x08081B70` dùng làm **bước** → nếu tin nhãn, đánh lùi sẽ "không bao giờ" (bán kính > 32). Trước khi dùng một ô làm luật,
  `re_scan disp` xem ai ghi.
- **Kỹ năng tự động đáp đòn vô hạn**: `[autoreplyskill]`/`[autoattackskill]` của `attribconstdata.ini` chính là danh sách kỹ năng
  **không** đánh thức lại danh sách; test dựng tay thiếu nó → tràn stack (nhị phân cũng thế). Dữ liệu test phải có `set_attrib_data`.
- **Hai bố cục ô sát thương**: `KSkill::damage_attribs` (bố cục của kỹ năng: `KSkill::damage_slot`, `addskillexp1` ở 15, `seriesdamage_p`
  cuối) khác `KDamageSlot` của `ReceiveDamage` (`seriesdamage_p` ở 0, mọi thứ lùi một ô). Test đọc nhầm bảng của kỹ năng bằng
  `damage_slot_add_skill_exp1` → 0. Khi đọc mảng của `KSkill` phải dùng `KSkill::damage_slot(id)`.
- **`MaxLevel` không nằm trong dòng chép vào `KSkill`** (`TSkillInfo+8`, `skills.json` `max_level`); `KSkillRow::from_cells` không
  đọc ô `MaxLevel` → test dựng dòng tay phải đặt `r.max_level`, không thì mọi kỹ năng "đã tối đa" (không lên cấp, không cộng điểm).
- **`re_calls callers` có thể gán nhầm cho hàm bao trùm**: `calls.json` thiếu mốc hàm nên `0x08096F50` (hàm log) "gọi"
  `0x080E5BF0` — thật ra là ProcessFunc `0x080993A0` nằm sau nó. Xác nhận bằng `grep call` trong dump rồi lần về `push ebp` gần nhất,
  và tra chỉ số ProcessFunc ở ctor `0x08099600` ((disp − 4)/8).
- **Không đặt tên vtable theo thứ tự JX1**: §11 cũ gán `IsTargetOnly` cho `+0x24` — dump `vtab.py` + đọc từng getter cho thấy
  `+0x24` là `IsExpSkill`, `+0x44` `PeaceCanUse`, `+0x4c` `IsAura`, `+0x50` `WeaponSkill`, `+0x5c` `ReqLevel`. Mỗi lần dùng `vtable+X`
  trong luật phải tra bảng đã kiểm (§15.1).
- **MSVC coi tham số che thành viên là lỗi** (`C4458` với `/WX`): `KSkillList::skills` (mảng ô) và tham số `KSkillManager* skills` — đặt
  tên tham số khác (`mgr`).
- **Script vá phải idempotent hoặc chạy đúng một lần**: `patch_b3b.py` chạy lại sau lỗi anchor đã chèn khối `KWeaponSkillTable`/ô lệnh
  3 lần (anchor còn nguyên vì `new` chứa `old`) → C2011/C2086. Kiểm `new in s` trước khi thay, hoặc tách từng tệp; gỡ bằng
  `dedupe_b3b.py` (giữ lần đầu). Và không sửa script bằng heredoc Bash khi có `\n` (bị đổi thành xuống dòng thật) — dùng Write.
- **Tên thuộc tính phải tra `KMagicAttribId.cpp`**: `defense_v` không tồn tại (đúng là `armordefense_v` 30 / `adddefense_v` 150);
  `parse_string_to_magic_attrib` lặng lẽ bỏ tên lạ → kỹ năng không có hiệu ứng mà test vẫn thấy hồi chiêu được đặt.
- **Nhánh "đi tới +300" không dành cho đòn thường**: `0x0809B840` gọi `CanCastSkill` (có kiểm tầm) *trước* khi `ProcessCommand`
  xét khoảng cách → mục tiêu ngoài tầm bị bỏ ngay; chỉ kỹ năng không kiểm tầm (dạng 3 Param1≠1, 4, style 1/3…) mới đi tới. Client
  2.0 tự đi tới rồi mới gửi gói; zone giữ `attack_request` cũ tự đi tới cho client Godot đến B4.
- **`+0x168c` (chế độ chiến đấu) mặc định 0 cho mọi npc** (chỉ `SetFightMode 0x08079B30` đổi): quái vẫn đánh được vì AI gọi thẳng
  `CastSkill` (kiểm `PeaceCanUse` chỉ ở nhánh người chơi); đọc ai ghi một ô trước khi coi nó là điều kiện chung.

### 0.6 Cách làm đang tốt — giữ nguyên

- **Con trỏ `KNpc*` trong test chết sau khi thêm thực thể** (`entities_.insert` có thể dời bảng): `Arena` giữ `h`/`p` → sau
  `spawn_player` thứ hai phải `mutable_entity` lại (giá trị rác `watchers.size() == 0` làm đoán sai hướng). Cùng lớp lỗi: test
  CI chập chờn "items survive spawn → snapshot → spawn" (run 112/114 của `main`, chỉ Windows) là `KItemList* list` đọc sau
  `remove_player` (đã `erase`) — Release đọc trúng bộ nhớ cũ nên qua, Debug 222/300 lần sai. Sửa: lấy `next_id` trước khi xoá.
  Muốn thấy lỗi gốc của lần chạy đầu (lần chạy lại qua): `tools/ci_annotate.py` giờ đọc `Testing/Temporary/LastTest.log` trước
  `--rerun-failed` và ghi chú thích "first run: …".
- **Vector `attribconstdata` bắt đầu từ `Data0`** (bộ nạp `0x080E75A0` ghi `[i] = Data_i`), nhưng vòng `0x0807D4C0` đi
  `size−1 … 1`: `Data0` của `[hide]` là độ trong suốt (70), không phải kỹ năng — đọc `.ini` gốc (`settings/attribconstdata.ini`,
  GBK) để biết nghĩa từng ô.
- **`+0x19a0` không phải "steal feature"**: `0x8FBFE40 = 0x8FBF4E0 + 200·12` → id 200 `[hide]`; đoán theo tên thuộc tính gần
  (186/187) là sai — luôn tính lại chỉ số từ địa chỉ vector.
- **Bảng ini theo thứ tự ô mặc, không theo thứ tự khoá**: `AbradeRate.ini` liệt kê `Weapon` trước nhưng `KItemSet::Init` ghi
  Head vào `+0x20`, Weapon vào `+0x2c` — phải lấy con trỏ ra (`lea eax, [ebx+N]` trước mỗi `GetInteger`) chứ không đếm thứ tự khoá.
  `[ecx+4] == 4` trong `0x0806D560` là `detail` (ngọc bội), không phải genre — tra layout `KItem` (+0 genre, +4 detail).
- **Chạy đánh (dạng 11) chỉ chạy `0x080E8650(sk, 0)` + 1 khung** (`WaitTime` của kỹ năng): test với `WaitTime 0` dừng sau 2 khung
  — không phải lỗi zone; đặt `WaitTime` cho kỹ năng thử. `walk_to` gọi `stop_action` → phải đặt thưởng tốc độ **sau** khi dừng
  hành động cũ, và `end_run` chỉ trừ khi `doing == run` (đúng prologue `m_Doing == 0x12` của nhị phân).
- **Đuôi tự lặp lệnh của người chơi** (`update_action`) lặp cả buff lên mình → mỗi lần lặp `cast_skill` làm vỡ ẩn thân; giờ chỉ
  lặp đòn `TargetEnemy` nhắm kẻ khác.
- **Đọc từng thân hàm rồi mới viết**, ghi địa chỉ vào chú thích và vào `LINUX-SERVER.md`; số trong test
  lấy từ bảng thật (`level_add.txt` hệ Kim: 4/9/8/1/8/0/1; `newplayerini00`: 35/25/25/15, máu 204).
- **Bản đồ offset → tên** (bảng §10.1) trước khi viết struct: `KNpcAttrib.h` ghi offset ở từng trường, nhờ
  đó đọc thêm hàm mới là map thẳng vào mã.
- **Sinh mã từ dữ liệu nhị phân** (`tools/gen_magic_ids.py` → 335 tên ma pháp) thay vì gõ tay.
- **Một commit một lát**, HANDOVER cập nhật cùng commit, safe branch/tag đi theo.
- **Client lấy đúng bản 2.0**: bố cục `.ini` xuất JSON, chuỗi từ `stringtable_core`, màu từ engine, luật
  gói chữ của `KMouseOver`; đối chiếu ảnh chụp với client thật khi có thể.
- Kiểm tra CI trên trình duyệt tích hợp, không cần đăng nhập; các step chạy nhanh trước (gofmt / include /
  catalogue) nên lỗi hiện trong 1–2 phút.

### 0.7 Lệnh chạy nhanh (Windows, PowerShell)

```
cmake --preset windows-msvc && cmake --build --preset windows-msvc-release     # build Release
ctest --preset windows-msvc-release                                            # 208 test C++
cd services && go test ./... && go vet ./... && gofmt -l .                     # Go
python tools/check_includes.py && python tools/check_log_catalog.py            # trước commit
python tools/dev.py assets        # xuất map/UI/vật phẩm/bảng người chơi (player.json) từ bản cũ
python tools/dev.py start         # gateway + zone (dừng server trước khi build lại)
client.cmd                        # nhấp đôi: mở client Godot (tự tìm Godot 4.7 trong WinGet; hoặc python tools/dev.py client)
python tools/dev.py e2e           # kịch bản đầu-cuối TCP + WS
Godot --headless --path client tests/UiCheck.tscn                              # 160 kiểm tra giao diện
```

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
| O1 | **PostgreSQL thay kho tệp** — `PgStore` **xong**, còn đo 20 000 nhân vật | lớn | `gateway.db` = chuỗi kết nối → `persist.PgStore` (pgx, SQL thuần, jsonb); test tuân thủ chung cho tệp + PostgreSQL, CI chạy thật (service container) cả unit lẫn e2e. Chưa đo khởi động với 20 000 nhân vật: máy này không có PostgreSQL. |
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
| **M9** *(đang làm)* | O1 (PostgreSQL) | 2 | 20 000 nhân vật, gateway khởi động < 3 giây; test crash giữa chừng không mất dữ liệu. **Kho PostgreSQL xong + CI thật**; số đo 20 000 cần một PostgreSQL tại chỗ (Docker) — chưa có trên máy này. |
| ~~**M10**~~ **đạt 2026‑09‑17** | Mổ nhị phân bản Linux: kỹ năng + hàm script | 3–4 | 1506 hàm script (game) + 438 (gateway), **chữ ký đọc bằng máy cho cả 1506** (1149 đối số cố định, 1496 biết số trả về); **109 tệp settings, 104 nối được cột/khoá mã đọc (736)**; hai lớp `KTabFile`/`KIniFile` đặt tên từng phương thức; 431 stub PLT có tên. Công cụ `re_elf/re_calls/re_luasig/re_tables`, [LINUX-SERVER.md]. |
| **M11** *(đang làm)* | Vật phẩm, túi đồ, trang bị, rơi đồ | 4 | Test tính chất: không âm, không nhân bản. **Lát A + B + C1 + C2 xong**: bảng vật phẩm đọc đúng cột và xuất JSON (`pkg/jxold/item`); zone có `KItem`/`KInventory`/`KItemList`/`KItemGenerator` theo luật cũ, lưu/nạp qua `RoleData.items`; giao thức vật phẩm client ↔ zone (luật uống thuốc/hồi máu đối chiếu nhị phân Linux); cửa sổ Túi đồ, Thông tin nhân vật (trang Trang bị + Thuộc tính) và chú thích vật phẩm dựng từ bố cục 2.0, icon từ bảng vật phẩm, nhấc–đặt–mặc–cởi–uống qua giao thức; script `AddItem`/`AddGoldItem` đúng thứ tự bản Linux + lệnh GM `?gm ds` (E phần 1); **lát D xong**: quái chết rơi tiền/đồ theo `DropRateFile`/`Treasure` (luật `GenRandomItem` bản Linux), vật thể trên đất (`ObjData`), nhặt/vứt; **lát F xong**: ma pháp tiền tố/hậu tố (`Gen_MagicAttrib` + `magicattrib_limit`, đúng nhị phân Linux) và hoàng kim khi rơi; **E phần 2 xong** (`AddStackItem`, `HaveItem`, `GetItemCount[Ex]`, `DelItem[Ex]`, `HaveCommonItem`, `DelCommonItem`); **chú thích vật phẩm từng dòng theo nhị phân client 2.0**, tên/mô tả từ bảng của client, hậu tố kích hoạt (`GetEquipEnhance`). Còn (dồn sang M12/M14): bạch kim (lỗ khảm), `AddItemEx`, móc Lua khi dùng (`Check_ItemUsable`/`OnUseItem`), chia đội khi rơi, kho đồ (cần hội thoại NPC — M13), giao dịch (M14). |
| **M12** *(bắt đầu 2026-09-18)* | Chiến đấu và kỹ năng theo **nhị phân Linux** (thuộc tính nhân vật → công thức → kỹ năng/`Skills.txt` → kinh nghiệm/cấp) | 6 | **Đối chiếu số với nhị phân Linux**: cùng đầu vào, cùng kết quả; các bảng `settings\npc\player\*` đọc đúng cột. |
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

### 2026-09-18 (phiên tiếp theo, phần 9) — M12 lát B3c-2: mòn đồ (`AbradeRate.ini`, `KItemList 0x08201940`)

- **Mổ nhị phân** (`LINUX-SERVER.md` §16.3): `KItemSet::Init 0x0806E250` (khoá → ô: Head `+0x20` … Mask `+0x4c`; Defend `+0x5c`, Move `+0x98`, AdvPlatina `+0xd4/+0x110/+0x14c`; `[Repair]` `+0x188..`), `0x0806D560` (chế độ ≤ 2, slot ≤ 14; ngọc bội `+4 == 4` cấp `+0x344 > 5` → AdvPlatina), `KItem::Abrade 0x08066570`, `0x08201940` (slot 0..13 trừ 11; gói 0x9b; hỏng: `[0x830D244]` "damage" không ai ghi → AbradeToZero: `G_STR_ITEM_ABRADETOZERO` `[0x978A614]`, `0x08067540` phế phẩm, `KItemList::Remove 0x082006B0`, `KPlayer::AddItem 0x080B5180` phòng 0/0xe, `KItemSet::Remove 0x0806DB90`), `0x08201D90`/`0x080658B0` mất n % (từ `0x080B9FA0`), gọi: `CastSkill`, `ReceiveDamage 0x0808B148` (`+0x118c < máu trước`), `0x0807C2F0` mỗi bước, Lua `AbradeEquipments 0x08107AA0`; `[0x830CA78]` không ai ghi; chuỗi `lang/vn/stringtable_core.txt` (TCVN3 → UTF-8 bằng bảng của `KTextTCVN3.go`).
- **Zone**: `KAbradeRate` (`KItem.h`, nạp `abrade_rate.json`, `range_of`), `KItem::amulet_tier` (+0x344, chưa sinh), `KItem::abrade` sửa (0 → −1), `KItem::abrade_percent`, `KSubWorld::abrade_equipments/abrade_equipments_percent/wear_result`, móc `cast_skill`/`receive_damage`/vòng `moved`, Lua `AbradeEquipments`, `main.cpp` `zone.abrade_rate_file`; Go `KAbradeRate.go` + test, `jxassets export-abrade-rate`, `dev.py`.
- **Test**: `test_KItem.cpp` +1 ca / 30 kiểm (đánh mòn kiếm, bị đánh mòn giáp, đi không mòn gì, hỏng → phế phẩm vào túi + `G2C_ITEM_MOVE` + thông báo, không mòn thêm, mất 10 %, đòn mất máu mòn giáp), ctest 208/208, Go 1 test (bảng thật: Attack Weapon 256, Defend 320, Move Foot 2560/Horse 5120).
- **Chưa**: bảng phế phẩm `+0x1e88`, cấp ngọc bội trong bộ sinh, tạo npc, đồng hành, chết/PK (dùng `abrade_equipments_percent`), ngựa, mượn dáng, gói 0x85/0x87 (B4).
- commit: `JX NEXT: M12 lat B3c-2 - mon do AbradeRate` (nhánh, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 8) — M12 lát B3c-1: thân pháp style 1 (sáu dạng `MisslesForm` 8..13)

- **Mổ nhị phân** (`LINUX-SERVER.md` §16.2): `0x08087F70` (bảng nhảy `0x08254AC0`), đường nhảy `0x08087CF0` (`+0x12a0 = 40` bước × `+0x129c`, `0x08081B70` bay, `> 20`, `+0x195c` khung, `+0x1960` hướng qua `g_nSin`), `0x0807B320` (doing 4, `+0x1938 = 5·bước − 5`, gói 0x54), mỗi khung `0x080818F0/0x080817E0` (`+0x2c` độ cao, `0x0807C2F0` dời phần nghìn), dạng 8 `0x08084930/0x08087620` (doing 14, con ở 60 %), dạng 10 `0x080807E0/0x08084E00` (doing 20, pha `+0x1964`), dạng 11 `0x08084A10/0x080853B0` (doing 18, `+0x14b0` thưởng, `+0x164c` đếm, `0x08080BD0` bước, `0x08078790` DoAttack bị DoStand huỷ), dạng 12 `0x08084B40/0x08086E50` (doing 19, `ChildSkillNum`, trễ `0x080E8650(sk, i)`), dạng 13 `0x08084C90/0x08080760` (doing 23, `Param1/Param2`, `SetPos 0x0807AFA0` gói 0x4f), bộ chia `0x08087770` (bảng `0x08254A5C`), `DoWalk 0x0807B620` (gói 0x51, ngưỡng thể lực), `DoAttack/DoMagic 0x08078790/0x08078850`; cột `Param1 +0x50`, `Param2 +0x54`, `ChildSkillNum +0xb4`, `ChildSkillId +0xb8`.
- **Zone**: `KDoing` thêm `jump/special_skill/run/special_cast/jump_attack/blink`, `KNpc::in_action`, trường `cast_kept1/2/_target`, `jump_steps/jump_dir/jump_arc/height/phase/run_counter/run_bonus`; `KSkills.cpp`: `do_special_skill`, `jump_to`, `start_jump`, `jump_frame`, `start_special_skill/special_skill_frame`, `start_run_attack/run_frame`, `start_special_cast/special_cast_frame`, `start_blink/blink_frame`, `start_jump_attack/jump_attack_frame`, `cast_child_skill`, `end_run`, `stop_action`; `update_action` 6 case, `do_stand`/`walk_to`/`set_pos`/lệnh đi dùng `in_action`; proto `ACTION_JUMP` (Go/GD sinh lại); client `KProtocolProcess.gd` (`aim`, `skill`), `KNpc.gd` (`ACTION_JUMP` → chạy tới điểm rơi).
- **Test**: `test_KNpcCommand.cpp` 11 ca / 257 kiểm (nhảy 200 → 16 khung, 12/khung, độ cao 35 giữa đường, 480 tối đa, ≤ 20 từ chối, hồi chiêu ngay; thuấn di 300 sau 6 khung; nhảy đánh rơi cạnh mục tiêu rồi đòn 18 khung, con ở khung 10 và đứng ngay; chạy đánh +90/s, tới nơi, thưởng trả; thi triển nhiều lần 3 pha; đòn con 60 % gây 5 máu; lệnh khác chờ khi đang thân pháp), ctest 207/207, Go xanh.
- **Chưa**: mòn đồ, tạo npc, đồng hành, chết/PK, ngựa, mượn dáng 186, gói 0x85/0x87 (B4), `NpcSetHide`, hoạt hình nhảy client 2.0 (B4).
- commit: `JX NEXT: M12 lat B3c-1 - than phap style 1` (nhánh, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 7) — M12 lát B3c-0: ẩn thân `[hide]` 200

- **Mổ nhị phân** (`LINUX-SERVER.md` §16.1): `ProcessFunc 200 0x08097860` ("Hide + %d = %d"; `[obj+0]` = cờ gỡ của `0x0807D210`/`0x08095B40`), `KNpc::SetHide 0x0807FF80` (gói 0x4f 5 byte / `0x0807FBB0` gói 0x4c), `IsInvisibleTo 0x08079200` (`+4 = m_Index`, `+0x19a4` cờ đang gửi), `0x0807A870` gửi quanh + duyệt 9 vùng `0x080E1C80/0x080E1B80` (bỏ người mà npc vô hình), ẩn thân vỡ `0x0807D4C0` (vector `[hide]` từ `size−1` xuống 1; `attribconstdata.ini`: `Data0=70` độ trong suốt, `Data1..4 = 713, 1235, 1258, 1267`), gọi từ `CastSkill 0x080884A3`, `DoDeath 0x08089359`, ngựa `0x0807D520`, Lua `OpenProgressBar 0x081082D0`; `0x0809F190` đồng bộ npc theo yêu cầu (từ chối khi ẩn), gói 0x4d `0x080810C0` (bit ẩn), `0x080873B0` (0x85 không phát khi ẩn, aura), 186 mượn dáng `0x08099170`, 187 `0x08099370`, Lua `SetHide 0x0810AD80` / `NpcSetHide 0x08101C10`; bộ nạp `attribconstdata` `0x080E75A0` (`[i] = Data_i`).
- **Zone**: `KNpc::hide/hide_syncing` (+0x19a0/+0x19a4; `clear_attrib` đặt 0), `KSubWorld::invisible_to/set_hide/break_hide` (`KNpc.cpp`), `wake_viewers_near` + lọc ẩn trong `look_around` (`KInterest.cpp`), `KNpcAttribModify` 200 (`ctx.set_hide`) và 187, `cast_skill`/`do_death` gọi `break_hide`, Lua `SetHide`, `EntityInfo.hide` (proto Go/GD sinh lại); sửa lỗi kèm: người chơi không tự lặp lệnh với kỹ năng không đánh địch / nhắm mình.
- **Test**: `test_KNpcCommand.cpp` 8 ca / 155 kiểm (2 ca mới: ẩn → người xem quên (DESPAWN), không học lại khi còn ẩn, thi triển làm vỡ → học lại ngay khung sau; `SetHide` trên npc, gỡ khỏi npc không ẩn giữ 0, chết làm vỡ), ctest 204/204, Go test xanh.
- **Chưa**: style 1 thân pháp, mòn đồ, tạo npc, đồng hành, chết/PK, ngựa, mượn dáng 186, gói 0x85/0x87 (B4), `NpcSetHide`.
- commit: `JX NEXT: M12 lat B3c-0 - an than [hide] 200` (nhánh, `safe/jxnext-2026-09-17`, `main`).
- **CI chập chờn giải xong**: `main` #112/#114 đỏ ở "a character's items survive spawn → snapshot → spawn" (Windows, chạy lại qua):
  `test_KItem.cpp` giữ `KItemList* list` qua `remove_player` (danh sách bị `erase`) rồi đọc `next_id()` — Debug tại chỗ sai
  222/300 lần (`5 == 1`), Release đọc trúng bộ nhớ cũ. Sửa test (`next_before`), `ci_annotate.py` ghi chú thích lỗi của lần chạy
  đầu từ `LastTest.log`. commit: `JX NEXT: sua test items round trip + ci_annotate doc LastTest.log`.

### 2026-09-18 (phiên tiếp theo, phần 6) — M12 lát B3b: lệnh thi triển của client, `CanCastSkill`, bảng vũ khí → kỹ năng

- **Mổ nhị phân** (`LINUX-SERVER.md` §16): `NpcSkillCommand 0x080DD130` (gói 80, `0x080A79B0` kiểm đồng bộ, `IsAura`), `SendCommand 0x0809B750` (vòng 5 ô `+0x169c`, `FindSame`), `0x0809B510`/`0x0809B4B0` (sống 18 khung, bỏ), `ProcessCommand 0x0809B9E0`, kiểm `0x0809B840` (`+0x194c`, `+0x168c`→`PeaceCanUse`, `SetActiveSkill`, `CanCast`, `CanCastSkill`, phí), `SetActiveSkill 0x08086D90` (`+0x12a8` = bán kính), `GetCurrentSkill 0x080848B0`, `CastSkill 0x08088350` (gói 0x5a, `0x08201940` mòn đồ), `DoSkill 0x08088150` (khung theo tốc độ, `clearallcd`), `0x08087F70` style 1 dạng 8..13 (bảng `0x08254AC0`), `0x08087770` bảng `m_Doing` 0..24 (`0x08254A5C`), bắn `0x08085020` (60 %, `Cast`, `SetSkillCoolTime`), `DoStand 0x08080030`, `CostSkill 0x08078B10`, `0x08079A90` + bộ nạp `武器物理攻击对照表.txt` `0x0805F18D` (3 bảng `0x0830AF00/0x0830B0A0/0x0830B230`), `0x0809F370` khoảng cách, `SetFightMode 0x08079B30` (chỗ duy nhất ghi `+0x168c`), `KItemList 0x081F92E0/0x081F9F70`, `[FSKILLS]` của `newplayerini00.ini`.
- **Zone**: `KNpcCommand` + `KNpc::commands`/`cast_param1/2`/`cast_target`, `KDoing::magic`; `KSkills.cpp`: `cast_skill_request`, `send_command`, `check_command`, `process_command` (gọi mỗi khung trước `update_action`), `cast_skill(e,p1,p2,target)`, `do_skill`, `on_skill(e)` thay `swing_skill`/`on_skill(e,target)`, `can_cast_skill`, `weapon_physics_skill`, `weapon_eqt_limit`, `cost_skill`, `set_active_skill`, `current_skill`, `skill_instance`; `attack_request` cũ = kỹ năng vũ khí (tự đi tới ngoài tầm, bật `fight_mode` vì đánh thường `PeaceCanUse = 0`, tự lặp khi mục tiêu sống — client Godot); AI `cast_skill(e,target)` → `cast_skill` thật; `KWeaponSkillTable` (`weapon_skill.json`, `zone.weapon_skill_file`); `load_skills` cho nhân vật không sổ hai đòn thường 1/2; proto `CastSkillReq` 1112, `EntityAction.skill_id/skill_level/aim`; Go `KWeaponSkill.go` + `export-weapon-skill` (+ `dev.py`), persist `NewRole` thêm `RoleData.skills` từ `[FSKILLS]`.
- **Test**: `test_KNpcCommand.cpp` 6 ca / 113 kiểm (SendCommand: sổ/đầy/già; từ chối: aura, id, điểm âm, mục tiêu, TargetEnemy lên mình, WeaponSkill, EqtLimit, ReqLevel; CastSkill: khung 18, hướng 48, bán kính 100, trừ 10 nội lực, bắn ở khung 10, hồi chiêu 30, thiếu nội lực; PeaceCanUse, TargetSelf tại điểm → lên mình, do_magic; đi tới +300 / bỏ xa; attack_request cũ lặp đòn); `test_KSubWorld`/`test_KNpcAI`/`test_KPlayer`/`test_KItem` đi qua đường mới; Go 2 test mới (bảng vũ khí, kỹ năng khởi đầu).
- **Chưa**: style 1 thân pháp, mòn đồ, ẩn thân, tạo npc, đồng hành, chết/PK, bảng vũ khí thật (xin chủ dự án), client (B4).
- commit: `JX NEXT: M12 lat B3b - lenh thi trien ky nang` (nhánh, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 5) — M12 lát B3a: sổ kỹ năng `KSkillList`, điểm và kinh nghiệm kỹ năng

- **Mổ nhị phân** (`LINUX-SERVER.md` §15): 15 chỗ tăng `[0x8BC99B4]` → mọi hàm của sổ `0x080E4230..0x080E5D90` đọc trọn (FindSame/FindFree/GetLevel/GetCurrentLevel/GetAllInc/GetCount×2/GetTotalLevel/CanCast/IsCooling/SetNextCastTime/Get…/ReduceCoolTime/ClearCoolTime/IsForbidden/SetForbidAll/SetNpcSkill/Add/Remove/IncreaseLevel/UpdateEnhance/ChangeCurrentLevel/AddSkillLevelInc/CastPassivesAtLevel/Rollback/GetExpPercent/AddSkillExp/Serialize/GetNext), ctor `0x080E4950`, bố cục 12 ô/ô kỹ năng + `KSkillLevelIncNode` (`0x0825830C`) + map `+0xf14` (= `KNpc+0x115c` của B2a); vtable `KSkill` kiểm từng getter (sửa §11); `KSkillManager` `m_SkillInfo[+8] = MaxLevel`; người chơi: handler gói 80 `0x080DD130`, `SendCommand 0x0809B750`, `ProcessCommand 0x0809B9E0`, kiểm `0x0809B840`, `SetActiveSkill 0x08086D90` (`+0x12a8` = bán kính), `GetCurrentSkill 0x080848B0`, `CastSkill 0x08088350`, `DoSkill 0x08088150`, `SetSkillCoolTime 0x080847B0`, `CostSkill 0x08078B10`, `GetPlayerIdx 0x08078A80`, `AddSkillPoint 0x080BD460` (+ handler `0x080DCB20`), `ForbitSkill 0x080B2950`, `SetAForbitSkill 0x080AE9E0`, nạp/lưu `0x080C0240/0x080C0390/0x080BF340`, ProcessFunc 139/288..290 (`0x080993A0`/`0x08097250`), `ReceiveDamage 0x0808AA38`, `LevelUp 0x080AFCBC`, `ClearAttrib 0x0807F341`, 17 hàm Lua (`0x0812C430`…), `NewPlayerBaseAttribute.ini` chỉ có 5 điểm (không có kỹ năng khởi đầu).
- **Zone**: `KSkillList.h/.cpp` mới (thuần + `KSkillListHost`), `KNpc::skill_list`/`skill_mgr` (thay `skill_enhance`), `clear_attrib` hạ cấp hiện tại, `KNpcAttribModify` 139/288..290 (+ `skill_host` trong ngữ cảnh), `trigger_auto_skills` dùng `CanCast` thật + đặt hồi chiêu, `receive_damage` chia kinh nghiệm kỹ năng, `KSubWorld` (`skill_host`, `load_skills`/`save_skills`, `send_skill_list`/`send_skill_level`, `give_skill_exp`, `set_skill_cool_time`, `forbit_skill`/`set_a_forbit_skill`, `add_skill_point_request`, thụ động lên cấp sau `level up`), `KNpcAI::set_active_skill` kiểm ô, `KPlayer::skill_max_level_addons`, `KSkillTable::id_of`, `KMapInstance` `C2G_ADD_SKILL_POINT`, proto `RoleSkill`/`AddSkillPointReq`/`SkillLevelSync`/`SkillListSync`/`SkillForbidSync` + id 1111/2116..2118 (Go + Godot sinh lại), `ScriptFuns.cpp` 17 hàm Lua.
- **Test**: `test_KSkillList.cpp` 8 ca / 260 kiểm (ô, đếm, nạp/lưu; IncreaseLevel + map tăng sát thương + thụ động; nút cộng cấp/ô chỉ-nhờ-cộng/học-bỏ học/ClearAttrib/nắp 64; hồi chiêu + cấm; kinh nghiệm: thanh 1/1024, lên cấp, cờ không lên, phần trăm, tối đa, rollback; thế giới: nạp từ RoleData → `G2C_SKILL_LIST` → lưu; AddSkillPoint 11 bước; kinh nghiệm từ đòn đánh + `allskill_v` + `reduceskillcd` + cấm); ctest 196/196, `check_log_catalog`/`check_includes` sạch, Go vet/test sạch.
- **Chưa**: B3b (đường lệnh thi triển của client, `CanCastSkill` + bảng vũ khí, style 1, `Abrade`), bảng chuyển sinh `0x0830CA14`, chuỗi thông báo (bảng ngôn ngữ), đồng hành, client (B4).
- commit: `JX NEXT: M12 lat B3a - so ky nang KSkillList` (nhánh, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 4) — M12 lát B2c: kỹ năng tự động, bản đồ thi triển kèm, ô trống khi đánh lùi

- **Mổ nhị phân** (`LINUX-SERVER.md` §14): `KNpc::Init 0x0807DBD0` dựng 5 danh sách (`0x08188F60`, tên GBK), nút `0x08189000` (khoá `id<<8|cấp`, % byte thấp của |v2|, khung chờ `|v2|>>8`, cờ `+0x34` kỹ năng của mình, `+0x38` bắn vào mục tiêu), ProcessFunc 272/195/196 (`0x08097420/0x080973D0/0x08097380`, `0x08188A10` chờ lần đầu), duyệt `0x08188BB0` (cây con mục tiêu → khung, `0x080E4540` KSkillList, `0x080847B0` hồi chiêu, gói `0x85`), đối số thật của 4 chỗ gọi (`0x0808BEDF`, `0x0808B507`, `0x0808B1D3` nạn nhân, `0x0808B13A`), `oncastskill 0x0809AE60` + `0x080821C0` (style `& 0x4007`), `ClearAttrib` xoá `+0x1830`/`+0x18e8`; **`0x08081B70`** ô trống (bước `+0x129c` = 12, `0x080F1E50/0x080E0990` bản đồ script, `0x080F0530/0x080E0A30` loại chướng ngại + hình chéo, bảng `0x08254A48`, `KnockBack` không đánh lùi khi thất bại); `CanCastSkill 0x080E8AE0` và tạo npc `0x080E8770` đọc trọn, ghi §14, chưa port; `+0x1624/+0x129c` đính chính = bước đi.
- **Zone**: `KNpc.h` (`KAutoSkillList` 5, `KAutoSkillEntry`, `auto_skills`, `on_cast_skills`, `clear_attrib` xoá đúng hai thứ), `KNpcAttrib::step_length` (+0x1624/+0x129c), `KNpcAttribModify` 272/195/196/274 (+ `tick` trong ngữ cảnh), `trigger_auto_skills` (port thật, danh sách mỗi khung gọi trong `tick` trước `process_frame_state`), `cast_on_cast_skills` cuối `skill_start_event`, `knock_back` dùng `knock_back_free_spot` (thất bại → không đánh lùi), `barrier_kind` (ô chéo; `TestBarrier` đạn dùng chung); CI Linux sửa (`apply_state_modifier` không dùng, commit `016e977`).
- **Test**: `test_KAutoSkill.cpp` 5 ca / 84 kiểm (thêm/bớt nút kể cả số âm và giữ cờ, chờ theo mục tiêu 0x08188A10, mỗi khung 5/10/15, bị đánh → 901 vào kẻ đánh + đánh trúng → 902 vào nạn nhân với `attribconstdata` chặn lặp, bản đồ thi triển kèm, đánh lùi 8×12 / dừng trước loại 1 / bay qua loại 3 / ô chéo 3 và 5 / bước 0); ctest 188/188, `check_log_catalog` 0 thiếu, `check_includes` 0 thiếu.
- **Chưa**: `CanCastSkill` + bảng vũ khí→kỹ năng, tạo npc + đếm ngược, `0x081FEE60`, gói 0x85, `KSkillList` (B3), client (B4).
- commit: `JX NEXT: M12 lat B2c - ky nang tu dong, o trong khi danh lui` (nhánh, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 3) — M12 lát B2b: hệ đạn (`KMissle`) đúng từng lệnh `jx_linux_y`

- **Mổ nhị phân** (`LINUX-SERVER.md` §13, mỗi hàm một dòng địa chỉ + công thức): bộ nạp `missles.txt` `0x0805D210/0x08074300` (18 cột server đọc, mẫu `0x0830ED80` 0x188 byte), `CreateMissle 0x080EA310` (chép mẫu `0x08074AA0` từng ô, 13 thuộc tính `missle_*`, `slowmissle_b`, cuộn `ColVanish` với `+0x141c`, bộ sửa trạng thái trên `missrate`), `KMissleSet::Add 0x08076F00`, `CastMissles 0x080ECAC0` (bảng nhảy `0x08258478` 8 dạng; luật mục tiêu `0x080EED70`; hướng `0x080EEEC0` = **`g_GetDirIndex` JX2**), 6 hàm sinh (`CastWall 0x080EBB20`, `CastLine 0x080EC2F0`, `CastExtractiveLineMissle 0x080EBF00`, `CastSpread 0x080EB150`, `CastCircle 0x080EB720`, `CastZone 0x080EC690`), độ trễ `0x080E8650` (bảng `0x08258424`), thi triển con `0x080EAFF0`, đạn tức thời `0x080EA720`, khung đạn `0x08076950` (từ `KRegion::Activate 0x080E2660`, sau npc), `PrePareFly 0x08076550`, `Activate 0x080760E0` (bước `[0x082E1B00] = 10`, chu kỳ `DmgInterval`, cờ va chạm theo đổi ô), `OnFly 0x080758E0` (7 kiểu bay), `CheckBeyondRegion 0x08074F10`, mốc tương đối `0x08074D70`, `CheckCollision 0x08075770` + `0x080749A0`/`0x080E20B0`/bộ lặp `0x080F2610`/`0x080F2280`, `ProcessCollision 0x08075630/0x08075710`, `ProcessDamage 0x080753F0`, `DoCollision 0x08075340`, `DoVanish 0x08075210`, `OnMissleEvent 0x080EE810` (chỉ kỹ năng style 0: `vtable+0x10` = `GetStyle`), `Cast` kiểu launcher 2, `TestBarrier 0x080F05C0` (loại 1/3), bảng `g_nSin/g_nCos 0x082E1080/0x082E1180` (cắt), vùng 16×32 (`[0x08258BE4/8]`).
- **Zone**: `KMissle.h/.cpp` mới (hàm thành viên `KSubWorld`, tên theo `Core/Src/KMissle.h` cũ, ô 32 tuyệt đối + lệch 1/1024 kể cả 32768), `KSubWorldConfig::missles` + `zone.missles_file`, `activate_missles()` trong `tick` sau npc trước `flush_pending_drops`, bản đồ không ngủ khi còn đạn; `skill_cast` style 0 → `cast_missles`, 14 → `cast_instant_missle`; `missle_skill_cast` (launcher đạn); `deliver_attribs` bỏ; `KMath.h` bảng sin/cos + `g_GetDirIndex` bản JX2 (`KnockBack`, hướng npc, đạn cùng dùng); `KNpcCurrentAttrib::missle_vanish_rate` (`+0x141c`); `KSkillRow::client_send` là số (2 = client tự bắn); `update_action` bỏ kiểm tầm giả — đạn quyết; quái test không mẫu `camp_animal`; `KMissleTable::basic_attack` (mẫu 64/65) khi không có bảng.
- **Go**: gói `jxold/missle` (`Parse/Load/Write/Read/Template`, test 1), `jxassets export-missles` → `client/assets/missles.json` (441 dòng/57 cột), `dev.py assets` gọi thêm.
- **Test**: `test_KMissle.cpp` 14 ca / 319 kiểm (bảng, độ trễ 6 kiểu, `CreateMissle` từng ô, đánh thường gần: 2000→2020→2040→trúng ở ô 64 khung 7 rồi `DmgInterval` chặn, hết đời khung 11; đánh xa `ColVanish` với lệch đúng 32768; vòng/tường/lưới/quạt từng toạ độ; kiểu 7 cửa sổ tới đích; kiểu 5 ngắm lại khung 9 (413/958, hướng 60) và nhảy; trượt/số lần trúng/`DmgRange`; chướng ngại 1/3 và mép bản đồ; sự kiện chạm + mất sinh đạn con có cha/mục tiêu; style 14; đạn rơi khi launcher đổi camp/rời map); `test_KMath` theo JX2 (phải 48, lên 32); `test_KSubWorld` đòn cận chiến chờ đạn (khung 17); ctest 183/183, Go 5/5, `check_log_catalog` 0 thiếu, `check_includes` 0 thiếu.
- **Chưa**: kỹ năng tự động (B2c), kỹ năng tạo npc, `0x08081B70`, `Player+0x5a50`, sổ kỹ năng nhân vật (B3), gói đạn/trạng thái ra client + `KMath.gd` JX2 (B4).
- commit: `JX NEXT: M12 lat B2b - he dan KMissle` (nhánh `claude/handover-doc-review-b0d015`, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo, phần 2) — M12 lát B2a + lát C: lõi sát thương và trạng thái đúng từng lệnh `jx_linux_y`

- **Mổ nhị phân** (`LINUX-SERVER.md` §12, mỗi hàm một dòng địa chỉ + công thức): `ReceiveDamage 0x0808A4A0` (1 152 dòng, chặn/đòn tăng cường, tổng ô, ngũ hành qua bảng **tương khắc** `0x830ED2C` + `0x0807BAA0` kháng tối đa, `CheckHitTarget 0x0807ED60`, 5 kiểu sát thương theo thứ tự vật lý → băng (đóng băng) → hoả → lôi → độc (`0x0807BD60` gộp) → ma pháp, hút máu/nội/thể, đánh lùi `0x08087940`, chí mạng, choáng, `0x0807F9D0 → OnHurt 0x0807F780` 50 % phẳng); `CalcDamage 0x08089C90` (khiên tĩnh + gỡ trạng thái `[staticmagicshield_v]`, khiên động, kháng `0x0807BCD0/0x0807BB20/0x08078910` với cặp anti và **anti kháng tối đa `+0x1350`**, trả đòn kiểu 6 và trả độc khi `bReturn`, PK `[0x8BADF50]`, `Posion2Mana`, `sorb` phần nghìn, khiên nội **không giảm đòn khi nội âm**, `0x08078A10` me2X/X2me, nội kẻ đánh ÷ −100.0f, bản ghi sát thương `min(dmg, máu)`, `damage2addmana`, chết ở `≤ 0` (`0x08089920`: máu = 0 cho npc)); `AppendSkillEffect 0x0807CE70` + `0x0807C700/0x0807C950/0x0807CD30/0x0807CA90/0x0807CBD0` (ô 14/15 **chéo** `+0x1400/+0x1418`; `f` = `Player+0x5954` nội công; vũ khí `0x080B0D50` theo detail/particular `0x081F92E0/0x081F9F70`); `MixPoisonDamage 0x08099F90` (ô type làm `interval` → không bao giờ gộp); `SetStateSkillEffect 0x08086260` (phản `returnskill_p`, `ignoreskill_p`, `ignorenegativestate` qua `IsTargetEnemy`, luật làm mới theo cấp/`a8`/`a9`, nút lưu giá trị **âm**, biểu tượng `0x08079240`), `RemoveStateSkillEffect 0x0807D310`, `0x080792C0` bộ sửa trạng thái; `ProcessState 0x0808B610` (độc tick theo `interval` với `bReturn = 1`, băng bỏ khung **lẻ**, choáng bỏ mọi khung, thuốc, danh sách trạng thái), `0x0808BE80` mỗi giây (`addblockrate` đếm npc 256 `0x0807A0E0`, `manatoskill` `+0x139c`); `Cast 0x080EA920` (bảng style `0x0825843C`), `CastInitiativeSkill 0x080EAC90`, `CastPassivitySkill 0x080E8530`, `StartEvent 0x080EAB90`, `CreateMissleMagicAttribsData 0x080E9E90` (map `+0x115c`, theo `ChildSkillId` tới `BaseSkill`), `ProcessDamage 0x080753F0`, `ProcessCollision 0x08075630` + bộ lặp, `CreateMissle 0x080EA310` (mẫu `missles.txt` `0x830ED80`), `attribconstdata.ini` ở `KSkillManager::Init 0x080E7532` (`[returnskill_p]` 725/25, `[ignoreskill_p]` 724/15, `[autoreplyskill]`, `[autoattackskill]`, `[staticmagicshield_v]` 721), `addphysicsdamage_p 0x0809A7F0` đủ 11 loại (bảng `0x082557E0`), hằng `10000.0f/100.0f/−100.0f` ở `0x08255294/8C/90`.
- **Zone**: `KNpc.cpp` (mới, 1 000 dòng) + `KSkills.cpp` (mới) là hàm thành viên `KSubWorld` (`receive_damage`, `calc_damage`, `calc_resist`, `append_skill_effect`, `set_state_skill_effect`, `set_immediately_skill_effect`, `remove_state_skill_effect`, `modify_attrib`, `set_poison`, `modify_five_resist_max`, `knock_back`, `do_hurt(anti)`, `do_hurt_chance`, `process_frame_state`, `per_second_attribs`, `skill_cast`, `cast_initiative_skill`, `cast_passivity_skill`, `skill_start_event`, `create_missle_magic_attribs_data`, `deliver_attribs`, `swing_skill`, `on_skill`); `KNpc.h`: `KStateNode` (giá trị âm), `KStateModifier`, `KDoing::knock_back` + `knock_from/knock_dest`, `crowd_block_rate`, `mana_skill_enhance`, 6 biểu tượng trạng thái, `damage_lock`, `boss_flag`, `last_damage_id`, `last_poison_id`, `skill_enhance` map; `KNpcAttrib.h`: `anti_resist_max` (`+0x1350`), `add_physics_damage_percent[9]`; `KSkill.h`: `KMissleMagicAttribsData`, `KDamageSlot`, `KDamageType`, `KSkill::basic_attack(1|2)` (dòng 2/3 thật, số 0), `KSkillTable::attrib_data`; `KSkill::load_skill_level_data` thiếu script → mỗi setting nhận "0" (thay vì bỏ trống — máy này thiếu script đánh thường); `KItemList::weapon_particular`; `KSubWorld::tick` gọi `process_frame_state` mỗi khung (khung bị choáng/băng lẻ **không** chạy AI/lệnh/di chuyển) và `per_second_attribs` mỗi 18 khung; `hit()`/`heal()` cũ bỏ: đòn ở 60 % = `on_skill` → `skill_cast` kỹ năng đang chọn (npc) hoặc 1/2 (người, theo vũ khí); style 0/14 tạm tải thẳng lên mục tiêu (`deliver_attribs`, đúng phần `ProcessDamage`) tới khi có đạn (B2b); `process_state` (mỗi 10 khung) đủ hồi máu/nội/thể theo nhị phân; `config/zone.json` không đổi (`pk_damage_percent` là trường cấu hình `KSubWorldConfig`, mặc định 100).
- **Go**: `skill.LoadAttribConst/ParseAttribConst` (`npcres.ParseIni`), `Table.AttribData` → `skills.json` `attrib_data` (6 mục trên máy này), `jxassets export-skills` tìm `attribconstdata.ini` cạnh `skills.txt`; test Go 4/4.
- **Test**: `test_KNpcDamage.cpp` 11 ca / 206 kiểm (kháng mềm, `AppendSkillEffect` từng ô kể cả ô chéo, `CalcDamage` khiên/sorb/chết, trả đòn + nội, `ReceiveDamage` thứ tự ô + hút + chí mạng + choáng + băng, ngũ hành hai chiều, trạng thái thêm/làm mới/hết hạn/gỡ, độc đặt/gộp/tick, băng bỏ khung lẻ, `manatoskill`, `Cast` style 2/3 + đánh thường không bảng); `test_KItem` "giết quái máu 1" nới 20 → 200 tick (đòn có thể trượt 5 %); **169/169 ctest**, Go 4/4, `check_log_catalog` 0 thiếu (29 câu + 20 trường mới), `check_includes` 0; zone bật cổng 18001: `Đã nạp bảng kỹ năng · số kỹ năng=1628`.
- **Chưa**: đạn bay (B2b), kỹ năng tự động, `0x08081B70`, sổ kỹ năng nhân vật (B3), gói trạng thái 0x87 và đánh lùi ra client (B4).
- commit: `JX NEXT: M12 lat B2a - loi sat thuong va trang thai` (nhánh `claude/handover-doc-review-b0d015`, `safe/jxnext-2026-09-17`, `main`).

### 2026-09-18 (phiên tiếp theo) — M12 lát B1: bảng kỹ năng + số theo cấp từ script, đúng nhị phân Linux

- **Mổ nhị phân** (`LINUX-SERVER.md` §11, từng thân hàm): tham chiếu duy nhất tới bộ nạp cấp là **vtable `KSkill`
  `0x082587A8`** (thứ tự khớp `ISkill` của `Skill.h` nên đặt tên được cả 18 hàm ảo; `LoadSkillLevelData` = ô +0x14);
  `KSkillManager::Init 0x080E7200` (id 1..2000, style âm bỏ, style 13 → `KThiefSkill`; **dòng sau ghi đè dòng trước**
  cùng id — bảng thật có id 521 hai dòng); `GetInfoFromTabFile 0x080E9200` (60 cột, mặc định `AttackRadius` 50, `DoHurt`
  100, `EqtLimit` −2; `this` của nó lệch **+4** so với `KSkill` — thấy được khi đối chiếu offset với `AddMagicAttrib`);
  `InstanceSkill 0x080E6E10` (chỉ style 0..4 và 14; `m_pOrdinSkill[2000][64]` ở `+0x8AAC0`); `LoadSkillLevelData
  0x080EE4B0(this, cấp, dòng)` (ghi chú phiên trước ngược đối số); `ParseString2MagicAttrib 0x080EE380` (`std::map`
  tên → id ở `0x0830EB98`, bỏ `skill_desc` 318, `"v1,v2,v3"` qua `KSG_StringGetInt` — số thực làm rớt hai giá trị sau);
  `AddMagicAttrib 0x080EDCC0` (4 mảng 20 ô: đạn 15..25/325..338, **sát thương 56..75 ở ô cố định** theo bảng nhảy
  `0x08258498` — chỉ ô 0..14 đếm, `addskillexp1/2` v1 = 0 → id chính nó, 76..82 bỏ; còn lại `v2 == 0` → tức thời, khác
  → trạng thái; `skill_param2_v` lấy v2 như JX1; 304..309 `addskilldamage` = `{v1, v3, v2}`).
- **Go**: `pkg/jxold/skill` (đọc như `Init` + `GetInfoFromTabFile`, ô giữ chuỗi để zone áp mặc định nhị phân; cột đường
  dẫn GBK, cột chữ TCVN3), `jxassets export-skills` → `client/assets/skills.json` (1 629 dòng, 114 cột, 285 script cấp,
  3,4 MB), `dev.py assets` chạy luôn; test gói.
- **Zone**: `KSkill.h/.cpp` (`KSkillRow` từ ô + mặc định; `KSkill::add_attrib`; `load_skill_level_data` gọi
  `GetSkillLevelData(setting, data, cấp)` của **chính script cấp đã chuyển sang Lua 5.4** qua `KLuaScript::call_value`
  mới — số hay chuỗi như `lua_isnumber`/`lua_isstring`; `KSkillTable`; `KSkillManager` một cho mỗi map, cache theo
  (id, cấp), luật style), `KSubWorld::skills()`, `zone.skills_file`, 5 câu log mới. Khởi động: `skill table loaded`
  1 628 kỹ năng trong 80 ms.
- **Test** `test_KSkill.cpp` 6 case / 151 khẳng định: `KSG_*`, từng nhánh `AddMagicAttrib`, mặc định cột, vòng
  `LoadSkillLevelData` (số / chuỗi / bỏ `LvlData` bắt đầu bằng '0' / dừng khi nil), bảng + manager (dòng sau thắng,
  style 7/13 không tạo, 14 tạo), và **dữ liệu thật**: kỹ năng 4 Thiếu Lâm Côn pháp cấp 1/10/20 qua `shaolin.lua`:
  `addphysicsdamage_p` {25,−1,2} / 60 / {100,−1,2}, `attackratingenhance_p` 35 / 148 / 275, `deadlystrikeenhance_p`
  6 / 45 (`Conic`) — đúng `SKILLS.shaolin_gunfa`. ctest **158/158**, Go xanh, `check_log_catalog`/`check_includes` sạch.
- **Dữ liệu thiếu**: 156/285 script cấp không có trong bản Linux trên máy (đồng hành `partner\*`, sự kiện, `npc\*`
  tên GBK) — kể cả `special\长兵物理攻击.lua` / `远程物理攻击.lua` của **đánh thường** (kỹ năng 1/2); chuỗi dự phòng
  `bin/Server` có bản JX1 với mọi số 0 (bản Linux `physicattack.lua` cho `physicsenhance_p` 100). Zone cảnh báo
  `skill level script missing`; cần chủ dự án lấy các tệp này từ server thật.
- CI: run #100 (main, 820c890) xanh; run #99 (nhánh login-system-upgrade, cùng commit) đỏ ở **Windows Test (Debug)**:
  test "a character's items survive spawn -> snapshot -> spawn" hỏng một lần rồi qua khi `ci_annotate` chạy lại →
  chập chờn, chưa tìm nguyên nhân.
- Còn của lát B: B2 thi triển + `ReceiveDamage` (lát C), B3 `KSkillList` của nhân vật, B4 client (xem §0.4).

### 2026-09-19 (tối) — tạm dừng theo yêu cầu chủ dự án; server bật để test; đọc dở hệ kỹ năng

- Chủ dự án yêu cầu: mở **đủ 980 map**, tạo tài khoản mới vào chơi như người chơi, đưa cách chạy
  client, **tạm dừng và ghi note**. Đã: `jxassets export-all` (2,4 GB) → `client/assets/maps/*`, zone
  `maps = "all"`; server chạy từ worktree này (`python tools/dev.py start`, cổng 17001/17100/17102);
  client: `python tools/dev.py client` (Godot 4.7, `next/client`), đăng nhập tài khoản mới bất kỳ
  (chế độ `auth_mode: dev`), tạo nhân vật (hệ + giới tính + quê) → số theo `newplayerini`.
- `main` đã fast-forward tới nhánh làm việc (commit HANDOVER §0 + client `--auto` chụp trang thuộc tính);
  safe branch/tag đi theo. CI của các push cuối bị **cancelled** do push dồn — run cuối cùng
  (`client --auto chup them trang thuoc tinh`) đang chạy lúc dừng; phiên sau xem lại trước tiên.
- Đọc dở **hệ kỹ năng** (ghi ở §0.4 dòng M12 lát B/C): bảng 114 cột, số theo cấp trong Lua
  (`SKILLS[...]`, `GetSkillLevelData`, bộ nạp `0x080EE4B0`), mọi đòn đi qua `ReceiveDamage 0x0808A4A0`.
  Chưa viết mã cho phần này.

### 2026-09-19 — M12 lát A phần 3: **kinh nghiệm, lên cấp, cộng điểm, đồng bộ thuộc tính ra client**

- Đọc thêm (`LINUX-SERVER.md` §10.5): `KDamageRecord::Add 0x0809BC70` (3 ô sát thương, 1200 khung),
  chia kinh nghiệm khi chết `0x0809BDD0` (`m_Experience × sátThương / máuMax`, quái chết **không** cộng
  trong OnDeath mà qua DoDeath; `OnGlobalNpcDeath` chỉ là script nhiệm vụ), `KPlayer::AddExp 0x080B00C0`
  + `CalcExp 0x080A7C80` (chênh cấp: ±5 đủ, 6–15 `(25−|d|)/20`, hơn 15 một nửa, ≥ cấp 100 chỉ 1 với
  quái < 90, quái cao hơn 55–69 cấp `(−19d−1030)/300`), lõi `0x080AFEA0` (kẹp ở mốc, lên cấp mất phần dư),
  `expenhance_v/_p`, `add120skillexpenhance_p` vào `Player+0xc8..+0xd4`.
- Zone: `KNpc::damage_records` + `add_damage_record` (ghi khi người chơi đánh trúng), `share_experience`
  khi chết, `KPlayer::calc_exp` / `add_exp` (đúng các bước trên, chia đội để lát đội), gói mới
  **`G2C_PLAYER_ATTRIB` / `PlayerAttribSync`** (35 trường: cấp, kinh nghiệm, điểm, 5 chỉ số gốc + hiện
  tại, máu/nội/thể, chính xác, phòng thủ, sát thương, 5 kháng, tốc độ) gửi khi vào map / lên cấp / cộng
  điểm / mặc–cởi đồ; lệnh **`C2G_ADD_POINT` / `AddPointReq`** (thuộc tính 0..3, số điểm, seq) →
  `KPlayer::add_base_*` có kiểm điểm; mặc/cởi đồ giờ chạy `UpdataCurData` thật (sức mạnh/phòng thủ của
  đồ tính vào); `EnoughAttrib` so với điểm **hiện tại**.
- Client: `Game.player_attrib` + tín hiệu `player_attrib_changed`, `Game.add_point()`; trang thuộc tính
  `UiStatus` hiện đủ số theo `KUiStatus::UpdateData` (điểm bị đồ đổi hiện `hiện tại(gốc)`, sát thương
  `min-max`, kinh nghiệm `x/y`, điểm còn), 4 nút `+` bật khi còn điểm và gửi lệnh.
- Test: `test_KPlayer` +2 case (thế giới: gói đồng bộ, cộng điểm, 2 con heo → lên cấp 2; CalcExp),
  UiCheck 155 → 160. Còn: chia đội, hình phạt chết, kỹ năng (lát B), công thức sát thương (lát C).

### 2026-09-19 — M12 lát A phần 2: **thuộc tính nhân vật trong zone theo nhị phân Linux**

- `KNpcAttrib.h`: `KNpc::base` (m_XXX) / `KNpc::cur` (m_CurrentXXX) với offset nhị phân từng trường, cặp
  "yan" (`life_max_v()` = max); `KNpc::clear_attrib` = `ClearAttrib 0x0807EE60`. Các trường phẳng cũ
  (`life`, `attack_rating`, `min_damage`…) đổi hết sang khối mới (KSubWorld, KNpcAI, test).
- `KMagicAttribId.h` sinh từ 335 tên của nhị phân (`tools/gen_magic_ids.py`); `KMagicAttrib.h` tách khỏi `KItem.h`.
- `KNpcAttribModify::modify`: **217 ProcessFunc đọc từng thân hàm** (bản gọn 2 832 dòng), viết lại đúng phép
  toán: `_p` nhân giá trị gốc (`lifemax_p`: `+= v × m_LifeMax / 100`), `staminamax_*` tính lại `SitAdd`,
  băng: thời gian đóng băng `min(54, 4·(v/5)+10)`, độc 60/10, `sorbdamage` kẹp 0..500, cặp `anti_*` giữ
  tổng ở hai ô, cờ `forbit_*` chỉ khi giá trị **== 1**, `add_boss_damage` gán không cộng, `addphysicsdamage_p`
  theo loại vũ khí (0..5, 10→6), `fastwalkrun_p/yan` gỡ bonus cũ − cộng bonus mới trên **gốc** × max(p, yan),
  `nomovespeed` trừ (`sub ecx, edx`), `strength_v/dexterity_v/vitality_v/energy_v` chỉ người chơi →
  `ChangeCurXXX` (nhanh nhẹn: `+4n` chính xác, `+n/4` phòng thủ). Chưa làm: kỹ năng (139–144, 183–189,
  195–198, 207–216, 272–274, 288–290, 295–297) — chờ lát kỹ năng.
- `KPlayer` (trong `KNpc::player`): `load_from` đúng thứ tự `LoadFrom 0x080C16D0`, `set_npc_physics_damage`
  (= `0x080AF740` + `GetWeaponDamage 0x081F9310`: `(gốc + Σmin_v) × (100 + Σenhance_p) / 100`, cận chiến `+
  sức mạnh/5`, xa `+ nhanh nhẹn/5`, tay không `sức mạnh/5+1`), `updata_cur_data` (ClearAttrib → điểm hiện
  tại = gốc → ReCalcEquip: 7 thuộc tính cơ bản + 6 ma pháp, hậu tố lẻ theo `GetEquipEnhance`), `level_up`
  (±5 điểm, ±1 kỹ năng, `level_add`, đổ đầy máu/thể/nội), `add_base_*` (kiểm điểm, sinh lực ×
  `LifePerVitality`…), `save_to` cho PlayerSave.
- `KPlayerSet` (zone) + `jxold/player` (Go) + `jxassets export-player` → `client/assets/player.json`;
  gateway `NewRole` tạo nhân vật mới từ `newplayerini%02d` (Thiếu Lâm 35/25/25/15, máu 204, nội 16, vũ khí
  khởi đầu); `RoleStats` thêm may mắn, điểm thuộc tính, điểm kỹ năng, trùng sinh, camp.
- Test mới `test_KPlayer.cpp` (5 case, 160 khẳng định) + Go `player` package; mọi test cũ qua (69 → 74 case).
  Chưa có: gói đồng bộ thuộc tính cho client, lệnh cộng điểm từ client, kinh nghiệm khi giết (lát tiếp).

### 2026-09-19 — M12 lát A phần 1: **bản đồ `KNpc`/`KPlayer` của nhị phân Linux** (`LINUX-SERVER.md` §10)

Công cụ mới `tools/re/re_attribmod.py`: duyệt 217 `ProcessFunc` của `KNpcAttribModify`, cho mỗi id
ma pháp biết ô `KNpc` nó cộng/ghi (dừng ở tail-jump cuối hàm — bản đầu chạy lố sang hàm kế nên gán
thừa; đã sửa và kiểm với `lifemax_v → +0x1a14`, `life_v → +0x118c`, `lifepotion_v → +0x1f0/+0x1f8`).
Đọc tay `KNpc::ClearAttrib 0x0807EE60`, `KNpc::Init 0x08082680`, `KNpc::SetTemplate 0x08082E20`,
`KPlayer::LoadFrom 0x080C16D0`, `LevelUp 0x080AF800`, `UpdataCurData 0x080AF550`, `ReCalcEquip
0x080AF3E0`, `SetNpcPhysicsDamage 0x080AF740`, bộ nạp `level_exp`/`level_add` `0x080C4FD0` và các
`Get*` của `KLevelAdd`, `stamina.ini` → `KPlayerSet`. Kết quả: bảng ô gốc ↔ hiện tại ↔ "yan"
(giá trị dùng thật = max(thường, yan)), công thức người chơi (`chính xác = nhanh nhẹn × 4 − 28`,
`phòng thủ = nhanh nhẹn / 4`, `sát thương tay không = sức mạnh / 5 + 1`, vũ khí cận chiến `+= sức
mạnh / 5`, xa `+= nhanh nhẹn / 5`; lên cấp `+5` điểm thuộc tính `+1` điểm kỹ năng, máu/thể/nội theo
`level_add`, kháng theo cấp có kẹp 120 khi hệ số âm; kinh nghiệm cấp = cột 2 + 10 000 × cột 3,
7 bảng trùng sinh). Chưa có code — lát A phần 2 là hiện thực trong zone theo đúng bảng này.

### 2026-09-18 (khuya) — hậu tố "kích hoạt" khi mặc: `KItemList::GetEquipEnhance` theo nhị phân Linux

`0x081FD2C0`: số hậu tố sáng của món đang mặc = (hệ nhân vật **sinh** hệ món: 1) + mỗi ô trong hai "ô kích hoạt"
(`ms_ActivedEquip`, bảng `0x082E7460` khớp nguồn cũ: nón←áo,dây chuyền; áo←nhẫn dưới,đai; đai←ngọc bội,hộ uyển;
vũ khí←dây chuyền,áo; giày←vũ khí,nón; hộ uyển←giày,nhẫn trên; dây chuyền←đai,nhẫn dưới; nhẫn trên←vũ khí,nón;
nhẫn dưới←hộ uyển,ngọc bội; ngọc bội←giày,nhẫn trên) có đồ mà hệ đồ đó sinh hệ món (+1); ngựa và các ô JX2 sau
nó = 3; `+0x4c7c ≠ −1` (kích hoạt toàn bộ — bộ hoàng kim) = 3 (chưa làm). Bảng ngũ hành của server
(`0x080741D0` → `0x0830ED18` sinh, `0x0830ED2C` khắc) vào `KMath.h` (`kAccrueSeries`, `g_IsAccrue`). Zone
`KItemList::equip_enhance(part, series)` (M12 sẽ dùng khi áp ma pháp lúc mặc); client `KUiItemView.equip_enhance`
→ chú thích món đang mặc sáng đúng hậu tố (CoreShell `GetActiveAttribNum` cho `UOC_EQUIPTMENT`). Test cả hai phía.

### 2026-09-18 (khuya) — M11 lát E phần 2: hàm script vật phẩm nhiệm vụ theo nhị phân Linux

`AddStackItem`, `HaveItem`, `GetItemCount`, `GetItemCountEx`, `DelItem`, `DelItemEx`, `HaveCommonItem`,
`DelCommonItem`, `GetTotalItemCount` (`ScriptFuns.cpp`), mỗi hàm mổ từ `jx_linux_y` (địa chỉ ở `LINUX-SERVER.md`
§9). Điểm khác nguồn Windows: bản JX2 nhận **tên** vật phẩm nhiệm vụ (tra `\settings\item\questkey.txt` cột 名称 →
`DetailType`) hoặc số; `DelItem` xoá **cả chồng** đầu tiên có detail đó (mọi phòng), `DelItemEx`/`GetItemCountEx`
chỉ trong túi (`pos_equiproom = 3`); `GetItemCount` đếm **số chồng**; `AddStackItem([tag,] count, genre, detail,
particular, level, series, luck[, magic1..6])` = AddItem + chồng `count` khi ≤ max (25 > 20 → chồng 1), rồi
`KPlayer::AddItem(.., bStack=1)` gộp vào chồng sẵn có như `give_item` của ta. Test `[lua]` mở rộng; zone 69/69.
Chưa có: `AddItemEx`, `forbit_takemedicine` / `*PotionCounter` (chờ M12 trạng thái), `Check_ItemUsable`/`OnUseItem` (chờ script).

### 2026-09-18 (tối) — tên / mô tả vật phẩm lấy từ **bảng của chính client 2.0** (`items/client_vNNN.json`)

Client thật tra bảng `\settings\item\NNN\*.txt` trong kho của nó (`KItem::operator=(row)` chép tên +0x30, ảnh
+0x80, mô tả +0xd0); chữ khác bảng server ("Kiếm cổ thời Đường, thân dài và sắc, chém ngựa như chém bùn." ≠
"…chém ngựa cũng đơn giản như chặt tay."). `jxassets export-items` nay bung 21 bảng của mỗi thư mục 000–004 từ
pak ra thư mục tạm, đọc bằng cùng bộ đọc và ghi `client_vNNN.json` (chỉ tên/ảnh/mô tả theo dòng; 004: 25 012
dòng — client có nhiều hoàng kim/bạch kim hơn server); `export-item-images` lấy cả ảnh các bảng đó (2 675 ảnh).
Client: `KLibOfBPT.gd` tìm dòng đúng như `KItemGenerator` (trang bị `particular*10+cấp−1`, thuốc `detail*5+cấp−1`,
hoàng kim/kịch bản theo `gen_param` mới của `ItemView`), `KUiItemView` dùng chữ đó cho túi và chú thích; không có
dòng → giữ chữ server. Ảnh `build/shots/auto_item_tip.png` chụp lại (mô tả của client, ngắt đều 2 dòng theo `g_StrWrap`).

### 2026-09-18 (tối) — CI: thêm 4 chỗ `-Werror=sign-conversion` (KItem.cpp) + KSocket; MSVC bật `/w44365` để bắt tại chỗ

Run #76/#77 vẫn đỏ: `equipment_[d]`/`equipment_[detail]` với `int` (KItem.cpp 105/108/109/250) và
`Listener::port()` trả `int` vào `uint16_t`. Máy này không có GCC, nên bật **`/w44365`** (signed/unsigned
mismatch, tương đương `-Wsign-conversion`) + `/external:W0` cho MSVC trong `CMakeLists.txt`: build tại chỗ
giờ đỏ đúng những chỗ CI Linux đỏ. ctest 145/145 Release + Debug sau khi sửa.

### 2026-09-18 (tối) — chú thích vật phẩm dựng lại từng dòng theo **nhị phân client 2.0** (mở UPX `gamecl.exe`); sửa bộ đọc bảng: ô trống = mặc định của server

Chủ dự án: "Thiếu hệ item và các dòng chưa nằm giữa"; "hình ảnh phải lấy đúng chuẩn bản 2.0 còn code thì bản
Linux"; "phải kiểm tra từng dòng nhị phân". Ảnh: `build/shots/auto_item_tip.png` (đã gửi).

- **Mở được nhị phân client 2.0**: `gamecl.exe` là core + UI nén UPX 3.03 → `tools/re/re_upx.py` (giải nén NRV2E,
  bỏ lọc E8, dựng lại import từ stub nạp) → ảnh bộ nhớ 34,6 MB mà mọi `re_*` đọc như ELF. Tài liệu mới
  [`CLIENT-2.0.md`](CLIENT-2.0.md): địa chỉ, bảng chuỗi, luật. (Thú thật: phiên trước đã có `upx_unpack.py` +
  `re_pe.py` cho việc này mà tôi không xem lại trước khi viết; giữ cả hai, `tools/re/README.md` ghi rõ — bộ mới
  thêm import tĩnh và nối được vào đồ thị gọi hàm.)
- **`KItem::GetDesc` `0x00636460`** đọc từng dòng: màu tên (Blue 100,100,255 khi có ma pháp; Yellow hoàng kim;
  Violet; Red hỏng), ` [Cấp N]`, dòng **"Thuộc tính Ngũ hành: <màu hệ>Kim "** (`G_ITEM_3..7`, chỉ trang bị không
  mặt nạ), mô tả qua **`g_StrWrap(.., 40)`** của `engineFree.dll` (chia đều `n/40+1` dòng), độ bền `"Độ bền: %3d /
  %3d"` / "Không thể phá hủy", yêu cầu trắng/đỏ, ma pháp tiền tố HBlue + **`[min-max]`** (`GetMagicRange` trên
  `m_CMAIT`) / hậu tố DBlue khi chưa kích hoạt, `"\n"` trống cuối (bọc `0x6b66a0`). Chuỗi lấy từ
  `\lang\vn\stringtable_core.txt` (xuất mới `ui/du-lieu/chuoi-core.json`; bộ xuất giữ khoảng trắng cuối và
  giải mã TCVN3 thuần). Bảng màu tên của engine dump từ `enginefree.dll` (Violet 188,64,255, DYellow 127,127,0…).
- **`KMouseOver`**: mọi dòng **căn giữa**, ngắt 64 ký tự, rộng ≥ 26 ký tự, 13 px/dòng, vị trí `ALW_GetWndPosition`
  (giữa con trỏ, dưới 32 px ở nửa trên màn hình) — `UiMouseHover.gd` viết lại; `KTextEncode.gd` (thẻ màu),
  `KMagicRange.gd` (`items/magic.json` mới của `export-items`), `ItemView.version` + đủ 6 ô ma pháp (chẵn/lẻ).
- **Lỗi thật tìm ra khi đối chiếu**: `KTabFile::GetInteger` của server Linux (`0x08227E10`) trả **mặc định khi ô
  trống** (`GetValue 0x08227A00` fail với ô dài 0) — bộ xuất Go dùng `atoi("") = 0`. Với `magicattrib.txt`
  (bộ đọc `0x081EEE30`, mọi số mặc định −1) **116/330 dòng có hệ trống = mọi hệ (−1)** chứ không phải Kim (0)
  → `Gen_MagicAttrib` của ta đã bỏ sót chúng cho hệ 1..4. Sửa: `cell(t,row,col,def)` + mặc định đúng từng bộ
  đọc (`tools/re/re_tabdesc.py` in mảng mô tả cột của mọi `KBPT_*::ReadRow`: trang bị hệ 0/giá 0/cấp 1/khác −1).
- Test: UiCheck 153 (+19: từng dòng của GetDesc, `g_StrWrap`, thẻ màu, `KMouseOver`), run.gd 262, zone
  `[item]` 18/18, e2e `AUTO_MAGIC count=6`. Còn (ghi §4 `CLIENT-2.0.md`): tên/mô tả theo **bảng của client**
  (chữ khác bảng server), `nActive` khi mặc (tương sinh), giá cửa hàng.

### 2026-09-18 (chiều) — M11 lát F: ma pháp tiền tố / hậu tố khi sinh trang bị (`Gen_MagicAttrib`) — đối chiếu nhị phân Linux từng dòng

Từ lát này trang bị rơi ra (và `AddItem(... magic1..6)`) có thuộc tính ma pháp thật: "Sắc bén", "của Mãnh Hổ"…
Mọi luật lấy từ `jx_linux_y`, ghi địa chỉ ở `LINUX-SERVER.md` §9; nguồn Windows chỉ dùng để đặt tên.
Ảnh: `build/shots/auto_item_tip.png` (đã gửi) — client thật, kiếm cấp 5 xin qua
`?gm ds AddItem(0,0,0,5,0,100,5,5,5,5,5,5)` trên bảng 004 ra 5 dòng ma pháp; `dev.py e2e|screenshot` in
`AUTO_MAGIC count=N`.

- **Đối chiếu nhị phân**: `KItemGenerator::Gen_MagicAttrib 0x0806AF70` — với mỗi ô i khi `level[i] ≠ 0`: ô chẵn
  tiền tố (`pos` 1), ô lẻ hậu tố (0); ứng viên = `GetCMIT(pos, loại, hệ, cấp)` (`0x08070B50`, bảng `m_CMAIT` dựng ở
  `0x08070D00`: dòng vào **mọi hệ** nếu `class = −1`, **mọi cấp từ cấp dòng tới 10**, mọi loại có tỷ lệ ≠ 0 — giống
  nguồn Windows); loại dòng đã dùng (`m_nUseFlag`) và dòng **cùng kind** với ô trước; giữ dòng có
  `DropRate[loại] > nDecide`; chọn đều một dòng; ba tham số quay `min + g_Random(max−min+1)`. **`nDecide` tuỳ phiên
  bản bảng của vật phẩm** (điểm bản JX2 khác Windows — nguồn Windows dùng `MAX_LUCKRAND 100000`):
  bản ≤ 1: `g_Random(100)/(luck/10+1)`; bản 2–3: `g_Random(1 000 000)/(luck/10+1)`; bản ≥ 4:
  `g_Random(1 000 000)·100/(10·luck+100)` — khớp thang `DropRate` của từng thư mục (000/001 tối đa 100 000,
  002+ tối đa 600 000). `Gen_Equipment 0x0806B3A0`: mặt nạ (loại 11) không ma pháp; sau khi quay, **kiểm
  `magicattrib_limit.txt`** (`0x08069D10`: thuộc tính có dòng giới hạn phải có `min[k] < value[k] < max[k]`, `max −1`
  = bỏ kiểm; bảng 004 cấm `allskill_v` 139) — rớt thì `g_Random(100)` rồi quay lại, **tối đa 21 lần** rồi không tạo.
  `SetAttrib_MA 0x08065710`: có `indestructible_b` (43) → độ bền −1. **Phiên bản vật phẩm hiện hành = 4**: ctor
  `KSubWorldSet 0x080F6A40` ghi `m_nItemVersion = 4` (`0x9777F34`, Lua `ITEM_GetLatestItemVersion`) — `zone.item_version
  = 0` của ta = bộ mới nhất (004) nên khớp. `g_Random 0x08226AD0` = LCG `seed·3877+29573` của `KRandom.cpp` cũ.
  `GenRandomItem`: `k = g_Random(4)+3`, ô `i ≤ k` = cấp → **4/5/6/6 ô** (lát D ghi "3–6" là chưa chính xác);
  quality 1 (hoàng kim) → `Gen_GoldEquip 0x0806A150` với `Detail` là dòng `goldequip.txt` (1-based).
- **Zone**: `KRandom.h` (mới, theo `Engine/Src/KRandom.h`); `KItemTemplateSet::magic_candidates()` (chỉ mục
  `m_CMAIT` 2×12×5×10), `magic_limit()` (đọc `magic_limits` của JSON); `KItemGenerator::equipment(detail, particular,
  series, level, &levels, luck)`, `gen_magic_attrib`, `check_new_item_attrib`, `set_attrib_ma`; `gen_random_item`
  quay ô ma pháp đúng luật trên và tạo **hoàng kim** cho `Quality 1` (349 dòng trong 77 bảng rơi đồ);
  `AddItem` của script nhận `magic1..6`. Phẩm chất 2/lỗ khảm (bạch kim) vẫn chưa làm — bảng rơi đồ của server Linux
  đặt `EnchasableRate 0` khắp nơi nên không gặp. May mắn (`m_nCurLucky`, `Player+0x5958`) chưa có → 0, sẽ có ở M12.
- **Bộ xuất Go**: `magicattrib_limit.txt` → `magic_limits` trong `items/v00x.json` (004 có một dòng).
- **Client**: chú thích theo `KItem::GetDesc` cũ + bảng màu `Engine/Text.cpp`: tên **xanh (100,100,255)** khi có
  tiền/hậu tố, vàng hoàng kim/bạch kim, tím tử tinh, vàng đồ nhiệm vụ, trắng còn lại; trang bị có " [Cấp N]"
  (`KUiItemView.name_color/title_of`); `--auto` xin thêm kiếm cấp 5 có ma pháp, chụp `auto_item_tip.png`.
- Test: 4 case mới (KRandom khớp dãy số cũ; chỉ mục CMAIT; Gen_MagicAttrib trên bảng nhỏ: tiền/hậu tố đúng ô, không
  trùng kind, giá trị trong khoảng, giới hạn làm rớt 21 lần → không có vật phẩm; **bảng thật 004**: 300 kiếm cấp 5
  quay ra tiền/hậu tố, không bao giờ ra 139); rơi đồ giờ kiểm luôn ô ma pháp; UiCheck +4 (màu tên). ctest 145/145
  Release + Debug, Go 17 gói, Godot run.gd 262, UiCheck 134, e2e `AUTO_ITEMS count=10 AUTO_MAGIC count=5
  AUTO_DROP dropped=true picked=true`.

### 2026-09-18 (trưa) — CI: GCC `-Werror=sign-conversion` ở `KItem.h` (từ lát B) — sửa

CI Linux đỏ từ commit ad86985 (lát B) vì `rooms_[room]` với `room` là `int` trong ba hàm inline của `KItemList`
(`room()`, `money()`, `set_money()`) — MSVC không bắt, GCC `-Wsign-conversion` bắt. Đã ép kiểu `std::size_t`.
Bài học: sau mỗi lát phải mở trang Actions (API bị giới hạn 60 lượt/giờ, dùng trình duyệt) — lần này bốn lát
liền không xem. Các job khác (MSVC, Go ×3, Godot) vẫn xanh.

### 2026-09-18 (trưa) — M11 lát D: rơi đồ, vật thể trên đất, nhặt, vứt — luật theo nhị phân Linux

Ảnh: `build/shots/auto_drop.png` (đã gửi) — client thật vứt kiếm xuống đất (vật thể `obj_wq_001.spr` của
`ObjData.txt`) rồi nhặt lại; `dev.py e2e` in `AUTO_DROP dropped=true picked=true` trên TCP và WebSocket.

- **Đối chiếu nhị phân** (ghi ở `LINUX-SERVER.md` §9): bộ nạp bảng rơi đồ `0x080A3B80` (mọi khoá + mặc định của
  `KIniFile::GetInteger`: `[Main] MoneyRate=20 MoneyScale=50 MinItemLevelScale=20 MaxItemLevelScale=10
  MinItemLevel=1 MaxItemLevel=10 Series=-1 EnchasableRate MinSocket/MaxSocket=1 IsTeamShare TeamShareRate`; `[i]
  Genre Quality Detail Particular RandRate MinItemLevel/MaxItemLevel=-1 Series=-1 … MagicLevel1..6`);
  `GenRandomItem 0x08083BB0` (chọn dòng theo `RandRate` cộng dồn trên `RandRange`; cấp = `(cấp quái−1)/scale+1`
  hai đầu, kẹp vào [Min,Max] của bảng rồi 1..10; **JX2 khác nguồn Windows**: 3–6 ô ma pháp = cấp vật phẩm
  thay vì chuỗi `MagicRate`; `Quality` 2/`EnchasableRate` → lỗ khảm bạch kim); vòng chết `0x08088B60`
  (`Treasure` lần: `g_Random(100) < MoneyRate` → tiền, không thì `LoseSingleItem 0x08088840`; bản đồ có cờ
  `+0x4f298` không rơi; `IsTeamShare` → chia đội); `LoseMoney 0x08081950` = `exp × MoneyScale/100 ×
  [ServerConfig] MoneyRate/100` (khoá của `gamesetting.ini`, mặc định 100); `SetItemBelong 0x080A4D90` =
  600 frame; nhặt (`0x080B8210`) kiểm `khoảng cách² ≤ 40000`. `NpcS.txt` cột 5 `Treasure`, cột 88 `DropRateFile`.
- **Dữ liệu**: `npcs.txt` giờ xuất `treasure` + `drop_rate_file`, và `npcs.json` mang luôn 77 bảng rơi đồ đọc được
  (`droprates`; 37 bảng sự kiện không có trong thư mục → quái đó không rơi, có cảnh báo); `export-objdata`
  (mới, `dev.py assets` chạy): 477 dòng `ObjData.txt` + 5 mức `MoneyObj.txt` → `client/assets/objdata.json`,
  265 sprite vật thể → `client/assets/sprites`.
- **Zone**: `KObj.h/.cpp` (`KObjDataSet`, `KGroundObject`, hằng 600 / 40000); vật thể trên đất là thực thể
  `KNpcKind::drop` (đi qua đúng hệ quan tâm/AOI như NPC, `EntityInfo.count` = số lượng/tiền, `template_id` = dòng
  ObjData); `drop_item`/`drop_money` (`GetFreeObjPos`: vòng 32 đơn vị quanh chỗ rơi, tránh vật thể khác, đất đi
  được), `object_tick` (đếm `LifeTime`, `belong` hết 600 frame thành của chung; map đang có đồ dưới đất không
  ngủ), `pick_up_request` (chủ sở hữu/khoảng cách/túi đầy → `G2C_ITEM_RESULT`; tiền → `add_money` + `G2C_MONEY`),
  `item_drop_request` (vứt xuống chân, đồ nhiệm vụ không vứt), `lose_treasure` + `gen_random_item` như trên
  (luck = 0, ma pháp/bạch kim/hoàng kim của bảng chưa quay — `Gen_MagicAttrib` chưa port; chia đội chưa có).
  `KNpcTemplate.treasure/drop_rate_file` + `KNpcTemplateSet::drop_rate()`; cấu hình `zone.objdata`,
  `zone.money_rate_percent`.
- **Giao thức**: `C2G_PICK_UP` (1109, `PickUpReq`), `EntityInfo.count`; gateway relay.
- **Client**: `scenes/KObj.gd` (vẽ sprite ObjData từ điểm tựa như vật thể bản đồ, tên/số tiền phía trên, click),
  `UiGame`: click vào đồ dưới đất → nhặt ngay nếu ≤ 180 đơn vị, không thì đi tới rồi nhặt; `Game.pick_up`;
  `--auto` thêm vứt + chụp `auto_drop.png` + nhặt.
- Test: 2 case zone mới (vứt/nhặt/xa quá/của người khác/hết hạn/tiền; giết 20 quái → 60 vật thể, cấp theo công
  thức, tiền theo `MoneyScale`); zone 65/65.
- **Lỗi tìm ra khi test lặp**: `EntityTable` là `std::vector` dày — thêm thực thể **trong** vòng lặp tick (quái chết
  → rơi đồ) làm vector dời chỗ, tham chiếu `KNpc& e` của vòng lặp treo (test đôi khi đổ). Sửa: đồ rơi xếp vào
  `pending_drops_`, đặt xuống đất sau vòng lặp (`flush_pending_drops`). **Lưu ý cho người sau**: script trap
  gọi `spawn_npc` trong tick cũng gặp đúng cạm bẫy này — chưa sửa, cần cơ chế hoãn tương tự.

### 2026-09-18 (sáng) — M11 lát E phần 1: `AddItem` / `AddGoldItem` cho script + lệnh GM `?gm ds <lua>` qua chat

Ảnh: `build/shots/auto_items.png` (đã gửi) — client thật trong map 1 nhận kiếm + thuốc do zone sinh qua
`?gm ds AddItem(...)`; `dev.py e2e` giờ in `AUTO_ITEMS count=2` trên cả TCP và WebSocket.

- **Đối chiếu nhị phân Linux** `AddItem` (`0x08120D30` → `0x08120B30`): < 6 số → 0; bản JX2 **chèn** ba giá trị lên
  đầu (phiên bản bảng hiện hành `g_SubWorldSet+0x34`, seed 0, 0) rồi gọi `Lua_NewItem` (`0x0811F230`, đòi ≥ 9 vì
  vậy) → `KItemSet::Add(genre, series, level, luck, detail, particular, magic[6]…)` — **cùng thứ tự nguồn Windows**
  `AddItem(genre, detail, particular, level, series, luck[, magic1..6])`; kiểm chứng thêm bằng 100+ chỗ gọi trong
  `D:\ServerLinux\server1\script` (`AddItem(6,1,18,1,0,0,0)`, `AddGoldItem(0, i)`, `AddGoldItem(szWhere, 0, 178)`).
  Seed ≠ 0 → `srand(seed)` trước khi sinh (chưa port — script không dùng). `AddItem` genre > 4 bị bản Linux từ chối
  (`cmp edi,4; ja`) nhưng script gọi `AddItem(6,…)` cho vật phẩm kịch bản → giữ theo script (genre 0/1/4/5/6).
- Zone: `ScriptFuns.cpp` thêm `AddItem`, `AddGoldItem` (`give_item` → túi; túi đầy → 0, bản cũ thả xuống đất — lát D);
  `KLuaScript::do_string` (LoadBuffer + ExecuteCode); `KSubWorld::gm_command` port `KGMCommand.cpp`: `?gm ds <lua>`
  (DoSct, chạy cho người gõ) / `?gm dw <lua>` (world), trả lời `GM: ok` / lỗi Lua, không phát ra chat; chỉ khi
  `zone.gm_chat` (mặc định tắt; `dev.py start` bật, `JX_GM_CHAT=0` để tắt); `zone.item_version` (0 = mới nhất) là
  chỗ của `g_SubWorldSet+0x34` (chưa tìm ra bản Linux đọc số này từ đâu).
- Test: 1 case (AddItem 5 loại, thiếu tham số/dòng sai → 0, AddGoldItem hai cách viết, GM tắt = chat thường, GM
  bật = ok/lỗi/không phát); zone 63/63. Client `--auto` thêm bước xin đồ + chụp `auto_items.png`.
- Còn của lát E: `AddStackItem`, `AddItemEx`, `RemoveItem*`, `GetItem*`, `Check_ItemUsable`/`OnUseItem`, `forbit_takemedicine`,
  `*PotionCounter` (đã có trường trong KNpc), ma pháp tiền/hậu tố `Gen_MagicAttrib`.

### 2026-09-18 (sáng) — M11 lát C2: túi đồ, cửa sổ nhân vật, chú thích vật phẩm — dựng từ bố cục client 2.0

Ảnh: `build/shots/ui_vat-pham.png` (đã gửi; dữ liệu mẫu từ bảng v004, chưa phải nhân vật trên zone).

- **Tệp bố cục đúng tên `gamecl.exe` 2.0 gọi** (344 tên `.ini` lấy từ ảnh exe đã giải nén): `随身物品.ini`
  (KUiItem — túi), `玩家装备与人物状态.ini` + `_装备.ini` + `_属性.ini` (KUiStatus — khung + trang trang bị + trang
  thuộc tính; JX2 tách trang thành tệp riêng, không phải `装备.ini` của nguồn 1.0), `弹出说明文字.ini` (KUiMouseHover),
  `储物箱.ini` (kho — mới xuất, chưa dựng). `export-ui` xuất thêm 6 màn này (`tui-do`, `thong-tin-nhan-vat*`,
  `chu-thich-vat-pham`, `kho-do`), bảng `<theme>\公共.ini` (`du-lieu/cong-cong`: `[ObjContColor]` màu ô vật phẩm,
  alpha 5 bit của renderer cũ), `\settings\magicdesc.ini` (`du-lieu/mo-ta-ma-phap`: câu mô tả từng thuộc tính,
  lỗ `#d1+`…) và `du-lieu/ten-ma-phap` (id → tên thuộc tính của bản JX2 lấy từ nhị phân Linux, 335 tên).
- **`export-item-images`** (mới): 2 305 sprite mà bảng vật phẩm gọi tên → `items/images/<đường dẫn>.png` +
  `items/images.json` (2 187 ghi được, 129 client không có → túi hiện tên). `dev.py assets` chạy luôn.
- **Godot** (`client/ui/`): `KWndObjContainer` (port `KWndObjectMatrix`/`KWndObjectBox`: ô = Width/HUnits, `UnitBorder`,
  màu nền theo `[ObjContColor]`, viền hoàng kim/bạch kim/tím nhấp nháy 60 ms, vật phẩm trên tay đặt **giữa** ô dưới
  chuột `left = x + (w+1)/2 − w`); `KUiDraggedObject` (tay cầm — client cũ không kéo-thả, click nhấc rồi click đặt);
  `KMagicDesc` (`#d/#f/#x/#s/#k/#m/#l` như `KMagicDesc::GetDesc`); `KUiItemView` (ô, màu tên theo `ex_type`, dòng chú
  thích, `EnoughAttrib` phía client với cấp/hệ/giới); `UiItem`, `UiStatus` (15 ô `ITEM_PART` + 2 ô JX2, trang thuộc
  tính điền tên/cấp/máu, số khác chờ M12), `UiMouseHover`; `KUiGameWindows` (lớp cửa sổ trong `UiGame`: phím **I**
  túi, **C** nhân vật, Esc trả đồ về tay/đóng; nhấc → `item_move`/`item_equip`, đúp/chuột phải → `item_use`/
  `item_equip`/`item_unequip`; đồ ở trên tay tới khi zone trả lời).
- Test: `tests/UiCheck.tscn` thêm 35 kiểm (130/130): KMagicDesc, ô 6×10×28 px, luật đặt giữa ô, 17 ô trang bị đúng toạ
  độ, trang mặc định, chú thích lật trái ở mép; `tests/UiItemPreview.tscn` chụp ảnh. Go vet/test ok; e2e client thật
  log `item windows ready`.
- Chưa: kho đồ/giao dịch (bố cục có, chưa dựng), kéo tiền, các nút Lời rao/Định giá/Rao bán/Rã/Khoá (không có hệ),
  tên môn phái trong `#m` (M12), trang Đánh giá/Kinh mạch.

### 2026-09-18 (sáng) — M11 lát C1: giao thức vật phẩm client ↔ zone; luật uống thuốc đối chiếu nhị phân Linux

Chủ dự án yêu cầu giữa chừng: *mọi thứ đối chiếu bản nhị phân Linux cho đồng bộ*. Nên trước khi viết,
từng luật của lát này được tìm trong `jx_linux_y` (mổ bằng `tools/re/re_elf.py` + hai script quét mới,
xem `docs/LINUX-SERVER.md` §9) và đối chiếu với nguồn Windows:

- **`KNpcAttribModify::LifePotionV`** = `0x08097E70`: `time = max(t1, t2)`, `value = (x1·t1 + x2·t2) / time`
  — giống nguồn Windows từng dòng. Bảng `ProcessFunc` của `KNpcAttribModify` (ctor `0x08099600`, đối tượng
  toàn cục `0x08BAC120`, phần tử 8 byte từ +4): id 153 = LifePotionV, 154 = ManaPotionV (`0x08097DE0`),
  190 cộng vào phần trăm hồi máu.
- **Khối thuốc trong `KNpc::ProcessState`** = `0x0808B7BC`: mỗi frame `time--`; khi `time % 10 == 0`
  cộng `value * percent / 100` (percent ở `KNpc+0x1194`, mặc định 100, log `"AddLifeState: %d * %d%% = %d"`),
  chặn trên ở max(`+0x1a14`, `+0x1a18`). Hồi máu tự nhiên (`0x0808B65F`) cũng nhân percent đó. Chỉ chạy khi
  `m_ProcessState` (DoDeath xoá, Revive bật); `DoRevive` → `ClearNormalState` xoá trạng thái thuốc.
  **Khác nguồn Windows**: có hệ số percent và hai giá trị max — port thêm `KNpc::life_replenish_percent`.
- **`KItemList::EatMecidine`** = `0x08204710`: người chết (`m_Doing == do_death`) không uống; cờ
  `KNpc+0x147a` (Lua `forbit_takemedicine`) → từ chối; đếm thuốc `KPlayer+0x86a4/+0x86a8`
  (`StartPotionCounter`…); gọi script `\script\item\forbiditem.lua` `Check_ItemUsable(map, genre,
  particular, level)` (chưa port — M11 E); `ApplyMagicAttribToNPC(npc, 3)` (`0x08068560`); móc
  `events.lua OnUseItem` (M11 E); **stack**: nếu dòng bảng *是否叠放* (`Item+0x14`) và `nStack > 1` thì
  `SetItemStack(nStack-1)` (`0x08200D30`, gói s2c 168), bằng 1 thì `Remove`; không xếp chồng → `Remove`;
  đang ngồi → đứng dậy. Nguồn Windows chỉ `Remove` — bản Linux mới có stack; dữ liệu phát hành:
  `potion.txt` mọi phiên bản đều *是否叠放 = 0* nên thuốc dùng một lần vẫn mất.
- **`KBPT_Medicine` đọc dòng** = `0x081ED430`: mảng mô tả 19 cột (kiểu 0 số, 1 chuỗi, 2 bỏ qua) →
  server Linux đọc đúng cột 1–13 và **hai** thuộc tính (14–19), bỏ ảnh/giới thiệu; cột 13 = 是否叠放.
  Bộ xuất Go giờ ghi `stackable` cho thuốc và kịch bản; zone chỉ lấy hai thuộc tính đầu.
- **`KItemList::ExchangeItem`** = `0x08206110` (6,8 KB): vẫn mô hình "tay cầm" như Windows (nhấc ô →
  đặt → cái bị đè lên tay); các phòng, `CheckSameDetailType` cho ô nhanh, chỉ giao dịch khi đang giao dịch.
  Giao thức mới gộp hai bước thành một: `C2G_ITEM_MOVE(id, room, x, y)` — ô trống thì đặt; đúng một vật
  phẩm nằm dưới thì nó về chỗ cũ của vật phẩm được kéo (chính là bước "đặt lại từ tay" của client cũ).

Giao thức (`proto/jx/client.proto`, `msg.proto`; `godobuf` không cho tên `ItemList` vì trùng lớp Godot →
`InventorySync`): `C2G_ITEM_MOVE/EQUIP/UNEQUIP/USE/DROP` (1104–1108), `G2C_ITEM_LIST/ADD/REMOVE/MOVE/RESULT`,
`G2C_MONEY` (2109–2114). `ItemView` mang đủ để client vẽ (tên, ảnh `\spr\item\…`, giới thiệu, kích cỡ ô,
độ bền/max, giá, thuộc tính cơ bản/yêu cầu/ma pháp).

- Zone: `KSubWorld::send_item_list` sau spawn (sau gói spawn của chính mình), `item_move_request`
  (`KItemList::exchange` mới + luật ô nhanh), `item_equip_request` (part −1 = `GetEquipPlace`; cái đang mặc
  về túi → hai `G2C_ITEM_MOVE`), `item_unequip_request`, `item_use_request` (theo bản Linux ở trên),
  `item_drop_request` (từ chối tới lát D), `give_item`/`take_item` cho script và rơi đồ sau này; `KNpc`
  thêm `life_state`, `life_replenish_percent`, `forbid_medicine`, `potion_counter/potion_count`;
  `process_potions` mỗi frame; `do_revive` xoá thuốc. `KMapInstance`/`KGameServer`/gateway relay nối 5 lệnh.
- Client: `KProtocolProcess` giữ `items{}` (id → dict), `money/bank_money`, `item_move/equip/unequip/use/drop`,
  `item_worn`, `item_at`, tín hiệu `items_changed/item_changed/item_removed/item_result/money_changed`.
  Cửa sổ túi (`UiItem*.ini` của 2.0) là lát C2.
- Test: 4 test case mới (danh sách khi vào và sau khi vào lại; chuyển ô/đổi chỗ/từ chối kèm `seq`; mặc/cởi;
  uống thuốc: gộp LifePotionV, hồi mỗi 10 frame, percent, stack 3 → 2 → 1 → mất, từ chối khi cấm/chết/kiếm)
  + harness `KGameServer` thấy `G2C_ITEM_LIST`. Zone Release 62/62 (25 153 assertion).
- Chưa làm trong lát này: ma pháp `manapotion_v` (chưa có nội lực — M12), `Check_ItemUsable`/`OnUseItem`
  (Lua — M11 E), ngồi/đứng, kho đồ chỉ mở tại NPC ngân hàng (chưa có NPC ngân hàng: hiện chuyển được tự do),
  giao dịch (từ chối). Mọi chỗ này có ghi chú trong mã.

### 2026-09-18 (rạng sáng) — M11 lát B: vật phẩm trong zone — mẫu, túi, ô trang bị, sinh vật phẩm, lưu vào nhân vật

Đọc `KItem.h/.cpp`, `KInventory.cpp`, `KItemList.h/.cpp`, `KItemGenerator.cpp`, `GameDataDef.h`,
`KMagicAttrib.h` của Core cũ và port giữ nguyên số: túi 6×10, kho 6×10, giao dịch 10×4, 3 ô nhanh, 15 ô
trang bị (`ITEM_PART`); lưới `KInventory` đặt/nhấc/tìm chỗ theo cột như cũ; `GetEquipPlace`/`Fit`
(nhẫn vào ring1 hoặc ring2); `CanEquip` theo `EnoughAttrib` (sức/thân/ngoại/nội/cấp ≥, hệ/giới/môn phái =);
`SetAttrib_CBR` quay thuộc tính cơ bản trong [min,max] một lần khi sinh, độ bền từ `magic_durability_v`;
tra mẫu đúng như `KItemGenerator` (`particular*10+level-1`, mặt nạ theo `particular`, thuốc
`detail*5+level-1`, nhiệm vụ theo `detail`, hoàng kim theo `row_id`, kịch bản theo `(detail, particular)`);
hoàng kim quay 6 ma pháp theo `nLuck` đúng công thức `Gen_GoldEquip` (0..200, ≥200 luôn tối đa).

- `server/zone/…/KItem.h/.cpp`: `KItemTemplateSet` + `KItemLibrary` (mọi phiên bản `items/v000..json`,
  mặc định phiên bản mới nhất), `KItem`, `KInventory`, `KItemList` (add/stack/move/swap/equip/wear/unequip,
  tiền túi + kho), `KItemGenerator` (trắng + hoàng kim; ma pháp tiền/hậu tố `Gen_MagicAttrib` chưa port).
- `RoleData.items` = `ItemData` (giữ cả phiên bản, tham số sinh và mọi thuộc tính đã quay → không quay lại),
  `next_item_id`, `money`, `bank_money`; `KSubWorld::items_of(sid)`, nạp khi spawn (vật phẩm mất bảng bị loại
  có cảnh báo), lưu trong mọi snapshot. `zone.items_dir` (mặc định `client/assets/items`), log
  `item tables loaded versions=0,1,2,3,4 default=4 items=14187`.
- Test: 7 test case/2 696 assertion (tra mẫu, quay thuộc tính, hoàng kim theo may mắn, lưới, danh sách, bảng
  thật v000, vòng spawn → snapshot → spawn giữ nguyên id/chỗ/giá trị). 134/134 Debug + Release, Go, Godot 262,
  e2e đạt.
- `dev.py` in UTF‑8 (trước đây tên tiếng Việt trong output client làm `print` chết trên console cp1252).

### 2026-09-17 (đêm) — M11 lát A: bảng vật phẩm của server cũ đọc đúng từng cột, xuất JSON

Đọc `Core/Src/KBasPropTbl.cpp` (`KLibOfBPT::Init`, từng `LoadRecord` đọc **theo số cột**) và đối chiếu
tiêu đề bảng thật của `D:\ServerLinux\server1\settings\item` (46 cột trang bị, 28 thuốc, 9 nhiệm vụ
với cột 9 = `ParticularType` khác bản JX1, 23 cột ma pháp — thêm cột "ngựa", 53 cột hoàng kim + `所在套装`).
Server JX2 giữ **6 bộ bảng**: `settings/item` (gốc, không có hoàng kim) và `000..004` (mỗi bộ đầy đủ,
kể cả `goldequip`, `magicattrib_ge`, `suite_activate_count`, `magicscript`).

- `pkg/jxold/item/KBasPropTbl.go`: `item.Load(dir, version)` → `item.Set` (12 bảng trang bị theo
  `EQUIPDETAILTYPE`, thuốc, nhiệm vụ, thổ địa phù, ma pháp tiền/hậu tố với `drop_rates` theo bề rộng tệp,
  hoàng kim + ma pháp hoàng kim + số món kích hoạt bộ, vật phẩm kịch bản); tên/mô tả giải mã sang UTF-8.
  Test tổng hợp theo đúng cột cũ + test trên dữ liệu thật (6 bộ).
- `text.DecodeMixed`: port `decline2` của `ReverseTools/port_3hd/dec2.py` (cắt tại `"`/`-`, đoán từng
  đoạn, dấu câu CP1252 trong TCVN3) — trước đó 330 dòng quest + 12 hoàng kim của v004 có byte hỏng, giờ 0.
- `jxassets export-items` (trong `dev.py assets`) → `client/assets/items/{base,v000..v004}.json`
  (không commit): 19 735 dòng; v004: 5 938 hoàng kim, 4 995 vật phẩm kịch bản, 903 mặt nạ.
- Kế tiếp: lát B — `KItemTemplate` + `KItemList` (túi, ô trang bị) trong zone, lưu vào `RoleData`.

### 2026-09-17 (đêm) — M9 phần 1: kho PostgreSQL cho tài khoản + nhân vật, CI chạy thật

- `pkg/persist/pgstore.go`: `PgStore` thực hiện cùng `Store` với kho tệp — pgx v5, SQL thuần (ADR-007),
  bảng `accounts` (name_key unique không phân biệt hoa thường), `characters` (jsonb = protojson của
  `RoleData`, `data_version`, `updated_at`), `schema_version`; id từ sequence; tạo nhân vật là một giao dịch;
  `SaveCharacter` là một `UPDATE` (crash giữa hai lần lưu không mất gì đã xác nhận); bản ghi cũ được
  `MigrateRole` khi đọc, bản ghi server mới hơn bị từ chối như kho tệp.
- `store_test.go`: **một kịch bản tuân thủ** chạy cho cả tệp lẫn PostgreSQL (mở lại như gateway khởi động
  lại); PostgreSQL chạy khi có `JX_TEST_PG`, CI có job `Go + PostgreSQL` (service container postgres:17)
  và job e2e chạy gateway với `JX_GATEWAY__DB` — kiểm cả dòng log `postgres store opened`.
- `gateway.db` / `JX_GATEWAY__DB`, `jxaccount -db`; `Config.Flatten` che mật khẩu **trong giá trị**
  (`postgres://jx:***@…`, `password=***`), có test.
- **Lưu theo từng nhân vật, không dồn cục** (yêu cầu của chủ dự án cho server 20 000 người): trước đây zone
  gửi `KCmdSaveRequest` cho **mọi** phiên trong cùng một tick mỗi 60 s, và gateway ghi **đồng bộ ngay trên
  luồng đọc đường zone** — với PostgreSQL 20 000 người là 20 000 lần ghi dồn vào một tick và đường zone đứng
  hàng chục giây. Giờ: (1) zone rải lịch, mỗi nhân vật có khe riêng `(tick + sid) % chu_kỳ` → 20 000 người ở
  60 s là ~19 lần lưu mỗi tick, đều đặn (test: 10 người/20 tick, không tick nào có 2 lần lưu; `PlayerSave.tick`
  ghi tick chụp); (2) gateway có `KSaveQueue`: `save_workers` (4) worker ghi riêng, lần lưu mới của cùng nhân
  vật **thay** lần cũ còn chờ (không ghi thừa), lần lưu **cuối** (thoát) không bao giờ bị thay mất, thử lại 3
  lần rồi mới báo `MẤT lần lưu cuối`, xả hết khi tắt (`saves flushed`), hàng đợi > 1000 thì cảnh báo `save
  backlog`; thống kê `saves_s / save_queue / save_peak / save_ms / save_errors / save_lost` trong `gw.stats`.
  Có test cho cả hai đầu.
- Chưa làm: đo 20 000 nhân vật (máy này không có PostgreSQL/Docker — cài đặt phần mềm lên máy chủ dự án
  không tự ý làm), sao lưu/khôi phục (O4).

### 2026-09-17 (đêm) — ADR-007: tuổi thọ công nghệ 5–10 năm; RUNNING.md cập nhật

Chủ dự án yêu cầu "mã nguồn dùng được 5–10 năm sau không cần nâng cấp nữa". Viết thành quyết định có
kiểm chứng: [ADR-007](adr/ADR-007-tuoi-tho-cong-nghe.md) — từng tầng (C++20, CMake+vcpkg ghim, Go 1.26,
protobuf 3, Lua 5.4, Godot 4.7, JSON/PostgreSQL, UTF-8, Windows+Linux) đang ghim gì và vì sao sống được,
những gì cố ý không chọn, chính sách nâng cấp mỗi năm một commit riêng, và bảng việc T1–T12 (8 xong, còn
PostgreSQL M9, validator D1, lịch nâng cấp, Docker O4). RUNNING.md thêm `JX_CONFIG`, `JX_PORT_OFFSET`,
`export-ui`, tham số `--shot`/`--auto` của client, thế giới phẳng khi không có dữ liệu, và ba sự cố mới.

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
