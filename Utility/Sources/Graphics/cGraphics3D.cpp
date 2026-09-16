// cGraphics3D.cpp: implementation of the cGraphics3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cGraphics3D.h"

#include "cGround3D.h"
#include "cScreen3D.h"
#include "cCamera3D.h"

//#include "cFont3D.h"
#include "cMesh3D.h"
#include "cTextureCache.h"
#include "cTextureVideo.h"
#include "cTextureVideoDirection.h"
#include "cTextureOne.h"
#include "cTextureVideoOther.h"
#include "cTextureMemory.h"
#include "cTextureSpr.h"

#include <Mmsystem.h>
#include <fstream>

#include "Font/KFontD3D.h"
#include "fileop.h"
#include "planedoodad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cGraphics3D::cGraphics3D()
{

}

cGraphics3D::~cGraphics3D()
{

}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
const int c_wScreen = 800;
const int c_hScreen = 600;

void cGraphics3D::GetScreen(int& x,int &y)
{
	x = m_d3dpp.BackBufferWidth; 
	y = m_d3dpp.BackBufferHeight;
	x = c_wScreen;
	y = c_hScreen;
}

/**************************************************************************/
//paint loop
/**************************************************************************/
BOOL cGraphics3D::IsPaint()
{
	return ( m_bActive && m_bReady );
}

HRESULT cGraphics3D::BeginPaint() 
{
	cGraphics::BeginPaint();

	HRESULT hr ;

    hr = GetDevice3D()->Clear( 0, NULL, D3DCLEAR_TARGET,
                         D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
	ASSERT(!FAILED(hr));
/*	
    hr = GetDevice3D()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
*/
//	hr = GetDevice3D()->Clear(0, NULL, D3DCLEAR_TARGET, 
//		D3DCOLOR_COLORVALUE(0,0,0,0),
//		1.f, 0);

	hr = GetDevice3D()->BeginScene();
	ASSERT_HR(hr);
	
	return hr;
}


HRESULT cGraphics3D::EndPaint()
{
	cGraphics::EndPaint();

	GetDevice3D()->SetStreamSource(0, NULL, 0);
	HRESULT hr = GetDevice3D()->EndScene();
	ASSERT_HR(hr);

	LARGE_INTEGER c1,c2;
	BOOL b = QueryPerformanceCounter(&c1);
//	hr = GetDevice3D()->Present(NULL, NULL, NULL, NULL);
	b = QueryPerformanceCounter(&c2);
	m_nFlipTicks = c2.LowPart - c1.LowPart;
/*
	if (FAILED(hr))
	{
		CString sMsg = (CString)m_param.szName + ": Error render !\n";
		OutputDebugString(sMsg);
		if( FAILED( hr = m_pd3dDevice->Reset( &m_d3dpp ) ) )
		{
			OutputDebugString("My Error when Reset\n");
			Sleep(1000);
			ResizeEnvironment();
		}
	}
*/	//	ASSERT_HR(hr);
	
	return hr;
}

/**************************************************************************/
//factory
/**************************************************************************/

HRESULT cGraphics3D::CreateGraphicsElement(ElementEnum id,LPVOID ppointer) 
{
	switch (id)
	{
	case GT_FONT:
		{
//		{*((cFont**)ppointer) = new cFont3D(this);
		*((iKFont**)ppointer) = new KFontD3D;
		}
		break;
	case GT_GROUND:
		*((cGround**)ppointer) = new cGround3D(this);
		break;
	case GT_SCREEN:
		*((cScreen**)ppointer) = new cScreen3D(this);
		break;		
	case GT_CAMERA:
		*((cCamera**)ppointer) = new cCamera3D(this);
		break;		
	case GT_MESH:
		*((cMesh**)ppointer) = new cMesh3D(this);
		break;		
	case GT_TEXTUREONE:
		*((cTextureOne**)ppointer) = new cTextureOne(this);
//		*((cTextureVideoDirection**)ppointer) = new cTextureVideoDirection(this);
		break;				
	case GT_TEXTUREVIDEO:
		*((cTextureVideo**)ppointer) = new cTextureVideo(this);
		break;				
	case GT_TEXTUREMEMORY:
		*((cTextureMemory**)ppointer) = new cTextureMemory(this);
		break;				
	case GT_TEXTURESPR:
		*((cTextureSpr**)ppointer) = new cTextureSpr(this);
		break;				
	case GT_TEXTUREVIDEOOTHER:
		*((cTextureVideoOther**)ppointer) = new cTextureVideoOther(this);
		break;				
	default:
		return cGraphics::CreateGraphicsElement(id,ppointer);
	}
	return 0;
}

HRESULT cGraphics3D::GetDevice(LPVOID ppointer)
{
	*((LPDIRECT3DDEVICE8*)ppointer) = GetDevice3D();	
	return S_OK;
}

/**************************************************************************/
//create device
/**************************************************************************/

HRESULT cGraphics3D::CreateDevice(const stParam& screen)
{
    // Create the D3D object.
/*
	m_pD3D       = NULL; // Used to create the D3DDevice
    if( NULL == ( m_pD3D = Direct3DCreate8( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Get the current desktop display mode, so we can set up a back
    // buffer of the same format
    D3DDISPLAYMODE d3ddm;
    if( FAILED( m_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	HWND hWnd = screen.hWnd;
    // Create the D3DDevice
    if( FAILED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &m_pd3dDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn off culling
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Turn off D3D lighting
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    // Turn on the zbuffer
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	InitDeviceObjects();
	RestoreDeviceObjects();

    return S_OK;
//*/
	HRESULT hr;
	
//	m_param = screen;
	m_param.hInst = screen.hInst;
	m_param.hWnd = screen.hWnd;
	m_param.x = screen.x;
	m_param.y = screen.y;
	m_param.fWidth = screen.fWidth;
	m_param.fHeight = screen.fHeight;
	m_param.bFullScreen = screen.bFullScreen;
	m_param.nFrame = screen.nFrame;

	hr = Create(screen.hInst,screen.hWnd,screen.fWidth,screen.fHeight,screen.bFullScreen);
//	if (!FAILED(hr))
//		ShowCursor(FALSE);
	
	return hr;
}

/**************************************************************************/
//Error report
/**************************************************************************/

HRESULT cGraphics3D::DXError(HRESULT hr)
{
	if (FAILED(hr))
	{
//		AfxMessageBox(DXGetErrorDescription8(hr));
		OutputDebugString(DXGetErrorDescription8(hr));
		OutputDebugString("\n");
//		ASSERT(0);
	}	
	return hr;
}

/**************************************************************************/
//save a bmp
/**************************************************************************/
HRESULT cGraphics3D::SaveBmpBuffer(DWORD* pbuffer, cRect rect)
{
	HDC dc = GetDC(m_hWnd);
	int bits = GetDeviceCaps(dc,BITSPIXEL);
	if (bits == 32)
		return SaveBmpBuffer32(pbuffer,rect);
	else if (bits == 16)
		return SaveBmpBuffer16(pbuffer,rect);
	return -1;
}

HRESULT cGraphics3D::SaveBmpBuffer16(DWORD* pbuffer, cRect rect)
{
	CRect r = CRect(rect.l,rect.t,rect.r,rect.b);

	int nTextureWidth = 800;
	int nTextureHeight = 600;
	if (m_bWindowed)
	{
		CRect rect;
		GetWindowRect(GetDesktopWindow(),rect); 
		nTextureWidth = rect.Width();
		nTextureHeight = rect.Height();

		CRect rcWindow;
		GetWindowRect(m_hWnd,rcWindow);
		r.OffsetRect(rcWindow.TopLeft());
	}

	HRESULT hr;
	IDirect3DSurface8 *pSurface;
	if(FAILED(hr = GetDevice3D()->CreateImageSurface(nTextureWidth,nTextureHeight,D3DFMT_A8R8G8B8,&pSurface)))
	{
		return false;
	}
	if(FAILED(hr = GetDevice3D()->GetFrontBuffer(pSurface)))
	{
		DXError(hr);
		pSurface->Release();
		return false;
	}

	WORD* buffer = (WORD*)pbuffer;
    D3DLOCKED_RECT lr;
	hr = pSurface->LockRect( &lr, NULL, 0 );
	char* pOff = (char*)lr.pBits + lr.Pitch * r.top + r.left * sizeof(DWORD);
	for (int i=0; i<rect.Height(); i++)
	{
		char* pSrc = pOff;

		for (int j=0; j<rect.Width(); j++)
		{
			char b = *pSrc++;
			char g = *pSrc++;
			char r = *pSrc++;
			char a = *pSrc++;
			*buffer++ = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
//			ASSERT(a == 0);
		}
		pOff += lr.Pitch;
	}
	pSurface->UnlockRect();
	pSurface->Release();
	return true;
}


HRESULT cGraphics3D::SaveBmpBuffer32(DWORD* pbuffer, cRect rect)
{
	CRect r = CRect(rect.l,rect.t,rect.r,rect.b);

	int nTextureWidth = 800;
	int nTextureHeight = 600;
	if (m_bWindowed)
	{
		CRect rect;
		GetWindowRect(GetDesktopWindow(),rect); 
		nTextureWidth = rect.Width();
		nTextureHeight = rect.Height();

		CRect rcWindow;
		GetWindowRect(m_hWnd,rcWindow);
		r.OffsetRect(rcWindow.TopLeft());
	}

	HRESULT hr;
	IDirect3DSurface8 *pSurface;
	if(FAILED(hr = GetDevice3D()->CreateImageSurface(nTextureWidth,nTextureHeight,D3DFMT_A8R8G8B8,&pSurface)))
	{
		return false;
	}
	if(FAILED(hr = GetDevice3D()->GetFrontBuffer(pSurface)))
	{
		DXError(hr);
		pSurface->Release();
		return false;
	}

	WORD* buffer = (WORD*)pbuffer;
    D3DLOCKED_RECT lr;
	hr = pSurface->LockRect( &lr, NULL, 0 );
	char* pOff = (char*)lr.pBits + lr.Pitch * r.top + r.left * sizeof(DWORD);
	for (int i=0; i<rect.Height(); i++)
	{
		char* pSrc = pOff;
		memcpy(pbuffer,pSrc,rect.Width()*sizeof(long));
		pbuffer += int(rect.Width());
		pOff += lr.Pitch;
	}
	pSurface->UnlockRect();
	pSurface->Release();
	return true;
}

HRESULT cGraphics3D::SaveBmp(const char* bmp_name, cRect rect)
{
	CRect r = CRect(rect.l,rect.t,rect.r,rect.b);
/*
	HRESULT hr;
	IDirect3DSurface8 *bbuf;
	hr = GetDevice3D()->GetRenderTarget(&bbuf);
//	hr = GetDevice3D()->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&bbuf);
	ASSERT(!FAILED(hr));
	hr = D3DXSaveSurfaceToFile(bmp_name,D3DXIFF_BMP,bbuf,NULL,&r);
	ASSERT(!FAILED(hr));
	bbuf->Release();
	return true;	
*/
	
	int nTextureWidth = 800;
	int nTextureHeight = 600;
	if (m_bWindowed)
	{
		CRect rect;
		GetWindowRect(GetDesktopWindow(),rect); 
		nTextureWidth = rect.Width();
		nTextureHeight = rect.Height();

		CRect rcWindow;
		GetWindowRect(m_hWnd,rcWindow);
		r.OffsetRect(rcWindow.TopLeft());
	}

	HRESULT hr;
	IDirect3DSurface8 *pSurface;
	if(FAILED(hr = GetDevice3D()->CreateImageSurface(nTextureWidth,nTextureHeight,D3DFMT_A8R8G8B8,&pSurface)))
	{
		return false;
	}
	if(FAILED(hr = GetDevice3D()->GetFrontBuffer(pSurface)))
	{
		DXError(hr);
		pSurface->Release();
		return false;
	}

	if(FAILED(hr = D3DXSaveSurfaceToFile(bmp_name,D3DXIFF_BMP,
								pSurface,NULL,&r)))
	{
		pSurface->Release();
		return false;
	}
	pSurface->Release();
	return true;
}

/**************************************************************************/
// Overridable functions for the 3D scene created by the app
/**************************************************************************/

//-----------------------------------------------------------------------------
// Nddame: ConfirmDevice()
// Desc: Called during device intialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT cGraphics3D::ConfirmDevice( D3DCAPS8* pCaps, DWORD dwBehavior,
										 D3DFORMAT Format )
{
//	return S_OK;
	if (dwBehavior == D3DCREATE_SOFTWARE_VERTEXPROCESSING)
		return S_OK;
	return -1;
}

//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT cGraphics3D::InitDeviceObjects()
{
	HRESULT hr;
//	hr = m_pDebugFont->InitDeviceObjects();

	if (m_pDebugFont)
	{
		m_pDebugFont->Init(GetDevice3D());
		CString strFile = "system\\font\\gbk_fs16.fnt";
		AppGetFullPath(strFile);
		m_pDebugFont->Load(strFile);
	}

	if (m_pGround)
		hr = m_pGround->InitDeviceObjects();
	if (m_pScreen)
		hr = m_pScreen->InitDeviceObjects();
	hr = m_pCamera->InitDeviceObjects();

	for (int j=0; j<USAGE_NUM; j++)
	{
		m_aTextureCache[j]->InitDeviceObjects();
	}

	return hr;
}
//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT cGraphics3D::RestoreDeviceObjects()
{
	HRESULT hr;
	if (m_pDebugFont)
		hr = m_pDebugFont->RestoreDeviceObjects();
	if (m_pGround)
		hr = m_pGround->RestoreDeviceObjects();
	if (m_pScreen)
		hr = m_pScreen->RestoreDeviceObjects();
	for (int j=0; j<USAGE_NUM; j++)
	{
		m_aTextureCache[j]->RestoreDeviceObjects();
	}
	return hr;
}
//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT cGraphics3D::InvalidateDeviceObjects()
{
	HRESULT hr;
	if (m_pDebugFont)
		m_pDebugFont->InvalidateDeviceObjects();
	if (m_pGround)
		hr = m_pGround->InvalidateDeviceObjects();
	if (m_pScreen)
		hr = m_pScreen->InvalidateDeviceObjects();
	for (int j=0; j<USAGE_NUM; j++)
	{
		m_aTextureCache[j]->InvalidateDeviceObjects();
	}
	return hr;
}
//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT cGraphics3D::DeleteDeviceObjects()
{
	HRESULT hr;
//	hr = m_pDebugFont->DeleteDeviceObjects();
	if (m_pScreen)
		hr = m_pScreen->DeleteDeviceObjects();
	if (m_pGround)
		hr = m_pGround->DeleteDeviceObjects();
	for (int j=0; j<USAGE_NUM; j++)
	{
		m_aTextureCache[j]->DeleteDeviceObjects();
	}
	return hr;
}
//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------

HRESULT cGraphics3D::FinalCleanup()
{
	return Cleanup();
}

HRESULT cGraphics3D::ToggleFullscreen() 
{
	return CD3DApplication::ToggleFullscreen();
}

HRESULT cGraphics3D::ResizeEnvironment()
{
	return CD3DApplication::Resize3DEnvironment();
}

HRESULT	cGraphics3D::PrintScreen(LPCSTR szFile)
{
	//相见不如怀念，有时候保持距离反而好些
	int width,height;
	GetScreen(width,height);
	SaveBmp(szFile,cRect(0,0,width,height));

	return S_OK;
}

HRESULT cGraphics3D::GraphicsRun()
{
	if (IsPaint())
		Render3DEnvironment();
	return S_OK;
}

HRESULT cGraphics3D::Render()
{
	return RunOnce();
}

LRESULT cGraphics3D::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return MsgProc(hWnd,uMsg,wParam,lParam);
}
