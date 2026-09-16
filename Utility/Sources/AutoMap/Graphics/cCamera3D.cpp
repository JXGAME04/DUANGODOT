// cCamera3D.cpp: implementation of the cCamera3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCamera3D.h"

#include "Graphics.h"
#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCamera3D::cCamera3D(cGraphics* p):cCamera(p)
{

}

cCamera3D::~cCamera3D()
{

}

/**************************************************************************/
//set matrix	
/**************************************************************************/
//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
VOID cCamera3D::SetViewParams( D3DXVECTOR3 &vEyePt, D3DXVECTOR3& vLookatPt,
							D3DXVECTOR3& vUpVec )
{
    // Set attributes for the view matrix
    m_vEyePt    = vEyePt;
    m_vLookatPt = vLookatPt;
    m_vUpVec    = vUpVec;
    D3DXVec3Normalize( &m_vView, &(m_vLookatPt - m_vEyePt) );
    D3DXVec3Cross( &m_vCross, &m_vView, &m_vUpVec );
	
    D3DXMatrixLookAtLH( &m_matView, &m_vEyePt, &m_vLookatPt, &m_vUpVec );
    D3DXMatrixInverse( &m_matBillboard, NULL, &m_matView );
    m_matBillboard._41 = 0.0f;
    m_matBillboard._42 = 0.0f;
    m_matBillboard._43 = 0.0f;
}

//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
VOID cCamera3D::SetProjParams( FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane,
							FLOAT fFarPlane )
{
    // Set attributes for the projection matrix
    m_fFOV        = fFOV;
    m_fAspect     = fAspect;
    m_fNearPlane  = fNearPlane;
    m_fFarPlane   = fFarPlane;
	
    D3DXMatrixPerspectiveFovLH( &m_matProj, fFOV, fAspect, fNearPlane, fFarPlane );
}

VOID cCamera3D::SetScaleParams( FLOAT fScale)
{
	D3DXMATRIX mScale;
	
	D3DXMatrixScaling(&m_matScale,fScale,fScale,1.f);
//	D3DXMatrixScaling(&m_matScale,fScale,fScale,fScale);
}

/**************************************************************************/
//device
/**************************************************************************/
LPDIRECT3DDEVICE8 cCamera3D::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

HRESULT cCamera3D::InitDeviceObjects()
{
	float fx = m_param.ptTo.x + m_param.fDis * sin(m_param.fAngleX * D3DX_PI / 180) ;
	float fy = m_param.ptTo.y + m_param.fDis * cos(m_param.fAngleX * D3DX_PI / 180) ;
	float fz = m_param.ptTo.z + m_param.fDis * tan(m_param.fAngle * D3DX_PI / 180);
	m_param.ptEye = cPoint(fx,fy,fz);
	
	m_param.ptUp = cPoint(0, 0, 1);
	
	// Set attributes for the view matrix
	SetViewParams( D3DXVECTOR3(m_param.ptEye.x,m_param.ptEye.y,m_param.ptEye.z), 
		D3DXVECTOR3(m_param.ptTo.x,m_param.ptTo.y,m_param.ptTo.z),
		D3DXVECTOR3(m_param.ptUp.x,m_param.ptUp.y,m_param.ptUp.z));
	// Set attributes for the projection matrix
	int width,height;
	GetGraphics()->GetScreen(width,height);
	
	m_param.fFOV = D3DX_PI/4;
//	m_param.fFOV = D3DX_PI/32;
	m_param.fAspect = (float)width / (float)height;
	m_param.fNearPlane = 1.f;
	m_param.fFarPlane = 10000.0f;
	
	SetProjParams(m_param.fFOV  , m_param.fAspect , m_param.fNearPlane , m_param.fFarPlane );
	
//	m_param.fScale = 1.f;
	SetScaleParams(m_param.fScale);
	
	return 0;
}

HRESULT cCamera3D::UpdateMatrix()
{
	D3DXMatrixIdentity(&m_matWorld);
	D3DXMatrixMultiply(&m_matProjAndScale,&m_matProj,&m_matScale);
	int w,h;
	GetGraphics()->GetScreen(w,h);
	D3DVIEWPORT8 viewData = { 0, 0, w, h, 0.0f, 1.0f };
//	HRESULT hr = GetDevice()->GetViewport(&viewData);
//	DX_ERROR(hr);
	m_matScreen= D3DXMATRIX(
		viewData.Width/2, 0, 0, 0,
		0, -(float)viewData.Height/2, 0, 0,
		0,0,viewData.MaxZ - viewData.MinZ, 0,
		viewData.X + viewData.Width/2, viewData.Y + viewData.Height/2, viewData.MinZ, 1);
	
	D3DXMatrixMultiply(&m_matTrans,&m_matWorld,&m_matView);
	D3DXMatrixMultiply(&m_matTrans,&m_matTrans,&m_matProjAndScale);
	D3DXMatrixMultiply(&m_matTrans,&m_matTrans,&m_matScreen);
	return S_OK;
}

HRESULT cCamera3D::SetMatrix()
{
	HRESULT hr = 0;
	hr = GetDevice()->SetTransform(D3DTS_WORLD, &m_matWorld);	
	hr = GetDevice()->SetTransform(D3DTS_VIEW, &m_matView);
	hr = GetDevice()->SetTransform(D3DTS_PROJECTION, &m_matProjAndScale);	
	return hr;
}

void cCamera3D::ScreenToGround(float x,float y, float z,float &tx,float &ty, float &tz)
{
	D3DXVECTOR3 vPickRayDir;
    D3DXVECTOR3 vPickRayOrig;
	
	int nx,ny;
	GetGraphics()->GetScreen(nx,ny);
	float fx,fy;
	fx = nx;fy = ny;
	
	D3DXMatrixInverse( &m_matIView, NULL, &m_matView );
	D3DXMatrixInverse( &m_matIProjAndScale, NULL, &m_matProjAndScale );
	D3DXMatrixInverse( &m_matIScreen, NULL, &m_matScreen );
	
	D3DXVECTOR3 v3;
	v3.x = x;
	v3.y = y;
	v3.z = 0;
	
	D3DXVECTOR4 v4;
	D3DXVec3Transform(&v4,&v3,&m_matIScreen);
	v3.x = v4.x;
	v3.y = v4.y;
	v3.z = v4.z;
	D3DXVec3Transform(&v4,&v3,&m_matIProjAndScale);
	v3.x = v4.x;
	v3.y = v4.y;
	v3.z = v4.z;
	//D3DXVec3Transform(&v4,&v3,&m_matIView); 
	//for it is a ray so need not add view offset!
	vPickRayDir.x  = v3.x*m_matIView._11 + v3.y*m_matIView._21 + v3.z*m_matIView._31;
	vPickRayDir.y  = v3.x*m_matIView._12 + v3.y*m_matIView._22 + v3.z*m_matIView._32;
	vPickRayDir.z  = v3.x*m_matIView._13 + v3.y*m_matIView._23 + v3.z*m_matIView._33;
	vPickRayOrig.x = m_matIView._41;
	vPickRayOrig.y = m_matIView._42;
	vPickRayOrig.z = m_matIView._43;
	
	// 1' let z = 0, find the tile it intersect,
	// z0.z + k * zk.z = z
	if (vPickRayDir.z != 0.f)
	{
		float k = (z - vPickRayOrig.z) / vPickRayDir.z;
		tx = vPickRayOrig.x + k * vPickRayDir.x;
		ty = vPickRayOrig.y + k * vPickRayDir.y;
	}
	else
	{
		tx = vPickRayDir.x;
		ty = vPickRayDir.y;
	}
	tz = z;
}

void cCamera3D::GroundToScreen(float tx,float ty,float tz,float& x, float& y, float& z)
{
	D3DXVECTOR4 v4;
	D3DXVECTOR4 v3(tx,ty,tz,1);
	D3DXVec4Transform(&v4,&v3,&m_matTrans);
	x = v4.x/v4.w;	
	y = v4.y/v4.w;
	z = v4.z/v4.w;
}

/*
h = cos(fov/2) / sin(fov/2);
w = h / aspect;
m11=w m12=0 m13=0 m14=0
m21=0 m22=h m23=0 m24=0
m31=0 m32=0 m33=f/(f-n) m34=1
m41=0 m42=0 m43=0 m44=n*f/(n-f)
*/
