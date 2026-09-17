#!/usr/bin/env python3
"""iatcalls.py <unfiltered-img> <slot-va-hex>...: every `call [slot]` / `mov reg,[slot]` of an import slot."""
import struct
import sys

BASE = 0x401000
CODE_END = 0x78316C - BASE
d = open(sys.argv[1], "rb").read()
for arg in sys.argv[2:]:
    slot = int(arg, 16)
    raw = struct.pack("<I", slot)
    i = 0
    print(f"slot {slot:08X}:")
    while True:
        i = d.find(raw, i, CODE_END)
        if i < 0:
            break
        kind = "?"
        if d[i - 2:i] == b"\xff\x15":
            kind = "call"
        elif d[i - 2] == 0x8B:
            kind = "mov reg"
        elif d[i - 1] == 0xA1:
            kind = "mov eax"
        elif d[i - 2:i] == b"\xff\x25":
            kind = "jmp"
        print(f"   {BASE + i - 2:08X}  {kind}")
        i += 1
