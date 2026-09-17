# KLoginServer - the regions and servers the player chooses from, and what was chosen last time
# (KLogin::GetServerRegionList / GetServerList / LoadLoginChoice / SaveLoginChoice of
# S3Client\Login\Login.cpp; struct KLoginServer { char Title[32]; unsigned char Address[4]; }).
#
# The old client read \Settings\ServerList.ini; the 2.0 client downloads the same file (XOR 0x32)
# to \UserDataCl\serverlist.ini from the ServerListUrl of \settings\autoupdate.ini:
#     [List]      AdviceRegion=0  RegionCount=6  Region_0=Máy chủ mới ...
#     [Region_1]  Count=20  0_Title=Tung Sơn  0_Address=a.b.c.d ...
# A region without a name and a server without a title or an address are skipped, exactly like
# the loops of Login.cpp, so the counts in the file may be larger than what is listed.
#
# Ours is the same shape as JSON, and an address is whatever KNetAddress understands
# ("host:port", "tls://host:port", "wss://host/ws"):
#     {"advice_region": 0, "regions": [{"title": "...", "servers": [{"title": "...", "address": "..."}]}]}
# It is looked for in user://serverlist.json (a list that was downloaded or edited on this machine)
# and then in res://config/serverlist.json (what the build ships with).
#
# What the player picked is kept in user://UiCommon.cfg, section [Login] - the UiCommon.ini of the
# old client, same keys: SelServerRegion, LastGameServer, LastAccount.  A password is never kept.
extends RefCounted

const USER_LIST := "user://serverlist.json"
const SHIPPED_LIST := "res://config/serverlist.json"
const CHOICE_FILE := "user://UiCommon.cfg"
const CHOICE_SECTION := "Login"

const MAX_RECENT_ACCOUNTS := 6     # RecentAccount0..5 of the 2.0 client's [AccountList]

var regions: Array = []            # [{title: String, servers: [{title, address}]}]
var advice_region := 0
var source := ""
var links := {}                    # user_notify_url, privacy_notify_url: what the agreement line opens


# Reads the list; `extra_address` (the --server= argument, JX_SERVER) becomes a region of its own
# at the top so a developer reaches any gateway without editing a file.
# `list_file` (the --serverlist= argument) replaces both standard places: tests and screenshots
# bring their own list without touching the player's.
static func load_list(extra_address: String = "", list_file: String = ""):
	var script: GDScript = load("res://net/KLoginServer.gd")
	var list = script.new()
	for path in ([list_file] if list_file != "" else [USER_LIST, SHIPPED_LIST]):
		if not FileAccess.file_exists(path):
			continue
		var f := FileAccess.open(path, FileAccess.READ)
		if f == null:
			continue
		var data = JSON.parse_string(f.get_as_text())
		f.close()
		if data is Dictionary and list.take(data) > 0:
			list.source = path
			break
	if extra_address.strip_edges() != "":
		list.regions.push_front({"title": "Dòng lệnh", "servers": [{"title": extra_address.strip_edges(), "address": extra_address.strip_edges()}]})
		list.advice_region = 0
	return list


# Takes a parsed list; returns how many servers it holds.
func take(data: Dictionary) -> int:
	regions = []
	var total := 0
	for r in data.get("regions", []):
		if not (r is Dictionary) or str(r.get("title", "")).strip_edges() == "":
			continue
		var servers: Array = []
		for s in r.get("servers", []):
			if not (s is Dictionary):
				continue
			var title := str(s.get("title", "")).strip_edges()
			var address := str(s.get("address", "")).strip_edges()
			if title == "" or address == "":
				continue
			servers.append({"title": title, "address": address})
		regions.append({"title": str(r["title"]).strip_edges(), "servers": servers})
		total += servers.size()
	advice_region = clampi(int(data.get("advice_region", 0)), 0, maxi(regions.size() - 1, 0))
	links = {}
	for key in ["user_notify_url", "privacy_notify_url"]:
		var url := str(data.get(key, "")).strip_edges()
		if url.begins_with("https://") or url.begins_with("http://"):
			links[key] = url
	return total


func region_titles() -> Array:
	var out: Array = []
	for r in regions:
		out.append(r["title"])
	return out


func server_titles(region: int) -> Array:
	var out: Array = []
	if region >= 0 and region < regions.size():
		for s in regions[region]["servers"]:
			out.append(s["title"])
	return out


func server(region: int, index: int) -> Dictionary:
	if region < 0 or region >= regions.size():
		return {}
	var servers: Array = regions[region]["servers"]
	if index < 0 or index >= servers.size():
		return {}
	return servers[index]


# The index of the server of that title in a region, -1 when it is not there.
func find_server(region: int, title: String) -> int:
	var titles := server_titles(region)
	for i in titles.size():
		if titles[i] == title:
			return i
	return -1


# ---- what was picked last time (m_Choices of KLogin)

static func load_choice() -> Dictionary:
	var cfg := ConfigFile.new()
	cfg.load(CHOICE_FILE)
	var recent: Array = []
	for i in MAX_RECENT_ACCOUNTS:
		var name := str(cfg.get_value("AccountList", "RecentAccount%d" % i, ""))
		if name != "":
			recent.append(name)
	return {
		"region": int(cfg.get_value(CHOICE_SECTION, "SelServerRegion", -1)),
		"server": str(cfg.get_value(CHOICE_SECTION, "LastGameServer", "")),
		"account": str(cfg.get_value(CHOICE_SECTION, "LastAccount", "")),
		"remember": bool(cfg.get_value(CHOICE_SECTION, "RememberAccount", false)),
		"invisible": bool(cfg.get_value(CHOICE_SECTION, "LastInvisible", false)),
		"agree": bool(cfg.get_value(CHOICE_SECTION, "HaveReadUserNotify", false)),
		"recent_accounts": recent,
	}


# Keeps what `choice` names.  "recent" is an account that just logged in: it moves to the top of
# the recent list, which is only kept while "remember" is on (accounts, never passwords).
static func save_choice(choice: Dictionary) -> void:
	var cfg := ConfigFile.new()
	cfg.load(CHOICE_FILE)
	for pair in [["region", "SelServerRegion"], ["server", "LastGameServer"], ["account", "LastAccount"],
			["remember", "RememberAccount"], ["invisible", "LastInvisible"], ["agree", "HaveReadUserNotify"]]:
		if choice.has(pair[0]):
			cfg.set_value(CHOICE_SECTION, pair[1], choice[pair[0]])
	if choice.has("recent") and bool(cfg.get_value(CHOICE_SECTION, "RememberAccount", false)):
		var recent: Array = [str(choice["recent"])]
		for i in MAX_RECENT_ACCOUNTS:
			var name := str(cfg.get_value("AccountList", "RecentAccount%d" % i, ""))
			if name != "" and not recent.has(name):
				recent.append(name)
		for i in MAX_RECENT_ACCOUNTS:
			cfg.set_value("AccountList", "RecentAccount%d" % i, recent[i] if i < recent.size() else "")
	elif choice.has("remember") and not bool(choice["remember"]):
		for i in MAX_RECENT_ACCOUNTS:
			cfg.set_value("AccountList", "RecentAccount%d" % i, "")
	var err := cfg.save(CHOICE_FILE)
	if err != OK:
		Log.warn("ui", "login choice not saved", {"file": CHOICE_FILE, "error": error_string(err)})
