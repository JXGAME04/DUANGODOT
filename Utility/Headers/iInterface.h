#pragma once

#include "iPublic.h"
#include <string>

enum DialogEnum {dlg_connect,dlg_inputaccount,dlg_selectplayer,};

class iInterface
{
public:
	virtual DWORD AddRef() = 0;
	virtual DWORD Release() = 0;

	virtual BOOL Create(HWND hwnd) = 0;

	virtual HRESULT	CreateDlg(DialogEnum id,LPVOID ppointer) = 0;

	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;

	virtual BOOL Run() = 0;
	//on input...
};

class iDialog
{
public:
	virtual DWORD AddRef() = 0;
	virtual DWORD Release() = 0;

	virtual DWORD domodal() = 0;

//used only in the interface
	virtual void Create(iInterface* p) = 0;
};

class iConnectDlg: public iDialog
{
public:
	virtual BOOL SetServerIP(const std::string& str) = 0;
	virtual BOOL SetServerPort(WORD port) = 0;
	virtual BOOL GetServerIP(std::string& str) = 0;
	virtual BOOL GetServerPort(WORD& port) = 0;
};

class iInputAccountDlg: public iDialog
{
public:
	virtual BOOL SetAccount(const std::string& str) = 0;
	virtual BOOL SetPassword(const std::string& str) = 0;
	virtual BOOL GetAccount(std::string& str) = 0;
	virtual BOOL GetPassword(std::string& str) = 0;
};

class iSelectPlayerDlg: public iDialog
{
public:
	virtual BOOL AddPlayerName(const std::string& strName) = 0;
	virtual BOOL GetSelectName(std::string& strName) = 0;
};

enum InterfaceEnum{interface_normal};
_declspec(dllexport) iInterface* Interface_CreateInterface(InterfaceEnum e);
