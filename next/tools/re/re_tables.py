#!/usr/bin/env python3
"""The settings tables of jx_linux_y, each with the columns / keys the code reads from it.

  re_tables.py <elf> objects            table object -> file it loads (KTabFile::Load / KIniFile::Load)
  re_tables.py <elf> columns            every table: its columns / keys and who reads them
  re_tables.py <elf> report <out.md>    the same as a document

How it links them: a KTabFile lives in a global object, in a member of a class (`this + offset`)
or on a function's stack; the loader passes its address as `this` and the path
(`mov [esp], <obj>; mov [esp+4], <path>; call KTabFile::Load`), and every later read passes the
same `this` with the column name (`call KTabFile::GetInteger(obj, row, "Level", ...)`).
re_calls.py resolves `this` as a constant, `("arg", 1, off)` (a member) or `("local", fn, off)` (a
stack object).  A member is the same object in every method of its class, so a read through it is
linked when the reader sits near the loader (same class, methods compiled together); a stack
object only inside its function.  What cannot be linked is listed by reader function, so nothing
is lost.

The methods are named from the old headers (SwordOnline/Sources/Engine/Src/KTabFile.h,
KIniFile.h) after reading each body: see docs/LINUX-SERVER.md §6.
"""
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
import re_calls  # noqa: E402
import re_elf  # noqa: E402

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# jx_linux_y: KTabFile (sizeof 0x20: vtable, m_Width +4, m_Height +8, m_Memory +0xc, m_OffsetTable +0x14)
TAB_LOAD = {0x082284E0: "KTabFile::Load"}
# by column name: this = [esp], nRow = [esp+4], szColumn = [esp+8]; bColumnLab (0 = "B", "AC" letter
# labels instead of names) at [esp+0x14] for GetInteger/GetFloat and [esp+0x18] for GetString
TAB_GET = {0x08228170: "GetInteger", 0x082280C0: "GetFloat", 0x08228220: "GetString",
           0x08227E90: "GetInteger(szRow, szColumn)", 0x08227FA0: "GetString(szRow, szColumn)"}
TAB_LAB_SLOT = {0x08228170: 0x14, 0x082280C0: 0x14, 0x08228220: 0x18}
# by column number: this, nRow, nColumn - the file is read column by column, there is no name
TAB_GET_INDEX = {0x08227E10: "GetInteger(nRow, nColumn)", 0x08227F40: "GetString(nRow, nColumn)"}
TAB_FIND_COLUMN = {0x08227AD0: "FindColumn"}          # this, szColumn = [esp+4]
TAB_OTHER = {0x08227B50: "FindRow(szRow, nStart)", 0x082279D0: "Clear", 0x08228630: "KTabFile() (constructor)",
             0x082285A0: "~KTabFile", 0x08227A00: "GetValue(nRow, nColumn, buf, size)", 0x08228030: "Str2Col"}
# KIniFile: this, lpSection = [esp+4], lpKeyName = [esp+8]
INI_LOAD = {0x08220140: "KIniFile::Load"}
INI_GET = {0x0821F2C0: "GetInteger", 0x0821F340: "GetString", 0x0821F1F0: "GetInteger2",
           0x0821EF40: "Get(nhiều giá trị x,y,...)"}
INI_WRITE = {0x0821F7C0: "WriteInteger", 0x0821F810: "WriteString"}
INI_OTHER = {0x0821FE80: "Save", 0x08220280: "KIniFile() (constructor)", 0x08220240: "~KIniFile",
             0x0821ED00: "GetKeyValue", 0x0821F3A0: "SetKeyValue"}
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_ ]{0,39}$")
LABEL = re.compile(r"^[A-Z]{1,2}$")


def analyse(elf):
    starts, calls = re_calls.load(elf)
    files = {}                       # object -> [(path, loader function)]
    columns = collections.defaultdict(dict)   # object -> {column: {readers}}
    by_index = collections.defaultdict(set)   # object -> {reader functions that read by column number}
    writes = collections.defaultdict(dict)    # object -> {key: {writers}}
    unlinked = collections.defaultdict(dict)  # reader function -> {column: getter}

    def text(v):
        return elf.text_at(v) if isinstance(v, int) else None

    def name_ok(name):
        return name and (IDENT.match(name) or any(c > "\x7f" for c in name))

    def is_path(t):
        return t and (".ini" in t.lower() or ".txt" in t.lower() or ".tab" in t.lower())

    # a path built at run time (sprintf("...\\%s.ini")) is not an argument of Load; the pattern
    # is the last path-like string the same function passed to any call before the Load
    patterns = collections.defaultdict(list)     # function -> [(site, pattern)]
    for caller, site, target, args in calls:
        for v in args.values():
            t = text(v)
            if is_path(t):
                patterns[caller].append((site, t))

    # Across calls: a function that gets a table (or a path) as its argument n uses it as
    # ("arg", n, off); its callers say what that is.  Only when every caller agrees - a helper
    # called with many different tables is left as it is.
    passed = collections.defaultdict(set)        # (callee, n) -> {(caller, value)}
    for caller, site, target, args in calls:
        for slot, v in args.items():
            if slot >= 0 and slot % 4 == 0 and (isinstance(v, tuple) or (isinstance(v, int) and elf.is_mapped(v))):
                passed[(target, slot // 4 + 1)].add((caller, v))

    def resolve(fn, v, depth=4):
        """v as seen in fn -> the same value in the terms of whoever really owns it."""
        if not (isinstance(v, tuple) and v[0] == "arg" and depth):
            return v
        got = set()
        for caller, base in passed.get((fn, v[1]), ()):
            base = resolve(caller, base, depth - 1)
            if isinstance(base, int):
                got.add(base + v[2])
            elif isinstance(base, tuple) and base[0] in ("local", "arg"):
                got.add((base[0], base[1], base[2] + v[2]))
        return got.pop() if len(got) == 1 else v

    def exact(obj):
        return isinstance(obj, int) or (isinstance(obj, tuple) and obj[0] == "local")

    def is_arg(v):
        return isinstance(v, tuple) and v[0] == "arg"

    callers_of = collections.defaultdict(list)
    for caller, site, target, args in calls:
        callers_of[target].append((caller, site, args))

    def substitute(v, args_c):
        if not is_arg(v):
            return v
        base = args_c.get((v[1] - 1) * 4)
        if isinstance(base, int):
            return base + v[2]
        if isinstance(base, tuple) and base[0] in ("local", "arg"):
            return (base[0], base[1], base[2] + v[2])
        return None

    def loads_of(fn, site, obj, path, depth=3):
        """(owner, site, obj, path) of a Load in fn.  When the path is one of fn's arguments, fn is
        a helper ("load this file into this table"): the real loads are at its callers."""
        if is_arg(path) and depth and callers_of[fn]:
            for c, site_c, args_c in callers_of[fn]:
                yield from loads_of(c, site_c, substitute(obj, args_c), substitute(path, args_c), depth - 1)
        else:
            yield fn, site, obj, path

    reads = []
    for caller, site, target, args in calls:
        obj = resolve(caller, args.get(0))
        if target in TAB_LOAD or target in INI_LOAD:
            for owner, site_o, obj_o, path_o in loads_of(caller, site, args.get(0), args.get(4)):
                obj_o = resolve(owner, obj_o)
                path = text(path_o)
                if not is_path(path):
                    before = [p for s, p in patterns[owner] if s < site_o]
                    path = f"{before[-1]} (đường dẫn ghép lúc chạy)" if before else "(đường dẫn ghép lúc chạy, không thấy mẫu)"
                key = obj_o if obj_o is not None else ("fn", owner)
                if (path, owner) not in files.setdefault(key, []):
                    files[key].append((path, owner))
        elif target in TAB_GET:
            name = text(args.get(8))
            if not name_ok(name):
                continue
            if target in TAB_LAB_SLOT and args.get(TAB_LAB_SLOT[target]) == 0 and LABEL.match(name):
                name = f"cột chữ {name}"
            if target in (0x08227E90, 0x08227FA0):
                row = text(args.get(4))
                name = f"{name} (dòng {row!r})" if row else name
            reads.append((caller, obj, name, TAB_GET[target]))
        elif target in TAB_FIND_COLUMN:
            name = text(args.get(4))
            if name_ok(name):
                reads.append((caller, obj, name, "FindColumn"))
        elif target in TAB_GET_INDEX:
            reads.append((caller, obj, None, TAB_GET_INDEX[target]))
        elif target in INI_GET or target in INI_WRITE:
            name, section = text(args.get(8)), text(args.get(4))
            if not name_ok(name):
                name = "(khoá ghép lúc chạy)"
            if not name_ok(section):
                section = "(section ghép lúc chạy)" if isinstance(args.get(4), tuple) else "?"
            key = f"[{section}] {name}"
            reads.append((caller, obj, key, INI_GET.get(target) or "ghi " + INI_WRITE[target]))
    for caller, obj, key, getter in reads:
        linked = False
        if obj is not None and obj in files:
            if exact(obj):
                linked = True          # a global, or the stack object of one function
            else:
                # a member object: the same class, so the reader sits near the loader
                linked = any(abs(caller - fn) < 0x10000 for _p, fn in files[obj])
        if key is None:
            if linked:
                by_index[obj].add(caller)
            else:
                unlinked[caller].setdefault("(đọc theo chỉ số cột)", getter)
        elif linked and getter.startswith("ghi "):
            writes[obj].setdefault(key, set()).add(caller)
        elif linked:
            columns[obj].setdefault(key, set()).add(caller)
        else:
            unlinked[caller][key] = getter
    return files, columns, by_index, writes, unlinked


def where_of(obj):
    if isinstance(obj, int):
        return f"đối tượng `0x{obj:08X}`"
    if isinstance(obj, tuple) and obj[0] == "arg":
        return f"thành viên `this+0x{obj[2]:X}`"
    if isinstance(obj, tuple) and obj[0] == "local":
        return f"bảng cục bộ `[ebp-0x{-obj[2]:X}]`"
    return "đối tượng cục bộ (con trỏ chưa theo dõi được)"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    elf = re_elf.Elf(sys.argv[1])
    files, columns, by_index, writes, unlinked = analyse(elf)
    cmd = sys.argv[2]
    if cmd == "objects":
        for obj, lst in sorted(files.items(), key=lambda x: str(x[0])):
            for path, fn in lst:
                print(f"{where_of(obj):40s}  {path:50s}  nạp ở {fn:08X}")
        print(len(files), "objects")
    elif cmd in ("columns", "report"):
        lines = ["# Bảng settings của jx_linux_y — cột / khoá mà mã đọc", "",
                 "Sinh bởi `tools/re/re_tables.py report`. Mỗi tệp: đối tượng bảng (toàn cục, thành viên lớp hay",
                 "trên stack), hàm nạp, và các cột (`KTabFile::Get*(nRow, szColumn)`) hoặc khoá",
                 "(`KIniFile::Get*(section, key)`) được đọc, kèm hàm đọc. `đọc theo chỉ số cột` = `Get*(nRow, nColumn)`:",
                 "tệp được đọc từng cột theo số thứ tự, không có tên cột trong mã.", ""]
        by_path = collections.defaultdict(lambda: collections.defaultdict(list))   # path -> obj -> loaders
        for obj, lst in files.items():
            for path, fn in lst:
                by_path[path][obj].append(fn)
        n_files_linked = 0
        for path in sorted(by_path, key=str.lower):
            lines.append(f"## `{path}`")
            got = False
            for obj, fns in by_path[path].items():
                cols, idx, wr = columns.get(obj, {}), by_index.get(obj, set()), writes.get(obj, {})
                got = got or bool(cols or idx or wr)
                lines.append(f"- {where_of(obj)}, nạp ở {', '.join(f'`0x{fn:08X}`' for fn in sorted(fns))}, **{len(cols)} cột/khoá**"
                             + (f", đọc theo chỉ số cột ở {', '.join(f'`0x{r:08X}`' for r in sorted(idx))}" if idx else "")
                             + (f", ghi {len(wr)} khoá" if wr else ""))
                for col in sorted(cols, key=str.lower):
                    readers = ", ".join(f"`0x{r:08X}`" for r in sorted(cols[col])[:4])
                    lines.append(f"  - `{col}` — đọc ở {readers}")
                for col in sorted(wr, key=str.lower):
                    writers = ", ".join(f"`0x{r:08X}`" for r in sorted(wr[col])[:4])
                    lines.append(f"  - `{col}` — **ghi** ở {writers}")
            n_files_linked += got
            lines.append("")
        lines.append("## Cột đọc qua đối tượng chưa nối được tên tệp (theo hàm đọc)")
        lines.append("")
        for fn in sorted(unlinked):
            cols = unlinked[fn]
            lines.append(f"- `0x{fn:08X}`: " + ", ".join(f"`{c}`" for c in sorted(cols, key=str.lower)[:40]) + (" …" if len(cols) > 40 else ""))
        text = "\n".join(lines)
        if cmd == "report":
            open(sys.argv[3], "w", encoding="utf-8", newline="\n").write(text)
            linked = sum(len(c) for c in columns.values())
            print(f"wrote {sys.argv[3]}: {len(by_path)} files ({n_files_linked} with linked columns), {linked} linked columns, "
                  f"{sum(len(v) for v in unlinked.values())} unlinked reads in {len(unlinked)} functions")
        else:
            print(text)


if __name__ == "__main__":
    main()
