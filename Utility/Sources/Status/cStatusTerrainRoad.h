// cStatusTerrainRoad.h: interface for the cStatusTerrainRoad class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSTERRAINROAD_H__41A989A2_C275_4685_A993_57B48D6361A8__INCLUDED_)
#define AFX_CSTATUSTERRAINROAD_H__41A989A2_C275_4685_A993_57B48D6361A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusTerrainRoad  : public cStatus
{
public:
	cStatusTerrainRoad();
	virtual ~cStatusTerrainRoad();

	//
	enum TypeEnum{TYPE_SMALL,TYPE_MIDDLE,TYPE_LARGE,TYPE_NUM};
	TypeEnum m_eType;
	int m_aSize[TYPE_NUM];
	int GetSize(){return m_aSize[m_eType];}
	void ChangeType(TypeEnum e);
//	void SetSize(int nSize){m_nSize = nSize;}
	//create
	BOOL Create();
	BOOL OnSetFocus();

	//select 
	HRESULT CreateDiamond(CPoint point);
	HRESULT CreateRoad(CPoint point, int nStyle);
	HRESULT CreateRoadObj(CPoint point, int nStyle);
	int GetRoadStyle(cPoint ptTile, int nStyleAdd);
	
	void GetSelectByPoint(CPoint point, cPoint& ptTile);
//	void GetSelectDiamondByPoint(CPoint point, cPoint& ptTile);

	void MoveSelect(CPoint point);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnCameraMove(CPoint point);
//	virtual bool Copy();

	//plane
	iObj* GetiPlane(CPoint point);

	//set
	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point);
//call back
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
};

#endif // !defined(AFX_CSTATUSTERRAINROAD_H__41A989A2_C275_4685_A993_57B48D6361A8__INCLUDED_)
