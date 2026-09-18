# Mổ nhị phân client 2.0 (`gamecl.exe`) — giao diện lấy đúng chuẩn bản 2.0

Nguyên tắc của chủ dự án: **hình ảnh / bố cục / chữ hiện ra lấy đúng client 2.0, luật chơi lấy từ
nhị phân server Linux** (`LINUX-SERVER.md`). Bố cục cửa sổ đã có từ `.ini` (U1–U5); tài liệu này ghi
phần **mã** của client 2.0: cách mở nhị phân, và những hàm đã đọc từng dòng để dựng lại.

## 1. Mở nhị phân

`C:\Users\nguye\Level Up Games\Vo Lam Truyen Ky 2.0\gamecl.exe` (1,3 MB) là toàn bộ core + UI của client,
nén **UPX 3.03** (NRV2E, bộ lọc gọi hàm 0x24, `cto` 0xae) — 34,6 MB khi mở. `enginefree.dll`,
`represent3free.dll` không nén (có bảng export, đọc bằng `pefile`).

```bash
python tools/re/re_upx.py build/re/gamecl.exe       # -> build/re/gamecl.exe.unpacked.img + .json
python tools/re/re_calls.py build/re/gamecl.exe.unpacked.img build
python tools/re/re_elf.py  build/re/gamecl.exe.unpacked.img strings "G_ITEM" 20
```

`re_upx.py` đọc tham số ngay trong stub nạp của UPX (nguồn dữ liệu nén, đích 0x401000, độ dài
vùng bỏ lọc 0x349142, `cto`, bảng import thu gọn, điểm vào gốc 0x5b1465), giải nén NRV2E, bỏ lọc
E8, dựng lại bảng import (`engineFree.dll!?g_StrWrap@@...`), ghi ảnh bộ nhớ thô + `.json` để mọi
công cụ `re_*` đọc như một ELF (`Elf.__init__` nhận `.img`; `function_start` hiểu cả prologue MSVC
`55 8B EC`). `build/re/` không commit (nhị phân thuộc chủ dự án).

Chuỗi của client nằm trong **bảng chuỗi** `\lang\vn\stringtable_core.txt` (1211 khoá `G_*`) và
`stringtable_client.txt` (752) — TCVN3, tra bằng `KStringTable::Get(key)` (`0x4105e0` trên
`0x9c0670`); mẫu `push 1; push "KEY"; mov [slot], eax(kết quả lần trước); call Get` → scratch
`strkeys.py` dựng bản đồ slot → khoá (`gamecl.exe.unpacked.img.strkeys.json`, 890 slot).
`jxassets export-ui` xuất cả hai bảng: `ui/du-lieu/chuoi-core.json`, `chuoi-client.json`.

## 2. Chú thích vật phẩm — `KItem::GetDesc` `0x00636460`

Đọc từng dòng (tham số: `buf, nUiType, nPriceScale, nActive, ?, ?, bCopyImage`); dựng lại ở
`client/ui/KUiItemView.gd::describe_text`, kiểm ở `tests/UiCheck.gd`.

| Bước | Nhị phân | Kết quả |
|---|---|---|
| Màu tên | `0x63657e`: genre 0 → quality 1/4/5 `<color=Yellow>`, 2 `<color=Violet>`, 3 `G_ITEM_0` ("vật phẩm tạm thời"), `+0x2f8` (ma pháp ô 0) ≠ 0 → `<color=Blue>`, còn lại White; genre ≠ 0: genre 7 hoặc độ bền (+0x4e4) = 0 → `<color=Red>`; bảng theo genre `0x81b398` (1 White, 4 Yellow, khác rỗng) | |
| `G_ITEM_28` `<trang bị tổn hại>` khi genre 7 / độ bền 0; tên (`"%d★ %s"` khi có sao +0x468); genre 0 hoặc genre 6 có cấp → `G_ITEM_22` ` [Cấp %d]` (quality 4: `"  %s+%d%s"`); `"\n"` (`0x78acdc`) | | dòng 1 |
| Khoá hồn / dấu hệ / hết hạn khoá / trạng thái khoá (`+0x69e`, `+0x4a8`, `+0x69c`, `+0x520`: `G_ITEM_LOCK_STATE`, `G_ITEM_BIND_STATE…`) | | chưa có trạng thái này |
| Giá: gọi ảo `[vtable+4]` chỉ khi `nUiType ≠ 0 && nPriceScale > 0` | | túi đồ: không |
| genre 0 và detail ≠ 11 (mặt nạ): `G_ITEM_3..7` theo hệ 0..4 (`"<color=White>Thuộc tính Ngũ hành: <color=Metal>Kim "`), rồi `"\n"` (hệ ngoài 0..4 chỉ `"\n"`); genre khác → **không có dòng này** | `0x636c10` | dòng 2 |
| `"<color=White>"` + **`g_StrWrap(buf, intro, 0x28)`** (engineFree `0x10035fc0`): mô tả cắt ở `<enter>`, mảnh ≤ 40 ký tự giữ nguyên, dài hơn chia đều `n/40+1` dòng (ngắt giữa từ), mỗi dòng `"\n"`; byte > 0x80 đếm 2 và đi 2 byte (TCVN3: chữ có dấu + chữ sau đi cùng) | `0x636d2c` | `KTextEncode.str_wrap` |
| genre 6 → mô tả riêng (`0x630cd0`) rồi **kết thúc**; khác → 7 thuộc tính cơ bản (`0x630ae0`, `+0x228`): độ bền (31) chỉ genre 0 không mặt nạ — −1 → `G_ITEM_8` vàng "Không thể phá hủy", còn lại `G_ITEM_9_1` `"Độ bền: %3d / %3d"` (≤ 0 → `G_ITEM_9_2` đỏ); thuộc tính khác qua `KMagicDesc` (`0x60a2b0`), độ bền 0 → `"<color=red>%s<color>"`; mỗi dòng `"\n"` | | |
| 6 yêu cầu (`0x62ea70`, `+0x298`): `EnoughAttrib` → `<color=White>` / `<color=Red>` + câu + `"\n"` | | |
| Ma pháp (`0x635430`, `+0x2f8`, trừ `nUiType` 19..28): ô chẵn tiền tố, lẻ hậu tố; hậu tố sáng khi `(i>>1) < nActive` (mặt nạ: 3; túi: 0); khoảng `[min-max]` = `GetMagicRange 0x63c260` (min nhỏ nhất / max lớn nhất trong các dòng cùng kind của `GetCMIT(pos, loại, hệ, cấp)`); màu: độ bền 0 Red; quality 1/4 Yellow/DYellow; 2 Violet/DViolet; 5 cam `0xff8c27`/`0xaa7f14` khi "hoàn hảo" (`0x62f020`) không thì Yellow/DYellow; còn lại **HBlue + `<color=0xc0c0c0>[a-b]`** / **DBlue + `<color=DBlue>[a-b]`**; ô trống của quality 2 với `MALevel` −1 → `<color=Yellow>` `G_ITEM_27` "Chưa khảm" | | |
| Ma pháp phụ / điểm chúc phúc / đá / văn sức / bộ hoàng kim (`0x630eb0`, `0x6310a0`, `0x631420`, `0x631690`, `0x633cc0`) | | không có với đồ thường |
| Bọc ngoài `0x6b66a0` (vtable KItem+0): gọi GetDesc rồi thêm **`"\n"`** (dòng trống cuối), hạn dùng, khoá cửa hàng | | |

Bảng màu tên của engine (`enginefree.dll`, `name[8] + r,g,b`): White 255,255,255 · Red 255,0,0 ·
Green 0,255,0 · DGreen 0,127,0 · Blue **100,100,255** · Yellow 255,255,0 · DYellow 127,127,0 · Gold
243,194,90 · Orange 255,199,0 · Pink 255,0,255 · Cyan 0,255,255 · Metal 246,255,117 · Wood 0,255,120 ·
Water 78,124,255 · Fire 255,90,0 · Earth 254,207,179 · DBlue 120,120,120 · HBlue 100,100,255 · Violet
188,64,255 · DViolet 111,40,156. `<color=0xRRGGBB>` = `strtoul`; `<color>` = về màu gốc; tên lạ giữ
nguyên chữ (`KTextEncode.gd`).

## 3. Cửa sổ chú thích — `KMouseOver` (`Ui\Elem\MouseHover.cpp`, bố cục `弹出说明文字.ini`)

Với vật phẩm: `SetMouseHoverInfo(.., bHeadTailImg=false, bFollowCursor=false)`, toàn bộ chữ là
khối "title": `TGetEncodedTextLineCountAE(text, 64)` — mỗi `"\n"` kết thúc một dòng (dòng rỗng vẫn
tính), phần sau `"\n"` cuối chỉ tính khi có chữ, dòng dài ngắt đúng 64 ký tự (không theo từ; chế độ
TCVN mỗi byte 1 ký tự); rộng = `Font × max(maxLen, 26) / 2 + 2 × Indent`, cao = `(Font+1) × số dòng`,
nền `TitleBgColor` alpha 0xB0; **mỗi dòng căn giữa** `x = left + w/2 − len × Font/4`; vị trí
`ALW_GetWndPosition`: giữa theo x con trỏ (kẹp màn hình), dưới con trỏ 32 px ở nửa trên màn hình,
trên con trỏ ở nửa dưới. Dựng lại ở `client/ui/uicase/UiMouseHover.gd`.

## 4. Còn phải mổ tiếp (client)

- ~~Tên / mô tả vật phẩm lấy từ bảng của client~~ — xong: `items/client_vNNN.json` (`export-items`), `KLibOfBPT.gd`.
  Còn: tên vật thể dưới đất (`KObj.gd`) vẫn là tên server gửi trong `EntityInfo`.
- ~~`nActive` khi mặc~~ — xong: `KUiItemView.equip_enhance` (bảng và luật của `jx_linux_y 0x081FD2C0`).
- Dòng giá trong cửa hàng (`[vtable+4]`), khoá / hạn dùng, bộ hoàng kim.

## 6. Sổ kỹ năng 2.0 — `KUiSkills` / `KUiFightSkill` (M12 lát B4a, đã đọc từng dòng `gamecl.exe`)

Tệp bố cục (tên đúng exe gọi, `client/assets/ui/…` sau `jxassets export-ui`): `技能主窗口.ini` (`ky-nang`: `Main` 519×280
`技能界面底版.spr`, `Title` "Kỹ Năng", `CloseBtn`, ba nút trang `FightBtn` "Chiêu Thức" / `LiveBtn` "Kỹ năng sống" / `CommonBtn`
"Thông dụng khác"), `战斗技能分页.ini` (`ky-nang-chien-dau`), `战斗技能细分页.ini`, `生活技能分页.ini`, `技能选择树.ini`
(`chon-ky-nang`, cây chọn kỹ năng cho chuột), `玩家信息主界面.ini` (`thanh-nhan-vat`: thanh chính có `ImediaLeftSkill` (791,724)
/ `ImediaRightSkill` (828,724) 36 px, `Item_0..8` ô thuốc nhanh y = 728), `顶部控制条.ini`, `工具控制条.ini`, `技能状态列表.ini`.

| Địa chỉ (`gamecl.exe.unpacked.img`, gốc 0x401000) | Làm gì | Zone / client mới |
|---|---|---|
| `0x00494030` | **`KUiFightSkill::LoadScheme`**: `Main`; pad `0x004933A0(this+0x14d88, scheme, 0)`; `RemainPoint`, `RemainPointTitle`; **`ImgSkillBG`** 3 hàng × 10 cột: `SetPosition(x0 + 0x33·cột, y0 + 0x3a·hàng)` (cột 51 px, hàng 58 px); **`ImgSplitLine`** 9 vạch dọc x0 + 51·i; **`ImgColTitle`** + **`TxtColTitle`** 10 cột (x0 + 51·i), chữ `[ColTitle] Title_%d` (1..10: Thấp, Cao, Nhập môn, Lv10, Lv20, Lv30, Lv40, Lv50, Trấn Phái, Lv60; `Tip_%d` chú thích; Title_11/12 = Lv120/Lv150 không được vẽ) | `UiSkills.gd` `_build_fight_page` |
| `0x004933A0` | **pad ô kỹ năng** `(scheme, mode)`: vòng y (ebx, 0..0x1fe bước 0x33 — 10 cột) trong vòng x (0..0xae bước 0x3a, hay 0x3d ở chế độ 1 — 3 hàng): mỗi ô `KWndObjectBox` `Init(ini, mode ? "CommonSkill" : "Skill")`, `0x45e040(0)`, vị trí mẫu + (cột·51, hàng·58); nút `AddPointBtn`/`AddCommonPointBtn` cạnh phải ô, đáy ngang đáy ô (`x + w ô`, `y + h ô − h nút`); `[SkillText] Font` (12), `Offset` (0,36), `Color`, `ColorAddon` (mặc định "50,50,255"), `ColorCutdown` ("255,50,50") | như trên; chữ cấp dưới ô căn giữa |
| `0x004938E0` / `0x00493A40` | **`UpdateData`**: `GetGameData(0x3f4 GDI_FIGHT_SKILLS, 50 ô 16 byte {genre, id, cấp học, cấp hiện tại})`; xoá 30 ô; với mỗi kỹ năng: `GetGameData(0x414, &vị trí, id·10 + slot)` cho slot 0..2 → ô `[slot·10 + bậc]` giữ {genre, id, cấp, cấp hiện tại}; màu chữ: hiện tại > học → ColorAddon, < → ColorCutdown; nút cộng điểm hiện khi `+0x590` (điểm còn) > 0 | `UiSkills.refresh` |
| `0x00493E40` | `WndProc`: `0x512` (click ô) → `OperationRequest(0x20004 GOI_SET_IMMDIA_SKILL?, 0x10, id)`; `0x565` (nút cộng điểm) → `+0x590 −= 1`, `OperationRequest(0x20004, 0xf, id)` = GOI_TONE_UP_SKILL, script `first_add_skill_level` | click trái = kỹ năng chuột trái, phải = chuột phải; nút → `C2G_ADD_SKILL_POINT` |
| `0x00661EBC` (case 0x414 của `0x00661470`, bộ chia `0x005B8400`) | `skill = tham số / 10`, `slot = tham số % 10`; `0x00607B40(bảng 0x1AB35A0, skill, slot, &out)` → map `+0x462124`: skill → slot → {bậc, …}; không có → out = {−1, −1} | `skill_ui.json` `place[skill] = {tier, slot}` |
| `0x00607860` | **nạp `\settings\skillui\skillui.txt`** (`KTabFile`): mỗi dòng cột 1 môn phái (< 11), cột 2 nhánh (< 3), cột 3 tên (16 byte), cột 4..39 = 12 bậc × 3 ô id kỹ năng (`低级绝学/高级绝学/入门/10级/20级/30级/40级/50级/镇派/60级/120级/150级` × 1..3); id ≠ 0 → map | Go `jxold/skill/KSkillUi.go` (`ParseSkillUi`, `PlaceOf`), `jxassets export-skill-ui` (23 dòng, 170 kỹ năng có chỗ) |
| `0x00472360` | `KUiPlayerBar::LoadScheme` 2.0 (`ImediaLeftSkill`, `ImediaRightSkill`) — chưa port | B4b |

Client mới: `client/ui/uicase/UiSkills.gd` (phím **K**), biểu tượng từ cột `SkillIcon` của `skills.txt` qua `jxassets export-skill-images`
(362 ảnh vào `items/images.json` dùng chung với vật phẩm), `KProtocolProcess.skills` (`G2C_SKILL_LIST/LEVEL/FORBID`),
`cast_skill(id, mục tiêu, x, y)` (`C2G_CAST_SKILL`), `add_skill_point`, `left_skill`/`right_skill` (= `m_nLeftSkillID`/`m_nRightSkillID`
của `KPlayer` client cũ, `GOI_SET_IMMDIA_SKILL`): click trái quái thi triển kỹ năng trái (không có → đánh thường), click phải
thi triển kỹ năng phải vào quái hay vào chỗ. Chưa: thanh nhân vật 2.0 (`玩家信息主界面.ini`) với hai ô kỹ năng chuột, cây chọn
kỹ năng (`技能选择树.ini`), phím F1..F11 (`ShortcutSkill(%d)`), trang sống / thông dụng, chú thích kỹ năng (`KUiSkillTree`), gói 0x87.
