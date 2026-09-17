#!/usr/bin/env python3
"""Developer launcher for the JX NEXT test system (Windows/Linux, stdlib only).

  python tools/dev.py build            build C++ (Debug), Go binaries, regenerate protocol code
  python tools/dev.py assets [ids]     export maps (default 1) + sprites from the old client into client/assets
  python tools/dev.py start            start zone + gateway, each in its own console window
  python tools/dev.py stop             stop them
  python tools/dev.py status           show what is running / listening
  python tools/dev.py bots [N] [SEC]   run N bots for SEC seconds against the gateway
  python tools/dev.py smoke            zone + gateway + 1 bot (--once), exit 0 when the whole path works
  python tools/dev.py e2e              smoke + the Godot client headless with --auto (login, enter, move)
  python tools/dev.py screenshot       same client run with a window; saves user://logs/auto_*.png
  python tools/dev.py client           launch the Godot client
  python tools/dev.py test             run every test suite (C++ ctest, go test, Godot headless)

Binaries: build/bin/<Config>/jx_zone.exe, build/go/{gateway,jxbot,jxaccount,jxassets}.exe.
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

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
PIDS = os.path.join(BUILD, "dev-pids.json")
EXE = ".exe" if os.name == "nt" else ""
PRESET = os.environ.get("JX_PRESET", "windows-msvc" if os.name == "nt" else "linux-gcc")
CONFIG = os.environ.get("JX_CONFIG", "Debug")


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


def alive(pid: int) -> bool:
    if os.name == "nt":
        out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"], capture_output=True, text=True).stdout
        return str(pid) in out
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
    elif new_console:
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


def cmd_start(new_console: bool = True) -> None:
    for exe in (zone_exe(), go_exe("gateway")):
        if not os.path.exists(exe):
            sys.exit(f"missing {exe} - run: python tools/dev.py build")
    os.makedirs(os.path.join(ROOT, "logs"), exist_ok=True)
    pids = load_pids()
    if any(alive(p) for p in pids.values()):
        print("already running:", pids)
        return
    # the npc level scripts (Lua) live in the old server folder of config/oldgame.local.json
    if "JX_ZONE__SCRIPT_ROOT" not in os.environ and old_server_dir():
        os.environ["JX_ZONE__SCRIPT_ROOT"] = old_server_dir()
    zone = spawn([zone_exe(), "--config", "config/zone.json"], "jx_zone", new_console)
    if not wait_port(17001, 10):
        kill(zone.pid)
        sys.exit("zone did not open port 17001 (see logs/zone.log)")
    gw = spawn([go_exe("gateway"), "-config", "config/gateway.json"], "jx_gateway", new_console)
    if not wait_port(17100, 10):
        kill(gw.pid)
        kill(zone.pid)
        sys.exit("gateway did not open port 17100 (see logs/gateway.log)")
    save_pids({"zone": zone.pid, "gateway": gw.pid})
    print(f"zone pid {zone.pid} :17001, gateway pid {gw.pid} :17100 - client connects to 127.0.0.1:17100")


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
    print(f"zone port 17001: {'open' if port_open(17001) else 'closed'}")
    print(f"gateway port 17100: {'open' if port_open(17100) else 'closed'}")


def cmd_bots(n: int, seconds: int) -> int:
    return subprocess.call([go_exe("jxbot"), "-gateway", "127.0.0.1:17100", "-bots", str(n), "-duration", f"{seconds}s"], cwd=ROOT)


def cmd_smoke() -> int:
    cmd_stop()
    cmd_start(new_console=False)
    try:
        rc = subprocess.call([go_exe("jxbot"), "-gateway", "127.0.0.1:17100", "-once", "-prefix", "smoke"], cwd=ROOT)
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
    # plus the templates the zone's test npcs use (1000 + i, see server/zone/src/main.cpp)
    subprocess.check_call([*jxassets_args(), "export-npcres", *(map_ids or ["1"]),
                           "-templates", "1000,1001,1002,1003", "-out", out], cwd=ROOT)
    print("assets ok")


def godot_headless_exe() -> str:
    exe = godot_exe()
    console = exe.replace("_win64.exe", "_win64_console.exe")
    return console if os.path.exists(console) else exe


def run_client_auto(account: str = "auto1", windowed: bool = False) -> int:
    """Godot client: login -> character -> enter world -> move -> quit(0 on arrival).
    Headless by default; windowed=True renders and saves screenshots to user://logs/auto_*.png."""
    cmd = [godot_headless_exe(), "--path", os.path.join(ROOT, "client")]
    if not windowed:
        cmd.insert(1, "--headless")
    cmd += ["--", "--auto", "--server=127.0.0.1:17100", f"--account={account}", "--password=auto"]
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


def cmd_e2e() -> int:
    """zone + gateway + bot --once + Godot client --auto; exit 0 when both clients made it."""
    cmd_stop()
    cmd_start(new_console=False)
    try:
        rc = subprocess.call([go_exe("jxbot"), "-gateway", "127.0.0.1:17100", "-once", "-prefix", "smoke"], cwd=ROOT)
        print("BOT OK" if rc == 0 else f"BOT FAILED ({rc})")
        rc2 = run_client_auto()
        print("CLIENT OK" if rc2 == 0 else f"CLIENT FAILED ({rc2})")
        return rc or rc2
    finally:
        cmd_stop()


def cmd_test() -> int:
    rc = subprocess.call(["ctest", "--preset", f"{PRESET}-{CONFIG.lower()}"], cwd=ROOT)
    rc |= subprocess.call(["go", "test", "./..."], cwd=os.path.join(ROOT, "services"))
    rc |= subprocess.call([godot_exe(), "--headless", "--path", os.path.join(ROOT, "client"), "-s", "tests/run.gd"], cwd=ROOT)
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
    elif cmd == "bots":
        sys.exit(cmd_bots(int(args[1]) if len(args) > 1 else 5, int(args[2]) if len(args) > 2 else 30))
    elif cmd == "smoke":
        sys.exit(cmd_smoke())
    elif cmd == "client":
        cmd_client()
    elif cmd == "assets":
        cmd_assets(args[1:])
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
