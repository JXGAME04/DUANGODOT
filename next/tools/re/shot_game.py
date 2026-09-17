#!/usr/bin/env python3
"""Start a game client, wait, capture ONLY its own window(s) with PrintWindow, optionally click, stop it.

  shot_game.py <exe> <out-prefix> [--wait S] [--shots N] [--every S] [--keep] [--click x,y ...]

Only windows owned by the process started here are captured - nothing else on the desktop.
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

from PIL import Image

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32
user32.SetProcessDPIAware()


def windows_of(pid):
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(hwnd, _):
        p = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid and user32.IsWindowVisible(hwnd):
            r = wt.RECT()
            user32.GetWindowRect(hwnd, ctypes.byref(r))
            n = user32.GetWindowTextLengthW(hwnd)
            buf = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(hwnd, buf, n + 1)
            found.append((hwnd, buf.value, r.right - r.left, r.bottom - r.top))
        return True

    user32.EnumWindows(cb, 0)
    return found


def capture(hwnd, path, client_only=True):
    r = wt.RECT()
    if client_only:
        user32.GetClientRect(hwnd, ctypes.byref(r))
    else:
        user32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    if w <= 0 or h <= 0:
        return False
    hdc = user32.GetWindowDC(hwnd)
    mdc = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    gdi32.SelectObject(mdc, bmp)
    PW_CLIENTONLY, PW_RENDERFULLCONTENT = 1, 2
    ok = user32.PrintWindow(hwnd, mdc, (PW_CLIENTONLY if client_only else 0) | PW_RENDERFULLCONTENT)

    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD),
                    ("biBitCount", wt.WORD), ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                    ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                    ("biClrImportant", wt.DWORD)]

    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(bi)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = w, -h, 1, 32
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(mdc)
    user32.ReleaseDC(hwnd, hdc)
    img = Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB")
    img.save(path)
    return bool(ok)


def main():
    a = sys.argv[1:]
    exe, prefix = a[0], a[1]
    wait, shots, every, keep = 20.0, 1, 5.0, False
    i = 2
    while i < len(a):
        if a[i] == "--wait":
            wait = float(a[i + 1]); i += 2
        elif a[i] == "--shots":
            shots = int(a[i + 1]); i += 2
        elif a[i] == "--every":
            every = float(a[i + 1]); i += 2
        elif a[i] == "--keep":
            keep = True; i += 1
        else:
            i += 1
    proc = subprocess.Popen([exe], cwd=os.path.dirname(exe))
    print("started pid", proc.pid)
    time.sleep(wait)
    try:
        for n in range(shots):
            if proc.poll() is not None:
                print("process exited with", proc.returncode)
                break
            wins = windows_of(proc.pid)
            print(f"shot {n}: windows", [(t, w, h) for _, t, w, h in wins])
            for k, (hwnd, title, w, h) in enumerate(wins):
                if w < 200 or h < 150:
                    continue
                out = f"{prefix}_{n}_{k}.png"
                ok = capture(hwnd, out)
                print("   saved", out, "PrintWindow ok" if ok else "PrintWindow returned 0")
            time.sleep(every)
    finally:
        if not keep and proc.poll() is None:
            proc.terminate()
            print("terminated", proc.pid)


if __name__ == "__main__":
    main()
