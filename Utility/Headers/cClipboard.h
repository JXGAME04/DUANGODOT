#pragma once

_declspec(dllexport) void Clipboard_Copy(const CString& str, HWND hWnd);
_declspec(dllexport) void Clipboard_Paste(CString& str, HWND hWnd);
