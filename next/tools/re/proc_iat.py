#!/usr/bin/env python3
"""Resolve import-table slots of a running UPX-packed game: start it, read the slots, name them.

  proc_iat.py <exe> <slot-va-hex>... [--range lo hi] [--wait S] [--dump out.json]

UPX rebuilds the import table at start-up, so only the live process knows which function each
`call [slot]` of the unpacked image really calls.
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import struct
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from re_dll import PE  # noqa: E402

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
k32.OpenProcess.restype = wt.HANDLE


class MODULEINFO(ctypes.Structure):
    _fields_ = [("lpBaseOfDll", ctypes.c_void_p), ("SizeOfImage", wt.DWORD), ("EntryPoint", ctypes.c_void_p)]


def modules(hp):
    arr = (wt.HMODULE * 1024)()
    need = wt.DWORD()
    out = []
    if psapi.EnumProcessModulesEx(wt.HANDLE(hp), arr, ctypes.sizeof(arr), ctypes.byref(need), 3):
        for i in range(need.value // ctypes.sizeof(wt.HMODULE)):
            buf = ctypes.create_unicode_buffer(1024)
            psapi.GetModuleFileNameExW(wt.HANDLE(hp), wt.HMODULE(arr[i]), buf, 1024)
            mi = MODULEINFO()
            psapi.GetModuleInformation(wt.HANDLE(hp), wt.HMODULE(arr[i]), ctypes.byref(mi), ctypes.sizeof(mi))
            out.append((mi.lpBaseOfDll or 0, mi.SizeOfImage, buf.value))
    return out


def main():
    a = sys.argv[1:]
    exe = a[0]
    wait = float(a[a.index("--wait") + 1]) if "--wait" in a else 12.0
    slots = [int(x, 16) for x in a[1:] if not x.startswith("--") and all(c in "0123456789abcdefABCDEFx" for c in x)]
    if "--range" in a:
        i = a.index("--range")
        lo, hi = int(a[i + 1], 16), int(a[i + 2], 16)
        slots = list(range(lo, hi, 4))
    proc = subprocess.Popen([exe], cwd=os.path.dirname(exe))
    time.sleep(wait)
    result = {}
    try:
        hp = k32.OpenProcess(0x0410, False, proc.pid)
        mods = modules(hp)
        cache = {}
        for slot in slots:
            buf = ctypes.create_string_buffer(4)
            got = ctypes.c_size_t()
            if not k32.ReadProcessMemory(wt.HANDLE(hp), ctypes.c_void_p(slot), buf, 4, ctypes.byref(got)):
                continue
            target = struct.unpack("<I", buf.raw)[0]
            name = None
            for base, size, path in mods:
                if base <= target < base + size:
                    if path not in cache:
                        try:
                            pe = PE(path)
                            cache[path] = {va - pe.base: n for n, va in pe.exports.items()}
                        except Exception:  # noqa: BLE001
                            cache[path] = {}
                    name = f"{os.path.basename(path)}!{cache[path].get(target - base, hex(target - base))}"
                    break
            if name:
                result[f"{slot:08X}"] = name
        k32.CloseHandle(wt.HANDLE(hp))
    finally:
        if proc.poll() is None:
            proc.terminate()
    if "--dump" in a:
        with open(a[a.index("--dump") + 1], "w", encoding="utf-8") as f:
            json.dump(result, f, indent=1)
        print(len(result), "slots resolved ->", a[a.index("--dump") + 1])
    else:
        for k, v in result.items():
            print(k, v)


if __name__ == "__main__":
    main()
