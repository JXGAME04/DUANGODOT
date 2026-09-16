// GMDebug.h : main header file for the GMDEBUG application
//

#if !defined(AFX_GMDEBUG_H__1B2325BB_0767_46BC_9A44_054D7BB92AC2__INCLUDED_)
#define AFX_GMDEBUG_H__1B2325BB_0767_46BC_9A44_054D7BB92AC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "../jxgm2/Headers/jxgm2_i.h"

#include "AfxMT.h"
#include "AfxTempl.h"

/////////////////////////////////////////////////////////////////////////////
// CGMDebugApp:
// See GMDebug.cpp for the implementation of this class
//

class CGMDebugApp;

class CSystemEvent : public ISystemEvent
{
public:
	CGMDebugApp* m_pParent;

	virtual void STDMETHODCALLTYPE GWLogin(char* szGWName, DWORD nIP);
	virtual void STDMETHODCALLTYPE GWLogout(char* szGWName, DWORD nIP);
	virtual void STDMETHODCALLTYPE GWRelayID(char* szGWName, DWORD nRelayID);
	virtual void STDMETHODCALLTYPE GWSubLogout(DWORD nGWIP, DWORD nSubIP);
	virtual void STDMETHODCALLTYPE Ping(DWORD nTick);
	virtual void STDMETHODCALLTYPE ConnectClosed();
	virtual void STDMETHODCALLTYPE PlayerMessage(char* szAccountFrom, char* szRole, char* szMessage, int nMsgLen, DWORD dwGWIP);
	virtual void STDMETHODCALLTYPE LosePlayer(char* szAccount, DWORD dwGWIP);

	virtual void STDMETHODCALLTYPE AccountUserReturn(DWORD Operate, char* szAccount, int nReturn);
	virtual void STDMETHODCALLTYPE OnlinePlayerCount(DWORD Operate, char* szGWName, DWORD nCount);
	virtual void STDMETHODCALLTYPE RegisterUserCount(DWORD Operate, DWORD nCount);
};

class CGMDebugDoc;

class CGMDebugApp : public CWinApp
{
public:
	CSystemEvent m_Event;
	void NewRelay(char* szGWName, DWORD nIP);
	void RemoveRelay(char* szGWName, DWORD nIP);
	void RelayID(char* szGWName, DWORD nIP);
	void CloseDocs();
	void NewMessage(const char* szAccountFrom, const char* szRole, const char* szMessage, int nMsgLen, DWORD dwGWIP);
	void ErrorMessage(const char* szError);

	void SendPing();
	void ExecuteAction(const char* szAccount, const char* szScript, DWORD nReturn);
	void ExecuteAction(const char* szRole, DWORD nRelayID, const char* szScript, DWORD nReturn);
	void ExecuteAction(DWORD nRelayID, const char* szScript, DWORD nReturn, DWORD nSendIP);

	void FreezeAccount(const char* szAccount);
	void UnlockPlayerAccount(const char* szAccount);
	void SendOnlinePlayerCount(const char* szGWName);
	void SendRegisterUserCount();

	void OnProcess();

	void GetDocs(CPtrList& List);
	void ConnectAndLogin();

	CGMDebugApp();
	~CGMDebugApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMDebugApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CGMDebugApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnSelfinfo();
	afx_msg void OnAutorun();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	HMODULE				m_hDllJXGM;
	bool m_bAuto;

	ISystemConnect2* m_pISystemConnect;

	BOOL m_bCloseDocs;

	CCriticalSection m_RelayDocCS;
	struct RELAYDOC
	{
		CString strDocName;
		DWORD docIP;
		int nCreate;
	};

	void CloseConnect();
	BOOL m_bConnect;
	DWORD m_dwSystemIP;
	short m_nSystemPort;
	CString m_strAccount;
	CString m_strPassword;

	CList<RELAYDOC, RELAYDOC> m_Docs;

	CCriticalSection m_RelayDataCS;
	struct RELAYDATA
	{
		CString strAccount;
		CString strRole;
		DWORD docIP;
		CString strMessage;
	};

	CList<RELAYDATA, RELAYDATA> m_Datas;
	CStringList m_Errors;

	CGMDebugDoc* FindDoc(DWORD nIP);

	DWORD m_nOperate;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMDEBUG_H__1B2325BB_0767_46BC_9A44_054D7BB92AC2__INCLUDED_)
