// cMapRegion.h: interface for the cMapRegion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMAPREGION_H__BF03ED1C_FB19_43EC_95D7_B6395D44E05F__INCLUDED_)
#define AFX_CMAPREGION_H__BF03ED1C_FB19_43EC_95D7_B6395D44E05F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cMap;
class cScreen;
class cGround;
class cRegion;

#include "iColor.h"

class cMapRegion  
{
public:
	cMapRegion(cGraphics* p);
	virtual ~cMapRegion();
/////////////////////////////////////////////////////////////////////////
//com
/////////////////////////////////////////////////////////////////////////
	ULONG m_ulCount;
	ULONG AddRef();
	ULONG Release();
/////////////////////////////////////////////////////////////////////////
//graphics;
/////////////////////////////////////////////////////////////////////////
	cGraphics* m_pGraphics;
	void SetGraphics(cGraphics* p){m_pGraphics = p;}
	cGraphics* GetGraphics(){return m_pGraphics;}
	cScreen* GetScreen();
	cMap* GetMap();
	cGround* GetGround();
	cRegion* GetRegion(int x,int y);
///////////////////////////////////////////////////////////////////////////
//get and set
///////////////////////////////////////////////////////////////////////////
	int m_x,m_y;
	void SetPos(int x,int y){m_x = x; m_y = y;}
	void GetPos(int& x,int& y){x = m_x; y = m_y;}
	bool IsPos(CPoint pt){return (m_x == pt.x && m_y == pt.y); }

	void Clear();
	bool m_bUseRegion;
	cRegion* m_pRegion;
//paint
	enum {LOAD_OBSTACLE,};
	HRESULT Paint();
	//绘制河流，山脉，房屋等多边形障碍.
	void DrawObstacle(cObstacle* p);
	void DrawObstacle(cObstacleList& olist);
	void DrawPlaneObstacle(cPlaneList& plist);
	void DrawDynamic(cPlaneList& glist);

	void DrawSeam(cPlaneVerList& plist);
	void DrawTree(cPlaneVerList& plist);
	void DrawGround(cPlaneHorList& plist);


	void DrawSeam(cPoint p0,cPoint p1);

	CString m_strFile;
	HRESULT Load(LPCSTR szFolder,long lDisplay);
};

#endif // !defined(AFX_CMAPREGION_H__BF03ED1C_FB19_43EC_95D7_B6395D44E05F__INCLUDED_)
