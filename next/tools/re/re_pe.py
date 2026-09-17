#!/usr/bin/env python3
"""Static analysis helpers for an UPX-unpacked 32-bit PE image (what upx_unpack.py writes).

  re_pe.py <img> sections                     the original section table UPX kept inside the image
  re_pe.py <img> unfilter <out>               undo the UPX call filter (0x24/0x26, cto from argv) over .text
  re_pe.py <img> xref <va-hex>                instructions that load this address
  re_pe.py <img> xrefstr <text>               same, for every string containing <text> (GBK)
  re_pe.py <img> dis <va-hex> [count]         disassemble, with strings and call targets annotated
  re_pe.py <img> func <va-hex>                from the function start (found by scanning back) to its ret

The image starts at RVA 0x1000 of image base 0x400000.
"""
import re
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
BASE = 0x401000


class Image:
    def __init__(self, path):
        self.d = bytearray(open(path, "rb").read())
        self.md = Cs(CS_ARCH_X86, CS_MODE_32)

    def off(self, va):
        return va - BASE

    def va(self, off):
        return off + BASE

    def sections(self):
        """UPX keeps the original PE header after the unpacked data: find it and read its table."""
        out = []
        for m in re.finditer(rb"PE\x00\x00\x4c\x01", bytes(self.d)):
            pe = m.start()
            nsec = struct.unpack_from("<H", self.d, pe + 6)[0]
            optsz = struct.unpack_from("<H", self.d, pe + 20)[0]
            if not 1 <= nsec <= 16 or optsz not in (0xE0, 0xF0):
                continue
            sec = pe + 24 + optsz
            rows = []
            for i in range(nsec):
                s = self.d[sec + i * 40: sec + i * 40 + 40]
                name = bytes(s[:8]).rstrip(b"\0").decode("latin-1")
                vsz, rva, rsz, roff, _, _, _, _, flags = struct.unpack_from("<IIIIIIHHI", s, 8)
                rows.append((name, rva, vsz, rsz, flags))
            out.append((pe, rows))
        return out

    def cstring(self, va, limit=200):
        o = self.off(va)
        if not 0 <= o < len(self.d):
            return None
        end = self.d.find(b"\0", o, o + limit)
        if end < 0 or end - o < 3:
            return None
        raw = bytes(self.d[o:end])
        if not all(0x20 <= c <= 0x7E or c >= 0x81 for c in raw):
            return None
        try:
            return raw.decode("gbk")
        except UnicodeDecodeError:
            return raw.decode("latin-1")

    def unfilter(self, code_end_va, cto, e9=False):
        """UPX ctok32 filter: E8 (and E9) operands were stored big-endian as absolute+cto<<24."""
        end = self.off(code_end_va) - 5
        d = self.d
        n = 0
        i = 0
        while i < end:
            b = d[i]
            if (b == 0xE8 or (e9 and b == 0xE9)) and d[i + 1] == cto:
                jc = struct.unpack_from(">I", d, i + 1)[0] - (cto << 24)
                rel = (jc - (i + 1)) & 0xFFFFFFFF
                struct.pack_into("<I", d, i + 1, rel)
                n += 1
                i += 5
            else:
                i += 1
        return n

    def xrefs(self, target, lo=0, hi=None):
        """push imm32 / mov r32, imm32 / mov [..], imm32 / lea-style absolute loads of `target`."""
        raw = struct.pack("<I", target)
        hits = []
        pos = lo
        hi = hi or len(self.d)
        while True:
            pos = self.d.find(raw, pos, hi)
            if pos < 0:
                break
            hits.append(self.va(pos))
            pos += 1
        return hits

    def dis(self, va, count=60, stop_at_ret=False):
        o = self.off(va)
        lines = []
        for ins in self.md.disasm(bytes(self.d[o:o + count * 16]), va):
            note = ""
            for m in re.finditer(r"0x([0-9a-f]{6,8})", ins.op_str):
                v = int(m.group(1), 16)
                s = self.cstring(v)
                if s:
                    note = f'   ; "{s}"'
                    break
            lines.append(f"{ins.address:08X}  {ins.mnemonic:6s} {ins.op_str}{note}")
            count -= 1
            if count <= 0 or (stop_at_ret and ins.mnemonic in ("ret", "retn")):
                break
        return lines

    def func_start(self, va):
        """Scan back for the usual prologue or the padding before it."""
        o = self.off(va)
        for k in range(o, max(o - 0x4000, 0), -1):
            if self.d[k - 1] in (0xCC, 0x90, 0xC3) and (
                    self.d[k:k + 3] == b"\x55\x8b\xec" or self.d[k:k + 2] == b"\x6a\xff" or self.d[k:k + 3] == b"\x83\xec" or
                    self.d[k:k + 2] == b"\x81\xec" or self.d[k:k + 1] == b"\x56" or self.d[k:k + 1] == b"\x53" or self.d[k:k + 5][:1] == b"\xb8"):
                if self.d[k - 1] in (0xCC, 0x90) or self.d[k - 3:k - 2] == b"\xc2" or self.d[k - 1] == 0xC3:
                    return self.va(k)
        return va


def main():
    a = sys.argv
    img = Image(a[1])
    cmd = a[2]
    if cmd == "sections":
        for pe, rows in img.sections():
            print(f"PE header copy at image offset {pe:#x}")
            for name, rva, vsz, rsz, flags in rows:
                print(f"   {name:8s} rva {rva:#010x} vsize {vsz:#010x} rawsize {rsz:#010x} flags {flags:#010x}")
    elif cmd == "unfilter":
        code_end = int(a[4], 16)
        cto = int(a[5], 16)
        n = img.unfilter(code_end, cto, e9=len(a) > 6 and a[6] == "e9")
        open(a[3], "wb").write(img.d)
        print(f"unfiltered {n} calls below {code_end:#x} -> {a[3]}")
    elif cmd == "xref":
        for va in img.xrefs(int(a[3], 16)):
            print(f"{va:08X}")
    elif cmd == "xrefstr":
        needle = a[3].encode("gbk")
        for m in re.finditer(re.escape(needle), bytes(img.d)):
            start = m.start()
            while start > 0 and img.d[start - 1] != 0:
                start -= 1
            sva = img.va(start)
            print(f'string {sva:08X} "{img.cstring(sva)}"')
            for x in img.xrefs(sva):
                print(f"    loaded at {x - 1:08X}")
    elif cmd == "dis":
        for line in img.dis(int(a[3], 16), int(a[4]) if len(a) > 4 else 60):
            print(line)
    elif cmd == "func":
        start = img.func_start(int(a[3], 16))
        print(f"; function starts at {start:08X}")
        for line in img.dis(start, int(a[4]) if len(a) > 4 else 400, stop_at_ret=True):
            print(line)


if __name__ == "__main__":
    main()
