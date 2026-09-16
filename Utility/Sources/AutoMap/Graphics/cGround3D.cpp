// cGround3D.cpp: implementation of the cGround3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGround3D.h"

#include "Graphics.h"
#include "cGraphics.h"

#include "cPicture.h"
#include "cCamera.h"
#include "cGround3D.h"
#include "position.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cGround3D::cGround3D(cGraphics* p):cGround(p)
{
	m_pVertexGround = NULL;	
}

cGround3D::~cGround3D()
{

}

/**************************************************************************/
//device
/**************************************************************************/
LPDIRECT3DDEVICE8 cGround3D::GetDevice()
{
	return (GetGraphics()->GetDevice3D());
}

HRESULT cGround3D::InitDeviceObjects()
{
	cGround::InitDeviceObjects();
	HRESULT hr;
	hr = GetDevice()->CreateVertexBuffer(4*sizeof(stVertexGround), 0,
		D3DFVF_GROUNDVERTEX, D3DPOOL_MANAGED, &m_pVertexGround);
	ASSERT(!FAILED(hr));
	return hr;
}

//begin and end

HRESULT cGround3D::RestoreDeviceObjects()
{
	cGround::RestoreDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cGround3D::InvalidateDeviceObjects()
{
	cGround::InvalidateDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cGround3D::DeleteDeviceObjects()
{
	cGround::DeleteDeviceObjects();
	HRESULT hr = S_OK;
	RELEASE(m_pVertexGround);
	return hr;
}

/**************************************************************************/
//Environment
/**************************************************************************/
HRESULT cGround3D::SetEnvironment()
{
	HRESULT hr;
	hr = SetMaterial();
	hr = SetStates();
	hr = SetVertexBuffer();
	return hr;
}

HRESULT cGround3D::SetVertexBuffer()
{
	HRESULT hr;
	hr = GetDevice()->SetStreamSource(0, m_pVertexGround, sizeof(stVertexGround));
	ASSERT(!FAILED(hr));
	hr = GetDevice()->SetVertexShader( D3DFVF_GROUNDVERTEX );
	ASSERT(!FAILED(hr));
	return hr;
}

HRESULT cGround3D::SetStates()
{
	HRESULT hr = 0;
//*
	hr = GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	ASSERT(!FAILED(hr));
	hr = GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	ASSERT(!FAILED(hr));
	hr = GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	ASSERT(!FAILED(hr));

    hr = GetDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	ASSERT(!FAILED(hr));
    hr = GetDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	ASSERT(!FAILED(hr));

    hr = GetDevice()->SetRenderState( D3DRS_DITHERENABLE, TRUE );
	ASSERT(!FAILED(hr));
    hr = GetDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	ASSERT(!FAILED(hr));
    hr = GetDevice()->SetRenderState( D3DRS_LIGHTING,     FALSE );
	ASSERT(!FAILED(hr));
//	GetDevice()->SetRenderState( D3DRS_DITHERENABLE, TRUE );

    hr = GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );
	ASSERT(!FAILED(hr));
    hr = GetDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	ASSERT(!FAILED(hr));
    hr = GetDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	ASSERT(!FAILED(hr));
	
	hr = GetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	ASSERT(!FAILED(hr));

    // Enable alpha testing (skips pixels with less than a certain alpha.)
	int nTextureParam = -1;
	switch (nTextureParam)
	{
	case 0:
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		break;
	case 1:
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_POINT );
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_POINT );
		break;
	case 2:
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
		hr = GetDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);
		break;
	default:
		break;
	}

	float PointSize = 5.f;
	GetDevice()->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&PointSize));	

	return hr;
}

HRESULT cGround3D::SetMaterial()
{
	HRESULT hr = 0;

	ZeroMemory( &m_mtrl, sizeof(D3DMATERIAL8) );
	m_mtrl.Diffuse.r = m_mtrl.Ambient.r = 1.0f;
	m_mtrl.Diffuse.g = m_mtrl.Ambient.g = 1.0f;
	m_mtrl.Diffuse.b = m_mtrl.Ambient.b = 1.0f;
	m_mtrl.Diffuse.a = m_mtrl.Ambient.a = 1.0f;
	GetDevice()->SetMaterial( &m_mtrl );
//	GetDevice()->SetRenderState( D3DRS_AMBIENT, m_param.crColor.Value() );

	return hr;	
}

/**************************************************************************/
//draw
/**************************************************************************/
HRESULT cGround3D::SetTexture(iTexture* pTex, int frame)
{
	IDirect3DTexture8* p = pTex->GetD3DTexture(frame);
	if (p == NULL)
		return -1;
	HRESULT hr;
	hr = GetDevice()->SetTexture(0,p);
	return hr;
}

void cGround3D::Draw(stVertexGround* pV, int n)
{
	HRESULT hr;
	stVertexGround *m_pVB;
	
	SetVertexBuffer();

	hr = m_pVertexGround->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB,pV,sizeof(stVertexGround)*n);
	
	hr = m_pVertexGround->Unlock();
	ASSERT_HR(hr);
	
	GetGraphics()->IncreaseFace(n-2);

	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, n-2);
	ASSERT_HR(hr);
}

void cGround3D::DrawTriangle(stVertexGround* pV)
{
	Draw(pV,3);
}

void cGround3D::DrawQuadrangle(stVertexGround* pV)
{
	Draw(pV,4);
}

void cGround3D::DrawQuadrangle(stVertexGround* pV, int time)
{
	HRESULT hr;
	stVertexGround *m_pVB;

	GetGraphics()->IncreaseFace(2);

	hr = m_pVertexGround->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB,pV,sizeof(stVertexGround)*4);
	
	hr = m_pVertexGround->Unlock();
	ASSERT_HR(hr);
	
	for (int i=0; i<time; i++)
		hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	ASSERT_HR(hr);	
}

void cGround3D::Draw3Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3)
{
	HRESULT hr;
	stVertexGround *m_pVB;
	
	hr = m_pVertexGround->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB++,&v1,sizeof(stVertexGround));
	memcpy(m_pVB++,&v2,sizeof(stVertexGround));
	memcpy(m_pVB++,&v3,sizeof(stVertexGround));
	
	hr = m_pVertexGround->Unlock();
	ASSERT_HR(hr);
	
	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 1);
	ASSERT_HR(hr);
}

void cGround3D::Draw4Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3,stVertexGround& v4)
{
	HRESULT hr;
	stVertexGround *m_pVB;
	
	hr = m_pVertexGround->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB++,&v1,sizeof(stVertexGround));
	memcpy(m_pVB++,&v2,sizeof(stVertexGround));
	memcpy(m_pVB++,&v3,sizeof(stVertexGround));
	memcpy(m_pVB++,&v4,sizeof(stVertexGround));
	
	hr = m_pVertexGround->Unlock();
	ASSERT_HR(hr);
	
	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	ASSERT_HR(hr);
}

/*
void cGround3D::DrawFan(iTexture* pTex, int frame, stVertexGround* pV, int n)
{
	HRESULT hr;
	stVertexGround *m_pVB;
	
	GetDevice()->SetTexture(0,Get3DTexture(pTex,frame));
	
	hr = m_pVertexGround->Lock(0, 0, (BYTE**)&m_pVB, 0);
	ASSERT_HR(hr);
	
	memcpy(m_pVB,pV,sizeof(stVertexGround)*n);
	
	hr = m_pVertexGround->Unlock();
	ASSERT_HR(hr);
	
	hr = GetDevice()->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, n - 2);
	ASSERT_HR(hr);
}
*/

////////////////////////////////////////////////////////////
//point and line
////////////////////////////////////////////////////////////

/*
void cGround3D::DrawPoint(cPoint pt, cColor cr) 
{
	cPoint p2 = pt;
	p2.x += 0.2f;
	DrawLine(pt,p2,cr);
}
*/

const int MAX_TILE = 4000;
stVertexGround aVertex[MAX_TILE];

void cGround3D::DrawPointAsRect(std::list<cPoint>& ptlist, cColor cr)
{
	if (ptlist.empty())
		return;
	int nPoint = ptlist.size() * 4*2;
//	ASSERT(nPoint < MAX_TILE);
	if (nPoint > MAX_TILE)
		return ;
//	stVertexGround* aVertex = new stVertexGround[nPoint];

	HRESULT hr;
	int j=0;
	for (std::list<cPoint>::iterator it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		cPoint aPoint[8];
		int i=0;
		aPoint[i++] = cPoint(pt.x-0.5f,pt.y-0.5f);
		aPoint[i++] = cPoint(pt.x+0.5f,pt.y-0.5f);
		aPoint[i++] = cPoint(pt.x+0.5f,pt.y-0.5f);
		aPoint[i++] = cPoint(pt.x+0.5f,pt.y+0.5f);
		aPoint[i++] = cPoint(pt.x+0.5f,pt.y+0.5f);
		aPoint[i++] = cPoint(pt.x-0.5f,pt.y+0.5f);
		aPoint[i++] = cPoint(pt.x-0.5f,pt.y+0.5f);
		aPoint[i++] = cPoint(pt.x-0.5f,pt.y-0.5f);
		for (i=0; i<8; i++)
		{
			stVertexGround& v = aVertex[i+j];
			SetPoint(v,aPoint[i]);
			v.color = cr.Value();
			v.tu = 0.f;
			v.tv = 0.f;
		}
		j+=8;
	}

	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_LINELIST,nPoint/2 , aVertex, sizeof(stVertexGround));
	DX_ERROR(hr);
	ASSERT_HR(hr);
//	delete aVertex;
}

void cGround3D::DrawPoint(std::list<cPoint>& ptlist, cColor cr) 
{
//	ASSERT(ptlist.size() < MAX_POINT);
	int i=0;
	int left = ptlist.size();
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);

	HRESULT hr;
	for (std::list<cPoint>::iterator it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
//		DrawPoint(pt,cr);
		stVertexGround& v = m_aVertexPoint[i++];
		SetPoint(v,pt);
		v.color = cr.Value();
		v.tu = 0.f;
		v.tv = 0.f;
		if (i == MAX_POINT)
		{
			hr = GetDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, MAX_POINT, m_aVertexPoint, sizeof(stVertexGround));
			DX_ERROR(hr);
			ASSERT_HR(hr);
			left -= MAX_POINT;
			i = 0;
		}
	}
	if (left > 0)
	{
		hr = GetDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, left, m_aVertexPoint, sizeof(stVertexGround));
		DX_ERROR(hr);
		ASSERT_HR(hr);
	}
}

void cGround3D::DrawPoint(cPoint* aPoint, int nPoint, cColor cr) 
{
	ASSERT(nPoint < MAX_POINT);
	for (int i=0; i<nPoint; i++)
	{
		cPoint pt(aPoint[i].x,aPoint[i].y);
		stVertexGround& v = m_aVertexPoint[i];
		SetPoint(v,pt);
		v.color = cr.Value();
		v.tu = 0.f;
		v.tv = 0.f;		
	}
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, nPoint, m_aVertexPoint, sizeof(stVertexGround));
	
	DX_ERROR(hr);
}

void cGround3D::DrawPoint(cPoint p, cColor cr) 
{
//	std::list<cPoint> ptlist;
//	ptlist.push_back(p);
//	DrawPoint(ptlist,cr);
	DrawPoint(&p,1,cr);
}

void cGround3D::DrawLine(cPoint pt1,cPoint pt2, cColor cr)
{
	stVertexGround v[2];
	v[0].sx = pt1.x;
	v[0].sy = pt1.y;
	v[0].sz = pt1.z;
	v[0].tu = 0.f;
	v[0].tv = 0.f;
	v[0].color = cr.Value();
	v[1].sx = pt2.x;
	v[1].sy = pt2.y;
	v[1].sz = pt2.z;
	v[1].tu = 1.f;
	v[1].tv = 1.f;
	v[1].color = cr.Value();
	DrawLine(v,2);
}

void cGround3D::DrawLine(stVertexGround* pV, int n)
{
	ASSERT(n > 0);
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP, n-1, pV, sizeof(stVertexGround));
	ASSERT_HR(hr);
}

void cGround3D::DrawPolygon(stVertexGround* pV, int num, cColor cr)
{
	stVertexGround v[MAX_POINT];
	memcpy(v,pV,sizeof(stVertexGround) * num);
	for (int i=0; i<num; i++)
		v[i].color = cr.Value();
	v[num] = v[0];
	
	HRESULT hr;
	GetGraphics()->SetCommonTexture((cGraphics::eTexKind)COLOR_WHITE,0);
	hr = GetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP, num, v, sizeof(stVertexGround));
	ASSERT_HR(hr);
}

void cGround3D::DrawRect(cRect r, cColor cr,float fZ)
{
	stVertexGround v[4];
	FillRect(v,r);
	for (int i=0; i<4; i++)
		v[i].sz = fZ;
	DrawRect(v,cr);	
}

void cGround3D::DrawRect(stVertexGround* pV, cColor cr)
{
	stVertexGround v[5];
	//µßµ¹Ò»ÏÂË³Ðò
	v[0] = pV[0];
	v[1] = pV[1];
	v[2] = pV[3];
	v[3] = pV[2];
	v[4] = pV[0];
	DrawPolygon(v,4,cr);
}
