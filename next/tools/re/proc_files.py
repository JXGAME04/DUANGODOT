#!/usr/bin/env python3
"""Start a program, wait, then list the FILES it holds open and the modules it loaded, then stop it.

  proc_files.py <exe> [--wait S] [--keep]

Read-only on the target: handles are duplicated only to ask Windows for their path.
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

ntdll = ctypes.WinDLL("ntdll")
k32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)

PROCESS_DUP_HANDLE = 0x0040
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
DUPLICATE_SAME_ACCESS = 2
SystemExtendedHandleInformation = 64
FILE_TYPE_DISK = 1


class SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX(ctypes.Structure):
    _fields_ = [("Object", ctypes.c_void_p), ("UniqueProcessId", ctypes.c_size_t), ("HandleValue", ctypes.c_size_t),
                ("GrantedAccess", wt.ULONG), ("CreatorBackTraceIndex", wt.USHORT), ("ObjectTypeIndex", wt.USHORT),
                ("HandleAttributes", wt.ULONG), ("Reserved", wt.ULONG)]


def handles_of(pid):
    size = 1 << 22
    while True:
        buf = ctypes.create_string_buffer(size)
        ret = wt.ULONG()
        st = ntdll.NtQuerySystemInformation(SystemExtendedHandleInformation, buf, size, ctypes.byref(ret))
        if st == 0:
            break
        if st & 0xFFFFFFFF == 0xC0000004:
            size *= 2
            continue
        raise OSError(f"NtQuerySystemInformation {st & 0xFFFFFFFF:#x}")
    count = ctypes.c_size_t.from_buffer(buf, 0).value
    base = 2 * ctypes.sizeof(ctypes.c_size_t)
    esz = ctypes.sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX)
    out = []
    for i in range(count):
        e = SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX.from_buffer(buf, base + i * esz)
        if e.UniqueProcessId == pid:
            out.append(e.HandleValue)
    return out


def file_paths(pid):
    k32.OpenProcess.restype = wt.HANDLE
    hp = k32.OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not hp:
        raise OSError(ctypes.get_last_error())
    me = k32.GetCurrentProcess()
    k32.GetCurrentProcess.restype = wt.HANDLE
    me = k32.GetCurrentProcess()
    names = set()
    for hv in handles_of(pid):
        dup = wt.HANDLE()
        if not k32.DuplicateHandle(wt.HANDLE(hp), wt.HANDLE(hv), wt.HANDLE(me), ctypes.byref(dup), 0, False, DUPLICATE_SAME_ACCESS):
            continue
        try:
            if k32.GetFileType(dup) != FILE_TYPE_DISK:
                continue
            buf = ctypes.create_unicode_buffer(1024)
            n = k32.GetFinalPathNameByHandleW(dup, buf, 1024, 0)
            if n:
                names.add(buf.value.replace("\\\\?\\", ""))
        finally:
            k32.CloseHandle(dup)
    mods = []
    arr = (wt.HMODULE * 1024)()
    need = wt.DWORD()
    if psapi.EnumProcessModulesEx(wt.HANDLE(hp), arr, ctypes.sizeof(arr), ctypes.byref(need), 3):
        for i in range(need.value // ctypes.sizeof(wt.HMODULE)):
            buf = ctypes.create_unicode_buffer(1024)
            if psapi.GetModuleFileNameExW(wt.HANDLE(hp), wt.HMODULE(arr[i]), buf, 1024):
                mods.append(buf.value)
    k32.CloseHandle(wt.HANDLE(hp))
    return sorted(names), mods


def main():
    a = sys.argv[1:]
    exe = a[0]
    wait = float(a[a.index("--wait") + 1]) if "--wait" in a else 25.0
    proc = subprocess.Popen([exe], cwd=os.path.dirname(exe))
    print("started pid", proc.pid)
    time.sleep(wait)
    try:
        files, mods = file_paths(proc.pid)
        print(f"--- {len(files)} open disk files")
        for f in files:
            try:
                sz = os.path.getsize(f) if os.path.isfile(f) else -1
            except OSError:
                sz = -1
            print(f"   {sz:>12}  {f}")
        print(f"--- {len(mods)} modules (outside Windows)")
        for m in mods:
            if "\\windows\\" not in m.lower():
                print("   ", m)
    finally:
        if "--keep" not in a and proc.poll() is None:
            proc.terminate()
            print("terminated", proc.pid)


if __name__ == "__main__":
    main()
