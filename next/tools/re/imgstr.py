#!/usr/bin/env python3
"""Strings of an unpacked PE image.

  imgstr.py <img> near <va_lo> <va_hi>      every string between two virtual addresses
  imgstr.py <img> grep <regex>              strings matching (on the GBK-decoded text)
Image base 0x400000, first section at RVA 0x1000 (what upx_unpack.py writes).
"""
import re
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
BASE = 0x401000
img = open(sys.argv[1], "rb").read()
PAT = re.compile(rb"[\x20-\x7e\x81-\xfe]{3,300}\x00")


def show(s):
    try:
        return s.decode("gbk")
    except UnicodeDecodeError:
        return s.decode("latin-1")


mode = sys.argv[2]
if mode == "near":
    lo, hi = int(sys.argv[3], 16) - BASE, int(sys.argv[4], 16) - BASE
    for m in PAT.finditer(img[lo:hi]):
        print(f"{BASE + lo + m.start():08X}  {show(m.group()[:-1])}")
elif mode == "grep":
    rx = re.compile(sys.argv[3], re.I)
    for m in PAT.finditer(img):
        t = show(m.group()[:-1])
        if rx.search(t):
            print(f"{BASE + m.start():08X}  {t}")
