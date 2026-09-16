// cMap.h: interface for the cMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMAP_H__5B3C972A_BAA6_4C00_B77D_262270066048__INCLUDED_)
#define AFX_CMAP_H__5B3C972A_BAA6_4C00_B77D_262270066048__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//小地图
//size 固定大小 800/4 * 600/4
#include "cScreenObj.h"
class cGround;
class cMapRegion;

//cMap is a element of the screen
class cMap  : public cScreenObj
{
public:
	cMap(cGraphics* p);
	virtual ~cMap();
	void Init(bool bCreateBuffer);

//graphics
	cGround*					GetGround();	
	HRESULT						DrawDynamic();
	void						DrawSpecial();
	
//param	
	stMapParam*					GetMapParam(){return (stMapParam*) GetObjParam();};
	HRESULT						setParam(stObjParam& param) ;
	HRESULT						OnCommand(long Command,stObjParam& param);

//region
	enum{MAX_MAP_REGION_WIDTH = 16, MAX_MAP_REGION_HEIGHT = 16,};
	cMapRegion*					m_aRegion[MAX_MAP_REGION_WIDTH][MAX_MAP_REGION_HEIGHT];
	void						ClearRegion();

//contain
	BOOL						IsShow();
	HRESULT						Draw();
	enum eTexKind{TEX_HOUSE,TEX_WALL,TEX_HILL,TEX_FOREST,TEX_ROAD,TEX_NUM};
	HRESULT						SetCommonTexture(eTexKind kind, int frame);
	iTexture*					FindCommonTexture(eTexKind kind);
	void						DrawPoint(cPoint p1,cColor cr);
	void						DrawLine(cPoint p1,cPoint p2,cColor cr);
//	void						DrawLine(stVertexScreen* pV, int n);
	void						VertexGroundToScreen(stVertexGround* pV,stVertexScreen v[],int n);
	void						DrawLine(stVertexGround* pV, int n);
	void						DrawFormatLine(cPoint p1,cPoint p2,cColor cr);
	void						DrawFan(stVertexGround* pV, int nVertex);
	void						Draw(stVertexGround* pV, int nVertex);
	CRect						GetRect();
	BOOL						IsPointInMap(cPoint pt);
	BOOL						IsPointInMap(CPoint pt);

//load
	CString						m_strFile;
	HRESULT						OnFileOpen(LPCSTR szNewFile);
	HRESULT						OnRegionUpdate(int xRegion, int yRegion);//当前的region变成xregion,yregion!

//坐标变换
	void						ScreenToGround(CPoint ptScreen ,cPoint& ptGround);

	void						GroundToScreen(float fx,float fy,long& nx, long& ny);
	void						GroundToScreen(cPoint ptGround, CPoint& ptScreen);
	void						GroundToScreen(const stVertexGround& vG, stVertexScreen& vS);

	void						RegionToScreen(float fx,float fy,long& nx, long& ny);
	void						RegionToScreen(cPoint ptGround, CPoint& ptScreen);
//contain
	virtual BOOL				Contain(POINT pt){return FALSE;}
	virtual BOOL				Contain(cPoint pt){return FALSE;}
	//包含在一个屏幕矩形中
	BOOL						ContainedScreen(RECT rc){return FALSE;}
	//包含在一个地图矩形中
	virtual BOOL				ContainedGround(cRect rc){return FALSE;}
	//包含在一个菱形中
	virtual BOOL				ContainedDiamond(cRect rc){return FALSE;}
};

#endif // !defined(AFX_CMAP_H__5B3C972A_BAA6_4C00_B77D_262270066048__INCLUDED_)
