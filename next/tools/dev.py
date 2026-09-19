#!/usr/bin/env python3
"""Developer launcher for the JX NEXT test system (Windows/Linux, stdlib only).

  python tools/dev.py build            build C++ (Debug), Go binaries, regenerate protocol code
  python tools/dev.py assets [ids]     export maps (default 1) + sprites from the old client into client/assets
  python tools/dev.py lua              convert the old Lua 4 scripts to Lua 5.4 into data/script, then parse them all
  python tools/dev.py start            start zone + gateway, each in its own console window
  python tools/dev.py stop             stop them
  python tools/dev.py status           show what is running / listening
  python tools/dev.py bots [N] [SEC]   run N bots for SEC seconds against the gateway
  python tools/dev.py load [N] [SEC] [hot|spread] [MAPS] [GATEWAYS]   load test: N bots over MAPS maps and GATEWAYS gateways
  python tools/dev.py smoke            zone + gateway + 1 bot (--once), exit 0 when the whole path works
  python tools/dev.py e2e              smoke + the Godot client headless with --auto (login, enter, move)
  python tools/dev.py screenshot       same client run with a window; saves user://logs/auto_*.png
  python tools/dev.py client           launch the Godot client
  python tools/dev.py test             run every test suite (C++ ctest, go test, Godot headless)

Binaries: build/bin/<Config>/jx_zone.exe, build/go/{gateway,jxbot,jxaccount,jxassets,jxrecord}.exe.
State (pids) is kept in build/dev-pids.json.  Logs: logs/zone.log, logs/gateway.log.
"""
from __future__ import annotations

import glob
import json
import os
import shutil
import socket
import subprocess
import sys
import time

# the servers and the client log UTF-8 (Vietnamese names): never let a cp1252 console kill a run
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        try:
            _stream.reconfigure(encoding="utf-8", errors="replace")
        except (ValueError, OSError):
            pass

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
PIDS = os.path.join(BUILD, "dev-pids.json")
EXE = ".exe" if os.name == "nt" else ""
PRESET = os.environ.get("JX_PRESET", "windows-msvc" if os.name == "nt" else "linux-gcc")
CONFIG = os.environ.get("JX_CONFIG", "Debug")
# A second copy of the system on the same machine (another checkout, a worktree) moves every port
# by this much: JX_PORT_OFFSET=1000 -> zone 20001, gateway 20100 / 20102.  Without it two
# checkouts fight over 19001 and the second one reports a zone that "did not open its port".
PORT_OFFSET = int(os.environ.get("JX_PORT_OFFSET", "0"))
ZONE_PORT = 19001 + PORT_OFFSET


def zone_exe() -> str:
    return os.path.join(BUILD, PRESET, "bin", CONFIG, "jx_zone" + EXE)


def go_exe(name: str) -> str:
    return os.path.join(BUILD, "go", name + EXE)


def godot_exe() -> str:
    cand = os.environ.get("JX_GODOT")
    if cand and os.path.exists(cand):
        return cand
    for name in ("godot", "godot4", "godot_console"):
        p = shutil.which(name)
        if p:
            return p
    if os.name == "nt":
        pkg = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Microsoft", "WinGet", "Packages")
        found = sorted(glob.glob(os.path.join(pkg, "GodotEngine.GodotEngine_*", "Godot_v*_win64.exe")))
        if found:
            return found[-1]
    sys.exit("Godot not found (set JX_GODOT)")


def load_pids() -> dict:
    try:
        with open(PIDS, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def save_pids(p: dict) -> None:
    os.makedirs(BUILD, exist_ok=True)
    with open(PIDS, "w", encoding="utf-8") as f:
        json.dump(p, f)


def strip_json_comments(text: str) -> str:
    """The config files carry // comments (nlohmann parses them with ignore_comments); a // inside
    a string ("ws://...") is not a comment."""
    out, in_string, i = [], False, 0
    while i < len(text):
        c = text[i]
        if in_string:
            out.append(c)
            if c == "\\" and i + 1 < len(text):
                out.append(text[i + 1])
                i += 1
            elif c == '"':
                in_string = False
        elif c == '"':
            in_string = True
            out.append(c)
        elif text.startswith("//", i):
            while i < len(text) and text[i] != "\n":
                i += 1
            continue
        else:
            out.append(c)
        i += 1
    return "".join(out)


def zone_config_string(key: str) -> str:
    """One string value of config/zone.json's "zone" object."""
    try:
        with open(os.path.join(ROOT, "config", "zone.json"), encoding="utf-8") as f:
            value = json.loads(strip_json_comments(f.read())).get("zone", {}).get(key, "")
        return value if isinstance(value, str) else ""
    except (OSError, ValueError):
        return ""


def alive(pid: int) -> bool:
    if os.name == "nt":
        # A loaded machine can refuse to start even tasklist ("the paging file is too small"):
        # that is not an answer about the process, so treat it as "still there" and try again.
        for attempt in range(3):
            try:
                out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"], capture_output=True, text=True).stdout
                return str(pid) in out
            except OSError:
                time.sleep(0.5 * (attempt + 1))
        return True
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def kill(pid: int) -> None:
    """Graceful first (the servers flush logs and save players on SIGTERM/SIGBREAK), force after 5 s."""
    if os.name == "nt":
        if not ctrl_break(pid):
            subprocess.run(["taskkill", "/PID", str(pid), "/T"], capture_output=True)   # WM_CLOSE fallback
    else:
        try:
            os.kill(pid, 15)
        except OSError:
            pass
    end = time.time() + 5
    while time.time() < end and alive(pid):
        time.sleep(0.1)
    if alive(pid):
        if os.name == "nt":
            subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)
        else:
            try:
                os.kill(pid, 9)
            except OSError:
                pass


def port_open(port: int, host: str = "127.0.0.1") -> bool:
    with socket.socket() as s:
        s.settimeout(0.3)
        return s.connect_ex((host, port)) == 0


def gateway_healthy(port: int = 0) -> bool:
    """GET /healthz on the gateway's WebSocket door: true only when it can take players now
    (zone link up, not shutting down)."""
    import urllib.request
    port = port or gateway_ports(0)[1]
    try:
        with urllib.request.urlopen(f"http://127.0.0.1:{port}/healthz", timeout=0.5) as r:
            return r.status == 200 and b'"zone_ready":true' in r.read()
    except Exception:
        return False


def wait_healthy(seconds: float, port: int = 0) -> bool:
    end = time.time() + seconds
    while time.time() < end:
        if gateway_healthy(port):
            return True
        time.sleep(0.2)
    return False


def wait_port(port: int, seconds: float) -> bool:
    end = time.time() + seconds
    while time.time() < end:
        if port_open(port):
            return True
        time.sleep(0.2)
    return False


def spawn(cmd: list[str], title: str, new_console: bool) -> subprocess.Popen:
    """Starts a server.  On Windows the child gets its own process group (and its own console,
    visible or hidden) so stop can deliver CTRL_BREAK for a graceful shutdown."""
    kwargs = {"cwd": ROOT}
    if os.name == "nt":
        flags = subprocess.CREATE_NEW_PROCESS_GROUP
        if new_console:
            flags |= subprocess.CREATE_NEW_CONSOLE
            cmd = ["cmd", "/c", f"title {title} && " + subprocess.list2cmdline(cmd)]
        else:
            flags |= subprocess.CREATE_NO_WINDOW
        kwargs["creationflags"] = flags
    if not (os.name == "nt" and new_console):
        # Without this a server that dies outside its own logging - a Go panic, an allocation the
        # operating system refuses - leaves nothing behind at all, and a load test just reports
        # that nobody could log in.
        kwargs["stdout"] = open(os.path.join(ROOT, "logs", title + ".console.log"), "ab")
        kwargs["stderr"] = subprocess.STDOUT
    return subprocess.Popen(cmd, **kwargs)


# Runs in a helper process without a console: attach to the target console and send CTRL_BREAK to
# its process group (jx_zone handles SIGBREAK, the Go gateway handles it as os.Interrupt).
_CTRL_BREAK_HELPER = """
import ctypes, sys
k = ctypes.windll.kernel32
pid = int(sys.argv[1])
k.FreeConsole()
if not k.AttachConsole(pid):
    sys.exit(2)
k.SetConsoleCtrlHandler(None, True)
ok = k.GenerateConsoleCtrlEvent(1, pid)
k.FreeConsole()
sys.exit(0 if ok else 3)
"""


def ctrl_break(pid: int) -> bool:
    res = subprocess.run([sys.executable, "-c", _CTRL_BREAK_HELPER, str(pid)],
                         creationflags=subprocess.CREATE_NO_WINDOW, capture_output=True)
    return res.returncode == 0


def cmd_build() -> None:
    subprocess.check_call([sys.executable, os.path.join(ROOT, "tools", "gen_proto.py"), "--go"])
    subprocess.check_call(["cmake", "--build", "--preset", f"{PRESET}-{CONFIG.lower()}"], cwd=ROOT)
    subprocess.check_call(["go", "build", "-o", os.path.join(BUILD, "go") + os.sep, "./cmd/..."], cwd=os.path.join(ROOT, "services"))
    print("build ok")


def gateway_ports(i: int) -> tuple[int, int]:
    """TCP and WebSocket port of gateway i (0 = the one in config/gateway.json)."""
    return 19100 + PORT_OFFSET + 10 * i, 19102 + PORT_OFFSET + 10 * i


def gateway_data_dir(i: int) -> str:
    return "data/gateway" if i == 0 else f"data/gateway{i + 1}"


def cmd_start(new_console: bool = True, gateways: int = 1) -> None:
    """Start the zone and `gateways` gateway processes.

    Several gateways share one zone on purpose: the zone carries the simulation, a gateway carries
    sockets, and on Windows a socket write is the expensive part (69 % of the gateway's CPU at
    5000 players, measured with pprof).  Each one keeps its own account store, so a load test
    copies the seeded store for the extra ones.
    """
    for exe in (zone_exe(), go_exe("gateway")):
        if not os.path.exists(exe):
            sys.exit(f"missing {exe} - run: python tools/dev.py build")
    os.makedirs(os.path.join(ROOT, "logs"), exist_ok=True)
    pids = load_pids()
    if any(alive(p) for p in pids.values()):
        print("already running:", pids)
        return
    # the npc level scripts (Lua) come from data/script, converted to Lua 5.4 by `dev.py lua`
    if "JX_ZONE__SCRIPT_ROOT" not in os.environ:
        root = lua_script_root()
        if not root:
            print("no converted scripts yet: run `python tools/dev.py lua` for the npc level data")
        if root:
            os.environ["JX_ZONE__SCRIPT_ROOT"] = root
    if port_open(ZONE_PORT):
        sys.exit(f"port {ZONE_PORT} is already taken - another checkout is running its servers. "
                 f"Stop it there (python tools/dev.py stop) or run this one with JX_PORT_OFFSET=1000.")
    zone_cmd = [zone_exe(), "--config", "config/zone.json"]
    if PORT_OFFSET:
        zone_cmd += ["--set", f"zone.port={ZONE_PORT}"]
    # a development server: "?gm ds <lua>" in the chat runs a script action (AddItem(...) and the
    # rest of the api) for whoever types it - never on a server players can reach
    if os.environ.get("JX_GM_CHAT", "1") != "0":
        zone_cmd += ["--set", "zone.gm_chat=true"]
    # JX_ZONE_LOG_LEVEL=trace: the zone's decisions frame by frame (a fight that does nothing, a dropped command)
    if os.environ.get("JX_ZONE_LOG_LEVEL"):
        zone_cmd += ["--log-level", os.environ["JX_ZONE_LOG_LEVEL"]]
    # a checkout without the exported game data (CI, a fresh clone): the zone refuses to start
    # with a map it cannot find, so it gets the built-in flat test world instead, and says so
    map_dir = zone_config_string("map_dir")
    if map_dir and "JX_ZONE__MAP_DIR" not in os.environ and not os.path.isdir(os.path.join(ROOT, map_dir)):
        print(f"no map bundle at {map_dir}: the zone runs the flat test world (python tools/dev.py assets exports the maps)")
        zone_cmd += ["--set", "zone.map_dir="]
    zone = spawn(zone_cmd, "jx_zone", new_console)
    # hosting every map means reading 980 of them before the door opens: 7 s warm, near a minute
    # from a cold file cache
    if not wait_port(ZONE_PORT, 90):
        kill(zone.pid)
        sys.exit(f"zone did not open port {ZONE_PORT} (see logs/zone.log)")
    started = {"zone": zone.pid}
    for i in range(max(1, gateways)):
        tcp, ws = gateway_ports(i)
        cmd = [go_exe("gateway"), "-config", "config/gateway.json"]
        if PORT_OFFSET:
            cmd += ["-set", f"gateway.listen=0.0.0.0:{tcp}",
                    "-set", f"gateway.listen_ws=0.0.0.0:{ws}",
                    "-set", f"gateway.zone=127.0.0.1:{ZONE_PORT}"]
        if i > 0:
            cmd += ["-set", f"gateway.id=gw{i + 1}",
                    "-set", f"gateway.listen=0.0.0.0:{tcp}",
                    "-set", f"gateway.listen_ws=0.0.0.0:{ws}",
                    "-set", f"gateway.data_dir={gateway_data_dir(i)}",
                    "-set", f"gateway.pprof=",
                    "-set", f"log.file=logs/gateway{i + 1}.log"]
        gw = spawn(cmd, f"jx_gateway{i + 1}", new_console)
        # a load test leaves tens of thousands of character files behind, and reading a freshly
        # copied store cold takes a while
        if not wait_port(tcp, 120):
            for pid in started.values():
                kill(pid)
            kill(gw.pid)
            sys.exit(f"gateway {i + 1} did not open port {tcp} (see logs/gateway.log)")
        started[f"gateway{i + 1}"] = gw.pid
    save_pids(started)
    # the gateway answers /healthz only once its zone link is up: wait for that, not just for the port
    if port_open(gateway_ports(0)[1]) and not wait_healthy(20):
        print("warning: gateway is listening but the zone link is not ready (see logs/gateway.log)")
    addrs = " ".join(f"127.0.0.1:{gateway_ports(i)[0]}" for i in range(max(1, gateways)))
    print(f"zone pid {zone.pid} :{ZONE_PORT}, {max(1, gateways)} gateway(s) - client connects to {addrs}")


def cmd_stop() -> None:
    pids = load_pids()
    for name, pid in pids.items():
        if alive(pid):
            kill(pid)
            print(f"stopped {name} ({pid})")
    save_pids({})


def cmd_status() -> None:
    pids = load_pids()
    for name, pid in pids.items():
        print(f"{name}: pid {pid} {'running' if alive(pid) else 'dead'}")
    print(f"zone port {ZONE_PORT}: {'open' if port_open(ZONE_PORT) else 'closed'}")
    print(f"gateway port {gateway_ports(0)[0]}: {'open' if port_open(gateway_ports(0)[0]) else 'closed'}")


def cmd_bots(n: int, seconds: int) -> int:
    return subprocess.call([go_exe("jxbot"), "-gateway", f"127.0.0.1:{gateway_ports(0)[0]}", "-bots", str(n), "-duration", f"{seconds}s"], cwd=ROOT)


def cmd_smoke() -> int:
    cmd_stop()
    cmd_start(new_console=False)
    try:
        rc = subprocess.call([go_exe("jxbot"), "-gateway", f"127.0.0.1:{gateway_ports(0)[0]}", "-once", "-prefix", "smoke"], cwd=ROOT)
        print("SMOKE OK" if rc == 0 else f"SMOKE FAILED ({rc})")
        return rc
    finally:
        cmd_stop()


def cmd_client() -> None:
    subprocess.Popen([godot_exe(), "--path", os.path.join(ROOT, "client")], cwd=ROOT)


def oldgame_config() -> dict:
    """config/oldgame.local.json (gitignored, see config/oldgame.example.json and docs/REFERENCES.md):
    {"client": <folder with package.ini or config.ini>, "server": <folder with package.ini + Settings>}."""
    p = os.path.join(ROOT, "config", "oldgame.local.json")
    if os.path.exists(p):
        with open(p, encoding="utf-8") as f:
            return json.load(f)
    return {}


def old_client_dir() -> str:
    """Reference client folder; with "client_fallback" in the config the result is "ref;fallback" and
    jxassets serves from the fallback only what the reference lacks (it reports those files)."""
    env = os.environ.get("JX_OLD_CLIENT") or oldgame_config().get("client")
    if env:
        fb = oldgame_config().get("client_fallback", "")
        return env + ";" + fb if fb and ";" not in env else env
    for rel in ("../bin/Client", "../../bin/Client"):
        p = os.path.abspath(os.path.join(ROOT, rel))
        if any(os.path.exists(os.path.join(p, ini)) for ini in ("package.ini", "config.ini")):
            return p
    sys.exit("old client data not found (set JX_OLD_CLIENT or config/oldgame.local.json)")


LUA_ROOT_NAMES = ["server1", "binserver", "extra1", "extra2", "3d"]   # "3d": the trap scripts of the 3D test maps (make_map3d.py)


def lua_script_root() -> str:
    """The converted Lua 5.4 tree the zone runs, "a;b" in the same order as old_server_dir().

    Empty when `python tools/dev.py lua` has not been run yet, and then the zone falls back to the
    raw Lua 4 trees, which no longer load: the compatibility layer is gone on purpose.
    """
    roots = []
    for name in LUA_ROOT_NAMES:
        p = os.path.join(ROOT, "data", "script", name)
        if os.path.isdir(os.path.join(p, "script")):
            roots.append(p)
    return ";".join(roots)


def old_server_dir() -> str:
    """Old server folder chain "a;b" (package.ini + pak/maps.pak, Settings, script): the reference server
    first, "server_fallback" for what it lacks (the per-map trap scripts); empty = let jxassets guess."""
    env = os.environ.get("JX_OLD_SERVER") or oldgame_config().get("server", "")
    fb = oldgame_config().get("server_fallback", "")
    return env + ";" + fb if env and fb and ";" not in env else env


def jxassets_args() -> list[str]:
    args = [go_exe("jxassets"), "-client", old_client_dir()]
    if old_server_dir():
        args += ["-server", old_server_dir()]
    return args


def cmd_assets(map_ids: list[str]) -> None:
    """Export map bundles + sprites from the old client into client/assets (default: map 1)."""
    subprocess.check_call(["go", "build", "-o", os.path.join(BUILD, "go") + os.sep, "./cmd/jxassets"], cwd=os.path.join(ROOT, "services"))
    out = os.path.join(ROOT, "client", "assets")
    for mid in map_ids or ["1"]:
        subprocess.check_call([*jxassets_args(), "export-map", mid, "-out", out], cwd=ROOT)
    # npc / character appearance (npcs.txt + Settings/npcres) for the npcs placed on those maps
    # plus the templates the zone's test npcs use (zone.test_npc_templates, see server/zone/src/main.cpp)
    subprocess.check_call([*jxassets_args(), "export-npcres", *(map_ids or ["1"]),
                           "-templates", zone_config_string("test_npc_templates") or "11,42,5,9", "-out", out], cwd=ROOT)
    # the windows of the login flow (layouts, pictures, the game's bitmap fonts, its sentences).
    # They only exist in a VLTK 2.0 client; with another kind of client the Godot client falls back
    # to its plain login, so a failure here is reported and does not stop the rest.
    if subprocess.call([*jxassets_args(), "export-ui", "-out", out], cwd=ROOT) != 0:
        print("export-ui: no login layouts in this client (need the VLTK 2.0 client) - the plain login will be used")
    # the item tables of the old server (settings/item and its version folders), for the zone
    # and for the client's bag window
    if subprocess.call([*jxassets_args(), "export-items", "-out", out], cwd=ROOT) != 0:
        print("export-items: no settings/item in the reference server folder - items stay unavailable")
    # the pictures those tables name (the icons of the bag), out of the client's archives
    elif subprocess.call([*jxassets_args(), "export-item-images", "-out", out], cwd=ROOT) != 0:
        print("export-item-images: the item sprites could not be written - the bag shows names instead")
    # the objects of the ground (ObjData.txt / MoneyObj.txt): what a dropped thing looks like
    if subprocess.call([*jxassets_args(), "export-objdata", "-out", out], cwd=ROOT) != 0:
        print("export-objdata: no settings/obj in the reference server folder - nothing can be dropped")
    # the player tables (level_exp, level_add, stamina.ini, the new-character templates): the
    # zone's KPlayerSet and the gateway's new characters
    if subprocess.call([*jxassets_args(), "export-player", "-out", out], cwd=ROOT) != 0:
        print("export-player: no settings/npc/player in the reference server folder - built-in numbers are used")
    # the skill table (settings/skills.txt): the zone's KSkillManager; the numbers per level come
    # from the converted skill scripts (dev.py lua) at run time, like the JX2 server
    if subprocess.call([*jxassets_args(), "export-skills", "-out", out], cwd=ROOT) != 0:
        print("export-skills: no settings/skills.txt in the reference server folder - skills stay unavailable")
    if subprocess.call([*jxassets_args(), "export-weapon-skill", "-out", out], cwd=ROOT) != 0:
        print("export-weapon-skill: no settings/武器物理攻击对照表.txt in the server folder or the client archives - the built-in basic attacks 1 / 2 are used")
    # the icons of those skills (SkillIcon column) into items/images, for the skill book and the mouse skill slots
    elif subprocess.call([*jxassets_args(), "export-skill-images", "-out", out], cwd=ROOT) != 0:
        print("export-skill-images: the skill icons could not be written - the skill book shows names instead")
    # where the skill book shows each skill (settings/skillui/skillui.txt of the 2.0 client)
    if subprocess.call([*jxassets_args(), "export-skill-ui", "-out", out], cwd=ROOT) != 0:
        print("export-skill-ui: no settings/skillui/skillui.txt in the client's archives - the skill book stays empty")
    if subprocess.call([*jxassets_args(), "export-skill-desc", "-out", out], cwd=ROOT) != 0:
        print("export-skill-desc: no lang/vn/stringtable_core.txt, magicdesc.ini or gamesetting.ini in the client's archives - the skill tip shows the name only")
    # the missile table (settings/missles.txt): the templates the skills fire (the zone's KMissleTable)
    if subprocess.call([*jxassets_args(), "export-missle-res", "-out", out], cwd=ROOT) != 0:
        print("export-missle-res: no settings/missles.txt in the client's archives - the missiles fly unseen")
    if subprocess.call([*jxassets_args(), "export-missles", "-out", out], cwd=ROOT) != 0:
        print("export-missles: no settings/missles.txt in the reference server folder - the skills fire the built-in basic attacks only")
    # the sounds of the skills (ManCastSnd / FMCastSnd) and the missiles (SndFile1..4), from sound.pak
    if subprocess.call([*jxassets_args(), "export-sounds", "-out", out], cwd=ROOT) != 0:
        print("export-sounds: the client's archives hold no sound.pak - the fights stay silent")
    # the state pictures (settings/npcres/状态图形对照表.txt): what a held state (aura, buff) draws on a character
    if subprocess.call([*jxassets_args(), "export-state-gfx", "-out", out], cwd=ROOT) != 0:
        print("export-state-gfx: no settings/npcres/状态图形对照表.txt in the client's archives - states show no picture")
    # the gold monster kinds (settings/npc/NpcGoldTemplate.txt): what a placed monster may revive as
    if subprocess.call([*jxassets_args(), "export-npc-gold", "-out", out], cwd=ROOT) != 0:
        print("export-npc-gold: no settings/npc/NpcGoldTemplate.txt in the reference server folder - no monster turns gold")
    # the weapon -> physical skill table (settings/武器物理攻击对照表.txt): which attack a weapon swings
    if subprocess.call([*jxassets_args(), "export-weapon-skill", "-out", out], cwd=ROOT) != 0:
        print("export-weapon-skill: no settings/武器物理攻击对照表.txt in the reference server folder - the basic attacks 1 / 2 stand in")
    # the wear table (settings/item/AbradeRate.ini): how fast a worn piece loses durability
    if subprocess.call([*jxassets_args(), "export-abrade-rate", "-out", out], cwd=ROOT) != 0:
        print("export-abrade-rate: no settings/item/AbradeRate.ini in the reference server folder - nothing wears")
    # the revive / reference points of every map (settings/revivepos.ini): where a new character is
    # born in its village and where the revive / SetRevPos put a character
    if subprocess.call([*jxassets_args(), "export-revive-pos", "-out", out], cwd=ROOT) != 0:
        print("export-revive-pos: no settings/revivepos.ini in the reference server folder - characters start at the map's spawn point")
    # the eleven factions (settings/faction/门派设定.ini + factionskill.txt): SetFaction of the script api, the
    # camp of a member, the branch pages of the client's skill book
    if subprocess.call([*jxassets_args(), "export-faction", "-out", out], cwd=ROOT) != 0:
        print("export-faction: no settings/faction/门派设定.ini in the reference server folder - no faction can be joined")
    print("assets ok")


def godot_headless_exe() -> str:
    exe = godot_exe()
    console = exe.replace("_win64.exe", "_win64_console.exe")
    return console if os.path.exists(console) else exe


def run_client_auto(account: str = "auto1", windowed: bool = False, server: str = "", extra: list[str] | None = None) -> int:
    """Godot client: login -> character -> enter world -> move -> quit(0 on arrival).
    Headless by default; windowed=True renders and saves screenshots to user://logs/auto_*.png.
    extra: more client arguments ("--2d" = the 2D world view, the default being the 3D / 2.5D one on this branch)."""
    server = server or f"127.0.0.1:{gateway_ports(0)[0]}"
    cmd = [godot_headless_exe(), "--path", os.path.join(ROOT, "client")]
    if not windowed:
        cmd.insert(1, "--headless")
    cmd += ["--", "--auto", f"--server={server}", f"--account={account}", "--password=auto"] + list(extra or [])
    try:
        # the client logs UTF-8 (map and character names); never let the console code page break the run
        res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
    except subprocess.TimeoutExpired:
        print("client auto run timed out")
        return 1
    out = (res.stdout or "") + (res.stderr or "")
    for line in out.splitlines():
        if line.startswith("AUTO_") or '"cat":"auto"' in line or "SCRIPT ERROR" in line:
            print(line)
    if res.returncode != 0:
        print(out[-2000:])
    return res.returncode


def map_index() -> list[dict]:
    """Every exported map with how much is on it, cached in build/map-index.json.

    Reading 980 map.json files takes a few seconds, and a load test wants them sorted by how much
    content they hold: those are the maps a real population would be spread over.
    """
    cache = os.path.join(BUILD, "map-index.json")
    maps_dir = os.path.join(ROOT, "client", "assets", "maps")
    newest = 0.0
    for name in os.listdir(maps_dir) if os.path.isdir(maps_dir) else []:
        f = os.path.join(maps_dir, name, "map.json")
        if os.path.exists(f):
            newest = max(newest, os.path.getmtime(f))
    if os.path.exists(cache) and os.path.getmtime(cache) >= newest:
        with open(cache, encoding="utf-8") as f:
            return json.load(f)
    out = []
    for name in sorted(os.listdir(maps_dir)):
        if not name.isdigit():
            continue
        f = os.path.join(maps_dir, name, "map.json")
        if not os.path.exists(f):
            continue
        with open(f, encoding="utf-8") as fh:
            j = json.load(fh)
        out.append({"id": int(name), "name": j.get("name", ""), "npcs": len(j.get("npcs") or []),
                    "regions": len(j.get("regions") or []), "traps": len(j.get("traps") or [])})
    out.sort(key=lambda m: (-m["npcs"], -m["regions"], m["id"]))
    os.makedirs(BUILD, exist_ok=True)
    with open(cache, "w", encoding="utf-8") as f:
        json.dump(out, f)
    return out


def load_maps(count: int) -> list[int]:
    """The `count` busiest maps: where a real population would be."""
    idx = map_index()
    if not idx:
        return [1]
    return [m["id"] for m in idx[:max(1, count)]]


LOAD_PASSWORD = "botbot"   # jxaccount enforces the old PaySys minimum of 6 characters


def seed_accounts(n: int, maps: list[int], data_dir: str = "data/gateway", first: int = 1,
                  prefix: str = "load", password: str = LOAD_PASSWORD) -> None:
    """Accounts `prefix<first>`..`prefix<first+n-1>` with one character each, spread over `maps`.

    One store per gateway, each holding only the accounts that gateway serves.  Copying one big
    store instead means tens of thousands of files per extra gateway, and Windows answers
    "insufficient system resources" while a load test is already running on the same machine.
    Run with the gateways stopped: jxaccount writes the same files they do.
    """
    subprocess.check_call(["go", "build", "-o", os.path.join(BUILD, "go") + os.sep, "./cmd/jxaccount"],
                          cwd=os.path.join(ROOT, "services"))
    subprocess.check_call([go_exe("jxaccount"), "-data", data_dir, "-n", str(n), "-first", str(first),
                           "-prefix", prefix, "-password", password,
                           "-maps", ",".join(str(m) for m in maps), "seed"], cwd=ROOT)


def show_stats(lines: list[str], title: str, gateway: list[str] | None = None) -> None:
    """Print what the servers measured, as numbers a human reads, not raw JSON."""
    print(f"--- {title}")
    if not lines:
        print("  (the zone logged no stats: it may not have started)")
        return
    rows = [json.loads(x) for x in lines]
    peak = max(rows, key=lambda s: int(s.get("players", 0)))
    worst = max(rows, key=lambda s: float(s.get("tick_ms_p99", 0)))
    print("  zone                players entities   awake   tick avg      p95      p99      max   RSS MB")
    for tag, r in (("most players", peak), ("worst p99  ", worst), ("at the end ", rows[-1])):
        print(f"  {tag} {int(r.get('players', 0)):>9} {int(r.get('entities', 0)):>8} {int(r.get('awake', 0)):>7}"
              f" {float(r.get('tick_ms_avg', 0)):>10.2f} {float(r.get('tick_ms_p95', 0)):>8.2f}"
              f" {float(r.get('tick_ms_p99', 0)):>8.2f} {float(r.get('tick_ms_max', 0)):>8.2f}"
              f" {int(r.get('rss_mb', 0)):>8}")
    if int(peak.get("dropped", 0)):
        print(f"  ticks dropped: {peak['dropped']}")
    busiest = peak.get("workers", "")
    if busiest:
        parts = sorted(busiest.split(), key=lambda w: -float(w.split(":")[1].split("ms")[0]))
        print("  busiest workers at the peak: " + "  ".join(parts[:4]) + f"   (map {peak.get('busiest_map')})")
    if gateway:
        g = max((json.loads(x) for x in gateway), key=lambda s: int(s.get("online", 0)))
        print(f"  gateway      online {int(g.get('online', 0)):>6}   in/s {int(g.get('msg_in_s', 0)):>7}"
              f"   out/s {int(g.get('msg_out_s', 0)):>8}   writes/s {int(g.get('writes_s', 0)):>7}"
              f"   MB out/s {int(g.get('kb_out_s', 0)) / 1024:>6.1f}")
        print(f"               logins {int(g.get('logins', 0)):>6}   failed {int(g.get('login_fails', 0)):>5}"
              f"   kicked {int(g.get('kicks', 0)):>5}   rate kicks {int(g.get('rate_kicks', 0)):>5}"
              f"   timeouts {int(g.get('timeouts', 0)):>5}   frames dropped {int(g.get('dropped', 0)):>5}")


def cmd_load(n: int, seconds: int, scenario: str = "hot", map_count: int = 1, gateways: int = 1) -> int:
    """MASTER SPEC 56-58: N simulated clients, then the numbers the zone and the gateways measured.

    map_count > 1 spreads the population over that many maps (the busiest ones), which is what a
    live server looks like; map_count = 1 is the worst case, everybody in one place.
    gateways > 1 splits the sockets over several gateway processes in front of the one zone.
    """
    cmd_stop()
    maps = load_maps(map_count)
    gateways = max(1, gateways)
    share = n // gateways
    for i in range(gateways):
        count = share if i < gateways - 1 else n - share * (gateways - 1)
        seed_accounts(count, maps, gateway_data_dir(i), first=1 + share * i)
    # Host only the maps this run puts players on.  All 980 cost the zone 1,8 GB, and with the bots
    # on the same machine that is what makes Windows say "the paging file is too small"; the number
    # being measured is the players, not the idle maps.  `dev.py start` still hosts zone.maps.
    os.environ["JX_ZONE__MAPS"] = ",".join(str(m) for m in maps)
    cmd_start(new_console=False, gateways=gateways)
    log_path = os.path.join(ROOT, "logs", "zone.log")
    before = os.path.getsize(log_path) if os.path.exists(log_path) else 0
    gw_path = os.path.join(ROOT, "logs", "gateway.log")
    before_gw = os.path.getsize(gw_path) if os.path.exists(gw_path) else 0
    ramp = max(10, n // 100)   # ~100 logins a second: argon2 is deliberately expensive
    procs, rc = [], 0
    try:
        for i in range(gateways):
            count = share if i < gateways - 1 else n - share * (gateways - 1)
            procs.append(spawn_retry(
                [go_exe("jxbot"), "-gateway", f"127.0.0.1:{gateway_ports(i)[0]}", "-bots", str(count),
                 "-first", str(1 + share * i), "-duration", f"{seconds}s", "-ramp", f"{ramp}s",
                 "-prefix", "load", "-password", LOAD_PASSWORD, "-scenario", scenario,
                 "-attack", "true", "-log-level", "warn"],
                os.path.join(ROOT, "logs", f"jxbot{i + 1}.log")))
        for p in procs:
            rc |= p.wait()
    finally:
        stats = tail_json(log_path, before, '"cat":"zone.tick"')
        gw = tail_json(gw_path, before_gw, '"cat":"gw.stats"')
        for i in range(1, gateways):
            gw += tail_json(os.path.join(ROOT, "logs", f"gateway{i + 1}.log"), 0, '"cat":"gw.stats"')
        cmd_stop()
    # the build configuration is part of the measurement: a Debug zone is several times slower
    show_stats(stats, f"{n} bots, {seconds}s, {scenario}, {len(maps)} map(s), "
                      f"{gateways} gateway(s), ramp {ramp}s, zone {CONFIG}", gw)
    if CONFIG != "Release":
        print(f"  NOTE: this was the {CONFIG} zone; numbers to compare with docs/TESTING.md need JX_CONFIG=Release")
    print("  bot logs: logs/jxbot*.log")
    return rc


def spawn_retry(cmd: list[str], log_path: str, tries: int = 5) -> subprocess.Popen:
    """Start a process, trying again when the machine momentarily cannot start one.

    A load test runs next to whatever else the machine is doing; Windows answers "the paging file
    is too small" for a CreateProcess that succeeds a second later, and losing a five minute test
    run to that is not a measurement.
    """
    last: OSError | None = None
    for attempt in range(tries):
        try:
            return subprocess.Popen(cmd, cwd=ROOT, stdout=open(log_path, "wb"), stderr=subprocess.STDOUT)
        except OSError as err:
            last = err
            print(f"  could not start {os.path.basename(cmd[0])}: {err}; retrying")
            time.sleep(2 * (attempt + 1))
    raise last if last else RuntimeError("spawn failed")


def tail_json(path: str, offset: int, marker: str) -> list[str]:
    """The stats lines a server wrote after `offset` bytes."""
    out = []
    if os.path.exists(path):
        # the servers truncate their log at every start, so an offset from the previous run would
        # skip the whole file
        if os.path.getsize(path) < offset:
            offset = 0
        with open(path, encoding="utf-8", errors="replace") as f:
            f.seek(offset)
            for line in f:
                if marker in line and '"msg":"' in line and "stats" in line:
                    out.append(line.strip())
    return out


def cmd_e2e() -> int:
    """zone + gateway + bot --once + Godot client --auto; exit 0 when both clients made it."""
    cmd_stop()
    cmd_start(new_console=False)
    try:
        rc = subprocess.call([go_exe("jxbot"), "-gateway", f"127.0.0.1:{gateway_ports(0)[0]}", "-once", "-prefix", "smoke"], cwd=ROOT)
        print("BOT OK" if rc == 0 else f"BOT FAILED ({rc})")
        rc2 = run_client_auto()
        print("CLIENT OK" if rc2 == 0 else f"CLIENT FAILED ({rc2})")
        # the same flow through the 2D world view (the 3D / 2.5D one is the default on this branch)
        rc2d = run_client_auto(account="auto2d", extra=["--2d"])
        print("CLIENT 2D OK" if rc2d == 0 else f"CLIENT 2D FAILED ({rc2d})")
        rc2 = rc2 or rc2d
        rc3 = 0
        if port_open(gateway_ports(0)[1]):   # the same client over the WebSocket door (web / mobile build)
            rc3 = run_client_auto(account="autows", server=f"ws://127.0.0.1:{gateway_ports(0)[1]}/ws")
            print("CLIENT WS OK" if rc3 == 0 else f"CLIENT WS FAILED ({rc3})")
        return rc or rc2 or rc3
    finally:
        cmd_stop()


def cmd_lua() -> int:
    """Convert every old Lua 4 script into real Lua 5.4 under data/script, then parse them all.

    One converted folder per reference root, so KScriptCache keeps the same fallback order it has
    for the raw trees.  data/ is gitignored: the converted scripts are generated, like the assets.
    """
    subprocess.check_call(["go", "build", "-o", os.path.join(BUILD, "go") + os.sep, "./cmd/jxlua"],
                          cwd=os.path.join(ROOT, "services"))
    roots = [r for r in old_server_dir().split(";") if r]
    if not roots:
        sys.exit("no reference server folder: set config/oldgame.local.json (see docs/REFERENCES.md)")
    out_roots = []
    for src, name in zip(roots, LUA_ROOT_NAMES):
        src_script = os.path.join(src, "script")
        if not os.path.isdir(src_script):
            print(f"  {src}: no script folder, skipped")
            continue
        out = os.path.join(ROOT, "data", "script", name)
        print(f"== {src_script} -> data/script/{name}/script")
        rc = subprocess.call([go_exe("jxlua"), "convert", "-in", src_script,
                              "-out", os.path.join(out, "script"), "-quiet",
                              "-report", os.path.join(out, "convert-report.json")], cwd=ROOT)
        if rc != 0:
            print("  some files need a human: see convert-report.json")
        out_roots.append(os.path.join(out, "script"))
    if not out_roots:
        sys.exit("nothing to convert")
    check = os.path.join(BUILD, PRESET, "bin", CONFIG, "jx_luacheck" + (".exe" if os.name == "nt" else ""))
    if not os.path.exists(check):
        print(f"skipping the Lua 5.4 parse check: {check} is not built yet")
        return 0
    return subprocess.call([check, *out_roots, "--max-errors", "20"], cwd=ROOT)


def cmd_test() -> int:
    rc = subprocess.call(["ctest", "--preset", f"{PRESET}-{CONFIG.lower()}"], cwd=ROOT)
    rc |= subprocess.call(["go", "test", "./..."], cwd=os.path.join(ROOT, "services"))
    rc |= subprocess.call([godot_exe(), "--headless", "--path", os.path.join(ROOT, "client"), "-s", "tests/run.gd"], cwd=ROOT)
    # the old windows rebuilt from assets/ui: a scene, not "-s", because they need the autoloads
    rc |= subprocess.call([godot_exe(), "--headless", "--path", os.path.join(ROOT, "client"), "tests/UiCheck.tscn"], cwd=ROOT)
    print("ALL TESTS OK" if rc == 0 else "TESTS FAILED")
    return rc


def main() -> None:
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return
    cmd = args[0]
    if cmd == "build":
        cmd_build()
    elif cmd == "start":
        cmd_start()
    elif cmd == "stop":
        cmd_stop()
    elif cmd == "status":
        cmd_status()
    elif cmd == "load":
        sys.exit(cmd_load(int(args[1]) if len(args) > 1 else 50,
                          int(args[2]) if len(args) > 2 else 30,
                          args[3] if len(args) > 3 else "hot",
                          int(args[4]) if len(args) > 4 else 1,
                          int(args[5]) if len(args) > 5 else 1))
    elif cmd == "bots":
        sys.exit(cmd_bots(int(args[1]) if len(args) > 1 else 5, int(args[2]) if len(args) > 2 else 30))
    elif cmd == "smoke":
        sys.exit(cmd_smoke())
    elif cmd == "client":
        cmd_client()
    elif cmd == "assets":
        cmd_assets(args[1:])
    elif cmd == "lua":
        sys.exit(cmd_lua())
    elif cmd == "e2e":
        sys.exit(cmd_e2e())
    elif cmd == "screenshot":
        cmd_stop()
        cmd_start(new_console=False)
        try:
            sys.exit(run_client_auto("shot1", windowed=True))
        finally:
            cmd_stop()
    elif cmd == "test":
        sys.exit(cmd_test())
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
