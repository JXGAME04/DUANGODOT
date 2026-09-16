// cScreen3D.cpp: implementation of the cScreen3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cScreen3D.h"

#include "Graphicspublic.h"
#include "cGraphics.h"

#include "cPicture.h"
#include "position.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cScreen3D::cScreen3D(cGraphics* p) : cScreen(p)
{
	m_pVertexScreen= NULL;	
}

cScreen3D::~cScreen3D()
{

}

/**************************************************************************/
//device
/**************************************************************************/
LPDIRECT3DDEVICE8 cScreen3D::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

HRESULT cScreen3D::InitDeviceObjects()
{
	cScreen::InitDeviceObjects();
	HRESULT hr = S_OK;
	hr = GetDevice()->CreateVertexBuffer(MAX_POINT*sizeof(stVertexScreen), 0,
		D3DFVF_SCREENVERTEX, D3DPOOL_MANAGED, &m_pVertexScreen);
	ASSERT(!FAILED(hr));
	return hr;
}

HRESULT cScreen3D::RestoreDeviceObjects()
{
	cScreen::RestoreDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cScreen3D::InvalidateDeviceObjects()
{
	cScreen::InvalidateDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cScreen3D::DeleteDeviceObjects()
{
	cScreen::DeleteDeviceObjects();
	HRESULT hr = S_OK;
	RELEASE(m_pVertexScreen);
	return hr;
}

/**************************************************************************/
//draw
/**************************************************************************/
/*
void cScreen3D::DrawRaw(iTexture* pTex,int frame, stVertexScreen* pV, int n)
{
	HRESULT hr;	
	IDirect3DVertexBuffer8* pData;
	UINT size;
	hr = GetDevice()->GetStreamSource(0,&pData,&size);
	ASSERT(!FAILED(hr));
	DWORD handle;
	hr = GetDevice()->GetVertexShader(&handle);
	
	hr = GetDevice()->SetStreamSource(0, m_pVertexScreen, sizeof(stVertexScreen));
	ASSERT(!FAILED(hr));
	hr = GetDevice()->SetVertexShader( D3DFVF_SCREENVERTEX );
	ASSERT(!FAILED(hr));
	Draw(pTex,frame,pV,n);	
	if (pData != NULL)
	{
		hr = GetDevice()->SetStreamSource(0, pData , size);
		ASSERT(!FAILED(hr));
		pData->Release();
	}
	if (handle != 0)
		hr = GetDevice()->SetVertexShader( handle);
	ASSERT(!FAILED(hr));
}
*/
//simplest function
void cScreen3D::Draw(iTexture* pTex, int frame, CPoint ptScreen)
{
	Draw(pTex,frame,ptScreen,cColor(255,255,255,255));
}

void cScreen3D::Draw(iTexture* pTexture,int frame, CPoint p, cColor cr)
{
	SetTexture(pTexture,frame);

	ULONG width,height;
	pTexture->GetImageSize(frame,width,height);
	cRect rc;
	pTexture->GetTextureRect(frame,rc);
	int x,y;
	pTexture->GetCenterToUpleft(frame,x,y);
	
	p.x = (int)p.x;
	p.y = (int)p.y;
	
	stVertexScreen v[4];
	
	v[0].color = v[1].color = v[2].color = v[3].color = cr.Value();
	v[0].rhw = v[1].rhw = v[2].rhw = v[3].rhw = 1.f;
	v[0].sz = v[1].sz = v[2].sz = v[3].sz = 0.f;
	
	v[0].sx = p.x; v[0].sy = p.y;
	v[1].sx = p.x + width; v[1].sy = p.y;
	v[2].sx = p.x; v[2].sy = p.y + height;
	v[3].sx = p.x + width; v[3].sy = p.y + height;
	
	v[0].tu = rc.l; v[0].tv = rc.t;
	v[1].tu = rc.r; v[1].tv = rc.t;
	v[2].tu = rc.l; v[2].tv = rc.b;
	v[3].tu = rc.r; v[3].tv = rc.b;
	
	Draw(v,4);
}

void cScreen3D::SetTexture(iTexture* pTex, int frame )
{
	HRESULT hr;
	hr = GetDevice()->SetTexture(0,pTex->GetD3DTexture(frame));
	ASSERT_HR(hr);
}

void cScreen3D::DrawFan(stVertexScreen* pV, int n)
{
	HRESULT hr;
	stVertexScreen *m_pVB;
	
	hr = m_pVertexScreen->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB,pV,sizeof(stVertexScreen)*n);
	
	hr = m_pVertexScreen->Unlock();
	ASSERT_HR(hr);
	
	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, n-2);
	ASSERT_HR(hr);
}

void cScreen3D::Draw(stVertexScreen* pV, int n )
{
	HRESULT hr;
	stVertexScreen *m_pVB;
	
	hr = m_pVertexScreen->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB,pV,sizeof(stVertexScreen)*n);
	
	hr = m_pVertexScreen->Unlock();
	ASSERT_HR(hr);
	
	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, n-2);
	ASSERT_HR(hr);
}

////////////////////////////////////////////////////////////
//point and line
////////////////////////////////////////////////////////////
void cScreen3D::DrawPoint(CPoint* aPoint, int nPoint, cColor cr) 
{
	ASSERT(nPoint < MAX_POINT);
	for (int i=0; i<nPoint; i++)
	{
		cPoint pt(aPoint[i].x,aPoint[i].y);
		stVertexScreen& v = m_aVertexPoint[i];
		SetPoint(v,pt);
		v.color = cr.Value();
		v.rhw = 1.f;
		v.tu = 0.f;
		v.tv = 0.f;		
	}
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, nPoint, m_aVertexPoint, sizeof(stVertexScreen)*nPoint);
	ASSERT_HR(hr);
}

void cScreen3D::DrawPoint(CPoint p, cColor cr) 
{
	DrawPoint(&p,1,cr);
}

void cScreen3D::DrawLine(CPoint p0,CPoint p1, cColor cr)
{
	stVertexScreen v[2];
	v[0].sx = p0.x;
	v[0].sy = p0.y;
	v[0].sz = 0.f;
	v[0].rhw = 1.f;
	v[0].tu = 0.f;
	v[0].tv = 0.f;
	v[0].color = cr.Value();

	v[1].sx = p1.x;
	v[1].sy = p1.y;
	v[1].sz = 0.f;
	v[1].rhw = 1.f;
	v[1].tu = 0.f;
	v[1].tv = 0.f;
	v[1].color = cr.Value();

	DrawLine(v,2);
}

void cScreen3D::DrawRect(cRect& rc, cColor cr)
{
	stVertexScreen v[4];
	FillRect(v,rc);
	DrawRect(v,cr);
}

void cScreen3D::DrawLine(stVertexScreen* pV, int n)
{
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP, n-1, pV, sizeof(stVertexScreen));
	ASSERT_HR(hr);
}

void cScreen3D::DrawPolygon(stVertexScreen* pV, int num, cColor cr)
{
	stVertexScreen v[MAX_POINT];
	memcpy(v,pV,sizeof(stVertexScreen) * num);
	for (int i=0; i<num; i++)
		v[i].color = cr.Value();
	v[num] = v[0];
	
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP, num, v, sizeof(stVertexScreen));
	ASSERT_HR(hr);
}

void cScreen3D::DrawRect(stVertexScreen* pV, cColor cr )
{
	stVertexScreen v[4];
	v[0] = pV[0];
	v[1] = pV[1];
	v[2] = pV[3];
	v[3] = pV[2];
	DrawPolygon(v,4,cr);
}

/**************************************************************************/
//Environment
/**************************************************************************/
HRESULT cScreen3D::SetEnvironment()
{
	HRESULT hr = SetStates();
	hr = SetVertexBuffer();
	return hr;
}

HRESULT cScreen3D::SetStates()
{
	HRESULT hr = 0;
	
	GetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    GetDevice()->SetRenderState( D3DRS_LIGHTING, FALSE );
    GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
	
	GetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	GetDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	GetDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	
	GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

	
	float PointSize = 5.f;
	GetDevice()->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&PointSize));	

	return hr;
}

HRESULT cScreen3D::SetVertexBuffer()
{
	HRESULT hr = GetDevice()->SetStreamSource(0, m_pVertexScreen, sizeof(stVertexScreen));
	ASSERT(!FAILED(hr));
	hr = GetDevice()->SetVertexShader( D3DFVF_SCREENVERTEX );
	ASSERT(!FAILED(hr));	
	return hr;
}
