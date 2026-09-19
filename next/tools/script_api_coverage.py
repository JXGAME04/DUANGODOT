#!/usr/bin/env python3
"""Which script functions of the old Linux server the JX NEXT zone has, and which the scripts still need.

  python tools/script_api_coverage.py [--luamap tools/re/jx_linux_y.luamap.txt] [--scripts data/script]
                                      [--skip PARTNER_] [--out docs/SCRIPT-API.md]

Three lists meet here:
  1. the C functions jx_linux_y registers for Lua: tools/re/jx_linux_y.luamap.txt ("ADDR  Name" lines, the
     registration tables of the binary read by tools/re/re_tables.py / re_luasig.py);
  2. the functions the zone registers: server/zone/src/ScriptFuns.cpp {"Name", l_Name} and the globals
     KLuaScript.cpp sets (Include, IncludeLib, print);
  3. every bare global call `Name(` (not after '.' or ':') in the .lua files under data/script - the Linux
     scripts converted by `python tools/dev.py lua`.
A name the scripts call that the binary registers and the zone does not is "missing": that is the list to
work down (docs/HANDOVER.md).  Names the scripts define themselves (function Name / Name = function) are
left out; Lua's own globals are counted apart; what is left is "unknown" (a Lua 4 builtin the converter
did not rewrite, or a helper defined in a way this scan does not see).
"""
import argparse
import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

LUA_KEYWORDS = {
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local",
    "nil", "not", "or", "repeat", "return", "then", "true", "until", "while",
}
LUA_GLOBALS = {
    "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "load", "loadfile", "loadstring",
    "next", "pairs", "pcall", "print", "rawequal", "rawget", "rawlen", "rawset", "require", "select",
    "setmetatable", "tonumber", "tostring", "type", "unpack", "xpcall", "math", "string", "table", "os", "io",
}
# the Lua 4.0 base library the old server linked (lbaselib/lstrlib/lmathlib of 4.0): a bare call to one of
# these that `dev.py lua` did not rewrite still needs a shim
LUA4_BUILTINS = {
    "strlen", "strsub", "strlower", "strupper", "strrep", "strbyte", "strchar", "strfind", "format", "gsub",
    "tinsert", "tremove", "getn", "sort", "foreach", "foreachi", "abs", "ceil", "floor", "mod", "sqrt", "pow",
    "min", "max", "random", "randomseed", "log", "log10", "exp", "sin", "cos", "tan", "asin", "acos", "atan",
    "atan2", "frexp", "ldexp", "deg", "rad", "date", "clock", "globals", "setglobal", "getglobal", "rawgettable",
    "rawsettable", "call", "dostring", "newtag", "settag", "tag", "settagmethod", "gettagmethod", "copytagmethods",
    "getargs", "openfile", "closefile", "readfrom", "writeto", "appendto", "remove", "rename", "tmpname",
    "read", "write", "exit", "getenv", "execute", "seek", "flush", "setlocale", "gcinfo", "PI",
}


def read_luamap(path):
    out = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"^([0-9A-Fa-f]{8})\s+([A-Za-z_]\w*)\s*$", line)
            if m:
                out.setdefault(m.group(2), m.group(1).upper())
    return out


def read_ours(root):
    out = {}
    src = os.path.join(root, "server", "zone", "src")
    for name in sorted(os.listdir(src)):
        if not name.endswith(".cpp"):
            continue
        with open(os.path.join(src, name), encoding="utf-8", errors="replace") as f:
            text = f.read()
        for m in re.finditer(r'\{\s*"([A-Za-z_]\w*)"\s*,\s*l_\w+\s*\}', text):
            out.setdefault(m.group(1), name)
        for m in re.finditer(r'lua_setglobal\(\s*L_?\s*,\s*"([A-Za-z_]\w*)"\s*\)', text):
            out.setdefault(m.group(1), name)
    return out


def strip_lua(text):
    text = re.sub(r"--\[(=*)\[.*?\]\1\]", " ", text, flags=re.S)   # block comments
    text = re.sub(r"--[^\n]*", " ", text)                          # line comments
    text = re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)              # strings
    text = re.sub(r"'(?:\\.|[^'\\\n])*'", "''", text)
    return text


CALL_RE = re.compile(r"(?<![\w.:])([A-Za-z_]\w*)\s*\(")
DEF_RE = re.compile(r"\bfunction\s+([A-Za-z_]\w*)\s*\(|(?<![\w.:])([A-Za-z_]\w*)\s*=\s*function\b")


def scan_scripts(scripts_root):
    """{name: Counter(file -> uses)}, {defined names}, number of files"""
    uses = collections.defaultdict(collections.Counter)
    defined = set()
    files = 0
    for base, _dirs, names in os.walk(scripts_root):
        for name in names:
            if not name.lower().endswith(".lua"):
                continue
            path = os.path.join(base, name)
            files += 1
            with open(path, "rb") as f:
                text = strip_lua(f.read().decode("latin-1"))
            rel = os.path.relpath(path, scripts_root).replace("\\", "/")
            for m in DEF_RE.finditer(text):
                defined.add(m.group(1) or m.group(2))
            for m in CALL_RE.finditer(text):
                n = m.group(1)
                if n in LUA_KEYWORDS:
                    continue
                uses[n][rel] += 1
    return uses, defined, files


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--luamap", default=os.path.join(ROOT, "tools", "re", "jx_linux_y.luamap.txt"))
    ap.add_argument("--scripts", default=os.path.join(ROOT, "data", "script"))
    ap.add_argument("--skip", action="append", default=[], help="name prefix to leave out of the missing list (e.g. PARTNER_)")
    ap.add_argument("--out", default="", help="write the markdown report here (default: stdout only)")
    ap.add_argument("--top", type=int, default=0, help="only the first N missing functions in the report (0 = all)")
    args = ap.parse_args()

    luamap = read_luamap(args.luamap)
    # the static Lua 4.0 library of the binary (lbaselib/lstrlib/lmathlib/liolib) sits above 0x08230000 in
    # jx_linux_y: tonumber, type, getn, floor ... - Lua's own, not game functions
    lua_lib = {n for n, a in luamap.items() if int(a, 16) >= 0x08230000}
    ours = read_ours(ROOT)
    uses, defined, files = scan_scripts(args.scripts)

    covered, missing, lua, unknown = {}, {}, {}, {}
    for name, per_file in uses.items():
        total = sum(per_file.values())
        if name in ours:
            covered[name] = total
        elif name in lua_lib or name in LUA_GLOBALS or name in LUA4_BUILTINS:
            lua[name] = total
        elif name in luamap:
            missing[name] = total
        elif name in defined:
            continue
        else:
            unknown[name] = total
    skipped = {n for n in missing if any(n.startswith(p) for p in args.skip)}
    work = sorted(((n, c) for n, c in missing.items() if n not in skipped), key=lambda x: (-x[1], x[0]))
    ours_extra = sorted(n for n in ours if n not in luamap)
    binary_unused = sorted(n for n in luamap if n not in uses)

    lines = []
    lines.append("# Hàm script của máy chủ Linux — JX NEXT đã có gì, script còn gọi gì")
    lines.append("")
    lines.append("Sinh bởi `python tools/script_api_coverage.py` (đừng sửa tay). Nguồn: `tools/re/jx_linux_y.luamap.txt` (%d hàm C "
                 "`jx_linux_y` đăng ký cho Lua), `server/zone/src/ScriptFuns.cpp` + `KLuaScript.cpp` (%d hàm zone đăng ký), %d tệp .lua "
                 "dưới `data/script` (bộ script Linux đã chuyển sang Lua 5.4)." % (len(luamap), len(ours), files))
    lines.append("")
    lines.append("| Số đo | Giá trị |")
    lines.append("|---|---|")
    lines.append("| Tên hàm toàn cục mà script gọi | %d |" % len(uses))
    lines.append("| … zone đã có (hàm C đã cài) | %d tên, %d lượt gọi |" % (len(covered), sum(covered.values())))
    lines.append("| … **còn thiếu** (nhị phân có, zone chưa) | **%d tên, %d lượt gọi**%s |" % (
        len(missing), sum(missing.values()),
        (" — trong đó bỏ qua %d tên `%s` (%d lượt)" % (len(skipped), "`/`".join(args.skip), sum(missing[n] for n in skipped))) if skipped else ""))
    lines.append("| … Lua tự có / Lua 4 gốc chưa chuyển | %d tên, %d lượt gọi |" % (len(lua), sum(lua.values())))
    lines.append("| … không rõ (không có trong nhị phân, không thấy định nghĩa trong script) | %d tên, %d lượt gọi |" % (len(unknown), sum(unknown.values())))
    lines.append("| Hàm nhị phân đăng ký mà script không gọi | %d (trong đó %d là thư viện Lua 4 tĩnh ≥ 0x08230000) |" % (
        len(binary_unused), sum(1 for n in binary_unused if n in lua_lib)))
    lines.append("| Hàm zone có mà nhị phân không có (riêng JX NEXT) | %d: %s |" % (len(ours_extra), ", ".join("`%s`" % n for n in ours_extra) or "—"))
    lines.append("")
    lines.append("## Còn thiếu — theo số lượt gọi (làm từ trên xuống)")
    lines.append("")
    lines.append("| # | Hàm | Địa chỉ `jx_linux_y` | Lượt gọi | Tệp | Ví dụ |")
    lines.append("|---|---|---|---|---|---|")
    shown = work if args.top <= 0 else work[:args.top]
    for i, (n, c) in enumerate(shown, 1):
        per_file = uses[n]
        example = min(per_file, key=lambda p: (len(p), p))
        lines.append("| %d | `%s` | `0x%s` | %d | %d | `%s` |" % (i, n, luamap[n], c, len(per_file), example))
    if skipped:
        lines.append("")
        lines.append("Bỏ qua (theo `--skip`): " + ", ".join("`%s` (%d)" % (n, missing[n]) for n in sorted(skipped, key=lambda x: -missing[x])))
    lines.append("")
    lines.append("## Lua tự có / Lua 4 gốc mà script còn gọi trần (cần shim nếu là Lua 4)")
    lines.append("")
    lines.append(", ".join("`%s` (%d)" % (n, c) for n, c in sorted(lua.items(), key=lambda x: (-x[1], x[0]))) or "—")
    lines.append("")
    lines.append("## Không rõ (xem lại tay: định nghĩa động, chuyển đổi, hay lỗi script gốc)")
    lines.append("")
    lines.append(", ".join("`%s` (%d, ví dụ `%s`)" % (n, c, min(uses[n], key=lambda p: (len(p), p)))
                           for n, c in sorted(unknown.items(), key=lambda x: (-x[1], x[0]))) or "—")
    lines.append("")
    lines.append("## Zone đã có (%d)" % len(covered))
    lines.append("")
    lines.append(", ".join("`%s` (%d)" % (n, c) for n, c in sorted(covered.items(), key=lambda x: (-x[1], x[0]))) or "—")
    lines.append("")
    report = "\n".join(lines) + "\n"
    if args.out:
        with open(args.out, "w", encoding="utf-8", newline="\n") as f:
            f.write(report)
    print("script_api_coverage: %d files; called %d names: covered %d (%d uses), missing %d (%d uses, %d skipped), lua %d, unknown %d%s" % (
        files, len(uses), len(covered), sum(covered.values()), len(missing), sum(missing.values()), len(skipped), len(lua), len(unknown),
        (" -> " + args.out) if args.out else ""))
    for n, c in work[:40]:
        print("  %-36s 0x%s %5d uses in %3d files" % (n, luamap[n], c, len(uses[n])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
