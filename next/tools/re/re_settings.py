#!/usr/bin/env python3
"""Which settings files a JX server binary reads, from where, and which columns / keys it takes.

  re_settings.py <elf> list                 every \\settings\\ path the binary names
  re_settings.py <elf> file <text>          the functions that name a file containing <text> and
                                            the string constants those functions use (column and
                                            key names of KTabFile::Get* / KIniFile::Get*)
  re_settings.py <elf> all <out.md>         the same for every settings file, as one document

How it works: the path is a string constant; the code that opens the file mentions the string
(`mov [esp+4], <va>`), so the function around that mention is the loader.  A loader reads columns
by name - `GetInteger(row, "Level", ...)` - so the other string constants the same function
mentions are the columns and keys.  A function that only forwards the path (a wrapper) has no
column strings; then the columns are in the callees, and `file` prints those too, one level down.
Strings with '%' or a path separator are formats and other files, not columns; they are left out.
"""
import re
import sys

sys.path.insert(0, __import__("os").path.dirname(__file__))
import re_elf  # noqa: E402

from capstone.x86 import X86_OP_IMM, X86_OP_MEM  # noqa: E402

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

SETTINGS = re.compile(r"(?i)\\settings\\|^settings\\")
COLUMN = re.compile(r"^[A-Za-z_][A-Za-z0-9_ ]{0,39}$|^[\x81-\xfe]")   # ascii identifier, or a Chinese name


def settings_paths(elf):
    out = []
    for va, raw in elf.strings():
        t = elf.show(raw)
        if SETTINGS.search(t) and re.search(r"\.(ini|txt|tab|dat)$", t, re.I):
            out.append((va, t))
    return out


def readable(raw):
    """A column or key name: ASCII text, or Chinese in GBK, never control characters."""
    if any(c < 0x20 or c == 0x7F for c in raw):
        return None
    try:
        return raw.decode("ascii")
    except UnicodeDecodeError:
        pass
    try:
        return raw.decode("gbk")
    except UnicodeDecodeError:
        return None


def string_operands(elf, va, limit=1500):
    """String constants a function passes to calls (`mov [esp+N], <va>` / `push <va>`), in order,
    without duplicates.  Only immediates: a memory operand `[disp]` is a table, not a string."""
    seen, out = set(), []
    for ins in elf.dis(va, limit, stop_at_ret=True):
        if ins.mnemonic not in ("mov", "push"):
            continue
        imm = ins.operands[-1]
        if imm.type != X86_OP_IMM:
            continue
        value = imm.imm & 0xFFFFFFFF
        if value in seen or not elf.is_mapped(value):
            continue
        raw = elf.cstr(value, 200)
        if not raw or len(raw) < 2:
            continue
        seen.add(value)
        out.append((value, raw))
    return out


def callees_of(elf, va, limit=1500):
    out = []
    for ins in elf.dis(va, limit, stop_at_ret=True):
        if ins.mnemonic == "call" and ins.operands and ins.operands[0].type == X86_OP_IMM:
            t = ins.operands[0].imm & 0xFFFFFFFF
            if elf.is_code(t) and t not in out and t not in elf.plt():
                out.append(t)
    return out


def columns(elf, va, deep=True):
    """Column / key names the loader at `va` uses: its own string constants, plus those of the
    functions it calls when it has few of its own."""
    names = []
    for _v, raw in string_operands(elf, va):
        t = readable(raw)
        if t is None or "%" in t or "\\" in t or "/" in t or len(raw) > 40:
            continue
        if COLUMN.match(t) and t not in names:
            names.append(t)
    if deep and len(names) < 3:
        for callee in callees_of(elf, va)[:12]:
            for n in columns(elf, callee, deep=False):
                if n not in names:
                    names.append(n)
    return names


def report(elf, va, path):
    refs = elf.xrefs(va)
    funcs = []
    for r in refs:
        f = elf.function_start(r)
        if f not in funcs:
            funcs.append(f)
    lines = [f"### `{path}`", ""]
    if not funcs:
        lines.append("- không thấy code nào nhắc tới (có thể đọc qua chuỗi ghép)")
    for f in funcs:
        cols = columns(elf, f)
        lines.append(f"- đọc ở hàm `0x{f:08X}`" + (": " + ", ".join(f"`{c}`" for c in cols[:60]) if cols else " (không thấy tên cột — đọc theo chỉ số cột hoặc qua hàm khác)"))
    lines.append("")
    return "\n".join(lines)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    elf = re_elf.Elf(sys.argv[1])
    cmd = sys.argv[2]
    paths = settings_paths(elf)
    if cmd == "list":
        for va, t in paths:
            print(f"{va:08X}  {t}")
        print(len(paths), "files")
    elif cmd == "file":
        want = sys.argv[3].lower()
        for va, t in paths:
            if want in t.lower():
                print(report(elf, va, t))
    elif cmd == "all":
        out = sys.argv[3]
        doc = ["# Tệp settings mà jx_linux_y đọc — hàm đọc và các cột / khoá nó dùng", "",
               "Sinh bởi `tools/re/re_settings.py all`. Với mỗi tệp: hàm nào nhắc tới đường dẫn, và các hằng chuỗi",
               "hàm đó dùng (tên cột của KTabFile / khoá của KIniFile). Địa chỉ là VA trong `jx_linux_y`.", ""]
        for va, t in sorted(paths, key=lambda p: p[1].lower()):
            doc.append(report(elf, va, t))
        open(out, "w", encoding="utf-8", newline="\n").write("\n".join(doc))
        print("wrote", out, len(paths), "files")


if __name__ == "__main__":
    main()
