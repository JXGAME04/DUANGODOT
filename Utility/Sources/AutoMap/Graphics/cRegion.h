// cRegion.h: interface for the cRegion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CREGION_H__33520A4B_D5AC_426F_A982_C8FEAD457420__INCLUDED_)
#define AFX_CREGION_H__33520A4B_D5AC_426F_A982_C8FEAD457420__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
#include "GraphicsPublic.h"

class iPack2D;
class cRegion  : public iRegion
{
public:
	cRegion(cGraphics* p);
	virtual ~cRegion();
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
	cGraphics* GetGraphics(){return m_pGraphics;}
	cCamera* GetCamera();
	cGround* GetGround();
	cScreen* GetScreen();
	LPDIRECT3DDEVICE8 GetDevice();

///////////////////////////////////////////////////////////////////////////
//get and set
///////////////////////////////////////////////////////////////////////////
	int m_x,m_y;
	void SetPos(int x,int y){m_x = x; m_y = y;}
	void GetPos(int& x,int& y){x = m_x; y = m_y;}
	bool IsPos(CPoint pt){return (m_x == pt.x && m_y == pt.y); }

	stParam m_param;
	HRESULT	GetParam(stParam& param) ;
	HRESULT	SetParam(stParam& param, bool bRunNow ) ;
	HRESULT	OnCommand(stParam& param);
	D3DFORMAT GetFormat();

///////////////////////////////////////////////////////////////////////////
//size
///////////////////////////////////////////////////////////////////////////

	void GetRegionRectTile(cRect& rcGround);
	void GetRegionRectGround(cRect& rcGround);
	BOOL IsPointInRegion(cPoint p);
	BOOL IsRectCrossRegion(cRect rc);
	BOOL CrossPlane(cPlane* p,eRegionDirection e);

/////////////////////////////////////////////////////////////////////////
//tile
/////////////////////////////////////////////////////////////////////////
	//+1因为光照的需要和mesh的需要
	cRect m_rcTile;
	stTile m_aTile[c_tWidthRegion+1][c_tHeightRegion+1];
	void	InitTile();

	stTile* GetTile(int x, int y);
	HRESULT Fill(LPCSTR szSprFile);
	HRESULT SetTile(int x, int y, const stTile& resTile);
	HRESULT SetTileVertex(int x, int y, stVertexGround v);
	HRESULT GetTile(int x, int y, stTile& resTile);
	
	HRESULT SetTileFileName(LPCSTR szFile);
	HRESULT GetTileFileName(CString&  strFile);
/////////////////////////////////////////////////////////////////////////
//load and save
/////////////////////////////////////////////////////////////////////////
	enum ModifyEnum
	{
		MODIFY_TERRAIN = 0x01, 
		MODIFY_HORBELOWVER = 0x02, 
		MODIFY_VER = 0x04,  
		MODIFY_HORABOVEVER = 0x08,
		MODIFY_LIGHT = 0x10, 
		MODIFY_OBSTACLE = 0x20,
		MODIFY_PLANE = 0x40,
		MODIFY_CLIENTNPC = 0x80,
		MODIFY_PROPERTY = 0x100,
		MODFIY_VERBELOWGROUND = 0x200,
	};
	void UpdateModify(iObj* p);
	long m_lModify;
	void ClearModify();
	BOOL IsModify(long mask);
	void ClearModify(long mask);
	void AddModify(long mask);

	void SetRegionFolder(LPCSTR szFolder);
	void GetRegionFolder(CString & strFolder);
	CString m_strName;
	
	bool IsValid(){return TRUE ;}
	HRESULT Load(LPCSTR szFolder);
	HRESULT LoadForMap(LPCSTR szFolder);
	HRESULT Save(LPCSTR szFolder);	
	
	void	PrepareTerrainTexture();

//element
//	HRESULT SaveLinkHor();
//	HRESULT LoadLinkHor(cPlaneHorList& plist);

	void	CreateFolder(CString& s);
	void	CreateFolder();
	HRESULT InitTerrain();
	HRESULT LoadTerrain();
	HRESULT SaveTerrain();
	HRESULT LoadTerrain(LPCSTR szFile);
	HRESULT SaveTerrain(LPCSTR szFile);
	HRESULT LoadProperty();
	HRESULT SaveProperty();
	HRESULT LoadPlane(LPCSTR file,iPack2D* p);
	HRESULT LoadPlane(cPlaneList& plist,LPCSTR file,iPack2D* p);
	HRESULT SavePlane(LPCSTR file, cPlaneList& rlist);
	HRESULT SavePlaneObstacle(LPCSTR szFile, cPlaneList& rlist);
	HRESULT LoadPlaneVer();
	HRESULT SavePlaneVer();
	HRESULT LoadPlaneHor();
	HRESULT SavePlaneHor();
//	HRESULT LoadPlaneHorAboveVer();
//	HRESULT SavePlaneHorAboveVer();
	HRESULT LoadLight();
	HRESULT SaveLight();
	HRESULT LoadObstacle();
	HRESULT SaveObstacle();

	//Load client only npc and save client only npc!
	//声效，飞鸟等等
	cPlaneList m_listClientNpc;
	HRESULT LoadClientOnlyNpc();
	HRESULT SaveClientOnlyNpc();
	BOOL AddClientOnlyNpc(cPlane* p);
	BOOL RemoveClientOnlyNpc(cPlane* p);

	//云彩等等
	cPlaneList m_listPlaneVerBelowGround;					//悬崖下的云彩
	HRESULT LoadPlaneVerBelowGround();
	HRESULT SavePlaneVerBelowGround();
	BOOL RemoveVerBelowGround(cPlane* p);
	BOOL AddVerBelowGround(cPlane* p);

	void	GetTerrainName(CString& str);

/////////////////////////////////////////////////////////////////////////
//mesh
/////////////////////////////////////////////////////////////////////////
	cMesh* m_pMesh;
	void ReCreateMesh();
	void CreateMesh();
	void SetMeshAnimation();
/////////////////////////////////////////////////////////////////////////
//ground texture
/////////////////////////////////////////////////////////////////////////
	iTexture* m_pTexture;
	BOOL m_bTextureDirty;
	BOOL IsTextureDirty(){return m_bTextureDirty;}
	cRect rcDirty;												//用来更新region
	void CreateTexture();
	void ChangeSubTexture(int x, int y, bool bUpdate = false);
	void SetResource(stResourceA& pos);

//	void UpdateTextureAfterDistribute();
	void CreateTerrainTexture();
	void UpdateGroundTexture();
	void UpdateTerrainTexture();
	void UpdateHorTexture();
//	BOOL m_bUpdateHorLink[3][3];
	BOOL m_bUpdateTexture;
	void RepaintTerrain();

	void FindTerrainTexture();
	void ClearTerrainTexture(){RELEASE(m_pTexture);};
	HRESULT BeginPainting();
	HRESULT EndPainting();
	
	void RepaintHor(cPlaneHor* pPlane);
	void RepaintRectTile(CRect& rc);
	void DrawHor(cPlaneHor* pPlane,const cRect& rcTile);

/////////////////////////////////////////////////////////////////////////
//paint loop
/////////////////////////////////////////////////////////////////////////
	bool m_bInScreen;
	bool IsInScreen(){return m_bInScreen;};
	bool CheckDisplay(const cRect& rc);
	cColor GetVertexColor(cColor cr);
	bool BeginLighting(cColor cr);
	bool EndLighting();

	BOOL m_bPaintTerrain;
	void SetPaintTerrain(BOOL b){m_bPaintTerrain= b;}
	BOOL IsPaintTerrain(){return m_bPaintTerrain;}
	BOOL m_bPaintPlane;
	void SetPaintPlane(BOOL b){m_bPaintPlane = b;}
	BOOL IsPaintPlane(){return m_bPaintPlane;}

	HRESULT PaintTerrain();
	HRESULT PaintBeforeTerrain();
	HRESULT PaintHorBelow();
	HRESULT PaintHorAbove();
	HRESULT PaintVer();
	HRESULT PaintVer(int yTile);
	HRESULT DrawTrace();
	HRESULT PaintOthers();
	HRESULT PaintObstacle();
	HRESULT PaintText();

/////////////////////////////////////////////////////////////////////////
//device
/////////////////////////////////////////////////////////////////////////
	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
	virtual HRESULT FinalCleanup();

	friend class cGround;

/////////////////////////////////////////////////////////////////////////
//elmenent of the region
/////////////////////////////////////////////////////////////////////////
//plane
	cPlaneList m_listPlane;									//使用这个list用于存盘。
	int GetPlaneNum(){return m_listPlane.size();}

//plane 可能会跨越region!
	cPlaneHorList m_listLinkPlaneHor;								//这个list和m_listPlane联合起来作为排序。
	BOOL AddLinkHorFromRegion(cRegion* pRegion);	
	BOOL RemoveLinkHor();

	BOOL AddLinkHor(cPlaneHor* p);	
	BOOL RemoveLinkHor(cPlaneHor* p); 
	BOOL AddcPlane(cPlane* p);
	BOOL FindcPlane(cPlane* p);
	BOOL RemovecPlane(cPlane* p);	

//hor
	cPlaneHorList* GetPlaneHorList(cPlaneHor* p);
	cPlaneHorList m_listPlaneHorBelowVer;
	cPlaneHorList m_listPlaneHorGround;
	BOOL AddHor(cPlaneHor* p, bool bRepaint );
	BOOL RemoveHor(cPlaneHor* p, bool bRepaint);

	BOOL GetLinkOffset(cPoint& ptOffset, cPlane* p, int eLink,cPoint ptPlane, float& fMinDis );
//light
	cLightList m_listLight;
	int GetLightNum(){return m_listLight.size();}
	BOOL AddcLight(cLight* p);
	BOOL RemovecLight(cLight* p);
//obstacle
	cObstacleList m_listObstacle;
	int GetObstacleNum(){return m_listObstacle.size();}
	BOOL AddcObstacle(cObstacle* p);
	BOOL RemovecObstacle(cObstacle* p);
	ObstacleEnum CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise);
	BOOL BlockObstacle(cPoint pt,ObstacleEnum e);
//filter
	HRESULT FilterByDisplay(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut);
	HRESULT FilterByOffset(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut, POINT ptOffset);
	HRESULT FilterByArea(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut);
	HRESULT FilterByY(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut);
	HRESULT FilterByZ(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut);
//obj
	HRESULT GetiObj(int nID, iObjList& iList);
	HRESULT	GetiLight(stSelect& st,cLightList& tlist) ;
	HRESULT	GetiObstacle(stSelect& st,cObstacleList& tlist) ;
	HRESULT	GetiPlane(stSelect& st,cPlaneList& tlist) ;
//	HRESULT	GetiPlane(stSelect& st,cPlaneList& tlist) ;

//for mapconvert
	int		MC_GetLinkInfo(LPCSTR szFolder);
	int		MC_Load(LPCSTR szFolder);
};

#endif // !defined(AFX_CREGION_H__33520A4B_D5AC_426F_A982_C8FEAD457420__INCLUDED_)
