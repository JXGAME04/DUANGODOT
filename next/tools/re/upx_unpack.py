#!/usr/bin/env python3
"""Unpack the image of a UPX-packed 32-bit PE so it can be read and disassembled.

  upx_unpack.py <packed.exe> <out.bin>

The VLTK 2.0 client (gamecl.exe, katgame.dll) is packed with UPX 3.x: method 8 = NRV2E_LE32,
filter 0x24 (call trick on E8, the "cto" byte picks which calls were rewritten).  The output is
the unpacked image starting at the RVA of the first section (0x1000), with the call filter undone
over the original .text, so `call` targets are real.  Imports are NOT rebuilt: UPX does that at
run time, which is why proc_iat.py reads the import slots from the live process instead.

Pure Python, no dependencies.  What is written here is only for reading: it is not a runnable exe.
"""
import struct
import sys
import time

sys.stdout.reconfigure(encoding="utf-8", errors="replace")


class Bits:
    """UCL bit reader, 32 bits at a time, little endian (the *_LE32 variants)."""

    def __init__(self, src, pos=0):
        self.src, self.pos, self.bb, self.bc = src, pos, 0, 0

    def bit(self):
        if self.bc == 0:
            self.bb = struct.unpack_from("<I", self.src, self.pos)[0]
            self.pos += 4
            self.bc = 32
        self.bc -= 1
        return (self.bb >> self.bc) & 1

    def byte(self):
        b = self.src[self.pos]
        self.pos += 1
        return b


def copy(dst, off, n):
    start = len(dst) - off
    if start < 0:
        raise ValueError("match before the start of the output")
    if off >= n:
        dst += dst[start:start + n]
    else:                     # overlapping match: the run repeats with period `off`
        chunk = bytes(dst[start:])
        reps = n // off + 1
        dst += (chunk * reps)[:n]


def nrv2e(src, pos=0):
    r, dst, last = Bits(src, pos), bytearray(), 1
    while True:
        while r.bit():
            dst.append(r.byte())
        m_off = 1
        while True:
            m_off = m_off * 2 + r.bit()
            if r.bit():
                break
            m_off = (m_off - 1) * 2 + r.bit()
        if m_off == 2:
            m_off, m_len = last, r.bit()
        else:
            m_off = (m_off - 3) * 256 + r.byte()
            if m_off == 0xFFFFFFFF:
                break
            m_len = (m_off ^ 0xFFFFFFFF) & 1
            m_off >>= 1
            m_off += 1
            last = m_off
        if m_len:
            m_len = 1 + r.bit()
        elif r.bit():
            m_len = 3 + r.bit()
        else:
            m_len += 1
            while True:
                m_len = m_len * 2 + r.bit()
                if r.bit():
                    break
            m_len += 3
        m_len += m_off > 0x500
        copy(dst, m_off, m_len + 1)
    return dst


def nrv2b(src, pos=0):
    r, dst, last = Bits(src, pos), bytearray(), 1
    while True:
        while r.bit():
            dst.append(r.byte())
        m_off = 1
        while True:
            m_off = m_off * 2 + r.bit()
            if r.bit():
                break
        if m_off == 2:
            m_off = last
        else:
            m_off = (m_off - 3) * 256 + r.byte()
            if m_off == 0xFFFFFFFF:
                break
            m_off += 1
            last = m_off
        m_len = r.bit()
        m_len = m_len * 2 + r.bit()
        if m_len == 0:
            m_len = 1
            while True:
                m_len = m_len * 2 + r.bit()
                if r.bit():
                    break
            m_len += 2
        m_len += m_off > 0xD00
        copy(dst, m_off, m_len + 1)
    return dst


METHODS = {2: nrv2b, 8: nrv2e}


def sections(d, pe):
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    out = []
    for i in range(nsec):
        s = pe + 24 + optsz + i * 40
        name = bytes(d[s:s + 8]).rstrip(b"\0").decode("latin-1")
        vsz, rva, rsz, roff = struct.unpack_from("<IIII", d, s + 8)
        out.append((name, rva, vsz, roff, rsz))
    return out


def main():
    src_path, out_path = sys.argv[1], sys.argv[2]
    d = open(src_path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    entry = struct.unpack_from("<I", d, pe + 40)[0]
    image_base = struct.unpack_from("<I", d, pe + 52)[0]
    secs = sections(d, pe)

    def rva2off(rva):
        for _, va, vsz, roff, rsz in secs:
            if va <= rva < va + max(vsz, rsz):
                return roff + rva - va
        return None

    ph = d.find(b"UPX!")
    if ph < 0:
        sys.exit("no UPX! pack header: the file is not packed (read it as it is)")
    method, filt, cto = d[ph + 6], d[ph + 28], d[ph + 29]
    u_len, c_len = struct.unpack_from("<II", d, ph + 16)
    print(f"pack header: method {method}, {c_len} -> {u_len} bytes, filter {filt:#x}, cto {cto:#x}")
    if method not in METHODS:
        sys.exit(f"method {method} is not supported here (2 = NRV2B_LE32, 8 = NRV2E_LE32)")

    # the stub starts `pushad; mov esi, <address of the packed data>`
    stub = rva2off(entry)
    if d[stub] != 0x60 or d[stub + 1] != 0xBE:
        sys.exit("entry stub is not `pushad; mov esi, imm32`")
    packed = rva2off(struct.unpack_from("<I", d, stub + 2)[0] - image_base)
    t = time.time()
    img = METHODS[method](d, packed)
    print(f"unpacked {len(img)} bytes in {time.time() - t:.1f} s")
    if len(img) != u_len:
        sys.exit(f"size mismatch: got {len(img)}, the header says {u_len}")

    # UPX keeps the original section table after the unpacked data: the filter covers .text only
    text_end = None
    at = bytes(img).rfind(b"PE\x00\x00\x4c\x01")
    if at >= 0:
        for name, rva, vsz, _, _ in sections(img, at):
            if name == ".text":
                text_end = rva + vsz - secs[0][1]
    if filt in (0x24, 0x26) and text_end:
        n, i, end = 0, 0, text_end - 5
        while i < end:
            b = img[i]
            if (b == 0xE8 or (filt == 0x26 and b == 0xE9)) and img[i + 1] == cto:
                target = struct.unpack_from(">I", img, i + 1)[0] - (cto << 24)
                struct.pack_into("<I", img, i + 1, (target - (i + 1)) & 0xFFFFFFFF)
                n += 1
                i += 5
            else:
                i += 1
        print(f"call filter undone on {n} calls (.text ends at image offset {text_end:#x})")
    elif filt:
        print(f"filter {filt:#x} left as it is: call targets in the output are NOT real")
    open(out_path, "wb").write(img)
    print(f"wrote {out_path}: image base {image_base:#x}, first byte = RVA {secs[0][1]:#x}")


if __name__ == "__main__":
    main()
