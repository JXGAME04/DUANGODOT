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
