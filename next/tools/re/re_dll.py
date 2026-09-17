#!/usr/bin/env python3
"""Exports and disassembly of a normal (not packed) 32-bit PE.

  re_dll.py <dll> exports [regex]
  re_dll.py <dll> dis <export-name-or-va-hex> [count]
"""
import re
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

sys.stdout.reconfigure(encoding="utf-8", errors="replace")


class PE:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.d, 0x3C)[0]
        self.nsec = struct.unpack_from("<H", self.d, pe + 6)[0]
        optsz = struct.unpack_from("<H", self.d, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.d, pe + 52)[0]
        self.dirs = pe + 24 + 96
        sec = pe + 24 + optsz
        self.secs = []
        for i in range(self.nsec):
            s = self.d[sec + i * 40: sec + i * 40 + 40]
            vsz, rva, rsz, roff = struct.unpack_from("<IIII", s, 8)
            self.secs.append((rva, max(vsz, rsz), roff))
        self.exports = self._exports()
        self.imports = self._imports()

    def r2o(self, rva):
        for va, sz, off in self.secs:
            if va <= rva < va + sz:
                return off + rva - va
        return None

    def cstr(self, off):
        end = self.d.index(b"\0", off)
        return self.d[off:end].decode("latin-1")

    def _exports(self):
        rva, size = struct.unpack_from("<II", self.d, self.dirs)
        if not rva:
            return {}
        o = self.r2o(rva)
        nfunc, nname, afunc, aname, aord = struct.unpack_from("<IIIII", self.d, o + 20)
        out = {}
        for i in range(nname):
            name = self.cstr(self.r2o(struct.unpack_from("<I", self.d, self.r2o(aname) + 4 * i)[0]))
            ordi = struct.unpack_from("<H", self.d, self.r2o(aord) + 2 * i)[0]
            out[name] = struct.unpack_from("<I", self.d, self.r2o(afunc) + 4 * ordi)[0] + self.base
        return out

    def _imports(self):
        rva, size = struct.unpack_from("<II", self.d, self.dirs + 8)
        out = {}
        if not rva:
            return out
        o = self.r2o(rva)
        while True:
            ilt, _, _, name_rva, iat = struct.unpack_from("<IIIII", self.d, o)
            if not name_rva:
                break
            dll = self.cstr(self.r2o(name_rva))
            t = self.r2o(ilt or iat)
            k = 0
            while True:
                v = struct.unpack_from("<I", self.d, t + 4 * k)[0]
                if not v:
                    break
                nm = f"ord{v & 0xffff}" if v & 0x80000000 else self.cstr(self.r2o(v) + 2)
                out[self.base + iat + 4 * k] = f"{dll}!{nm}"
                k += 1
            o += 20
        return out

    def string_at(self, va):
        o = self.r2o(va - self.base)
        if o is None:
            return None
        end = self.d.find(b"\0", o, o + 200)
        if end < 0 or end - o < 3:
            return None
        raw = self.d[o:end]
        if not all(0x20 <= c <= 0x7E or c >= 0x81 for c in raw):
            return None
        try:
            return raw.decode("gbk")
        except UnicodeDecodeError:
            return raw.decode("latin-1")

    def dis(self, va, count=80):
        rev = {v: k for k, v in self.exports.items()}
        o = self.r2o(va - self.base)
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        for ins in md.disasm(self.d[o:o + count * 16], va):
            note = ""
            for m in re.finditer(r"0x([0-9a-f]{6,8})", ins.op_str):
                v = int(m.group(1), 16)
                if v in self.imports:
                    note = "   ; " + self.imports[v]
                elif v in rev:
                    note = "   ; " + rev[v]
                else:
                    s = self.string_at(v)
                    if s:
                        note = f'   ; "{s}"'
                if note:
                    break
            print(f"{ins.address:08X}  {ins.mnemonic:6s} {ins.op_str}{note}")
            count -= 1
            if count <= 0:
                break


def main():
    pe = PE(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "exports":
        rx = re.compile(sys.argv[3], re.I) if len(sys.argv) > 3 else None
        for name, va in sorted(pe.exports.items(), key=lambda kv: kv[1]):
            if not rx or rx.search(name):
                print(f"{va:08X}  {name}")
    elif cmd == "dis":
        target = sys.argv[3]
        va = pe.exports.get(target)
        if va is None:
            va = int(target, 16)
        pe.dis(va, int(sys.argv[4]) if len(sys.argv) > 4 else 80)


if __name__ == "__main__":
    main()
