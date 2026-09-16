// cCamera.cpp: implementation of the cCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCamera.h"

#include "Graphics.h"
#include "cGraphics.h"

#include "cGround.h"
#include "iItem.h"

#include <math.h>

#include "iZip.h"

#include "planedoodad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCamera::cCamera(cGraphics* p)
{
	m_pGraphics = p;
	m_ulCount = 1;
	ZeroMemory(&m_param,sizeof(m_param));

	m_bNotify = false;
	m_param.fAngleX = 0.f;
}

cCamera::~cCamera()
{
}

/**************************************************************************/
//com interface
/**************************************************************************/
ULONG cCamera::AddRef() 
{
	return ++m_ulCount;
}

ULONG cCamera::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

/**************************************************************************/
//Graphics
/**************************************************************************/

cGround* cCamera::GetGround()
{
	return GetGraphics()->GetcGround();
}

/**************************************************************************/
//begin and end
/**************************************************************************/

HRESULT cCamera::Begin()
{
	Init();
	return 0;
}

HRESULT cCamera::End()
{
	return 0;
}

/**************************************************************************/
//param	
/**************************************************************************/

HRESULT cCamera::GetParam(stParam& param) 
{
	if (param.mask == 0)
		param = m_param ;
	return 0;
}

ULONG cCamera::UpdateCamera()
{
	SetViewParams( D3DXVECTOR3(m_param.ptEye.x,m_param.ptEye.y,m_param.ptEye.z), 
		D3DXVECTOR3(m_param.ptTo.x,m_param.ptTo.y,m_param.ptTo.z),
		D3DXVECTOR3(m_param.ptUp.x,m_param.ptUp.y,m_param.ptUp.z));
	SetProjParams(m_param.fFOV  , m_param.fAspect , m_param.fNearPlane , m_param.fFarPlane );	
	SetScaleParams(m_param.fScale);

	UpdateMatrix();
	if (m_bNotify)
		GetGround()->CameraNotify(this,NOTIFY_POSITION_CHANGED,0);
	
	return 0;
}

HRESULT cCamera::setParam(stParam& param) 
{
	bool bUpdate;
	if (param.mask & MASK_EYE || param.mask & MASK_TO || param.mask & MASK_UP ||
		param.mask & MASK_ANGLEX || param.mask & MASK_ANGLE || param.mask & MASK_DIS )
	{
		if (param.mask & MASK_EYE)
		{
			m_param.ptEye = param.ptEye;
		}
		if (param.mask & MASK_TO)
		{
			m_param.ptTo = param.ptTo;
		}
		if (param.mask & MASK_UP)
		{
			m_param.ptUp = param.ptUp;
		}
		if (param.mask & MASK_ANGLE)
		{
			if (param.fAngle <= 0 || param.fAngle >= 90)
				return 0;
			m_param.fAngle= param.fAngle;
//			GetGraphics()->SetMulXY(1.f,1.f/sin(m_param.fAngle*D3DX_PI/180.f));
		}
		if (param.mask & MASK_ANGLEX)
		{
			if (param.fAngleX < -45 || param.fAngleX > 45)
				return 0;
			m_param.fAngleX= param.fAngleX;
		}
		if (param.mask & MASK_DIS)
		{
			if (param.fDis <= 0)
				return 0;
			m_param.fDis= param.fDis;
		}
		if (param.mask & MASK_ANGLEX || param.mask & MASK_ANGLE || param.mask & MASK_DIS || param.mask & MASK_TO)
		{
			float fx = m_param.ptTo.x + m_param.fDis * cos(m_param.fAngle * D3DX_PI / 180) * sin(m_param.fAngleX * D3DX_PI / 180) ;
			float fy = m_param.ptTo.y + m_param.fDis * cos(m_param.fAngle * D3DX_PI / 180) * cos(m_param.fAngleX * D3DX_PI / 180) ;
			float fz = m_param.ptTo.z + m_param.fDis * sin(m_param.fAngle * D3DX_PI / 180);
			m_param.ptEye = cPoint(fx,fy,fz);
		}
		bUpdate = true;
	}

	if (param.mask & MASK_FOV || param.mask & MASK_ASPECT || 
		param.mask & MASK_NEARPLANE || param.mask & MASK_FARPLANE
		)
	{
		if (param.mask & MASK_FOV)
		{
			m_param.fFOV = param.fFOV;
		}
		if (param.mask & MASK_ASPECT)
		{
			m_param.fAspect = param.fAspect;
		}
		if (param.mask & MASK_NEARPLANE)
		{
			m_param.fNearPlane = param.fNearPlane;
		}
		if (param.mask & MASK_FARPLANE)
		{
			m_param.fFarPlane = param.fFarPlane;
		}
		bUpdate = true;
	}

	if (param.mask & MASK_SCALE)
	{
		m_param.fScale = param.fScale;
		bUpdate = true;
	}

	if (param.mask & MASK_COMMAND)
	{
		switch(param.eCommand)
		{
		case CCOM_UPDATE:
			UpdateCamera();
			break;
		case CCOM_LOAD:
			Load(param.szFile);
			break;
		case CCOM_SAVE:
			Save(param.szFile);
			break;
		}
	}

	return 0;
}

HRESULT cCamera::SetParam(stParam& param, bool bRunNow)
{
	if (bRunNow)
		return setParam(param);
	AddRef();
	param.wParam = (long)this;
	param.lParam = 0;
	GetGraphics()->PushMessage(&param,sizeof(param),GT_CAMERA);
	return 0;
}

/**************************************************************************/
//trace
/**************************************************************************/
void cCamera::Trace()
{
	char sMsg[128];
	sprintf(sMsg,"(F4)Camera: From(%.1f,%.1f,%.1f) To(%.1f,%.1f,%.1f) AngleXY(%.1f) AngleZ(%.1f) Scale(%.1f)",
		m_param.ptEye.x,m_param.ptEye.y,m_param.ptEye.z,
		m_param.ptTo.x,m_param.ptTo.y,m_param.ptTo.z,
		m_param.fAngleX,m_param.fAngle,m_param.fScale
		);

	GetGraphics()->Trace(sMsg);
}

/**************************************************************************/
//save and load
/**************************************************************************/
void cCamera::Init()
{
	m_bNotify = false;

	iCamera::stParam param;
	param.mask = iCamera::MASK_ANGLE | iCamera::MASK_DIS | iCamera::MASK_TO | iCamera::MASK_SCALE;
	param.fAngle = 30.f;
	param.fDis = 32.f;
	param.fScale = 1.f;
	//for i dont how to decrease the effect of the prespecive,so choose a stupid way
	param.fDis = 32.f * 10.f;
	param.fScale = 1.f * 14.2f;
	
//	param.fDis = 2700.0f;
	//pay attention to this!
	//D3DXMatrixScaling(&m_matScale,fScale,fScale,1.f);

	param.ptTo.x = 1600.f;
	param.ptTo.y = 3200.f;
	param.ptTo.z = 0.f;

	setParam(param);	

	UpdateCamera();
}

HRESULT cCamera::New()
{
	m_bNotify = false;
	Init();
	SetMatrix();
	m_bNotify = true;
	GetGround()->CameraNotify(this,NOTIFY_POSITION_CHANGED,0);
	
	return S_OK;
}

HRESULT cCamera::Save(LPCSTR szFile)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"eye",m_param.ptEye);
	SetItemValue(pItem,"to",m_param.ptTo);
	SetItemValue(pItem,"scale",m_param.fScale);
	pItem->Save();
	return S_OK;
}

HRESULT cCamera::Load(LPCSTR szFile)
{
	iZipRead* pZip = GetGround()->GetZip();
	if (pZip)
	{
		if (pZip->Open(PKFILE_CAMERA))
		{
			stParam param;
			pZip->Read(&param,sizeof(param));
			m_param.ptEye = param.ptEye;
			m_param.ptTo = param.ptTo;
			m_param.fScale = param.fScale;
		}
	}
	else
	{
		iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
		if (pItem != NULL)
		{
			wrap_ptr<iItem> ptr(pItem);
			GetItemValue(pItem,"eye",m_param.ptEye);
			GetItemValue(pItem,"to",m_param.ptTo);
			GetItemValue(pItem,"scale",m_param.fScale);
		}
	}

	float fx = m_param.ptTo.x + m_param.fDis * cos(m_param.fAngle * D3DX_PI / 180) * sin(m_param.fAngleX * D3DX_PI / 180) ;
	float fy = m_param.ptTo.y + m_param.fDis * cos(m_param.fAngle * D3DX_PI / 180) * cos(m_param.fAngleX * D3DX_PI / 180) ;
	float fz = m_param.ptTo.z + m_param.fDis * sin(m_param.fAngle * D3DX_PI / 180);
	m_param.ptEye = cPoint(fx,fy,fz);

	m_bNotify = false;
//	Init();
	UpdateCamera();
	SetMatrix();
	m_bNotify = true;
	GetGround()->CameraNotify(this,NOTIFY_POSITION_CHANGED,0);
	return S_OK;
}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/

void cCamera::GroundToScreen(cPoint p,cPoint& pt)
{
	GroundToScreen(p.x,p.y,p.z,pt.x,pt.y,pt.z);
}

void cCamera::GroundToScreen(stVertexGround p,stVertexGround& pt) 
{
	GroundToScreen(p.sx,p.sy,p.sz,pt.sx,pt.sy,pt.sz);	
}

void cCamera::GroundToScreen(cPoint p,CPoint& pt)
{
	cPoint p2(pt.x,pt.y,0.f);
	GroundToScreen(p,p2);
	pt.x = p2.x;
	pt.y = p2.y;
}

void cCamera::GroundToScreen(cPoint& pt)
{
	GroundToScreen(pt,pt);
}

void cCamera::ScreenToGround(cPoint p,cPoint& pt)
{
	ScreenToGround(p.x,p.y,p.z,pt.x,pt.y,pt.z);
}

void cCamera::ScreenToGround(stVertexGround p,stVertexGround& pt) 
{
	ScreenToGround(p.sx,p.sy,p.sz,pt.sx,pt.sy,pt.sz);
}

void cCamera::ScreenToGround(CPoint p,cPoint& pt) 
{
	cPoint p2(p.x,p.y,0.f);
	ScreenToGround(p2,pt);
}

void cCamera::ScreenToGround(cPoint& pt)
{
	ScreenToGround(pt,pt);
}

bool cCamera::IsPointInScreen(cPoint p)
{
	cPoint pt;
	GroundToScreen(p,pt);
	int x,y;
	GetGraphics()->GetScreen(x,y);
	return pt.x >= 0 && pt.x <= x && pt.y >= 0 && pt.y <= y;
}