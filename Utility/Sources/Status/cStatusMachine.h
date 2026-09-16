// cStatusMachine.h: interface for the cStatusMachine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSMACHINE_H__90F7E539_54B1_4BCE_A432_19F11D40120A__INCLUDED_)
#define AFX_CSTATUSMACHINE_H__90F7E539_54B1_4BCE_A432_19F11D40120A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"


typedef std::vector<cStatus*> cStatusVector;
typedef cStatusVector::iterator itcStatus;

class cStatusMachine : public cStatus
{
public:
	cStatusMachine();
	virtual ~cStatusMachine();

	virtual	iCommandMachine* CreateCommandMachine(iGraphics* p);
	virtual	BOOL Create(iGraphics* p,CWnd* pWnd,OutputMsgFunc func);
	cStatus* CreateStatus(eStatusType e);
	cStatus* GetcStatus(){return m_pStatus;};
	cStatus* m_pStatus;
	BOOL	ChangeToStatus(eStatusType nType);

/////////////////////////////////////////////////////////////////////////////
//input
	virtual BOOL OnNewDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnClose();

	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnLButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL OnTimer(UINT nIDEvent);
	virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL Copy();
	virtual BOOL Cut();
	virtual BOOL Paste();
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//window operator
	BOOL OnRightSelectedFile(LPCTSTR lpszPathName, int nFrame);
	BOOL OnLeftSelectedFile(LPCTSTR lpszPathName, int nFrame);
	BOOL OnSelectedType(LPCTSTR lpszPathName);
};

#endif // !defined(AFX_CSTATUSMACHINE_H__90F7E539_54B1_4BCE_A432_19F11D40120A__INCLUDED_)
