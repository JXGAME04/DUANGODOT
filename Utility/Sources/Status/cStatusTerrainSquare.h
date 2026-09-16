// cStatusTerrainSquare.h: interface for the cStatusTerrainSquare class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSTERRAINSQUARE_H__CE41E74C_6306_4D95_92FB_1BA1EFC2A0B8__INCLUDED_)
#define AFX_CSTATUSTERRAINSQUARE_H__CE41E74C_6306_4D95_92FB_1BA1EFC2A0B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class iAutoGround;

class cStatusTerrainSquare : public cStatus
{
public:
	cStatusTerrainSquare();
	virtual ~cStatusTerrainSquare();

//////////////////////////////////////////////////////////////////////
//create
	BOOL Create();
	BOOL OnSetFocus();

	bool m_bMouseStart;
	cPoint m_ptMouseStart;

//////////////////////////////////////////////////////////////////////
//command
	BOOL m_bAutoFillGround;
	CRect m_rcFillGround;
	void DeleteTile(CPoint point);
	void ChangeTile(LPCSTR szSpr,CPoint point,BOOL bFill,BOOL bIgnore);
	void FillTile(CRect rect);
	void ModifyTile(CPoint pt, cPoint v);
	void FillDiamondTile(CRect r);
	void ChangeSelectTile(CPoint point, BOOL bFill, BOOL bIgnore);
	
//////////////////////////////////////////////////////////////////////
//input
	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	void GetSelectByPoint(CPoint ptScreen, cPoint& ptTile);
	void MoveSelect(CPoint point);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnCameraMove(CPoint point);

//////////////////////////////////////////////////////////////////////
//virtual call back
	void OnSelectedSetting(LPCTSTR szFile);
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);

//////////////////////////////////////////////////////////////////////
//custom data
	enum TypeEnum{TYPE_MIDDLE,TYPE_LARGE,TYPE_NUM};
	TypeEnum m_eType;
	iAutoGround* m_aGround[TYPE_NUM];
	int m_aSize[TYPE_NUM];
	float m_aTile[TYPE_NUM];
	iAutoGround* GetAutoGround(){return m_aGround[m_eType];};
	int GetSize(){return m_aSize[m_eType];};
	float GetTile(){return m_aTile[m_eType];};
	void ChangeType(TypeEnum e);
	void SetSize(int size);

//////////////////////////////////////////////////////////////////////
//document operator
};

#endif // !defined(AFX_CSTATUSTERRAINSQUARE_H__CE41E74C_6306_4D95_92FB_1BA1EFC2A0B8__INCLUDED_)
