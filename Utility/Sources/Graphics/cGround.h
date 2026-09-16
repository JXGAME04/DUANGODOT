// cGround.h: interface for the cGround class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGROUND_H__45B2974B_97A6_4AB2_87B2_9CED55E3B184__INCLUDED_)
#define AFX_CGROUND_H__45B2974B_97A6_4AB2_87B2_9CED55E3B184__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
#include "GraphicsPublic.h"
#include "cBtree.h"

typedef std::list<iSpecial*> SpecialList;
typedef SpecialList::iterator itSpecial;

class cColorMap;
class cGroundObj;
typedef std::list<cGroundObj*> cGroundObjList;
typedef cGroundObjList::iterator itcGroundObj;

//Ground是唯一的,就是整个的游戏地图
//这个游戏地图,当前可能只显示一些cRegion,
//cRegion是16*16的地图单位(512*512的屏幕单位)
class cScreen;
class cMap;
class cLightMap;
class cMapSmall;

struct stLine;
typedef std::list<stLine*> LineList;
typedef LineList::iterator itLine;
typedef std::vector<stLine*> LineArray;
typedef LineArray::iterator itALine;

class iPack2D;

enum PackFileEnum{
	PACK_NPC,PACK_BOX,PACK_TRAP,
	PACK_CLIENTONLYNPC,PACK_CLIENTONLYBOX,PACK_CLIENTONLYTRAP,
	PACK_PLANEVER,PACK_PLANEVERBELOWGROUND,PACK_PLANEHOR,
	PACK_LIGHT,PACK_OBSTACLE,
	PACK_GROUND,PACK_REGION,
	PACK_NUM,
};

class iZipRead;
class cMapObstacle;

class cGround : public iGround
{
public:
	cGround(cGraphics* p);
	virtual ~cGround();

//////////////////////////////////////////////////////////////////////
//game
//////////////////////////////////////////////////////////////////////
	BOOL	IsPlaying(){return m_param.bPlayering || m_pZip!=NULL;}
	BOOL	IsEditing(){return !IsPlaying();}
	BOOL	Game_Load(LPCSTR szMap,cPoint ptCameraTo) ;

//////////////////////////////////////////////////////////////////////
//com
//////////////////////////////////////////////////////////////////////
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;

//////////////////////////////////////////////////////////////////////
//Graphics
//////////////////////////////////////////////////////////////////////
	cScreen* GetcScreen();
	
	cMap* GetcMap();
	cMapSmall* GetcMapSmall();

	cCamera* GetcCamera();
	cSelect* m_pSelect;
	iObj* GetiSelect();
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}	
	cMapObstacle* m_pMapObstacle;
	cMapObstacle* GetMapObstacle(){return m_pMapObstacle;}

//////////////////////////////////////////////////////////////////////
//thread
//////////////////////////////////////////////////////////////////////
//begin and end
	HRESULT BeginThread();
	HRESULT EndThread();
	virtual HRESULT Begin();
	virtual HRESULT End();
//loop
	virtual HRESULT Run();
//thread
	cThread* m_pThread;
	static UINT Thread( LPVOID pParam );

	BOOL UpdateNearestRegionTexture();
	BOOL LoadNearestRegion();

	UINT ThreadRun();
	//BOOL UpdateRegionTerrain();
	cRegion* ThreadAddcRegion(int x,int y, LPCSTR szFolder);
//哪些region已经读好了
	CRITICAL_SECTION m_csUpdateRegionPosOrSize;
	BOOL GetNearestUnloadRegion(int& x, int& y, BOOL bFindNotUpdateTexture);		//寻找最近的一个region，放入队列中
	void SetLoadedRegion(cRegion* pRegion);
//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
	iGround::stParam m_param;
	HRESULT GetParam(stParam& param);
	HRESULT SetParam(stParam& param, bool bRunNow = false) ;
	HRESULT setParam(const stParam& param) ;

	LPCSTR GetDynamic(){return m_param.szDynamic;}
	BOOL IsUpdateRegionTexture(){return m_param.bUpdateRegionTexture;}
	HRESULT UpdateRegionTexture();
//////////////////////////////////////////////////////////////////////
//trace
//////////////////////////////////////////////////////////////////////
	void Trace();

//////////////////////////////////////////////////////////////////////
//new load and save
//////////////////////////////////////////////////////////////////////
//load 
	long	GetVerson(){return m_param.lVerson;}

	iZipRead* m_pZip;	
	iZipRead* GetZip(){return m_pZip;}
	iPack2D* m_aPack[PACK_NUM];
	iPack2D* GetPack(PackFileEnum e){return m_aPack[e];}
	
	HRESULT LoadWor();
	HRESULT Load(LPCSTR strMapFile);

	HRESULT AskSave();
	HRESULT Save(LPCSTR strMapFile);
	HRESULT New(LPCSTR szFile);
	HRESULT Init(LPCSTR szGround);
	BOOL	IsModify();
	HRESULT SaveStatic(CString& strFolder);
//dynamic
	BOOL	FindDynamicNick(LPCSTR szNick);
	HRESULT ClearDynamic();
	BOOL	IsDisplayEvent(LPCSTR szEvent);
	LPCSTR GetEvent(){return m_param.szDynamic;}
//utitily
	void GetGroundFolder(CString& str);
//	void GetRegionFolder(CString& strFolder, LPCSTR path, int x,int y);
//////////////////////////////////////////////////////////////////////
//region function
///////////////////////////////////////////////////////////////////////
//data
	int m_xRegion,m_yRegion; //当前左上的Region
	bool IsRegionValid(){return m_xRegion >= 0 && m_yRegion >= 0;}
	int m_wRegion,m_hRegion; 
	enum{MAX_REGION_WIDTH = 7, MAX_REGION_HEIGHT = 7,};
	cRegion* m_aRegion[MAX_REGION_WIDTH][MAX_REGION_HEIGHT];
	cRegion* GetLinkRegion(cRegion* p, eRegionDirection e);
	enum {MAX_REGION = 24,};
	cRegionList m_listRegion;
	int GetRegionNum();
	int GetRegionMemory();

	void	RepaintTerrain();
//根据坐标得到region的指针
	cRegion* GetcRegionByCoordinate(int tx,int ty);								//得到最终的m_aRegion[x][y]
	cRegion* GetcRegionByRegion(cPoint ptRegion);						//ptRegion是region的坐标
	cRegion* GetcRegionByRegion(int tx,int ty);							
	cRegion* GetcRegionByTile(cPoint ptTile);
	cRegion* GetcRegionByGround(cPoint ptGround);
	iRegion* GetiRegion(cPoint ptGround);
	cRegion* GetcRegionByScreen(CPoint ptScreen);
	BOOL IsRegionCoordinateValid(int x,int y);
	cRegion* GetcRegion(int tx,int ty);										//tx,ty没有offset

	void RegionToCoordinate(int& tx,int& ty);
	void CoordinateToRegion(int& tx,int& ty);
	void RegionOffset(int& tx,int& ty);										//得到offset后的x,y
	void RegionUnoffset(int& tx,int& ty);									

	//region terrain need paint
	bool IsCoordinateInPaint(int x,int y);
	//the plane of the region need paint
	bool IsCoordinateInPaint2(int x,int y);

	cRegion* GetFocusRegion();												//得到camera指向的region

//得到region的范围
	cRect GetRectGround();													//得到覆盖当前的ground的最大的rect
	cRect GetRectGroundFormat();											//得到覆盖当前的ground的最大的有效的rect
	cRect GetRectTile();
	cRect GetRectTileFormat();
	void GetRegionSize(int& wRegion, int& hRegion);
	void ComputeRegionSize(int& wRegion, int& hRegion);						//计算当前的camera来计算region的长宽
	void ComputeRegionPosBySize(int& x,int& y, int w, int h, const cRect& r);		//根据w,h和r四舍五入计算起始点!
	void RecomputeRegionPos(int& x,int& y);									//重新计算m_xRegion和m_yRegion
	cPoint GetCenterTileByRegion();											//get center according to m_xRegion,m_yRegion and m_wRegion,m_hRegion
	bool IsRegionCoverScreen();												//region是否覆盖了屏幕
//region clear
	HRESULT ClearRegion(cRegion* pRegion);
	HRESULT RemoveRegion(cRegion* pRegion);
	HRESULT ClearAllRegion();
	HRESULT ClearGround();
	HRESULT ClearRegionList(cRegionList& rlist);
	HRESULT ClearFarestRegion();											//根据camera的位置决定最远的一个region在那里
	HRESULT ClearFarestRegion(cPoint ptFocusTile);
	HRESULT UpdateRegion();

	BOOL	m_bRebuildRegion;
	HRESULT RebuildRegion();
	HRESULT OffsetRegion();
//操作Region中的一些元素

	HRESULT	BackFile(int xRegion, int yRegion, LPCSTR szFile) ;
	HRESULT	RestoreFile(int xRegion, int yRegion, LPCSTR szFile) ;

	HRESULT UpdateVertex(int x,int y, stVertexGround v);
	
	HRESULT GetRegionTile(int x,int y,stTile& resTile);			//
	HRESULT SetRegionTile(int x,int y,const stTile& resTile);
	HRESULT FillRegion(CRect rcFill,LPCSTR szSprFile);
	HRESULT FillRegion(int x,int y,LPCSTR szSprFile);
	HRESULT SetRegionDraw(float fx,float fy,BOOL bDraw);
	HRESULT SetRegionFormat(float x,float y,int nFormat);
	HRESULT SetRegionColor(float x,float y,cColor cr);
	HRESULT GetRegionColor(float x,float y,cColor& cr);
	int		GetPlaneNum();
	int		GetLightNum();

//////////////////////////////////////////////////////////////////////
//从地图上获取元素
//////////////////////////////////////////////////////////////////////
//得到自动拼接的点
	BOOL	GetLinkOffsetTile(cPoint& ptOffset, cPoint ptPlane, float& fMinDis );
	BOOL	GetLinkOffsetVer(cPoint& ptOffset, cPlaneVer* p, int eLink,cPoint ptPlane, float& fMinDis );
	BOOL	GetLinkOffset(cPoint& ptOffset, cPlane* pExcept, int eLink,cPoint ptPlane, float& fMinDis );
	BOOL	GetLinkOffset(cPoint& ptOldOffset, iObj* pPlane, int eLink) ;
//////////////////////////////////////////////////////////////////////
//得到鼠标信息的点
//////////////////////////////////////////////////////////////////////
	HRESULT GetiObj(int nID, iObjList& iList);
	HRESULT	GetiLight(stSelect& st,iObjList& tlist) ;
	HRESULT	GetiObstacle(stSelect& st,iObjList& tlist) ;
	HRESULT	GetiPlane(stSelect& st,iObjList& tlist) ;
	HRESULT	GetiPlaneByPoint(POINT ptScreen,iObjList& tlist);
	HRESULT	GetiObstacleByPoint(POINT ptScreen,iObjList& tlist) ;
	HRESULT	GetiLightByPoint(POINT ptScreen,iObjList& tlist) ;
	HRESULT GetiPlaneByRect(cRect rcGround,iObjList& tlist) ;
	HRESULT	GetiObstacleByRect(cRect rcGround,iObjList& tlist) ;
	HRESULT	GetiLightByRect(cRect rcGround,iObjList& tlist) ;
	HRESULT	GetiPlaneByDiamond(cRect rcGround,iObjList& tlist) ;
	HRESULT	GetiObstacleByDiamond(cRect rcGround,iObjList& tlist) ;
	HRESULT	GetiLightByDiamond(cRect rcGround,iObjList& tlist) ;

//得到obstacle的类型,
//bytile:使用格子点还是线段,
	ObstacleTypeEnum GetObstacleType(){return m_param.eObstalceType;} 
	ObstacleEnum	CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise);
//判断是否是被阻挡,
	BOOL BlockObstacle(cPoint pt,ObstacleEnum e);
	HRESULT	GetTileType(cPoint ptGround,long& l);

//添加和删除元素
	
	//地图编辑器选择并且移动的图素,主要是为了移动组件的效率

	cBTree*			m_pTree;
	//地图编辑器适用
	cPlaneHorList	m_listMovingObj;		//其中给出简单的排序即可
	cGroundObjList	m_listDynamicObj;		//其中给出简单的排序即可

	cPlaneVerList	m_listPlaneVer;	
	cPlaneVerList	m_listPlaneVerPoint;	
	cPlaneVerList	m_listPlaneVerSeam;
	LineArray		m_aLine;
//	BOOL			m_bBuildTree;
	HRESULT			RebuildTree();

	cPlaneList		m_listPlaneAboveVer;	//高于地表的元素也统一排序
	cLightList		m_listLight;

	BOOL AddVer(cPlaneVer* p);
	BOOL RemoveVer(cPlaneVer* p);
	BOOL AddPlaneAbove(cPlane* p);
	BOOL RemovePlaneAbove(cPlane* p);

	HRESULT AddiObj(iObj* p, bool bRunNow );
	HRESULT RemoveiObj(iObj* p, bool bRunNow );
	HRESULT AddcObj(iObj* p);
	HRESULT RemovecObj(iObj* p);
	HRESULT AddcPlane(cPlane* p);
	HRESULT RemovecPlane(cPlane* p);
	//if bToRegion = true 表示这个plane是外来的需要加入Region中去
	//否则这个plane是region自身的
	void	AddPlane(cPlane* p,cRegion* pRegion,BOOL bToRegion);
	void	RemovePlane(cPlane* p,cRegion* pRegion);

	HRESULT AddcObstacle(cObstacle* p);
	HRESULT RemovecObstacle(cObstacle* p);

	HRESULT AddClientOnlyNpc(cPlane* p);
	HRESULT RemoveClientOnlyNpc(cPlane* p);
	HRESULT AddVerBelowGround(cPlane* p);
	HRESULT RemoveVerBelowGround(cPlane* p);

	void	AddLight(cLight* p);
	void	RemoveLight(cLight* p);
	HRESULT AddcLight(cLight* p);
	HRESULT RemovecLight(cLight* p);
//notify
	HRESULT ObjNotify(cGroundObj* p,eNotifyType eType,long lParam = 0);
	HRESULT CameraNotify(cCamera* p,eNotifyType eType,long lParam);
		
//////////////////////////////////////////////////////////////////////
//light
//////////////////////////////////////////////////////////////////////
	void BeginLighting();
	void GetLightMap();
	void EndLighting();
	cColorMap* m_pColorMap;
	cColorMap* GetColorMap(){return m_pColorMap;}
	bool GetColor(const stVertexGround& v, DWORD& value);
	bool GetColor(cPoint ptGround, DWORD& value);
	bool GetLightParam(const stVertexGround& v, stLightParam& param);
	bool GetLightParam(cPoint ptGround, stLightParam& param);
	void RebuildColorMap();

//////////////////////////////////////////////////////////////////////
//paint loop
//////////////////////////////////////////////////////////////////////
	virtual void Paint();
	void CheckDisplay();
	void Draw();
	void PaintOthers();
	//to get how much plane draw?
	int m_nPlaneDraw;
	int GetPlaneDrawNum(){return m_nPlaneDraw++;}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
	
//vertex buffer
//device
	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
	virtual HRESULT FinalCleanup();

//Environment
	virtual HRESULT SetEnvironment(){return S_OK;};
	virtual HRESULT ClearEnvironment(){return S_OK;};
//	DWORD GetBrightness(){return m_param.crColor.a;}
	cColor GetColor(){return m_param.crColor;}
	cPoint GetWindSpeed(){return m_param.ptWindSpeed;}
	
//draw
	virtual HRESULT SetTexture(iTexture* pTex, int frame = 0) = 0;

	//面的函数
	virtual void Draw3Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3) = 0;
	virtual void Draw4Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3,stVertexGround& v4) = 0;
	virtual void DrawQuadrangle(stVertexGround* pV) = 0;
	virtual void DrawTriangle(stVertexGround* pV) = 0;
	virtual void DrawQuadrangle(stVertexGround* pV,int time) = 0;
	BOOL IsCreateObstacle(){return IsEditing();}

	//描线的函数
	virtual void DrawPolygon(stVertexGround* pV, int num, cColor cr) = 0;
	virtual void DrawRect(cRect r, cColor cr,float fZ = 0.f) = 0;
	virtual void DrawRect(stVertexGround* pV, cColor cr) = 0;
	virtual void DrawLine(cPoint pt1,cPoint pt2, cColor cr) = 0;	
	virtual void DrawLine(stVertexGround* pV, int n) = 0;

	virtual void DrawPoint(cPoint pt, cColor cr) = 0;	
	virtual void DrawPoint(std::list<cPoint>& ptlist, cColor cr) = 0;
	virtual void DrawPoint(cPoint* aPoint, int nPoint, cColor cr) = 0;
	virtual void DrawPointAsRect(std::list<cPoint>& ptlist, cColor cr) = 0;


public:
//坐标变换	
	float	GetGroundHeight(cPoint ptGround);
	HRESULT ScreenToGround(CPoint ptScreen, cPoint& ptGround) ;
	HRESULT ScreenToGround(POINT ptScreen, cPoint& ptGround) {return ScreenToGround(CPoint(ptScreen.x,ptScreen.y),ptGround);};

//special
	SpecialList	m_listSpecial;
	HRESULT	AddiSpecial(iSpecial* p);
	HRESULT	RemoveiSpecial(iSpecial* p);
	void	LoadSpecial(LPCSTR szFile);
	void	SaveSpecial(LPCSTR szFile);
	void	DrawSpecial();
	iSpecial* SelectiSpecial(POINT ptScreen) ;					//一次只能选择一个
	friend class cGraphics;	


	BOOL	FindWalkPath(std::list<cPoint>& list, cPoint ptStart, cPoint ptEnd);
};

BOOL AddPlaneVerToLineArray(LineArray& m_aLine, cPlaneVer* p);
void SortLineArray(LineArray& aline);

#endif // !defined(AFX_CGROUND_H__45B2974B_97A6_4AB2_87B2_9CED55E3B184__INCLUDED_)
