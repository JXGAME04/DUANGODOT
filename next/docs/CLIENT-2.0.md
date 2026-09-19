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
| `0x00651280` (ô `0x1f0/4 = 0x7c` của bảng handler — mảng con trỏ `[obj + 4·(gói+1)]` điền trong hàm tạo `0x0065DCB0`, §12 — tức **gói `0x7b`** vì bảng lệch một: ô 0x5a = gói 0x59 camp) | gói `{0x7b, phe, môn phái hiện tại, môn phái sau cùng, dword số lần}`: `SetCamp(npc, byte+1)`, `+0x12078 = int8 +2`, **`+0x12080 = int8 +3`** (sau cùng), `+0x12084 = dword +4`, báo UI; `0x006511B0` (gói `0x7c`): `+0x12078 = −1`, phe 4 | `G2C_PLAYER_FACTION` → `Game.faction / faction_last / faction_count / camp`, tín hiệu `faction_changed` |
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
(`玩家信息主界面.ini`) với hai ô kỹ năng chuột, cây chọn kỹ năng (`技能选择树.ini`), phím tắt Q W E A S D Z X C (`ShortcutSkill(0..8)`;
§8.1), trang sống, chú thích kỹ năng (`KUiSkillTree`), gói 0x87, gói vào/ra chiến đấu — B4b-2..B4b-5 đã làm các mục này trừ chú thích kỹ năng.

## 7. Ba thanh của màn hình 2.0 — `KUiControlBar` (thanh trên `顶部控制条.ini`, thanh công cụ `工具控制条.ini`), `KUiPlayerBar` (`玩家信息主界面.ini`) (M12 lát B4b-2, đã đọc từng dòng `gamecl.exe`)

Tệp (theme `\Ui\ui3_1024\`, xuất ra `client/assets/ui/thanh-dieu-khien-tren`, `thanh-cong-cu`, `thanh-nhan-vat`, `thanh-nhan-vat-thu-nho`):
- `顶部控制条.ini`: `[Main]` (236,0) 550×27 ảnh `血条底.spr`, `Button0..5 = Life Mana Stamina Exp Level WorldSort`; `Txt_Level` "Cấp" (28,1), `Txt_WorldSort`
  "Hạng" (475,1); mỗi mục `[Life]` (182,2) 91×10 `Tip` `Part=1` `ClassType=Player_Life` + `[Life_Image]` (`PartType=0`, ảnh thanh) + `[Life_Text]`
  (0,12) 91×10 `HAlign=1`; `Mana` (277,2) `PartType=1`; `Stamina` (87,2) `PartType=1`; `Exp` (372,2) `PartType=0`; `Level` (55,1) 50×12 chữ
  màu 55,231,63; `WorldSort` (498,1).
- `工具控制条.ini`: `[Main]` không ảnh; `Button0..9 = Status Items ItemEx Skills Team Faction ChatRoom Task Friend Options` (`Button14=ZhenFa` "không có");
  mỗi nút 21×23 y = 739: Status x=518, Items 543, ItemEx 568, Task 593, Skills 618, Team 643, Friend 668, Faction 693, ChatRoom 718, Options 743;
  `ClassType=Player_Status …`, `Esc_Options`.
- `玩家信息主界面.ini`: `[Main]` 1024×768 ảnh `玩家信息下版1024.spr` `DummyWnd=1` `PositionType=2` `ToolBoxSchema=工具控制条.ini`; `DateTime` (0,2)
  120×13 (khoá `GameLogo`, `SmoothMsg/CrowdMsg/BlockMsg` theo ping); `Recorder`; `Item_0..8` 36×36 y=728, x = 158 + 38·i (463 ở ô 8); `ImediaLeftSkill`
  (791,724) / `ImediaRightSkill` (828,724) 36×36 `EnableClickEmpty=1`; `InputEdit` (25,697) 232×15 `Type=2 MaxLen=80 Color=254,255,160 FocusBKColor
  25,31,11 α180 FocusNoCanBKColor 97,2,0 α150`; `SendBtn` (281,693) 21×20; `ChannelBtn` (1,692) 20×20; `Face` (260,694) 20×20; `InputBack` (0,692)
  304×24; `StatusBack` (0,508) 800×92 (không ảnh); `AdultPermit` (10,18). Bản thu nhỏ `玩家信息主界面最小化.ini` (`thanh-nhan-vat-thu-nho`) có
  thêm `Life/Mana/Stamina/Exp/Angry`, `Run/Sit/Status/Item/Horse/Skill/Options`, 3 ô thuốc, `ScenePos0/1`, `SwitchSizeBtn`.

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x00469840` | **`KUiControlBar::LoadScheme(ini, mục)`**: `Init(ini, mục)` (`0x00457D80`), `AddChild` `+0x554` `Txt_Level`, `+0xaec` `Txt_WorldSort` (`Init` với tên mục — thanh công cụ không có → trống); vòng `i = 0..`: `GetString(mục, "Button%d", "", tên, 32)` còn → `AddElement 0x004695B0` | `UiControlBar.load_scheme` |
| `0x004695B0` | **`AddElement(ini, mục)`**: mảng `+0x1088` (36 byte/mục: cửa sổ + tên 32) `realloc`; `GetString(mục, "ClassType")` → `0x0044E050` (bộ đăng ký toàn cục `0x8329E0`) `0x0044E150(tên)` → mô tả lớp `+0x20` = hàm tạo → `wnd->vtable[3](ini, mục)` (Init/LoadScheme) → `AddChild`. Lớp đăng ký `0x00449C40`: `Player_Life Player_Mana Player_Stamina Player_Exp Player_Level Player_WorldSort Player_Status Player_Items Player_ItemEx Player_Skills Player_Team Player_Faction Player_ChatRoom Player_Sit Player_Run Player_Horse Player_Exchange Player_PK Player_Recorder Player_Task Player_Friend Esc_Options` (vtable qua RTTI `.?AVPlayer_Life@@` = `0x0078E5E4`…) | `_add_element` |
| `0x00451C40` | **`LoadScheme` phần tử thanh** (Player_Life…): `0x00451580(ini, mục)` (cửa sổ nút, `Tip`); `[mục] Part` (mặc định 1) → `+0x17d0`; `sprintf("%s_Image")` → `Part` ≠ 0 ? `KWndPartImage +0xcdc` : `KWndImage +0x770`; `"%s_Text"` → `KWndText +0x1238`; `AddChild` ảnh + chữ; `vtable[10](0)` (không nhận chuột) | phần tử `{root, image, text}` |
| `0x004583D0` / `0x004584D0` / `0x00450F00` | **`KWndPartImage`**: `Init` đọc `PartType` (0..3, khác → 0), `ImgType`; `SetPart(cur, max)`: `max` = 0 → thôi, else `0x00450F00(sprite +0x494, cur, max)`: `cur ≥ max` → cả ảnh; `cur < 0` → rỗng; **PartType 0**: `right = w·cur/max`; **1**: `left = w − w·cur/max`; **2**: `bottom = h·cur/max`; **3**: `top = h − h·cur/max` (chia nguyên); vẽ `0x00458200` chỉ phần đó | `KWndPartImage.gd` + `KUiPartMath.part_rect` (test headless) |
| `0x0044AAA0` | **`Player_Life::Update`**: `GetGameData(0x3ea, 0x58 byte)` → `{+0 máu, +4 máu tối đa, +8 tối đa 2}` (khối `0x00661724`: npc `+0x105c`, `+0x12b14`, `+0x12b18`); `SetPart(máu, max(hai tối đa))`; công tắc `showplayernumber` (`[0x80ED44]`, **mặc định 1**) → chữ `"%d/%d"` (`0x00451DB0`, dấu `/` 0x2f) hoặc xoá (`0x00451DD0`); click `0x0044AB60` → Lua `Switch([[showplayernumber]])` | `refresh()` Life; click đảo `show_numbers` |
| `0x0044AB80` / `0x0044AC60` | `Player_Mana` (`+0xc`, `+0x10`, `+0x14`) / `Player_Stamina` (`+0x18`, `+0x1c`) như trên | |
| `0x0044AD20` | **`Player_Exp`**: ba số int64 của `KUiPlayerBar +0x8db8/+0x8dc0/+0x8dc8` (`0x00473910`) → phần trăm vào cấp (nhân 100.0, `ftol`) → `SetPart(pct, 100)`; chữ từ `GDI 0x3eb` | `KUiPartMath.exp_percent(exp, level_exp, next_level_exp)`; zone gửi `PlayerAttribSync.level_exp` = `level_exp[cấp − 1]` |
| `0x0044AFE0` / `0x0044B050` | `Player_Level`: `GDI 0x3eb +0` (cấp) → `SetText(số)` (`0x00451DC0`); `Player_WorldSort`: `GDI 0x3e9 +0x64` > 0 → số, else `"-"` (`0x7B8FB4`) | cấp; hạng "-" (chưa có bảng xếp hạng) |
| `0x0044B250` / `0x0044B290` / `0x0044B310` / `0x0044BAF0` / `0x0044B470` | click nút công cụ (vtable[17]): Lua `Open([[status]])` / `Open([[items]])` / `Open([[skills]])` / `Open([[system]])` / `Switch([[sit]])` (`0x00433E30` biên dịch, `0x0043A000` chạy); vtable[18] (`0x0044B230`…): cùng chuỗi khi `0x00524D70` cho phép | tín hiệu `command("status"/"items"/"skills"/…)` → `KUiGameWindows._on_bar_command` |
| `0x004750E0` | **`KUiPlayerBar::LoadScheme` (tĩnh)**: `+0x8d90` → `玩家信息主界面最小化.ini` else `玩家信息主界面.ini`; `0x00472360`; `[Main] ToolBoxSchema` → `0x00474B20(dir, tên)` nạp thanh công cụ | `UiPlayerBar.load_scheme` + `UiControlBar("thanh-cong-cu")` |
| `0x00472360` | **`KUiPlayerBar::LoadScheme(ini)`**: `Init(ini, [0x80EC58])`; `Face +0x554`, `Market +0xcc4`, `DateTime +0x1434`, `AdultPermit +0x1e7c`; **`Item_%d` i = 0..8** (`[0x838840] +0x23d0 + i·0x4f0`, `+0x28b8 + i·0x4f0 = 0`); `ImediaLeftSkill +0x5040`, `ImediaRightSkill +0x5528` (`0x0045E040(0)`); `InputEdit +0x6180` (`FocusNoCanBKColor` → `[0x83884C]`, alpha → `(255 − a) << 21` vào byte cao; thiếu → `0x6c0c…`); `SendBtn +0x5a10`, `ChannelBtn +0x686c`, `OpenChannelBtn +0x6d64`, `SwitchSizeBtn +0x74d4`, `InputBack +0xa164`, `StatusBack +0xa6b8` | `UiPlayerBar` (chat `LineEdit` trên ô `InputEdit`, `SendBtn`, 9 ô thuốc trống, hai ô kỹ năng chuột theo `Game.left_skill/right_skill`) |
| `0x004730D0` | đổi kênh chat (`ChannelBtn +0x686c`) | chưa |

Client mới: `client/ui/uicase/UiControlBar.gd` (hai thanh), `UiPlayerBar.gd` (thanh dưới), `client/ui/elem/KWndPartImage.gd` + `client/ui/KUiPartMath.gd`,
`KUiGameWindows._build_bars/_on_bar_command/_refresh_bars/_refresh_mouse_skills`; `UiGame` dùng dòng chat của thanh dưới. Chưa (ô thuốc nhanh: §7.2): `DateTime` ping/`GameLogo`, `ChannelBtn`/kênh chat, `Face` biểu cảm, thanh thu nhỏ `SwitchSizeBtn`, `Market`, hạng giang hồ.

### 7.1 Kỹ năng chuột mặc định — đánh thường theo vũ khí (M12 lát B4b-6a, đã đọc từng dòng)

Bản 2.0 nạp **cùng tệp với server** `\settings\武器物理攻击对照表.txt` (có trong `slistcl.pak` của client, 92 dòng, BOM UTF-8; cột `DetailType`,
`ParticularType`, `PhysicsSkillID`) trong lõi khởi động `0x005CADC0` tại `0x005CB396`: detail 0 → `0x9bcbf0[particular ≤ 99]`, 1 → `0x9bca60[…]`, −1 →
`[0x9bca5c]` (tay không), kỹ năng 1..0xbb6. Nội dung: tay không / đoản kiếm / đoản đao / song kiếm / song chuỳ / (6) → 53, thương / côn → 1, ám khí
(detail 1) → 2; các kỹ năng 229..232 không có trong bảng. `jxassets export-weapon-skill` (rơi về pak client khi thư mục server thiếu) →
`client/assets/weapon_skill.json` cho cả zone (`KWeaponSkillTable`, `LINUX-SERVER.md` §16) và client.

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x0060D660` / `0x0060E3B0` | `KItemList::GetWeaponType / GetWeaponParticular`: `[list+0x34]` = ô vũ khí đang mặc → bảng vật phẩm `[0x1f179a4]` bước 0x74c, `+0xc` / `+0x10`; không có → −1 | `Game.item_worn(3)` → `detail` / `particular` của vật |
| `0x005EBBA0` | **đánh thường theo vũ khí của một npc** (nhân vật mình: qua bảng trên; mục 0 của cây chọn kỹ năng §8) | `Game.weapon_attack_skill()` (`KWeaponSkillTable.skill_of`) |
| `0x005FE820` | **`UpdateWeaponSkill`**: đặt **cả hai** ô chuột = kỹ năng của bảng theo vũ khí đang cầm (`SetLeftSkill 0x005F7550` → `+0x2c`, `SetRightSkill 0x005FB280` → `+0x34` của `KPlayer +0xa878`; mỗi hàm chỉ nhận kỹ năng đang giữ cấp ≥ 1: `0x006233B0(sổ, id, 1) > 0`, rồi báo UI `0x005B8150(4/5, {0x40004, id})`); **người gọi duy nhất**: `KProtocolProcess::SyncEnd 0x00654F5B` (đồng bộ xong lúc vào game) | `Game.update_weapon_skill()` sau `G2C_SKILL_LIST` + `G2C_ITEM_LIST` và khi vật ở ô vũ khí đổi chỗ (`item moved`); tín hiệu `mouse_skill_changed` → thanh dưới vẽ lại |
| `0x00624F80` | `KPlayer::RemoveSkill(id)`: sau khi bỏ kỹ năng, ô chuột đang giữ id đó rơi về kỹ năng của bảng theo vũ khí (trái `0x625032..`, phải `0x625072..`) | chưa (client không bỏ kỹ năng) |
| op `0xd` của `OperationRequest 0x005C1CF0` (`0x5c2fd9`) | đặt kỹ năng chuột: id > 0 → `SetLeft/RightSkill(id)`; id = −1 → kỹ năng của bảng theo vũ khí (`0x5c2ff8..`) | `KUiGameWindows._on_skill_clicked` |

`--auto` (đệ tử mới, tay không): `AUTO_SKILL_TREE … mouse=53/53 weapon=53`, hai ô chuột của thanh dưới hiện Quyền cơ bản (53) ngay khi vào game
(`auto_world.png`). Chưa đọc: các chỗ khác gọi `SetLeftSkill` (`0x005FB340`, `0x005FDF60`, `0x006004C0`) — client mới tạm gọi lại luật khi vật ở ô vũ khí
đổi chỗ.

### 7.2 Ô thuốc nhanh `Item_0..8` — `ShortcutUseItem(0..8)` (M12 lát B4b-6b, đã đọc từng dòng)

Chín ô của thanh dưới (`Item_%d`, `KWndObjectBox` 36 px tại (158 + 38·i, 728)) là **lưới 9 × 1 phía client** trong `KItemList` (`+0x4ce8`; `0x00637DD0`
đọc ô (x, y), `0x00637CE0` ghi): mỗi ô giữ chỉ số một vật **trong túi** hoặc một kỹ năng (id | 0x4000000). Không liên quan phòng `room_immediacy`
3 × 1 của server (JX1: `pos_immediacy` = 7 phía client; `pos_equiproom` = 3 là túi — `GameDataDef.h` cũ). `autoexec.lua`: phím `1`..`9` →
`ShortcutUseItem(0..8)`.

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x0042F990` (Lua `ShortcutUseItem`) → `0x00472F80(k)` | `k > 8` → về; `GetObject 0x0045DD40` của ô k (`+0x23d0 + k·0x4f0`; 6 dword `+0x4b0..`) → `{genre, id, k, 0, x chuột, y chuột}` (`0x004664B0` = vị trí con trỏ) → `OperationRequest(0xa, &struct, 3)` | `KUiGameWindows._quick_key(k)` |
| op `0xa` (`0x5c2d91`) | nguồn 3 → loại 7 (ô nhanh), 5 → 3; genre 4 (kỹ năng): loại 7 → `0x0042BB20(id)` rồi thi triển tại (x, y) `0x005BC500`; genre 5 (vật): loại 7 → `UseItem(id) 0x005FD4B0` (vật phải ở túi: bản ghi `+0x18cc + idx·20 == 3`), loại khác → Lua `UiManage:ScriptHandleRCLickItem` rồi `0x005FA990`; genre 0x9e → `0x00612980` | kỹ năng → tín hiệu `quick_skill` → `UiGame.cast_skill_at_mouse`; vật → `Game.item_use` (trang bị → `item_equip`) khi còn trong túi |
| `WndProc 0x00475A10` | msg `0x511` (thả từ con trỏ) → `0x00446590(0x831978, 0)` rồi `0x00472E70(wParam, lParam)`; msg `0x513` (bấm ô có vật) → `0x00446590(…, 2)` rồi duyệt 9 ô so lParam → `ShortcutUseItem(k)` (`0x004763C0`); msg `0x512` trên `ImediaLeft/RightSkill` → `0x00496400(1/0)` mở cây | `UiPlayerBar.quick_put / quick_clicked` (KWndObjContainer `put_requested` / `object_clicked`) |
| `0x00472E70` | **thả vào ô**: struct nguồn `{genre, id, ô, 0, +0x10, +0x14, 3, …, con trỏ ô}` từ `GetObject` của ô (hoặc từ tay `0x00466310`), chỉ số ô = vị trí trong `+0x23d0 + i·0x4f0` → `OperationRequest(3, &từ, &đến)` | `_quick_put(k)` |
| op `3` (`0x5c2532`) → `Exchange 0x005FAD10` loại 7 (`0x5FADD5`) | `0x005F7C10` (đang giao dịch → thôi); vật trên tay `[+0x1878]` → bảng vật `[0x1f179a4]` bước 0x74c `+4/+0xc/+0x10` = genre/detail/particular → `0x006386C0(lưới +0x6544)` **đã có vật cùng loại trong thanh → báo `[0x9c00d8]`** (`0x005B8150(0x1f)`), else `0x00612A60(list, &từ, &đến)` đặt ô (bỏ khi vật ở `pos_immediacy` 7) | `KUiShortcutItem.put_item` (từ chối trùng loại), tay được thả, vật vẫn ở túi |
| `0x0060EB50(list, ô, genre, id)` | đặt ô: genre 5 → `0x0060C130(id)` vật phải tồn tại; genre 4 → `0x0060CF80(id, 0)` kỹ năng phải giữ → `id | 0x4000000`; `0x00637CE0(lưới +0x4ce8, ô, id, 0, 1, 1)` | `put_item` / `put_skill` |
| `GetGameData 0x3ec` (`0x00661B03`) | 0x58 byte: 9 × `{genre, id}` từ lưới (`0x00637DD0(+0x10dbc, i, 0)`; bit 0x4000000 → genre 4, else 5), rồi `{0x40004, kỹ năng chuột trái +0xa8a4}`, `{0x40004, phải +0xa8ac}` | `quick.slots` |
| `0x00473B10` (`SaveConfig`) / `0x00473970` (`LoadConfig`) | `[Player]`: `ShowLife`/`ShowName`/`…` (GDI 0x403/0x402/0x401); 9 ô: struct 0x14 `{genre, id, +GetGameData 0x7dd(id) 3 dword = genre/detail/particular của vật}` → `WriteStruct("Player", "Item_%d")`; nạp: `GetStruct` → `OperationRequest(0xe, &struct, i)`; rồi `LeftSkill`/`RightSkill` 8 byte | `user://shortcuts_<pid>.json` `items` (`{genre, id, kind, detail, particular}`) |
| op `0xe` (`0x5c30e1`) | `i ≤ 8`; genre 5 → `0x0060CA60(list, genre, detail, particular, &id…)` = `0x00638500` tìm trong túi vật cùng loại → `[ebx+4] = id`, không có → xoá struct; rồi `0x0060EB50(list, i, genre, id)` | `KUiShortcutItem.resolve(items, túi)` mỗi khi túi đổi (`items_changed`/`item_changed`/`item_removed`) |

Client mới: `client/ui/KUiShortcutItem.gd` (9 ô `{genre 4/5, id, kind, detail, particular}`, `put_item` (từ chối trùng loại), `put_skill`, `remove`,
`resolve`, `slot_of_key` 1..9 / bàn phím số, JSON; test `test_shortcut_items`), `UiPlayerBar` (`set_quick`, `set_hand`, tín hiệu `quick_clicked/quick_put/
quick_right_clicked`; ô `accept_free`), `KUiGameWindows` (`_quick_key`, `_quick_put`, `assign_quick`, `_quick_clear` = chuột phải xoá ô — thay cho kéo ra bằng tay,
`_refresh_quick`, tệp `shortcuts_<pid>.json` `{skills, items}`), `UiGame.cast_skill_at_mouse` (dùng chung với chuột phải). `--auto`: thuốc đầu tiên trong túi vào
ô 0 → `auto_quick.png`, phím 1 dùng thuốc → `AUTO_QUICK item=3 name=Kim Sáng Dược (tiểu) count=1 assigned=true count_after=-1 slot0=0 (thuốc dùng hết, ô tự xoá theo op 0xe)`. Chưa: kéo vật ra khỏi ô bằng tay (2.0: thả ô → tay), thông báo trùng loại (`[0x9c00d8]`),
genre 0x9e, `ShortcutEatMedicine` (`0x00439DB0`: 0 → `0x005A5B40`, 1 → `0x005A5B30` — MouseWheel của autoexec), chú thích ô.

### 7.3 Cập nhật và vẽ ô của thanh dưới — `KUiPlayerBar::UpdateData 0x00474350`, `KWndObjectBox::PaintWindow 0x004606E0` (M12 lát B4c-2, đã đọc từng dòng)

`KUiPlayerBar` (đối tượng 0xac10 byte, ctor `0x00474D50`, bảng ảo `0x790374`: [1] `UpdateData 0x00474350`, [4] `WndProc 0x00476170`, [6] `0x00474470` chọn câu
gợi ý theo giờ (`0x004730D0(n)`: chuỗi `+0x8c4c + n·32` → ô chữ `+0x686c`), [8]/[9] hiện/ẩn; tạo ở `0x00475525`).

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x00474350` | **`UpdateData`**: `GetGameData(0x3e9)` (0xcc byte, tên nhân vật → `+0x8d94`) → `0x004732B0` (`GetGameData(0x3ea)` 0x58 byte máu/nội lực/thể lực → `+0x8db8..+0x8dcc`, 17 nút `GetGameData(0x407)`, `0x005B9210`/`0x004C59F0` → hai số cho `0x00473430`); `GetGameData(0x3ec)` (9 ô nhanh + 2 kỹ năng chuột `{genre, id}`) → mỗi ô `KWndObjectBox::SetObject 0x0045DDB0(genre, id, 0, 0)` (ghi `+0x4b0/+0x4b4/+0x4c0/+0x4c4`; nếu chú thích đang mở trên ô này (`0x0044E530`) → `ShowObjectTip 0x0044EBC0` lại) | `_refresh_quick`, `_refresh_mouse_skills` |
| `0x004606E0` | **`KWndObjectBox::PaintWindow`**: nền cửa sổ `0x0046D180`; có đối tượng (`+0x4b0`) → `GetGameData(0x7d8, &{genre, id, …})` = cờ trạng thái; cờ kiểu ô (`+4` & 0x400/0x2000) và bit 1/2/4 → màu nền `0x80f428/42c/430/434` vẽ hình chữ nhật (`0x9bb0e0` +0x4c); `+0x4d8` (kiểu vẽ 1..0x30) → cờ; **`0x005B8FE0(genre, id, x, y, w, h, −1, cờ)`** vẽ đối tượng; bit 8 / 0x10 / 0x20 → `0x0045FF20(x, y, w, h, kind)` vẽ **hai khung viền** màu `0x0045F120(+0x494, kind)` (kind 0x10 → `+0xc`, 0x20 → `+0x14`, khác → `+0`) nhân độ sáng `+0x4e0` % | ô vật phẩm của túi đã có viền/nền theo `KUiItemView`; ô kỹ năng: không có lớp phủ |
| `GetGameData 0x7d8` (`0x00662F6D`, dải 0x7d1..0x7e2 → `0x00662B50`, bảng `0x663624`) | **chỉ vật phẩm**: genre 5 (bảng vật `[0x1f179a4]` bước 0x74c), 0xb/0xf/0x10 (kho/ghép); `[vật+8]` 1..5 → bit 8/0x10/0x20 (viền theo chất lượng), `0x0060F3F0(list, vật)` dùng được → bit 1 else bit 2; rồi `0x00662AA0` chép tên. **Genre 4 (kỹ năng) → 0**: không viền, không phủ | không port gì cho kỹ năng |
| `0x005B8FE0` → `0x00670130(genre, id, x, y, w, h, −1, cờ)` | bộ vẽ đối tượng: genre 3..16 bảng `0x670b40`; **genre 4** (`0x00670955`): cấp = `0x006233B0(sổ, id, 1)` (≥ 1), `KSkill* 0x0042BB20(id, cấp)` (bộ đệm `0x3a681c`, else `0x006053E0` tạo); nếu có npc mình và `[npc+0x1050] ≥ 0`: `còn = max(NextCastTime 0x006235F0, khung hiện [0x1f178c4 + +0x1050·0x138 + 0x50]) − khung hiện`, `tổng = 0x00623620` → `KSkill::vtable[24](x, y, w, h, còn, tổng)`; id −1 → vẽ `icon_sk_ty_ap.spr` | `KWndObjContainer` vẽ ảnh; hồi chiêu: xem dưới |
| `KSkill::Draw 0x007039D0` (bảng ảo lớp con `0x7b549c`, ô 24; lớp gốc `0x7b4fcc` ô 24 = rỗng `0x5b0a96`) | chép `SkillIcon (+0x64)` vào struct vẽ (`+0xe8`), đặt (x, y), khung 0, gọi bộ vẽ sprite (`0x1f16e80` +0x4c, (1, &struct, 3, 1)); **hai tham số hồi chiêu bị bỏ qua** → bản 2.0 này không vẽ hồi chiêu trên ô kỹ năng (thanh dưới, cây) | không có lớp phủ hồi chiêu (đúng như 2.0) |
| `WndProc 0x00476170` + `KWndObjectBox` rê chuột | rê lên ô có đối tượng → `ShowObjectTip 0x0044EBC0` (`SetObject` cũng gọi khi chú thích đang mở trên ô); kỹ năng → §10, vật phẩm → chú thích vật | `UiPlayerBar.quick_hovered / mouse_skill_hovered` → `KUiGameWindows._on_quick_hovered` (vật → `_on_item_hovered`, kỹ năng → `_show_skill_tip`), ô kỹ năng chuột → `_show_skill_tip` |

`--auto` rê lên ô kỹ năng chuột trái → `auto_bar_tip.png` (`AUTO_BAR_TIP skill=53 lines=16 (auto_bar_tip.png: chú thích Công kích vật lý với cấp, phạm vi, độ chính xác, sát thương, cấp kế)`). Chưa: câu gợi ý theo giờ `+0x8c4c` (`0x00474470`), 17 nút `GetGameData(0x407)`,
`0x004732B0`/`0x00473430` (số cho thanh trên đã lấy đường khác), viền chất lượng vật trên ô nhanh (`GetGameData 0x7d8` bit 8/0x10/0x20).

## 8. Cây chọn kỹ năng cho chuột — `KUiSkillTree` (`技能选择树.ini`, M12 lát B4b-3, đã đọc từng dòng `gamecl.exe`)

Tệp `\Ui\ui3_1024\技能选择树.ini` (`chon-ky-nang`): chỉ `[Main]`: `LeftBtnPos=760,650` (vị trí ô thấp nhất bên trái), `RightBtnPos=710,450`,
`BtnSize=36,36`, `KeyFont=16`, `KeyColor=255,0,0`, `MaxBtnCountPerRow=7`. Mở bằng Lua `Open([[leftskill]])` / `Open([[rightskill]])`
(bảng tên `0x80dae8` ô 9/10 của `0x0042EF50`) khi bấm ô `ImediaLeftSkill` / `ImediaRightSkill` của thanh dưới.

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x0042F0EB` / `0x0042F115` | `Open("leftskill")`: đang mở (`0x004957A0`: đối tượng `0x83f4d0` và cờ `+4 & 0x80000000`) → `0x004957C0` đóng (vtable[9]; tham số ≠ 0 → huỷ), else `0x00496400(1)`; `Open("rightskill")` → `0x00496400(0)` (`esi` = 0 sau `xor esi, esi` ở `0x0042EF87`) | `UiSkillTree.toggle_for(right)` |
| `0x00496400` | **`OpenWindow(side)`**: đối tượng đơn 0x8d0 byte (ctor `0x00495670`, `Init 0x00496360` → `LoadScheme`, `0x00466180(this, 2)`), `+0x8b0 = (side ≠ 0)` (1 = trái), `vtable[1]` (UpdateData), `vtable[8]` (Show), `0x0046BE90` (lên trên) | `open_for(right)` |
| `0x00495C20` | **`LoadScheme`**: `[Main] LeftBtnPos` → `+0x8b4/+0x8b8`, `RightBtnPos` → `+0x8bc/+0x8c0`, `BtnSize` → `+0x8c4/+0x8c8` (≤ 0 → 1), `KeyFont` (12) `+0x8a8`, `KeyColor` `+0x8ac`, `MaxBtnCountPerRow` `+0x8cc`; rồi xếp `0x00495A40` | `load_scheme` |
| `0x00495AE0` | **`UpdateData`**: `GetGameData(+0x8b0 ? 0x3f7 : 0x3f8, danh sách +0x498, 0)` → số mục `+0x494`; duyệt bảng phím tắt `0x83f440` (9 × 16 byte `{genre, id, bên, …}` từ `[ShortSkill] ShortcutSkill_%d` — `0x00495810`) đối chiếu; rồi xếp | `open_for`: danh sách + `Layout.place` |
| `0x006239F0` (GDI 0x3f7, trái) / `0x00623B70` (GDI 0x3f8, phải) | mục 0 = `{0x40004, đánh thường theo vũ khí đang cầm (`0x005EBBA0`, §7.1), 0, nhóm 0}`; mỗi ô sổ kỹ năng (`+0x38`, bước 0x1c) có id 1..3000 và cấp 1..64: phiên bản `(id, cấp)`; style (`vtable[3]`): **trái**: 5..12 → nhận; 0..4 và 14 → nhận khi (`IsAura` (`vtable +0x4c`) = 0 và `LRSkill` (`+0x110`, cột 12 của `skills.txt`) = 0) hoặc `LRSkill` = 1, rồi `ReqLevel (+0x6c) ≤ cấp npc`; 13 → bỏ. **phải**: chỉ style 0..4 và 14 khi (`IsAura` = 0 và `LRSkill` = 0) hoặc `LRSkill` = 2, cùng kiểm cấp (`LRSkill` 3 = không lên cây: 8/4/6 Thiếu Lâm; 15/16 = 2 chỉ chuột phải); mục = `{0x40004, id, ?, nhóm = chỉ số / 8}` (`0x00623B0C`), tối đa 0x40 | `KUiSkillTreeLayout.listed(style, aura, lr, right, req_level, level)`, `SkillStyle/IsAura/LRSkill/ReqLevel` của `skills.json` |
| `0x00495A40` | **xếp**: duyệt mục: cùng nhóm với mục trước và số ô trong hàng < `MaxBtnCountPerRow` → cùng hàng; else hàng mới (đếm hàng, hàng rộng nhất); `SetPosition(LeftBtnPos.x, LeftBtnPos.y − h·hàng + h)`, `SetSize(w·rộng nhất, h·hàng)` — **cả hai bên đều neo `LeftBtnPos`** (`RightBtnPos` đọc mà không dùng trong phần đã đọc) | `Layout.place`, `Layout.window_rect` |
| `0x00496170` | **vẽ**: từ đáy cửa sổ đi lên (`+0x1c + +0x14` trừ `h` mỗi hàng); mỗi mục vẽ đối tượng `{genre, id}` `0x005B8FE0(genre, id, x, y, w, h, −1, 0)`; mục trùng bảng phím tắt (cùng bên, genre, id — `0x00496260`) → hỏi phím của lệnh Lua `ShortcutSkill(k)` / `DirectShortcutSkill(k)` (`0x00433E30`, bảng `AddCommand` của `autoexec.lua`) → chữ phím bằng `KeyFont`/`KeyColor` (`0x0049627D`) | `_draw`: `shortcuts.slot_of(id, bên)` → `KFont.of(KeyFont).draw(chữ, KeyColor)` (§8.1) |
| `0x00495B80` | **hit test**: hàng = `(đáy − y) / h`, cột = `(x − trái) / w`, duyệt lại cùng thuật toán xếp | `Layout.hit` |
| `0x00495E10` | **`WndProc`**: `0x202` (nhả chuột trái): mục dưới chuột → `OperationRequest(0xd, &{genre, id}, +0x8b0 == 0)` (đặt kỹ năng chuột; 1 = phải) rồi ẩn; `0x205` (chuột phải) → ẩn; `0x100` phím `0x1b` Esc → ẩn; di chuột (`0x2a1`) → chú thích đối tượng `0x0044EBC0(genre, id, 0x10)` | `_gui_input`: `picked(id, right)`, ẩn; `hovered(id)` → chú thích |

Client mới: `client/ui/uicase/UiSkillTree.gd` + `client/ui/KUiSkillTreeLayout.gd` (xếp/hit/danh sách, test headless `test_skill_tree_layout`),
`KUiGameWindows.skill_tree` (bấm ô kỹ năng chuột của thanh dưới → `toggle_for`, chọn → `_on_skill_clicked`, rê → chú thích), `--auto` chụp
`auto_skill_tree.png`. Chưa: `RightBtnPos`. Phím tắt: §8.1. Bảy đánh thường (53, 1, 2, 229..232, cùng biểu tượng `icon_sk_ty_ap.spr`) lên cây vì
nhân vật mới được server phát cả bảy (`newplayerini00.ini [FSKILLS]`) và chúng có `LRSkill` 0 — đúng như 2.0 sẽ hiện.

### 8.1 Phím tắt kỹ năng — `ShortcutSkill(k)` / `DirectShortcutSkill(k)` (M12 lát B4b-5, đã đọc từng dòng)

**Đã đọc thêm (2026-09-18, chưa port)**: `MouseWheelUp/Down` → `ShortcutEatMedicine(0/1)` (Lua `0x00439DB0` → `0x005AB7B0/0x005AB710`) dùng đối tượng tuỳ chọn `0x8230c0` (`+0x118` ô, `+0x11c/+0x120` và `+0x124/+0x128` bật/số; ctor `0x005E6430` mặc 0 → **tắt** khi chưa có cửa sổ tuỳ chọn treo máy); `DirectShortcutSkill` (`0x0042F930` → `0x00496070`) **không có phím** trong `autoexec.lua`.
Bản 2.0 **không** dùng F1..F11 cho kỹ năng: `\Ui\autoexec.lua` (trong `\reslst.dat`) gắn `AddCommand("Q", "", "ShortcutSkill(0)")`, `W` 1, `E` 2, `A` 3, `S` 4,
`D` 5, `Z` 6, `X` 7, `C` 8 (chín ô); `1`..`9` → `ShortcutUseItem(0..8)` (ô thuốc nhanh); `F1` activityguide, `F2` options, `F3` status, `F4` items, `F5` skills,
`F6` friend, `F7` showplayername, `F8` showplayerlife, `F9`/`Ctrl+H` pk, `F11` tasknote, `F12` NewTask, `Tab` map, `Esc` system, `Enter` commandline, `P` team,
`Space` SpacePickUp, `Del` ClearMessage, `` ` `` battlereport; chuột: `LButton` `Mouse_Action`, `RButton` `Mouse_Force1`, `Shift+LButton` `Mouse_Force0`,
`Ctrl+LButton` `Mouse_Say`, `Ctrl+RButton` `Mouse_Menu`, `Alt+LButton` `Mouse_PartnerAction`, `Alt+RButton` `Mouse_Emote_Menu`.

| Địa chỉ | Làm gì | Client mới |
|---|---|---|
| `0x0042F8D0` (Lua `ShortcutSkill`) → `0x00495F70(k)` | `k > 8` → về; **cây đang mở** (`0x004957A0`): mục dưới chuột (`+0x8d4`, hit `0x00495B80`) → ô `k` của bảng `0x83f440` (9 × 16 byte `{genre, id, bên (+0x8b0), …}`); **mọi ô khác** cùng `{bên, genre, id}` bị xoá (`0x00495FEA`); ghi thiết lập; vẽ lại. **Cây đóng**: ô `k` có id → `OperationRequest(0xd, &{genre, id}, bên == 0)` = đặt kỹ năng chuột của bên đó | `KUiGameWindows._shortcut_key(k)`: cây mở → `assign_shortcut(k, hovered_skill(), right_side)` (`KUiShortcut.assign` xoá ô trùng); đóng → `_on_skill_clicked(id, right)` |
| `0x0042F930` (Lua `DirectShortcutSkill`) → `0x00496070(k)` | như trên khi cây mở; cây đóng → thi triển ngay ô `k` (`0x005BC500`) — `autoexec.lua` 2.0 không gắn phím cho lệnh này | chưa (không có phím) |
| `0x00495810` (từ `0x00447390` lúc vào game) | đọc bảng: `GetStruct("ShortSkill", "ShortcutSkill_%d", ô, 16)` qua `0x0052D720` (tệp thiết lập riêng từng nhân vật) | `user://shortcuts_<player_id>.json` (`[{id, right}] × 9`), nạp khi dựng cửa sổ, ghi mỗi lần gán |
| `0x0049627D` (trong vẽ `0x00496170`) | mục có ô phím tắt (`0x00496260`: cùng bên, genre, id) → tên phím của lệnh `ShortcutSkill(k)` (`0x00433E30`) vẽ bằng `KeyFont` (16) / `KeyColor` (255,0,0) | `UiSkillTree._draw` (`KFont.of(16)`, viền đen; không có phông → phông mặc định) |

Client mới: `client/ui/KUiShortcut.gd` (bảng 9 ô, `assign`/`slot_of`/`key_of`/`slot_of_key`, JSON; test headless `test_shortcuts`), `KUiGameWindows` (phím
`Q W E A S D Z X C`, `F3`/`F4`/`F5` mở nhân vật/túi/kỹ năng — `I`/`K` vẫn giữ, `M` lên/xuống ngựa `Game.ride`), `UiSkillTree.hovered_skill()` + chữ phím; `--auto` mở
cây, rê lên hai mục đầu rồi `ShortcutSkill(0)`/`(1)` → `auto_skill_tree.png` có chữ Q, W đỏ (cây chỉ liệt kê kỹ năng cấp 1..64 — kỹ năng phái cấp 0 chưa có). Chưa: `DirectShortcutSkill`, ô thuốc nhanh `ShortcutUseItem(0..8)` (phím 1..9), các
`Open([[...]])` khác (map, team, friend, options, system), `Switch` (pk, tên/máu người chơi), `Mouse_*`.

## 9. Danh sách trạng thái kỹ năng — `KUiSkillState` (`技能状态列表.ini`, gói 0x87; M12 lát B4b-4, đã đọc từng dòng)

Tệp `\Ui\ui3_1024\技能状态列表.ini` (`trang-thai-ky-nang`): `[Main]` (146,28) 240×72 không ảnh; `[BuffImage]` 24×24 `Trans=1`; `[txtBuffTime]` (0,21) 24×12
`Font=12 HAlign=1 Color=55,231,63`; `[DebuffImage]` / `[txtDebuffTime]` như trên; `[BuffList]` `BuffCount=225`, mỗi mục `Buff_%d_ID` (id kỹ năng),
`Buff_%d_Name`, `Buff_%d_Image` (`\spr\Ui\状态图标\*.spr` — bộ xuất đã ghi `bufflist-buff-N-image.png`), `Buff_%d_Desc`, `Buff_%d_IsDebuff` (tuỳ), `Buff_%d_Level` (tuỳ).

| Địa chỉ | Làm gì | Client mới / zone |
|---|---|---|
| `0x0041F460` | **`KUiSkillState::LoadScheme`**: `Main`; vòng `i = 0..9` (bước 0x18 = 24 px): `BuffImage[i]` (`+0xf60 + i·0x554`) `SetPosition(24·i, 0)` + `txtBuffTime[i]` (`+0x44a8 + i·0x598`) con, ẩn; `DebuffImage[i]` (`+0x7c98`) `SetPosition(24·i, 0x24)` + `txtDebuffTime[i]` (`+0xb1e0`), ẩn; rồi `0x0041EFF0(ini, "BuffList", map +0x554)` | `UiSkillState.load_scheme` (`KUiStateMath.slot_pos`) |
| `0x0041EFF0` | **bảng `[BuffList]`**: `BuffCount`; mỗi `k`: `Buff_%d_ID` > 0 → bản ghi 0xc8 byte `{Level (mặc định −1), IsDebuff, Name, Image, Desc}` vào map theo id (`0x0041E900`) | `_table[id] = [{level, debuff, name, desc, image}]` |
| `0x0041EA60` | **cập nhật** (mỗi khung thứ 9, `[0x822e10] % 9`): `OperationRequest(0x8c, &buf, 0)` → danh sách trạng thái của nhân vật; với mỗi trạng thái: tìm map theo id kỹ năng (`+0x10`), mục có `Level == −1` hay `== cấp (+0x14)`; `IsDebuff` (`+0xd4`) → hàng debuff, else hàng buff (tối đa 10 mỗi hàng); `SetImage` (`0x004580A0`), chữ thời gian `0x0041D720`, chú thích `"%s\n%s\n%s"` (tên, mô tả, thời gian) | `refresh()` |
| `0x0041D720` | **chữ thời gian** `(giây, buf, n, dài)`: ≤ 0 → `"N/A"`; dạng dài → `"%02dh:%02dm:%02ds"`; < 60 → `"%ds"`; < 3600 → `"%dm"`; < 356400 (0x57030) → `"%dh"`; else ngày | `KUiStateMath.time_text` (test headless) |
| `0x006526E0` (ô 0x88 của bảng handler = **gói 0x87**) | `{0x87, word cỡ = 19 + n·16, dword id npc, dword kỹ năng, dword cấp, dword thời gian, dword +0x13, byte +0x17, n × 16 byte trạng thái}` (n = 10 − (0xb8 − cỡ)/16) → `KNpc::SetStateSkillEffect 0x005EDFC0(npc, kỹ năng, cấp, &trạng thái, n, thời gian, 0, +0x13, 0, 0, +0x17)` (bên client: hiệu ứng + sổ trạng thái) | `G2C_ENTITY_STATE` → `Game.states[id] = {level, time (khung), until_ms, special_id, states}`, tín hiệu `state_changed` |
| server `0x08086892` (trong `SetStateSkillEffect 0x08086260`) | gói 0x87 dựng sau khi nút mới đầy (`byte 0x87; word 0x153 − (0x14 − n)·16; id npc; kỹ năng; cấp; thời gian; byte a9; memcpy n·16 trạng thái`), chỉ khi npc là người chơi (`+0x24 == 1`, `0x08086902`) → `0x080A8400(người chơi, buf, cỡ)`; các nhánh làm mới nút cũ **không** gửi | `KSubWorld::emit_state(e, node, false)` sau `push_back` |
| server `0x0807D40A` (trong `RemoveStateSkillEffect 0x0807D310`) | gói 0x87 rỗng `{0x87, 0x13, id npc, kỹ năng, cấp 0x3f, thời gian 0, byte 0}` cho người chơi (và chủ của đồng hành `+0x1698`) khi `bNotify` | `emit_state(e, node, true)` khi `notify` (`removed = true`) |

Client mới: `client/ui/uicase/UiSkillState.gd` (dưới thanh trên, 10 buff + 10 debuff, thời gian đếm lùi từ `until_ms` = khung/18 s, chú thích),
`client/ui/KUiStateMath.gd`, `KUiGameWindows.state_window`, `KProtocolProcess.states / state_changed`; zone `emit_state` (`KSubWorld.cpp`), proto
`EntityState` / `StateAttrib` (2122); `--auto` (đệ tử cấp 20, `add_sl(30)`) thi triển Bất Động Minh Vương (15) lên mình → `auto_state.png`. Chưa: đồng hành
(`+0x1698`), hiệu ứng hình ảnh trạng thái trên npc (client `0x005EDFC0`), `+0x13/+0x17` của gói (a9).

## 10. Chú thích kỹ năng — `KSkill::GetDesc` (`0x006FBC90`; M12 lát B4c-1, đã đọc từng dòng `gamecl.exe`)

Rê chuột lên ô kỹ năng (sổ, cây) → `ShowObjectTip 0x0044EBC0(obj {genre, id}, x, y)`: chọn kiểu 1..4 theo phím (`0x004463E0`: Shift → 3, Ctrl/Alt → 2/4,
`GetGameData 0xbbd`), rồi `GetGameData(kiểu, &obj, &buf)` (dải 1..12 → `0x00663990`; genre `0x40004` = kỹ năng → `0x006FBC90(id, cấp, buf, chỉ số chủ,
cờ, lệch cấp, −1)` hoặc `0x007036B0` chỉ tên) và đổ vào cửa sổ chú thích `0x8329e8` (`0x0044E5C0`…). Bản 2.0 tính số theo cấp **bằng Lua ngay trên client**
(`0x006FC7A0`: `LvlSetScript`, `LvlSetting%d`/`LvlData%d`, gọi `GetSkillLevelData(levelname, data, level)` "ssd" của `\script\skill\*.lua` — cùng script
server chạy); client mới hỏi zone (`C2G_SKILL_DESC` → `G2C_SKILL_DESC`, zone đã chạy `LoadSkillLevelData`).

**Dữ liệu chữ** (`jxassets export-skill-desc` → `client/assets/text/skill_desc.json`): bảng chuỗi toàn cục `\lang\vn\stringtable_core.txt` (`key\tvalue`, TCVN3,
1360 dòng; nạp bởi `0x005DBA60(tên tệp)` từ `0x005CAB29` `"\lang\%s\stringtable_core.txt"` với thư mục ngôn ngữ = bảng `0x80ec60[chỉ số]` (5 = `vn`); mỗi khoá
`G_*` → ô `0x9bfXXX`, `\n` trong tệp là hai ký tự và được đổi thành xuống dòng); `[Descript]` của `\settings\magicdesc.ini` (`0x00608BD0` nạp; đã có sẵn ở
`ui/du-lieu/mo-ta-ma-phap.json` cho vật phẩm); `[SkillAttrib]` / `[SkillType]` / `[WeaponLimit]` của `\settings\gamesetting.ini` (KIniFile `0x24ec3e8`).

| Địa chỉ | Làm gì (thứ tự ghép chuỗi) | Client mới (`KUiSkillDesc.build`) |
|---|---|---|
| `0x006FBD3D` | `"<color=Yellow>"` + `SkillName` (+4); nếu `+0x7a4 == 75` (thuộc tính `seriesdamage_p` của cấp) và `Series (+0x58) ≠ −1` → `" "` + `G_S_GOLD..G_S_EARTH[Series]` (`0x9bf45c..`) | tên vàng, + ngũ hành khi có `seriesdamage_p` |
| `0x006FBEEA` | `"\n<bclr=Black><color>"` | như vậy |
| `0x006FBF2F` | `ReqLevel (+0x6c, word) > cấp nhân vật` → `sprintf(G_SkillList_6 "Đẳng cấp yêu cầu: %d", ReqLevel)` | như vậy |
| `0x006FBF83` | `"\n"` + `SkillDesc (+0x114)` + `"\n\n"` (`[0x7afd6c]`) | như vậy |
| `0x006FC022` | `_itoa(Attrib (+0x4e8))` → `GetString("SkillAttrib", số)` của `gamesetting.ini` + `"\n"` (vd 202 = "Võ công lưu phái: <color=Cyan>Quyền pháp (Ngoại công)<color>") | `skill_attrib[Attrib]` |
| `0x006FC085` | `Attrib == 1` hoặc `2` (đánh thường): `IsMelee (+0x3c) ≠ 0` → `G_Skills_35 " (Công kích gần) \n"` else `G_Skills_36` | như vậy |
| `0x006FC114` | **`IsWeaponSkill (vtable +0x4c của KSkill client = [+0x34]) ≠ 0`** (đính chính B4d-1: bảng ảo client `0x7b50ac` là `+0x44 AttackRadius [+0x48]`, `+0x48 IsAura [+0x38]`, `+0x4c IsWeaponSkill [+0x34]` — thiếu một ô so với server; đánh thường bỏ dòng cấp) → nhảy tới hạn chế vũ khí (không dòng cấp, không cấp kế). Không aura: lệch cấp (tham số) = 0 → `G_Skills_37 "Cấp hiện tại: %d"` + `"\n"`; ≠ 0 → `"<color=Blue>"` + `G_Skills_38 "Cấp hiện tại: %d (%d+%d)"` **(cấp hiện, cấp hiện − lệch, lệch)** + `"\n<bclr=Black><color>"`. Người gọi `0x00663FDB`: cấp = `0x006233B0(sổ +0x124, id, 1)` (cấp hiện tại **có** cộng thêm), lệch = cấp − `0x00623380(sổ, id)` (cấp đã học `+0xc` của ô) | zone gửi `held_level` (`get_current_level(id, true)`) và `level_inc` (− `get_level`); dòng 38 xanh khi ≠ 0 (B4c-5) |
| `0x006FC27F` | **`0x00602420(sổ +0x124, id)`** = con trỏ vào map `sổ+0xf14` (id → %; `0x006246C2`: khi một kỹ năng đang giữ đổi cấp, 6 ô `addskilldamage` của bản cũ trừ đi, bản mới cộng vào — đúng `KSkillList::update_enhance` của zone) → `*p ≠ 0` → `G_Skills_39 "Tăng từ kỹ năng: %d%%\n"`; `[SkillType][itoa(Attrib +0x4e8)]` (`gamesetting.ini`) = 1 hoặc 2 → `Player+0x1278 + Player+0x1148` ≠ 0 → `sprintf("%s%d%%\n", G_Skills_76 "Trang bị gồm có:", tổng)`; `0x005EC4F0(Player, id)`: id = `Player+0x12ac8` (hay id 715 khi đó là 723) → `&Player+0x12ad8` (một thuộc tính) → `KMagicDesc::GetDesc 0x0060A2B0` → `G_Skills_76` + chữ + `"\n"` | zone gửi `enhance` = `skill_list.enhance[id]` → dòng 39 (B4c-5). **B4c-8**: `+0x1148` = thuộc tính client **244 `magicdamage_p`** (bảng tên client `0x1f16e90 + id·4`, bảng hàm `0x00700C00`, ô 244 → `0x006FF3F0` cộng `+0x1148`; trùng số với server 244 → `+0x1284` = `cur.magic_damage_percent`) → zone gửi `equip_percent`; `+0x1278` do thuộc tính client **299 `movedistanc_710_enhance`** (`0x006FF6E0`: `+0x1274 = 1, +0x127c = v0`; `Activate 0x005F3CB5`: `+0x1278 = máu%·(+0x127c)/100`) — bảng tên client từ 299 có 9 mục 2.0 (`validtiem_1366_enhance`, `daoxutian_enhance`, `foxinciyou_enhance`, `dec_pskill_cdtime`, `expenhance_p2`, `enhancehit_effect`, `anti_enhancehit_effect`, `monthcard_power`) mà `jx_linux_y` **không có** (server 299 = `melee_returnres_p`) → phần này = 0. `+0x12ac8..+0x12ae4` = bộ sửa trạng thái của client (`0x005EC5B0(kỹ năng, kiểu, ô, delta, cờ, thuộc tính)`, = `KNpc+0x19d8` server `state_modifier`) → zone gửi `modifier` (thuộc tính với `v[ô] = delta`) khi `state_modifier.skill_id == id`; client in `G_Skills_76 + %d%%` khi `[SkillType][Attrib]` ∈ {1,2} và `equip_percent ≠ 0`, rồi `G_Skills_76 + dòng [Descript]` của `modifier` |
| `0x006FC473` | `IsExpSkill (+0x44)` → `0x006F7190(sổ, id)` × 100 → `G_Skills_40 "Độ tu luyện: %d%%"` | `exp_percent` của kỹ năng đang giữ |
| `0x006FC504` → **`0x006FB140` `GetDescAboutLevel(cấp hiện)`** | `"\n"` + `+0xb30` (chuỗi cấp, thường rỗng); `+0x7a4 == 75` → `G_Skills_75 "Ngũ Hành Tương Khắc: %d%%"` với `+0x7a8`; `GetSkillCost (vtable +0x1c)` ≠ 0 → theo `SkillCostType (+0x9c)`: 0 `G_Skills_45` nội lực, 1 `G_Skills_46` thể lực, 2 `G_Skills_47` sinh lực; `GetAttackRadius (vtable +0x44)` ≠ 0 → `G_Skills_48`; **`0x006FAA00` → `0x006F82F0`**: ba nhóm thuộc tính `0x006F6E30(buf, mảng, n)` — tức thời `+0x7d8`/`+0x918`, sát thương `+0x694` 20 ô cố định, trạng thái `+0x91c`/`+0x0a5c` — mỗi mục `KMagicDesc::GetDesc 0x0060A2B0` (`[Descript][tên]`, rỗng → bỏ) + `"\n"`; rồi **`0x006FAA00(this, buf, &đếm, cờ 0)`** với `đếm = 1` (§10.1: các kỹ năng mà cấp này nêu tên), rồi 6 mục `+0x4fc` (addskilldamage `{id, %}`): id ≠ 0, % ≠ 0, có bản (`[0x1e59dc0 + id<<8]` hay `GetSkill(id, 1)`), tên (vtable+8) không rỗng và **`ShowAddition (+0x4f0) ≠ 0` của kỹ năng đích** → `G_Skills_49 "Tăng cho kỹ năng %s: %d%%"` | `level_lines(cur)`: chi phí/phạm vi/thuộc tính (zone gửi `attribs` nhóm 0/1/2 + `related` + `appends`; client lọc `ShowAddition` bằng `skills.json`); chưa: `+0xb30`, `G_Skills_75` |
| `0x006FC527` | `EqtLimit (+0xac)`: −2 → bỏ; ≥ 0 → khoá `"%d"`; < 0 → `"F%d"` (−1 → `F1` Tay không) → `GetString("WeaponLimit", khoá)` → `G_Skills_41 "Hạn chế vũ khí:"` + chữ + `"\n"` | như vậy (khoá chữ thường) |
| `0x006FC617` | `HorseLimit (+0xb0)`: 1 → `G_Skills_42`, 2 → `G_Skills_43` | như vậy |
| `0x006FC697` | không phải kỹ năng vũ khí (vtable +0x4c, xem trên), tham số "cấp kế" và có kỹ năng cấp kế → `G_Skills_44 "\n<color=Red> Đẳng cấp tiếp theo \n"` + `0x006FB140(cấp kế)` | `has_next` → `level_lines(next)` |
| `0x0060A2B0` (`KMagicDesc::GetDesc` 2.0) | `GetString("Descript", tên thuộc tính)`; duyệt `#`: dấu `+`/`~` (`0x00608BE0`), chữ số 1..9 → `0x00608C00`: `idx = c − '1'`, giá trị `value[idx % 3]`, `idx/3` = 0 nguyên, 1 `>> 8`, 2 `& 0xff`; chữ cái → `0x0060A110` (d/f/k/s/m/x/l như `KMagicDesc.cpp` cũ; mỗi dấu **luôn 4 ký tự** nên `[#l1]` nuốt `]`) | `KMagicDesc.describe_line` (thuần, test `test_magic_desc`) |

### 10.1 Các kỹ năng mà một cấp nêu tên — `0x006FAA00` / `0x006F7F70` (M12 lát B4c-5, đã đọc từng dòng)

`0x006FAA00(this = KSkill, buf, &đếm, cờ)`: (1) ba nhóm thuộc tính `0x006F82F0`; (2) **vector `+0xa74`** (`{id, cấp}` 8 byte; vector có kiểm tra iterator của MSVC:
`+0xa78/+0xa7c/+0xa80`) — chỉ được đẩy ở **`AddMagicAttrib` của client `0x006FC7A0`** nhánh **id 11 `skill_appendskill`** (`0x006FCA13` → `0x006FC710 push_back{v0, v1}`
khi `v0 ≠ 0`; cùng số với server, zone `append_skills`): mỗi mục → `cấp = 0x006233B0(sổ của mình, id, 1)` (cấp đang giữ, có cộng thêm; **không giữ → 0**), `cấp = min(cấp,
cấp liệt kê)` → **`0x006F7F70(this, buf, id, cấp, &đếm, cờ 1)`**; (3) **`+0x4ec` = `ShowEvent`** (cột `ShowEvent` của `skills.txt` — chỉ 1110 có; hoặc thuộc tính
**`skill_showevent`** = id 323 của client (`0x006FCB3D`) = id 317 của server, `v0`): bit 1 → `StartSkillId +0xf8`, bit 2 → `FlySkillId +0xf0`, bit 4 → `CollidSkillId +0x100`,
bit 8 → `VanishedSkillId +0xfc`, cấp = `EventSkillLevel +0x104` (−1 → cấp của chính nó `+0x4f4`), cờ = cờ nhận vào (0 → có "Chiêu N:"); bit 0x10 → duyệt danh sách
trạng thái `+0x91c` (đếm `+0xa5c`): loại 195..198 (`autoreplyskill..autodeathskill`) → id = `v0 >> 8`, cấp = `v0 & 0xff`, cờ 5.

`0x006F7F70(this, buf, id, cấp, &đếm, cờ)`: id ≤ 0 hay cấp ≤ 0 hay `GetSkill 0x0042BB20(id, cấp)` NULL → **không gì cả**; `!(cờ & 1)` → `G_Skills_(50 + đếm)` (20 chuỗi
`"<color=Blue> Chiêu N: <color>"` chép về `0x24f3910`) rồi `đếm++` — **đếm bắt đầu 1** (`0x006FB376` đặt ngay trước khi gọi) nên mục đầu là **"Chiêu 2:"** (chiêu 1 là
chính kỹ năng); `!(cờ & 2)` → `"<color=yellow>" + SkillName + "<color>"`; `!(cờ & 4)` → `"<color=blue>" + sprintf(G_ITEM_22 "[Cấp %d]", cấp của bản) + "<color>"`;
`(cờ & 7) ≠ 7` → `"\n"`; rồi **đệ quy `0x006FAA00(bản đó, buf, &đếm, 0)`** (ba nhóm thuộc tính của nó + các kỹ năng nó nêu tên; không in chi phí/phạm vi/addskilldamage của
nó — các dòng ấy chỉ ở `GetDescAboutLevel`).

Client mới: zone gửi `SkillDescLevel.related[]` `{skill_id, level, flags, attribs}` theo đúng thứ tự in (đệ quy tối đa 4 mức), `KUiSkillDesc.level_lines` in "Chiêu N:"
(đếm từ 1) / tên vàng / `[Cấp N]` xanh / thuộc tính; test zone "the skill tip names the skills of a level", client `test_skill_desc`. Nhị phân không chặn đệ quy vòng —
dữ liệu không có vòng.

Zone: `KSubWorld::skill_desc_request(sid, id, cấp)` → `G2C_SKILL_DESC {skill_id, has_cur, cur {level, cost, cost_type, attack_radius, attribs[{group 0/1/2,
name, v0, v1, v2}], related[{skill_id, level, flags, attribs}], appends[{skill_id, value}]}, has_next, next, max_level, level_inc, enhance, held_level}` từ `skill_of(id, cấp)` (cấp 0 = chưa giữ → chỉ cấp kế, như `GetDesc` JX1 với
`ulCurLevel == 0`); test `[command]` "the skill tip". Client: `KProtocolProcess.skill_desc_request / skill_desc / skill_text / skill_row / skill_name`,
`KUiSkillDesc.gd` (`build`, `level_lines`, test `test_skill_desc`), `KMagicDesc.describe_line` (bổ sung; `describe({type, value})` của vật phẩm giữ nguyên),
`KUiGameWindows._show_skill_tip` (sổ + cây; phần tĩnh hiện ngay, số liệu khi zone trả). `--auto` chụp `auto_skill_tip.png` → `AUTO_SKILL_TIP skill=14 answered=true cur_attribs=2 next=true lines=30 (auto_skill_tip.png: tên vàng, mô tả, Võ công lưu phái, Cấp hiện tại, tiêu hao/phạm vi/sát thương/6 dòng tăng kỹ năng, Hạn chế vũ khí, Đẳng cấp tiếp theo đỏ)`.

## 11. Đạn trên client — `KMissle::Paint` / `KMissleRes::Draw` (M12 lát B4c-3; sai khác có chủ ý: zone báo đạn)

Bản 2.0 (như JX1, `Core/Src/KMissle.cpp` biên dịch cho client) **không** nhận đạn từ server: client nhận gói thi triển 0x5a rồi tự chạy `CastMissles`
(cùng mã với server, `missles.txt` của client) để tạo/bay/vẽ đạn; server chỉ tính va chạm và sát thương. Client mới **không** mô phỏng lại: zone đã bay đạn
(`LINUX-SERVER.md` §13) nên zone báo cho các client quanh người bắn — `G2C_MISSLE` `MissleSync {index, missle_id, skill_id, level, launcher, x, y, z, dir,
x_factor, y_factor, speed, life_time, start_life_time, current_life, status, removed, move_kind, collided}` khi **sinh** (trạng thái chờ, cuối `missle_fire`), **khung bay
đầu tiên và mỗi 6 khung sau** (sau `missle_activate`), **va chạm** (`missle_process_collision` có ≥ 1 đòn trúng → cờ `collided`, đạn bay tiếp) và **tan**
(`activate_missles` trước `missle_remove`, cả khi rơi khỏi bản đồ). Giữa hai gói client
tự bay theo `KMissle::OnFly`: `pos += (factor · speed) / 1024` mỗi khung 18 fps; đường bay không thẳng (theo mục tiêu, vòng, quay về) lệch tối đa 6 khung.
Sai khác có chủ ý so với 2.0 (không đổi hình ảnh, chỉ đổi nguồn dữ liệu); chi phí: một gói ~60 byte mỗi 6 khung mỗi viên đạn.

**Dữ liệu vẽ** (`jxassets export-missle-res` → `client/assets/missles/missle_res.json` + sprite trong `sprites/`): `KMissle::Init` (`KMissle.cpp` 242..282) đọc
mỗi dòng `missles.txt` bốn trạng thái `MS_DoWait 0, MS_DoFly 1, MS_DoVanish 2, MS_DoCollision 3` (`SkillDef.h` 43): `AnimFile{i+1}`, `AnimFileInfo{i+1}` =
`"khung,hướng,nhịp"` (mặc định 100, 16, 1), `SndFile{i+1}`; bộ **B** (`AnimFileB*`) dùng thay bộ A với xác suất 1/2 khi `MultiShow` (`KMissle.cpp` 1704); `LoopPlay`,
`SubLoop`, `SubStart`, `SubStop`. Chỉ xuất các đạn mà kỹ năng dùng (`ChildSkillId` của 451 kỹ năng trong `skills.json` → 423 đạn, 674 lượt sprite); bộ xuất `Exporter.SpriteID` ghi atlas như
`export-npcres`. Bản zone `missles.json` cố ý bỏ các cột vẽ.

| Mã cũ | Làm gì | Client mới |
|---|---|---|
| `KMissle::Paint` (`KMissle.cpp` 1519) | `Map2Mps` → toạ độ; không gia tốc Z → `m_MissleRes.Draw(trạng thái, x, y, z = m_nCurrentMapZ, m_nDir, life − start, cur − start)`; có gia tốc Z → hướng từ vector (`g_GetDirIndex(0,0,XFactor,YFactor)` → `g_DirIndex2Dir(…, 64)` = chính chỉ số); khi `m_bHaveEnd` và mọi phim đặc biệt đã hết → xoá đạn | `KMissle.gd` (`_place`: `to_screen(pos) − (0, z)`; hướng từ gói, **đạn có Zacc quay theo vector** (B4c-6); hết phim tan → `gone` + `queue_free`) |
| `KMissleRes::Draw` (`KMissleRes.cpp` 113) | `MS_DoFly`: `cur < 0` hay `all ≠ 0 && all < cur` → không vẽ; khối hướng `dir / (64/n)`, làm tròn lên khi phần dư `≥ 32/n`, `≥ n` → 0; `perDir = frames/n`; `all == 0 → perDir`; `LoopPlay`: `(cur/nhịp) % perDir` hoặc `SubLoop` (`cur/nhịp < SubStart` → `cur/nhịp`; `SubStart == SubStop` → `SubStart`; else `SubStart + ((cur − SubStart)/nhịp) % (SubStop − SubStart)`); không lặp: `perDir · cur / all`; `> perDir − 1` → không vẽ; khung = `khốiHướng · perDir + khung`; `DrawPrimitives(RU_T_IMAGE)` với `RUIMAGE_RENDER_FLAG_REF_SPOT` (`KMissleRes.cpp` 38): bộ vẽ (`KRepresentShell2.cpp` 2004) trừ tâm `(CenterX, CenterY)` của spr, không tâm mà rộng > 160 → trừ (160, 192), rồi **cộng `OffsetX/OffsetY` của khung và vẽ từ góc trên trái** | `KMissleResMath.sprite_dir / fly_frame / frame_index` (test `test_missle_math`); `KMissle.gd`/`KMissleEffect.gd`: `Sprite2D.centered = false`, `position = −ref_spot + frame_offset` (**sửa 2026-09-18**: trước quên `centered = false` nên đạn vẽ lệch nửa khung — chủ dự án phát hiện "skill lệch với người chơi"; `--auto` in `AUTO_CAST_OPEN … m66:1@(3,-14)222x145` = tâm khung cách nhân vật 3 px) |
| `KMissle::CreateSpecialEffect` (`KMissle.cpp` 1998) | phim đặc biệt (`MS_DoVanish` khi tan/`ColVanish`, `MS_DoCollision` khi chạm mà không tan): `AnimFile[trạng thái]`, tại `(x, y − 5, z)`, bắt đầu = giờ game, kết thúc = + `nhịp · khung`, hướng `g_DirIndex2Dir(m_nDirIndex, n)`; không có phim → chỉ tiếng | `KMissleResMath.special_frame` (khung = elapsed / nhịp trong khối hướng, hết sau `nhịp · perDir`); phim tan `AnimFile3` do `KMissle.gd` phát khi zone báo tan; phim va chạm `AnimFile4` do `KMissleEffect.gd` phát tại điểm khi zone báo `collided` |
| `KMissle::DoVanish` (1765) / `DoCollision` (1792) | client: `m_bHaveEnd = TRUE`, `CreateSpecialEffect(MS_DoVanish)` khi `ColVanish`, else `MS_DoCollision` + tiếp tục bay | tan theo gói `removed`; va chạm theo gói `collided` (zone: `missle_process_collision` có ≥ 1 đòn trúng → `emit_missle(m, false, true)`) |

Zone: `KSubWorld::emit_missle` (`KSubWorld.cpp`), gọi trong `missle_fire`, `missle_frame` (sau `missle_activate`), `missle_process_collision`, `activate_missles`; log `missile sync`; test
`[missle]` "the clients hear a missile". Client: `KProtocolProcess.missle_sync / missle_row`, `scenes/KMissle.gd`, `scenes/KMissleResMath.gd`, `UiGame._on_missle`
(node trong lớp thực thể, y-sort), `KMissleEffect.gd` (phim va chạm), `--auto` in `AUTO_MISSLE packets=137 spawned=41 effects=7 live=0 (AUTO_FIGHT actions=4 sau khi chuyển bước chú thích kỹ năng lên trước cú thi triển)`. Chưa: tiếng (`SndFile*`), `RedLum/GreenLum/BlueLum/
LightRadius` (ánh sáng), `MissleHeight` khi bay (z chỉ lấy lúc sinh), gia tốc Z (`Zspeed/Zacc`) và hướng theo vector, đạn kiểu theo mục tiêu/vòng lệch tới
6 khung, bộ B cho phim va chạm (luôn bộ A).

**Trục Z (B4c-6)**: `KMissle::OnFlyFPS` (`KMissle.cpp` 1018, bản client chia một khung logic thành `nStep` bước vẽ) = `ZAxisMove` của server (`missle_activate`,
`0x080760E0`): mỗi khung `height += height_speed`, `< 0 → 0`, `map_z = height >> 10`, `height_speed −= z_acceleration`; đạn dạng 7 (parabol) sinh với
`height_speed = (khung − 1)·Zacc/2` (`0x080EC263`). Zone gửi thêm `height, height_speed, z_acceleration` trong `MissleSync`; client `KMissle.gd` chạy cùng luật giữa hai
gói (`KMissleResMath.z_step`, test `test_missle_math`) và, khi `Zacc ≠ 0`, quay mặt theo vector (`KMath.get_dir_index(0, 0, x_factor, y_factor)`, luật 2.0 `64 − k`).
**Bước con trong khung (đã đọc 2026-09-18, không cần port)**: bản 2.0 **không** còn `OnFlyFPS` theo khung vẽ (mã JX1 2004 gọi từ `GOI_PROCFRAME_POSSHIFT`,
`CoreShell.cpp` 6227 — bốn nơi gọi `0x006B2F00` đều nằm trong `KMissle::Activate` khung logic). Với đạn `MoveKind 1` (thẳng): `n = speed / [0x81f878]` (= **10**
đơn vị, `0x006B3549`) lần `OnFly(10, kiểm va chạm) 0x006B2F00` rồi `OnFly(speed % 10)` (`0x006B360D`) — chia bước chỉ để **kiểm va chạm phía client** (zone đã kiểm),
`KMissle::Paint 0x006B24E0` vẽ đúng ô hiện tại, không nội suy → đạn 2.0 đổi chỗ 18 lần/giây. **Chủ dự án (2026-09-18): bản dự án phải nội suy chuẩn FPS ngay từ đầu**
(HANDOVER §0.1 luật 13) → client mới làm như `KMissle::OnFlyFPS` của client JX1 2004 (`KMissle.cpp` 1014, gọi mỗi khung vẽ từ `GOI_PROCFRAME_POSSHIFT` với
`nStep` = số khung vẽ trong một khung logic: `speed/nStep` mỗi bước, phần dư rải; Z: `heightSpeed/nStep` mỗi bước, hết `nStep` bước mới trừ `Zacc`): `KMissle.gd`
`_process(delta)` đi phần `delta·18` của một khung logic (vector·speed/1024 và độ cao), `_tick` chỉ còn đếm tuổi/khung ảnh; đánh lùi của `KNpc.gd` cũng vẽ nội suy
giữa hai bước trượt của khung logic. `--auto`: `AUTO_MISSLE … smooth=11 fps=129` (một đạn đổi chỗ 11 khung vẽ trong một khung logic).

## 12. Đánh lùi trên client — gói `0x56` (`SendSyncAction`) → `KNpc::KnockBack` `0x005EE950` / `OnKnockBack` `0x005EFE00` (M12 lát B4c-4, đã đọc từng dòng)

**Server** (`jx_linux_y`): `KNpc::KnockBack 0x08087940` (`LINUX-SERVER.md` §16) kết thúc bằng **`SendSyncAction(this, 0x18, +0x14a0, +0x14a4, max(1, frames), 0)`
= `0x0807A970`**: gói **`0x56`** 22 byte `{byte 0x56, dword id npc (+1), byte doing (+5), dword x (+6), dword y (+10), dword frames (+14), dword a6 (+18)}`
→ `0x0807A870(this, gói, 0x16, 0x64, 0)` (người chơi quanh 100 ô). `OnHurt 0x0807F780` gửi cùng gói với doing 9: x/y = vị trí mình (Map2Mps `0x080EF710`),
frames = `+0x22c`, a6 = 0. Không có bù trễ nào ở server.

**Client 2.0**: bảng handler gói server→client là **mảng con trỏ `[obj + 4·(gói+1)]` điền trong hàm tạo `0x0065DCB0`** (`0x59 → +0x168 = 0x006500C0`,
`0x7b → +0x1f0 = 0x00651280`, **`0x56 → +0x15c = 0x00650250`**). `0x00650250`: tìm npc theo id (`0x0066C6C0` trên map `0x21a0438`, mảng npc `[0x1ab34f4]`
bước 0x12bd4) rồi **`KNpc::OnSyncAction 0x005F3300(this, trễ, doing, x, y, frames, a6)`** (stdcall 6 tham số; `trễ` = tham số thứ hai của handler = số khung
đã trôi từ lúc gói gửi), ghi `npc+0x16b8 = [0x1f178c4]+0x50` (mốc thời gian gói). `OnSyncAction`: `+0x2c == 1 && m_Doing (+0xfc) == 10 && doing ≠ 0x15` → bỏ
(xác người chơi chỉ nhận hồi sinh); bảng byte `0x5f3474` + bảng nhảy `0x5f3454` theo `doing − 1`: **1** đứng → `0x005EEE30`, `0x005ED9B0`, `0x005EB480`
(DoStand); **5** kỹ năng → đi tới điểm rồi thi triển (`0x00623860`, `0x005EB800`, `0x005F3050`); **8** ngồi → `0x005EF000`; **9** bị đánh →
**`DoHurt 0x005EEDB0(trễ, frames, x, y)`**: `m_Frames (+0x104) = frames − trễ` (`0x005E92E0`: tổng (≤ 0 → 1), khung 0, mốc `[0x9bb0f0]`), `SetDoing(9)
0x005EDDF0` (đang chạy 0x12 → tắt tiếng chạy `0x006DFA00` trên `+0x1a14`, trừ thưởng chạy `+0x13c4` khỏi `+0x1154`), `0x005ED9D0`, **`+0x100 = 7` (cdo_hurt)**,
`+0x1904/+0x1908 = x, y`, `+0x1900 = +0x34 > 0 ? +0x34 : 0`; **10** chết → `0x005EEBE0(x, 0)`; **21** hồi sinh → `0x005EEE30`, `0x005ED9B0`, `+0x1974 = 1`,
`0x005EB480`; **24 đánh lùi → `KnockBack 0x005EE950(trễ, frames, x, y)`**; doing 2, 3, 4, 6, 7, 11..20, 22, 23 → **bỏ qua** (`ret 0x18`).

**`KNpc::KnockBack 0x005EE950`**: vùng `+0x1054 < 0`, `m_Doing` 0x18 hay 0xa → thôi; `0x004C4B20(this, &cx, &cy)` = vị trí mình; đích ≠ mình →
**`+0x1390 = 0x005E8DB0(cx − x, cy − y)`** (hướng **từ đích về mình** = quay mặt về phía kẻ đánh; trùng chỗ → giữ hướng cũ); `SetDoing(0x18)`, `0x005ED9D0`,
`m_Frames = frames − trễ`, **`+0x100 = 7` (cdo_hurt: đánh lùi dùng hoạt ảnh bị đánh)**, **`+0x13b4/+0x13b8 = đích`** (cùng ô với điểm rơi khi nhảy).
**Mỗi khung** `0x005F1BF0`: `m_Doing` 4/0x14 giữ `+0x34` (độ cao), khác → 0; `+0x16a0 ≠ 0` → thôi; bảng nhảy `0x5f1c70` theo `m_Doing − 1`: 1 → `0x005ECD70`,
2 → `0x005F0BE0`, 3 → `0x005F14B0`, 4 → `0x005EF7F0`, 6/7 → `0x005EF2F0`, 8 → `0x005ECE10`, 9 → `0x005F07F0`, 10 → `0x005F07B0`, 14 → `0x005F0990`,
15 → `0x005F0A20`, 18 → `0x005F1B40`, 19 → `0x005EF950`, 20 → `0x005EFB60`, 21 → `0x005E9450`, 23 → `0x005EF730`, **24 → `OnKnockBack 0x005EFE00`**;
5, 11..13, 16, 17, 22 → không. **`OnKnockBack 0x005EFE00`**: `0x00620BE0(vùng, +0x13a0, +0x13a4, +0x13ac, +0x13b0, &cx, &cy)` (Map2Mps từ ô + phần
1/1024); `còn = max(1, tổng (+0x104) − khung (+0x108))`; **`dx = ((đích.x − cx) << 10) / còn`, `dy` như thế → `0x005ECEC0(this, dx, dy)`** (cộng phần
1/1024 vào `+0x13ac/+0x13b0`, sang ô khi tràn); `WaitForFrame 0x005EA700` hết → `DoStand 0x005EEE30` + `0x005ED9B0`. **`0x005E8DB0(dx, dy)`** (= `g_GetDirIndex`
của client): cả hai 0 → −1; `len = (int)sqrt`; `t = (dy << 10) / len`; tìm trong `g_nSin` (`[0x820088]`, 64 ô) ô cuối `i` (0..31) có `t ≤ sin[i]`, sang
`i + 1` khi gần hơn (so với `sin[i+1]`); `dx ≥ 0 && i ≠ 0 → 64 − i` — **cùng luật với server `0x080EEEC0`**, không phải JX1 (`63 − i`, bảng nửa bước).

**Client mới**: `G2C_ENTITY_ACTION` **`ACTION_KNOCK_BACK 6`** (`aim` = đích, `frames`, `target` = kẻ đẩy, `dir` của zone) do `KSubWorld::knock_back` phát thay
`ACTION_HURT`; `fill_info` (gói 0x4c) chỉ báo doing đánh lùi, không đích → người vào sau thấy loạng choạng tại chỗ (2.0 cũng chỉ có `m_Doing` trong 0x4c).
`KNpc.gd`: `apply_action` `ACTION_KNOCK_BACK` → hướng `KMath.get_dir_index(đích → mình)`, hoạt ảnh `HURT` `frames` khung; `_tick` mỗi khung **`KMath.knock_step`**
`= ((đích − mình) << 10) / còn` (1/1024, cắt về 0, `int()` của toạ độ như Map2Mps) rồi đếm khung như bị đánh, hết → đứng. **`KMath.get_dir_index` đổi sang
luật 2.0 = server** (`SIN64` 64 ô đúng bảng nhị phân, ô gần nhất, `64 − k`: phải = 48, lên = 32, lên-phải = 40; trước là JX1 `63 − k`). Không bù trễ (`trễ`
của 2.0 cần mốc thời gian trong gói — không có): client bắt đầu khi gói tới, gói `EntityMove` cuối đường của zone chốt vị trí. Test: zone `[autoskill][knockback]`
"a free way" kiểm gói; client `test_knock_back`, `test_npcres_tables` (hướng).

| Mã cũ / 2.0 | Client mới |
|---|---|
| `SendSyncAction 0x0807A970` gói 0x56 (`KnockBack 0x08087940`, `OnHurt 0x0807F780`) | `KSubWorld::emit_action` (`ACTION_KNOCK_BACK` kèm `aim`; `ACTION_HURT` kèm `pos`) |
| handler `0x00650250` → `OnSyncAction 0x005F3300` | `KProtocolProcess` `G2C_ENTITY_ACTION` → `entity_action` → `UiGame._on_action` → `KNpc.apply_action` |
| `KNpc::KnockBack 0x005EE950` | `KNpc.apply_action` nhánh `ACTION_KNOCK_BACK` |
| `OnKnockBack 0x005EFE00` + `0x005ECEC0` | `KNpc._tick` (`_knocked`) + `KMath.knock_step` |
| `0x005E8DB0` | `KMath.get_dir_index` |

## 13. Hào quang trên client — `SetRightSkill 0x005FB280` → `KNpc::SetAura 0x005EA870` → gói 0x6f; gói 0x85 `0x00652600` (M12 lát B4d-1, đã đọc từng dòng)

50 hào quang (`IsAura`) đều `LRSkill 2` — chỉ đặt được vào **chuột phải**. `SetRightSkill(id) 0x005FB280`: cấp đang giữ > 0 → `Player+0x34 = id`; bản `(id, 1)`
(`[0x1e59dc0 + id<<8]` hay `GetSkill`) → **`vtable+0x48 IsAura`** → `0x005EA870(npc của mình, IsAura ? id : 0)`; rồi `SetGameData(4, {0x40004, id}, −2, 0)`.
**`KNpc::SetAura 0x005EA870(id)`**: id 1..1999, cấp giữ > 0 (`0x006233B0(sổ +0x124, id, 1)`), `GetSkill(id, cấp)` `IsAura` (+0x48) → `npc+0x120 = id` (không → 0);
**gửi `{byte 0x6f, dword id}` 5 byte** qua `[0x9bd88c]->vtable+0x10(buf, 5)` (đối tượng mạng). Đặt kỹ năng phải không phải hào quang → gửi `{0x6f, 0}` = tắt.
Server: ô 111 `0x080DC460` (`LINUX-SERVER.md` §16.10). Gói **0x85** (server `0x080873B0`): handler `0x00652600` (`+0x218` của hàm tạo `0x0065DCB0`, ô 0x86):
`{+1 launcher (−1), +5 kỹ năng con, +9 id npc, +0xd id npc, +0x11 cấp, +0x15 cờ}` → `GetSkill(con, cấp)` → **`KSkill::Cast` phía client `0x006FACD0`** (tự tạo đạn để
vẽ) và `cờ == 1` → `0x005ED9F0(npc, con, cấp)` (đặt hồi chiêu `0x00623860`). Đạn của La Hán Trận (con 202 → đạn 92 "友好光环传递子弹") **không có ảnh**: hào quang chỉ
thấy qua biểu tượng (`StateSpecialId` 45, gói 0x7a) và hiệu ứng trạng thái trên npc (§14, B4e).

Client mới: `KUiGameWindows._on_skill_clicked(id, phải)` → `Game.set_aura(IsAura ? id : 0)` (`C2G_SET_AURA`); zone thi triển kỹ năng con → `G2C_MISSLE`; biểu tượng
6 ô → `G2C_STATE_ICONS` → `Game.entities[id].state_icons` (vẽ ở §14). `--auto`: `AUTO_AURA skill=16 child=202 packets=18 spawned=3 icons=[0,0,0,45,45,52] icon_ok=true
child_state=true` (45 hai lần: hào quang + trạng thái con cùng `StateSpecialId`, đúng như `0x08087160`).

| Mã cũ / 2.0 | Client mới |
|---|---|
| `SetRightSkill 0x005FB280` → `KNpc::SetAura 0x005EA870` → gói 0x6f | `KUiGameWindows._on_skill_clicked` → `KProtocolProcess.set_aura` |
| gói 0x85 → `0x00652600` → `KSkill::Cast` client | `G2C_MISSLE` (zone thi triển thật) |
| gói 0x7a (biểu tượng) | `G2C_STATE_ICONS` → `entities[id].state_icons`, tín hiệu `state_icons_changed` |

## 14. Hiệu ứng trạng thái trên npc — bảng `状态图形对照表.txt`, `KNpcRes::SetState 0x006DF7E0`, `KSprControl 0x0070B920`, `KNpcRes::Draw 0x006E0340`, `0x006DFAC0` (M12 lát B4e, đã đọc từng dòng `gamecl.exe` + mã 2004)

Mã 2004 đã có cơ chế này (`KNpcRes.h`: `KStateSpr m_cStateSpr[18]`, `CStateMagicTable` trong `KNpcResNode.cpp`, `KSprControl.cpp`); bản 2.0 giữ nguyên
thuật toán, đổi bảng (thêm cột `图分几瓣` split và loại `MiniMap`), còn **6 ô** thay vì 18, và chuyển ảnh loại Head sang hàm vẽ trên đầu.

**Bảng** `\settings\npcres\状态图形对照表.txt` (323 dòng: `Status1..Status322`, id = thứ tự dòng — loader `0x006AE200` (đối tượng `g_NpcResList+0x501c`) lưu theo
thứ tự dòng, getter `0x006AE540(id 1..count, tên, &loại, &không-lặp, &sau-đầu, &sau-cuối, &khung, &hướng, &nhịp, &split)`): cột 2 `FileName` (spr; **"Special"** = 5
dòng đầu 眩晕/中毒/冰冻/燃烧/混乱 — client tự vẽ, không kỹ năng nào trong `skills.txt` dùng), cột 3 `Head` 0 / `Foot` 2 / `MiniMap` 3 / khác → `Body` 1 (mã 2004:
`STATE_MAGIC_HEAD/BODY/FOOT`), cột 4 `Loop` → lặp (khác → chơi một lần), cột 5/6 khung "sau lưng nhân vật" [bắt đầu, kết thúc), cột 7 tổng khung (mặc 1), cột 8 hướng
(mặc 1), cột 9 **nhịp = số khung logic cho một lượt** (mặc 1), cột 10 split kẹp 1..3 (mọi dòng = 1). 170 id được `StateSpecialId` của `skills.txt` dùng (Foot 61, Body
40, Head 68, MiniMap 1 = Status77 `star2.spr` "小地图高亮显示"); 6 sprite không có trong pak 2.0 (68/69/70/161/203/221) → không vẽ.

**`KSprControl`** (0x84 byte: `+0 m_bChange, +4 m_nTotalFrame, +8 m_nCurFrame, +0xc m_nTotalDir, +0x10 m_nCurDir, +0x14 m_dwTimer, +0x18 m_dwInterval, +0x2c
m_szName[80], +0x7c m_dwNameID`; đồng hồ = `SubWorld[0].m_dwCurrentTime` `[0x1f178c4]+0x50`, tăng mỗi khung logic 18 Hz):
`SetSprFile 0x0070B580(tên, khung, hướng, nhịp)` (cùng tên → không đổi; hướng ≥ 1, khung ≥ hướng, `timer = now`); `SetCurDir64 0x0070B7C0(dir)`: khối =
`(dir + 32/hướng) / (64/hướng)` (≥ hướng → trừ hướng), đổi khối → `frame = khối·fpd`, `timer = now`, trả 0 (**bỏ bước khung ấy**), cùng khối trả 1;
`GetNextFrame 0x0070B920(lặp)`: `trôi = now − timer` (so **unsigned**); `trôi ≥ nhịp` → lặp: `timer = now, frame = khối·fpd`; không lặp: `frame = (khối+1)·fpd − 1`;
còn lại **`frame = khối·fpd + fpd·trôi/nhịp`** (fpd = khung/hướng); `CheckEnd 0x0070B880`: `frame == (khối+1)·fpd − 1`. Ví dụ La Hán Trận (Status45: 10 khung,
nhịp 8) chạy 10 khung trong 8 khung logic (0,44 s một vòng); 不动明王 (Status52 `c.spr`) 13 khung / 12; 眩晕 8 / 50.

**Ô trạng thái** (`KNpcRes` = `KNpc+0x1a14`, 6 ô tại `+0x15b0` bước 0xa0: `+0 id, +4 loại, +8 không-lặp, +0xc/+0x10 sau-lưng, +0x14 split, +0x18 đã nạp, +0x1c
KSprControl`): **`KNpcRes::SetState 0x006DF7E0(&KNpc+0x110 danh sách trạng thái, g_NpcResList)`** gọi mỗi `KNpc::Paint` (`0x005F37BF`; mã 2004 `KNpc.cpp:424`) — ô có
id không còn trong danh sách (`nút+0xcc` = `m_StateGraphics`, từ gói **0x7a** `KNpc::SetNpcState`) → xoá `0x006DDFB0`; id mới → getter, tên rỗng hay loại > 3 → bỏ;
ô trống đầu tiên nhận (id, loại, không-lặp, sau-lưng, split, nạp = 1, `SetSprFile`).

**Vẽ** (`KNpcRes::Draw 0x006E0340(dir, x, y, z = KNpc+0x34 m_nHeight, …)`, gọi từ `KNpc::Paint 0x005F2FAF`): (1) mỗi ô `CheckExist` → `SetCurDir64(dir)` (đổi hướng →
thôi) → lặp ? `GetNextFrame(1)` : `GetNextFrame(0)` rồi `CheckEnd` → **`+0x18 = 0`** (ảnh một lần chơi xong thì biến); (2) danh sách vẽ 1: bóng (z 0), **Foot** (`x +
lệch nhảy, y, z 0`), **Body có split 1 và khung ∈ [sau-đầu, sau-cuối)** (`z = m_nHeight + 38 nếu [KNpcRes+0x2c] cưỡi ngựa`) → `DrawPrimitives`; (3) các bộ phận
thân (z = m_nHeight); (4) **Body ngoài khoảng** + ảnh đặc biệt `+0x1a10`; **Head không vẽ ở đây** (mã 2004 vẽ ở đây với z = m_nHeight + 38 cưỡi ngựa). Mọi ảnh vẽ
REF_SPOT tại (x, y) chân: `màn = (x − tâm.x + lệch khung, y/2 − tâm.y + lệch khung − (z·887 >> 10))` (`KRepresentShell2::CoordinateTransform`).
Split 2/3 (`0x0070B900`) không có dòng nào dùng — không port.

**Head**: `KNpc::Paint 0x005F2FC4` → `0x005EB7C0(x + lệch nhảy, y, h)` với `h = 0x005F2DB0 (chiều cao khối tên: ebx 12 (14 khi là mục tiêu); edi 3 + 5 khi
tuỳ chọn bit `0x1eb4` bật; + ebx + 2 dòng tên; người chơi thêm danh hiệu 6, `+ebx+3` mỗi dòng bang/…) + GetNpcPate 0x005EBCF0 ([+0x3c] m_nStature + [+0x34]
m_nHeight, ngồi trừ dần, cưỡi +38)` → **`0x006DFAC0(x, y, h, [KNpc+0x40])`**: `h += 9`; (混乱 = 5 vẽ ba ảnh `+0x1b1c` và chữ, `h += 20`); ảnh đặc biệt `+0x1ac4` nếu
có; rồi mỗi ô loại 0 đã nạp → phần tử `(x, y, z = h − 100)` → `DrawPrimitives` → trả `h + 20`. `m_nStature` người chơi = **84** (`0x005EC13D`, không có mẫu npc
−1/−2), npc = cột `Stature` mẫu (`template+0x124`, `0x005EC0ED`). Mặc định tên hiện: khối = 3 + 5 + 12 + 2 = 22 → **z Head = pate + 22 + 9 − 100** (người chơi 15,
thú thử nghiệm Stature 100 → 31).

Client mới (`client/scenes/KSprControl.gd`, `KStateSpr.gd`, `KNpcRes.gd`, `KNpc.gd`; bảng `jxassets export-state-gfx` → `npcres/state_gfx.json` + sprite,
`NpcResList.state_gfx(id)`): `G2C_STATE_ICONS` → `UiGame._on_state_icons` → `KNpc.set_state_icons` → `KNpcRes.set_state_spr` (`KStateSpr.sync`, hàm tĩnh test
được) → mỗi tick 18 Hz `KNpcRes.paint(dir, …, head_z)`: `KStateSpr.step` (SetCurDir64/GetNextFrame/CheckEnd), đặt `Sprite2D` (`centered = false`) tại `−tâm + lệch
khung − (z·887 >> 10)` và xếp con: bóng, Foot, Body-sau, bộ phận, Body-trước, Head. Chưa: lệch x khi nhảy, +38 cưỡi ngựa, 5 dòng "Special", MiniMap, split 2/3.
`--auto`: `AUTO_AURA … pictures=[52:1@-119,-110 256x154,45:2@-42,-36 98x71 behind]` + `auto_aura_state.png` (vòng La Hán Trận dưới chân, ánh 不动明王 quanh thân).

| Mã cũ / 2.0 | Client mới |
|---|---|
| `CStateMagicTable::Init/GetInfo` (`KNpcResNode.cpp`), loader `0x006AE200` / getter `0x006AE540` | Go `pkg/jxold/npcres/CStateMagicTable.go` (`ParseStateMagicTable`), `jxassets export-state-gfx`, `NpcResList.state_gfx` |
| `KSprControl` (`KSprControl.cpp`; `0x0070B540..0x0070B9B4`) | `client/scenes/KSprControl.gd` |
| `KStateSpr` (`KNpcRes.h`), 6 ô `KNpcRes+0x15b0` | `client/scenes/KStateSpr.gd` (`sync`, `step`, `behind`) |
| `KNpcRes::SetState 0x006DF7E0`, `KNpc::SetNpcState` (gói 0x7a) | `KNpc.set_state_icons` → `KNpcRes.set_state_spr` |
| `KNpcRes::Draw 0x006E0340` (Foot/Body-sau/thân/Body-trước), `0x006DFAC0` (Head) | `KNpcRes._step_state_sprs` + `_reorder`, `KNpc._head_effect_z` |

## 15. Tiếng của kỹ năng và đạn — `KSkill::PlayCastSound 0x006F6D90`, `KMissleRes::PlaySound 0x00717ED0`, bộ âm thanh `0x1f9cf28` (M12 lát B4f-1, đã đọc từng dòng `gamecl.exe` + mã 2004)

Mã 2004 (`Engine/Src/KWavSound.cpp`, `KSoundCache.cpp`, `Core/Src/KMissleRes.cpp`, `KSkills.cpp:2971`) đặt tên; bản 2.0 giữ cùng luật, thêm một kiểm tra.

**Bộ âm thanh** (`0x1f9cf28`, `g_SoundCache` + `KWavSound` của engine): `Play 0x0064E600(tên, pan, âm lượng, lặp)` → `0x0064E150(tên)` (nút cache) →
`KWavSound::Play` (`[0x784b04]`): **3 buffer** một tệp (`BUFFER_COUNT`), lấy buffer đầu tiên không đang phát, hết thì **bỏ**; `IsPlaying 0x0064E220(tên)` = có buffer
đang phát; `Stop 0x0064E1C0(tên)` dừng buffer đang phát đầu tiên. Đơn vị DirectSound: âm lượng −10000..0 (phần trăm dB), pan −10000..10000.

**Luật âm lượng / pan** (cả hai chỗ giống nhau; tâm = `g_ScenePlace.GetFocusPosition 0x00671B00` (x, y Mps của tiêu điểm), `opt = [0x220e53c]` tuỳ chọn âm lượng):
`âm lượng = (10000 − (|dx| + |dy|)) · opt / 100 − 10000` (`KMissleRes::GetSndVolume 0x00717CC0` với `nVol = −(|dx|+|dy|)`), **`pan = dx · 5`**.

**Tiếng thi triển** — `KSkill+0x420 ManCastSnd[100]`, `+0x484 FMCastSnd[100]` (loader `0x006F699E/0x006F69BB`); **`KSkill::PlayCastSound 0x006F6D90(giới, x, y)`**:
giới ≠ 0 → FMCastSnd; gọi từ (a) `KNpc::DoSkill` đồng bộ `0x005EF90F` **chỉ khi `KNpc+0x192c > 0`** (`0x005EA901`: ghi 1..2 khi kind == 1 — tức người chơi; quái
không có tiếng thi triển), (b) `0x005F1E26` khi **npc là nhân vật mình** (`[0x1ab3584]+0xc0cc`) lúc bắt đầu chiêu (cùng chỗ đặt `PreCastSpr +0x3bc`). Không kiểm
IsPlaying → tiếng chồng được (3 buffer). Bảng `skills.txt` của client 2.0 (`slistcl.pak`) trùng cột `ManCastSnd/FMCastSnd` với bảng server (1629/1629).

**Tiếng đạn** — `KMissleRes` nằm tại `KMissle+0x1d8`, 8 bản ghi bước 0xd4 (`+0x14 AnimFile[0x64]`, `+0x78 khung`, `+0x7c nhịp`, `+0x80 hướng`, **`+0x84 SndFile[0x50]`**;
`SetRes 0x00717BD0(trạng thái, anim, snd)`; loader `KMissle::Init 0x006B1119` cột `SndFile%d`/`SndFileB%d`). **`KMissleRes::PlaySound 0x00717ED0(trạng thái, x, y, lặp)`**:
tên rỗng → thôi; **`IsPlaying(tên)` → thôi** (mã 2004 để chú thích; 2.0 bật: cùng tệp đang phát thì không phát lại); rồi Play(pan, âm lượng, lặp), `+0x6b8 = trạng
thái` (chỉ số tiếng cuối cho `StopSound 0x00717CF0`). Gọi: `KMissle::Activate 0x006B38C8`: `m_nCurrentLife == m_nStartLifeTime` và chưa tan → `PrePareFly 0x006B2A90`
→ **`PlaySound(1 = MS_DoFly, x, y, 0)`** (`0x006B393D`) → `DoFly`; `CreateSpecialEffect 0x006B1DC0(trạng thái, x, y, z, npc)`: **AnimFile rỗng → về ngay (không
tiếng)** (mã 2004 phát trước khi kiểm), trùng npc → về, thêm nút rồi **`PlaySound(trạng thái, x, y, 0)`** (`0x006B2044`) — tan (2) tại `0x006B38BA`, va chạm (3) từ
`DoCollision`. Không nơi nào truyền lặp = 1.

Client mới (`client/scenes/KWavSound.gd` — `snd_volume`/`snd_pan` tĩnh, `play(tệp, vị trí, lặp, unless_playing)`, `is_playing`, `stop`, 3 `AudioStreamPlayer2D` một tệp;
`Assets.sound(đường dẫn)` = `KSoundCache`; `jxassets export-sounds` → `client/assets/sounds/<id>.wav` + `sounds.json` (69 tệp; 3 thiếu trong pak: 刀剑刺中声 (đánh thường
chạm), 护体寒冰, 补充\投石爆炸)): `UiGame._on_action` (ACTION_ATTACK có `skill`, npc kiểu người chơi → `cast_sound(id, giới)` — không kiểm IsPlaying), `KMissle.gd`
(`_on_fly_start` khi sang bay: SndFile2; `_begin_vanish`: SndFile3 chỉ khi có AnimFile3), `UiGame._on_missle` va chạm: SndFile4 chỉ khi có AnimFile4 — ba chỗ đạn đều
`unless_playing`. Âm lượng theo luật cũ (`volume_db = âm lượng / 100`, tuỳ chọn 100); **pan là pan của Godot** (tuyến tính theo x màn hình, Godot không có gain từng
kênh) — sai khác có chủ ý. Tiếng hành động của nhân vật: §15.1. Chưa: tiếng nền/nhạc (`0x006AC670`), tiếng giao diện
(`0x0066BC50`), tiếng vật thể cảnh (`0x00669A50`), tuỳ chọn âm lượng. `--auto`: `AUTO_SOUNDS ["不动明王咒.wav", "sound_k003.wav", "行龙不雨.wav", "sound_k003.wav", "行龙不雨.wav"]`.

| Mã cũ / 2.0 | Client mới |
|---|---|
| `KSoundCache` (`g_SoundCache`), bộ `0x1f9cf28` `0x0064E150` | `Assets.sound` (`KPakFile.gd`), `jxassets export-sounds` |
| `KWavSound::Play/IsPlaying/Stop` (`0x0064E600/0x0064E220/0x0064E1C0`) | `KWavSound.gd` `play/is_playing/stop` |
| `KMissleRes::GetSndVolume 0x00717CC0`, pan `dx·5` | `KWavSound.snd_volume/snd_pan` |
| `KSkill::PlayCastSound 0x006F6D90` (từ `0x005EF90F`/`0x005F1E26`) | `UiGame._on_action` + `cast_sound` |
| `KMissleRes::PlaySound 0x00717ED0` (từ `Activate 0x006B393D`, `CreateSpecialEffect 0x006B2044`) | `KMissle._on_fly_start/_begin_vanish`, `UiGame._on_missle` |

### 15.1 Tiếng hành động của nhân vật — `KNpcRes::PlaySound 0x006DFA20`, bảng `主角动作声音表.txt` / `npc动作声音表.txt` (M12 lát B4f-2, đã đọc từng dòng)

**Bảng** (tên tệp trong bảng tệp npcres của client `0x0081F01B` / `0x0081F017`, nằm trong `reslst.dat`; mã 2004 `KNpcResNode::Init` `PLAYER_SOUND_FILE`/`NPC_SOUND_FILE`):
`主角动作声音表.txt` 47 dòng × 3 cột (`ActionName`, `MainMan`, `MainLady`) — hàng = tên hành động của `人物类型.txt`, ô = tên tệp (`FreeWalk`/`NormalWalk`… `sound_m39`,
`*Run` `sound_m40`, `*Wound` `m01`/`m20`, `*Die` `m02`/`m21`, `FreeAttack` `m03`/`m22`, `MeleeWPuncture` `m04`/`m23`, `MeleeWCut` `m05`/`m24`, …, `*Magic` `m11`/`m30`,
`Ride*` `m13..m19`/`m32..m38`); `npc动作声音表.txt` 448 dòng × 15 cột (`NpcList`, `FightStand`, `NormalStand1`, `NormalStand2`, `FightWalk`, `NormalWalk`, `FightRun`,
`NormalRun`, `Wound`, `Die`, `Attack1`, `Attack2`, `Magic`, `SitDown`, `JunpFly`) — hàng = tên tài nguyên npc (141/447 có tiếng: thú `sound_aNNN_{pst,bat,die,at}`, quái
`sound_eNNN_{bat,die,at}`, dân `sound_cNNN_pst`). Đường dẫn = `ComposePathAndName("sound", tên)` = `\sound\<tên>` (`sound.pak`). Trong pak 2.0 thiếu 39 tệp bảng nêu
(mọi `_bat`/`_pst` của thú `a0xx`, vài `e0xx_bat`, `sound_m11/m17/m30/m36`) — client thật cũng câm ở đó.

**Luật** — `KNpcRes+0x34 m_szSoundName` (`GetSoundName 0x006DDD10` = `KNpcResNode::GetActionSoundName(action)` khi đổi action); `KNpcRes::Draw 0x006E0340`: tỉ lệ tiến
= `(now_ms − KNpc+0x10c) / (KNpc+0x104 · 1000/18)` (`0x006E038E..0x006E0408`, kẹp 0..1; **1000/18: hằng `0x38e38e39 sar 2` = /18**) → **`< 0.05`** (`[0x7b4788]`, `0x006E06E5`)
→ `PlaySound 0x006DFA20(x, y)`: tên rỗng → thôi; **`IsPlaying` → thôi**; âm lượng/pan như §15 (`0x006DDD30`), không lặp. `KNpc::WaitForFrame 0x005EA700`: khung vượt
tổng → `+0x108 = 0`, **`+0x10c = now`** → mỗi vòng lặp của đi/chạy/đứng phát lại (bước chân mỗi chu kỳ, nếu tiếng trước đã dứt). `KNpc+0x104` = khung 18 Hz (cũng là
gốc của nội suy vẽ). Quái và người chơi đều phát (không điều kiện kind); thi triển kỹ năng của người chơi: action `*Magic` (`sound_m11`, thiếu trong pak) + tiếng
ManCastSnd (§15).

Client mới: `jxassets export-sounds` đọc thêm hai bảng (`pkg/jxold/npcres/KActionSound.go`: `ParsePlayerSoundTable`, `ParseNpcSoundTable`, `SoundPath`) → `npcres/action_sounds.json`
(`player.MainMan/MainLady`, `npc.<res>` cho các res đã xuất) + tệp; `NpcResList.action_name/action_sound`; `KNpcRes.set_action` đặt `sound_name`; `KNpc._play_action_sound`
(`cur_frame·20 < total_frame`, gọi khi bắt đầu action `_set_doing/_set_action` và mỗi tick sau `paint` — khung 0 sau khi vòng lặp về 0) → `KWavSound.play(…, unless_playing)`.
`--auto`: `AUTO_SOUNDS ["sound_k003.wav", "行龙不雨.wav", "sound_m03.wav", …, "sound_a009_at.wav", "sound_a013_at.wav", …, "sound_a009_die.wav", "sound_m02.wav"]`
(thi triển, đạn bay, đánh thường `FreeAttack`, thú tấn công, heo chết, nhân vật chết), `sounds=27 dropped=8` (8 lần IsPlaying chặn).

| Mã cũ / 2.0 | Client mới |
|---|---|
| `KNpcResNode::Init` đọc `PLAYER_SOUND_FILE`/`NPC_SOUND_FILE`, `GetActionSoundName`, `ComposePathAndName` | `pkg/jxold/npcres/KActionSound.go`, `npcres/action_sounds.json`, `NpcResList.action_sound` |
| `KNpcRes::GetSoundName 0x006DDD10` (`+0x34`) | `KNpcRes.sound_name` (đặt trong `set_action`) |
| `KNpcRes::Draw 0x006E06E5` (tỉ lệ < 0.05) → `PlaySound 0x006DFA20`; `WaitForFrame 0x005EA700` | `KNpc._play_action_sound` |


## 16. Quái vàng trên client — bộ nạp `0x006E35C0`, `KNpcGold` `KNpc+0x4c` (`SetGoldType 0x006E3560`), màu tên `0x005F23E5`, gói 0x9a `0x00653110`, lọc treo máy `0x00642550`, con trỏ `0x0069D25D` (M12 lát B5a, đã đọc từng dòng `gamecl.exe`)

Luật ở `LINUX-SERVER.md` §16.12. Client 2.0 chỉ nhận **một word** "loại vàng" (dòng `NpcGoldTemplate.txt` + 1; boss = số dòng bảng server + 1) và tô màu tên theo nó.

| Địa chỉ | Đã đọc | Client mới |
|---|---|---|
| `0x006E35C0` (gọi từ `0x005CB2F0`) | nạp `\settings\npc\NpcGoldTemplate.txt` của client (trong `reslst.dat`, **17 dòng**: thêm Cổ Thụ với kỹ năng "BUFF tăng ích Kích Cổ Thụ") vào 30 bản ghi 0x7c (`0x21a0438`), số dòng `+0xe88 = [0x21a12c0]`; cùng thứ tự cột như server (col 14 → `0x00702DD0` tên → id), dòng có 类型 rỗng dừng; **không lưu tên** — client không hiện tên loại | `npc_gold.json` `client_rows` (17) — `NpcResList.gold_rows()` |
| `KNpcGold` `KNpc+0x4c` | `SetGoldType(word) 0x006E3560`: `w > 0 → +0xc = w − 1, +8 = +4 = 1`, không thì `+4 = +8 = 0`; `GetGoldKind 0x006E3540` = `+4 && +8 ? +0xc + 1 : 0`; gọi từ handler gói **0x4c** `0x0065C070` (`movzx word [gói+0x11]`, `0x0065C31A`; bố cục gói: +3 camp, +4 hệ, +5 máu·128/max, +6 x, +0xa doing, +0xb camp gốc, +0xc y, +0x11 word vàng, +0x13 kind, +0x14 id, +0x18 −1, +0x1c word cấp, +0x1e word mẫu, +0x20 tên) và handler gói **0x9a** `0x00653110` (`{0x9a, id npc +1, word +5}`; chỉ khi `+0x2c` (kind) == 0) | `KNpc.gold_type` từ `EntityInfo.gold_type` / `G2C_NPC_GOLD` (`Game.gold_changed` → `UiGame._on_gold` → `set_gold_type`) |
| Vẽ tên `0x005F21B0(x, y, h, cờ hiện, cỡ chữ 0xe/0xc, màu nền)` | kind 1/2 → nhánh người chơi (màu theo `+0xf8`, danh hiệu `<%s>%s`); kind 3 (`0x005F2283`) → chỉ tên `+0x1409` màu −1; khác (quái): `+0x40 == 0` hay cờ hiện == 0 → **không vẽ gì**; vẽ dòng máu `"%d/%d"` (`+0x105c/+0x12b14`, `0x005F2358`) rồi dòng **`"%s/Lv:%d"`** (`+0x1409`, `+0x28`; `0x005F242F`) với màu: `GetGoldKind == 0 → −1` (trắng); ≠ 0 → `kind > [0x21a12c0] ? 0xFFEBB200 : 0xFF6365FF` (`0x005F23FF..0x005F2419`, `setg/sbb`). Với dữ liệu thật: boss = 16 + 1 = 17, client có 17 dòng → 17 > 17 sai → **boss cũng màu 0xFF6365FF** (tím xanh), chỉ word ≥ 18 mới vàng cam. Cờ hiện: `0x006702BD` tuỳ chọn 1 (`0x0066B600(1)`, F7 hiện tên) → 1 cỡ 14; tuỳ chọn 2 → 1 cỡ 12; không → 0 (quái không tên) | `KNpcGold.gd` `name_text` ("%s/Lv:%d" cho `ENTITY_MONSTER`), `name_color` (trắng / `6365ff` / `ebb200` theo `client_rows`); `KNpc._refresh_name`. Dòng máu `"%d/%d"` và tuỳ chọn F7/F8: §17 (B5c) |
| `0x00642550(npc)` (từ `0x00643281`) | lớp = `GoldKind == 0 ? 1 : (kind > số dòng ? 3 : 2)`; duyệt 8 mục `[core+0x8181 + i·0x24]` (bật) so `[core+0x81ac + i·0x24]` với lớp hoặc `hệ + 4` → bộ lọc mục tiêu treo máy (đánh quái thường/vàng/boss/theo hệ) | `KNpcGold.npc_class`, `KNpc.npc_class()` (bộ lọc treo máy chưa có) |
| `0x0069D25D` / `0x0069D4D5` / `0x0069D5B3` | con trỏ chuột trên npc: kind 1 → 0x10, 2 → 0x11, khác: `GoldKind ≠ 0 ? 0xf : 0xe` | chưa (client chưa đổi con trỏ) |

`--auto`: `AUTO_GOLD rows=17 npc1 kind=7 row=Kim class=2 color=6365ff life=160/160 …` + `auto_gold.png` (`JX_ZONE__TEST_NPC_GOLD=7`).


## 17. Khối tên/máu trên đầu npc — vòng vẽ `0x00670130` (`0x0067021A` npc dưới chuột, `0x00670243` thanh máu, `0x006702BD` khối chữ), công tắc F7/F8 (`Switch([[showplayername]]/[[showplayerlife]])` `0x0042FBF7/0x0042FC2D`, từ tuỳ chọn `[0x21a0438+0x1eb4]` `0x0066B4C0..0x0066B6F9`), `KNpc::PaintLife 0x005EACF0` (M12 lát B5c, đã đọc từng dòng `gamecl.exe`)

| Địa chỉ | Đã đọc | Client mới |
|---|---|---|
| Từ tuỳ chọn `+0x1eb4` (đối tượng `0x21a0438`, ctor `0x0066DACC` đặt **1**) | bốn giá trị 2 bit: A bit 0/0x100 (`0x0066B4E0`), **B "showplayername" bit 2/0x200** (`0x0066B4C0`; getter theo mặt nạ `0x0066B600(1)` = bit 2, `(2)` = bit 0x200; setter `0x0066B5C0(v)`), **C "showplayerlife" bit 4/0x400** (`0x0066B500`, `0x0066B660`, `0x0066B620`), D bit 8/0x800 (`0x0066B520`). `GetGameData(0x402, p)` `0x006620B7`: p 1 → B, p 2 → `0x0066B4A0(B)` = **B ≤ 1 → 3, không thì 0**, khác → `0x0066B600(1)`; `0x403` `0x00662116` y hệt cho C. Lua `Switch` (`0x0042FA60`, bảng tên `0x80dbc8`: run/sit/trade/pk/horse/showplayername/showplayerlife/showplayermana/showplayernumber): `showplayername` → `OperationRequest(0x2f, 0, GetGameData(0x402, 2))` (`0x005C3936` → setter B), `showplayerlife` → `0x30` → setter C: **F7/F8 lật 0 ↔ 3**. Cửa sổ treo máy `0x005E6E8E`: cờ `[esi+0xa]` → B = C = 3, `[esi+0xb]` → 0/0. Khi mở game: từ = 1 → **B = C = 0** (không tên quái, không thanh máu) — client mới **mở sẵn tên** (`name_switch = 3`), máu tắt: lựa chọn có ghi | `KNpcGold.gd` `toggle_switch`; `KNpc.name_switch/life_switch` (static); `UiGame` F7/F8 → `set_show_switches` |
| Vòng vẽ `0x00670130` mỗi npc (sau `0x005EA920` = npc được vẽ) | `[esp+0x10]` = npc này là `[core+0xa8c4]` (npc dưới chuột); **thanh máu** `0x00670243`: kind 1/2 → `0x0066B660(1)` → `PaintLife(x, y, h, force 0)`; kind 0 → dưới chuột **hoặc** `0x0066B660(2)` → `PaintLife(…, force 1)`; kind khác → không. **Khối chữ** `0x006702BD`: `0x0066B600(1)` tắt **và** kind 0 → không vẽ gì (kể cả dưới chuột); dưới chuột → `PaintName(x, y, h, hiện 1, cỡ 0xe, viền 0xff000000)`; không thì `0x0066B600(2)` → `(hiện 1, cỡ 0xc, viền 0)`; không → `(hiện 0, 0xc, 0)` (tham số cuối = `BorderColor` của `iRepresentShell::OutputText(nFontId, psText, nCount, x, y, Color, nLineWidth, nZ, BorderColor)` — **viền chữ**, không phải nền; đính chính sau khi chủ dự án thấy khối đen) (quái: `0x005F2316` thôi; người/npc: vẫn tên) | `KNpcGold.name_block(kind, B, chuột \|\| mục tiêu)` → 0 / 12 / 14; `KNpcGold.life_bar(kind, C, …)`; `KNpc.hovered` từ `UiGame._set_hovered` (`InputEventMouseMotion` → `_entity_at`) |
| `KNpc::PaintLife 0x005EACF0(x, y, h, force)` | `!force && kind ∉ {1, 2}` → thôi (không có điều kiện "bị thương" như mã 2004); `max(+0x12b14, +0x12b18) ≤ 0` → thôi; nhảy (`+0xfc == 8`, `+0x104 ≠ 0`) dời y `±9·+0x108/+0x104` theo `+0x1390`; `pct = round(+0x105c · 100 / max)` (`0x5b0cc0`); màu (`0x005EADB8..0x005EAE5B`): kind 1/2 khác mình và `0x0066D070(mình, npc) == 8` (cùng đội) → (230, 190, 0); `+0x16e8 ≠ 0` → (255, 0, `+0x16e4 == 2` ? 0 : 64); `+0x16e4 == 2` → (255, 105, 180); còn lại theo pct: ≥ 50 xanh lá (0, 255, 0), ≥ 25 vàng (255, 255, 0), dưới đỏ (255, 0, 0); hình: phần đầy rộng `pct·38/100` từ `x − 19`, cao 3 (`0x005EAE64..0x005EAEBB`, `DrawPrimitives` vtable+0x4c kiểu 1), phần còn lại tới `x + 19` xám (128, 128, 128) (`0x005EAEC1..0x005EAEF4`). `+0x16e4` = byte `+0x1f & 3`, `+0x16e8` = byte `+0x1e & 1` của gói đồng bộ người chơi (`0x0065D29F/0x0065D5B1`: trạng thái PK — chưa port) | `KNpc._draw`: 38×3, `KNpcGold.life_bar_color(pct)` + xám; cùng đội/PK: chưa |
| `PaintName 0x005F21B0` quái với hiện 1 | dòng máu `"%d/%d"` (`+0x105c/+0x12b14`, màu −1) rồi `"%s/Lv:%d"` (màu §16); mỗi dòng đo bề rộng (`0x005F22A2`, `0x005F2445`) để căn giữa | hai `Label` (`_life_label`, `_label`) cỡ 12/14, viền đen 2 px khi 14 (`outline_size`), rộng theo chữ |

| `PaintName 0x005F2507` người chơi (kind 1/2) | màu theo `+0xf8` (camp hiện tại, byte +3 của gói 0x4c; bảng nhảy `0x5f2d94`): 0 begin `0xFFFFFFFF`, 1 justice `0xFFFFA85E`, 2 evil `0xFFFF92FF`, 3 balance `0xFF55FF91`, 4 free `0xFFFF0000`, > 4 `0xFFFF69B4`; rồi `+0x40 ≠ 0` và chuỗi `0x21a03b0` → `<%s>%s` (danh hiệu; chính mình `0x21a0390` → `<%s%s>%s` với `0x00653AB0`), các dòng bang/… `+0xb0/+0xa0/+0xc0` (`0x9bf4c8`) — **chưa port** (chưa có danh hiệu/bang) | `KNpcGold.player_name_color(current_camp)`; `EntityInfo.camp/current_camp` (+0xb/+3 của gói 0x4c), `G2C_ENTITY_CAMP` → `KNpc.set_camp` |

`--auto` `auto_fight.png`: mục tiêu npc3 "78/160" + "npc3/Lv:10" cỡ 14 viền đen, các quái khác cỡ 12, thanh máu chỉ dưới mục tiêu (C = 0).

## 18. Ngồi thiền trên client — `Switch([[sit]])` `0x0044B470` → `OperationRequest(6, 2)` `0x005C37A6`, gói 0x71 `0x0067D080`, gói 0x83 / 0x9f → `KNpc::DoAction(8) 0x005EA2E0`, `GetNpcPate 0x005EBCF0` (M12 lát B6a, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| nút ngồi của thanh công cụ `0x0044B470` | Lua `Switch([[sit]])` → `0x0042FA60` (bảng tên `0x80dbc8`: 0 run, **1 sit**, 2 trade, 3 pk, 4 horse, rồi `showplayer*`) → `OperationRequest(6, 2, 0)` (`0x005C1CF0`; `run` là `(6, 1, 0)`) | `UiControlBar` lệnh `"sit"` → `KUiGameWindows._on_bar_command` |
| `0x005C37A6..0x005C37FC` (thao tác 6, tham số 2) | npc mình `[0x1ab34f4] + [core+0xc0cc]·0x12bd4`; **`+0x19c0 ≠ 0` (cưỡi) → hộp thoại `0x005C3801`**, không gửi; `m_Doing (+0xfc) == 8` → `KNpc::DoAction(1, 0, 0, 0)` **tại chỗ** (đứng ngay, không chờ server) rồi `0x0067D080(0)`; khác → `0x005F7F60(core+0xa878, 1, 1)` (`+0x44 \|= 2`, cờ chờ ngồi) rồi `0x0067D080(1)` | `Game.sit(not sitting)` (`sitting` = `doing == ACTION_SIT` của mình); client mới **chờ `ACTION_STAND` của zone** thay vì đứng trước |
| `0x0067D080(ngồi)` | `[core+0xa8a0] ≠ 0` → thôi; gói **`{0x71, byte ngồi ≠ 0}`** 2 byte qua `[0x9bd88c]->vtable+0x10` | `KProtocolProcess.sit(on)` → `SitReq{sit, seq}` `C2G_SIT` (gateway chuyển tiếp `session.go`) |
| gói 0x83 `0x00650400` (`{0x83, dword npc}`) | npc → `KNpc::DoAction(8, 0, 0, 0) 0x005EA2E0`: `m_Doing = 8`, hành động ngồi (cột 12 của bảng doing → hành động **36**, `MA_*_019_ZZ01.spr` 72 khung / 8 hướng, vũ khí `*_000_ZZ01`) chạy hết khung rồi **giữ khung cuối** | `G2C_ENTITY_ACTION ACTION_SIT` → `KNpc._set_action(Doing.SIT, SIT_FRAME 15)`; `_tick` giữ khung cuối; gói đồng bộ (`doing` 8) cho người vào sau → khung cuối ngay; đi / đánh / bị đánh ghi đè |
| gói 0x9f giá trị 6 | chính mình `DoAction(8)` (sau khi server nhận) | cùng một `ACTION_SIT` (zone phát cho cả mình) |
| `KNpc::GetNpcPate 0x005EBCF0` (mã 2004 `KNpc.cpp` 6139) | `pate = m_nStature (+0x3c, người chơi 84) + m_nHeight (+0x34)`; mẫu −1/−2 (nhân vật): `m_Doing == 8` **và** `MulDiv(10, +0x108 cur, +0x104 total) ≥ 8` **và** `+0x13f4 (m_ArmorType) ≠ 45` → `pate −= MulDiv(30, cur, total)` (15 khung: 24 / 26 / 28 ở ba khung cuối, giữ 28); `+0x19c0` (cưỡi) → `+ 38`. (Mã 2004 không có điều kiện áo 45; `MulDiv` làm tròn nửa lên) | `KNpcGold.sit_pate_drop`, `KNpc._pate` + `_place_labels` mỗi khung logic (tên và hiệu ứng đầu hạ theo khung, lên lại khi đứng); áo 45 chưa có trên client mới |
| sprite ngồi trong gói xuất | — | `jxassets export-npcres` thêm `npcres.DoSit` vào `Doings` (8 phần × ZZ01) — chạy lại `python tools/dev.py assets`, không thì người ngồi **biến mất** (không có sprite) |

`--auto` `auto_sit.png` (sau trận): `AUTO_SIT sat=true frame=14 life_before=596 life_after=596 stood=true` — ngồi khoanh chân, giữ khung 14, tên hạ 28 điểm, đứng dậy
khi gửi 0x71 = 0 (máu đầy nên không thấy hồi; luật hồi có test `[sit]` của zone).

## 19. Dáng trang bị và ngựa trên client — gói 0x4a `0x0065D180` / 0x4b `0x0065D460` / 0xad `0x006515A0` → `KNpc::SetPlayerRes 0x005ED920` → `0x005EBF90`, `0x005F1A15..0x005F1A63` (`KNpcRes::SetRideHorse 0x006DF420`, `SetArmor 0x006E1070`, `SetHelm 0x006DECF0`, `SetMantle 0x006DEDB0`, `SetHorse 0x006DEFB0`, vũ khí `0x006DF080/0x006DF140`) (M12 lát B6b, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| `KNpc` ô dáng | `+0x13ec` mẫu (−1 nam / −2 nữ), `+0x13f0` mũ, `+0x13f4` áo (`m_ArmorType`), `+0x13f8` áo choàng, `+0x13fc` ngựa (−1 không), `+0x1400` vũ khí, `+0x1404` vũ khí 2 (≠ −1 → `+0x1666` = 1: dùng tay kia), `+0x19c0` cưỡi, `+0x1408` phiên bản dáng | `KNpc.equip_rows` {0 mũ, 1 áo, 2 vũ khí, 3 ngựa, 4 áo choàng}, `riding` |
| handler 0x4a `0x0065D180` (`SyncPlayer` 2004) | id `+0x10`; `+0x12b3c..+0x12b48` (dword `+4/+8/+0x16/+0x1a`), chạy `+0xc` → `+0x1154`, đi `+3` → `+0x1150`, **mũ `+0xe` → `+0x13f0`, áo `+0xd` → `+0x13f4`, ngựa `+0x14` (int8) → `+0x13fc`, áo choàng `+0x1e` → `+0x13f8`**, `+0x2c = 1` (người chơi), `+0xf` → `+0x38`, `+0x1f` → `+0x1430`, dword `+0x25` → `+0x1910`, cờ `+0x15`: `&3` → `+0x16e4` (PK, sự kiện 0x15 khi đổi), `&4` → chiến `0x005F3290`, `&8` → `+0x18fc` + `KNpcRes 0x006DDE10`, `>>4 &1` → `+0x190c`, **`>>5 &1` → `KNpc::SetRideHorse 0x005EC3E0`**, `0x40/0x80` → `+0x1914`; word `+0x23` → hạng `0x0047CD60` → `+0x15f0` (0x64 byte); word `+1 > 0x2a` → tên bang `0x005EC810(gói+0x2b)`. Không có vũ khí trong 0x4a | `EntityInfo.helm_res/armor_res/weapon_res/horse_res/mantle_res` + `riding` (bộ `_res_dict`) |
| handler 0x4b `0x0065D460` | id `+1`; phiên bản `+0xe` ≠ `+0x1408` → `0x0067CBA0(id)` xin đồng bộ đầy đủ rồi thôi; người khác: `+5/+9/+0x12/+0x16` → `+0x12b3c..`, chạy `+0xd`, đi `+0x11`; **mũ `+0xf`, ngựa `+0x10` (int8), áo `+0x1a`, áo choàng `+0x1b`, vũ khí `+0x1c` (int8) → `+0x1400`, vũ khí 2 `+0x1d` → `+0x1404`**, dword `+0x24` → `+0x1910`, cờ `+0x1e` (bit 1 → `+0x16e8`, 2/4/8 → `+0x191c` 0/1/2, mình → `[core+0x149ec]`), `+0x1f &3` → `+0x16e4`, `+0x20/+0x21` → `+0x1664/+0x1665` … | `G2C_ENTITY_RES` (phiên bản trong `EntityRes.version`; client mới không xin lại) |
| handler 0xad `0x006515A0` | `{0xad, dword npc +1, byte +5, byte +6, dword +7 mẫu, 6 word +0xb.., byte +0x17}` → struct (`0x005E9180`) → **`KNpc::SetPlayerRes 0x005ED920`**: `+4 == 0` → `0x005EBF90(…)` rồi `+0x13e0 = 0` (dáng thật), `== 1` → `0x005EBF90` (mượn dáng); `+0x1408 = byte +0x17` | `KNpc.set_equip_rows(rows)` → `KNpcRes.set_equips` |
| `0x005EBF90(npc, p1, p2, p3, mẫu, mũ, áo, áo choàng, ngựa, vũ khí, vũ khí 2)` | `+0x13dc/+0x13e4/+0x13e8 = p1..p3`, `+0x13e0 = 1`, `+0x13ec = mẫu`; mẫu ≠ −1/−2 → npc mẫu (`[0xa38bc0 + mẫu·0x10e0]`, tạo `0x12c` byte nếu chưa có), năm ô 0/−1, `+0x3c` = `mẫu+0x124`; mẫu −1/−2 → **`+0x13f0 = mũ, +0x13f4 = áo, +0x13f8 = áo choàng, +0x13fc = ngựa, +0x1400 = vũ khí, +0x1404 = vũ khí 2`**, `+0x3c = 0x54` | `equip_rows` |
| `0x005F19F1..0x005F1A63` (dựng `KNpcRes` `+0x1a14` khi vẽ/khởi tạo) | `Init(tên +0x1560)`; `SetAction(+0x100)`; **`SetRideHorse(+0x19c0, byte +0x1666)` `0x006DF420`**; `SetArmor(+0x13f4)`; `SetHelm(+0x13f0)`; `SetMantle(+0x13f8)`; `SetHorse(+0x13fc)`; `+0x1666 == 0` → `0x006DF140(−1)` + `0x006DF080(+0x1400)`; `== 1` → tay kia với `+0x1404` | `KNpcRes.set_equips/set_ride`: hàng theo nhóm, `act_no(res, doing, weapon, ride)` chọn lại hành động (bảng `on_horse`), nạp lại ảnh |
| `KNpcRes::SetRideHorse 0x006DF420(ride, ?)` | `[res+0x103d4]` (node) rỗng → 0; `+0x2c == ride` → thôi; `+0x2c = ride`; `+4 = 0x006AF390(+0 doing, +0x1c vũ khí, ride)` = `GetActNo`; `[res] == 8` → cờ; `0x006AF430(node+0x40c0, act, …)` lấy khung/ảnh của hành động mới | `set_ride` → `_repick` |
| mặc/cởi của chính mình `0x00610FE0..0x006110C0` (`0x0060FE30` = dòng dáng) / `0x00614000..` | theo `detail` (0..15, bảng `0x6110d8`): mũ → `+0x13f0`, áo → `+0x13f4`, áo choàng → `+0x13f8`, ngựa → `+0x13fc`, vũ khí → `+0x1400`; cởi: ngựa/vũ khí = −1, áo `0x006E42F0(0,0,0)`, áo choàng `0x006E4280` (trơn) — client tự tính dáng của **mình** từ `g_ItemChangeRes 0x24f2200`, người khác chờ gói | client mới lấy cả của mình từ zone (một đường) |
| `GetNpcPate 0x005EBCF0` | `+0x19c0 ≠ 0` → **`+ 38`** (`0x005EBD58`) | `KNpc._pate` `riding → +38` |
| sprite | hành động 38..47 của bảng `on_horse` (đứng 38, đứng 2 47, đi 39, chạy 40, đánh 41/42, thi triển 43, bị đánh 44, chết 45; ngồi/nhảy −1); phần ngựa 12 `HorseFront` (`MA_HH_%03d_RD01`), 13 `HorseMiddle` (`MA_HB_`), 14 `HorseBack` (`MA_HT_`) theo hàng ngựa (+1 trong tên tệp: hàng 8 = `009`), 112 khung / 8 hướng | `jxassets export-npcres -equip-rows "2:1,2;3:8"` (mặc định): hàng vũ khí 1/2 (kiếm cấp 1..5 của `MeleeRes`) + ngựa hàng 8 (mọi "普通马" của `HorseRes.txt` cột 2 = 10) và các hành động trên ngựa của mọi hàng đã xuất; hàng chưa xuất → client giữ hàng mặc định (`KNpcRes._row_known`) thay vì mất thân; `-equip-rows all` = mọi hàng năm bảng `*Res.txt` nêu (`ItemChangeRes.AllRows`: mũ 0..39/131, áo 0..65/131, vũ khí 0..61, ngựa 0..55 — hàng trăm sprite mỗi phần, chỉ khi cần) |

`--auto` `auto_ride.png`: `?gm ds AddItem(0,10,0,1,0,0)` → mặc → `AUTO_RIDE mounted=true horse_row=8 action=38 parts=10 down=true action_down=1 parts_down=7`
(7 phần thân + 3 phần ngựa, tên cao thêm 38; xuống ngựa về hành động 1 với 7 phần). Túi của nhân vật thử được dọn (`bag cleaned`) trước `AUTO_ITEMS` vì nhân vật giữ đồ giữa các lần chạy.

## 20. PK trên client — `Switch([[pk]])` `0x0042FB3D` → `OperationRequest(0x14)` `0x005C3271` → `0x005FB240` (gói `{0x6d, 2}`), gói 0x4a/0x4b cờ `& 3` → `KNpc+0x16e4`, `PaintLife 0x005EADB8` (M12 lát B3c-4, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| `autoexec.lua` | `F9` và `Ctrl+H` → `Switch([[pk]])`; `V` → `Switch([[sit]])`; `R` run, `M` horse, `T`/`O` trade | `UiGame`: F9 xoay 0 → 1 → 2 → 0 (`C2G_PK_STATE`), V ngồi/đứng |
| `Switch([[pk]])` `0x0042FB3D` | `OperationRequest(0x14, 0, 1)` → `0x005C3271`: `0x005F7C10(core+0xa878)` (`+0x6f98 == 3`, đang trong game) → `[core+0x11a28] == 0` → `0x005FB240(0)` (không gửi gì) / ≠ 0 → `0x005FB240(1)`: gói **`{0x6d, 2}`** 2 byte | jx_linux_y đọc 0x6d là giao dịch (`0x080B2C70`), handler trạng thái PK của nó là gói 0x76 `{byte}` → zone dùng ý nghĩa 0x76 |
| gói 0x4a `0x0065D282` / 0x4b `0x0065D617` | byte cờ `& 3` → `+0x16e4` (trạng thái PK), khác trước → sự kiện 0x15 cho UI (`0x005C66A0`); 0x4b `+0x1e & 1` → `+0x16e8` (cờ PK bật) | `EntityInfo.pk_state`, `G2C_ENTITY_PK` → `KNpc.pk_state` (`+0x16e8` = trạng thái ≠ 0) |
| gói 0x90 / 0x93 (handler ô 0x91/0x94 của bảng) | trạng thái / giá trị PK của mình → HUD | `G2C_PK_STATE{state, value, refused}` → `Game.pk_state/pk_value`, HUD `pk <tên>/<giá trị>`, dòng chat khi bị từ chối |
| `PaintLife 0x005EADB8..0x005EAE5B` | kind 1/2 không phải mình và `0x0066D070 == 8` (cùng đội) → (230, 190, 0); khác: `+0x16e8 ≠ 0` → R 255, G 0, `+0x16e4 == 2` → B 0 (đỏ) khác B 64; `+0x16e8 == 0 && +0x16e4 == 2` → (255, 105, 180); còn lại theo % (§17) | `KNpcGold.life_bar_color(pct, pk_state, pk_flag)`; đội chưa có |

`--auto` `auto_pk.png` + `AUTO_PK on=true bar_state=1 value=0 back=false refused=true` (đang chiến → về 0 cần `NormalPKTimeLong`).

## 21. Thông điệp 0x86 và chuỗi của tổ đội trên client — handler `0x00657AD0` (bảng nhảy `0x658674`, 0x38 mã), bộ nạp chuỗi `0x005DBA60` (`\lang\<vn|zh|tw>\stringtable_core.txt`), khoá `MSG_TEAM_*` (M14 lát T1, đã đọc từng dòng `gamecl.exe`)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| gói 0x86 `{0x86, word dài (4), word id, [dword]}` → ô `+0x21c` = `0x00657AD0` | `id − 1` ngoài 0..0x37 → bỏ; bảng nhảy `0x658674`; mỗi mã lấy chuỗi từ biến toàn cục (`0x9bf828..0x9c0220`) rồi `sprintf` với tên/số ở `+5` → khung thông báo | `TeamEvent{event = TEAM_EV_MSG, arg = id}` → `UiTeam` (T2) tra khoá dưới đây |
| bộ nạp chuỗi `0x005DBA60` (gọi từ `0x005CAB3A` với `\lang\%s\stringtable_core.txt`, `%s` = `zh/tw/vn` theo `[0x80ec60 + 4·ngôn ngữ]`, mặc `?`) | `KIniFile 0x9c0670`: `push 1; push "KHOÁ"; mov [G_trước], eax; call GetString 0x004105e0` — **kết quả của KHOÁ được ghi ở lệnh `mov [G], eax` kế tiếp** (lệch một); 1169 khoá `G_*/L_*/MSG_*` | `jxassets` chưa xuất; T2 xuất `stringtable_core.txt` (VN, mã TCVN3 như bảng kỹ năng) → `text/strings.json` |
| mã → khoá (`msgkeys2.py`) | 1 (tên), 2 `MSG_TEAM_DISMISS_CAPTAIN`, 3 `MSG_TEAM_LEAVE_SELF_MSG`, 4 (tên), 5 `MSG_TEAM_SELF_ADD`, 6 `MSG_TEAM_CHANGE_CAPTAIN_FAIL1 + FAIL2` (`%s không đủ sức thống soái`), 7 `FAIL1 + FAIL3` (`không thể chuyển giao cho người mới`), 8 `MSG_OBJ_CANNOT_PICKUP`, 9 `MSG_OBJ_TOO_FAR`, 0xa `MSG_DEC_MONEY`, 0xb/0xc `MSG_TRADE_*_ROOM_FULL`, 0xd `MSG_TRADE_REFUSE_APPLY`, 0xe `MSG_TRADE_TASK_ITEM`, 0x10 `MSG_ITEM_DAMAGED`, 0x11 `MSG_MONEY_CANNOT_PICKUP`, 0x12/0x13 `MSG_TEAM_TARGET_CANNOT_ADD_TEAM`, 0x14..0x1a `MSG_PK_ERROR_1..7`, 0x1d `G_PLAYERTONG_3`, 0x1e..0x22 `MSG_TONG_*`, 0x23 `MSG_PK_ERROR_8`, **0x24..0x28 `MSG_TEAM_ERROR01..05`** ("Nhóm trưởng do hệ thống chỉ định, không thể tăng thêm thành viên nhóm / đuổi người khỏi nhóm / đảm nhận tân nhóm trưởng / mời người khác gia nhập nhóm / mở rộng nhóm"), 0x29 `G_ProtocolProcess_4`, 0x2a `MSG_ITEM_USINGTIMES_END`, 0x2b..0x38 `G_ProtocolProcess_19..31` | |
| chuỗi tổ đội khác trong `stringtable_core.txt` | `MSG_TEAM_SEND_INVITE` "Bạn đã gửi lời mời tổ đội đến %s!", `MSG_TEAM_GET_INVITE` "%s mời tổ đội!", `MSG_TEAM_REFUSE_INVITE` "%s từ chối lời mời tổ đội của bạn!", `MSG_TEAM_CREATE` "Bạn sáng lập nên một nhóm mới", `MSG_TEAM_CREATE_FAIL`, `MSG_TEAM_CANNOT_CREATE`, `MSG_TEAM_OPEN/CLOSE`, `MSG_TEAM_ADD_MEMBER` "%s đã trở thành đồng đội của bạn.", `MSG_TEAM_SELF_ADD` "Đã vào đội của %s.", `MSG_TEAM_DISMISS_CAPTAIN/MEMBER`, `MSG_TEAM_KICK_ONE` "%s bị khai trừ khỏi đội!", `MSG_TEAM_BE_KICKEN`, `MSG_TEAM_APPLY_ADD` "%s xin vào đội!", `MSG_TEAM_APPLY_ADD_SELF_MSG`, `MSG_TEAM_LEAVE` "%s rời đội.", `MSG_TEAM_LEAVE_SELF_MSG`, `MSG_TEAM_CHANGE_CAPTAIN` "%s được bổ nhiệm làm Đội trưởng!", `MSG_TEAM_CHANGE_CAPTAIN_SELF`, `MSG_TEAM_AUTO_CAPTAIN(_SELF)` "Bạn được hệ thống chỉ định là nhóm trưởng!", `MSG_TEAM_CANT_INVITE`, `MSG_TEAM_(NOT_)AUTO_REFUSE_INVITE`, `G_STR_TEAMSHAREITEM_MODEPUNISH1/2` (chia đồ) | `UiTeam` (T2) |

### 21.1 Cửa sổ Tổ Đội `KUiTeamManage` (`队伍管理.ini` của theme `\Ui\ui3_1024`), gói 0x53 phía client, handler 0x69 `0x006516D0`, hộp `KUiInformation` `提示.ini` (M14 lát T2, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| `KUiTeamManage::OpenWindow 0x004AE880` (đối tượng `[0x845aac]`, 0x965c byte) | tạo → `0x004ADAF0` nạp `%s\队伍管理.ini` → `0x004AD930` gắn ô: `Main`, `LeaderAbility` (+0x584), `InputEdit` (+0xb1c), `MemberList` (+0x1004), `NearbyList` (+0x14ec), `MemberScroll`, `NearbyScroll`, `Invite` (+0x2eb4), `Kick` (+0x3854), `Appoint` (+0x41f4), `Leave` (+0x4b94), `Dismiss` (+0x5534), `Refresh` (+0x6874), `CloseTeam` (+0x5ed4), `Cancel` (+0x7214), `txtTitle/txtLeaderAbility/txtSearch/txtOurTeam/txtAroundPlayer`; rồi `0x004AE300` cập nhật, mở, Lua `\script\ui\freshbubble.lua OpenTeamWnd`. Theme 1024 (`\Ui\ui3_1024\队伍管理.ini` trong `slistcl.pak`): Main 404×253 tại (230,170) trên `组队界面底板.spr`, nút chữ `小按钮四字.spr` nhãn "Mời vào / Rời đội (Kick) / Chuyển / Tạo mới (Refresh) / Rời đội / Giải tán đội", ô `带勾按钮.spr` "Đóng tổ đội", chữ "Tổ Đội / Tài lãnh đạo / Tìm kiếm / Đội mình / Lân cận" | `jxassets export-ui` → `ui/to-doi`; `UiTeam.gd` (`KUiGameWindows.team_window`, lệnh thanh công cụ `team`) |
| cập nhật `0x004AE300` | `GetGameData(0x3fc)` → `[esi] = core+0x11aac` (**cấp thống lĩnh** client tự tính từ kinh nghiệm thống lĩnh `+0x7230` bằng bảng `\settings\npc\player\level_lead_exp.txt` của client `0x006E2270`), `+4 = core+0x11aa8`, `+8 = core+0xc084`) → `LeaderAbility` `SetText("%d")` `0x00468BD0`; `OperationRequest(0, &trạng thái)` 12 byte (`+0x957c` đội trưởng?, `+0x957d` số người, `+0x957f` mở); đội trưởng → ô `CloseTeam = +0x957f`, `Dismiss` hiện/bật, `Leave` ẩn; khác → `Leave` hiện (bật khi `+0x957d > 0`), `Dismiss` ẩn; `n = +0x957d` → `OperationRequest(1, buf, n)` điền n mục 0xf0 byte (tên +0, id npc +0x40, chỉ số player +0x44) → `MemberList`; rồi `Refresh` | `UiTeam.refresh()` từ `Game.team` (`TeamSelf`: `lead_level` của zone) |
| `Refresh 0x004ADE50` | xoá `NearbyList`; `GetGameData(0x3fa, 0, 0)` = số người → cấp phát → `GetGameData(0x3fa, buf, n)`: `0x0066B180` duyệt npc bản đồ: kind 1, không phải mình, **camp ≠ 0 bị bỏ khi camp mình == 0**, `+0x1054 ≥ 0`, không phải đội trưởng/đội viên (`0x1f17610/0x1f17614..`) | `UiTeam.refresh_nearby()` từ `Game.entities` |
| nút (`WndProc 0x004AE0D0`, thông điệp 0x565) | `Invite 0x004ADE00`: chọn ở `NearbyList` (`+0x14b4`); chưa trong đội (`+0x957d == 0`) → `OperationRequest(5)` lập đội; `OperationRequest(7, mục, 0)`. `Kick 0x004ADDD0`: `(8, mục MemberList)`. `Appoint 0x004ADDA0`: `(6, mục)`. `Leave/Dismiss 0x004ADEF0`: `(9, 0, 0)`. `CloseTeam`: `(0xa, 0, checked)`. `Cancel`: `0x004ADF20` huỷ cửa sổ. Chọn mục (0x691): `MemberList → 0x004ADBD0` (so tên với tên mình → bật `Kick/Appoint` khi không phải mình), `NearbyList → 0x004ADC70` (bật `Invite` khi có mục và (chưa đội hay đội trưởng)). 0x693 (nhấp đúp): menu tên `0x00475690(0, tên, 1, 0x2f)` | `_on_invite/_on_kick/_on_appoint/_on_leave/_on_close_team`, `_refresh_buttons` |
| `OperationRequest` (`0x005B9560`, bảng nhảy `0x5b9844`, this = `core+0xa878`) | 5 → `0x005F6F40` lập (npc camp 6 → thôi; `0x0061B440`); 6 → `0x005F7100(npc)` **gói `{0x53, word 7, 8, dword npc}`** nhường chức; 7 → mời: `Npc[mình]+0xf8 == 0 && Npc[mục]+0xf8 ≠ 0` (camp) → thông điệp `[0x9bff8c]`; không thì `0x005F75B0(npc)` + `[0x9bff88]` (`MSG_TEAM_SEND_INVITE`) qua `0x006083D0(0x1f)`; 8 → `0x005F70B0(npc)` `{0x53, 7, 7, npc}` đuổi; 9 → `0x005F7070` `{0x53, 7, 6, 0}` rời; 0xa → `0x005FA7C0(mở)`; 0xc → `0x005F6F70(npc)` nhận đơn (xoá khỏi danh sách đơn `+0x726c` 32×0x2c) | **cùng số lệnh con với `jx_linux_y`** (khác PK) → `Game.team_request(cmd, npc, cờ)` = `C2G_TEAM` |
| handler 0x69 `0x006516D0` (ô `+0x1a8`; bảng nhảy `0x6518ac` theo lệnh con 1..0xe, kiểm độ dài) | 1 (0x27) `0x005F8270` `s2c_teaminfo`; 2 (0x14c) `0x005F8280` **`s2c_teamselfinfo`**: `+0x7258 = 1`, `+0x725c` = 0 nếu đội trưởng là mình, `0x1f17610` đội trưởng, `0x1f17614..` 7 id, tên `+0x25/+0x45..`, cấp `+0x12d..` → `0x1f17638..`, id đội `+0x125 → 0x1f17758`, **kinh nghiệm thống lĩnh `+0x129 → +0x7230`** → cấp `+0x7234` (`0x006E2270`), số người `+0x180c` (`0x006E2300`); 3 (4) `0x005F8420`; 4 (8) `0x005F8430` lập xong; 5 (4) `0x005F8570` lập hỏng; 6 (4) `0x005F8620` mở/đóng; **7 (7) `0x00603780` đơn xin** (đội trưởng: `sprintf(MSG_TEAM_APPLY_ADD "%s xin vào đội!", tên)`, hộp `0x005B8150(0x1f, …)`); 8 (0x2b) `0x005F86F0` thêm người; 9 (7) `0x005F87E0` rời; 0xa/0xb (0xb) `0x005F8890/0x005F8A80`; **0xc (0x27) `0x00604630` lời mời**; 0xd (0xb) `0x005F8AF0` nhường chức; 0xe (9) `0x005F8CF0` | `G2C_TEAM_SELF` → `Game.team` + `team_changed`; `G2C_TEAM_EVENT` → `KUiGameWindows._on_team_event` |
| `KUiInformation` (`提示.ini`: `Main` 433×120 tại (182,180) `对话条2.spr`, `Info` nhiều dòng, `FirstBtn`/`SecondBtn` nút chữ y = 91) + `UiSysMsgCentre` 2004 (`SMCT_UI_TEAM_INVITE/APPLY`: "同意"/"拒绝" → `TeamOperation(INVITE_RESPONSE/APPLY_RESPONSE, player, nSelAction == 0)`) — 2.0: chuỗi `G_SysMsgCentre_0` "%s mời bạn vào đội", `G_SysMsgCentre_1` "%s xin vào đội của bạn", nút `G_ACCEPT_WORD` "Đồng ý" / `G_REFUSE_WORD` "Từ chối" (`stringtable_client.txt`, nạp `0x0041120D/0x0041224B`) | `jxassets export-ui` → `ui/hop-thoai`; `UiInformation.gd` (`show_box(text, nút 1, nút 2, tham số)` → `answered(0/1)`); lời mời → `TEAM_REPLY_INVITE{captain, cờ}`, đơn → `TEAM_ACCEPT` khi đồng ý |
| `PaintLife 0x005EADB8` (`0x005EADD5`) | kind 1/2 không phải mình và `0x0066D070(npc) == 8` (cùng đội) → **(230, 190, 0)** trước mọi màu PK | `KNpcGold.life_bar_color(pct, pk_state, pk_flag, team_mate)`, `KNpc.set_team_mate` (từ `Game.is_team_mate` khi `team_changed`/spawn) |

`--auto`: `AUTO_TEAM created=true captain=true open=1 lead_level=1 members_max=3 window=true closed=true dismissed=true changes=3`, `auto_team.png` (cửa sổ với "Auto20834", "Tài lãnh đạo 1", chat "Bạn sáng lập nên một nhóm mới"). Chưa: `队伍一览信息.ini` / `teamoverview\*` (xem đội quanh, `s2c_teaminfo`), menu tên 0x693, `InputEdit` (2.0 không đọc), "Người mới không mời được người đội khác" (`[0x9bff8c]` = `MSG_TEAM_CANT_INVITE`).

## 22. Giao dịch trên client — `KUiTrade` (`玩家间交易.ini`, `0x004C02D2`), `Switch([[trade]])` `0x005C3236`, menu người chơi `0x004C2450` (`G_UIGAME_*`, `Ctrl+RButton`), bảng rao trên đầu (`界面状态与图形对照表.txt`, `KNpc+0x1cb4`), hộp xin giao dịch `G_SysMsgCentre_3` (M14 lát G2, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Hàm | Luật (đã đọc) | Client mới |
|---|---|---|
| `autoexec.lua` (`\ui\autoexec.lua`) | `T`/`O` → `Switch([[trade]])`; `P` → `Open([[team]])`; `Ctrl+RButton` → `Mouse_Menu()` (menu người chơi); `Ctrl+LButton` → `Mouse_Say()`; `Alt+LButton` → `Mouse_PartnerAction()`; `Alt+RButton` → `Mouse_Emote_Menu()` | `KUiGameWindows`: T/O `toggle_trade_sign`, P cửa sổ đội; `UiGame`: Ctrl+chuột phải lên người chơi → `open_player_menu` |
| `Switch([[trade]])` `0x005C3236` | trạng thái menu của npc mình (`0x005EB2B0`) == 2 → `TradeApplyClose 0x005F7460` (gói 0x6a); khác → `TradeApplyOpen 0x005FB010(câu, dài)` (thanh 2.0 không đưa câu) | `TRADE_APPLY_CLOSE` / `TRADE_APPLY_OPEN{text}` |
| **menu người chơi** `0x004C2450(KUiPlayerItem*)` (`+0x44` chỉ số npc, `+0x48` trạng thái menu đích, `+0x40` cờ) | 18 mục `G_UIGAME_0..17` (`0x821f7c..`): **2 "Giao Dịch"** khi hai cờ `[esp+0x14]/[esp+0x18]` (`GetGameData 0xbbd`: mình/đích đang giao dịch?) = 0, đích có chỉ số, **đích trạng thái 2**; **3 "Nhập đội"** khi … **đích trạng thái 1** và `0x5c0520(2, npc) == 0`; **4 "Tổ đội"** (mời) khi `+0x40`, `OperationRequest(0)` > 0 (đội trưởng/không đội) …; 1 "Hảo Hữu" khi chưa là bạn (`0x49f440`); 5 "Theo sau" khi có chỉ số; 6 "Thông tin" khi `+0x40`; 7 "Bang" …; 0xa "Tên truy nhập" …; mục chọn → `WND_M_MENUITEM_SELECTED` → 2004 `ProcessPeople(ACTION_TRADE → TradeApplyStart nếu đích TRADEOPEN, ACTION_JOINTEAM → ApplyAddTeam nếu TEAMOPEN)` | `KWndPopupMenu.gd` (hộp đơn giản, chưa lấy sprite menu 2.0) với 2 / 3 / 4 theo bảng rao của đích và đội của mình → `TRADE_APPLY_START` (+ dòng `MSG_TRADE_SEND_APPLY`), `TEAM_APPLY_ADD`, `TEAM_CREATE`+`TEAM_INVITE` |
| `TradeApplyStart 0x005F7480(npc idx)` (vtable CoreShell `0x005B8C10`) | gói **`{0x6b, dword npc}`** 5 byte (không có dword mã chống bot mà `jx_linux_y` đọc — lệch phiên bản như PK); rồi `sprintf([0x9c0120] = MSG_TRADE_SEND_APPLY, tên)` + hộp | |
| gói 0x8b (`0x652750`) / UiSysMsgCentre 2004 `SMCT_UI_TRADE_APPLY` | hộp "%s mong muốn giao dịch với bạn" (`G_SysMsgCentre_3`), `G_ACCEPT_WORD`/`G_REFUSE_WORD` → `TeamOperation`-kiểu `TradeReplyStart(người xin, đồng ý)` | `UiInformation.show_box(..., {"kind": "trade", id})` → `TRADE_REPLY{target, 1/0}`; dòng chat `MSG_TRADE_GET_APPLY` |
| **`KUiTrade`** (`OpenWindow 0x004C0D69(KUiPlayerItem* đối tác)`, `Init 0x004BF000`, `WndProc 0x004C0AB0`) — `\Ui\ui3_1024\玩家间交易.ini`: `Main` 461×404 tại (186,100) `玩家交易底板_JX20.spr`; `TakewithItemsBox` (+0x10b4) túi 6×10 + `TakewithMoney` (+0x584); `SelfItemsBox` (+0x15ac) **8×4** + `SelfMoney` (+0x1aa4, MaxLen 9) + `AddMoney` (+0x419c) / `ReduceMoney` (+0x490c) `SendHoldMsg`; `OtherItemsBox` (+0x650c) 8×4 + `OtherName` (+0x5f74) + `OtherMoney` (+0x6a04); `OkBtn` (+0x24bc, `CheckBox`, "Khoá"), `TradeBtn` (+0x37fc, `CheckBox`, "Xác nhận"), `CancelBtn` (+0x2e5c, "Huỷ giao dịch"); `InfoText` (+0x7534: `WaitTradeMsgColor`/`LockMsgColor`/`UnlockMsgColor`); `*BindGold` (kim đĩnh = phòng 4) | `WndProc`: 0x565 `OkBtn → 0x004C0400(checked)` = `TradeOperation(0x15, 0, checked)` (khoá); `TradeBtn → 0x004C0020` = `TradeOperation(0x16)` (xác nhận, sau ba kiểm `0x1770`); `CancelBtn → 0x004C0080` (huỷ); `AddMoney/ReduceMoney → 0x004BFE20(1/0)`: **±1** mỗi nhấn giữa `+0x5f5c` (tiền đặt) và `+0x5f6c` (tiền còn), giữ nút (0x568) lặp; 0x62d (ô tiền đổi) → `0x004BFEB0` + `0x004BFFB0` gửi tiền; 0x513 (ô đồ) → `0x48f570` kéo đồ với mã phòng 5 (`SelfItemsBox`) / 3 (`TakewithItemsBox`) | `UiTrade.gd` (`giao-dich` từ `export-ui`): ba hộp đồ (túi / bàn mình = phòng 2 / bàn đối tác từ `Game.trade.other_items`), nhấp đồ túi → `item_move` vào ô trống đầu 8×4, nhấp bàn → về túi; tiền: ô nhập + ±1 → `TRADE_MONEY`; Khoá → `TRADE_DECISION 2` (không mở lại), Xác nhận (bật khi cả hai khoá) → `1`, Huỷ → `0`; `InfoText`: `G_STR_OTHER_NOT_OK` "Chờ đối phương khóa" / `G_STR_OTHER_OK` "Đối phương đã khóa" / `G_STR_WAIT_TRADING` "Chờ đợi"; mở khi `Game.trade.state == 2`, đóng khi khác; dòng chat `MSG_TRADE_SUCCESS/FAIL` (0x78) |
| **gói 0x4c** `0x0065C070` (đồng bộ npc đầy đủ khi vào tầm nhìn; bố cục ở dòng `KNpcGold` §16) | `+0x10` byte trạng thái menu, `+0x20` byte cờ (bit0 PK → `+0x40`, bit1 chiến đấu → `+0x44`, bit2 ngủ → `0x005F3290`, bit3 bang mở → `+0x48`; chỉ npc khác), `+0x21` byte `== 1` → `+0x138c` (vẽ với cờ `0x10a` ở `0x005F37FA`), **`+0x22` tên** (chuỗi NUL), sau tên còn `size − 0x22 − len(tên)` byte → **câu rao** → `KNpc::SetMenuState(trạng thái, câu, dài) 0x005EB2A0` (`0x0065C4ED`); không còn → `SetMenuState(trạng thái, 0, 0)`. (`jx_linux_y` `0x0807FDD8` ghi `+0x20` = số lần trùng sinh, `+0x21` = `" "`, câu từ `+0x23` — lệch một byte cờ so với client 2.0, cùng kiểu lệch phiên bản như gói 0x6b; bố cục ở dòng `KNpcGold` phía trên ghi "+0x20 tên" theo bản server) | `EntityInfo.menu_state/menu_sentence` (`fill_info`) → `_entity_dict` → `KNpc.setup` — người vào tầm nhìn thấy bảng rao đang treo (không chỉ lúc đổi) |
| gói 0x76 `0x006522F0` (`s2c_npcsetmenustate`) | npc mình: trạng thái 2 → chuỗi `[0x9c00f8]`, 5/6 → hộp 0x2c/0x2b; npc khác → `KNpc::SetMenuState 0x005EB2A0 → 0x006DDD70`: `+0x1cb4` trạng thái (≤ 6, bản cũ `+0x1cb8`), câu `+0x1cc0` (≤ 0x1ff), nạp sprite `0x006DDC00` từ `KPlayerMenuStateGraph 0x24f... ` (`0x007042F0`: bảng 7 hàng × 0x50) | `G2C_ENTITY_MENU_STATE` → `entities[id].menu_state/menu_sentence` → `KNpc.set_menu_state` |
| **`\settings\npcres\界面状态与图形对照表.txt`** (`KPlayerMenuStateGraph::Init 0x00703F40`, nạp ở `0x00704102`) | 1 đội mở `\spr\npcres\style\menustate01.spr`, 2 giao dịch mở `menustate02`, 3 đang giao dịch `menustate03`, 4 ngủ `menustate04`, 5 bày sạp `02`, 6 sạp đang bán `03`; `KNpcRes::Draw 0x006DFD83`: trạng thái ≠ 5 và có tên → sprite (`+0x1a98`, `KSprControl 0x0070B920`) là **ảnh phụ đầu tiên** (`+0x1f10`) tại toạ độ npc với z lệch `+0x14` sau ảnh trước; câu (`0x006DFBD3`, ≤ 24 ký tự, rộng `len·6`, tối đa theo `[0x1ab3574]`) vẽ ở `−0x1e` | `jxassets export-menu-state` → `npcres/menu_state.json` + 4 sprite; `KNpc._refresh_sign`: sprite ngay trên khối tên, câu (`Label` 12 px, màu 255,217,78) trên sprite — **vị trí chính xác của 2.0 chưa đo** (`+0x14` z) |

`--auto` **hai client** (2026-09-19): `dev.py screenshot` chạy thêm **`jxbot -partner -prefix auto -first 2 -meet-map 1`** (`services/cmd/jxbot/main.go`): bot vào thế giới, `?gm ds NewWorld(1, 1551, 3150)` (ô Mps **tuyệt đối** = gốc bản đồ `region_left·512 / region_top·1024` + `spawn` của `map.json`, chia 32 — `mapSpawn`), treo bảng `TRADE_APPLY_OPEN "bot ban do"` mỗi 1,5 s khi chưa treo, **đồng ý mời đội** (`G2C_TEAM_EVENT` INVITE → `TEAM_REPLY_INVITE 1`), **đồng ý xin giao dịch** (`G2C_TRADE_APPLY` → `TRADE_REPLY 1`), khoá khi client khoá (`G2C_TRADE_SYNC dest_lock` → quyết định 2), xác nhận khi cả hai khoá (quyết định 1), treo lại bảng sau `G2C_TRADE_END`. Client `_auto_team` mời `_auto_partner_id()` (người chơi treo bảng 2 quanh mình), `_auto_trade`: `Earn(50)` qua GM chat, `TRADE_APPLY_START` → cửa sổ mở (`state 2`) → `item_move(kiếm, phòng 2)` → 5 lượng gõ vào `SelfMoney` (`put_money`) → khoá → chờ cả hai khoá + đối tác OK → ảnh → OK → `end=1`. Kết quả: `AUTO_TEAM_PARTNER partner=… invited=true members=1 mate_color=true` (cửa sổ đội 2 tên, máu đồng đội màu (230,190,0)), `AUTO_TRADE partner=… started=true placed=true both_locked=true dest_ok=true window=true my_table=1 end=1 item_gone=true money=50->45`; zone log `trade started` / `item traded` (`new_id`) / `trade done items=1 money=5`. Phát hiện nhờ đó: bảng rao phải đi trong gói spawn (dòng 0x4c ở trên). Chưa: sprite menu 2.0 của `0x00475690`, các mục menu khác (chat/bạn/theo sau/thông tin/bang), phòng 4 kim đĩnh, `SendHoldMsg` lặp khi giữ, kéo–thả đồ vào ô cụ thể (2.0 kéo bằng tay, client mới nhấp = ô trống đầu), `OnRoomMoneyChange`.

## 23. Kênh chat trên client — `KUiMsgCentrePad` (`消息集合面板_左.ini`, `0x004B5E64`), `KUiPlayerBar::SendChat 0x00475A10`, nút kênh `0x004730D0` / menu kênh `0x00472620`, lịch sử `+0x7c46` (M14 lát C2, đã đọc từng dòng `gamecl.exe` + mã 2004)

Mã 2004 để đặt tên: `Ui/UiCase/UiMsgCentrePad.h/.cpp` (`KUiMsgCentrePad`: `NewChannelMessageArrival`, `ChannelMessageArrival`, `MSNMessageArrival`, `GetChannelMenuinfo`, `GetChannelIndex`, `SetChannelTextColor`, `PushChannelData`), `Ui/UiCase/UiPlayerBar.cpp` (`SendChat`, `SetChannel`), `Ui/Elem/PopupMenu` (menu kênh).

| Địa chỉ / tệp | Luật (đã đọc) | Client mới |
|---|---|---|
| **`\ui\ui3_1024\消息集合面板_左.ini`** (`0x004B5E64`: `_左` / `_右` theo `[0x784960]`; nạp `0x004B4A30` → `0x004B50D0..0x004B55D5`) | `[Channels] Channel0..14 = CH_NEARBY, CH_TEAM, CH_WORLD, CH_FACTION, CH_SYSTEM, CH_CITY, CH_TONG, CH_TONGUNION, CH_CHATROOM, CH_ATTACK, CH_DEFEND, CH_JABBER, CH_SONG, CH_JIN, CH_CUSTOM`, `DefaultChannel=CH_SYSTEM`, `DefaultChannelSendName`; mỗi `[CH_*]`: `ShortName0..2` (`%s%d` `0x004B5187`), `FormatName` (tên kênh relay: `NEARBY`, `TEAM`, `WORLD`, `\F<Faction#>`, `GM`, `CITY`, `\O<Tong#>`, `\U<TongUnion#>`, `CHATROOM`), `TextColor`/`TextBorderColor` (`0x004B5110/0x004B513C`), `MenuText`/`DeactivateMenuText`, `MenuImage`/`DeactivateMenuImage`/`TextImage`, `MenuBkColor`, `SendMsgInterval`/`SendMsgNum` (`0x004B53EB/0x004B5408`), `NeverClose`, `Sound`; `[Main] NameTextColor=220,220,220`; `[MSNRoom] TextColorSelf 255,226,168 / Unknown 252,151,255 / Friend 255,202,255` + ảnh; `[ChatTab] ChatTabNum=6` (`ChatTabLabel_%d`, `ChatTabType_%d` 4 tất cả / 1 kênh / 0 hệ thống / 2 riêng / 3 phòng, `ChannelType_%d` mặt nạ) `0x004B4216..` | `jxassets export-ui` → `client/assets/ui/khung-chat/bo-cuc.json` (+ 40 ảnh); `UiMsgCentrePad.gd` (`load_scheme`, `from_sections`, `channels[i] = {key, short, format, color, border, menu_text, texture, interval, num}`) |
| **`KUiPlayerBar::SendChat 0x00475A10(text, ?, bHistory)`** | lấy dòng (`0x00456910`, ≤ 0x200); **lịch sử** 8 × 0x200 tại `+0x7c46`, chỉ số `+0x7c45` (`0x00475AB0..0x00475B02`); rỗng → thôi; ký tự đầu **`/`**, **`&`**, **`%`**: cắt ở dấu cách đầu (`0x00475BE0`), **`&`** → `0x004738F0` + `0x00473030(tên ngắn)` = chỉ số kênh theo `ShortName`; **`/`** → `strncpy(tên, 0x1f)`; **`%`** → `0x00472A10(tên, câu)`; không tiền tố → kênh hiện tại `+0x8c48` (không hợp lệ → kênh đầu có cờ 3 `0x004B3D20(i, 3)`); kênh cờ 4 (GM) đòi `?gm`/`[gm]` (`0x00475D2B`) không thì `G_GSCHANGEDNOTIFY_6`; **`/`** → Lua `Say("%s", "%s")` (`0x790264`), khác → Lua `Chat("%s", "%s")` (`0x790274`) chạy `0x00447EC0`; xử lý biểu cảm `0x005A03D0`; **≥ 0x200 → `G_STR_MSG_VOERFLOW`** (`[0x82241c]`, `0x00475F15`); lọc từ cấm `0x0058DF90(0x9bb4f0)` / `0x00617B90(0x1f17550)` → `G_PLAYERBAR_2`; kênh `&` → `0x004B1CA0/0x004B8040/0x00472C40`; khác → `0x00474600(tên kênh, câu)` → `0x004737F0` → xoá dòng, `0x00475900(kênh, 1)` | `UiMsgCentrePad.parse_input` (`/tên câu` → `CH_WHISPER` + target; `&ngắn câu` → kênh theo `ShortName`; khác → `current`), `KUiGameWindows.send_chat` (≥ 0x200 → `G_STR_MSG_VOERFLOW`; `throttle` `SendMsgNum`/`SendMsgInterval` → `G_PLAYERBAR_3 "%d giây"`) → `Game.chat(text, channel, target)`; `UiPlayerBar` lịch sử 8 dòng, ↑/↓; chưa: `%`, kênh GM, bộ lọc `chatsent.flt` |
| **`0x00475900(kênh, cờ)`** / **`0x004730D0(kênh)`** (`KUiPlayerBar::SetChannel`) | kênh thật (< `0x004B1D30()`): `0x004B1CE0(kênh, buf, 0x2f)` tên → `0x00475690(1, tên)` (viết vào dòng nhập: bỏ tiền tố `/tên ` cũ) rồi `0x004730D0(kênh)`: `+0x8c48 = kênh`, `0x004B6530(kênh, &màu)` → chuỗi 3 byte `{6, màu word}` làm chữ nút `ChannelBtn +0x686c` (`0x004650A0`); kênh phụ (`+0x8d8c`): chuỗi `+0x8c4c + n·32` | `KUiGameWindows.set_channel` → `msg_pad.current` + `UiPlayerBar.set_channel(ShortName0, TextColor)` (`KWndLabeledButton` trên ô `[ChannelBtn]` 20×20 tại (1,692)) |
| **menu kênh `0x00472620`** (từ `ChannelBtn` `0x004764A8`, vị trí chuột `0x0046BF10`) | số kênh `0x004B1D30()` + `+0x8d8c`; mỗi mục 0x4c byte: `0x004B6530(i, &ảnh, &cao, &màu chữ, &màu nền, tên, &ảnh chọn)` → mã màu `6` + tên (`0x00472710..`), ảnh `9`; `0x0044F590(menu, x, y, 2)` mở | `_open_channel_menu` → `KWndPopupMenu` với mục `{text: MenuText, color: TextColor}` (5 kênh nói được: NEARBY, TEAM, WORLD, FACTION, CITY); `_on_channel_picked` → `set_channel` |
| **`KUiMsgCentrePad::ChannelMessageArrival`** (mã 2004; 2.0 `NewChannelMessageArrival`) | dòng = `KTC_INLINE_PIC + TextImage` + màu chữ/viền kênh; tên người nói (không phải `\t` biểu cảm) màu `NameTextColor` + `:` + `KTC_COLOR_RESTORE`; câu; `MSNMessageArrival` (thì thầm): mình → `TextColorSelf`, bạn → `TextColorFriend`, lạ → `TextColorUnknown`; `Sound` của kênh | `UiMsgCentrePad.line(msg, tên mình)` → `{texture, bbcode}`; `UiGame._on_chat`: `add_image(TextImage)` + `[color=Name]tên:[/color] [color=TextColor]câu[/color]` |
| **`KUiMsgCentrePad::PushChannelData` / `SendMsgInterval`** | mỗi kênh tối đa `SendMsgNum` dòng trong `SendMsgInterval` ms (đội 800/2, gần 2000/2, thành 20000/2, môn phái 10000/2, thế giới 60000/2, GM 15000/0) | `throttle(kênh, ms)` → chờ `n` giây (`G_PLAYERBAR_3`) |

`--auto` (`dev.py screenshot`, sau khi mời bot vào đội): `RestoreMana()`, `set_channel(CH_WORLD)`, `send_chat("&T doi oi")` (kênh đội theo tên ngắn `T`), `send_chat("ca the gioi")` (kênh hiện tại), mở menu kênh, `auto_chat.png` → `AUTO_CHAT team=doi oi world=ca the gioi button=Công menu=true`: nút "Công" xanh lá (`CH_WORLD` 146,255,143), menu 5 kênh mỗi dòng một màu, dòng đội xanh dương (64,190,255) và dòng thế giới xanh lá với ảnh kênh trước tên. Chưa: cửa sổ pad thật (`ChatRoom_List`, tab `ChatTab*`, `SysRoom`, `MSNRoom`), `%`, kênh GM, bộ lọc từ, `Sound` kênh, `DefaultChannelSendName`, trang `_右`.

## 24. Hộp thoại npc trên client — `KPlayer::OnScriptAction 0x006004C0` (gói 0x63), `KUiMsgSel` (`滚动选择界面.ini`, `0x0051CF66`), `KUiInformation2` (`提示2.ini`), `KPlayer::OnSelectFromUI 0x005FC7D0` (gói 0x5f) (M13 lát D1, đã đọc từng dòng `gamecl.exe` + mã 2004)

Mã 2004 để đặt tên: `Ui/UiCase/UiMsgSel.h/.cpp` (`KUiMsgSel::OpenWindow(KUiQuestionAndAnswer*)`, `Show`, `OnClickMsg` → `OperationRequest(GOI_QUESTION_CHOOSE, 0, nMsg)`, `m_MsgScrollList` `[Select]`, `m_InfoText` `[InfoText]`), `Ui/UiCase/UiInformation2.cpp` (`KUiInformation2::SpeakWords(KUiInformationParam*, n)`, `[Main]/[Info]/[OK]`), `Ui/GameSpaceChangedNotify.cpp` (`GDCNI_QUESTION_CHOOSE` → `KUiMsgSel::OpenWindow`, `GDCNI_SPEAK_WORDS` → `g_UiInformation2.SpeakWords`), `Core/Src/CoreShell.cpp` (`GOI_QUESTION_CHOOSE` → `OnSelectFromUI(idx, UI_SELECTDIALOG)`, `GOI_INFORMATION_CONFIRM_NOTIFY` → `OnSelectFromUI(0, UI_TALKDIALOG)`), `Core/Src/KPlayer.cpp` (`OnScriptAction`, `OnSelectFromUI`, `DialogNpc(int)`).

| Địa chỉ / tệp | Luật (đã đọc) | Client mới |
|---|---|---|
| **`KPlayer::OnScriptAction 0x006004C0`** (gói 0x63) | `+3` (`m_nOperateType`): 1 → `SCRIPTACTION_EXESCRIPT`: chạy script client `"OnCall"` với nội dung (`0x005FC580`); 0 → `+4` (`m_bUIId`) ≤ 0x2e nhảy bảng `0x601e10`: **0 `UI_SELECTDIALOG` → `0x00600AE0`**: cấp `KUiQuestionAndAnswer` (`0x104·(số+2)` byte, `+0x200` dài câu, `+0x204` số trả lời, `+0x208` mảng 0x104: chữ + dài), `+6 == 0` → câu từ `+0x11` tới `\0`, lựa chọn nối tiếp sau `\0` (byte kế `\0` → 0 lựa chọn); `+6 ≠ 0` → `g_GetStringRes(dword +0x11)` (`0x006C3A50`) rồi lựa chọn từ `+0x15`; mỗi chuỗi qua `0x006C3AB0` (bảng dịch) + `TEncodeText 0x005B1A56`; `g_bUISelIntelActiveWithServer [0x9bd898] = +7`; → `KUiMsgSel::OpenWindow`; **2 `UI_TALKDIALOG` → `0x00600D64`**: `+5` trang × 0x244 byte, tách `"| |"` (`0x7a740c`), trang là số (`+6`) → `atoi` + `g_GetStringRes`; nhãn nút: trước trang cuối `[0x9bf514]` = **`G_PLAYER_14` "Tiếp tục"**, trang cuối `[0x9bf518]` = **`G_PLAYER_15` "Hoàn thành"**; `+9 == 1` → `bNeedConfirmNotify` (`+0x42 = 1`); `g_bUISpeakActiveWithServer [0x9bd89c] = +7` | `Game.script_action` → `KUiGameWindows._on_script_action`: ui 0 → `UiMsgSel.open_dialog(text, options)`, ui 2 → `UiInformation2.speak_words(pages, param == 1)`; `text_id` chưa có bảng → `"[id]"` |
| **`KUiMsgSel`** (`0x0051CF66`: `%s\滚动选择界面.ini` — `[Main]` 455×287 tại (182,130) `NPC对话框2.spr` `PositionType=1`, `[InfoText]` (30,24) 380×96 font 14 màu 202,230,171, `[Select]` DummyWnd + `[Select_List]` (30,136) 400×122 `MsgLineCount=1,9` `MsgColor` 202,230,171 `SelColor` 255,253,122 `SelBgColor` 140,121,99 `HighLightColor` 0,255,0, `[Select_Scroll]`) | `Show 0x0051D0F0`: câu vào `InfoText`, `+0x204 > 0` → từng trả lời vào danh sách, `== 0` → một dòng **`G_UiMsgSel_0` "Kết thúc đối thoại"** (`[0x8220f8]`); `OnClickMsg 0x0051D020`: đóng, xoá danh sách, `OperationRequest(9, 0, idx)` (`0x0051D056`) | `UiMsgSel.gd` (`hop-thoai-chon`): `KWndText` InfoText + `KWndList` Select_List (đọc `MsgColor/MsgBorderColor/SelBgColor`), `KUiDialogMath.lines_for`; click → `chosen(idx)` → `Game.dialog_answer(idx, 0)`; không lựa chọn → chỉ đóng |
| **`OperationRequest(GOI_QUESTION_CHOOSE = 9)`** `0x005C1CF0` bảng `0x5c5470[8]` → `0x005C2CF5` | `[0x9bd8a0]` và `[0x9bd88c]` (CoreShell) phải có; npc mình `+0x1914 == 2` (đang ở sạp) → `{0x7c, 7, 5}` gói khác; khác → gói tại `ebp+0x30`: `+1 = idx`, `+5 = +9 = +0xd = 0` → **`KPlayer::OnSelectFromUI 0x005FC7D0(gói, loại 0)`**: loại 0 cần `[0x9bd898]`, loại 2 cần `[0x9bd89c]`, khác → thôi; `[0] = 0x5f`, gửi **0x11 = 17 byte** qua `[0x9bd88c]->vtable+0x10`; không "với server" → `0x005FC5D0` (script client) | `Game.dialog_answer(index, kind = 0)` → `C2G_DIALOG_ANSWER{index, kind}` |
| **`KUiInformation2`** (`提示2.ini`: `[Main]` 455×175 tại (182,180) `NPC对话框1.spr` `PositionType=1`, `[Info]` (27,24) 410×75 font 14 màu 202,230,171, `[OK]` nút chữ (27,91) 377×13 `OverColor` 255,247,112 `SelColor` 255,130,47) | `SpeakWords`: nối trang mới sau các trang chưa hiện; cửa sổ đang ẩn → `Show(trang đầu, nhãn, caller = bNeedConfirmNotify ? WND_GAMESPACE : 0)`; bấm OK → trang kế; trang cuối có notify → `GOI_INFORMATION_CONFIRM_NOTIFY` → `OnSelectFromUI(0, UI_TALKDIALOG)` → gói 0x5f chỉ số 0 | `UiInformation2.gd` (`hop-thoai-mot-nut`): `speak_words(pages, notify)`, nhãn `KUiDialogMath.page_label` (`G_PLAYER_14`/`G_PLAYER_15` từ `chuoi-core.json`), `confirmed` → `Game.dialog_answer(0, 0)` |
| **`KPlayer::DialogNpc(int)`** (2004) / click npc 2.0 | npc có `ActionScript` → script client; không → gói `c2s_dialognpc {id npc}` = 2.0 gói 0x6e | `UiGame._unhandled_input`: click trái lên npc `npc_kind == 3` (`EntityInfo.npc_kind`) → `Game.npc_dialog(id)` (`C2G_NPC_DIALOG`) thay vì đánh |

`--auto` (từ M13 D3): khi dialoger gần nhất xa hơn 240 px, `_auto_dialog` `move_to` tới cách npc 120 px rồi mới `npc_dialog` (client 2.0 cũng đi tới npc khi
xa — mã đi tới chưa đọc, còn trong "chưa"): `AUTO_DIALOG npc=4294967977 name=Bành Tiểu đệ distance=115 ui=0 text_len=81 options=0 window=true answered=true`. Trước đó: `_auto_dialog` chọn dialoger gần nhất (`Bành Tiểu đệ` cạnh điểm sinh, 247 px), `npc_dialog` → `script_action` (ui 0, 81 ký tự, 0 lựa chọn) → `auto_dialog.png` (câu tiếng Việt đã giải mã TCVN3 + dòng "Kết thúc đối thoại") → bấm dòng đầu → `AUTO_DIALOG … window=true answered=true`. Chưa: `text_id` (`g_GetStringRes` — bảng chuỗi tài nguyên của client), `SCRIPTACTION_EXESCRIPT` "OnCall", các ui id khác của `UIInfo` (note/msg/news/music/tong), cuộn tự động khi rê chuột mép danh sách (`Breathe` 200 ms), `Wnd_SetExclusive` (khoá cửa sổ khác), con trỏ nói chuyện, đi tới npc khi xa.

## 25. Giá trị nhiệm vụ trên client — gói 0xa7 `0x006512F0`, 0xb5 `0x00651350`, `KPlayer::SetTaskValue 0x00601ED0` (map `KPlayer+0xa1a0`), bảng `player_task_def.txt` `0x006CF940`, gói 0xa9 lên máy chủ (M13 lát D2, đã đọc từng dòng `gamecl.exe` + mã 2004)

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| Bảng ô xử lý s2c (`0x0065DD00..0x0065E1C0`: `mov [esi + 4 + id·4], hàm`; 0x4c tại `0x0065DD88`, 0x86 tại `0x0065DFE4`) | `[esi+0x2a0]` 0xa7 → `0x006512F0`; `[esi+0x2d8]` 0xb5 → `0x00651350`; `[esi+0x2dc]` 0xb6 → `0x00651390` (chuỗi ≤ 0x80 byte → thông điệp UI 0x5d khi byte đầu 0x10, khác 0x52) | `KProtocolProcess.gd` `G2C_TASK_VALUE 2140` / `G2C_TASK_VALUES 2141` |
| 0xa7 `0x006512F0(gói)` | `id = [+1]`, `giá trị = [+5]`; **`id == 0x92c` (2348)** → `0x00602920(KPlayer = core+0xa878, giá trị)`: giá trị là chỉ số vào bảng toàn cục `0x24ee4cc` (8 byte/ô, ngoài → thôi) → `SetTaskValue(0x92c, chỉ số, 0)` rồi `+0x6654 = [ô+4]`, `+0x6650 = 0`; khác → `SetTaskValue(id, giá trị, 0)`; rồi luôn `0x005B8150(0x54, id, giá trị, 0)` = thông điệp UI 0x54 qua `[0x9bca44]->vtable[0]` | `_set_task_value` + tín hiệu `task_value_changed(id, value)` |
| 0xb5 `0x00651350(gói)` | từ `+1`, tối đa **79** cặp (`cmp edi, 0x4f`), **dừng ở `id == 0`**, mỗi cặp `SetTaskValue(id, giá trị, 0)`, **không** thông điệp UI | `KPlayerTask.apply_batch` (mọi cặp, gói ta không đệm 0) |
| **`KPlayer::SetTaskValue 0x00601ED0(this, id, giá trị, bSync)`** | map `this+0xa1a0`: `0x00616D60(map, id)` lấy (không có → 0); khác → `0x00617610(map, id, giá trị)`; `0x005FFF00(0x24ec6e8, &cờ, id)` tra bảng client (`_Tree` MSVC tại `+0xc`, không có → cờ 0); **cờ bit1 (CLIENT_FLAG)** và `bSync` và có kết nối `[0x9bd88c]` → gửi **0xa9** 9 byte `{0xa9, int id, int giá trị}` (`vtable+0x10`); ghi chú: máy chủ Linux nhận 0xa9 ở bộ đăng ký `0x978bf20` (`0x080DAFF0` đọc `[+9]` ngoài gói 9 byte), luật cờ CLIENT nằm ở ô 0xaa `0x080DB070` (`LINUX-SERVER.md` §21) | `Game.set_task_value(id, value)` → `C2G_TASK_VALUE 1123` (zone chỉ nhận id CLIENT_FLAG) |
| Bảng client `0x006CF940` → `0x006CF740(0x24ec6e8, "\settings\task\player_task_def.txt")` (cột `TASK_ID_FIRST TASK_ID_LAST TASK_NAME SYNC_FLAG CLIENT_FLAG TASK_DESCRIBE`) | bản client 690 dòng, 164 SYNC, 26 CLIENT — khác bản máy chủ (650 / 68 / 3: 1276, 2881, 2882); zone theo bản máy chủ (`jxassets export-task-def`) | `client/assets/task_def.json` (chỉ zone dùng) |
| Ai đọc giá trị trên client | thông điệp UI 0x54 (cửa sổ nhiệm vụ/`KUiTask*`, lát sau), `0x00602920` (2348: bảng `0x24ee4cc` → `+0x6654`), `GetTaskValue` của các script client | `Game.task_value(id)` cho các cửa sổ sau |

`--auto`: `_auto_task` (sau `_auto_dialog`): đếm `Game.task_values` sau khi vào game, gửi `set_task_value(1276, v+1)` (1276 có CLIENT_FLAG), rồi `?gm ds SyncTaskValue(1276)` và
`?gm ds SetTask(100, v)` (100 có SYNC_FLAG), chờ `task_value_changed` cả hai: `AUTO_TASK packets=181 synced=2 client_set=665 script_set=664 expected=664 stored=664` (181 = 179 gói 0xa7 lúc vào game + 2; `synced=2` = hai giá trị của lần chạy trước còn trong role data)
+ `auto_task.png`; zone: `task def table loaded ids=179 ranges=68`.

## 26. Describe và TaskTip trên client — `OnScriptAction` ui 12 `0x006007FD` → thông điệp UI 0x40 → cửa sổ `npc描述界面.ini` `0x00508410`, gói 0xb6 `0x00651390` → 0x5d/0x52 → bảng thông điệp hệ thống `系统消息.ini` `0x004C4060`, bộ nhận thông điệp UI `0x00428970` (M13 lát D6, đã đọc từng dòng `gamecl.exe`)

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| **Bộ nhận thông điệp UI** | `0x005B8150(msg, p1, p2, p3)` → `[0x9bca44]->vtable[0](msg, p1, p2, p3)`; đối tượng tĩnh `0x9bb518` đặt bởi `0x005B81B0` (gọi tại `0x005923A1`), phương thức gọi thẳng `0x0042B0A0` (thunk) → **`0x00428970`**: `msg − 1 ≤ 0xbb` → bảng nhảy `0x42ad94` (188 ô); các số trùng enum 2003 `GDCNI_*` (`Core/Src/CoreShell.h`): 0x16 `GDCNI_QUESTION_CHOOSE` → `0x0042917E` → `0x0051D5F0(obj, 0)` (`KUiMsgSel::OpenWindow`, Say), 0x17 `GDCNI_SPEAK_WORDS` → `0x00429611`, 0x1f `GDCNI_SYSTEM_MESSAGE`; thêm của 2.0 (ngoài 0x31): **0x40 → `0x00429664`** (Describe), **0x52 → `0x0042A0C3`**, 0x54 → `0x0042A157` (giá trị nhiệm vụ), **0x5d → `0x0042A10A`** (TaskTip) | `KUiGameWindows._on_script_action` (ui 12), `Game.task_tip` |
| **`OnScriptAction` ô 12 `0x006007FD`** (bảng `0x601e10`) | `[+0xd]` (dài) ≤ 0 → thôi; cấp `0x510` byte (`[+5] == 0`) hoặc `0x510 + ([+5]−1)·0x104` (`+0x400` dài chữ, `+0x404` số lựa chọn, `+0x408 = [+9]`); hai đệm 0x583 về 0; `[+6] == 0` → `strncpy(đệm, +0x11, dài+1)`, `strstr(đệm, "| |")` (`0x7a740c`): không có → `[+5] = 0`, `+0x404 = 0`; có → `0x005F7A40` cắt, phần sau +3 = lựa chọn; chữ qua `0x006C3AB0(đệm, ra, 0x584)` (bảng dịch) rồi `strncpy(obj, …, 0x400)`, `+0x400 = strlen`; `[+6] ≠ 0` → `0x006C3A50(dword +0x11, ra, 0x3e8, 0x400)` (`g_GetStringRes`), lựa chọn từ `+0x15` với `dài − 3`; mỗi lựa chọn tách `"| |"`, dịch, `strncpy(obj+0x40c+i·0x104, …, 0x100)`, `[+0x100] = −1`, `+0x404 = i+1`; `[0x9bd898] = [+7]`, `[0x9bd8a0] = obj+0x404`; **`0x005B8150(0x40, obj, 0, 0)`** rồi `free(obj)` | `Game.script_action` ui 12 → `UiNpcDescribe.open_dialog(text, options, tên npc)`; `Game.dialog_npc` (npc của lần `npc_dialog` cuối) cho tên |
| **Cửa sổ `npc描述界面.ini`** (`0x8c0320`, 0x4dcc byte): `new 0x0050843B`, ctor `0x00507FB0` (vtable `0x79821c`), `Init 0x00508090` (năm con `+0x584/+0x1268/+0x28e0/+0x2dd0/+0x3324`, `0x00461080(+0x28e0, +0x584)`), `LoadScheme 0x00507A30` (`"%s\%s"` với `0x7981e0`, `[Main]`), `OpenWindow 0x00508410`, `Đang mở? 0x005079D0` (`+0x4dc8`), `WndProc 0x00508510` (0x5c9 nhấp dòng → `0x00507C50`; 0x100/0x200/0x501), lật trang `0x00507ED0` (`+0x39b0/+0x39b4/+0x39b8`) | `[Main]` 573×287 tại (247,220) `\Spr\Ui4\主界面\npc对话条\NPC对话框3.spr` `PositionType=1` `Moveable=1`; `[Image]` 64×82 tại (42,34) (chân dung npc); `[Text]` 64×82 tại (42,28) chữ 16 màu 0,255,0 giữa, nhiều dòng (tên npc); `[MessageList]` (136,17) 378×136 chữ 14 `MsgColor 202,255,202` `MaxMsgCount 20` `MsgLineCount 40,40` `Selable 1` + `[Msg_Scroll]` (545,17) 14×126; `[Select_List]` (24,155) 525×128 chữ 12 `MsgLineCount 1,5` `MsgColor 202,230,171` `SelColor 255,253,122` `HighLightColor 0,255,0` `SelBgColor 140,121,99` `SelBorderColor 255,0,0` + `[Select_Scroll]` (545,155) 14×153 (`\Spr\Ui4\common\拖动条.spr`) | `export-ui` `mo-ta-npc` → `UiNpcDescribe.gd` (`KWndText` nhiều dòng cho `[MessageList]`, `KWndList` cho `[Select_List]`, `KWndText` tên; chân dung `[Image]` để trống — ảnh đầu npc chưa xuất) |
| Trả lời | nhấp dòng → `OperationRequest` `0x005C1C00` ô 9 `GOI_QUESTION_CHOOSE` (`0x005C2CF5`: `[0x9bd8a0] ≠ 0`, `[0x9bd88c] ≠ 0`; trạng thái `+0x1914 == 2` và chỉ số 0 → gói 0x7c 7 byte thay vì trả lời) → `0x005C2D70` `OnSelectFromUI(&{0, chỉ số, 0, 0, 0}, 0)` `0x005FC7D0` → gói 0x5f `{chỉ số, kind 0}` như Say | `describe.chosen → Game.dialog_answer(index, 0)` |
| **Gói 0xb6 `0x00651390`** | `strncpy(đệm, gói+1, 0x80)`; byte đầu `0x10` → `0x005B8150(0x5d, đệm+1, 0, 0)`, khác → `0x005B8150(0x52, đệm, 0, 0)` | `G2C_TASK_TIP{text}` → `Game.task_tip` |
| Thông điệp 0x5d `0x0042A10A` / 0x52 `0x0042A0C3` | dựng tham số trên ngăn xếp `{chữ[0x104], +0x104 loại, +0x105 nháy, +0x106 ưu tiên, +0x107 thêm}`: 0x5d → `1, 1, 3, 0`; 0x52 → `5, 0x12, 3, 0`; `0x004C4060(&tham số, 0)` | `sys_msg_pane.add_message(text, 1, true, 3)` |
| **Bảng thông điệp hệ thống** (`0x846eac`, 0x4c54 byte): `new 0x004C4A7A`, ctor `0x004C45C0`, `Init 0x004C4760` (`SysMsgDisappearInterval` của `[Main]`, mặc định 0x7530 = 30000 ms, `0x004C47E0`), `LoadScheme 0x004C3CC0` (`系统消息.ini` `0x794490`), 8 con biểu tượng 0x770 byte tại `+0x528` (vtable `0x79444c`, `0x004C3BF0`), `OpenWindow 0x004C4060`, `Có rồi? 0x004C3820`, `AddMsg 0x004C39F0`, vẽ `0x004C3A90`, đóng `0x004C3C90` | `OpenWindow(tham số, n)`: không tham số/không đối tượng → 0; `+0x107 ≠ 0 && n == 0` → 0; loại 0 hoặc 0xa → `0x004B7930(chữ, dài, loại == 0xa)` (dòng chat); `+0x105 == 3` và `0x8c0740` rảnh (`0x00524D70`) → `0x004ACBD0(…)` (hộp); loại 9 → `0x004ACBD0(…, −1)`; **khác: `0x004C3820` (cùng loại, nháy, ưu tiên, `+0x107` ≤, cùng chữ → có rồi, thôi), chép `0x108 + [+0x107]` byte, `0x004C39F0(bản sao, loại − 1, 0)`: danh sách theo loại (`this + (loại·3 + 0x132)·4`, cấp thêm 4 ô), chèn sau các mục có `+0x106` ≤ (`0x004C3A4D..0x004C3A6F`), `+0x100 = 0x00608270()` (thời điểm)**; vẽ `0x004C3A90`: nền `[Main]` rồi 8 loại: loại có mục → biểu tượng con thứ i (`+0x40a8 + i·0xb8`) vẽ, mục đầu `+0x105 == 1` → đổi khung (`+0x4668 + i·2`) lúc vẽ (nháy) | `UiSysMsg.gd` + `KUiDialogMath.sys_msg_add/prune/latest/blinks` (trùng → bỏ, sắp theo ưu tiên, mất sau `SysMsgDisappearInterval`, biểu tượng nháy 500 ms); chữ mới nhất của loại ghi trên dòng `[MsgText]` dưới biểu tượng — **WndProc (trỏ/nhấp biểu tượng) chưa đọc** |
| `系统消息.ini` | `[Main]` 240×250 tại (790,593) `NormalMsgColor 255,0,0` (không ảnh); `[MsgText]` (0,25) 240×14 chữ 12 màu 0,255,0; `[MsgIcon_1..8]` 24×24 tại (200, (n−1)·25): `系统` (hệ thống) / `升级` (lên cấp) / `组队` (tổ đội) / `聊天` (chat) / `任务` (nhiệm vụ) / `帮会` (bang) / `交易` (giao dịch) / `定位` (định vị) (`\Spr\Ui\系统消息图标\*.spr`) | `export-ui` `thong-diep-he-thong`; **neo góc dưới phải** (luật 14: `KUiDialogMath.bottom_right_anchor` giữ khoảng cách tới mép phải/dưới của 1024×768 — 240×250 tại (790,593) tràn 6 px phải, 75 px dưới; 1280×720 → (1046,545)) |

`--auto`: `_auto_describe` (sau `_auto_task`): `?gm ds TaskTip("Ban nhan duoc mot nhiem vu ngau nhien")` và `?gm ds Describe("Day la loi mo ta …", 2, "Lua chon mot/OnOne", "Lua chon hai")`,
chờ `script_action` ui 12 và `task_tip`, chụp `auto_describe.png`, nhấp lựa chọn 1: `AUTO_DESCRIBE ui=12 options=2 window=true tip_len=37 pane=true` (zone: `dialog answer without a script` 1 — hộp mở từ lệnh GM không có script để gọi lại, đúng như thiết kế §20).

## 27. Hộp đưa vật phẩm trên client — `OnScriptAction` ui 11 `0x0060194C` → thông điệp UI 0x3e `0x00429EAD` → `KUiGiveItem 0x00519C80` (`给予界面.ini`), gói 0x89, 0xdf → 0xa8 (M13 lát D8, đã đọc từng dòng `gamecl.exe`)

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| `OnScriptAction` ô 11 `0x0060194C` | `[+6] == 0` → `strncpy(đệm, +0x11, 0x200)` nội dung, rồi **tiêu đề 0x20 byte sau NUL** (`+0x12 + dài`); `[+6] ≠ 0` → `g_GetStringRes(dword +0x11)` + tiêu đề từ `+0x15`; bit0 của `KPlayer+0x64b4` = (`[+9] ≠ 0`); `[+8]` → byte cờ (báo đổi); `0x005B8150(0x3e, &{nội dung, tiêu đề…}, [+9], 0)` | `Game.script_action` ui 11 → `UiGiveItem.open_box(text, options[0], notify, param)` |
| Thông điệp 0x3e `0x00429EAD` | `0x00519C80(obj)` = `KUiGiveItem::OpenWindow` (đối tượng `0x8c0354`, 0xb7cc byte, `new 0x00519CAA`, ctor `0x00519A50` vtable `0x799134`, `Init 0x00519710` (11 con), `0x00519020` nạp `"%s\给予界面.ini"`), `0x00518F30(obj, tham số)` điền chữ (`+0x8010` nội dung qua `0x00468460`, `+0x85a8` tiêu đề `0x0045D020`, `+0x554` = cờ [+0x220]), `OperationRequest 0x3b(1)` (mở) ; đóng `0x00518FB0`: `0x3b(0)` rồi `0x5d(cờ)` | — |
| **`给予界面.ini`** | `[Main]` 223×325 tại (294,116) `\Spr\Ui4\主界面\给予界面\给予底图.spr` `PositionType 1` `Moveable 1`; `[LabelTitle]` "Giao Nộp" (5,7) 195×16 chữ 16 trắng giữa; `[Title]` (16,36) 190×17 chữ 12 màu 201,215,162 giữa; `[ContentList]` (11,59) 188×96 chữ 12 `MsgColor 255,217,78` `MsgLineCount 40,40` `MaxMsgCount 40` + `[ContentScroll]` (200,74) 14×96; **`[Items]` (27,160) 170×112 `HUnits 6` `VUnits 4` `UnitBorder 2`** (24 ô = 24 mục của `GetGiveItemUnit`); `[Assemble]` "Đồng ý" (20,292) 74×26 `大按钮二字.spr` chữ 14 màu 218,165,105 / over 225,213,43; `[Close]` "Hủy bỏ" (128,292); `[CloseBtn]` (198,2) 24×24 `关闭.spr` | `export-ui` `dua-vat-pham` → `UiGiveItem.gd` (`KWndText` ×3, `KWndObjContainer` 6×4, `KWndLabeledButton` ×2, `KWndButton`) |
| Hộp `[Items]` và gói 0x89 | vật vẫn nằm trong túi; hộp chỉ nhớ (core giữ danh sách: `GetGameData 0x7da` ở `0x00519390`, mỗi mục 0x18 byte); thả vật vào ô → `OperationRequest 3(&nguồn, &đích)` (`0x00519307`, đích là hộp đưa); "Đồng ý" → gói 0x89 `{0x89, word cỡ = N·5 + 10, int kind, int N, {phòng, x, y, ôX, ôY}[N]}` (bố cục theo `0x080AB980`); cờ báo đổi → gói 0x89 kind 0 mỗi lần hộp đổi | nhấp vật túi (cầm lên) rồi nhấp ô hộp → `put_item` (kiểm trong hộp, không chồng, `KUiDialogMath.give_box_place`), nhấp vật trong hộp → bỏ ra; "Đồng ý" → `Game.give_items(1, entries)` (`C2G_GIVE_ITEMS`), `notify` → `give_items(0, …)`; `entries` = `{room, x, y}` của vật trong túi + ô |
| Huỷ / đóng | `0x00518FB0`: `OperationRequest 0x5d(cờ)` → `0x005C3B0F` → `KPlayer 0x005F8050(cờ)` → gói 0x5f chỉ số 1 (hàm huỷ) | `cancelled → Game.dialog_answer(1, 0)` |
| Gói 0xdf `0x0064FD10` / 0xd8 `0x00654C60` | 0xdf: chép chữ `[+9..]` dài `[+1] − 9` → `0x005B8150(0xa8, chữ, 0, 0)` (hộp hỏi lại trước khi đưa); 0xd8 ở client 2.0 đọc `[+1]` như id npc → `0x005ECA70(0x3e9, …)` — không phải gợi ý của `SetUiGiveItemMsg` | `G2C_GIVE_ITEM_MSG` kind 1 → câu hỏi lại thay nội dung, "Đồng ý" lần hai mới gửi; kind 0 → gợi ý thay nội dung |

## 28. Sổ nhật ký "Ký Sự" trên client — `KUiTaskNote` (`任务记事.ini` + 4 trang), `WakeUp 0x004D2550` từ thông điệp UI 0x24 (AddNote), `MissionMemory.dat` (M13 lát D9, đã đọc từng dòng `gamecl.exe` + mã 2004)

Mã 2004 để đặt tên: `Ui/UiCase/UiTaskNote.h/.cpp` (`KUiTaskNote : KWndPageSet` với `KUiTaskNote_System` (danh sách bản ghi + nút xoá, `OnDelete`), `KUiTaskNote_Personal`
(`KTaskEdit` 2048 ký tự + `OnSave`), `WakeUp(chữ, dài, giá trị)`), `Ui/UiCase/UiTaskDataFile.h/.cpp` (`KTaskDataFile`: tệp `MissionMemory.dat` trong thư mục riêng của người chơi,
đầu `{"\0TDF", byte ghi chép cá nhân, số bản ghi hệ thống, dự trữ}`, bản ghi `{time, uReserved = giá trị, dài, chữ}`, `InsertSystemRecord` chèn **đầu** danh sách, không giới hạn số).

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| Thông điệp 0x24 `0x004298DE` | `0x004D2550(obj, [obj+0x100] = dài, [obj+0x104] = giá trị)` = `WakeUp`: chữ ≠ 0, dài > 0; cửa sổ `[0x8bf42c]` chưa có → `0x004D1230` nạp tệp; bản ghi `{time(), giá trị, dài, chữ}` → `0x004D15A0` chèn; cửa sổ chưa có → (chèn được → `0x004D13E0` lưu) rồi `0x004D1030` xoá bộ nhớ; có → `0x004D1AC0(cửa sổ + 0x590)` vẽ lại trang hệ thống | `UiTaskNote.add_system_record`: chèn đầu (`KUiDialogMath.journal_insert`), lưu ngay `user://journal_<player id>.json` `{personal, system[{time, value, text}]}`, vẽ lại nếu đang mở |
| **`任务记事.ini`** (`0x004D21B2` nạp trong `LoadScheme 0x004D2150`, `OpenWindow 0x004D2334` gọi qua bảng tên `Switch([[tasknote]])` `0x80db08`) | `[Main]` 420×287 tại (294,200) `\spr\Ui4\主界面\排名 记事界面资源\记事界面底图.spr` `Moveable 1`; `[Title]` "Ký Sự" (185,7) 50×17 chữ 16 trắng giữa; 4 nút trang (checkbox `分页1.spr` 21 cao, y = 29, chữ 12 màu 169,211,190 / chọn 225,213,43): `[BulletinBtn]` "Cập nhật sự kiện" x = 7, `[SystemBtn]` "Ghi chú nhiệm vụ" 109, `[PersonalBtn]` "Nhật ký" 211, `[ItemBindBtn]` "Mở khóa vật phẩm" 313; `[CloseBtn]` (392,3) `关闭.spr` | `export-ui` `nhat-ky` → `UiTaskNote.gd` (trang mặc định: hệ thống) |
| `任务记事-系统任务分页.ini` | `[Main]` (4,49) 412×228 `个人游戏记事页面.spr`; `[List]` + `[List_List]` (2,2) 393×200 chữ 12 `MsgColor 218,255,165` `MsgBorderColor 23,68,0` `Selable 1` `SelColor 255,253,122` `SelBgColor 150,142,105` `MsgLineCount 10,500` + `[List_Scroll]`; `[DeleteBtn]` "Xóa" (161,201) 74×26 `大按钮二字.spr` | `nhat-ky-he-thong` → `KWndList` + nút xoá (`OnDelete`: bỏ bản ghi đang chọn, chọn lùi khi là cuối) |
| `任务记事-个人记事分页.ini` | `[Editor]` (0,0) 396×200 chữ 12 màu 218,255,165 `MaxLen 2000` `MultiLine 1` `Type 2` + `[EditorScroll]`; `[SaveBtn]` "Lưu" (161,201) | `nhat-ky-ca-nhan` → `KWndEdit` + nút lưu (`OnSave`: giữ ≤ 2000 ký tự) |
| `任务记事-游戏更新记录.ini` / `任务记事-装备绑定.ini` | trang cập nhật: `[TitleList]` (2,2) 188×224 + `[MessageList]` (208,2) 188×224 (tệp lịch sử cập nhật của client); trang ràng buộc: `[List_List]` 393×224 `Selable 0` | `nhat-ky-cap-nhat`, `nhat-ky-rang-buoc`: chỉ nền (dữ liệu tệp client chưa xuất) |
| Mở cửa sổ | bảng tên lệnh `0x80dac8..` (`rec/endrec/play/…/team/map/status/Items/skills/system/friend/activityguide/tasknote/…`) → `Switch([[tasknote]])`; nút thanh công cụ `Player_Task` ("Nhiệm Vụ", `任务.spr`) gọi thẳng (không trong dãy `Open([[…]])` `0x0044B250`), đích chưa dò | thanh công cụ `"task"` → `journal.toggle_window()` (tạm, ghi chú) |

## 29. Hộp nhập số / chuỗi của script trên client — gói 0xa3 `0x00653500 → 0x006AC550 → 0x006AC330` → thông điệp UI 0x37 `0x00429DFF` → `KUiGetString 0x0051CD00` (`输入字串界面.ini` — **một hộp cho cả số lẫn chuỗi**) (M13 lát D10, đã đọc từng dòng `gamecl.exe`)

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| Gói 0xa3 | ô `[esi+0x290]` → `0x00653500` → `0x006AC550(gói)`: `[+3]` 1 → `0x006AC290` (đặt `[0x220fd98] = [+4]`, thông điệp 0x29 — cửa hàng), 2 → **`0x006AC330`**: `kind = [+4]`, tiêu đề `strncpy(+5, 0x20)` dịch `0x006C3AB0`; kind ≠ 0 → `min = [+0x25]`, `max = [+0x29]`; kind 0 → thêm mặc định `strncpy(+0x2d, 0x20)`; `0x005B8150(0x37, &{byte kind, tiêu đề[0x20], int min +0x24, int max +0x28, mặc định +0x2c}, 0, 0)` | `KProtocolProcess` `G2C_SCRIPT_ASK` → tín hiệu `script_ask{kind, title, min, max, default_text}` |
| Thông điệp 0x37 `0x00429DFF` | `0x0051C650()` (đang mở? → thôi); `0x0051CD00(tiêu đề, mặc định \| NULL (kind ≠ 0), callback 0x8468b0, mode = 2 + (kind ≠ 0), min, max, 1, kind)` | `KUiGameWindows._on_script_ask` → `UiGetString.open_box(title, default, min, max, numeric = kind ≠ 0)` |
| **Cửa sổ `0x8c038c` = `KUiGetString`** (RTTI `.?AVKUiGetString@@` 0x8120dc, vtable 0x7994b4, 0x2530 byte, ctor `0x0051CB70`) | **`Init 0x0051CA10` thêm đúng 4 con**: tiêu đề `+0x55c`, ô nhập `+0xafc`, "Đồng ý" `+0x11e8`, "Hủy bỏ" `+0x1b88`; `0x0051C690` nạp **`%s\输入字串界面.ini`** (`0x00444AF0` = thư mục ui) với `[Main] [Title] [StringInput] [OkBtn] [CancelBtn]`; `Init(kind ≠ 0)`: ô nhập tối đa **9 ký tự** (`+0xfa0 = 9`, `0x0051CA8D`), chế độ 0 (`0x00455E00`), `+0xfd8 = +0xfdc = 1` → **hộp chuỗi ở chế độ số**, không có bàn phím số. (`数字键盘.ini` = `KUiNumberPad` RTTI 0x812120, cửa sổ `0x8c0394`, nạp `0x0051D740..` `[Main] [Number] …` là cửa sổ khác, chưa dò nơi mở; trong ini ấy `[Back]` (131,56) đè `[2]` (124,60) và `[00]` (54,173) đè `[9]` (40,180) — các mục thừa.) | `UiGetString.gd` (`export-ui` `nhap-chuoi`), `KWndEdit.set_limits(9, chỉ số)` |
| **`OpenWindow 0x0051CD00`** | `+0x554 = 1`, `+0x558 = kind`, `+0xaf4 = min`, `+0xaf8 = max`, `+0x2528 = callback`, `+0x252c = mode`; tiêu đề vào `+0x55c` (`0x00468460`, NULL → "" 0x7b8f70); **kind 0**: `+0xfa0 = max` (dài nhất), mặc định → `0x00456770`, không có → `0x00456820(0)` xoá; **kind ≠ 0**: `0x00456890(min)` — **ô hiện sẵn min**; `0x0046BE90` đưa cửa sổ lên trên cùng (nối lại danh sách anh em `+0x54/+0x58`), `vtable+0x20` hiện. `[Main]` nằm (0,0) — client 2.0 không dời hộp trong OpenWindow | `open_box`: số → `set_limits(9, true)` + `set_text(str(min))`; chuỗi → `set_limits(max, false)` + mặc định; hộp đặt giữa màn hình (`fix_pos`, luật 14) — khác 2.0 (góc trái trên) có chủ ý |
| **"Đồng ý" `0x0051C800`** | `+0x558 ≠ 0`: `giá trị = 0x004571F0(ô)` (số trong ô); **`giá trị < min` → hộp thông báo `0x004ACBD0([0x8220bc] = G_UiGetString_0, 0,0,0,0,0,-1)`, hộp giữ nguyên** (`0x0051C831 → 0x0051C919`); đủ → `[+0x2528]->vtable[4](0x501, mode 3, giá trị)` (`0x0051C846`) rồi đóng `0x0051C610(1)`; **không kiểm max**. `+0x558 = 0`: chữ `0x00456910(ô, buf, 0x200, 1)` → dài; `dài < min` → `G_UiGetString_1` (`0x0051C8B6`), `dài > max` → `G_UiGetString_2` (`0x0051C8BA`); đủ → callback(0x501, mode 2, chữ), đóng. "Hủy bỏ" chỉ đóng (máy chủ vẫn chờ) | `_on_ok`: `KUiDialogMath.ask_number` / `ask_check_number(value, min)` / `ask_check_text(len, min, max)`; từ chối → tín hiệu `rejected(lý do)` → bảng thông điệp hệ thống (hộp thông báo `0x004ACBD0` của 2.0 chưa có); nhận → `confirmed_number` → `Game.script_input(3, v)`, `confirmed_text` → `script_input(2, 0, chữ)` |
| Callback `0x8468b0` → gói 0x82 | mode 3 → `0x006AC510(giá trị)`: `{0x82, word 7, byte 3, int}` gửi `[0x9bd88c]->vtable[4](buf, 8)`; mode 2 → `0x006AC470(chữ)`: `{0x82, word dài+5, byte 2, word dài, chữ}`; `0x006AC420` `{0x82, 0xf, 1, tham số, giá trị, [0x220fd98]}` = thuế thành. (Chuỗi gọi từ vtable[4] tới hai hàm gửi chưa dò — không có xref trực tiếp.) | `C2G_SCRIPT_INPUT` |
| Ba câu báo `[0x8220bc] [0x8220c0] [0x8220c4]` | nạp lúc khởi động `0x00411DB1..0x00411DF3` từ **`\lang\%s\stringtable_client.txt`** (0x7a4128; `\lang\vn\`): `G_UiGetString_0` = "Số ký tự điền nhập quá ít!" (bản `tw`: 輸入的數位太小！ — số quá nhỏ), `G_UiGetString_1` = "Số ký tự điền nhập quá ít!" (輸入的字串太少！), `G_UiGetString_2` = "Số ký tự điền nhập vượt quá độ dài cho phép!" | `KUiDialogMath.ASK_TEXTS` (Unicode) |
| **`输入字串界面.ini`** (`\Ui\ui3_1024\`, 1006 byte) | `[Main]` 154×119 `\Spr\Ui4\主界面\输入字符串界面\输入字符串底1.spr` tại (0,0); `[Title]` (15,7) 125×14 chữ 12 màu 255,252,178 giữa; `[StringInput]` (10,49) 134×14 chữ 12 màu 255,253,122 `MaxLen 500` `FocusBKColor 25,31,11/180`; `[OkBtn]` "Đồng ý" (20,90) 46×19 `小按钮二字.spr`; `[CancelBtn]` "Hủy bỏ" (90,90). (`\Ui\ui3\` cũ: 122×96 `输入文字条2.spr`.) Một lớp khác cũng nạp ini này tại `0x004F2B01` (đọc `[0x8220c0]/[0x8220c4]` ở `0x004F2C8F`) — luồng khác | `export-ui` `nhap-chuoi` → `UiGetString.gd` |

## 30. Script action không hộp thoại trên client — `OnScriptAction` ui 4 `0x0060124F` / ui 5 `0x00601386`, thông điệp UI 0x1f/0x20/0x22, cửa sổ tin `新闻消息来了.ini`, `SendTaskOrder` 0x52, chat npc 0x2f (S7, đã đọc từng dòng `gamecl.exe` + mã 2004)

Enum `GDCNI_*` của `Core/Src/CoreShell.h` (2004, từ 1) đặt tên thông điệp UI: 0x1f `GDCNI_SYSTEM_MESSAGE`, 0x20 `GDCNI_NEWS_MESSAGE`, 0x22 `GDCNI_OPEN_STORE_BOX`, 0x24 `GDCNI_MISSION_RECORD` (§24), 0x16 `QUESTION_CHOOSE`, 0x17 `SPEAK_WORDS`.

| Chỗ | Đọc được | JX NEXT |
|---|---|---|
| Bảng handler gói (`0x0065DCB0`): `[esi+0x190]` 0x63 → `0x00652120`, `[esi+0x224]` **0x88 → `0x006504C0`**, `[esi+0x2dc]` 0xb6 → `0x00651390` (§26), `[esi+0x3f0]` **0xfb → `0x00655180`** | | |
| **`OnScriptAction` ui 4 `0x0060124F`** | `+0xd` (dài) > 0; chữ hoặc id (`+6` → `g_GetStringRes 0x006C3A50`), qua bảng dịch `0x006C3AB0`; tham số `{chữ[0x104], +0x104 kiểu = 2, +0x105 nháy = 2, +0x106 ưu tiên = 0, +0x107 = 0}` (`0x00601300..`) → `0x005B8150(0x1f, &tham số, 0, 0)` → `0x004297C7` → `0x004C4060` = `KUiSysMsg::OpenWindow` (§26) | `KUiGameWindows._on_script_action` ui 4 → `sys_msg_pane.add_message(text, 2, false, 0)` (nháy ≠ 1 = không nháy) |
| **`OnScriptAction` ui 5 `0x00601386`** | `+5`: 0 → `0x006015B9` (kiểu 0), 1 → `0x0060148A` (kiểu 1: chữ dài `−4`, **word đếm** ở cuối `0x00601596`), 2 → `0x006013AF` (kiểu 2: 4 dword sau chữ = ngày giờ); chữ → dịch + `TEncodeText 0x005B1A56` (0x200); tham số `{kiểu, chữ[0x200], +0x204 dài}` → `0x005B8150(0x20, &tham số, &đếm/ngày, 0)` → `0x0042991F` → `0x00425C80` → `[0x8bf730]->0x004D60E0` | `ScriptAction.param` = kiểu, `.count` = đếm; băng tin chưa vẽ (S7b) |
| **Cửa sổ tin `[0x8bf730]`** (0x724 byte; tạo `0x004D5950`: `new` → ctor `0x004D5850` (cơ sở `0x004D5B30` vtable `0x7953e4`: `+0x4b0 = 8`, `+0x4c4 = +0x4c8 = 4`) → `Init 0x004D6070` → `0x004D5BE0(0)` → `0x004D5DA0`; vtable `0x795444`: `+0x4c 0x004D59F0` = **`新闻消息来了.ini`**, `+0x54 0x004D5AE0` nạp **`\Ui\DefaultMessage.ini`** vào `+0x704`, `+0x50 0x004D5A00` = dòng mặc định ngẫu nhiên `[Main] Count` / `[Main] <n>`, `+0x18 0x004D6230` = Paint, `+0x40 0x004D69B0` = Breathe) | `0x004D60E0(obj, tham số, phụ)`: `+0x204` 1..0x200; kiểu 0 → `0x004D5C20(tham số, 0, 0)`; kiểu 1 → `(tham số, word đếm hay 3, 0)`; kiểu 2 → ngày giờ → `mktime` (`[0x7844ec]`) → `(tham số, thời điểm, 0)`; `0x004D5C20`: nút 0x214 byte `{tham số 0x208, +0x208 đếm/thời điểm, +0x20c số lần đã hiện, +0x210 kế}` chèn **đầu** danh sách `+0x4a0`. Breathe: có tin hiện (`+0x4a4`) → hẹn giờ `0x00450DD0(+0x4d4, +0x6e8)` → bước `0x004D6660` (x `+0x6f4 −=` bước `+0x4c4/+0x4c8` luân phiên, `+0x700` đếm dừng; hết chữ → 1) → `0x004D5C80` (bỏ tin hiện) + `0x004D5CF0` (nối cuối để lặp); không có → `0x004D6860` chọn tin kế (kiểu 0: hiện một lần; kiểu 1: `+0x20c++ ≤ +0x208` số lần; kiểu 2: theo thời điểm; rỗng → sau `+0x4cc` ms dòng mặc định `vfunc+0x50`) → `0x004D6600` bắt đầu (`0x004D63D0`: kiểu 1 tìm `%d` (`0x25` rồi `'d'`) trong chữ → `0x004D6470` điền số còn lại). Paint `0x004D6230`: `[0x9bb0e0]->vtable+0x18(font +0x4b0, khung {x = +0x4ac + +0x1c, …}, chữ từ `+0x6f0`, dài, màu `0xffff8001`)` | **S7b**: `UiNews.gd` từ `tin-toan-cuc.json` + `tin-mac-dinh.json` (đã thêm vào `export-ui`) |
| **Gói 0x88 `0x006504C0`** (`OpenBox` `0x081299F0`) | `0x0060E630(core+0xc0d4, gói)`: `+1` bit0 → `+0x4d38`, bit1 → `+0x4d54`, bit2 → `+0x4d70` (ba trang kho có/không), `+2` bit0 → `+0x4c34 = 1`, bit1 → `+0x4c38`; rồi `0x005B8150(0x22, 0, 0, 0)` → `0x004297DE` → `0x00491ED0()` + `0x00497570()` = mở cửa sổ kho | lát kho riêng |
| **Gói 0xb6 `0x00651390`** (§26) | byte đầu `0x10` → 0x5d `{1, 1, 3, 0}`; khác → **0x52 `{5, 0x12, 3, 0}`** (`0x0042A0C3`) | `TaskTip.kind` 1 → `add_message(text, 5, false, 3)` |
| **Gói 0xfb `0x00655180`** | họ gói chat: `+3 − 0x20` (0..0xf) qua bảng `0x6555bc` → nhảy `0x6555a0`; **0x2f → `0x0065558D`** → `0x00652860(gói, cỡ − 2)`: `+1` dword id npc, `+5` word dài, `+7` chữ; tìm npc theo id trong bảng `[0x1ab34f4]` (0x320 ô × 0x12bd4 byte, id ở `+8`); **`0x005EAAB0(npc, tên +0x1409, chữ, dài, 0x32, 0xa)`** = lời trên đầu (chuỗi `"tên:"` + mã màu 3 + chữ ≤ 0x1ff, `0x005EAA90` lưu vào `npc+0x16ec`, ngắt dòng `[0x739538]` 0x32 cột, giới hạn 0xa dòng; người chơi có `+0x191c` → `"<pic=%d>"`); rồi `[0x9bca44]->vtable[1](1, tên, chữ, dài, 1, 0)` = dòng vào khung chat | `G2C_NPC_CHAT` → `chat_msg` kênh gần với tên npc; bong bóng: lát "lời nói trên đầu" |
