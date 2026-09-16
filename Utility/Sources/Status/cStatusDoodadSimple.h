// cStatusDoodadSimple.h: interface for the cStatusDoodadSimple class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSDOODADSIMPLE_H__D300A50B_1503_4CBC_87F2_C49219DD26F8__INCLUDED_)
#define AFX_CSTATUSDOODADSIMPLE_H__D300A50B_1503_4CBC_87F2_C49219DD26F8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusDoodadSimple : public cStatus  
{
public:
	cStatusDoodadSimple();
	virtual ~cStatusDoodadSimple();
	virtual void Clear();

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
	BOOL m_bLinkVer;
	BOOL m_bLinkHor;
	BOOL m_bLinkTile;
	virtual BOOL Create();
	BOOL OnSetFocus();

	void SetPlaneLeft(iObj* p);
	void SetPlaneRight(iObj* p);
	void RemovePlaneRight();
	void RemovePlaneLeft();
	virtual BOOL GetOffsetOfPlane(iObj* p,cPoint& pt){return FALSE;};
/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
	BOOL MoveSelect(CPoint pt);
	BOOL OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnCameraMove(CPoint point);

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////
	void OnSelectedSetting(LPCTSTR szFile);
	
/////////////////////////////////////////////////////////////////////
//Plane
/////////////////////////////////////////////////////////////////////
	iObj* m_pPlaneLeft;
	iObj* m_pPlaneRight; //右健点击获得的plane
	
	void AddPlaneSortOffY(iObj* pPlane,float fOff);
	void AddPlaneLayer(iObj* pPlane,int layer);
//	void ObjProperty(iObj* pPlane);

	void RefreshSpr();
	virtual void CreatePlane(CPoint point,iObj* &pPlane, int nFrame);
	virtual void SelectPlane(CPoint point,iObj* & pPlane);

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
//	stSelect m_stSelect;
	void OnSelectSetting();

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
	bool m_bTrace;
//	void SetPlaneSelect(iObj* pPlane,bool bTrace);
//	void SetPlaneTrace(iObj* pPlane,bool bTrace);
	void TogglePlaneTrace();

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
	BOOL OnLButtonDblClk(UINT nFlags, CPoint point);

	CString m_strCopyDataFile;
	virtual BOOL Copy();
	virtual	BOOL OnSelectedFile(LPCTSTR lpszPathName, int frame);
};

//int StringToPlaneTable(int table, const CString& strValue)

#endif // !defined(AFX_CSTATUSDOODADSIMPLE_H__D300A50B_1503_4CBC_87F2_C49219DD26F8__INCLUDED_)
