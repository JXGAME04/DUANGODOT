// cStatusGroup.h: interface for the cStatusGroup class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_cStatusGroup_H__923C00E1_A929_4C20_A950_79ADA45460FC__INCLUDED_)
#define AFX_cStatusGroup_H__923C00E1_A929_4C20_A950_79ADA45460FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"
class cDoodad;

class cStatusGroup : public cStatus  
{
public:
	cStatusGroup();
	virtual ~cStatusGroup();
	void Clear();

	virtual LPCSTR GetTypeString(){return "group";};
	virtual LPCSTR GetGroupFolder(){return "游戏资源\\组件";};
/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
	BOOL m_bShadow;
	CString m_strDoodadCapture;
	void GetDoodadFileString(CString& sFile);
	void CaptureDoodad();
	void SaveDoodad(cDoodad* p,LPCSTR szFile);
	BOOL OnSetFocus();
	BOOL Create();

	BOOL SaveDoodadOfFirstDoodad();
	BOOL SaveDoodadOfFirstPlane();	
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
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
	
/////////////////////////////////////////////////////////////////////
//Doodad
/////////////////////////////////////////////////////////////////////
	cDoodad* m_pDoodadLeft;
	cDoodad* m_pDoodadRight; //右健点击获得的Doodad
	
	void OffsetDoodad(cDoodad* pDoodad,cPoint ptOffset);
	void OffsetDoodadCenter(cDoodad* pDoodad,cPoint ptOffset);

	void CreateDoodad(CPoint point,cDoodad* &pDoodad);
	void RemoveDoodad(cDoodad* pDoodad);
	virtual BOOL SelectDoodad(CPoint point,cDoodad* & pDoodad);
	virtual BOOL SelectDoodad(cPoint point1, cPoint point2, cDoodad* & pDoodad);

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
	bool m_bTrace;
	void SetDoodadMoving(cDoodad* pDoodad, bool bMoving);
	void SetDoodadSelect(cDoodad* pDoodad,bool bTrace);
	void SetDoodadTrace(cDoodad* pDoodad,bool bTrace);
	void ToggleDoodadTrace();

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
	bool m_bMouseStart;
	cPoint m_ptMouseStart;
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
	BOOL OnLButtonDblClk(UINT nFlags, CPoint point);
	BOOL Copy();

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
//	stSelect m_stSelect;
	void OnSelectSetting();

	virtual void AfterCreatePlane(iObj* pPlane,const CString& m_strSelectFile,const CString& m_strCopyDataFile){};
};

#endif // !defined(AFX_cStatusGroup_H__923C00E1_A929_4C20_A950_79ADA45460FC__INCLUDED_)
