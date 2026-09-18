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

## 6. Sổ kỹ năng 2.0 — `KUiSkills` ba trang nhánh (Quyền / Bổng / Đao) + trang thông dụng (M12 lát B4a + B4b-1, đã đọc từng dòng `gamecl.exe`)

Tệp bố cục (tên đúng exe gọi, `client/assets/ui/…` sau `jxassets export-ui`): `技能主窗口.ini` (`ky-nang`: `Main` 519×280
`技能界面底版.spr`, `Title` "Kỹ Năng", `CloseBtn`, `FightBtn` "Chiêu Thức" 101×21 tại (8,29), `CommonBtn` "Thông dụng khác" tại
(317,29); `LiveBtn` có trong tệp nhưng **exe không nạp**), `战斗技能分页.ini` (`ky-nang-chien-dau`: `Main` (4,49) 519×280,
`RemainPoint` (460,205) 40×13 canh giữa, `RemainPointTitle` (270,205) canh phải "Điểm kỹ năng còn:", `ImgSplitLine` (51,1),
`ImgColTitle` (2,3) 48×20, `TxtColTitle` (−1,3) 55×20, `ImgSkillBG` (6,28), `ImgCommonSkillBG` (6,12), `Skill` (8,30) 36×36,
`CommonSkill` (8,14), `AddPointBtn` / `AddCommonPointBtn` 14×14, `[SkillText]` Offset 0,36 Font 12 Color 218,255,165,
`[ColTitle] Title_1..12`), `战斗技能细分页.ini`, `生活技能分页.ini`, `技能选择树.ini` (`chon-ky-nang`), `玩家信息主界面.ini`
(`thanh-nhan-vat`: `ImediaLeftSkill` (791,724) / `ImediaRightSkill` (828,724) 36 px, `Item_0..8` y = 728), `顶部控制条.ini`,
`工具控制条.ini`, `技能状态列表.ini`. Thư mục theme: `\Ui\ui3_1024\` (`\Ui\Setting.ini [Theme]`).

**Cửa sổ nào là "sổ kỹ năng"**: Lua `Open("skills")` (`0x0042EF50`, bảng tên `0x80dae8`: 0 team, 1 map, 2 status, 3 Items,
4 skills, …) → `0x004955B0` mở `KUiSkills` (đối tượng đơn `0x83f438`, 0xae3e0 byte, ctor `0x004953C0`). Còn `Open("status")`
→ `0x00478330` là cửa sổ nhân vật; trong đó có trang "門派多修" (`mutiple_faction\门派多修_技能主窗口.ini` "Phương án kỹ
năng", 2 trang con 5×5 ô `门派多修_战斗技能细分页.ini`, 3 khung `门派多修_单个门派框.ini`) nuôi bằng gói riêng
(`MutipleFaction*` Lua `0x005CBE30..`, GDI `0x3f5` → `0x00641E90`) — **`jx_linux_y` không có hệ này** (không có chuỗi nào),
nên không port.

| Địa chỉ (`gamecl.exe.unpacked.img`, gốc 0x401000) | Làm gì | Zone / client mới |
|---|---|---|
| `0x004953C0` | **`KUiSkills` ctor**: `__ehvec_ctor(this+0x590, 0x2c600, 3, 0x00494FC0)` = **ba `KUiFightSkill`** (một trang mỗi nhánh), `__ehvec_ctor(this+0x85790, 0x9a0, 3)` = **ba nút nhánh**, `+0x87470` trang thông dụng (`0x004951E0`), `+0xacd38` `CommonBtn`, `+0xad6d8` `CloseBtn`, `+0xade48` `Title` | `UiSkills.gd`: `_pages[0..2]` + `_pages[3]` (thông dụng), `_branch_buttons`, `_common_button` |
| `0x00494EF0` | **`KUiSkills::Init`**: mỗi i 0..2: `pages[i].Init` (`0x00493F70`), `AddPage(pages[i], buttons[i])` (`0x00463590`), **`pages[i].SetBranch(i)`** (`0x00494470` → pad `+0x17874`); trang thông dụng + `CommonBtn`; `LoadScheme` | trang i = nhánh i |
| `0x00494940` | **`KUiSkills::LoadScheme`**: `Main`, `Title`, `CloseBtn`; mỗi i: `pages[i].LoadScheme` (`0x00494030`), `buttons[i].Init(ini, "FightBtn")` rồi **`SetPosition(x + 0x67·i, y)`** (103 px); trang thông dụng `0x00494540`, `CommonBtn` | `Layout.branch_button_x(8, i)` = 8, 111, 214 (317 = ô thứ tư, đúng `CommonBtn` trong tệp) |
| `0x004947C0` | **`KUiSkills::UpdateData`**: mỗi i: `GetGameData(0x413, tên[16], i)` → có tên → nút i `Show` + `SetLabel(tên)`; i == 0 → trang mặc định = trang 0; không tên → nút i `Hide`, **`CommonBtn` dời vào chỗ nút trống đầu tiên**, chưa có mặc định → trang thông dụng (i == 0 → `0x004633A0(3, 0)`); cuối cùng `trang mặc định->vtable[1]()` (kích hoạt) | `UiSkills.refresh`: `branch_titles()` từ `skill_ui.json` `titles[faction_last]` |
| `0x00494D10` | `KUiSkills::WndProc` `0x565`: `CloseBtn` → ẩn; `buttons[0..2]` → `pages[0..2]->vtable[1]()`; `CommonBtn` → trang thông dụng | `_on_page_button` |
| `0x00494B40` | kỹ năng đổi cấp (`0x20004`): mỗi i: `GetGameData(0x414, id·10 + i)` có bậc → `pages[i].UpdateOne`; i == 2 → cả trang thông dụng; cấp ≤ 9 (`GDI 0x3eb`) → gợi ý `InfoString` | `refresh()` toàn bộ |
| `0x00494FC0` | **`KUiFightSkill` ctor** (0x2c600 byte): `+0x554` 10 `ImgColTitle`, `+0x3a9c` 10 `TxtColTitle`, `+0x728c`/`+0x7824` RemainPoint(+Title), `+0x7dbc` 30 `ImgSkillBG`, `+0x11d94` 9 `ImgSplitLine`, `+0x14d88` **pad** (`0x00494DA0`: 30 ô `KWndObjectBox` `+0x594` bước 0x4e8, 30 nút `+0x98c4` bước 0x770, màu `+0x177f0..`, **nhánh `+0x17874`**) | |
| `0x00494030` | **`KUiFightSkill::LoadScheme`**: `Main`; pad `0x004933A0(this+0x14d88, scheme, 0)`; `RemainPoint`, `RemainPointTitle`; `ImgSkillBG` 3 hàng × 10 cột `SetPosition(x0 + 0x33·cột, y0 + 0x3a·hàng)`; `ImgSplitLine` 9 vạch x0 + 51·i; `ImgColTitle` + `TxtColTitle` 10 cột chữ `[ColTitle] Title_%d` (1..10; Title_11/12 = Lv120/Lv150 không vẽ) | `_build_page(false)` |
| `0x00494540` | **`KUiCommonSkill::LoadScheme`**: `Main`; `ImgCommonSkillBG` 3 hàng × 10 cột, **hàng bước 0x3d (61 px)**; `ImgSplitLine` 9; `RemainPoint`(+Title); pad chế độ 1 (`0x004933A0(…, 1)`: ô `CommonSkill`, nút `AddCommonPointBtn`, hàng 0x3d); **không** có tiêu đề cột | `_build_page(true)` |
| `0x004933A0` | **pad ô kỹ năng** `(scheme, mode)`: vòng y (10 cột bước 0x33) trong vòng x (3 hàng bước 0x3a, hay 0x3d ở mode 1): mỗi ô `KWndObjectBox Init(ini, mode ? "CommonSkill" : "Skill")`, vị trí mẫu + (cột·51, hàng·58/61); nút `AddPointBtn`/`AddCommonPointBtn` cạnh phải ô, đáy ngang đáy ô; `[SkillText] Font` (12), `Offset` (0,36), `Color`, `ColorAddon` ("50,50,255"), `ColorCutdown` ("255,50,50") | như trên |
| `0x004938E0` | **`KUiFightSkillPad::UpdateData`** (trang chiến đấu): `GetGameData(0x3f4, 50 ô 16 byte {genre, id, cấp học, cấp hiện tại})`; xoá 30 ô + 30 nút; mỗi kỹ năng: **`GetGameData(0x414, &{ô, bậc}, id·10 + nhánh của trang (+0x17874))`** (`0x0049393D`) → bậc ≠ −1 → ô `[ô·10 + bậc]` (`0x00493982`: hàng = ô 1..3 của bậc, cột = bậc); màu chữ: hiện tại > học → ColorAddon, < → ColorCutdown; nút cộng điểm khi `[+0x590]` (điểm còn) > 0 và bậc ≥ 2 | `refresh()`: `place_of(id, nhánh)` → `Layout.box_index(slot, tier)` |
| `0x004936B0` | `UpdateOne(entry)` của pad: cùng phép tra với nhánh `+0x17874` cho một kỹ năng | |
| `0x00493A40` | **trang thông dụng `UpdateData`**: `GetGameData(0x3f4)`; xoá 30 ô; mỗi kỹ năng: **cả 3 nhánh** `GetGameData(0x414, id·10 + k)` đều −1 (không nhánh nào đặt) → ô trống đầu tiên quét **bậc ngoài (0..9), ô trong (0..2)** → ô `[ô·10 + bậc]`; `0x00493790` `UpdateOne` tương tự (tìm ô đang giữ id trước) | `Layout.common_fill_index(n)` = 0, 10, 20, 1, 11, … |
| `0x00493320` | `SetBranch` của pad (`+0x17874 = n`) — **không ai gọi** (bản pad rời); bản thật đặt nhánh qua `0x00494470` (`+0x2c5fc` của trang) từ `0x00494EF0` | |
| `0x00661EBC` (case `0x414` của `0x00661470`, bộ chia `0x005B8400`) | `skill = tham số / 10`, **`nhánh = tham số % 10`**; `0x00607B40(bảng 0x1AB35A0, skill, nhánh, &out)` → map `+0x462124`: skill → **nhánh** → `{ô, bậc}`; không có → `{−1, −1}` | `skill_ui.json` `place[skill][nhánh] = {slot, tier}` |
| `0x00661E6A` (case `0x413`) | **`GetBranchTitle`**: `0x00604A60(bảng, PlayerData+0x12080 (môn phái gia nhập sau cùng), nhánh)` → 16 byte tên tại `+0x462130 + (môn phái·3 + nhánh)·16` (môn phái ≤ 10, nhánh ≤ 2), không có → rỗng | `skill_ui.json` `titles[faction][nhánh]`; `Game.faction_last` |
| `0x00607860` | **nạp `\settings\skillui\skillui.txt`** (`KTabFile`): mỗi dòng cột 1 môn phái (≥ 11 → **dừng nạp**), cột 2 nhánh (≥ 3 → dừng), cột 3 tên (16 byte) vào ô tên; cột 4.. = **10 bậc × 3 ô** (`cmp ebx, 0xa`: `低级绝学 高级绝学 入门 10级 20级 30级 40级 50级 镇派 60级`; cột `120级/150级` của tiêu đề **không đọc**); id ≠ 0 → `map[id][nhánh] = {ô = ebp, bậc = ebx}` (`0x00607A30`); **cùng id lặp trong cùng nhánh → thoát nạp ngay** (`0x00607ABF`) | Go `jxold/skill/KSkillUi.go` (`ParseSkillUi`, `PlaceOf(id, nhánh)`, `TitleOf`, `Truncated`), `jxassets export-skill-ui` (23 dòng, 10 môn phái × 2–3 nhánh) |
| `0x00651280` (ô `0x1f0/4 = 0x7c` của bảng handler `0x0065AEE0`, tức **gói `0x7b`** vì bảng lệch một: ô 0x5a = gói 0x59 camp) | gói `{0x7b, phe, môn phái hiện tại, môn phái sau cùng, dword số lần}`: `SetCamp(npc, byte+1)`, `+0x12078 = int8 +2`, **`+0x12080 = int8 +3`** (sau cùng), `+0x12084 = dword +4`, báo UI; `0x006511B0` (gói `0x7c`): `+0x12078 = −1`, phe 4 | `G2C_PLAYER_FACTION` → `Game.faction / faction_last / faction_count / camp`, tín hiệu `faction_changed` |
| `0x006500C0` (ô 0x5a = gói `0x59`) | `{npc id dword +1, phe byte +5}` → `SetCamp` của npc | `G2C_ENTITY_CAMP` → `entities[id].camp / current_camp` |
| `0x00472360` | `KUiPlayerBar::LoadScheme` 2.0 (`ImediaLeftSkill`, `ImediaRightSkill`) — chưa port | B4b |

Client mới: `client/ui/uicase/UiSkills.gd` (phím **K**) + `KUiSkillsLayout.gd` (số đo, có test headless): ba trang nhánh với
nút từ `FightBtn` cách 103 px, nhãn = tên nhánh của môn phái gia nhập sau cùng (`skill_ui.json` `titles`), nút nhánh thiếu
ẩn và `CommonBtn` dời vào chỗ trống đầu; trang thông dụng cho kỹ năng không nhánh nào đặt (điền theo cột). Biểu tượng từ
cột `SkillIcon` của `skills.txt` qua `jxassets export-skill-images`, `KProtocolProcess.skills` (`G2C_SKILL_LIST/LEVEL/FORBID`),
`cast_skill(id, mục tiêu, x, y)` (`C2G_CAST_SKILL`), `add_skill_point`, `left_skill`/`right_skill`: click trái quái thi triển
kỹ năng trái (không có → đánh thường), click phải thi triển kỹ năng phải vào quái hay vào chỗ. Môn phái của nhân vật: zone
`KFaction` / `KPlayerFaction` (`LINUX-SERVER.md` §16.7), `--auto` làm đệ tử Thiếu Lâm (`SetFaction("shaolin")` rồi
`Include("\\script\\global\\skills_table.lua")` + `add_sl(20)` như script NPC phái: nhập môn + nhiệm vụ cấp 10 → 14, 10, 8, 4, 6) rồi chụp
`auto_skills.png` (Quyền) và `auto_skills_2.png` (Bổng). Chưa: thanh nhân vật 2.0
(`玩家信息主界面.ini`) với hai ô kỹ năng chuột, cây chọn kỹ năng (`技能选择树.ini`), phím F1..F11 (`ShortcutSkill(%d)`), trang
sống, chú thích kỹ năng (`KUiSkillTree`), gói 0x87, gói vào/ra chiến đấu.
