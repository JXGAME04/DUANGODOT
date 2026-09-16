// cSpecialSnow.cpp: implementation of the cSpecialSnow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cSpecialSnow.h"
#include "stringvalue.h"
#include "cCamera.h"
#include "cGround.h"
#include "cGraphics.h"
#include "position.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cSpecialSnow::cSpecialSnow(cGraphics* p)  : cSpecial(p)
{
	m_param.eElement = GT_SPECIAL_SNOW;
	strcpy(m_param.szImage,"system\\spr\\snow.spr");
	m_param.nImageFrames = 1;
	m_param.ptImageSize = CPoint(0,0);		//雪的图片一帧的大小,如此可以算出当前帧的贴图位置
	m_param.nImagespeed = 0;

	m_param.fAcceleration = -0.001f;
	m_param.fInitZ = 30.f;
	m_param.ptInitSpeed = cPoint(0.01f,0.01f,-0.1f);
	
	m_param.ePath = PATH_LINE;
	m_param.fSize = 5.f;
	m_param.fScale = 400.f;
	m_param.rcRange = cRect(-16.f,-8.f,16.f,32.f);
	m_param.color = cColor(255,255,255,255);

	m_param.nNumber = 128;
	m_param.nFrameNumber = 1;

	m_pTextureSnow = NULL;
	m_pVB = NULL;

	InitParam();
}

cSpecialSnow::~cSpecialSnow()
{
	RELEASE(m_pTextureSnow);
	RELEASE(m_pVB);
}

void cSpecialSnow::CreateBuffer()
{
	HRESULT hr;
	RELEASE(m_pVB);
	if(FAILED(hr = GetGraphics()->GetDevice3D()->CreateVertexBuffer( m_param.nNumber * sizeof(POINTVERTEX), 
													D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY | D3DUSAGE_POINTS, 
													D3DFVF_POINTVERTEX, D3DPOOL_DEFAULT, &m_pVB )))
	{
		DX_ERROR(hr);
	}
}

void cSpecialSnow::FindTexture()
{
	RELEASE(m_pTextureSnow);
	stResourceA stA;
	stA.stDevice.eMedia = MEDIA_VIDEO;
	stA.stDevice.eUsage = USAGE_EFFECT;
	stA.stRes.idResource = 0;
	strcpy(stA.stRes.szResource,m_param.szImage);
	m_pTextureSnow = GetGraphics()->FindTexture(stA);
	ASSERT(m_pTextureSnow);
}

//////////////////////////////////////////////////////////////////////
//string
//////////////////////////////////////////////////////////////////////
const CString c_aStrItem[] = 
{
	"elementtype",
	"image","imageframe","imagesize","imagespeed",
	"雪的加速度","从多高下落","雪的初始速度",
	"雪的运动轨迹","雪花大小","远近的缩放","雪的颜色","覆盖的范围",
	"雪花的数目","开始一帧下落多少雪花",
	"",
};

const CString* cSpecialSnow::GetItemStringArray()
{
	return c_aStrItem;
}

const CString c_aStrDescription[] = 
{
	"不能修改",
	"输入相对文件名称","int","int","int",
	"int","float","cpoint",
	"line,wave,swing","float","float","color","rect",
	"int","int",
	"",
};

const CString* cSpecialSnow::GetDescriptionStringArray()
{
	return c_aStrDescription;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialSnow::GetValue(int index, CString& strValue)
{
	int n = GT_SPECIAL_SNOW;
	switch (index)
	{
	case PARAM_ELEMENTTYPE:
		ValueToString(n,strValue);
		break;
	case PARAM_IMAGE:
		ValueToString(m_param.szImage,strValue);
		break;
	case PARAM_IMAGEFRAME:
		ValueToString(m_param.nImageFrames,strValue);
		break;
	case PARAM_IMAGESIZE:
		ValueToString(m_param.ptImageSize,strValue);
		break;
	case PARAM_IMAGESPEED:
		ValueToString(m_param.nImagespeed,strValue);
		break;

	case PARAM_ACCELERATION:
		ValueToString(m_param.fAcceleration,strValue);
		break;
	case PARAM_INITZ:
		ValueToString(m_param.fInitZ,strValue);
		break;
	case PARAM_INITSPEED:
		ValueToString(m_param.ptInitSpeed,strValue);
		break;

	case PARAM_PATH:
		ValueToString(cast_int(m_param.ePath),strValue);
		break;
	case PARAM_SIZE:
		ValueToString(m_param.fSize,strValue);
		break;
	case PARAM_SCALE:
		ValueToString(m_param.fScale,strValue);
		break;
	case PARAM_COLOR:
		ValueToString(m_param.color,strValue);
		break;
	case PARAM_RANGE:
		ValueToString(m_param.rcRange,strValue);
		break;

	case PARAM_NUMBER:
		ValueToString(m_param.nNumber,strValue);
		break;
	case PARAM_FRAMENUMBER:
		ValueToString(m_param.nFrameNumber,strValue);
		break;
	}
	return S_OK;
}

HRESULT	cSpecialSnow::SetParam(int index, LPCSTR szValue)
{
	CString strValue = szValue;
	switch (index)
	{
	case PARAM_IMAGE:
		{
			StringToValue(strValue,m_param.szImage);
			FindTexture();
		}
		break;
	case PARAM_IMAGEFRAME:
		StringToValue(strValue,m_param.nImageFrames);
		break;
	case PARAM_IMAGESIZE:
		StringToValue(strValue,m_param.ptImageSize);
		break;
	case PARAM_IMAGESPEED:
		StringToValue(strValue,m_param.nImagespeed);
		break;

	case PARAM_ACCELERATION:
		StringToValue(strValue,m_param.fAcceleration);
		break;
	case PARAM_INITZ:
		StringToValue(strValue,m_param.fInitZ);
		break;
	case PARAM_INITSPEED:
		StringToValue(strValue,m_param.ptInitSpeed);
		break;

	case PARAM_PATH:
		StringToValue(strValue,cast_int(m_param.ePath));
		break;
	case PARAM_SIZE:
		StringToValue(strValue,m_param.fSize);
		break;
	case PARAM_SCALE:
		StringToValue(strValue,m_param.fScale);
		break;
	case PARAM_COLOR:
		StringToValue(strValue,m_param.color);
		break;
	case PARAM_RANGE:
		StringToValue(strValue,m_param.rcRange);
		break;

	case PARAM_NUMBER:
		{
			StringToValue(strValue,m_param.nNumber);
			InitParam();
		}
		break;
	case PARAM_FRAMENUMBER:
		StringToValue(strValue,m_param.nFrameNumber);
		break;
	}

	return S_OK;
}
//////////////////////////////////////////////////////////////////////
//load binary
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialSnow::LoadBinary(CFile& f)
{
	f.Read(&m_param,sizeof(m_param));
	InitParam();
	return S_OK;
}

HRESULT	cSpecialSnow::SaveBinary(CFile& f)
{
	f.Write(&m_param,sizeof(m_param));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//run
//////////////////////////////////////////////////////////////////////
void cSpecialSnow::InitParam()
{
	m_aSnow.resize(m_param.nNumber);
	m_nStart = 0;
	cPoint pt = GetCamera()->GetFocus();
	pt.x += m_param.rcRange.l;
	pt.y += m_param.rcRange.t;
	int width = m_param.rcRange.Width();
	int height = m_param.rcRange.Height();

	for (int i=0; i<m_param.nNumber; i++)
	{
		m_aSnow[i].ptSpeed = m_param.ptInitSpeed; 
		m_aSnow[i].ptSpeed.x += m_aSnow[i].ptSpeed.x * (rand() % 10 - 5) / 10.f; 
		m_aSnow[i].ptSpeed.y += m_aSnow[i].ptSpeed.y * (rand() % 10 - 5) / 10.f; 
		m_aSnow[i].ptSpeed.z += m_aSnow[i].ptSpeed.z * (rand() % 10 - 5) / 10.f; 
		m_aSnow[i].ptPos = cPoint(pt.x + rand() % width - width / 2, pt.y + rand() % height - height / 2, m_param.fInitZ);
	}

	FindTexture();
	CreateBuffer();
}

void cSpecialSnow::RunSnow(stSnow& snow)
{
	snow.ptPos += snow.ptSpeed;
	snow.ptSpeed.z += m_param.fAcceleration;
	cPoint ptSpeed = GetGround()->GetWindSpeed();
	snow.ptPos += ptSpeed;

	if (snow.ptPos.z < 0)
	{
		int width = m_param.rcRange.Width();
		int height = m_param.rcRange.Height();
		cPoint pt = GetCamera()->GetFocus();
		snow.ptPos = cPoint(pt.x + rand() % width - width / 2, pt.y + rand() % height - height / 2, m_param.fInitZ);
//		snow.ptPos = cPoint(pt.x + rand() % width, pt.y + rand() % height, m_param.fInitZ);
		snow.ptSpeed = m_param.ptInitSpeed;
		return;
	}
	cRect& rc = m_rcGround;
	if (snow.ptPos.x < rc.l)
		snow.ptPos.x = rc.r;
	else if(snow.ptPos.x > rc.r)
		snow.ptPos.x = rc.l;
	if (snow.ptPos.y < rc.t)
		snow.ptPos.y = rc.b;
	else if(snow.ptPos.y > rc.b)
		snow.ptPos.y = rc.t;
}

BOOL cSpecialSnow::Active()
{
	m_bActive = TRUE;
	return m_bActive;
}

BOOL cSpecialSnow::Run()
{
	cPoint pt = GetCamera()->GetFocus();
	m_rcGround = cRect(pt.x + m_param.rcRange.l, pt.y + m_param.rcRange.t,
		pt.x + m_param.rcRange.r, pt.y + m_param.rcRange.b);
	for (int i=0; i<min(m_nStart,m_aSnow.size()); i++)
	{
		RunSnow(m_aSnow[i]);		
	}
	m_nStart += m_param.nFrameNumber;
	if (m_nStart > m_aSnow.size())
		m_nStart = m_aSnow.size();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
//draw
//////////////////////////////////////////////////////////////////////
BOOL cSpecialSnow::IsDraw(DrawEnum e) //是否绘制
{
	if ( e == SD_DRAWAFTERALL)
		return TRUE;
	return FALSE;
}


HRESULT cSpecialSnow::Draw(DrawEnum e)
{
	ASSERT(e == SD_DRAWAFTERALL);

	ASSERT(m_pVB);

	LPDIRECT3DDEVICE8 pd3dDevice = GetGraphics()->GetDevice3D();
	HRESULT hr;

	float fdis = 320.f-m_param.rcRange.b/sqrt(3)*2.f;
	float scale = m_param.fScale;
	pd3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  TRUE );
    pd3dDevice->SetRenderState( D3DRS_POINTSIZE,     FtoDW(m_param.fSize) );
    pd3dDevice->SetRenderState( D3DRS_POINTSIZE_MIN, FtoDW(m_param.fSize/2) );
    pd3dDevice->SetRenderState( D3DRS_POINTSIZE_MAX, FtoDW(m_param.fSize*2) );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALE_A,  FtoDW(fdis*fdis * scale) );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALE_B,  FtoDW(-fdis*2 * scale) );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALE_C,  FtoDW(1.f * scale) );

    pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(POINTVERTEX) );
    pd3dDevice->SetVertexShader( D3DFVF_POINTVERTEX );

	POINTVERTEX* pVertex;
	hr = m_pVB->Lock(0,0,(BYTE**)&pVertex,0);

	for (int i=0; i<m_param.nNumber; i++)
	{
		pVertex->color = m_param.color.Value();
		pVertex->v.x = m_aSnow[i].ptPos.x;
		pVertex->v.y = m_aSnow[i].ptPos.y;
		pVertex->v.z = m_aSnow[i].ptPos.z;
		pVertex++;

//		SetPoint(*pVertex,m_aSnow[i].ptPos);
	}
	hr = m_pVB->Unlock();

	GetGround()->SetTexture(m_pTextureSnow,0);

    if(FAILED(hr = pd3dDevice->DrawPrimitive( D3DPT_POINTLIST, 0, m_param.nNumber)))
		return hr;


    pd3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  FALSE );
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//device
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialSnow::RestoreDeviceObjects()
{
	CreateBuffer();
	return S_OK;
}

HRESULT	cSpecialSnow::InvalidateDeviceObjects()
{
	RELEASE(m_pVB);
	return S_OK;
};

HRESULT	cSpecialSnow::DeleteDeviceObjects()
{
	RELEASE(m_pVB);
	return S_OK;
};

HRESULT	cSpecialSnow::FinalCleanup()
{
	RELEASE(m_pVB);
	return S_OK;
};
