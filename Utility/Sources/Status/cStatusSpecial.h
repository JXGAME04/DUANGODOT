// cStatusSpecial.h: interface for the cStatusSpecial class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSSPECIAL_H__B4597489_51C0_4FC7_BAA9_262C813A411C__INCLUDED_)
#define AFX_CSTATUSSPECIAL_H__B4597489_51C0_4FC7_BAA9_262C813A411C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusSpecial : public cStatus  
{
public:
	cStatusSpecial();
	virtual ~cStatusSpecial();
	void Clear();

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
	BOOL Create();
	BOOL m_bShowSpecial;
	BOOL OnSetFocus();
	BOOL OnLostFocus();
/////////////////////////////////////////////////////////////////////
//Special
/////////////////////////////////////////////////////////////////////
	iSpecial* m_pSpecialLeft;
	void CreateSpecial(CPoint point,iSpecial*& pLight);
	void SelectSpecial(CPoint point,iSpecial*& pLight);
/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////
//	void OnSelectedSetting(LPCTSTR szFile);
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
	BOOL MoveSelect(CPoint pt);
	BOOL OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnCameraMove(CPoint point);
};

#endif // !defined(AFX_CSTATUSSPECIAL_H__B4597489_51C0_4FC7_BAA9_262C813A411C__INCLUDED_)
