// iGraphics.h: interface for the iGraphics class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IGRAPHICS_H__E4C4F444_2461_4F89_B782_A9476A8635FA__INCLUDED_)
#define AFX_IGRAPHICS_H__E4C4F444_2461_4F89_B782_A9476A8635FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iPublic.h"
#include "iColor.h"

#include <d3d8.h>
#include <d3dx8.h>

#include "iTexture.h"
#include "iObj.h"
/************************************************************
const declare
/************************************************************/
#include <math.h>

const float		c_fPI				= 3.14159265f;
const float		c_fHeightRange		= 6.f;
const float		c_fSqrt3			= (float)sqrt(3);
const cPoint	c_ptMul				= cPoint(1.f,2.f,2.f/c_fSqrt3);
const cPoint	c_ptScale			= cPoint(32.f/c_ptMul.x,32.f/c_ptMul.y,32.f/c_ptMul.z);

const int		c_nWidthGround			= 32;
const int		c_nHeightGround			= 16;
const int		c_nWidthTile			= 32;
const int		c_nHeightTile			= 32;
const int		c_nWidthRegionTexture	= 512;
const int		c_nHeightRegionTexture  = 512;

const int		c_tWidthRegion		= c_nWidthRegionTexture / c_nWidthTile;
const int		c_tHeightRegion		= c_nHeightRegionTexture / c_nHeightTile;
const int		c_nWidthRegion		= c_nWidthRegionTexture / c_nWidthGround;
const int		c_nHeightRegion		= c_nHeightRegionTexture / c_nHeightGround;

const int		MAX_DISPLAY_REGION = 3;
const int		GROUND_DIVIDE = 1;
const int		MAX_TILE_X = c_nWidthRegion * GROUND_DIVIDE * MAX_DISPLAY_REGION;
const int		MAX_TILE_Y = c_nHeightRegion * GROUND_DIVIDE * MAX_DISPLAY_REGION;
const int		MAX_LIGHT_RADIUS = 8;
const int		MAX_LIGHT_TILE = MAX_LIGHT_RADIUS*GROUND_DIVIDE*2+1;

/************************************************************
//select struct:
/************************************************************/

struct stSelect 
{
	SelectPointEnum				ePoint;
	POINT						ptScreen;
	cPoint						ptGround;
	RECT						rcScreen;
	cRect						rcGround;

	SelectTypeEnum				eType;
	KindEnum					eKind;
	SortEnum					eSort;

	SelectFilterEnum			eFilter;
	SelectFilterParamEnum		eFilterParam;
};

/************************************************************
//class iCamera
/************************************************************/
class iCamera
{
public:
	enum CommandEnum{CCOM_UPDATE,CCOM_SAVE,CCOM_LOAD};

	enum MaskEnum{
		MASK_EYE				= 0x01,
		MASK_TO					= 0x02,
		MASK_UP					= 0x04,
		MASK_ANGLE				= 0x08,
		MASK_DIS				= 0x10,

		MASK_FOV				= 0x20,
		MASK_ASPECT				= 0X40,
		MASK_NEARPLANE			= 0x80,
		MASK_FARPLANE			= 0x100,
		MASK_ANGLEX				= 0x200,

		MASK_SCALE				= 0x400,
		MASK_MAPEDIT			= 0x800,
		MASK_COMMAND			= 0x1000, 
	};

	struct stParam
	{
		cPoint					ptEye;
		cPoint					ptTo;
		cPoint					ptUp;
		float					fAngle;
		float					fDis;

         // Attributes for projection matrix
		FLOAT					fFOV;
		FLOAT					fAspect;
		FLOAT					fNearPlane;
		FLOAT					fFarPlane;
		float					fAngleX;

		char					szFile[MAX_CHAR];

		float					fScale;
		CommandEnum				eCommand;

		long					mask;
		long					lParam,wParam;
		
	};

	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;
	virtual HRESULT				GetParam(stParam& param) = 0;
	virtual HRESULT				SetParam(stParam& param, bool bRunNow = true) = 0;

	virtual void				GroundToScreen(cPoint p,POINT& pt) = 0;	
	virtual void				GroundToScreen(cPoint p,cPoint& pt) = 0;	
	virtual void				GroundToScreen(cPoint& pt) = 0;	
	virtual void				ScreenToGround(POINT p,cPoint& pt) = 0;	
	virtual void				ScreenToGround(cPoint p,cPoint& pt) = 0;	
	virtual void				ScreenToGround(cPoint& pt) = 0;	
	virtual bool				IsPointInScreen(cPoint p) = 0;
};

/************************************************************
//class iScreen
/************************************************************/
class iScreen
{
public:
	enum CommandEnum{SCOM_ADDOBJ,SCOM_REMOVEOBJ,};

	enum MaskEnum{
		MASK_COMMAND			= 0x01, 
		MASK_TYPE				= 0x02,
		MASK_COMMON				= 0x04,

		MASK_SCREENSIZE			= 0x1000,
		MASK_SELECTRECT			= 0x2000,
	};
	
	struct stParam
	{
		long					mask;
		long					lParam, wParam;
		CommandEnum				eCommand;		

		int						fWidth,fHeight;
		RECT					rcSelect;
		iObj*					pObj;
	};
	
	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;
	virtual HRESULT				GetParam(stParam& param) = 0;
	virtual HRESULT				SetParam(stParam& param, bool bRunNow = true) = 0;

	virtual iObj*				GetiMap() = 0;
	virtual iObj*				GetiMapSmall() = 0;

	virtual HRESULT				AddiObj(iObj* p) = 0;
	virtual HRESULT				RemoveiObj(iObj* p) = 0;
};

/************************************************************
//class iGround
/************************************************************/

/************************************************************
//base struct:
/************************************************************/
struct stVertexGround
{
	float sx, sy, sz;
	DWORD color;
	float tu, tv;
};

struct stVertexScreen
{
	float sx, sy, sz, rhw;
	DWORD color;
	float tu, tv;
};
typedef std::list<stVertexGround*>			VGList;
typedef VGList::iterator				itVG;
typedef std::vector<stVertexGround*>			VGArray;
typedef VGArray::iterator				itVGA;

struct stTile
{
	stVertexGround v;
	long idResource;				
	char szResource[MAX_CHAR];
	int iFrame;
	int iDirection;
	stTile(){idResource = 0; szResource[0] = 0;}
};

struct stTerrainTile
{
//	cPoint ptPos;
	char idResource;
	unsigned char iDirection;
	unsigned short iFrame;
};

enum TileEnum
{
	TILE_BLOCKLIGHT = 0x01,			//是否阻挡了光线
	TILE_BLOCKWALK	= 0x02,			//是否阻挡了行走
	TILE_BLOCKFLY	= 0x04,			//是否阻挡了飞行
	TILE_BLOCKJUMP	= 0x08,			//是否阻挡了跳跃
};

enum LinkEnum
{
	LINK_NULL = 0,
	LINK_VER = 0x01,				//跟垂直的面相连
	LINK_HOR = 0x02,				//跟平行的面相连
	LINK_TILE = 0x04,				//跟整数的点相连
};

class iRegion
{
public:
	enum MaskEnum
	{
		MASK_COMMAND			= 0x01,
		MASK_COLOR				= 0x02,
		MASK_DRAW				= 0x04,
		MASK_MAPEDIT			= 0x08,
		MASK_TRANSPARENT		= 0x10,
		MASK_ANIMATION			= 0x20,
		MASK_ANIMATIONSPEED		= 0x40,	
		MASK_ANIMATIONDEGREE	= 0x80,
	};
	enum AnimationEnum
	{
		ANI_NULL,
		ANI_BLEND,				//草地微微摆动
	};
	enum CommandEnum
	{
		RCOM_NULL,
		RCOM_DELETE,
		RCOM_NUM,
	};
	struct stParam
	{
	public:
		long					mask;
		long					lParam, wParam;
		CommandEnum				eCommand;		
		BitOPEnum				eOPMapedit;
		long					lMapeditEnum;
		cColor					crColor;
		BOOL					bDraw;
		BOOL					bTransparent;
		AnimationEnum			eAnimation;
		int						nSpeed;
		float					fDegree;
	};

	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;

	virtual HRESULT				GetParam(stParam& param) = 0;
	virtual HRESULT				SetParam(stParam& param, bool bRunNow = true) = 0;
};

//特殊的ground对象，例如雨雪，卷轴等
//class CString;

class iSpecial
{
public:
	enum TypeEnum{ST_NORMAL,ST_SCROLL};

	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;

//它们之间很难有共同的属性以及派生关系，不会频繁的改变属性,
//但是又想接口统一,不频繁的修改头文件，所以使用字符串来作为参数,比mask要方便但是要慢!
	virtual HRESULT				SetParam(LPCSTR szItem, LPCSTR szValue) = 0;
	virtual HRESULT				GetParam(LPCSTR szItem, LPSTR szValue) = 0;
	virtual HRESULT				GetParam(LPCSTR szItem, std::string& szValue) = 0;

	virtual ElementEnum			GetElementType() = 0;
	virtual HRESULT				Load(const stSection* p) = 0;
	virtual HRESULT				Save(stSection* p) = 0;
	virtual HRESULT				LoadBinary(CFile& f) = 0;
	virtual HRESULT				SaveBinary(CFile& f) = 0;

//遍历所有的参数,为了编辑框容易制定
	virtual	int					GetItemCount() = 0;
	virtual HRESULT				GetParam(int index, std::string& szItem, std::string& szValue, std::string& szDescription) = 0;
	virtual HRESULT				SetParam(int index, LPCSTR szValue) = 0;
//	virtual BOOL				GetFirstParam() = 0;
//	virtual BOOL				GetNextParam(CString& szItem, CString& szValue, CString& szDescription) = 0;

	enum EventEnum{
		SE_CAMERANOTIFY,//		接受Camera move的消息
		};
	enum DrawEnum
		{
		SD_DRAWBEFOREALL,
		SD_DRAWAFTERALL,
		};
	virtual BOOL				IsAcceptEvent(EventEnum e) = 0;
	virtual HRESULT				OnEvent(EventEnum e, long lParam, long wParam) = 0;
	virtual BOOL				IsActive() = 0;			//是否激活
	virtual BOOL				Active() = 0;	
	virtual BOOL				Run() = 0;
	virtual	BOOL				IsDraw(DrawEnum e) = 0; //是否绘制
	virtual HRESULT				Draw(DrawEnum e) = 0;

	virtual	void				Select(BOOL bSelect) = 0;	//被编辑选中
	virtual HRESULT				Draw() = 0;					//绘制自身为了编辑操作
	virtual BOOL				Contain(cPoint ptGround) = 0;

	virtual HRESULT				InitDeviceObjects() = 0;
    virtual HRESULT				RestoreDeviceObjects() = 0;                     
    virtual HRESULT				InvalidateDeviceObjects() = 0;
    virtual HRESULT				DeleteDeviceObjects() = 0;
    virtual HRESULT				FinalCleanup() = 0;
};

enum ObstacleTypeEnum{OT_SEAM,OT_TILE,};

class iGround
{
public:
	enum TypeEnum{GT_NORMAL,};
	enum CommandEnum
	{
		GCOM_NEW, GCOM_LOAD, GCOM_SAVE, GCOM_APPEND,
		GCOM_ADDOBJ, GCOM_REMOVEOBJ,

		GCOM_FILLREGION,

		GCOM_NEWDYNAMIC,
		GCOM_OPENDYNAMIC,
		GCOM_SAVEDYNAMIC,

		GCOM_CHANGETILE,
		GCOM_GETTILE,

		GCOM_UPDATEREGIONTEXTURE,

		GCOM_VERTEXHIGHER		= 0x20,
		GCOM_VERTEXLOWER		= 0x40,
		GCOM_VERTEXUPDATE		= 0x80,		
	};

	enum MaskEnum{
		MASK_COMMAND			= 0x01, 
		MASK_TYPE				= 0x02,
		MASK_UPDATEREGIONTEXTURE= 0x04,
		MASK_MAPEDIT			= 0x08,
		MASK_BUILDTREE			= 0x10,
		MASK_RENDERMAP			= 0x20,
		MASK_COLOR				= 0x40,

		MASK_PLAYING			= 0x80,
		MASK_NOTLOADCAMERA		= 0x100,

		MASK_COMMON				= 0x1000,
//		MASK_BRIGHTNESS			= 0x2000,
		MASK_WINDSPEED			= 0x4000,
		MASK_OBSTACLE_TYPE		= 0x8000,
	};

	struct stParam
	{
		long					mask;
		long					lParam, wParam;
		CommandEnum				eCommand;		
		TypeEnum				eType;
		BOOL					bUpdateRegionTexture;
		BOOL					bBuildTree;
		BOOL					bRenderMap;
		BitOPEnum				eOPMapedit;
		long					lMapeditEnum;

		long					lVerson;
		RECT					rcGround;
		
		BOOL					bPlayering;
		
		cPoint					ptOffset;
		POINT					ptScreen;
		char					szFile[MAX_CHAR];
		char					szDynamic[MAX_CHAR];
//		int						nBrightness;		//亮度
		cPoint					ptWindSpeed;
		cColor					crColor;

		int						xPos,yPos;			//change tile
		RECT					rcFill;				//fill region
		stTile					tile;
		ObstacleTypeEnum		eObstalceType;

		iObj*					pObj;
	};

	virtual BOOL				Game_Load(LPCSTR szMap,cPoint ptCameraTo) = 0;

	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;

	virtual iRegion*			GetiRegion(cPoint ptGround) = 0;

	virtual HRESULT				GetParam(stParam& param) = 0;
	virtual HRESULT				SetParam(stParam& param, bool bRunNow = true) = 0;

	virtual BOOL				IsModify() = 0;//地图是否改变
	virtual HRESULT				GetiObj(int id, iObjList& ilist ) = 0;
	virtual HRESULT				GetiPlane(stSelect& st,iObjList& tlist) = 0;
	virtual HRESULT				GetiObstacle(stSelect& st,iObjList& tlist) = 0;
	virtual HRESULT				GetiLight(stSelect& st,iObjList& tlist) = 0;

	virtual HRESULT				GetiPlaneByPoint(POINT ptScreen,iObjList& tlist) = 0;
	virtual HRESULT				GetiObstacleByPoint(POINT ptScreen,iObjList& tlist) = 0;
	virtual HRESULT				GetiLightByPoint(POINT ptScreen,iObjList& tlist) = 0;

	virtual HRESULT				GetiPlaneByRect(cRect rcGround,iObjList& tlist) = 0;
	virtual HRESULT				GetiObstacleByRect(cRect rcGround,iObjList& tlist) = 0;
	virtual HRESULT				GetiLightByRect(cRect rcGround,iObjList& tlist) = 0;
 
	virtual HRESULT				GetiPlaneByDiamond(cRect rcGround,iObjList& tlist) = 0;
	virtual HRESULT				GetiObstacleByDiamond(cRect rcGround,iObjList& tlist) = 0;
	virtual HRESULT				GetiLightByDiamond(cRect rcGround,iObjList& tlist) = 0;
	virtual BOOL				FindDynamicNick(LPCSTR szNick) = 0;

	virtual iObj*				GetiSelect() = 0;
	virtual HRESULT				ScreenToGround(POINT ptScreen, cPoint& ptGround) = 0;

	//为了自动拼接的需要提供，只限于地图编辑器
	//根据当前的plane的物理信息,返回offset,在该offset上可以跟地图中的某一plane的顶点接在一起
	virtual BOOL				GetLinkOffset(cPoint& ptOldOffset, iObj* pPlane, int eLink) = 0;

	virtual HRESULT				AddiObj(iObj* p, bool bRunNow = true) = 0;
	virtual HRESULT				RemoveiObj(iObj* p, bool bRunNow = true) = 0;

	//碰撞检测
	virtual ObstacleEnum		CheckObstacle(cPoint ptGround,cPoint& vSpeed, bool bClockWise) = 0;					//根据多边形来检测
	virtual BOOL				BlockObstacle(cPoint pt,ObstacleEnum e) = 0; 
	virtual HRESULT				GetTileType(cPoint ptGround, long& type) = 0;		//根据格子来检测

	//Special
	virtual HRESULT				AddiSpecial(iSpecial* p) = 0;
	virtual HRESULT				RemoveiSpecial(iSpecial* p) = 0;
	virtual iSpecial*			SelectiSpecial(POINT ptScreen) = 0;					//一次只能选择一个

	virtual HRESULT				BackFile(int xRegion, int yRegion, LPCSTR szFile) = 0;
	virtual HRESULT				RestoreFile(int xRegion, int yRegion, LPCSTR szFile) = 0;

	virtual BOOL				FindWalkPath(std::list<cPoint>& list, cPoint ptStart, cPoint ptEnd) = 0;
};

//坐标变换	

_declspec(dllexport) void	TileToGround(float x,float y,float &tx,float &ty);
_declspec(dllexport) void	TileToGround(float &tx,float &ty) ;
_declspec(dllexport) void	TileToGround(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	TileToGround(cPoint& pt) ;

_declspec(dllexport) void	GroundToTile(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void	GroundToTile(float &tx,float &ty) ;
_declspec(dllexport) void	GroundToTile(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	GroundToTile(cPoint& pt) ;

_declspec(dllexport) void	DiamondSmallToTile(float &tx,float &ty) ;
_declspec(dllexport) void	DiamondSmallToTile(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void	DiamondSmallToTile(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	DiamondSmallToTile(cPoint& pt) ;

_declspec(dllexport) void	TileToDiamondSmall(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void	TileToDiamondSmall(float &tx,float &ty) ;
_declspec(dllexport) void	TileToDiamondSmall(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	TileToDiamondSmall(cPoint& pt) ;

_declspec(dllexport) void	TileToDiamond(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void	TileToDiamond(float &tx,float &ty) ;
_declspec(dllexport) void	TileToDiamond(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	TileToDiamond(cPoint& pt) ;

_declspec(dllexport) void	DiamondToTile(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void	DiamondToTile(float &tx,float &ty) ;
_declspec(dllexport) void	DiamondToTile(cPoint p,cPoint& pt) ;
_declspec(dllexport) void	DiamondToTile(cPoint& pt) ;

_declspec(dllexport) void	TileToRegion(float x,float y,float &tx,float &ty);
_declspec(dllexport) void	TileToRegion(float &tx,float &ty);
_declspec(dllexport) void	TileToRegion(cPoint p,cPoint& pt);
_declspec(dllexport) void	TileToRegion(cPoint& pt);
_declspec(dllexport) void	TileToRegion(int x,int y,int &tx,int &ty);
_declspec(dllexport) void	TileToRegion(int &tx,int &ty);

/************************************************************
//class iGraphics
/************************************************************/

class iGraphics  
{
public:
	enum ShowEnum
	{
		GS_NULL				= 0,
		GS_PICTURE			= 0x01,
		GS_HOR				= 0x02,
		GS_VER				= 0x04,
		GS_DYNAMIC			= 0x08,
		GS_LIGHT			= 0x10,
		GS_OBSTACLE			= 0x20,
		GS_TERRAIN			= 0x40,
//		GS_HOR_GROUND		= 0x80,
//		GS_HOR_BELOW_VER	= 0x100,
//		GS_HOR_ABOVE_VER	= 0x200,
		GS_MAP				= 0x400,
		GS_SPECIAL			= 0x800,	//显示特殊效果
		GS_SMALLMAP			= 0x1000,
		GS_ALL				= -1,
	};
	enum CommandEnum{
		GCOM_INIT,	
	};
	
	enum TraceEnum
	{
		TRACE_MOUSE = 0x01,
		TRACE_GROUND = 0x02,
		TRACE_CACHE = 0x04,
		TRACE_CAMERA = 0x08,
		TRACE_FPS = 0x10,
	};
	enum MaskEnum{
		MASK_COMMAND			= 0x01, 

		MASK_INSTANCE			= 0x10,
		MASK_WND				= 0x20,
		MASK_POS				= 0x40,
		MASK_SIZE				= 0x80, 
		MASK_FULLSCREEN			= 0x100,
		MASK_FRAME				= 0x200,
		MASK_TRACE				= 0x400,
		MASK_PATH				= 0x1000,
		MASK_NAME				= 0x2000,
		MASK_PAUSEPAINT			= 0x4000,

		MASK_MOUSE				= 0x40000,
		MASK_GAMELOOP			= 0x80000,
		
		MASK_SHOW				= 0x1000000,		//显示所有的排序线段
	};

	struct stParam
	{
		CommandEnum				eCommand;
		HINSTANCE				hInst;
		HWND					hWnd;
		int						x,y;
		int						fWidth,fHeight;
		bool					bFullScreen;
		int						nFrame;				//保持在多少帧

		char					szPath[MAX_PATH];
		char					szName[MAX_PATH];

		BOOL					bPausePaint;
		int						nGameLoop;			//游戏循环的计数器
		cPoint					ptMouse;			//鼠标的位置

		BitOPEnum				eOPTrace;
		long					lTraceEnum;			//显示什么样的信息
		BitOPEnum				eOPShow;
		long					lShowEnum;			//显示哪些集合

		long					mask;
		long					lParam, wParam;
	};

	virtual HRESULT				CreateGraphicsElement(ElementEnum id,LPVOID ppointer) = 0;

	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;

	virtual HRESULT				SetParam(stParam& param, bool bRunNow = true) = 0;
	virtual HRESULT				GetParam(stParam& param) = 0;

	virtual HRESULT				Begin() = 0;
	virtual HRESULT				End() = 0;
	virtual HRESULT				GraphicsRun() = 0;			//single thread
	virtual HRESULT				RunThread() = 0;	//multipul thread
	virtual HRESULT				EndThread() = 0;
	virtual ULONG				GetFrames() = 0;
	
	virtual iGround*			GetiGround() = 0;  
	virtual iScreen*			GetiScreen() = 0;
	virtual iCamera*			GetiCamera() = 0;
	virtual HRESULT				PrintScreen(LPCSTR szFile) = 0;//截获屏幕
	virtual HRESULT				SaveBmp(const char* bmp_name, cRect rect) = 0;
	virtual HRESULT				SaveBmpBuffer(DWORD* buffer, cRect rect) = 0;
	virtual void				Trace(LPCSTR str) = 0;

	virtual HRESULT				RefreshSpr(LPCSTR szSpr, bool bRunNow = true) = 0;
	virtual LRESULT				WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) = 0;
};

class cGraphics;

/************************************************************
//class iFont
/************************************************************/
/*class iFont
{
public:
	struct stParam
	{
		cPoint					ptPos;				//起始点
		cColor					crColor;			//颜色
		DWORD					lFlag;				//标记(粗体，斜体)
	};

	enum {
		D3DFONT_BOLD			= 0x0001,
		D3DFONT_ITALIC			= 0x0002,
	};

	enum {
		D3DFONT_CENTERED		= 0x0001,
		D3DFONT_TWOSIDED		= 0x0002,
		D3DFONT_FILTERED		= 0x0004,
	};


	virtual ULONG				AddRef() = 0;	
	virtual ULONG				Release() = 0;
	
	virtual HRESULT				Create(LPCSTR strFontName, DWORD dwHeight, DWORD dwFlags=0L) = 0;
    virtual HRESULT				DrawText( const stParam& param, LPCSTR strText, int nLength) = 0;
	virtual HRESULT				InitDeviceObjects() = 0;
    virtual HRESULT				RestoreDeviceObjects() = 0;                     
    virtual HRESULT				InvalidateDeviceObjects() = 0;
    virtual HRESULT				DeleteDeviceObjects() = 0;
};*/

enum eEngineType{Graphics_3D,Graphics_2D,Graphics_GDI,};

_declspec(dllexport) iGraphics* Graphics_CreateGraphics(eEngineType eType,long verson1 = 0,long verson2 = 0);

//mapconvert function!
_declspec(dllexport) int updateimageinfo(LPCSTR szPlaneFile) ;
_declspec(dllexport) int createlinkhor(LPCSTR szRegionFolder) ;
_declspec(dllexport) void GetSeamCrossTile(cPoint p1,cPoint p2,std::list<cPoint> & ptlist);

#endif // !defined(AFX_IGRAPHICS_H__E4C4F444_2461_4F89_B782_A9476A8635FA__INCLUDED_)
