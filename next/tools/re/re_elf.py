#!/usr/bin/env python3
"""Static analysis of a 32-bit x86 ELF that has no section headers (the Linux game server
jx_linux_y, its gateway s3relay_y, their .so files).  Everything is found through the PROGRAM
headers and the DYNAMIC segment, the way the loader does it.

  re_elf.py <elf> info                      segments, needed libraries, entry point
  re_elf.py <elf> imports [text]            imported functions and the PLT stub that calls each
  re_elf.py <elf> exports [text]            exported symbols (shared objects)
  re_elf.py <elf> strings <regex> [max]     strings whose text matches (GBK / latin-1 decoded)
  re_elf.py <elf> xref <va-hex>             instructions that mention this address as an immediate
  re_elf.py <elf> xrefstr <text>            the same for every string that contains <text>
  re_elf.py <elf> dis <va-hex> [count]      disassemble, calls and strings annotated
  re_elf.py <elf> func <va-hex>             the whole function around / at an address
  re_elf.py <elf> luamap                    Lua registration tables: {name, function} pairs

Addresses are virtual addresses as the process sees them.
"""
import bisect
import json
import os
import re
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_32, Cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

PT_LOAD, PT_DYNAMIC = 1, 2
DT = {1: "NEEDED", 5: "STRTAB", 6: "SYMTAB", 10: "STRSZ", 11: "SYMENT", 4: "HASH", 0x6ffffef5: "GNU_HASH",
      23: "JMPREL", 2: "PLTRELSZ", 17: "REL", 18: "RELSZ", 3: "PLTGOT", 12: "INIT", 13: "FINI", 14: "SONAME"}


class Elf:
    def __init__(self, path):
        self.path = path
        self.d = open(path, "rb").read()
        d = self.d
        self.imports = {}     # {slot va: "dll!name"} of an unpacked PE image (re_upx.py)
        if path.endswith(".img") and os.path.exists(path[:-4] + ".json"):
            # a raw memory image with its layout beside it (re_upx.py): no ELF headers
            meta = json.load(open(path[:-4] + ".json"))
            self.entry = meta.get("entry", 0)
            self.segments = meta["segments"]
            self.dynamic = None
            self.imports = {int(k, 16): v for k, v in meta.get("imports", {}).items()}
            self.md = Cs(CS_ARCH_X86, CS_MODE_32)
            self.md.detail = True
            self._dyn = {"NEEDED": []}
            self._plt = dict(self.imports)
            self._strings = None
            return
        if d[:4] != b"\x7fELF" or d[4] != 1:
            raise SystemExit("not a 32-bit ELF")
        self.entry, phoff = struct.unpack_from("<II", d, 0x18)
        phentsize, phnum = struct.unpack_from("<HH", d, 0x2A)
        self.segments = []
        self.dynamic = None
        for i in range(phnum):
            t, off, va, _pa, filesz, memsz, flags, _al = struct.unpack_from("<8I", d, phoff + i * phentsize)
            if t == PT_LOAD:
                self.segments.append({"off": off, "va": va, "filesz": filesz, "memsz": memsz, "flags": flags})
            elif t == PT_DYNAMIC:
                self.dynamic = (off, filesz)
        self.md = Cs(CS_ARCH_X86, CS_MODE_32)
        self.md.detail = True
        self._dyn = None
        self._plt = None
        self._strings = None

    # ---- addresses
    def off(self, va):
        for s in self.segments:
            if s["va"] <= va < s["va"] + s["filesz"]:
                return s["off"] + va - s["va"]
        return None

    def is_code(self, va):
        return any(s["flags"] & 1 and s["va"] <= va < s["va"] + s["filesz"] for s in self.segments)

    def is_mapped(self, va):
        return any(s["va"] <= va < s["va"] + s["memsz"] for s in self.segments)

    def u32(self, va):
        o = self.off(va)
        return struct.unpack_from("<I", self.d, o)[0] if o is not None and o + 4 <= len(self.d) else None

    def cstr(self, va, limit=400):
        o = self.off(va)
        if o is None:
            return None
        end = self.d.find(b"\x00", o, o + limit)
        if end < 0:
            return None
        return self.d[o:end]

    @staticmethod
    def show(raw):
        for enc in ("ascii", "gbk"):
            try:
                return raw.decode(enc)
            except UnicodeDecodeError:
                continue
        return raw.decode("latin-1")

    def text_at(self, va):
        raw = self.cstr(va)
        if not raw or len(raw) < 3 or any(c < 0x20 and c not in (9, 10, 13) for c in raw):
            return None
        return self.show(raw)

    # ---- dynamic section
    def dyn(self):
        if self._dyn is None:
            self._dyn = {"NEEDED": []}
            if self.dynamic:
                off, size = self.dynamic
                for i in range(0, size, 8):
                    tag, val = struct.unpack_from("<II", self.d, off + i)
                    if tag == 0:
                        break
                    name = DT.get(tag)
                    if name == "NEEDED":
                        self._dyn["NEEDED"].append(val)
                    elif name:
                        self._dyn[name] = val
        return self._dyn

    def dynstr(self, index):
        base = self.dyn().get("STRTAB")
        raw = self.cstr(base + index) if base else None
        return raw.decode("latin-1") if raw is not None else ""

    def symbols(self):
        """Every entry of the dynamic symbol table: (index, name, value, size, info, shndx)."""
        dyn = self.dyn()
        symtab, strtab = dyn.get("SYMTAB"), dyn.get("STRTAB")
        if not symtab or not strtab:
            return []
        # the table has no length of its own: it ends where the string table begins (that is how
        # every linker lays them out), else at the hash table's chain count
        count = (strtab - symtab) // 16 if strtab > symtab else 0
        if count <= 0 and dyn.get("HASH"):
            count = self.u32(dyn["HASH"] + 4) or 0
        out = []
        for i in range(count):
            o = self.off(symtab + 16 * i)
            if o is None:
                break
            name, value, size, info, _other, shndx = struct.unpack_from("<IIIBBH", self.d, o)
            out.append((i, self.dynstr(name), value, size, info, shndx))
        return out

    def jmprel_tables(self):
        """[(va, entries)] of every table of R_386_JMP_SLOT relocations in the file: the one the
        dynamic section names, and any other run of them - a protector that rewrote the dynamic
        section leaves the program's original .rel.plt behind, and its symbol indices still count
        into the symbol table the protector kept (checked on jx_linux_y: strtol, sprintf, strncpy,
        strtod, __cxa_atexit all land where their callers expect them)."""
        dyn = self.dyn()
        out = []
        if dyn.get("JMPREL"):
            out.append((dyn["JMPREL"], dyn.get("PLTRELSZ", 0) // 8))
        for s in self.segments:
            if not s["flags"] & 1:
                continue
            blob = self.d[s["off"]:s["off"] + s["filesz"]]
            for align in (0, 4):
                start = n = 0
                for at in range(align, len(blob) - 8, 8):
                    r_offset, r_info = struct.unpack_from("<II", blob, at)
                    if r_info & 0xFF == 7 and self.is_mapped(r_offset):
                        if not n:
                            start = at
                        n += 1
                    else:
                        if n >= 8 and all(s["va"] + start != va for va, _c in out):
                            out.append((s["va"] + start, n))
                        n = 0
        return out

    def plt(self):
        """{address of the PLT stub: imported name}.  A stub is `jmp [GOT slot]`; the slot is named
        by the JMPREL relocation that fills it (see jmprel_tables)."""
        if self._plt is None:
            self._plt = {}
            syms = {i: name for i, name, *_ in self.symbols()}
            slots = {}
            for table, count in self.jmprel_tables():
                for i in range(count):
                    o = self.off(table + 8 * i)
                    if o is None:
                        break
                    r_offset, r_info = struct.unpack_from("<II", self.d, o)
                    slots.setdefault(r_offset, syms.get(r_info >> 8, "?"))
            for s in self.segments:
                if not s["flags"] & 1:
                    continue
                blob = self.d[s["off"]:s["off"] + s["filesz"]]
                at = blob.find(b"\xff\x25")
                while at >= 0:
                    slot = struct.unpack_from("<I", blob, at + 2)[0] if at + 6 <= len(blob) else 0
                    if slot in slots:
                        self._plt[s["va"] + at] = slots[slot]
                    at = blob.find(b"\xff\x25", at + 1)
        return self._plt

    # ---- strings
    def strings(self):
        if self._strings is None:
            self._strings = []
            pat = re.compile(rb"[\x20-\x7e\x81-\xfe\t]{4,400}\x00")
            for s in self.segments:
                blob = self.d[s["off"]:s["off"] + s["filesz"]]
                for m in pat.finditer(blob):
                    self._strings.append((s["va"] + m.start(), m.group()[:-1]))
        return self._strings

    # ---- code
    def dis(self, va, count=60, stop_at_ret=False):
        o = self.off(va)
        if o is None:
            return
        n = 0
        for ins in self.md.disasm(self.d[o:o + count * 12 + 64], va):
            yield ins
            n += 1
            if n >= count or (stop_at_ret and ins.mnemonic in ("ret", "retn")):
                break

    def annotate(self, ins):
        notes = []
        plt = self.plt()
        for op in ins.operands:
            value = op.imm if op.type == X86_OP_IMM else (op.mem.disp if op.type == X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0 else None)
            if value is None:
                continue
            value &= 0xFFFFFFFF
            if value in plt:
                notes.append(plt[value] + "@plt")
            elif self.is_mapped(value) and not ins.mnemonic.startswith("j") and ins.mnemonic != "call":
                # .rodata shares the r-x segment with .text in these binaries: a string is
                # whatever reads as one, not "an address outside the code"
                t = self.text_at(value)
                if t:
                    notes.append('"' + t[:70].replace("\n", "\\n") + '"')
        return ("   ; " + " ".join(notes)) if notes else ""

    def function_start(self, va):
        """Scans back for the usual prologue (push ebp; mov ebp, esp) on a 16-byte aligned or
        ret/nop-preceded address."""
        o = self.off(va)
        if o is None:
            return va
        for back in range(0, 0x4000):
            p = o - back
            if p < 1:
                break
            # push ebp; mov ebp, esp - as GCC (89 E5) and as MSVC (8B EC) encode it
            if self.d[p:p + 3] in (b"\x55\x89\xe5", b"\x55\x8b\xec") and (self.d[p - 1] in (0xC3, 0x90, 0xCC, 0x00) or (va - back) % 16 == 0 or self.d[p - 3:p - 1] == b"\xc2"):
                return va - back
        return va

    def xrefs(self, target):
        pat = struct.pack("<I", target)
        out = []
        for s in self.segments:
            if not s["flags"] & 1:
                continue
            blob = self.d[s["off"]:s["off"] + s["filesz"]]
            at = blob.find(pat)
            while at >= 0:
                out.append(s["va"] + at)
                at = blob.find(pat, at + 1)
        return out


def luamap(elf, minrun=3):
    """Registration tables: runs of {char* name, lua_CFunction fn} in the data segments."""
    ident = re.compile(rb"[A-Za-z_][A-Za-z0-9_]{1,63}$")
    out = []
    for s in elf.segments:
        if s["flags"] & 1 and not s["flags"] & 2:
            pass
        blob = elf.d[s["off"]:s["off"] + s["filesz"]]
        words = struct.unpack_from("<%dI" % (len(blob) // 4), blob, 0)
        i = 0
        while i + 1 < len(words):
            run = []
            j = i
            while j + 1 < len(words):
                name_va, fn_va = words[j], words[j + 1]
                if not name_va or not elf.is_code(fn_va) or elf.is_code(name_va) and False:
                    break
                raw = elf.cstr(name_va, 80)
                if not raw or not ident.match(raw):
                    break
                run.append((s["va"] + 4 * j, raw.decode("latin-1"), fn_va))
                j += 2
            if len(run) >= minrun:
                out.append(run)
                i = j
            else:
                i += 1
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    elf = Elf(sys.argv[1])
    cmd, args = sys.argv[2], sys.argv[3:]
    if cmd == "info":
        print(f"entry {elf.entry:08X}")
        for s in elf.segments:
            print(f"LOAD va {s['va']:08X}..{s['va'] + s['memsz']:08X} file {s['off']:08X}+{s['filesz']:08X} "
                  f"{'r' if s['flags'] & 4 else '-'}{'w' if s['flags'] & 2 else '-'}{'x' if s['flags'] & 1 else '-'}")
        for n in elf.dyn()["NEEDED"]:
            print("needs", elf.dynstr(n))
        print("symbols", len(elf.symbols()), " plt stubs", len(elf.plt()))
    elif cmd in ("imports", "exports"):
        want = args[0].lower() if args else ""
        if cmd == "imports":
            for va, name in sorted(elf.plt().items()):
                if want in name.lower():
                    print(f"{va:08X}  {name}")
        else:
            for _i, name, value, size, info, shndx in elf.symbols():
                if shndx != 0 and value and want in name.lower():
                    print(f"{value:08X}  {size:6d}  {'func' if info & 15 == 2 else 'data' if info & 15 == 1 else 'other'}  {name}")
    elif cmd == "strings":
        pat = re.compile(args[0])
        limit = int(args[1]) if len(args) > 1 else 200
        n = 0
        for va, raw in elf.strings():
            t = elf.show(raw)
            if pat.search(t):
                print(f"{va:08X}  {t}")
                n += 1
                if n >= limit:
                    break
    elif cmd == "xref":
        for va in elf.xrefs(int(args[0], 16)):
            print(f"{va:08X}  (function {elf.function_start(va):08X})")
    elif cmd == "xrefstr":
        for va, raw in elf.strings():
            if args[0].encode("gbk", "ignore") in raw:
                refs = elf.xrefs(va)
                print(f'string {va:08X} "{elf.show(raw)[:80]}"  refs: ' + " ".join(f"{r:08X}" for r in refs[:12]))
    elif cmd in ("dis", "func"):
        va = int(args[0], 16)
        if cmd == "func":
            va = elf.function_start(va)
            print(f"; function starts at {va:08X}")
        count = int(args[1]) if len(args) > 1 else (400 if cmd == "func" else 60)
        for ins in elf.dis(va, count, stop_at_ret=(cmd == "func")):
            print(f"{ins.address:08X}  {ins.mnemonic:6s} {ins.op_str}{elf.annotate(ins)}")
    elif cmd == "luamap":
        tables = luamap(elf)
        total = 0
        for run in tables:
            print(f"# table at {run[0][0]:08X}: {len(run)} functions")
            for _at, name, fn in run:
                print(f"{fn:08X}  {name}")
            total += len(run)
        print(f"# {len(tables)} tables, {total} functions")
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
