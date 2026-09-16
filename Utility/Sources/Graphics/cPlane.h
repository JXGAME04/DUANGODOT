// cPlane.h: interface for the cPlane class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANE_H__7F34187D_70F1_46E6_8F59_939631B102B3__INCLUDED_)
#define AFX_CPLANE_H__7F34187D_70F1_46E6_8F59_939631B102B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
//typedef vector<VGArray> VGArray2;
//typedef VGArray2::iterator itVGA2;

//#define GET_FILELIST

#ifdef GET_FILELIST
#include <hash_map>
typedef std::hash_map<std::string, int, std::hash<std::string>, std::equal_to<std::string> > fileMap;
typedef fileMap::iterator itFile;
extern fileMap g_mapFile;
#endif

class cMesh;
class cObstacle;
class iItem;
class cObstacle;
struct stTemplateOne;

//				排序	受光	挡光
//道路			hor		seam	no
//小的物件		hor		point	no
//小的物件		ver		point	no
//屋檐			hor		seam	no
//人物			ver		point	no
//墙壁			ver		seam	seam
//树木			ver		seam	circle
struct stTexInfo
{
	int index;
	ULONG width,height;
	int x,y;
	cRect rcTex;
};

#include "cGroundObj.h"

enum ePlane2Plane{PP_FRONT,PP_BACK,PP_CROSS,PP_UNKNOW,};

class cPlane: public cGroundObj
{
public:
	cPlane(cGraphics* p);
	virtual ~cPlane();
	virtual void			Init(bool bCreateBuffer);

///////////////////////////////////////////////////////////////////
//save and load	
///////////////////////////////////////////////////////////////////
	HRESULT					Load(const stSection* p);
	HRESULT					Save(stSection* p);
	HRESULT					SaveObstalce(iItem* pItem, int& num);
	virtual HRESULT			LoadPack(void** pParam);
	virtual HRESULT			LoadBinary(CFile& f);
	virtual HRESULT			SaveBinary(CFile& f);
//static only
	void					SaveToName(LPCSTR szFile);
//	virtual LPCSTR			GetDefaultTemplate(){return "";};
	HRESULT					LoadFromTemplate(LPCSTR szFile);
	HRESULT					LoadFromTemplate(stTemplateOne* pTemplate);

	HRESULT					SetPlaneResource(stTemplateOne* pItem);
	virtual HRESULT			SetPlaneProperty(stTemplateOne* pItem);
	HRESULT					SetPlaneSave(stTemplateOne* pItem);
	HRESULT					SetPlaneObstacle(stTemplateOne* pItem);

///////////////////////////////////////////////////////////////////
//device
///////////////////////////////////////////////////////////////////
    virtual HRESULT			FinalCleanup();

///////////////////////////////////////////////////////////////////
//param
///////////////////////////////////////////////////////////////////
	ElementEnum				GetElementByType();
	void					GetTypeString(CString& s);
	stPlaneParam*			GetPlaneParam(){return (stPlaneParam*) m_pParam;};
	virtual SortEnum		GetSortType(){return GetPlaneParam()->eSort;}
	BOOL					IsSortType(SortEnum e){return GetSortType() == e;}

	BOOL					IsSortVerBelowGround(){return GetSortType() == SORT_VERBELOWGROUND;}

	PlaneEnum				GetType(){return GetPlaneParam()->ePlane;}
	cPoint					GetCenter(){return GetPlaneParam()->ptCenter;}
	ObstacleEnum			GetObstacleType(){return GetPlaneParam()->eObstacle;}
	float					GetHeight(){return GetPlaneParam()->fHeight;}
	void					GetSprFile(CString& strSpr);
	int						GetLayer(){return GetPlaneParam()->nLayer;}
	int						GetAniSpeed(){return GetPlaneParam()->nAniSpeed;}
	BOOL					IsAniLoop(){return GetPlaneParam()->bAniLoop;}
	int						m_nFrame;

//res
//	stResourceArray&		GetResArray(){return GetPlaneParam()->stResArray;}
//	int						GetResNum(){return GetPlaneParam()->stResArray.nResource;}
//	stResource&				GetResA(int i){return GetPlaneParam()->stResArray.aResource[i];}
	virtual int				GetResNum();
	virtual stResource&		GetResA(int i);
	int						GetiFrame(int i){return GetResA(i).iFrame;}
//	bool					IsUseTemplate(){ return GetResNum() == 1;}
	HRESULT					OnCommand(long eCommand, stObjParam& param);
	virtual HRESULT			setParam(stObjParam& pm) ;
//utility
	virtual BOOL			SetInGround(BOOL b);
	virtual void			Offset(cPoint pt);
	virtual void			OffsetVB(cPoint ptOffset);

///////////////////////////////////////////////////////////////////
//几何信息
///////////////////////////////////////////////////////////////////
	HRESULT					BeginPainting();
	HRESULT					EndPainting();
//texture
	iTexture*				m_pTexture;
	virtual iTexture*		GetTexture(int i);
	HRESULT					ClearTexture();
	HRESULT					FindTexture();
//vertex 
	stVertexGround			m_aVG[4];
	virtual stVertexGround*	GetVG(int i);
	virtual void			GetSelectVG(stVertexGround v[4], int i);
	void					GetQuadrangle(stVertexGround v[4], int i);
	HRESULT					GetTexInfo(stTexInfo& info, int i);
	virtual HRESULT			UpdateVertexInfo();//will call after change the frame

	//不通过graphics来得到所有的几何信息，
	virtual HRESULT			FillVertexInfoNotCallGraphics() = 0;
	virtual HRESULT			FillTextureUV() = 0;
	virtual HRESULT			FillVertexInfo() = 0;

///////////////////////////////////////////////////////////////////
//paint
///////////////////////////////////////////////////////////////////
//color
	cColor					m_crColorLight;
	cColor					GetLightColor(){return m_crColorLight;}
	cColor					GetFinalColor();
//before Paint
	virtual bool			BeginLighting(cColor cr);
	virtual bool			EndLighting();
	bool					IsLighting(){return m_bLighting;}
	bool					m_bLighting;
	virtual bool			CheckDisplay(const cRect* pRc);
//draw
	virtual HRESULT			Paint();
	virtual HRESULT			Draw();
	virtual HRESULT			DrawNick();
	HRESULT					DrawOffset();
	BOOL					IsObstacleTrace();
	virtual HRESULT			DrawObstacle();
	virtual HRESULT			DrawTrace();
	virtual HRESULT			DrawSelect();
//frame
	void					ChangeDirection(int nDirection);
	void					UpdateFrame();

///////////////////////////////////////////////////////////////////
//utility
///////////////////////////////////////////////////////////////////
	//sort
	virtual cPoint			GetBottomPoint(){return GetOffset();}//得到最底的点	
//得到外框的四个点
	virtual void			GetRectPoint(cPoint p[4]);
	virtual	float			GetArea()=0;//获得面积!
//一般的面之间的关系，例如horbelowground < ver
	virtual ePlane2Plane	PositionPlane(cPlane* p);
//obstacle
	cObstacleList			m_listObstacle;
	cObstacle*				m_pObstacle;
	void					OffsetObstacle(cPoint ptOffset);
	virtual void			UpdateObstacle(cObstacle* p){};	//利用自身的几何信息来生成阻挡
	virtual ObstacleEnum	CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise);
	virtual cObstacle*		GetObstacle(CPoint ptScreen);
	BOOL					BlockObstacle(cPoint pt,ObstacleEnum e);
	void					SetRegion(int x,int y);
//contain
	//包含屏幕的一个点
	virtual BOOL			Contain(POINT pt);
	virtual BOOL			Contain(cPoint pt);
	//包含在一个屏幕矩形中
	BOOL					ContainedScreen(RECT rc);
	//包含在一个地图矩形中
	virtual BOOL			ContainedGround(cRect rc);
	//包含在一个菱形中
	virtual BOOL			ContainedDiamond(cRect rc);
	//中心在那里
	virtual cPoint			GetCenterPosition();

//for mapconvert
	int						MC_GetImageInfo();

	//texinfo;
	stTexInfo				m_info;
};


#endif // !defined(AFX_CPLANE_H__7F34187D_70F1_46E6_8F59_939631B102B3__INCLUDED_)
