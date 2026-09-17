#!/usr/bin/env python3
"""callers.py <unfiltered-img> <target-va-hex> [code-end-hex]: every `call rel32` that lands on target,
plus vtable slots that hold it."""
import struct
import sys

BASE = 0x401000
d = open(sys.argv[1], "rb").read()
target = int(sys.argv[2], 16)
code_end = (int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x78316C) - BASE
i = 0
while True:
    i = d.find(b"\xe8", i, code_end)
    if i < 0:
        break
    rel = struct.unpack_from("<i", d, i + 1)[0]
    if BASE + i + 5 + rel == target:
        print(f"call at {BASE + i:08X}")
    i += 1
raw = struct.pack("<I", target)
j = code_end
while True:
    j = d.find(raw, j)
    if j < 0:
        break
    print(f"pointer at {BASE + j:08X} (vtable / table slot)")
    j += 1
