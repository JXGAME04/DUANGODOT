// cStatusTerrainDiamond.h: interface for the cStatusTerrainDiamond class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSTERRAINDIAMOND_H__BCBFE8A2_43EE_4DA1_A132_36D9D9B524F2__INCLUDED_)
#define AFX_CSTATUSTERRAINDIAMOND_H__BCBFE8A2_43EE_4DA1_A132_36D9D9B524F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusTerrainDiamond  : public cStatus
{
public:
	cStatusTerrainDiamond();
	virtual ~cStatusTerrainDiamond();

	//create
	BOOL Create();
	BOOL OnSetFocus();

//////////////////////////////////////////////////////////////////////
//command
	CRect m_rcFillGround;
	int m_nRadius;
	HRESULT FillTile(CRect rect, bool bCreate);
	HRESULT CreateDiamond(CPoint point, bool bCreate);
	HRESULT CreateDiamond(cPoint ptTile, bool bCreate);

//////////////////////////////////////////////////////////////////////
//select 
	void GetSelectByPoint(CPoint point, cPoint& ptTile);
	void MoveSelect(CPoint point);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnCameraMove(CPoint point);

	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
	void OnSelectedSetting(LPCTSTR szFile);

//////////////////////////////////////////////////////////////////////
//set
	bool m_bMouseStart;
	cPoint m_ptMouseStart;
	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point);
};

#endif // !defined(AFX_CSTATUSTERRAINDIAMOND_H__BCBFE8A2_43EE_4DA1_A132_36D9D9B524F2__INCLUDED_)
