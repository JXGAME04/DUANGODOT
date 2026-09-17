#!/usr/bin/env python3
"""Start a game client, walk through its windows and capture ONLY its own window with PrintWindow.

  shot_game.py <exe> <out-prefix> [--wait S] [--do "<steps>"] [--keep]

<steps> is a ';'-separated script, run after the first wait:
  shot            save <out-prefix>_<n>.png (n counts up from 0)
  wait:2.5        sleep
  key:enter       a key press sent to the game's window (enter, esc, tab, up, down, left, right)
  click:x,y       a left click at client coordinates of the game's window
  move:x,y        move the mouse there (hover states)
Without --do it takes one shot.

Input goes to the window of the process started here as window messages (PostMessage): the real
mouse and keyboard are not touched and nothing else on the desktop is looked at or clicked.  The tool
is for LOOKING at the old client's windows; it types no text, so it cannot log in anywhere.
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

WM_KEYDOWN, WM_KEYUP = 0x0100, 0x0101
WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP = 0x0200, 0x0201, 0x0202
MK_LBUTTON = 0x0001
KEYS = {"enter": 0x0D, "esc": 0x1B, "tab": 0x09, "up": 0x26, "down": 0x28, "left": 0x25, "right": 0x27}


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
    mem = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    gdi32.SelectObject(mem, bmp)
    ok = user32.PrintWindow(hwnd, mem, 3 if client_only else 2)   # PW_CLIENTONLY | PW_RENDERFULLCONTENT

    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD),
                    ("biBitCount", wt.WORD), ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                    ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                    ("biClrImportant", wt.DWORD)]

    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(bi)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = w, -h, 1, 32
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)
    Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB").save(path)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(mem)
    user32.ReleaseDC(hwnd, hdc)
    return bool(ok)


def game_window(pid):
    best = None
    for hwnd, _title, w, h in windows_of(pid):
        if w >= 200 and h >= 150 and (best is None or w * h > best[1]):
            best = (hwnd, w * h)
    return best[0] if best else None


def lparam(x, y):
    return (y << 16) | (x & 0xFFFF)


def main():
    a = sys.argv[1:]
    exe, prefix = a[0], a[1]
    wait, keep, steps = 20.0, False, "shot"
    i = 2
    while i < len(a):
        if a[i] == "--wait":
            wait = float(a[i + 1]); i += 2
        elif a[i] == "--do":
            steps = a[i + 1]; i += 2
        elif a[i] == "--keep":
            keep = True; i += 1
        else:
            i += 1
    proc = subprocess.Popen([exe], cwd=os.path.dirname(exe))
    print("started pid", proc.pid)
    time.sleep(wait)
    shot = 0
    try:
        for step in [s.strip() for s in steps.split(";") if s.strip()]:
            if proc.poll() is not None:
                print("process exited with", proc.returncode)
                break
            hwnd = game_window(proc.pid)
            if hwnd is None:
                print("no window yet for step", step)
                time.sleep(1.0)
                continue
            name, _, arg = step.partition(":")
            if name == "shot":
                out = f"{prefix}_{shot}.png"
                ok = capture(hwnd, out)
                print("saved", out, "" if ok else "(PrintWindow returned 0)")
                shot += 1
            elif name == "wait":
                time.sleep(float(arg))
            elif name == "key":
                vk = KEYS[arg.lower()]
                user32.PostMessageW(hwnd, WM_KEYDOWN, vk, 1)
                time.sleep(0.05)
                user32.PostMessageW(hwnd, WM_KEYUP, vk, 0xC0000001)
            elif name in ("click", "move"):
                x, y = (int(v) for v in arg.split(","))
                user32.PostMessageW(hwnd, WM_MOUSEMOVE, 0, lparam(x, y))
                if name == "click":
                    time.sleep(0.05)
                    user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lparam(x, y))
                    time.sleep(0.08)
                    user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lparam(x, y))
            else:
                print("unknown step", step)
    finally:
        if not keep and proc.poll() is None:
            proc.terminate()
            print("terminated", proc.pid)


if __name__ == "__main__":
    main()
