// cGraphics.cpp: implementation of the cGraphics class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cGraphics.h"

#include "cGround.h"
#include "cScreen.h"
#include "cCamera.h"
#include "cThread.h"

//#include "cFont.h"
#include "cPicture.h"
#include "cMap.h"
#include "cMapSmall.h"

#include "cPlaneVerNormal.h"
#include "cPlaneVerMesh.h"
#include "cPlaneVerDynamic.h"
#include "cPlaneHor.h"
#include "cLight.h"
#include "cSelect.h"
#include "cRegion.h"
#include "cObstacle.h"

#include "cSpecialScroll.h"
#include "cSpecialSnow.h"
#include "cSpecialFirefly.h"
#include "cSpecialSign.h"

#include "cTextureCache.h"

#include "iFps.h"
#include "iMsg.h"
#include <Mmsystem.h>
#include <math.h>
#include "Font/KFontD3D.h"
#include "cLock.h"

#include "cGround3D.h"
#include "iTemplate.h"

#include "planedoodad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//UINT Thread( LPVOID pParam );

cGraphics::cGraphics()
{
	m_ulCount = 1;
	ZeroMemory(&m_param,sizeof(m_param));
	m_param.lShowEnum = GS_ALL & ~GS_OBSTACLE;

	m_pThread = NULL;
	
	m_nFlipTicks = 0;
	m_pDebugFont = NULL;

	m_pScreen = NULL;
	m_pGround = NULL;
	m_pCamera = NULL;
	m_pMsgQueue = NULL;

//	BitOP(m_param.lTraceEnum,Bit_Add,GS_VER);
	m_param.lTraceEnum = 0;
	m_pFps = NULL;
	m_nFaces = 0;

	for (int i=0; i<USAGE_NUM; i++)
		m_aTextureCache[i] = NULL;
}

cGraphics::~cGraphics()
{
}

/**************************************************************************/
//com interface
/**************************************************************************/
ULONG cGraphics::AddRef() 
{
	return ++m_ulCount;
}

ULONG cGraphics::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}


/**************************************************************************/
//param 
/**************************************************************************/
int cGraphics::GetFps()
{
	if (m_pFps == NULL)
		return 0;
	return m_pFps->GetNowFps();
}

HRESULT cGraphics::GetParam(stParam& param) 
{
	if (param.mask == 0)
		param = m_param; 
	return 0;
};

HRESULT cGraphics::SetParam(stParam& param, bool bRunNow)
{
	if (bRunNow)
		return setParam(param);

	AddRef();
	param.wParam = (long)this;
	param.lParam = 0;
	iMsg* pMsg = Message_CreateMsg(Msg_Graphics);
	pMsg->SetData(&param,sizeof(param),GT_GRAPHICS);
	m_pMsgQueue->PushMsg(pMsg);
	return 0;
}

HRESULT cGraphics::setParam(const stParam& param)
{
	if (param.mask == MASK_COMMAND)
	{
		switch (param.eCommand)
		{
		case GCOM_INIT:
			{
				HRESULT hr = CreateDevice(param);
				return hr;
			}
		}
	}
	if (param.mask & MASK_FULLSCREEN)
	{
		if (param.bFullScreen != m_param.bFullScreen)
		{
			HRESULT hr = ToggleFullscreen();
			m_param.bFullScreen = param.bFullScreen;
			DX_ERROR(hr);
		}
	}
	if (param.mask & MASK_PATH)
		strcpy(m_param.szPath,param.szPath);
	if (param.mask & MASK_NAME)
		strcpy(m_param.szName,param.szName);
	if (param.mask & MASK_MOUSE)
		m_param.ptMouse = param.ptMouse;
	if (param.mask & MASK_GAMELOOP)
		m_param.nGameLoop = param.nGameLoop;
	if (param.mask & MASK_PAUSEPAINT)
		m_param.bPausePaint = param.bPausePaint;	
	if (param.mask & MASK_SHOW)
	{
		BOOL b = m_param.lShowEnum & GS_TERRAIN;
		BitOP(m_param.lShowEnum,param.eOPShow,param.lShowEnum);
		if (b != (m_param.lShowEnum & GS_TERRAIN))
		{
			GetcGround()->RepaintTerrain();
		}
	}
	if (param.mask & MASK_TRACE)
		BitOP(m_param.lTraceEnum,param.eOPTrace,param.lTraceEnum);
	return 0;
}

BOOL cGraphics::IsShow(ShowEnum e)
{
	return m_param.lShowEnum&e;
}

/**************************************************************************/
//Trace and help
/**************************************************************************/
void cGraphics::Trace(LPCSTR str)
{
	CString s = str;
	m_listTrace.AddTail(s);
}

void cGraphics::PrintTrace()
{
	int m_nLine = 0;
	for (POSITION pos = m_listTrace.GetHeadPosition(); pos!=NULL; )
	{
		CString s = m_listTrace.GetNext(pos);
		PrintTrace(s,m_nLine++);
	}
	m_listTrace.RemoveAll();
	return ;
}

void cGraphics::PrintTrace(LPCSTR sz, int nLine)
{
//	iFont::stParam param;
	cColor color[13] = 
	{
		cColor(255,255,255,255),
		cColor(127,255,255,255),cColor(255,127,255,255),cColor(255,255,127,255),
		cColor(255,127,127,255),cColor(127,255,127,255),cColor(127,127,255,255),
		cColor(255,127,0.f,255),cColor(255,0.f,127,255),cColor(127,255,0.f,255),cColor(0.f,255,127,255),cColor(127,0.f,255,255),cColor(0.f,127,255,255),
	};
//	param.crColor = color[nLine % 13];
//	param.lFlag = 0;
//	param.ptPos = cPoint(2,1 + nLine*16 );
	//m_pDebugFont->TextOut(param,s,strlen(s));	
	if (sz && m_pDebugFont)
		m_pDebugFont->TextOut(sz, strlen(sz), 2, 1 + nLine*16, color[nLine % 13].Value());
}

/**************************************************************************/
//begin and end
/**************************************************************************/
HRESULT cGraphics::Begin()
{
//Create texturebuffer
	InitializeCriticalSection(&m_csTextureCache);

	Common_GetPack1D(Pack1D_Plane)->Open(FILE_TEMPLATE_ZIP,FILE_TEMPLATE_INDEX);

	//							静态图片	菜单界面	人物动画	场景动画	公共资源	特效动画	临时生成的
	static int aSprSize[] = {	48,				4,			64,			8,			2,			2,			0};
	static int aMemorySize[] = {0,				0,			0,			0,			0,			2,			24};
	static int aVideoSize[] = {	24,				2,			24,			2,			2,			2,			12};//(4*5/2)
	static int aVideoOneSize[] = {0,			0,			24,			0,			0,			0,			0};//(4*5/2)
	static int aOtherSize[] = {	0,				4,			0,			2,			0,			0,			0};//(4*5/2)
	static CString aResourceName[] = {"VideoRes","MemoryRes","SprRes",""};
	static CString aTextureName[] = {"静态图片","菜单界面","人物动画","场景动画","公共资源","特效动画","临时生成",""};
 
	int* aSize[3];
	aSize[0] = aSprSize;
	aSize[1] = aMemorySize;
	aSize[2] = aVideoSize;

	CString strName;
	for (int i=0; i<USAGE_NUM; i++)
	{
		m_aTextureCache[i] = new cTextureCache;
		m_aTextureCache[i]->Create(this,(UsageEnum)i);
		m_aTextureCache[i]->SetName(aTextureName[i]);
		m_aTextureCache[i]->SetTotalSize(MEDIA_SPR,aSprSize[i]*1024*1024);
		m_aTextureCache[i]->SetTotalSize(MEDIA_VIDEO,aVideoSize[i]*1024*1024);
		m_aTextureCache[i]->SetTotalSize(MEDIA_VIDEOONE,aVideoOneSize[i]*1024*1024);
		m_aTextureCache[i]->SetTotalSize(MEDIA_MEMORY,aMemorySize[i]*1024*1024);
		m_aTextureCache[i]->SetTotalSize(MEDIA_VIDEOOTHER,aOtherSize[i]*1024*1024);
	}
	m_aTextureCache[USAGE_ANIMATION_ACTOR]->SetDownloadFrame(TRUE);

//Create element
	CreateGraphicsElement(GT_GROUND,&m_pGround);
	CreateGraphicsElement(GT_SCREEN,&m_pScreen);
	CreateGraphicsElement(GT_CAMERA,&m_pCamera);
	CreateGraphicsElement(GT_FONT,&m_pDebugFont);
//	m_pDebugFont->Create( _T("Arial"), 12, iFont::D3DFONT_BOLD );
	
	m_pMsgQueue = Message_CreateMsgQueue(MsgQueue_Graphics);

	if (m_pGround)
		m_pGround->Begin();
	if (m_pScreen)
		m_pScreen->Begin();
	m_pCamera->Begin();

	m_pFps = Common_CreateFps();
	m_pFps->Init();
	return S_OK;
}


HRESULT cGraphics::End()
{
	if (m_pGround)
		m_pGround->End();
	if (m_pScreen)
		m_pScreen->End();
	m_pCamera->End();

	CleanDevice();

	RELEASE(m_pDebugFont);
	RELEASE(m_pScreen);
	RELEASE(m_pGround);
	RELEASE(m_pCamera);

	RELEASE(m_pMsgQueue);

	for (int i=0; i<USAGE_NUM; i++)
	{
		delete (m_aTextureCache[i]);
		m_aTextureCache[i] = NULL;
	}

	DeleteCriticalSection(&m_csTextureCache);

	m_pFps->Release();

	return S_OK;	
}

HRESULT cGraphics::Cleanup()
{
	HRESULT hr = 0;
	if (m_pGround)
		hr = m_pGround->FinalCleanup();
	if (m_pScreen)
		hr = m_pScreen->FinalCleanup();
	for (int j=0; j<USAGE_NUM; j++)
	{
		m_aTextureCache[j]->FinalCleanup();
	}

	return hr;	
}

/**************************************************************************/
//thread 
/**************************************************************************/
HRESULT cGraphics::RunThread() 
{
	ASSERT(m_pThread == NULL);
	m_pThread = new cThread;
	m_pThread->RunThread(Thread,this,THREAD_PRIORITY_IDLE);	
	return S_OK;
}

HRESULT cGraphics::EndThread() 
{
	ASSERT(m_pThread);
	m_pThread->EndThread();	
	delete m_pThread;
	
	return S_OK;
}

/**************************************************************************/
//graphics element!
/**************************************************************************/
iCamera* cGraphics::GetiCamera()
{
	return GetcCamera();
}

iGround* cGraphics::GetiGround() 
{
	return GetcGround();
}

iScreen* cGraphics::GetiScreen()
{
	return GetcScreen();
}

/**************************************************************************/
//message Queue
/**************************************************************************/
ULONG cGraphics::PushMessage(void* buffer,ULONG size,ElementEnum eMsg)
{
	ASSERT(0);
	iMsg* pMsg = Message_CreateMsg(Msg_Graphics);
	pMsg->SetData(buffer,size,eMsg);
	m_pMsgQueue->PushMsg(pMsg);
	return S_OK;
}

#define Set_Param(class) class::stParam* p = (class::stParam*) pMsg->GetData(); \
						class* pClass = (class*) p->wParam; \
						pClass->setParam(*p);\
						pClass->Release();

ULONG cGraphics::HandleMessage()
{
	iMsg* pMsg;
	while(pMsg = m_pMsgQueue->PopMsg())
	{
		ASSERT(0);
		switch(pMsg->GetType())
		{
		case GT_GRAPHICS:
			{
				Set_Param(cGraphics);
				break;
			}
		case GT_CAMERA:
			{
				Set_Param(cCamera);
				break;
			}
		case GT_GROUND:
			{
				Set_Param(cGround);
				break;
			}
		case GT_SCREEN:
			{
				Set_Param(cScreen);
				break;
			}
		default:
			{
				stObjParam* p = (stObjParam*)pMsg->GetData();
				iObj* pObj= (iObj*) p->wParam; 
				pObj->setParam(*p);
				pObj->Release();
				break;
			}
		}
		pMsg->Release();
		pMsg = NULL;
	}
	return 0;
}

/**************************************************************************/
//texture and image cache
/**************************************************************************/
HRESULT cGraphics::RefreshSpr(LPCSTR szSpr, bool bRunNow)
{
	stResourceA res;
	res.stDevice.eMedia = MEDIA_SPR;
	res.stDevice.eUsage = USAGE_STATIC;
	res.stRes.idResource = 0;
	res.stRes.iFrame = 0;
	strcpy(res.stRes.szResource,szSpr);
	iTexture* p = FindTexture(res);
	if (p)
		p->Refresh();

	res.stDevice.eMedia = MEDIA_VIDEO;
	res.stDevice.eUsage = USAGE_STATIC;
	res.stRes.idResource = 0;
	res.stRes.iFrame = 0;
	strcpy(res.stRes.szResource,szSpr);
	p = FindTexture(res);
	if (p)
		p->Refresh();

	CString strTxt = szSpr;
	strTxt = strTxt.Left(strTxt.GetLength() - 3) +"txt";
	Common_GetPack1D(Pack1D_Plane)->Refresh(strTxt);

	return 0;
}

HRESULT cGraphics::SetCommonTexture(eTexKind kind, int frame)
{
	ASSERT(kind < TEX_NUM);
	iTexture* pTexture = FindCommonTexture(kind);

	ASSERT(pTexture);
	HRESULT hr = GetDevice3D()->SetTexture(0,pTexture->GetD3DTexture(frame));
	ASSERT(!FAILED(hr));
	pTexture->Release();
	return hr;
}


iTexture* cGraphics::FindCommonTexture(eTexKind kind)
{
	ASSERT(kind < TEX_NUM);

	static CString file[] = 
	{
		"system\\spr\\White.spr",
		"system\\spr\\select.spr",
		"system\\spr\\grid.spr",
		"",
	};

	stResourceA res;
	res.stDevice.eMedia = MEDIA_VIDEO;
	res.stDevice.eUsage = USAGE_COMMON;
	res.stRes.idResource = kind + RESOURCE_GRAPHICS_START;
	res.stRes.iFrame = 0;
	strcpy(res.stRes.szResource,file[kind]);
	return FindTexture(res,true);
}

iTexture* cGraphics::FindSprResource(LPCSTR szSprFile)
{
	stResourceA res;
	res.stDevice.eUsage = USAGE_STATIC;
	res.stDevice.eMedia = MEDIA_SPR;
	res.stRes.idResource = 0;
	res.stRes.iFrame = 0;
	strcpy(res.stRes.szResource,szSprFile);
	return FindTexture(res,true);
}

HRESULT cGraphics::ClearTexture(const stResourceA& resPos)
{
	cLock lock(&m_csTextureCache);
	m_aTextureCache[resPos.stDevice.eUsage]->FinalCleanup();
	return S_OK;
}

iTexture* cGraphics::FindTexture(const stResourceA& resPos, bool AutoCreate )
{
	cLock lock(&m_csTextureCache);
	iTexture* p = m_aTextureCache[resPos.stDevice.eUsage]->FindTexture(resPos,AutoCreate);
	return (p);
}


iTexture* cGraphics::CreateiTexture(const stResourceA& resPos, int width, int height)
{
	ASSERT(resPos.stDevice.eUsage == USAGE_REGION);
	iTexture *p = NULL;
	switch (resPos.stDevice.eMedia)
	{
	case MEDIA_MEMORY:
		{
			CreateGraphicsElement(GT_TEXTUREMEMORY,&p);
			p->Create(resPos,width,height);
		}
		break;
	default:
		ASSERT(0);
	}
	
	return p;		
}

void cGraphics::AddTexture(const stResourceA& resPos,iTexture* p)
{
	cLock lock(&m_csTextureCache);
	m_aTextureCache[resPos.stDevice.eUsage]->AddTextureOut(resPos,p);
}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/


/**************************************************************************/
//paint loop
/**************************************************************************/
HRESULT cGraphics::BeginPaint()
{
	return S_OK;
}

#define CUSTOMVERTEX stVertexGround
#define D3DFVF_CUSTOMVERTEX cGround3D::D3DFVF_GROUNDVERTEX
LPDIRECT3DVERTEXBUFFER8 g_pVB        = NULL; // Buffer to hold vertices
  
HRESULT InitTest(LPDIRECT3DDEVICE8 g_pd3dDevice)
{
	LPDIRECT3DTEXTURE8      g_pTexture   = NULL; // Our texture
	HRESULT hr = 0;
    if( FAILED( D3DXCreateTextureFromFile( g_pd3dDevice, "d:\\texture.bmp",
                                           &g_pTexture ) ) )
        return E_FAIL;

	hr = g_pd3dDevice->SetTexture(0,g_pTexture);
	ASSERT(!FAILED(hr));
	if( FAILED( g_pd3dDevice->CreateVertexBuffer( 4*sizeof(CUSTOMVERTEX),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_MANAGED, &g_pVB ) ) )
    {
        return E_FAIL;
    }

    // Fill the vertex buffer. We are setting the tu and tv texture
    // coordinates, which range from 0.0 to 1.0
    CUSTOMVERTEX* pVertices;
    if( FAILED( g_pVB->Lock( 0, 0, (BYTE**)&pVertices, 0 ) ) )
        return E_FAIL;
	CUSTOMVERTEX* v = pVertices;
	v[0].sx = -4;
	v[0].sy = -8;
	v[0].tu = 0;
	v[0].tv = 0;
	v[1].sx = 4;
	v[1].sy = -8;
	v[1].tu = 1;
	v[1].tv = 0;
	v[2].sx = -4;
	v[2].sy = 8;
	v[2].tu = 0;
	v[2].tv = 1;
	v[3].sx = 4;
	v[3].sy = 8;
	v[3].tu = 1;
	v[3].tv = 1;
	for (int i=0; i<4; i++)
	{
		v[i].sz = 0.f;
		v[i].color = 0xff00ff00;
	}	
    g_pVB->Unlock();
	return 0;
}

HRESULT DrawTest(LPDIRECT3DDEVICE8 g_pd3dDevice)
{
//	SetStates();
	HRESULT hr = 0;
	hr = g_pd3dDevice->SetStreamSource( 0, g_pVB, sizeof(CUSTOMVERTEX) );
	ASSERT(!FAILED(hr));
	hr = g_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );
	ASSERT(!FAILED(hr));
	hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);			
	ASSERT(!FAILED(hr));
	return 0;
}

HRESULT cGraphics::FlushPaint()
{
	HRESULT hr = 0;
	//should use different format draw ground and screen!!!
	m_pCamera->SetMatrix();

	if (0)
	{
		static bInit = false;
		if (!bInit)
		{
			InitTest(GetDevice3D());
			bInit = true;
		}
		else
		{
			DrawTest(GetDevice3D());
		}
	}

	if (m_pScreen)
		m_pScreen->PaintBefore();
	if (m_pGround)
		m_pGround->Paint();
	if (m_pScreen)
		m_pScreen->Paint();
	//should not put it here!
	return hr;
}

HRESULT cGraphics::EndPaint()
{
	m_nFrames++;
	PrintTrace();
	return S_OK;
}

/**************************************************************************/
//factory
/**************************************************************************/
HRESULT cGraphics::CreateGraphicsElement(ElementEnum id,LPVOID ppointer) 
{
	//因为使用的数据在最后的子类才new，构造函数中无法赋值,所以添加一个init来解决问题
	switch (id)
	{
	case GT_OBSTACLE_OLD:
	case GT_OBSTACLE:
		*((cObstacle**)ppointer) = new cObstacle(this);
		(*((cObstacle**)ppointer))->Init(true);
		break;				
	case GT_PLANEHORMESH:
	case GT_PLANEHOR_OLD:
	case GT_PLANEHORNORMAL:
		*((cPlaneHor**)ppointer) = new cPlaneHor(this);
		(*((cPlaneHor**)ppointer))->Init(true);
		break;		
	case GT_PLANEVERNORMAL:
		*((cPlaneVer**)ppointer) = new cPlaneVerNormal(this);
		(*((cPlaneVer**)ppointer))->Init(true);
		break;		
	case GT_PLANEVER_OLD:
	case GT_PLANEVERMESH:
		*((cPlaneVerMesh**)ppointer) = new cPlaneVerMesh(this);
		(*((cPlaneVerMesh**)ppointer))->Init(true);
		break;
	case GT_PLANEVERDYNAMIC:
		*((cPlaneVerDynamic**)ppointer) = new cPlaneVerDynamic(this);
		(*((cPlaneVerDynamic**)ppointer))->Init(true);
		break;
	case GT_SELECT:
		*((cSelect**)ppointer) = new cSelect(this);
		(*((cSelect**)ppointer))->Init(true);
		break;
	case GT_LIGHT:
		*((cLight**)ppointer) = new cLight(this);
		(*((cLight**)ppointer))->Init(true);
		break;		
	case GT_PICTURE:
		*((cPicture**)ppointer) = new cPicture(this);
		(*((cPicture**)ppointer))->Init(true);
		break;		
	case GT_MAP:
		*((cMap**)ppointer) = new cMap(this);
		(*((cMap**)ppointer))->Init(true);
		break;		
	case GT_SMALLMAP:
		*((cMapSmall**)ppointer) = new cMapSmall(this);
		(*((cMapSmall**)ppointer))->Init(true);
		break;		
	case GT_SPECIAL_SCROLL:
		*((cSpecialScroll**)ppointer) = new cSpecialScroll(this);
		break;		
	case GT_SPECIAL_SNOW:
		*((cSpecialSnow**)ppointer) = new cSpecialSnow(this);
		break;		
	case GT_SPECIAL_FIREFLY:
		*((cSpecialFirefly**)ppointer) = new cSpecialFirefly(this);
		break;		
	case GT_SPECIAL_SIGN:
		*((cSpecialSign**)ppointer) = new cSpecialSign(this);
		break;		
	}
	return 0;
}

/**************************************************************************/
//create device
/**************************************************************************/

/**************************************************************************/
//save a bmp
/**************************************************************************/

/**************************************************************************/
//	Thread
/**************************************************************************/
HRESULT cGraphics::Trace()
{
	iFps& fps = *m_pFps;

	char s[128];

	if (1)
	{
		cPoint ptGround;
		GetcCamera()->ScreenToGround(m_param.ptMouse,ptGround);
		cPoint ptTile = ptGround;
		GroundToTile(ptTile);
		cPoint ptRegion = ptTile;
		TileToRegion(ptRegion);
		cPoint ptDiamond;
		TileToDiamondSmall(ptTile,ptDiamond);

		cPoint pt0 = GetcCamera()->GetFocus();
		GroundToTile(pt0);
		pt0.x -= 0.5f;
		pt0.y -= 0.5f;
		cPoint pt1 = GetcCamera()->GetFocus();
		GroundToTile(pt1);
		pt1.x += 0.5f;
		pt1.y += 0.5f;
		TileToGround(pt0);
		TileToGround(pt1);
		GetcCamera()->GroundToScreen(pt0);
		GetcCamera()->GroundToScreen(pt1);
		int sx = fabs(pt1.x - pt0.x);
		int sy = fabs(pt1.y - pt0.y);

		sprintf(s,"mouse %d,%d ground %d,%d diamond %d,%d region %d,%d rect %d(+%d),%d(+%d) size %d,%d",
			(int)m_param.ptMouse.x,(int)m_param.ptMouse.y,
			(int)ptGround.x,(int)ptGround.y,(int)ptDiamond.x,(int)ptDiamond.y,
			(int)ptRegion.x, (int)ptRegion.y,
			GetcGround()->m_xRegion,GetcGround()->m_wRegion,
			GetcGround()->m_yRegion,GetcGround()->m_hRegion,
			sx,sy);
		if (m_param.lTraceEnum & TRACE_MOUSE)
			Trace(s);
		sprintf(s,"fps %d, faces %d, planever %d, light %d timer %d",
			fps.GetNowFps(), m_nFaces, GetcGround()->GetPlaneDrawNum(), GetcGround()->GetLightNum(),	m_param.nGameLoop);
//		if (m_param.lTraceEnum & TRACE_FPS)
			Trace(s);
		m_nFaces = 0;	
		sprintf(s,"cache regions:%d 地图(%d/%d) video(%d/%d),Staticspr(%d/%d),video(%d/%d),动画spr(%d/%d),video(%d,%d/%d)", 
			m_pGround->GetRegionNum(),
			(m_aTextureCache[USAGE_REGION]->GetCurrentSize(MEDIA_MEMORY)>>20)*2,m_aTextureCache[USAGE_REGION]->GetTotalSize(MEDIA_MEMORY)>>20,
			m_aTextureCache[USAGE_REGION]->GetCurrentSize(MEDIA_VIDEO)>>20,m_aTextureCache[USAGE_REGION]->GetTotalSize(MEDIA_VIDEO)>>20,
			m_aTextureCache[USAGE_STATIC]->GetCurrentSize(MEDIA_SPR)>>20,m_aTextureCache[USAGE_STATIC]->GetTotalSize(MEDIA_SPR)>>20,
			m_aTextureCache[USAGE_STATIC]->GetCurrentSize(MEDIA_VIDEO)>>20,m_aTextureCache[USAGE_STATIC]->GetTotalSize(MEDIA_VIDEO)>>20,
			m_aTextureCache[USAGE_ANIMATION_ACTOR]->GetCurrentSize(MEDIA_SPR)>>20,m_aTextureCache[USAGE_ANIMATION_ACTOR]->GetTotalSize(MEDIA_SPR)>>20,
			m_aTextureCache[USAGE_ANIMATION_ACTOR]->GetCurrentSize(MEDIA_VIDEO)>>20,m_aTextureCache[USAGE_ANIMATION_ACTOR]->GetCurrentSize(MEDIA_VIDEOONE)>>20,m_aTextureCache[USAGE_ANIMATION_ACTOR]->GetTotalSize(MEDIA_VIDEO)>>20
			);
//		if (m_param.lTraceEnum & TRACE_CACHE)
			Trace(s);	
		cColor cr = GetcGround()->GetColor();
		sprintf(s, "ground color: %d,%d,%d,%d, event: '%s'",cr.r,cr.g,cr.b,cr.a,GetcGround()->GetEvent());
		if (m_param.lTraceEnum & TRACE_GROUND)
			Trace(s);
	}

	if (m_param.lTraceEnum & TRACE_CAMERA)
	{
		GetcCamera()->Trace();
	}

	return S_OK;
}

HRESULT cGraphics::TextureCacheRun()
{
	cLock lock(&m_csTextureCache);
	for (int j=0; j<USAGE_NUM; j++)
		m_aTextureCache[j]->Run();
	return S_OK;
}

HRESULT cGraphics::RunOnce()
{
	iFps& fps = *m_pFps;
	fps.Frame();
	HandleMessage();
	//run will clean some texture!
	TextureCacheRun();
	GetcGround()->Run();
	BeginPaint();
	FlushPaint();
	Trace();
	EndPaint();
	if (GetFrame() > 0)
		fps.KeepFps(GetFrame());
	return 0;
}

UINT cGraphics::ThreadRun()
{
	while(m_pThread->WaitKillEvent() == WAIT_TIMEOUT)
	{
		if (!IsPausePaint())
		{
			GraphicsRun();		
		}
		else
		{
			Sleep(10);
		}
	}
//在跳出循环后处理所有的消息
	HandleMessage();
	m_pThread->SetEndEvent();
	return 0;
}

UINT cGraphics::Thread( LPVOID pParam )
{
	OutputDebugString("\nGraphics thread start.\n\n");
	
	cGraphics* pGraphics= (cGraphics*)pParam;
	pGraphics->ThreadRun();	
	
	OutputDebugString("\nGraphics thread exit.\n\n");
	return 0;
}

void BitOP(long& param, BitOPEnum eOP,long lBit)
{
	switch (eOP)
	{
	case Bit_Add:
		param |= lBit;
		break;
	case Bit_Remove:
		param &= ~lBit;
		break;
	case Bit_Clear:
		param = 0;
		break;
	case Bit_Toggle:
		param ^= lBit;
		break;
	case Bit_Set:
		param = lBit;
		break;
	}
}


