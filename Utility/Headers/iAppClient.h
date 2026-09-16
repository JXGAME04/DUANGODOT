#pragma once

class iClient;
class iGraphics;

class iInput;
class iMenu;

class iServerLink;
struct stMsg;
//client use the same thread like iGraphics

class iAppClient
{
public:
	//create it after client connected and entergameworld
	virtual BOOL Create(iClient* pClient,iGraphics* pGraphics) = 0;

	virtual void PushMessage(stMsg* p,DWORD size,void* pPlayer,iServerLink* pLink) = 0;

	virtual BOOL Begin() = 0;
	virtual BOOL Run() = 0;
	virtual BOOL End() = 0;

	//haddle input message 
	//virtual BOOL HandleInput(iInput* p) = 0;
	//haddle menu message
	//virtual BOOL HandleMenu(iMenu* p) = 0;

	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
};

_declspec(dllexport) iAppClient* AppClient_GetAppClient();