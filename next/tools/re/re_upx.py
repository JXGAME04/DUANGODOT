#!/usr/bin/env python3
"""Unpack a UPX-packed 32-bit PE (the 2.0 client's gamecl.exe: UPX 3.03, NRV2E, filter 0x24) into
a raw memory image the other re_* tools can read - they take `<exe>.unpacked.img` like an ELF.

  re_upx.py <exe>                      writes <exe>.unpacked.img + <exe>.unpacked.json

The loader stub of the packed file says everything the unpacker needs (dis of its entry point):
  mov esi, <packed data va>; lea edi, [esi - N]   -> where the data goes (the first section's va)
  the NRV2E decompressor inline
  pop esi; mov ecx, <len>; mov al, 0xE8; repne scasb; cmp byte [edi], <cto>   -> the call filter
  lea edi, [esi + <imports>] ... call [esi + <LoadLibraryA>] / [esi + <GetProcAddress>]
  jmp <original entry point>
The pack header ("UPX!" + version, format, method, level, lengths, filter, cto) sits in the
header padding.  Only what the client needs is implemented: method 8 (NRV2E LE32) and the
E8 / E8E9 call-trick filters 0x24 / 0x26 (the loader's own numbers are used, not the header's).
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
import re_elf  # noqa: E402

from capstone import CS_ARCH_X86, CS_MODE_32, Cs  # noqa: E402
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG  # noqa: E402

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def nrv2e_decompress(src, out_len):
    """ucl_nrv2e_decompress_le32 (UCL n2e_d.c) with a fixed output length."""
    out = bytearray(out_len)
    ip = 0
    op = 0
    bb = 0
    bc = 0
    last_m_off = 1
    n = len(src)

    def getbit():
        nonlocal bb, bc, ip
        if bc == 0:
            bb = struct.unpack_from("<I", src, ip)[0]
            ip += 4
            bc = 32
        bc -= 1
        return (bb >> bc) & 1

    while ip < n:
        while getbit():
            out[op] = src[ip]
            op += 1
            ip += 1
        m_off = 1
        while True:
            m_off = m_off * 2 + getbit()
            if getbit():
                break
            m_off = (m_off - 1) * 2 + getbit()
        if m_off == 2:
            m_off = last_m_off
            m_len = getbit()
        else:
            m_off = (m_off - 3) * 256 + src[ip]
            ip += 1
            if m_off == 0xFFFFFFFF:
                break
            m_len = (m_off ^ 0xFFFFFFFF) & 1
            m_off >>= 1
            m_off += 1
            last_m_off = m_off
        if m_len:
            m_len = 1 + getbit()
        elif getbit():
            m_len = 3 + getbit()
        else:
            m_len += 1
            while True:
                m_len = m_len * 2 + getbit()
                if getbit():
                    break
            m_len += 3
        if m_off > 0x500:
            m_len += 1
        # copy m_len + 1 bytes from op - m_off (may overlap)
        src_pos = op - m_off
        if src_pos < 0:
            raise ValueError("nrv2e: bad match offset at %d" % op)
        for _ in range(m_len + 1):
            out[op] = out[src_pos]
            op += 1
            src_pos += 1
    return bytes(out[:op])


def unfilter_cto32(buf, length, cto8, e9=False):
    """u_cto32_e8[e9]_bswap_le of UPX filter/ctok.h with addvalue 0: a call whose rel32 was stored
    as big-endian absolute (cto8 in the high byte) becomes a rel32 again."""
    b = bytearray(buf)
    ic = 0
    end = min(length, len(b)) - 5
    calls = 0
    while ic < end:
        op = b[ic]
        if op == 0xE8 or (e9 and op == 0xE9):
            if b[ic + 1] == cto8:
                jc = (b[ic + 2] << 16) | (b[ic + 3] << 8) | b[ic + 4]
                rel = (jc - ic - 1) & 0xFFFFFFFF
                b[ic + 1:ic + 5] = struct.pack("<I", rel)
                ic += 5
                calls += 1
                continue
        ic += 1
    return bytes(b), calls


def read_loader(d, ep_off, ep_va):
    """The parameters the UPX 3.x win32 loader carries as immediates."""
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    params = {"unfilter_len": 0, "cto": None, "e9": False}
    ins_list = list(md.disasm(d[ep_off:ep_off + 0x400], ep_va))
    for i, ins in enumerate(ins_list):
        if ins.mnemonic == "mov" and ins.operands[0].type == X86_OP_REG and ins.operands[1].type == X86_OP_IMM:
            reg = ins.reg_name(ins.operands[0].reg)
            imm = ins.operands[1].imm & 0xFFFFFFFF
            if reg == "esi" and "src" not in params:
                params["src"] = imm
            elif reg == "ecx" and params["unfilter_len"] == 0 and imm > 0x1000:
                params["unfilter_len"] = imm
            elif reg == "al" and imm == 0xE9:
                params["e9"] = True
        elif ins.mnemonic == "lea" and "src" in params and "dst" not in params and ins.operands[1].type == X86_OP_MEM:
            params["dst"] = (params["src"] + ins.operands[1].mem.disp) & 0xFFFFFFFF
        elif ins.mnemonic == "cmp" and ins.operands[0].type == X86_OP_MEM and ins.operands[1].type == X86_OP_IMM and params["cto"] is None:
            params["cto"] = ins.operands[1].imm & 0xFF
        elif ins.mnemonic == "lea" and "dst" in params and ins.operands[1].type == X86_OP_MEM and ins.operands[1].mem.disp > 0x100000 and "imports" not in params:
            params["imports"] = ins.operands[1].mem.disp
        elif ins.mnemonic == "lea" and "imports" in params and "names" not in params and ins.operands[1].type == X86_OP_MEM and ins.operands[1].mem.index != 0:
            params["names"] = ins.operands[1].mem.disp
        elif ins.mnemonic == "jmp" and ins.operands[0].type == X86_OP_IMM and i > 20:
            params["oep"] = ins.operands[0].imm & 0xFFFFFFFF
    return params


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    path = sys.argv[1]
    d = open(path, "rb").read()
    pe_off = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", d, pe_off + 20)[0]
    ep_rva = struct.unpack_from("<I", d, pe_off + 24 + 16)[0]
    base = struct.unpack_from("<I", d, pe_off + 24 + 28)[0]
    secs = []
    at = pe_off + 24 + opt_size
    for _ in range(nsec):
        name, vsize, va, rsize, roff = struct.unpack_from("<8sIIII", d, at)
        secs.append({"name": name.rstrip(b"\0").decode("latin-1"), "va": va, "vsize": vsize, "off": roff, "rsize": rsize})
        at += 40

    def off_of(rva):
        for s in secs:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s["off"] + rva - s["va"]
        return None

    hdr = d.find(b"UPX!")
    if hdr < 0:
        raise SystemExit("not a UPX file")
    version, fmt, method, level = d[hdr + 4], d[hdr + 5], d[hdr + 6], d[hdr + 7]
    u_adler, c_adler, u_len, c_len, u_file_size = struct.unpack_from("<5I", d, hdr + 8)
    print(f"UPX header: version {version} format {fmt} method {method} level {level} u_len {u_len} c_len {c_len}")
    if method != 8:
        raise SystemExit("only NRV2E (method 8) is implemented; this file uses %d" % method)
    ep_off = off_of(ep_rva)
    p = read_loader(d, ep_off, base + ep_rva)
    print("loader:", {k: (hex(v) if isinstance(v, int) else v) for k, v in p.items()})
    src_off = off_of(p["src"] - base)
    print(f"decompressing {c_len} bytes at file offset {src_off:#x} into {u_len} bytes ...")
    data = nrv2e_decompress(d[src_off:src_off + c_len + 16], u_len)
    print(f"decompressed {len(data)} bytes")
    if p["cto"] is not None and p["unfilter_len"]:
        data, calls = unfilter_cto32(data, p["unfilter_len"], p["cto"], p["e9"])
        print(f"unfiltered {calls} calls in the first {p['unfilter_len']:#x} bytes (cto {p['cto']:#x})")
    dst_rva = p["dst"] - base
    # the imports the loader rebuilds: {dll off, iat off} then names (bit 7 = ordinal) until 0
    imports = {}
    if "imports" in p and "names" in p:
        names_base = p["dst"] + p["names"]   # va of the name table (in the packed file's sections)
        q = p["imports"]   # offset from dst
        while True:
            dll_off, iat_rva = struct.unpack_from("<II", data, q)
            if dll_off == 0:
                break
            dll_name = re_elf_cstr(d, off_of(names_base + dll_off - base))
            q += 8
            slot = p["dst"] + iat_rva   # the loader adds the destination va (esi), not the image base
            while True:
                c = data[q]
                q += 1
                if c == 0:
                    break
                if c & 0x80:
                    ordinal = struct.unpack_from("<H", data, q)[0]
                    q += 2
                    imports[slot] = f"{dll_name}!#{ordinal}"
                else:
                    end = data.index(b"\0", q)
                    imports[slot] = f"{dll_name}!{data[q:end].decode('latin-1')}"
                    q = end + 1
                slot += 4
    out_img = path + ".unpacked.img"
    open(out_img, "wb").write(data)
    meta = {
        "base": base,
        "entry": p.get("oep", 0),
        "segments": [{"va": base + dst_rva, "off": 0, "filesz": len(data), "memsz": len(data), "flags": 7}],
        "imports": {f"{k:#x}": v for k, v in imports.items()},
        "packed": os.path.basename(path),
    }
    json.dump(meta, open(path + ".unpacked.json", "w"), indent=1)
    print(f"wrote {out_img} ({len(data)} bytes) + .json: entry {meta['entry']:#x}, {len(imports)} imports")


def re_elf_cstr(d, off):
    end = d.index(b"\0", off)
    return d[off:end].decode("latin-1")


if __name__ == "__main__":
    main()
