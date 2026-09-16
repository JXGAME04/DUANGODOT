#include "stdafx.h"

_declspec(dllexport) void Clipboard_Copy(const CString& str, HWND hWnd)
{
//	HGLOBAL HClip;
	HGLOBAL HProg;
//	char FAR * PClip;
	char FAR * PProg;
	if (!OpenClipboard(hWnd))
	{
		AfxMessageBox("Can not Open clipboard!");
		return ;
	}
	if (!::EmptyClipboard())
	{
		AfxMessageBox("Can not Empty clipboard!");
		return ;
	}
/*
	HClip=::GetClipboardData(CF_TEXT);
	if (HClip==NULL)
	{
		::CloseClipboard();
		AfxMessageBox("error obtaining text from Clipboard!");
		return ;
	}
*/
	HProg=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE ,str.GetLength()+1);
	if (HProg==NULL)
	{
		::CloseClipboard();
		AfxMessageBox("Out of memory!");
		return ;
	}

	PProg=(char *)::GlobalLock(HProg);
//	PProg=(char *)(HProg);
	::lstrcpy(PProg,(LPCTSTR)str);
	::GlobalUnlock(HProg);
	
	if (!::SetClipboardData(CF_TEXT,HProg))
	{
		AfxMessageBox("Copy to Clipboard wrong!");
	}
	::CloseClipboard();
}

_declspec(dllexport) void Clipboard_Paste(CString & str, HWND hWnd)
{
	HGLOBAL HClip;
	HGLOBAL HProg;
	char FAR * PClip;
	char FAR * PProg;
	if (!OpenClipboard(hWnd))
	{
		AfxMessageBox("Can not Open clipboard!");
		return ;
	}
	HClip=::GetClipboardData(CF_TEXT);
	if (HClip==NULL)
	{
		::CloseClipboard();
		AfxMessageBox("error obtaining text from Clipboard!");
		return ;
	}
	HProg=GlobalAlloc(GMEM_MOVEABLE,::GlobalSize(HClip));
	if (HProg==NULL)
	{
		::CloseClipboard();
		AfxMessageBox("Out of memory!");
		return ;
	}
	PClip=(char *)::GlobalLock(HClip);
	PProg=(char *)::GlobalLock(HProg);
	::lstrcpy(PProg,PClip);
	::GlobalUnlock(HClip);
	::CloseClipboard();
	CString strName(PProg);
	::GlobalUnlock(HProg);
	::GlobalFree(HProg);
	str = strName;
}


