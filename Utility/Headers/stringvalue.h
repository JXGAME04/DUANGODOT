#pragma once

#include "iCommon.h"

_declspec(dllexport) BOOL StringToValue(const CString& s,bool& b);
_declspec(dllexport) void ValueToString(const bool& b, CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,float& n);
_declspec(dllexport) void ValueToString(const float& n, CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,int& n);
_declspec(dllexport) void ValueToString(const int& n, CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,long& n);
_declspec(dllexport) void ValueToString(const long& n, CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,LPSTR sz);
_declspec(dllexport) void ValueToString(LPCSTR sz, CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,CRect& rc);
_declspec(dllexport) void ValueToString(const CRect& rc,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,cRect& rc);
_declspec(dllexport) void ValueToString(const cRect& rc,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,CString& str);
_declspec(dllexport) void ValueToString(const CString& str,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s,cColor& cr);
_declspec(dllexport) void ValueToString(const cColor& cr,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s, CPoint& pt);
_declspec(dllexport) void ValueToString(const CPoint& pt,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s, POINT& pt);
_declspec(dllexport) void ValueToString(const POINT& pt,CString& s);
_declspec(dllexport) BOOL StringToValue(const CString& s, cPoint& pt);
_declspec(dllexport) void ValueToString(const cPoint& pt,CString& s);
_declspec(dllexport) void PointArrayToString(const cPoint aPoint[], int nPoint, CString& sValue);
_declspec(dllexport) void PointArrayToString(const CPoint aPoint[], int nPoint, CString& sValue);
_declspec(dllexport) void StringToPointArray(const CString& sValue,cPoint aPoint[]);
