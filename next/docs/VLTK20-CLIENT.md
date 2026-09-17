# Client VLTK 2.0 — những gì mổ nhị phân cho biết

Mọi điều dưới đây **đọc ra từ nhị phân hoặc đo trên tiến trình đang chạy**, có địa chỉ kèm theo để
kiểm lại. Không có dòng nào là phỏng đoán. Công cụ: [`tools/re`](../tools/re/README.md).

Cây mổ: `C:\Users\<you>\Level Up Games\Vo Lam Truyen Ky 2.0` — `gamecl.exe` (UPX, PDB gốc
`D:\newBuilder\projects\jxvn20\code\product\win32\client\`), `engineFree.dll` (không nén).

## 1. Kho thứ 14: `\reslst.dat`

**Sai lầm cũ.** Phiên 2026‑09‑17 sáng kết luận "client 2.0 không có `.ini` cho màn đăng nhập, ba màn
đó là Flash". Sai. `update.swf` chỉ là cửa sổ **launcher** (tin tức, thanh tiến trình, nút Start).
Màn đăng nhập là cửa sổ C++ như JX1, và tệp bố cục của nó **có** — trong một kho mà bộ đọc của ta
không biết.

**Bằng chứng.** `filetrace.py` ghi lại 538 lần game mở tệp lúc khởi động:

```text
14  pak#11 id=86cd9466 elem=7 size=13477978   \reslst.dat
23  pak#13 id=d428f04c elem=8419 size=349     \Ui\ui3_1024\vn\fontsetting.ini
    pak#13 id=67b6e835 elem=3875 size=1098    \Ui\ui3_1024\UiNewLogin\开始.ini
```

Game chỉ mở 13 tệp `.pak` (kho #0..#12) nhưng tìm thấy 232 tệp ở **kho #13**. Kho đó chính là
`\reslst.dat`: một tệp `PACK` hoàn chỉnh, 13,4 MB, **nằm bên trong `font.pak`**, được
`engineFree.dll` mở ngay sau danh sách kho và tra **sau cùng** (nên tệp vá trong `slistcl.pak`,
`update.pak` vẫn thắng).

| Trong `reslst.dat` | Số mục | Cỡ |
|---|---:|---:|
| Script Lua phía client | 6 738 | 19,7 MB |
| Bố cục / cấu hình `.ini` | 1 161 | 4,4 MB |
| Bảng tab (`\settings\item\...`, kỹ năng...) | 840 | 14,2 MB |
| Văn bản khác | 844 | 1,7 MB |
| **Tổng** | **10 267** | (674 mục bị kho vá che) |

Trước khi sửa, `check-trace` báo **225/490 tệp game thấy mà ta không thấy** — gồm mọi bảng
`\settings\item\*`, `\script\player\playerdata.lua`, và 12/14 bố cục đăng nhập. Sau khi sửa:
**khớp 503, thiếu 0, thừa 0**.

## 2. Phần tử lưu theo mảnh (fragment)

Cờ `0x10000000` ở mã nguồn JX1 là `TYPE_FRAME` — "chỉ dùng cho sprite" (`Engine/Src/XPackFile.cpp:44`).
Engine 2.0 dùng **cùng bit đó** cho mọi tệp lớn (`XPackList::ElemIsPackedByFragment`,
`ElemReadFragment`, `ElemGetFragmentCount` trong bảng export của `engineFree.dll`):

```text
u32 số mảnh | u32 vị trí bảng | các mảnh ... | số mảnh x { u32 vị trí, u32 cỡ, u32 cỡ lưu | phương pháp }
```

Vị trí tính từ đầu phần tử. Mỗi mảnh nén riêng, hoặc để nguyên nếu nén không lợi. `\reslst.dat` =
7 mảnh 2 MB: ba mảnh đầu để nguyên, bốn mảnh sau nén UCL. Bộ đọc cũ coi mọi phần tử có cờ này là
sprite nên bỏ qua. Sửa ở `services/pkg/jxold/pak/XPackFile.go` (`readFragments`, `mountNested`,
`OpenBytes`); 5 test, trong đó một test chạy trên client thật.

Hàm băm tên **không đổi** (`g_FileName2Id` @ `engineFree+0x1CDE0` giống hệt `KPakList::FileNameToId`),
và `KPakFile::Open` @ `+0x2DEB0` thử theo thứ tự: tệp trong bộ nhớ (`KMemFile`, do server đẩy xuống
qua gói mạng, xử lý ở `gamecl+0x251560`) → đĩa → các kho.

## 3. Giao diện: thư mục theme và cách đặt cửa sổ

`\Ui\Setting.ini` `[Theme]`: `0_Path=ui3_800`, `1_Path=ui3_1024`. Bố cục nằm ở
`\Ui\<theme>\...`, riêng luồng đăng nhập ở `\Ui\<theme>\UiNewLogin\`. Ảnh thuộc bộ **Ui4**
(`\Spr\Ui4\主界面\登入界面\...`, tên có đuôi `vn`).

`KWndWindow::Init` @ `gamecl+0x6BB50` đọc thêm hai khoá mà JX1 không có:

| `PositionType` | Nghĩa (mã ở `0x46C8B0`..`0x46C970`) |
|---:|---|
| 0 / không ghi | đặt tại `Left,Top`; nếu `PositionByRate=1` thì `x·rộng/800`, `y·cao/600` |
| 1 | **giữa màn hình** — `Left`/`Top` bị bỏ qua |
| 2 / 3 | giữa, dính đáy / dính đỉnh |
| 4 / 5 | giữa, dính trái / dính phải |
| 6 / 7 / 8 / 9 | góc trái‑trên / phải‑trên / trái‑dưới / phải‑dưới |

Mọi cửa sổ đăng nhập đều `PositionType=1`. Ví dụ "Chọn Máy Chủ" 574x396 ở 1024x768 nằm tại
`(512−287, 384−198) = (225, 186)` — khớp ảnh chụp client thật.

## 4. Luồng đăng nhập thật của bản 2.0

Tài khoản được xác thực ở **launcher Level Up** (exe nhận `launcher-address`, `launcher-token`,
`launcher-path`, chuỗi ở `0x7A3F60`). Chạy thẳng `gamecl.exe` thì vào ngay **Chọn Máy Chủ**, rồi
tới ô tài khoản / mật khẩu của chính game. 14 bố cục, tất cả ở `\Ui\ui3_1024\UiNewLogin\`:

| Tệp | Lớp C++ | Section đọc (từ `LoadScheme`) |
|---|---|---|
| `开始.ini` | `KUiInit` | Main, EnterGame, GameConfig, OpenRep, ExitGame (+ `*Border` của từng nút), KingSoft, 3DPlayRepWarn (WarnInfo, Font, TextPos, LastTime, Color) |
| `选服务器.ini` | `KUiSelServer` @ `0x489080` | Main, List, LeftList, RightList, IpList, NameBigger, Scroll*, Login, Cancel |
| `登陆.ini` | `KUiLogin` @ `0x4853F0` | Main, Account, Password, Login, Cancel, Remember(+Txt), Invisible(+Txt), VirtualKeyboard(+Txt), AccountList, BtnOpenAccountList, Agree, AgreeInfo, UserNotify, PrivacyNotify, SelServerInfo, ServerName, SelServer, HaveNotRead*, NotifyUrl |
| `登陆过程背景窗口.ini` | `KUiLoginBackGround` | Init, Login, Login2, Login3 (+ `_Butterfly_0..2`), VersionText, HealthGame, Limit16YearsOld |
| `登陆过程提示.ini` | `KUiConnectInfo` | RuningImgBg, ConfirmBtn, ContinueBtn, Password, ActiveCode(+Confirm, BgImg), DelRole(+BgImg), CancelDelRole, RoleName, BtnUnlock, Message (MsgColor, MsgBorderColor, MsgColor2, MsgRedColor, ColorChangeInterval, Font, Pos, Size) |
| `选游戏存档人物.ini` | `KUiSelPlayer` @ `0x49A040` | SelRole, Ok, Cancel, Del, Transfer, New, Next, Pre, Butterfly, Player, Name, Level, PlayerInfoBg, LimitTime*, RefuseLogin, RefuseRole%d, LifeTime, Setting |
| `新建角色.ini` | `KUiNewPlayer` | NewPlayer (LoginBg, PlayerImgPrefix), Name, NameBg, Male, Female, MaleImg, FemaleImg, OK, Cancel, Container, PropertyShow, PropertyBg (PropertyBgImgPrefix), Gold..Earth |
| `选新手村.ini` | `KUiSelNativePlace` | Main, List, PlaceImg, RecommendImg, Ok, Cancel, PlaceDescText — 8 tân thủ thôn, mô tả tiếng Việt ở `\Settings\NativePlaceList.ini` |
| `虚拟键盘.ini`, `健康游戏公告.ini`, `提示公告界面.ini`, `PK提示.ini`, `ActiveAccount.ini`, `startkat.ini` | | bàn phím ảo, các hộp thông báo |

Chữ tiếng Việt của các nút/nhãn nằm ngay trong `.ini` (TCVN3): *Nhớ tài khoản*, *Đăng nhập ẩn*,
*Bàn phím nhỏ*, *Đổi Server*, *Điều Khoản Sử Dụng*... Thông báo đăng nhập (90 câu) ở
`\Ui\Setting.ini` `[InfoString]`.

## 5. Làm lại

```bash
build/go/jxassets -client "<client 2.0>" census -out build/census     # mọi mục của mọi kho là gì
build/go/jxassets -client "<client 2.0>" find "Ui/ui3_1024/UiNewLogin/登陆.ini"
build/go/jxassets -client "<client 2.0>" check-trace trace.tsv        # đối chiếu với game thật
```
