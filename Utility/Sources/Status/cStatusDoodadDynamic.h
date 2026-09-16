// cStatusDoodadDynamic.h: interface for the cStatusDoodadDynamic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_cStatusDoodadDynamic_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_)
#define AFX_cStatusDoodadDynamic_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNpcSettingDlg;
class cStatusDynamic;

#include "cStatusDoodadSimple.h"

class cStatusDoodadDynamic : public cStatusDoodadSimple  
{
public:
	cStatusDoodadDynamic();
	virtual ~cStatusDoodadDynamic();
	cStatusDynamic* m_pDynamic;

//virtual event	
	virtual BOOL Create();
	virtual void CreatePlane(CPoint point,iObj* &pPlane, int nFrame);
	virtual void SelectPlane(CPoint point,iObj* & pPlane);
//event
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDblClk(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
//dynamic
//file
	virtual BOOL GetOffsetOfPlane(iObj* p,cPoint& pt);
	virtual BOOL GetObjIniFile(iObj* p,CString& sini);

	void OnSelectedSetting(LPCTSTR szFile);
};

#endif // !defined(AFX_cStatusDoodadDynamic_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_)
