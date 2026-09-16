//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KWin32App.h
// Date:	2000.08.08
// Code:	WangWei(Daphnis)
// Desc:	KWin32App Class
//---------------------------------------------------------------------------
#ifndef KWin32App_H
#define KWin32App_H
#include "KTimer.h"
//---------------------------------------------------------------------------
#define SWORD_ICON 101
#define ID_TRAYICON 9998
#define WMAPP_TRAY (WM_USER + 98)

//---------------------------------------------------------------------------
class KWin32App
{
protected:
	char	m_szClass[32];
	char	m_szTitle[32];
	BOOL	m_bShowMouse;
	BOOL	m_bActive;
	BOOL	m_bMultiGame;
	virtual	BOOL	InitClass(HINSTANCE hInstance);
	virtual	BOOL	InitWindow(HINSTANCE hInstance);
	virtual	BOOL	GameInit();
	virtual BOOL	GameLoop(bool bForceCoreLoop = false);
	virtual BOOL	GameExit();
	virtual int		HandleInput(UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void	AddTrayIcon(HWND hWnd, LPCSTR tip){};
	virtual void	AppSendInfoToTool(const void * const pData, const size_t &datalength){};
public:
	KWin32App();
	virtual BOOL	Init(HINSTANCE hInstance,char* AppName="JXWC");
	virtual void	Run();
	virtual	void	ShowMouse(BOOL bShow);
	virtual void	SetMultiGame(BOOL bMulti);
	virtual LRESULT	MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	void			SetMouseHoverTime(unsigned int uHoverTime);
public:
	int				m_nLastWidth;
	int				m_nLastHeight;
	UINT			g_uTaskbarCreated;
	BOOL			g_bTrayActive;
	char			g_szTip[64];
private:
	void			GenerateMsgHoverMsg();
	unsigned int	m_uMouseHoverTimeSetting;
	unsigned int	m_uMouseHoverStartTime;
	int				m_nLastMousePos;
	unsigned int	m_uLastMouseStatus;
	KTimer			m_gTimer;
};
//---------------------------------------------------------------------------
#endif
