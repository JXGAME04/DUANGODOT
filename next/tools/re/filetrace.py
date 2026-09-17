#!/usr/bin/env python3
"""Trace which files a 32-bit game opens through its engine, and where each one was found.

  filetrace.py <exe> <engine-dll-name> <seconds> <out.tsv> [name-filter-regex]

Runs the game under the Win32 debug API with breakpoints inside KPakFile::Open of the engine DLL
(offsets below are for engineFree.dll of the VLTK 2.0 client):

   +0x2DEB0  entry                        -> the file name asked for
   +0x2DECF  after KMemFile::Open         -> eax != 0: served from the in-memory file manager
   +0x2DEEE  after KFile::Open (mode 0)   -> eax != 0: served from a real file on disk
   +0x2DF01  after XPackList::FindElemFile (mode 0)
   +0x2DF12  after XPackList::FindElemFile (mode 1)
   +0x2DF1E  after KFile::Open (mode 1)

Read-only tracing: the only bytes written into the target are the INT3 breakpoints.
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import sys
import time

k32 = ctypes.WinDLL("kernel32", use_last_error=True)

DEBUG_ONLY_THIS_PROCESS = 0x2
EXCEPTION_DEBUG_EVENT, CREATE_PROCESS_DEBUG_EVENT, EXIT_PROCESS_DEBUG_EVENT, LOAD_DLL_DEBUG_EVENT = 1, 3, 5, 6
CREATE_THREAD_DEBUG_EVENT, EXIT_THREAD_DEBUG_EVENT = 2, 4
DBG_CONTINUE, DBG_EXCEPTION_NOT_HANDLED = 0x10002, 0x80010001
BP_CODES = (0x80000003, 0x4000001F)
STEP_CODES = (0x80000004, 0x4000001E)
WOW64_CONTEXT_CONTROL = 0x10001
WOW64_CONTEXT_FULL = 0x10007


class EXCEPTION_RECORD(ctypes.Structure):
    _fields_ = [("ExceptionCode", wt.DWORD), ("ExceptionFlags", wt.DWORD), ("ExceptionRecord", ctypes.c_void_p),
                ("ExceptionAddress", ctypes.c_void_p), ("NumberParameters", wt.DWORD),
                ("ExceptionInformation", ctypes.c_size_t * 15)]


class EXCEPTION_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("ExceptionRecord", EXCEPTION_RECORD), ("dwFirstChance", wt.DWORD)]


class CREATE_THREAD_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hThread", wt.HANDLE), ("lpThreadLocalBase", ctypes.c_void_p), ("lpStartAddress", ctypes.c_void_p)]


class CREATE_PROCESS_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hFile", wt.HANDLE), ("hProcess", wt.HANDLE), ("hThread", wt.HANDLE), ("lpBaseOfImage", ctypes.c_void_p),
                ("dwDebugInfoFileOffset", wt.DWORD), ("nDebugInfoSize", wt.DWORD), ("lpThreadLocalBase", ctypes.c_void_p),
                ("lpStartAddress", ctypes.c_void_p), ("lpImageName", ctypes.c_void_p), ("fUnicode", wt.WORD)]


class LOAD_DLL_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hFile", wt.HANDLE), ("lpBaseOfDll", ctypes.c_void_p), ("dwDebugInfoFileOffset", wt.DWORD),
                ("nDebugInfoSize", wt.DWORD), ("lpImageName", ctypes.c_void_p), ("fUnicode", wt.WORD)]


class U(ctypes.Union):
    _fields_ = [("Exception", EXCEPTION_DEBUG_INFO), ("CreateThread", CREATE_THREAD_DEBUG_INFO),
                ("CreateProcessInfo", CREATE_PROCESS_DEBUG_INFO), ("LoadDll", LOAD_DLL_DEBUG_INFO), ("pad", ctypes.c_byte * 164)]


class DEBUG_EVENT(ctypes.Structure):
    _fields_ = [("dwDebugEventCode", wt.DWORD), ("dwProcessId", wt.DWORD), ("dwThreadId", wt.DWORD), ("u", U)]


class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("lpReserved", wt.LPWSTR), ("lpDesktop", wt.LPWSTR), ("lpTitle", wt.LPWSTR),
                ("dwX", wt.DWORD), ("dwY", wt.DWORD), ("dwXSize", wt.DWORD), ("dwYSize", wt.DWORD),
                ("dwXCountChars", wt.DWORD), ("dwYCountChars", wt.DWORD), ("dwFillAttribute", wt.DWORD),
                ("dwFlags", wt.DWORD), ("wShowWindow", wt.WORD), ("cbReserved2", wt.WORD), ("lpReserved2", ctypes.c_void_p),
                ("hStdInput", wt.HANDLE), ("hStdOutput", wt.HANDLE), ("hStdError", wt.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", wt.HANDLE), ("hThread", wt.HANDLE), ("dwProcessId", wt.DWORD), ("dwThreadId", wt.DWORD)]


class FLOATING_SAVE_AREA(ctypes.Structure):
    _fields_ = [("ControlWord", wt.DWORD), ("StatusWord", wt.DWORD), ("TagWord", wt.DWORD), ("ErrorOffset", wt.DWORD),
                ("ErrorSelector", wt.DWORD), ("DataOffset", wt.DWORD), ("DataSelector", wt.DWORD),
                ("RegisterArea", ctypes.c_byte * 80), ("Cr0NpxState", wt.DWORD)]


class CONTEXT32(ctypes.Structure):
    _fields_ = [("ContextFlags", wt.DWORD), ("Dr0", wt.DWORD), ("Dr1", wt.DWORD), ("Dr2", wt.DWORD), ("Dr3", wt.DWORD),
                ("Dr6", wt.DWORD), ("Dr7", wt.DWORD), ("FloatSave", FLOATING_SAVE_AREA), ("SegGs", wt.DWORD),
                ("SegFs", wt.DWORD), ("SegEs", wt.DWORD), ("SegDs", wt.DWORD), ("Edi", wt.DWORD), ("Esi", wt.DWORD),
                ("Ebx", wt.DWORD), ("Edx", wt.DWORD), ("Ecx", wt.DWORD), ("Eax", wt.DWORD), ("Ebp", wt.DWORD),
                ("Eip", wt.DWORD), ("SegCs", wt.DWORD), ("EFlags", wt.DWORD), ("Esp", wt.DWORD), ("SegSs", wt.DWORD),
                ("ExtendedRegisters", ctypes.c_byte * 512)]


k32.CreateProcessW.argtypes = [wt.LPCWSTR, wt.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, wt.BOOL, wt.DWORD, ctypes.c_void_p,
                               wt.LPCWSTR, ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION)]
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.WriteProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.FlushInstructionCache.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t]
k32.GetFinalPathNameByHandleW.argtypes = [wt.HANDLE, wt.LPWSTR, wt.DWORD, wt.DWORD]
k32.OpenThread.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
k32.OpenThread.restype = wt.HANDLE
k32.Wow64GetThreadContext.argtypes = [wt.HANDLE, ctypes.POINTER(CONTEXT32)]
k32.Wow64SetThreadContext.argtypes = [wt.HANDLE, ctypes.POINTER(CONTEXT32)]
k32.WaitForDebugEvent.argtypes = [ctypes.POINTER(DEBUG_EVENT), wt.DWORD]
k32.ContinueDebugEvent.argtypes = [wt.DWORD, wt.DWORD, wt.DWORD]
k32.TerminateProcess.argtypes = [wt.HANDLE, wt.UINT]
k32.DebugSetProcessKillOnExit.argtypes = [wt.BOOL]

psapi = ctypes.WinDLL("psapi", use_last_error=True)
ENTRY_BYTES = bytes([0x56, 0x8B])   # push esi; mov esi,[esp+8]: the first bytes of KPakFile::Open


class MODULEINFO(ctypes.Structure):
    _fields_ = [("lpBaseOfDll", ctypes.c_void_p), ("SizeOfImage", wt.DWORD), ("EntryPoint", ctypes.c_void_p)]


def module_base(hproc, dllname):
    arr = (wt.HMODULE * 1024)()
    need = wt.DWORD()
    if not psapi.EnumProcessModulesEx(wt.HANDLE(hproc), arr, ctypes.sizeof(arr), ctypes.byref(need), 3):
        return None
    for i in range(need.value // ctypes.sizeof(wt.HMODULE)):
        buf = ctypes.create_unicode_buffer(1024)
        if psapi.GetModuleFileNameExW(wt.HANDLE(hproc), wt.HMODULE(arr[i]), buf, 1024):
            if os.path.basename(buf.value).lower() == dllname:
                return arr[i] or None
    return None


OFF_ENTRY, OFF_MEM, OFF_FILE0, OFF_PAK0, OFF_PAK1, OFF_FILE1 = 0x2DEB0, 0x2DECF, 0x2DEEE, 0x2DF01, 0x2DF12, 0x2DF1E
SOURCES = {OFF_MEM: "memfile", OFF_FILE0: "disk", OFF_PAK0: "pak", OFF_PAK1: "pak", OFF_FILE1: "disk"}


def main():
    exe, dllname, seconds, out = sys.argv[1], sys.argv[2].lower(), float(sys.argv[3]), sys.argv[4]
    flt = re.compile(sys.argv[5], re.I) if len(sys.argv) > 5 else None
    si = STARTUPINFO()
    si.cb = ctypes.sizeof(si)
    pi = PROCESS_INFORMATION()
    if not k32.CreateProcessW(exe, None, None, None, False, DEBUG_ONLY_THIS_PROCESS, None, os.path.dirname(exe),
                              ctypes.byref(si), ctypes.byref(pi)):
        sys.exit(f"CreateProcess failed: {ctypes.get_last_error()}")
    k32.DebugSetProcessKillOnExit(True)
    hproc = pi.hProcess
    threads = {pi.dwThreadId: pi.hThread}
    bps = {}          # address -> original byte
    pending = {}      # thread id -> address whose breakpoint must be put back after a single step
    current = {}      # thread id -> the name it is opening
    results = []      # (name, source)
    base = None
    mapped = False

    def rd(addr, n):
        buf = ctypes.create_string_buffer(n)
        got = ctypes.c_size_t()
        k32.ReadProcessMemory(hproc, ctypes.c_void_p(addr), buf, n, ctypes.byref(got))
        return buf.raw[:got.value]

    def wr(addr, data):
        got = ctypes.c_size_t()
        k32.WriteProcessMemory(hproc, ctypes.c_void_p(addr), data, len(data), ctypes.byref(got))
        k32.FlushInstructionCache(hproc, ctypes.c_void_p(addr), len(data))

    def set_bp(addr):
        orig = rd(addr, 1)
        if len(orig) == 1:
            bps[addr] = orig
            wr(addr, b"\xcc")

    def cstring(addr):
        raw = rd(addr, 300)
        end = raw.find(b"\0")
        return raw[:end if end >= 0 else len(raw)]

    ev = DEBUG_EVENT()
    deadline = time.time() + seconds
    first_bp = True
    while time.time() < deadline:
        if not k32.WaitForDebugEvent(ctypes.byref(ev), 200):
            continue
        status = DBG_CONTINUE
        code = ev.dwDebugEventCode
        if mapped and base is None:
            found = module_base(hproc, dllname)
            if found and rd(found + OFF_ENTRY, 2) == ENTRY_BYTES:
                base = found
                for off in (OFF_ENTRY, OFF_MEM, OFF_FILE0, OFF_PAK0, OFF_PAK1, OFF_FILE1):
                    set_bp(base + off)
                print(f"{dllname} at {base:#x}: breakpoints set", flush=True)
        if code == CREATE_THREAD_DEBUG_EVENT:
            threads[ev.dwThreadId] = ev.u.CreateThread.hThread
        elif code == EXIT_THREAD_DEBUG_EVENT:
            threads.pop(ev.dwThreadId, None)
        elif code == LOAD_DLL_DEBUG_EVENT:
            buf = ctypes.create_unicode_buffer(1024)
            h = ev.u.LoadDll.hFile
            if h and k32.GetFinalPathNameByHandleW(h, buf, 1024, 0):
                if os.path.basename(buf.value).lower() == dllname:
                    mapped = True   # mapped, but maybe not relocated yet: wait for the loader to list it
            if h:
                k32.CloseHandle(h)
        elif code == EXIT_PROCESS_DEBUG_EVENT:
            print("process exited")
            break
        elif code == EXCEPTION_DEBUG_EVENT:
            rec = ev.u.Exception.ExceptionRecord
            xc = rec.ExceptionCode
            addr = rec.ExceptionAddress or 0
            tid = ev.dwThreadId
            ht = threads.get(tid)
            if xc in BP_CODES and addr in bps and ht:
                ctx = CONTEXT32()
                ctx.ContextFlags = WOW64_CONTEXT_FULL
                k32.Wow64GetThreadContext(ht, ctypes.byref(ctx))
                off = addr - base
                if off == OFF_ENTRY:
                    arg = int.from_bytes(rd(ctx.Esp + 4, 4), "little")
                    current[tid] = cstring(arg)
                elif off in SOURCES and ctx.Eax != 0 and tid in current:
                    src = SOURCES[off]
                    if src == "pak":
                        # XPackElemFileRef: id, pak index (u16 at +4), element index, cache, offset?, size
                        ref = ctx.Edi if off == OFF_PAK0 else ctx.Edi + 0x1C
                        raw = rd(ref, 0x18)
                        if len(raw) == 0x18:
                            uid = int.from_bytes(raw[0:4], "little")
                            pak = int.from_bytes(raw[4:6], "little")
                            elem = int.from_bytes(raw[8:12], "little")
                            size = int.from_bytes(raw[20:24], "little")
                            src = f"pak#{pak} id={uid:08x} elem={elem} size={size}"
                    results.append((current.pop(tid), src))
                elif off in (OFF_PAK0, OFF_FILE1) and ctx.Eax == 0 and tid in current:
                    results.append((current.pop(tid), "NOT FOUND"))
                wr(addr, bps[addr])
                ctx.Eip = addr
                ctx.EFlags |= 0x100
                k32.Wow64SetThreadContext(ht, ctypes.byref(ctx))
                pending[tid] = addr
            elif xc in STEP_CODES and tid in pending:
                wr(pending.pop(tid), b"\xcc")
            elif xc in BP_CODES:
                first_bp = False   # the loader's own initial breakpoint
            else:
                status = DBG_EXCEPTION_NOT_HANDLED
        k32.ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status)

    k32.TerminateProcess(hproc, 0)
    with open(out, "wb") as f:
        for n, (name, src) in enumerate(results):
            if flt and not flt.search(name.decode("latin-1")):
                continue
            f.write(str(n).encode() + b"\t" + src.encode() + b"\t" + name + b"\n")
    by = {}
    for name, src in results:
        key = src.split(" ")[0]
        by[key] = by.get(key, 0) + 1
    print(f"{len(results)} opens: {dict(sorted(by.items()))} -> {out}")


if __name__ == "__main__":
    main()
