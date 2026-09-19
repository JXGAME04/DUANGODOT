package export

// The old client draws every window from an .ini: one section per widget with its rectangle, its
// sprite and, for text, its font and colours.  KUiBase::GetCurSchemePath picks the theme folder,
// each window class loads its own file (KUiLogin::LoadScheme reads <theme>\UiNewLogin\登陆.ini and
// hands each section to KWndEdit / KWndButton / KWndText ::Init).
//
// This file turns such a layout into JSON the Godot client lays out the same way, and writes every
// picture it names.  What it guarantees:
//
//   - The layout is read BY THE NAME THE GAME ASKS FOR.  gamecl.exe of VLTK 2.0 names fourteen
//     files under UiNewLogin\ (docs/VLTK20-CLIENT.md); they live in the nested archive \reslst.dat.
//     An earlier version guessed a window from its section names because it could not find these
//     files at all - and guessed wrong for three of them.
//   - A section is transcribed WHOLE.  Every key goes into `values` under its lower-cased name,
//     with the value decoded to UTF-8, so the client reads a section exactly the way the old
//     KWnd*::Init did and nothing is lost to a field this exporter did not think of.
//   - A picture keeps what the old renderer needed to draw it.  A window image is drawn WITHOUT
//     RUIMAGE_RENDER_FLAG_FRAME_DRAW (IR_InitUiImageRef clears bRenderFlag), so each frame sits
//     at its own offset inside the sprite's box.  A sprite becomes one atlas .png plus, in the
//     JSON, where each frame is in the atlas (x, y, w, h) and where it goes in the box (ox, oy).
//     The first exporter wrote the bare first frame and every such picture landed a few pixels up
//     and to the left; writing each frame on a full-size canvas instead was exact but cost 65 MB
//     and an 800x528 texture for every frame of every animated figure.
//   - Every frame of a sprite is kept.  A button's states are frame NUMBERS (Up, Down,
//     OverFrame - `Over` itself is only the switch), an ornament or a figure is an animation.
//
// Pictures are named after the widget they belong to, in Vietnamese, never after a hash:
// ui/dang-nhap/nut-dang-nhap.png.  files{} in the JSON maps each old game path to what it became.

import (
	"encoding/json"
	"fmt"
	"image/png"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/spr"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// UiScreenDef names one window of the old client: the file its class loads and what we call it.
type UiScreenDef struct {
	Name  string // folder under ui/, Vietnamese without diacritics
	Label string // what to call it in Vietnamese
	Class string // the old C++ class, the name the project owner knows the window by
	File  string // path below the theme folder, exactly as the executable spells it
}

// LoginScreens are the windows of the login flow, in the order the player meets them.  The file
// names are the ones KUi*::LoadScheme of gamecl.exe passes to KIniFile::Load.
var LoginScreens = []UiScreenDef{
	{"nen-dang-nhap", "Nền của luồng đăng nhập", "KUiLoginBackGround", `UiNewLogin\登陆过程背景窗口.ini`},
	{"bat-dau", "Bảng chọn đầu tiên", "KUiInit", `UiNewLogin\开始.ini`},
	{"chon-may-chu", "Chọn máy chủ", "KUiSelServer", `UiNewLogin\选服务器.ini`},
	{"dang-nhap", "Đăng nhập", "KUiLogin", `UiNewLogin\登陆.ini`},
	{"thong-bao-ket-noi", "Thông báo trong lúc kết nối", "KUiConnectInfo", `UiNewLogin\登陆过程提示.ini`},
	{"chon-nhan-vat", "Chọn nhân vật", "KUiSelPlayer", `UiNewLogin\选游戏存档人物.ini`},
	{"tao-nhan-vat", "Tạo nhân vật", "KUiNewPlayer", `UiNewLogin\新建角色.ini`},
	{"chon-tan-thu-thon", "Chọn tân thủ thôn", "KUiSelNativePlace", `UiNewLogin\选新手村.ini`},
	{"ban-phim-ao", "Bàn phím ảo", "KUiVirtualKeyboard", `UiNewLogin\虚拟键盘.ini`},
	{"khuyen-cao", "Khuyến cáo chơi game lành mạnh", "KUiHealthGameNotice", `UiNewLogin\健康游戏公告.ini`},
	{"bang-thong-bao", "Bảng thông báo", "KUiNotice", `UiNewLogin\提示公告界面.ini`},
	{"canh-bao-pk", "Cảnh báo PK", "KUiPKNotice", `UiNewLogin\PK提示.ini`},
	{"kich-hoat-tai-khoan", "Kích hoạt tài khoản", "KUiActiveAccount", `UiNewLogin\ActiveAccount.ini`},
}

// GameScreens are the windows of the game itself that the new client has rebuilt so far, in the
// order the systems came: the bag and the character window with its pages (M11).  The names are
// the ones gamecl.exe 2.0 asks for below the theme folder (its string table; the pages of the
// character window are separate files, "<window>_<page>.ini").
var GameScreens = []UiScreenDef{
	{"tui-do", "Túi đồ", "KUiItem", `随身物品.ini`},
	{"thong-tin-nhan-vat", "Thông tin nhân vật (khung + các trang)", "KUiStatus", `玩家装备与人物状态.ini`},
	{"thong-tin-nhan-vat-trang-bi", "Trang trang bị", "KUiStatus (trang bị)", `玩家装备与人物状态_装备.ini`},
	{"thong-tin-nhan-vat-thuoc-tinh", "Trang thuộc tính", "KUiStatus (thuộc tính)", `玩家装备与人物状态_属性.ini`},
	{"chu-thich-vat-pham", "Chú thích vật phẩm / trình đơn chuột phải", "KUiMouseHover", `弹出说明文字.ini`},
	{"kho-do", "Kho đồ (rương)", "KUiStoreBox", `储物箱.ini`},
	// M12 B4: the skill book and its pages (KUiSkills, KUiFightSkill / KUiFightSkillSubPage, KUiLiveSkill), the
	// skill picker of a mouse button (KUiSkillTree), the main bar (KUiPlayerBar: life, mana, the two mouse skills),
	// the two control bars and the list of states on the character
	{"ky-nang", "Kỹ năng (khung)", "KUiSkills", `技能主窗口.ini`},
	{"ky-nang-chien-dau", "Trang kỹ năng chiến đấu", "KUiFightSkill", `战斗技能分页.ini`},
	{"ky-nang-chien-dau-chi-tiet", "Trang con kỹ năng chiến đấu", "KUiFightSkillSubPage", `战斗技能细分页.ini`},
	{"ky-nang-doi-song", "Trang kỹ năng sinh hoạt", "KUiLiveSkill", `生活技能分页.ini`},
	{"chon-ky-nang", "Cây chọn kỹ năng cho chuột", "KUiSkillTree", `技能选择树.ini`},
	{"thanh-nhan-vat", "Thanh nhân vật (máu, nội, kỹ năng chuột)", "KUiPlayerBar", `玩家信息主界面.ini`},
	{"thanh-nhan-vat-thu-nho", "Thanh nhân vật thu nhỏ", "KUiPlayerBar (nhỏ)", `玩家信息主界面最小化.ini`},
	{"thanh-dieu-khien-tren", "Thanh điều khiển trên", "KUiHeaderControlBar", `顶部控制条.ini`},
	{"thanh-cong-cu", "Thanh công cụ", "KUiToolsControlBar", `工具控制条.ini`},
	{"trang-thai-ky-nang", "Danh sách trạng thái kỹ năng", "KUiSkillStateList", `技能状态列表.ini`},
	// M14 T2: the team window (KUiTeamManage::OpenWindow 0x004ADAF0 loads "%s\队伍管理.ini") and the message box with up
	// to two text buttons (KUiInformation, 提示.ini: [Info], [FirstBtn], [SecondBtn]) the invitations and applications use
	{"to-doi", "Tổ đội", "KUiTeamManage", `队伍管理.ini`},
	{"hop-thoai", "Hộp thông báo hai nút", "KUiInformation", `提示.ini`},
	// M14 G2: the trade window (KUiTrade, gamecl.exe 0x004C02D2 loads "%s\玩家间交易.ini")
	{"giao-dich", "Giao dịch giữa hai người chơi", "KUiTrade", `玩家间交易.ini`},
	// M14 C2: the chat pad (KUiMsgCentrePad 0x004B5E64 loads "%s\消息集合面板_左.ini" / _右): [Channels] Channel0..14 and one
	// [CH_*] section per channel (ShortName, FormatName, TextColor, MenuText, SendMsgInterval, images), the [ChatTab] pages
	{"khung-chat", "Khung chat và các kênh", "KUiMsgCentrePad", `消息集合面板_左.ini`},
	// M13: the npc dialog - the question and its answers (KUiMsgSel, gamecl.exe 0x0051CF66 loads "%s\滚动选择界面.ini") and the
	// one-button pages of a Talk (KUiInformation2, 提示2.ini)
	{"hop-thoai-chon", "Hộp thoại chọn câu trả lời của npc", "KUiMsgSel", `滚动选择界面.ini`},
	{"hop-thoai-mot-nut", "Hộp thoại một nút (các trang npc nói)", "KUiInformation2", `提示2.ini`},
	// M13 D6: Lua Describe (ui 12 of the 0x63 packet: OnScriptAction 0x006007FD -> ui message 0x40 -> 0x00508410 loads
	// "%s\npc描述界面.ini" at 0x00507A87) and the system message pane the 0xb6 packet of TaskTip lands in (0x00651390 -> ui
	// message 0x5d -> 0x004C4060; 0x004C3D1D loads "%s\系统消息.ini", [Main] SysMsgDisappearInterval default 30000 ms)
	{"mo-ta-npc", "Hộp thoại mô tả của npc (Describe)", "KUiNpcDescribe", `npc描述界面.ini`},
	{"thong-diep-he-thong", "Thông điệp hệ thống (nhắc nhiệm vụ)", "KUiSysMsg", `系统消息.ini`},
	// M13 D8: Lua GiveItemUI (ui 0xb of the 0x63 packet: OnScriptAction 0x0060194C -> ui message 0x3e -> 0x00519C80 opens
	// "%s\给予界面.ini"): the box the player puts items into for a npc script
	{"dua-vat-pham", "Đưa vật phẩm cho npc (GiveItemUI)", "KUiGiveItem", `给予界面.ini`},
	// M13 D9: the journal (KUiTaskNote of 2004, Ui/UiCase/UiTaskNote.cpp): Lua AddNote (ui 3 of the 0x63 packet: OnScriptAction
	// 0x00601058 -> ui message 0x24 GDCNI_MISSION_RECORD) adds a line to the personal notes page
	{"nhat-ky", "Nhật ký nhiệm vụ (khung)", "KUiTaskNote", `任务记事.ini`},
	{"nhat-ky-ca-nhan", "Trang ghi chép cá nhân", "KUiTaskNote (cá nhân)", `任务记事-个人记事分页.ini`},
	{"nhat-ky-he-thong", "Trang nhiệm vụ hệ thống", "KUiTaskNote (hệ thống)", `任务记事-系统任务分页.ini`},
	{"nhat-ky-rang-buoc", "Trang trang bị ràng buộc", "KUiTaskNote (ràng buộc)", `任务记事-装备绑定.ini`},
	{"nhat-ky-cap-nhat", "Trang ghi chép cập nhật game", "KUiTaskNote (cập nhật)", `任务记事-游戏更新记录.ini`},
	// M13 D10: Lua AskClientForNumber / AskClientForString (the 0xa3 packet -> the client's 0x00653500): the input box
	{"nhap-chuoi", "Hộp nhập chuỗi cho script (AskClientForString)", "KUiGetString", `输入字串界面.ini`},
}

// UiImage is one picture of the old client, written out.
type UiImage struct {
	GamePath string `json:"game_path"`
	File     string `json:"file"`               // below ui/: "dang-nhap/nut-dang-nhap.png" - a sprite's atlas, or the .jpg as it was
	Width    int    `json:"width"`              // the sprite's box (0 for a copied .jpg)
	Height   int    `json:"height"`             //
	Interval int    `json:"interval,omitempty"` // milliseconds per frame (KImageParam.nInterval)
	// One entry per frame: x, y, w, h cut it out of the atlas, ox, oy place it inside the box.
	Frames []spr.AtlasFrame `json:"frames,omitempty"`
}

// UiWidget is one section of a layout.
type UiWidget struct {
	Name   string              `json:"name"`             // as the file spells it: "BtnOpenAccountList"
	Key    string              `json:"key"`              // lower-cased: what the client looks a widget up by
	Slug   string              `json:"slug"`             // the Vietnamese name its pictures are filed under
	Values map[string]string   `json:"values"`           // every key of the section (lower-cased), decoded
	Images map[string]*UiImage `json:"images,omitempty"` // key that named a picture -> that picture
}

// UiScreen is one window of the old client.
type UiScreen struct {
	Name    string     `json:"name"`
	Label   string     `json:"label"`
	Class   string     `json:"class"`
	Source  string     `json:"source"` // game path and the archive it was read from
	Theme   string     `json:"theme"`  // "ui3_1024"
	Width   int        `json:"width"`  // the screen the theme was drawn for
	Height  int        `json:"height"`
	Widgets []UiWidget `json:"widgets"`
	// The character pictures the select / create windows build by name, keyed "<series>_<sex>_<n>":
	// KUiSelPlayer::GetRoleImageName makes "<PlayerImgPrefix>_<series>_<sex>_<n>.spr".
	Portraits map[string]*UiImage `json:"portraits,omitempty"`
	// Every old game path this window used, and the first file it became: the way back to the client.
	Files map[string]string `json:"files,omitempty"`
}

// uiSlugs gives the sections of the login windows Vietnamese file names.  A section the table
// does not know keeps its own name, lower-cased - still readable, just not translated.
var uiSlugs = map[string]string{
	"main": "nen", "init": "nen-mo-dau", "newplayer": "nen", "selrole": "nen",
	"login2": "nen-chon-nhan-vat", "login3": "nen-3",
	"account": "o-tai-khoan", "password": "o-mat-khau", "name": "o-ten", "namebg": "khung-ten",
	"level": "cap-do", "ok": "nut-xac-dinh", "cancel": "nut-huy", "new": "nut-tao-nhan-vat",
	"del": "nut-xoa", "transfer": "nut-chuyen", "remember": "o-nho-tai-khoan",
	"invisible": "o-dang-nhap-an", "virtualkeyboard": "o-ban-phim-ao",
	"male": "nam", "female": "nu", "pre": "nut-trang-truoc", "next": "nut-trang-sau",
	"gold": "the-kim", "wood": "the-moc", "water": "the-thuy", "fire": "the-hoa", "earth": "the-tho",
	"propertyshow": "mo-ta", "propertybg": "nen-mo-ta", "player": "cho-dung",
	"playerinfobg": "khung-thong-tin", "versiontext": "phien-ban", "setting": "cai-dat",
	"healthgame": "khuyen-cao", "limit16yearsold": "nhan-do-tuoi", "lifetime": "thoi-han",
	"refuselogin": "bao-loi", "entergame": "nut-bat-dau", "gameconfig": "nut-tuy-chon",
	"openrep": "nut-xem-lai", "exitgame": "nut-thoat", "kingsoft": "ban-quyen",
	// KUiSelServer::GetList of gamecl.exe: LeftList takes the first 14 regions, RightList the rest,
	// IpList the servers of the region that is picked
	"leftlist": "danh-sach-cum", "rightlist": "danh-sach-cum-cot-2", "iplist": "danh-sach-may-chu",
	"list": "danh-sach", "namebigger": "ten-cum", "scroll": "thanh-cuon", "scroll_btn": "con-truot",
	"up_btn": "nut-len", "down_btn": "nut-xuong", "btnopenaccountlist": "nut-mo-danh-sach",
	"accountlist": "danh-sach-tai-khoan", "agree": "o-dong-y", "selserver": "nut-doi-may-chu",
	"runingimgbg": "khung-thong-bao", "confirmbtn": "nut-quay-lai", "continuebtn": "nut-tiep-tuc",
	"cancelbtn": "nut-huy-ket-noi", "delrole": "nut-xoa-nhan-vat", "canceldelrole": "nut-thoi-xoa",
	"delrolebgimg": "khung-xoa-nhan-vat", "activecodebgimg": "khung-ma-kich-hoat",
	"activecodeconfirm": "nut-xac-nhan-ma", "placeimg": "anh-thon", "recommendimg": "dau-de-cu",
	"placedesctext": "mo-ta-thon", "close": "nut-dong", "key": "phim",
	"login_butterfly_0": "la-roi", "login_butterfly_1": "trang-tri-1", "login_butterfly_2": "logo",
	"login2_butterfly_0": "trang-tri-2a", "login2_butterfly_1": "la-roi-2", "login2_butterfly_2": "trang-tri-2c",
	"login3_butterfly_0": "trang-tri-3a", "login3_butterfly_1": "la-roi-3", "login3_butterfly_2": "trang-tri-3c",
	"refuse": "nut-tu-choi", "quit": "nut-thoat", "on_off": "nut-bat-tat",
	"messagelist": "danh-sach-tin", "messagescroll": "thanh-cuon-tin", "messagescroll_btn": "con-truot-tin",
	"privacytext": "chinh-sach", "privacyscroll": "thanh-cuon-chinh-sach", "privacyscroll_btn": "con-truot-chinh-sach",
	"usernotify": "dieu-khoan", "privacynotify": "chinh-sach-bao-mat", "usertitle": "tieu-de-dieu-khoan",
	"privacytitle": "tieu-de-chinh-sach", "remembertxt": "chu-nho-tai-khoan", "invisibletxt": "chu-dang-nhap-an",
	"virtualkeyboardtxt": "chu-ban-phim-ao", "agreeinfo": "chu-dong-y", "selserverinfo": "chu-may-chu",
	"servername": "ten-may-chu", "message": "thong-diep", "btnunlock": "nut-mo-khoa", "rolename": "ten-nhan-vat",
	"activecode": "o-ma-kich-hoat", "sprimg": "hang", "3dplayrepwarn": "canh-bao-xem-lai",
	// the bag (随身物品.ini) and the character window (玩家装备与人物状态*.ini)
	"title": "tieu-de", "closebtn": "nut-dong", "itembox": "o-vat-pham", "money": "tien", "moneytitle": "chu-tien",
	"goldcoin": "xu", "goldcointitle": "chu-xu", "goldcoinscore": "diem-xu", "scoretitle": "chu-diem",
	"bindinggold": "kim-dinh", "bindinggoldtitle": "chu-kim-dinh", "getmoneybtn": "nut-goi-tien",
	"opencurrency": "nut-tien", "openstatus": "nut-thu", "decomposeequip": "nut-ra", "makeadvbtn": "nut-loi-rao",
	"markpricebtn": "nut-dinh-gia", "makestallbtn": "nut-rao-ban", "settings": "cai-dat-mau",
	"btnattribpage": "nut-trang-thuoc-tinh", "btnequippage": "nut-trang-trang-bi", "btnjudgepage": "nut-trang-danh-gia",
	"btnmeridianpage": "nut-trang-kinh-mach", "item": "nut-tui-do",
	"cap": "o-non", "weapon": "o-vu-khi", "necklace": "o-day-chuyen", "mask": "o-mat-na", "bangle": "o-ho-uyen",
	"cloth": "o-y-phuc", "sash": "o-dai", "ring1": "o-nhan-1", "ring2": "o-nhan-2", "pendant": "o-ngoc-boi",
	"shoes": "o-giay", "horse": "o-ngua", "mantle": "o-phi-phong", "signet": "o-an", "shipin": "o-trang-suc",
	"seal": "o-an-chien", "factionpendant": "o-trang-suc-mon-phai", "pkvalue": "diem-pk", "translife": "chuyen-sinh",
	"btnlock": "nut-khoa", "btnbinditem": "nut-thao-dinh", "btnlockitem": "nut-khoa-hon",
	"btnhorsemanage": "nut-thu-cuoi", "btntitlemanage": "nut-danh-hieu", "face": "chan-dung", "clickhere": "chu-chon-hinh",
	"luck": "may-man", "prestige": "danh-vong", "worldrank": "hang", "life": "sinh-luc", "mana": "noi-luc",
	"stamina": "the-luc", "status": "trang-thai", "strength": "suc-manh", "vitality": "sinh-khi", "dexterity": "than-phap",
	"energy": "noi-cong", "addstrength": "nut-cong-suc-manh", "addvitality": "nut-cong-sinh-khi",
	"adddexterity": "nut-cong-than-phap", "addenergy": "nut-cong-noi-cong", "exp": "kinh-nghiem",
	"leftdamage": "luc-tay-trai", "rightdamage": "luc-tay-phai", "attack": "chinh-xac", "defense": "ne-tranh",
	"movespeed": "toc-chay", "attackspeed": "toc-danh", "remainpoint": "diem-con", "resistphy": "phong-thuong",
	"resistcold": "phong-bang", "resistlighting": "phong-loi", "resistfire": "phong-hoa", "resistpoison": "phong-doc",
	"mouseoverwnd": "khung-chu-thich", "menu": "trinh-don",
}

// uiScreenSlugs settles the sections that mean something else from one window to the next: [Login]
// is the backdrop picture in the background window, the OK button when choosing a server and the
// login button in the login window.
var uiScreenSlugs = map[string]map[string]string{
	"nen-dang-nhap":      {"login": "nen-chon-may-chu"},
	"chon-may-chu":       {"login": "nut-xac-dinh"},
	"dang-nhap":          {"login": "nut-dang-nhap"},
	"thong-tin-nhan-vat": {"item": "nut-tui-do", "title": "tieu-de"},
	// the equipment page: [Male] / [Female] are the two backdrops, not the sex buttons of KUiNewPlayer
	"thong-tin-nhan-vat-trang-bi": {"male": "nen-nam", "female": "nen-nu"},
	"chu-thich-vat-pham":          {"main": "nen"},
}

// uiSeries / uiSex are the names KUiSelPlayer::GetRoleImageName builds the file name from, in the
// order the game numbers them (series_metal .. series_earth; 0 = male).
var uiSeries = []struct{ key, vi, cn string }{
	{"metal", "kim", "金"}, {"wood", "moc", "木"}, {"water", "thuy", "水"},
	{"fire", "hoa", "火"}, {"earth", "tho", "土"},
}

var uiSex = []struct{ key, vi, cn string }{{"male", "nam", "男"}, {"female", "nu", "女"}}

// uiSeriesSections are the sections of the five element buttons, in the order of uiSeries
// (the table at 0x81054C of gamecl.exe: Gold, Wood, Water, Fire, Earth).
var uiSeriesSections = []string{"gold", "wood", "water", "fire", "earth"}

// pictureExts are what a value has to end in to be taken for a picture.
var pictureExts = map[string]bool{".spr": true, ".jpg": true, ".jpeg": true, ".bmp": true, ".png": true}

func atoiOr(s string, def int) int {
	s = strings.TrimSpace(s)
	// the old KIniFile::GetInteger reads the leading number and ignores what follows ("5000 ;ms")
	end := 0
	for end < len(s) && (s[end] == '-' || s[end] == '+' || (s[end] >= '0' && s[end] <= '9')) {
		end++
	}
	v, err := strconv.Atoi(s[:end])
	if err != nil {
		return def
	}
	return v
}

// uiSlugIn is uiSlug with the window taken into account.
func uiSlugIn(screen, section string) string {
	if s, ok := uiScreenSlugs[screen][section]; ok {
		return s
	}
	return uiSlug(section)
}

func uiSlug(section string) string {
	if s, ok := uiSlugs[section]; ok {
		return s
	}
	keep := strings.Map(func(r rune) rune {
		switch {
		case r >= 'a' && r <= 'z', r >= '0' && r <= '9':
			return r
		case r >= 'A' && r <= 'Z':
			return r + 32
		case r == '_' || r == '-':
			return '-'
		}
		return -1
	}, section)
	if keep == "" {
		return "o"
	}
	return keep
}

// isGamePath reports whether a raw value names a file of the game rather than being text.
func isGamePath(v string) bool {
	if strings.ContainsAny(v, `\/`) && !strings.Contains(v, "://") {
		return true
	}
	return pictureExts[strings.ToLower(filepath.Ext(v))]
}

// DecodeValue turns the bytes of one .ini value into UTF-8.  The Vietnamese client mixes two
// encodings in one file: paths are GBK (the Chinese folder names of the original), what the
// player reads is TCVN3.  A path is recognised by its separators; anything else is TCVN3 when
// every high byte has a TCVN3 meaning, and GBK otherwise (a Chinese label nobody translated).
func DecodeValue(raw string) string {
	high := false
	for i := 0; i < len(raw); i++ {
		if raw[i] >= 0x80 {
			high = true
			break
		}
	}
	if !high {
		return raw
	}
	if isGamePath(raw) {
		return text.GBKToUTF8([]byte(raw))
	}
	if text.IsTCVN3([]byte(raw)) {
		return text.TCVN3ToUTF8([]byte(raw))
	}
	return text.GBKToUTF8([]byte(raw))
}

// uiBuilder carries what one window's export needs.
type uiBuilder struct {
	ex     *Exporter
	screen *UiScreen
	dir    string              // <out>/ui/<folder>
	folder string              // the folder below ui/ the pictures are written to
	done   map[string]*UiImage // game path -> what it became, so a sprite named twice is written once
	taken  map[string]string   // file stem -> the game path that owns it, so two pictures never share a name
}

// UiTheme returns the theme folder the client runs with: \Ui\Setting.ini [Theme] lists them
// ("ui3_800", "ui3_1024") and the game takes the one that fits the resolution.  `want` picks one by
// a fragment of its name ("1024"); empty means the largest.  A JX1 client has no such section and
// its single scheme is \Ui\Ui3.
func (e *Exporter) UiTheme(want string) (path string, width, height int) {
	data, err := e.Set.ReadFile(`\Ui\Setting.ini`)
	if err != nil {
		return `\Ui\Ui3`, 800, 600
	}
	best, bestW, bestH := "", 0, 0
	for _, sec := range parseIniOrdered(data) {
		if sec.name != "theme" {
			continue
		}
		for i := 0; i < atoiOr(sec.values["count"], 0); i++ {
			p := strings.TrimSpace(sec.values[strconv.Itoa(i)+"_path"])
			if p == "" {
				continue
			}
			w, h := 800, 600
			if strings.Contains(p, "1024") {
				w, h = 1024, 768
			}
			if want != "" && !strings.Contains(strings.ToLower(p), strings.ToLower(want)) {
				continue
			}
			if w*h > bestW*bestH {
				best, bestW, bestH = p, w, h
			}
		}
	}
	if best == "" {
		return `\Ui\Ui3`, 800, 600
	}
	return `\Ui\` + best, bestW, bestH
}

// UiByName exports one window: the file <theme>\<def.File>, the name the game asks for.
func (e *Exporter) UiByName(def UiScreenDef, theme string, width, height int) (*UiScreen, error) {
	gbk, err := text.UTF8ToGBK(theme + `\` + def.File)
	if err != nil {
		return nil, err
	}
	gamePath := string(gbk)
	f, entry, ok := e.Set.Lookup(gamePath)
	if !ok {
		return nil, fmt.Errorf("khong co %s trong client nay", theme+`\`+def.File)
	}
	data, err := f.Read(entry)
	if err != nil {
		return nil, err
	}
	source := fmt.Sprintf(`%s\%s (%s)`, theme, def.File, filepath.Base(strings.ReplaceAll(f.Path, `\`, "/")))
	return e.ui(def, source, strings.TrimPrefix(theme, `\Ui\`), width, height, data)
}

// Ui exports the window in `data`, the raw bytes of a layout .ini.  Kept for callers and tests
// that already hold the bytes; UiByName is the normal way in.
func (e *Exporter) Ui(name, label, source string, data []byte) (*UiScreen, error) {
	w, h := CanvasOf(data)
	if w <= 0 || h <= 0 {
		w, h = 800, 600
	}
	return e.ui(UiScreenDef{Name: name, Label: label}, source, "", w, h, data)
}

func (e *Exporter) ui(def UiScreenDef, source, theme string, width, height int, data []byte) (*UiScreen, error) {
	sections := parseIniOrdered(data)
	if len(sections) == 0 {
		return nil, fmt.Errorf("ui %s: khong co section nao", def.Name)
	}
	b := &uiBuilder{
		ex:     e,
		dir:    filepath.Join(e.Out, "ui", def.Name),
		folder: def.Name,
		done:   map[string]*UiImage{},
		screen: &UiScreen{
			Name: def.Name, Label: def.Label, Class: def.Class, Source: source, Theme: theme,
			Width: width, Height: height, Files: map[string]string{},
		},
	}
	if err := os.MkdirAll(b.dir, 0o755); err != nil {
		return nil, err
	}
	for _, sec := range sections {
		w := UiWidget{Name: sec.raw, Key: sec.name, Slug: uiSlugIn(def.Name, sec.name), Values: map[string]string{}}
		for _, k := range sec.order {
			raw := sec.values[k]
			w.Values[k] = DecodeValue(raw)
			// A picture is any value that ends like one.  The key says what it is for ("image",
			// "sprimg" of a list row, "keyshift" of the keyboard) and names the file.
			if !pictureExts[strings.ToLower(filepath.Ext(raw))] || strings.Contains(raw, "%") {
				continue
			}
			slug := w.Slug
			if k != "image" {
				slug += "-" + uiSlug(k)
			}
			if img := b.picture(slug, raw); img != nil {
				if w.Images == nil {
					w.Images = map[string]*UiImage{}
				}
				w.Images[k] = img
			}
		}
		// KUiNewPlayer builds the picture that names the picked element in code (0x4976C0 of
		// gamecl.exe: sprintf("%s\%svn.spr", PropertyBgImgPrefix, 金|木|水|火|土)); filed under the
		// section names of the five elements so the client asks for image("PropertyBg", "Gold").
		if p := sec.values["propertybgimgprefix"]; p != "" {
			for i, series := range uiSeries {
				cn, err := text.UTF8ToGBK(series.cn)
				if err != nil {
					continue
				}
				if img := b.picture("ten-he-"+series.vi, p+`\`+string(cn)+"vn.spr"); img != nil {
					if w.Images == nil {
						w.Images = map[string]*UiImage{}
					}
					w.Images[uiSeriesSections[i]] = img
				}
			}
		}
		if p := sec.values["playerimgprefix"]; p != "" && b.screen.Portraits == nil {
			b.screen.Portraits = b.portraits(p)
			for _, img := range b.screen.Portraits {
				b.screen.Files[img.GamePath] = img.File
			}
		}
		b.screen.Widgets = append(b.screen.Widgets, w)
	}

	blob, err := json.MarshalIndent(b.screen, "", "  ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(b.dir, "bo-cuc.json"), blob, 0o644); err != nil {
		return nil, err
	}
	log.Info("asset", "ui screen exported", log.F("name", def.Name), log.F("class", def.Class), log.F("source", source),
		log.F("widgets", len(b.screen.Widgets)), log.F("pictures", len(b.done)),
		log.F("screen", fmt.Sprintf("%dx%d", width, height)))
	return b.screen, nil
}

// picture writes what a game path names: a sprite as an atlas of its frames, a .jpg as it is.
func (b *uiBuilder) picture(slug, gamePath string) *UiImage {
	if img, ok := b.done[gamePath]; ok {
		return img
	}
	shown := text.GBKToUTF8([]byte(gamePath))
	img := &UiImage{GamePath: shown}
	slug = b.freeStem(slug, gamePath)
	if strings.ToLower(filepath.Ext(gamePath)) != ".spr" {
		data, err := b.ex.Set.ReadFile(gamePath)
		if err != nil {
			log.Warn("asset", "ui picture missing", log.F("path", shown), log.F("error", err))
			b.done[gamePath] = nil
			return nil
		}
		name := slug + strings.ToLower(filepath.Ext(gamePath))
		if err := os.WriteFile(filepath.Join(b.dir, name), data, 0o644); err != nil {
			log.Error("asset", "ui picture write failed", log.F("file", name), log.F("error", err))
			return nil
		}
		img.File = b.folder + "/" + name
	} else {
		s := b.ex.uiSprite(gamePath)
		if s == nil || len(s.Frames) == 0 {
			b.done[gamePath] = nil
			return nil
		}
		atlas, meta := s.PackAtlas(shown)
		name := slug + ".png"
		out, err := os.Create(filepath.Join(b.dir, name))
		if err == nil {
			err = png.Encode(out, atlas)
			if cerr := out.Close(); err == nil {
				err = cerr
			}
		}
		if err != nil {
			log.Error("asset", "ui picture write failed", log.F("file", name), log.F("error", err))
			return nil
		}
		img.File = b.folder + "/" + name
		img.Width, img.Height, img.Interval, img.Frames = s.Width, s.Height, s.Interval, meta.Frames
	}
	b.ex.Exported++
	b.done[gamePath] = img
	b.screen.Files[shown] = img.File
	return img
}

// freeStem returns a file stem nobody else in this window uses: the slug itself, or slug-2, slug-3...
// when two sections translate to the same words.
func (b *uiBuilder) freeStem(slug, gamePath string) string {
	if b.taken == nil {
		b.taken = map[string]string{}
	}
	stem := slug
	for n := 2; ; n++ {
		owner, used := b.taken[stem]
		if !used || owner == gamePath {
			b.taken[stem] = gamePath
			return stem
		}
		stem = fmt.Sprintf("%s-%d", slug, n)
	}
}

// PortraitFolder is where the character figures go: the select and the create window name the
// same thirty sprites, so they are written once and both layouts point there.
const PortraitFolder = "nhan-vat"

// portraits writes every picture the prefix can name: five elements x two sexes x three views,
// the same set the old window could ask for.
func (b *uiBuilder) portraits(prefix string) map[string]*UiImage {
	if done, ok := b.ex.uiPortraits[prefix]; ok {
		return done
	}
	shared := &uiBuilder{ex: b.ex, dir: filepath.Join(b.ex.Out, "ui", PortraitFolder), folder: PortraitFolder,
		done: map[string]*UiImage{}, screen: &UiScreen{Files: map[string]string{}}}
	if err := os.MkdirAll(shared.dir, 0o755); err != nil {
		log.Error("asset", "ui folder", log.F("dir", shared.dir), log.F("error", err))
		return nil
	}
	out := map[string]*UiImage{}
	for _, series := range uiSeries {
		for _, sex := range uiSex {
			for n := 0; n < 3; n++ {
				cn, err := text.UTF8ToGBK(series.cn + "_" + sex.cn)
				if err != nil {
					continue
				}
				path := fmt.Sprintf("%s_%s_%d.spr", prefix, cn, n)
				slug := fmt.Sprintf("vai-%s-%s-%d", series.vi, sex.vi, n)
				if img := shared.picture(slug, path); img != nil {
					out[fmt.Sprintf("%s_%s_%d", series.key, sex.key, n)] = img
				}
			}
		}
	}
	if b.ex.uiPortraits == nil {
		b.ex.uiPortraits = map[string]map[string]*UiImage{}
	}
	b.ex.uiPortraits[prefix] = out
	return out
}

// uiSprite decodes a sprite, caching by game path so a window that names the same picture twice
// only pays for it once.
func (e *Exporter) uiSprite(gamePath string) *spr.Sprite {
	if e.uiCache == nil {
		e.uiCache = map[string]*spr.Sprite{}
	}
	if s, ok := e.uiCache[gamePath]; ok {
		return s
	}
	f, entry, ok := e.Set.Lookup(gamePath)
	if !ok {
		log.Warn("asset", "ui sprite missing", log.F("path", text.GBKToUTF8([]byte(gamePath))))
		e.uiCache[gamePath] = nil
		return nil
	}
	s, err := spr.ReadFromPak(f, entry)
	if err != nil {
		log.Warn("asset", "ui sprite decode failed", log.F("path", text.GBKToUTF8([]byte(gamePath))), log.F("error", err))
		e.uiCache[gamePath] = nil
		return nil
	}
	e.uiCache[gamePath] = s
	return s
}

// UiTable exports one .ini that is DATA rather than a layout - the list of starting villages, the
// texts of the five elements, the ninety login messages - as JSON with every value decoded, and
// writes the pictures it names next to it.
func (e *Exporter) UiTable(name, gamePath string) (int, error) {
	gbk, err := text.UTF8ToGBK(gamePath)
	if err != nil {
		return 0, err
	}
	data, err := e.Set.ReadFile(string(gbk))
	if err != nil {
		return 0, err
	}
	dir := filepath.Join(e.Out, "ui", "du-lieu")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return 0, err
	}
	b := &uiBuilder{ex: e, dir: dir, folder: "du-lieu", done: map[string]*UiImage{}, screen: &UiScreen{Files: map[string]string{}}}
	type section struct {
		Name   string              `json:"name"`
		Values map[string]string   `json:"values"`
		Images map[string]*UiImage `json:"images,omitempty"`
	}
	var out []section
	for _, sec := range parseIniOrdered(data) {
		s := section{Name: sec.raw, Values: map[string]string{}}
		for _, k := range sec.order {
			raw := sec.values[k]
			s.Values[k] = DecodeValue(raw)
			if pictureExts[strings.ToLower(filepath.Ext(raw))] && !strings.Contains(raw, "%") {
				if img := b.picture(name+"-"+uiSlug(sec.name)+"-"+uiSlug(k), raw); img != nil {
					if s.Images == nil {
						s.Images = map[string]*UiImage{}
					}
					s.Images[k] = img
				}
			}
		}
		out = append(out, s)
	}
	blob, err := json.MarshalIndent(map[string]any{"source": gamePath, "sections": out}, "", "  ")
	if err != nil {
		return 0, err
	}
	return len(out), os.WriteFile(filepath.Join(dir, name+".json"), blob, 0o644)
}

// UiNames writes a numbered name table (attribute id -> name) as assets/ui/du-lieu/<name>.json,
// the way UiTable writes a file of the client: {"names": {"28": "weapondamagemin_v", ...}}.
func (e *Exporter) UiNames(name string, names map[int]string) error {
	dir := filepath.Join(e.Out, "ui", "du-lieu")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return err
	}
	out := map[string]string{}
	for id, n := range names {
		out[strconv.Itoa(id)] = n
	}
	blob, err := json.MarshalIndent(map[string]any{"source": "jx_linux_y KMagicDesc (docs/linux/jx_linux_magicattrib.tsv)", "names": out}, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(dir, name+".json"), blob, 0o644)
}

// iniSection keeps the file order, which the old client relies on: the first section is the window
// and the ones after it are drawn in the order they appear.
type iniSection struct {
	raw    string // the name as written
	name   string // lower-cased
	order  []string
	values map[string]string
}

func parseIniOrdered(data []byte) []iniSection {
	var out []iniSection
	var cur *iniSection
	for _, line := range strings.Split(strings.ReplaceAll(string(data), "\r\n", "\n"), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || line[0] == ';' || line[0] == '#' {
			continue
		}
		if line[0] == '[' {
			if end := strings.IndexByte(line, ']'); end > 0 {
				raw := strings.TrimSpace(line[1:end])
				out = append(out, iniSection{raw: raw, name: strings.ToLower(raw), values: map[string]string{}})
				cur = &out[len(out)-1]
			}
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq <= 0 || cur == nil {
			continue
		}
		k := strings.ToLower(strings.TrimSpace(line[:eq]))
		if _, dup := cur.values[k]; !dup {
			cur.order = append(cur.order, k)
		}
		cur.values[k] = strings.TrimSpace(line[eq+1:])
	}
	return out
}

// SectionNames lists the sections of a layout, in file order.
func SectionNames(data []byte) []string {
	secs := parseIniOrdered(data)
	out := make([]string, 0, len(secs))
	for _, s := range secs {
		out = append(out, s.name)
	}
	return out
}

// CanvasOf returns the window size the first section declares (0,0 when it declares none).
func CanvasOf(data []byte) (int, int) {
	secs := parseIniOrdered(data)
	if len(secs) == 0 {
		return 0, 0
	}
	return atoiOr(secs[0].values["width"], 0), atoiOr(secs[0].values["height"], 0)
}

// HasSections reports whether every name is a section of the layout.
func HasSections(data []byte, want []string) bool {
	have := map[string]bool{}
	for _, n := range SectionNames(data) {
		have[n] = true
	}
	for _, n := range want {
		if !have[n] {
			return false
		}
	}
	return true
}

// UiStrings exports a string table of the client - \lang\vn\stringtable_client.txt, "KEY<TAB>text"
// lines in TCVN3 - as one JSON object.  The windows look their fixed texts up in it by key
// (G_STR_SERVERLIST_STATUS1 is the "(Đầy)" that paints a full server red in KUiSelServer).
func (e *Exporter) UiStrings(name, gamePath string) (int, error) {
	data, err := e.Set.ReadFile(gamePath)
	if err != nil {
		return 0, err
	}
	dir := filepath.Join(e.Out, "ui", "du-lieu")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return 0, err
	}
	table := map[string]string{}
	for _, line := range strings.Split(strings.ReplaceAll(string(data), "\r\n", "\n"), "\n") {
		tab := strings.IndexByte(line, '\t')
		if tab <= 0 || strings.HasPrefix(line, "//") {
			continue
		}
		key := strings.TrimSpace(line[:tab])
		if key == "" || key == "key" { // the header row
			continue
		}
		// what the player reads, TCVN3 throughout (a "/" inside "Độ bền: %3d / %3d" is no path),
		// with its trailing spaces: KItem::GetDesc appends "...<color=Metal>Kim " space and all
		value := strings.TrimRight(line[tab+1:], "\t")
		if text.IsTCVN3([]byte(value)) {
			table[key] = text.TCVN3ToUTF8([]byte(value))
		} else {
			table[key] = DecodeValue(value)
		}
	}
	blob, err := json.MarshalIndent(map[string]any{"source": gamePath, "strings": table}, "", "  ")
	if err != nil {
		return 0, err
	}
	return len(table), os.WriteFile(filepath.Join(dir, name+".json"), blob, 0o644)
}
