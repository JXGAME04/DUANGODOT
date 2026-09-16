// cPlane.cpp: implementation of the cPlane class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cPlane.h"

#include "cGraphics.h"
#include "cGround.h"
#include "cCamera.h"
#include "cLight.h"

#include "iMsg.h"
#include "position.h"
#include "cMesh.h"
#include "cObstacle.h"

#include "iitem.h"
#include "D3DUtil.h"
#include "Font/KFontD3D.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"

#include "mmsystem.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef GET_FILELIST
fileMap g_mapFile;
#endif

cPlane::cPlane(cGraphics* p) : cGroundObj(p)
{
	m_pTexture = NULL;
	m_pObstacle = NULL;
	m_nFrame = 0;
	m_crColorLight = cColor(255,255,255,255);
	m_bDisplay = TRUE;
}

cPlane::~cPlane()
{
	FinalCleanup();
}

void cPlane::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneParam;
	cGroundObj::Init(false);
	stPlaneParam& m_param = *GetPlaneParam();
	m_param.eObstacle = OBSTACLE_NULL;
	m_param.eDynamic = DYNAMIC_STATIC;
	m_param.fHeight = 0.f;
	m_param.ptCenter = cPoint(0,0);
	m_param.nLayer = 0;
	m_param.wTex = 64;
	m_param.hTex = 128;
	m_param.xTex = 0;
	m_param.yTex = 0;
}

///////////////////////////////////////////////////////////////////
//param
///////////////////////////////////////////////////////////////////
ElementEnum	cPlane::GetElementByType()
{
	switch (GetType())
	{
	case PLANE_VERNORMAL:
		return GT_PLANEVERNORMAL;
	case PLANE_VERMESH:
		return GT_PLANEVERMESH;
	case PLANE_VERDYNAMIC:
		return GT_PLANEVERDYNAMIC;
	case PLANE_HORNORMAL:
		return GT_PLANEHORNORMAL;
	case PLANE_HORMESH:
		return GT_PLANEHORMESH;
	}
	return GT_UNKNOW;
}

void cPlane::GetTypeString(CString& s)
{
	CString s1 = "Unknow";
	switch (GetType())
	{
	case PLANE_HORMESH:
		s1 = "HorMesh";
		break;
	case PLANE_HORNORMAL:
		s1 = "Hor";
		break;
	case PLANE_VERMESH:
		s1 = "VerMesh";
		break;
	case PLANE_VERNORMAL:
		s1 = "Ver";
		break;
	case PLANE_VERDYNAMIC:
		s1 = "VerDynamic";
		break;
	}
	CString s2;
	switch (GetSortType())
	{
	case SORT_VERBELOWGROUND:
		s2 = "(低于地表)";
		break;
	case SORT_HORGROUND:
		s2 = "(地表)";
		break;
	case SORT_HORBELOWVER:
		s2 = "(低于垂直)";
		break;
	case SORT_VER:
		s2 = "(垂直)";
		break;
	case SORT_HORABOVEVER:
		s2 = "(高于垂直)";
		break;
	case SORT_HORSKY:
		s2 = "(天空)";
		break;
	}
	s = s1 + s2;
}

HRESULT cPlane::setParam(stObjParam& pm)
{
	HRESULT hr = cGroundObj::setParam(pm);
	if (!pm.IsEMask(MASK_PLANE))
		return hr;

	stPlaneParam& param = *((stPlaneParam* )&pm);
	stPlaneParam& m_param = *GetPlaneParam();

	if (param.IsSetMask(stPlaneParam::MASK_PLANE_ANISPEED))
	{
		m_param.nAniSpeed = param.nAniSpeed;
		m_param.bAniLoop = param.bAniLoop;
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_TYPE))
		m_param.ePlane = param.ePlane;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_CENTER))
	{
		m_param.ptCenter.x = param.ptCenter.x;
		m_param.ptCenter.y = param.ptCenter.y;
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_OBSTACLE))
		m_param.eObstacle = param.eObstacle;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_RESFRAME))
		m_param.stResA.stRes.iFrame = param.stResA.stRes.iFrame;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_RESFILE))
	{
		ClearTexture();
		strcpy(m_param.stResA.stRes.szResource,m_param.stResA.stRes.szResource);
//		FindTexture();
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_RESOURCE))
	{
		ClearTexture();
		m_param.stResA = param.stResA;
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_SORT))
	{
		if (param.eSort >= 0 && param.eSort < SORT_NUM)
			m_param.eSort = param.eSort; //sort 不能动态修改，但是layer可以
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_HEIGHT))
	{
		m_param.fHeight = param.fHeight;
		if (m_param.ptOffset.z == 0)
			m_param.ptOffset.z = param.fHeight;
	}
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_LAYER))
		m_param.nLayer = param.nLayer;
	if (param.IsSetMask(stPlaneParam::MASK_OBJ_MAPEDIT))
	{
		stObjParam param;
		param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_MAPEDIT);
		param.lMapeditEnum = m_param.lMapeditEnum;
		param.eOPMapedit = Bit_Set;
		for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
		{
			cObstacle* p = (*it);
			p->setParam(param);
		}
	}
	return 0;
}

HRESULT cPlane::OnCommand(long eCommand, stObjParam& pm)
{
	cGroundObj::OnCommand(eCommand,pm);

	stPlaneParam& param = *((stPlaneParam* )&pm);
	stPlaneParam& m_param = *GetPlaneParam();
	switch (eCommand)
	{
	case stPlaneParam::COM_PLANE_CHANGEDIRECTION:
		{
			ChangeDirection(param.nDirection);
			break;
		}
	case stPlaneParam::COM_PLANE_GENERATEID:
		{
			CreateID();
			break;
		}
	case stPlaneParam::COM_PLANE_LOADFROMNAME:
		{
			strcpy(m_param.szTemplate,param.szTemplate);
			LoadFromTemplate(GetTemplateFile());
			break;
		}
	case stPlaneParam::COM_PLANE_UPDATEVERTEX:
		{
			UpdateVertexInfo();
			OffsetVB(GetOffset());
			break;
		}
	case stPlaneParam::COM_PLANE_UPDATETEXTURE:
		{
			FindTexture();
			break;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////
//virtual 
///////////////////////////////////////////////////////////////////
void cPlane::OffsetObstacle(cPoint ptOffset)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_OFFSET);
	ptOffset.z = 0;
	param.ptOffset = ptOffset;
//ref is not easy to use
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		p->setParam(param);
	}
}

void cPlane::OffsetVB(cPoint ptOffset)
{
	for (int i=0; i<GetResNum(); i++)
	{
		stVertexGround* aVG = GetVG(i);
		for (int i=0; i<4; i++)
			AddPoint(aVG[i],ptOffset);
	}
	OffsetObstacle(ptOffset);
	cGroundObj::Offset(ptOffset);
}

void cPlane::Offset(cPoint pt)
{
	cPoint ptOffset = pt - GetOffset();
	for (int i=0; i<GetResNum(); i++)
	{
		stVertexGround* aVG = GetVG(i);
		for (int i=0; i<4; i++)
			AddPoint(aVG[i],ptOffset);
	}
	OffsetObstacle(pt);
	cGroundObj::Offset(pt);
}

BOOL cPlane::SetInGround(BOOL b)
{
	cGroundObj::SetInGround(b);
	for_all(m_listObstacle,	bind2nd(mem_fun1<BOOL,cObstacle,BOOL>(&cObstacle::SetInGround),b));
	return b;
}

///////////////////////////////////////////////////////////////////
//几何信息
///////////////////////////////////////////////////////////////////
HRESULT cPlane::GetTexInfo(stTexInfo& info, int iResource)
{
	stPlaneParam& m_param = *GetPlaneParam();

	int index = GetiFrame(iResource);
	iTexture* pTex = GetTexture(iResource);
	if (!pTex)
	{
		ASSERT(0);
		return -1;
	}

	HRESULT hr = GetGround()->SetTexture(pTex,index);
	if (FAILED(hr))
		return hr;
		
	pTex->GetImageSize(index,info.width,info.height);
	m_param.wTex = info.width;
	m_param.hTex = info.height;
	pTex->GetTextureRect(index,info.rcTex);
	if (m_param.ptCenter.x == -1 && m_param.ptCenter.y == -1)
	{
		pTex->GetCenterToUpleft(index,info.x,info.y);
	}
	else
	{
		info.x = m_param.ptCenter.x;
		info.y = m_param.ptCenter.y;
		if (m_param.ptCenter.x == 0 && m_param.ptCenter.y == 0)
		{
			info.x = 0;
			info.y = info.height;
		}
	}
	m_param.xTex = info.x;
	m_param.yTex = info.y;
	return S_OK;
}

HRESULT cPlane::UpdateVertexInfo()
{
	stPlaneParam& m_param = *GetPlaneParam();
	HRESULT hr ;
	if (m_pTexture)
		hr = FillVertexInfo();
	else
	{
		if (m_info.width > 0 && m_info.height > 0)
			hr = FillVertexInfoNotCallGraphics();
		else
		{
			FindTexture();
			hr = FillVertexInfo();
		}
	}
	if (FAILED(hr))
		return hr;
	if (m_pObstacle != NULL)
	{
		UpdateObstacle(m_pObstacle);
	}
	return S_OK;
}

void cPlane::SetRegion(int x,int y)
{
	cGroundObj::SetRegion(x,y);
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		(*it)->SetRegion(x,y);
	}
}
////////////////////////////////////////////////////////////////////
//texture
////////////////////////////////////////////////////////////////////
int cPlane::GetResNum()
{
	return 1;
}

stResource&	cPlane::GetResA(int i)
{
	ASSERT(i < GetResNum());
	return GetPlaneParam()->stResA.stRes;
}

HRESULT	cPlane::FindTexture()
{
	stPlaneParam& m_param = *GetPlaneParam();
	if (m_pTexture == NULL)
		m_pTexture = GetGraphics()->FindTexture(m_param.stResA);
	return S_OK;
}

HRESULT cPlane::ClearTexture()
{
	RELEASE(m_pTexture);
	return S_OK;
}

stVertexGround*	cPlane::GetVG(int i)
{
	return m_aVG;
}

iTexture* cPlane::GetTexture(int i)
{
	if (m_pTexture == NULL)
	{
		FindTexture();
		FillTextureUV();
//		OutputDebugString("find texture when meet plane :");
//		OutputDebugString(GetTemplateFile());
//		OutputDebugString("\n");
	}
	return m_pTexture;
}

////////////////////////////////////////////////////////////////////
//vertex
////////////////////////////////////////////////////////////////////
//draw 顺时针的四边形
void cPlane::GetSelectVG(stVertexGround v[4], int i)
{
	GetQuadrangle(v,i);
}

void cPlane::GetQuadrangle(stVertexGround v[4], int j)
{
	stVertexGround* aVG = GetVG(j);
	for (int i=0; i<4; i++)
		v[i] = aVG[i];
}

///////////////////////////////////////////////////////////////
//color
///////////////////////////////////////////////////////////////
cColor cPlane::GetFinalColor()
{
	//先不考虑偏色
	cColor cr = GetColor();
	cColor crLight = GetLightColor();
	cColor c;
	c.a = cr.a * crLight.a / 255;
	c.r = cr.r * crLight.r / 255;
	c.g = cr.g * crLight.g / 255;
	c.b = cr.b * crLight.b / 255;
	return c;
}

///////////////////////////////////////////////////////////////
//before Paint
///////////////////////////////////////////////////////////////

bool cPlane::CheckDisplay(const cRect* pRc)
{
	m_bDisplay = false;
	const stVertexGround* v = GetVG(0);
	for (int i=0; i<4; i++)
	{
		if (IsVertexInRectDisplay(v[i],*pRc))
		{
			m_bDisplay = true;
			break;
		}
	}
	return m_bDisplay;
}

void cPlane::UpdateFrame()
{
	int time = timeGetTime();
	if (m_nFrame == 0)
	{
		m_nFrame = time;
		return;
	}

	ASSERT(m_pTexture);
	int nFrames = m_pTexture->GetFrames();
	
	int nDirections = m_pTexture->GetDirections();
	int nFramesOfDirections = nFrames / nDirections;

	int iDirection = GetResA(0).iFrame / nFramesOfDirections;
	int iFrameOfDirection = GetResA(0).iFrame % nFramesOfDirections;

	if (!IsAniLoop() && iFrameOfDirection == nFramesOfDirections-1)
		return;

	int nAdd = (time - m_nFrame) / GetAniSpeed();
	//increase the frame!
	//要求动画的大小一致

	iFrameOfDirection += nAdd;
	if (iFrameOfDirection >= nFramesOfDirections-1)
	{
		if (IsAniLoop())
			iFrameOfDirection = iFrameOfDirection % nFramesOfDirections;
		else
			iFrameOfDirection = nFramesOfDirections-1;
	}
	m_nFrame += GetAniSpeed() * nAdd;
	GetResA(0).iFrame = iDirection * nFramesOfDirections + iFrameOfDirection;

	stPlaneParam param;
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	setParam(param);
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	setParam(param);
}

void cPlane::ChangeDirection(int nDirection)
{
	if (!m_pTexture)
		return;
//	ASSERT(m_pTexture);

	int nFrames = m_pTexture->GetFrames();
	int nDirections = m_pTexture->GetDirections();
	if (nDirection >= nDirections)
		return;

	int nFramesOfDirections = nFrames / nDirections;
//	int iDirection = GetResA(0).iFrame / nFramesOfDirections;
	int iFrameOfDirection = GetResA(0).iFrame % nFramesOfDirections;
	
	GetResA(0).iFrame = nDirection * nFramesOfDirections + iFrameOfDirection;

	stPlaneParam param;
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	setParam(param);
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	setParam(param);
}

bool cPlane::BeginLighting(cColor cr)
{
	m_bLighting = false;
	m_crColorLight = cr;

	if (GetAniSpeed() > 0)
		UpdateFrame();
	//increase the frame of the plane
	return true;
}

bool cPlane::EndLighting()
{
	return true;
}

///////////////////////////////////////////////////////////////
//Paint
///////////////////////////////////////////////////////////////
HRESULT cPlane::Paint()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
	Draw();
	DrawObstacle();
	DrawNick();
	DrawSelect();
	DrawTrace();
	return S_OK;
}

HRESULT	cPlane::DrawTrace()
{
	stPlaneParam& m_param = *GetPlaneParam();
	if (GetMapedit() & Mapedit_Trace)
	{
		char sMsg[128];
		CString sType;
		GetTypeString(sType);
		cPoint ptCenter = GetCenterPosition();
		cPoint ptRegion = ptCenter;
		GroundToTile(ptRegion);
		TileToRegion(ptRegion);
/*
		sprintf(sMsg,"(F6)Plane Type(%s) Offset(%.1f,%.1f,%.1f) Center(%.1f,%.1f,%.1f) Region(%d,%d)",
			sType,
			GetOffset().x,GetOffset().y,GetOffset().z,
			ptCenter.x,ptCenter.y,ptCenter.z,
			(int)ptRegion.x,(int)ptRegion.y);
*/
		sprintf(sMsg,"(F6)Plane Type(%s) Offset(%.1f,%.1f,%.1f) Kind(%d) Region(%d,%d)",
			sType,
			GetOffset().x,GetOffset().y,GetOffset().z,
			GetKind(),
			(int)ptRegion.x,(int)ptRegion.y);
		GetGraphics()->Trace(sMsg);
		if (strlen(m_param.szGroup) != 0)
		{
			sprintf(sMsg,"(F6)Plane Grp(%s)",
				m_param.szGroup);
			GetGraphics()->Trace(sMsg);
		}
		sprintf(sMsg,"(F6)Plane Tmp(%s(%d))",
			m_param.szTemplate,m_param.iTemplate);
		GetGraphics()->Trace(sMsg);
		sprintf(sMsg,"(F6)Plane Spr(%s(%d))",
			m_param.stResA.stRes.szResource,m_param.stResA.stRes.iFrame);
		GetGraphics()->Trace(sMsg);
	}	
	return S_OK;
}

HRESULT cPlane::Draw()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;

	stPlaneParam& m_param = *GetPlaneParam();
	DWORD value = GetFinalColor().Value();
	stVertexGround* aVG = GetVG(0);
	for (int i=0; i<4; i++)
		aVG[i].color = value;
	GetGround()->SetTexture(GetTexture(0),GetiFrame(0));
	GetGround()->DrawQuadrangle(aVG);

	return S_OK;
}

HRESULT cPlane::DrawOffset()
{
	cPoint pt = GetOffset();
	GetGround()->DrawPoint(pt,cColor(255,0,0));
	return S_OK;
}

HRESULT cPlane::DrawNick()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
//	if (!(GetMapedit() & Mapedit_Select))
//		return S_OK;
	if (strlen(GetEvent()) == 0 || IsStatic())
		return S_OK;
	CString s;
	s.Format("%s(%d)",GetEvent(),GetID());
	cPoint pt = GetOffset();
	GetCamera()->GroundToScreen(pt);
	GetGraphics()->GetDebugFont()->TextOut(s,s.GetLength(),pt.x, pt.y,0xff00ffff);
	return S_OK;
}

HRESULT cPlane::DrawSelect()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
	if (GetMapedit() & Mapedit_Select)
		GetGround()->DrawRect(GetVG(0),cColor(0,255,255));
	return S_OK;
}

BOOL cPlane::IsObstacleTrace()
{
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		if (p->GetMapedit() & Mapedit_Trace)
			return TRUE;
	}
	return FALSE;
}

HRESULT cPlane::DrawObstacle()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
	for_all(m_listObstacle,mem_fun<HRESULT,cObstacle>(&cObstacle::Draw));
	if (IsObstacleTrace())
	{
		stPlaneParam& m_param = *GetPlaneParam();
		BitOP(m_param.lMapeditEnum,Bit_Add,Mapedit_Trace);
		DrawTrace();
		BitOP(m_param.lMapeditEnum,Bit_Remove,Mapedit_Trace);
	}
	return S_OK;
}

//device
HRESULT cPlane::FinalCleanup()
{
	ClearTexture();
	for_all(m_listObstacle,mem_fun<ULONG,cObstacle>(&cObstacle::Release));
	m_listObstacle.clear();
	m_pObstacle = NULL;
	return S_OK;
}

////////////////////////////////////////////////////////////////////
//load and save
////////////////////////////////////////////////////////////////////

HRESULT cPlane::Load(const stSection* p)
{
	cGroundObj::Load(p);

	stPlaneParam& m_param = *GetPlaneParam();
	if (p != NULL)
	{
		GetSectionValue(p,"layer",m_param.nLayer);
		GetSectionValue(p,"iframe",GetResA(0).iFrame);
		GetSectionValue(p,"texWidth",m_param.wTex);
		GetSectionValue(p,"texHeight",m_param.hTex);
		GetSectionValue(p,"texX",m_param.xTex);
		GetSectionValue(p,"texY",m_param.yTex);
#pragma message (ATTENTION("保持兼容"))
		CString s = m_param.szTemplate;
		if (s.Find("group.txt") != -1)
			return -1;
	}
#ifdef GET_FILELIST
	g_mapFile[m_param.szTemplate] = 0;
#endif
	return LoadFromTemplate(m_param.szTemplate);
}

HRESULT cPlane::Save(stSection* p)
{
	cGroundObj::Save(p);

	ASSERT(p);

	stPlaneParam& m_param = *GetPlaneParam();
	SetSectionValue(p,"layer",m_param.nLayer);
	SetSectionValue(p,"iframe",GetResA(0).iFrame);
	SetSectionValue(p,"texWidth",m_param.wTex);
	SetSectionValue(p,"texHeight",m_param.hTex);
	SetSectionValue(p,"texX",m_param.xTex);
	SetSectionValue(p,"texY",m_param.yTex);

	return 0;
}

HRESULT	cPlane::SaveObstalce(iItem* pItem, int& num)
{
	if (m_listObstacle.size() == 0)
		return S_OK;
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		stSection* pSection = pItem->GetIndexSection(num++,FALSE);
		p->Save(pSection);
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////////
//painting
////////////////////////////////////////////////////////////////////

HRESULT cPlane::BeginPainting()
{
	if (m_pTexture == NULL)
	{
		FindTexture();
		HRESULT hr = UpdateVertexInfo();
		OffsetVB(GetOffset());
	}
	return S_OK;
}

HRESULT cPlane::EndPainting()
{
	ClearTexture();
	return S_OK;
}

////////////////////////////////////////////////////////////////////
//utility
////////////////////////////////////////////////////////////////////
cPoint cPlane::GetCenterPosition()
{
	cPoint p[4];
	GetRectPoint(p);
	cPoint pt = cPoint(0,0,0);
	for (int i=0; i<4; i++)
	{
		pt.x += p[i].x;
		pt.y += p[i].y;
		pt.z += p[i].z;
	}
	pt.x /= 4;
	pt.y /= 4;
	pt.z /= 4;
	pt.y -= pt.z * c_fSqrt3;
	return pt;
}

void cPlane::GetSprFile(CString& sValue)
{	
	sValue = GetResA(0).szResource;
}


////////////////////////////////////////////////////////////////////
//load from name
//static only
////////////////////////////////////////////////////////////////////
HRESULT cPlane::LoadFromTemplate(LPCSTR szFile)
{
	//检验该文件是否存在
	stTemplateOne* pTemplate = (stTemplateOne* )Common_GetPack1D(Pack1D_Plane)->LocateToFrame(szFile,GetiFrame(0));
	if (pTemplate == NULL)
		return -1;
	return LoadFromTemplate(pTemplate);
/*
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return -1;
	wrap_ptr<iItem> ptr(pItem);
	return LoadFromTemplate(pItem);
*/
}

HRESULT	cPlane::LoadFromTemplate(stTemplateOne* pTemplate)
{
	HRESULT hr;
	hr = SetPlaneProperty(pTemplate);
	if (FAILED(hr))
		return hr;
	hr = SetPlaneResource(pTemplate);
	if (FAILED(hr))
		return hr;
	if (GetGround() && GetGround()->IsCreateObstacle())
		hr = SetPlaneObstacle(pTemplate);
	if (FAILED(hr))
		return hr;
	hr = SetPlaneSave(pTemplate);
	if (FAILED(hr))
		return hr;
	return S_OK;
}

HRESULT cPlane::SetPlaneResource(stTemplateOne* pTemplate)
{
	stPlaneParam param;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_RESOURCE);		
	stResourceA& stResA = param.stResA;
	CString sValue = GetTemplateFile();
	sValue = sValue.Left(sValue.GetLength() - 4) + ".spr";		
	AppGetShortPath(sValue);
	stResA.stRes.iFrame = GetiFrame(0);
	strcpy(stResA.stRes.szResource,sValue);
	stResA.stDevice.format = D3DFMT_A4R4G4B4;
	stResA.stDevice.pool = D3DPOOL_MANAGED;
	stResA.stDevice.eMedia = MEDIA_VIDEO;
	stResA.stDevice.eUsage = USAGE_STATIC;
	setParam(param);

	int n= 0;
	m_info.width = pTemplate->nSprWidth;
	m_info.height = pTemplate->nSprHeight;
	m_info.x = pTemplate->nSprCenterX;
	m_info.y = pTemplate->nSprCenterY;
	m_info.index = GetiFrame(0);
	m_info.rcTex = cRect(0,0,1,1);

	stPlaneParam& m_param = *GetPlaneParam();
	if (m_param.ptCenter.x == -1 && m_param.ptCenter.y == -1)
	{
	}
	else
	{
		m_info.x = m_param.ptCenter.x;
		m_info.y = m_param.ptCenter.y;
		if (m_param.ptCenter.x == 0 && m_param.ptCenter.y == 0)
		{
			m_info.x = 0;
			m_info.y = m_info.height;
		}
	}

	return S_OK;
}

HRESULT cPlane::SetPlaneProperty(stTemplateOne* pTemplate)
{
	stPlaneParam& param = *GetPlaneParam();
	CString strValue;
	param.ePlane = (PlaneEnum)pTemplate->ePlaneType;
	param.eSort = (SortEnum)pTemplate->eSort;
	param.ptCenter.x = pTemplate->nCenterX;
	param.ptCenter.y = pTemplate->nCenterY;
	param.fHeight = pTemplate->fHeight;
	param.nAniSpeed = pTemplate->nAnispeed;
	param.bAniLoop = pTemplate->bAniLoop;
	return S_OK;
}

HRESULT cPlane::SetPlaneObstacle(stTemplateOne* pTemplate)
{
	stPlaneParam& m_param = *GetPlaneParam();
	m_param.eObstacle  = (ObstacleEnum)pTemplate->eObstacleType;
	if (m_param.eObstacle != OBSTACLE_NULL)
	{
		cObstacle* p;
		GetGraphics()->CreateGraphicsElement(GT_OBSTACLE,&p);
		p->SetChild(TRUE);
		stObstacleParam param;
		param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_TYPE);
		param.eObstacle = m_param.eObstacle;
		p->setParam(param);
		m_pObstacle = p;
		m_listObstacle.push_back(p);
	}

	int nObstacle = pTemplate->nObstacleNum;
	ASSERT(nObstacle == pTemplate->nObstacleNum);
	if (nObstacle > 0)	
	{
		stTemplateObstacle* pObstacle = (stTemplateObstacle*)++pTemplate;
		for (int i=0; i<nObstacle; i++)
		{
			cObstacle* p;
			GetGraphics()->CreateGraphicsElement(GT_OBSTACLE,&p);
			p->SetChild(TRUE);

			stObstacleParam param;
			param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO | stObstacleParam::MASK_OBSTACLE_TYPE);
			param.eObstacle = (ObstacleEnum)pObstacle->eObstacleType;
			param.nPoint = pObstacle->eObstacleNum;
			ASSERT(param.nPoint > 0);
			for (int j=0; j<param.nPoint; j++)
			{
				param.aPoint[j] = pObstacle->aPoint[j];
			}
			p->setParam(param);
			param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_KIND);
			param.eKind = (KindEnum)pObstacle->eObstacleKind;
			p->setParam(param);

			m_listObstacle.push_back(p);

			pObstacle++;
		}
	}

	return S_OK;
}

HRESULT cPlane::SetPlaneSave(stTemplateOne* pTemplate)
{
	stPlaneParam& param = *GetPlaneParam();
	param.eDynamic = (DynamicEnum)pTemplate->eDynamicType;
	param.eKind = (KindEnum)pTemplate->eKind;
	return S_OK;
}

////////////////////////////////////////////////////////////////////
//contain
////////////////////////////////////////////////////////////////////
ePlane2Plane cPlane::PositionPlane(cPlane* p)
{
	if (GetSortType() < p->GetSortType())
		return PP_BACK;
	if (GetSortType() > p->GetSortType())
		return PP_FRONT;
	return PP_UNKNOW;
}

BOOL cPlane::Contain(POINT ptScreen)
{
	if (!IsShow())
	{
//		AfxMessageBox("it is can not select because it is not show!");
		return false;
	}

	cPoint pt(ptScreen.x,ptScreen.y);

	for (int j=0; j<GetResNum(); j++)
	{
		stVertexGround v[4];
		GetSelectVG(v,j);
		//it is a 四边形 not a 矩形
		cPoint point[4];
		for (int i=0; i<4; i++)
		{
			GetCamera()->GroundToScreen(v[i],v[i]);
			point[i] = GetPoint(v[i]);
		}
		if (IsPointInQuadrangle(pt,point))
			return true;
	}
	return false;
}

BOOL cPlane::Contain(cPoint ptGround)
{
	return false;
}

BOOL cPlane::ContainedGround(cRect rc)
{
	#pragma message (ATTENTION("HRERE!"))
	return FALSE;
}

BOOL cPlane::ContainedDiamond(cRect rc)
{
	#pragma message (ATTENTION("HRERE!"))
	return FALSE;
}

BOOL cPlane::ContainedScreen(RECT rc)
{
	stVertexGround v[4];
	for (int j=0; j<GetResNum(); j++)
	{
		GetQuadrangle(v,j);
		for (int i=0; i<4; i++)
		{
			GetCamera()->GroundToScreen(v[i],v[i]);
			if (!IsVertexInRect(v[i],rc))
				return false;
		}
	}
	return true;
}

void cPlane::GetRectPoint(cPoint p[4])
{
	for (int i=0; i<4; i++)
		p[i] = GetPoint(GetVG(0)[i]);
}

HRESULT	cPlane::LoadPack(void** pParam)
{
	HRESULT hr = cGroundObj::LoadPack(pParam);
	if (FAILED(hr))
		return hr;
	return LoadFromTemplate(GetTemplateFile());
}

HRESULT	cPlane::LoadBinary(CFile& f)
{
	HRESULT hr = cGroundObj::LoadBinary(f);
	if (FAILED(hr))
		return hr;
	#pragma message (ATTENTION("兼容旧的错误版本"))
	f.Read(m_aVG,sizeof(stVertexGround)*4);	
	int n;
	f.Read(&n,sizeof(n));
	for (int i=0; i<n; i++)
	{
		cObstacle* p;
		GetGraphics()->CreateGraphicsElement(GT_OBSTACLE,&p);
		hr = p->LoadBinary(f);
		if (FAILED(hr))
			return hr;
		m_listObstacle.push_back(p);
	}
	if (GetObstacleType() != OBSTACLE_NULL)
		m_pObstacle = m_listObstacle.back();
//	ClearTexture();
//	FindTexture();
	return S_OK;
}

HRESULT	cPlane::SaveBinary(CFile& f)
{
	cObj::SaveBinary(f);
	f.Write(m_aVG,sizeof(stVertexGround)*4);	
	int n = m_listObstacle.size();
	f.Write(&n,sizeof(n));
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		(*it)->SaveBinary(f);
	}
	return S_OK;
}


///////////////////////////////////////////////////////////////////
//obstacle
///////////////////////////////////////////////////////////////////
cObstacle*	cPlane::GetObstacle(CPoint ptScreen)
{
	itcObstacle it = find_if(m_listObstacle.begin(),m_listObstacle.end(),
		bind2nd(mem_fun1<BOOL,cObstacle,POINT>(&cObstacle::Contain),ptScreen));
	if (it == m_listObstacle.end())
		return NULL;
	return (*it);
}

BOOL cPlane::BlockObstacle(cPoint pt,ObstacleEnum e)
{
	for (itcObstacle it = m_listObstacle.begin(); it!= m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		if (p->BlockObstacle(pt,e))
			return TRUE;
	}
	return FALSE;
}

ObstacleEnum cPlane::CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise)
{
	for (itcObstacle it = m_listObstacle.begin(); it!= m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		ObstacleEnum e = p->CheckObstacle(pt,vSpeed,bClockWise);
		if (e != OBSTACLE_NULL)
			return e;
	}
	return OBSTACLE_NULL;
}

/////////////////////////////////////////////////////////
//list function
/////////////////////////////////////////////////////////
#include "iImage.h"
int cPlane::MC_GetImageInfo()
{
	stPlaneParam& m_param = *GetPlaneParam();
	iImage* pImage = Image_CreateImage(m_param.stResA.stRes.szResource);
	pImage->LoadFile(m_param.stResA.stRes.szResource);
	stFrameInfo info;
	pImage->GetFrameInfo(info,m_param.stResA.stRes.iFrame);m_param.hTex = info.height;
	if (m_param.ptCenter.x == -1 && m_param.ptCenter.y == -1)
	{
		m_param.xTex = info.xCenterToUpleft;
		m_param.yTex = info.yCenterToUpleft;
	}
	else
	{
		m_param.xTex = m_param.ptCenter.x;
		m_param.yTex = m_param.ptCenter.y;
		if (m_param.ptCenter.x == 0 && m_param.ptCenter.y == 0)
		{
			m_param.xTex = 0;
			m_param.yTex = info.height;
		}
	}
	pImage->Release();
	return 0;	
}
