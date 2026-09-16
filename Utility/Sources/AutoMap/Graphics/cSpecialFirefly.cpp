// cSpecialFirefly.cpp: implementation of the cSpecialFirefly class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGraphics.h"
#include "cGround.h"
#include "cCamera.h"
#include "stringvalue.h"
#include "cSpecialFirefly.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cSpecialFirefly::cSpecialFirefly(cGraphics* p) : cSpecial(p)
{
	m_param.eElement = GT_SPECIAL_FIREFLY;

	strcpy(m_param.szImage,"system\\spr\\fly.spr");
	m_param.fSize = 128.f;
	m_param.fScale = 400.f;
	m_param.ptFocus = cPoint(0,0);
	m_param.rcRange = cRect(-16.f,-32.f,16.f,32.f);
	m_param.color = cColor(255,255,255,255);
	m_param.fMinZ = 0.5f;
	m_param.fMaxZ = 2.5f;
	
	m_param.nNumber = 32;
	
	m_pTextureSnow = NULL;
	m_pVB = NULL;

	InitParam();
}

cSpecialFirefly::~cSpecialFirefly()
{
	RELEASE(m_pTextureSnow);
	RELEASE(m_pVB);
}

//buffer and texture
void cSpecialFirefly::CreateBuffer()
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

void cSpecialFirefly::FindTexture()
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
//load binary
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialFirefly::LoadBinary(CFile& f)
{
	f.Read(&m_param,sizeof(m_param));
	InitParam();
	return S_OK;
}

HRESULT	cSpecialFirefly::SaveBinary(CFile& f)
{
	f.Write(&m_param,sizeof(m_param));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//device
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialFirefly::RestoreDeviceObjects()
{
	CreateBuffer();
	return S_OK;
}

HRESULT	cSpecialFirefly::InvalidateDeviceObjects()
{
	RELEASE(m_pVB);
	return S_OK;
};

HRESULT	cSpecialFirefly::DeleteDeviceObjects()
{
	RELEASE(m_pVB);
	return S_OK;
};

HRESULT	cSpecialFirefly::FinalCleanup()
{
	RELEASE(m_pVB);
	return S_OK;
};

//////////////////////////////////////////////////////////////////////
//string
//////////////////////////////////////////////////////////////////////
const CString c_aStrItem[] = 
{
	"elementtype",
	"image","数目","最低","最高",
	"雪花大小","远近的缩放","颜色","位置","覆盖的范围",
	"",
};

const CString* cSpecialFirefly::GetItemStringArray()
{
	return c_aStrItem;
}

const CString c_aStrDescription[] = 
{
	"不能修改",
	"输入相对文件名称","int","float","float",
	"float","float","color","point","rect",
	"",
};

const CString* cSpecialFirefly::GetDescriptionStringArray()
{
	return c_aStrDescription;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialFirefly::GetValue(int index, CString& strValue)
{
	int n = GT_SPECIAL_FIREFLY;
	switch (index)
	{
	case PARAM_ELEMENTTYPE:
		ValueToString(n,strValue);
		break;
	case PARAM_IMAGE:
		ValueToString(m_param.szImage,strValue);
		break;
	case PARAM_NUMBER:
		ValueToString(m_param.nNumber,strValue);
		break;
	case PARAM_MINZ:
		ValueToString(m_param.fMinZ,strValue);
		break;
	case PARAM_MAXZ:
		ValueToString(m_param.fMaxZ,strValue);
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
	case PARAM_FOCUS:
		ValueToString(m_param.ptFocus,strValue);
		break;
	case PARAM_RANGE:
		ValueToString(m_param.rcRange,strValue);
		break;
	}

	return S_OK;
}

HRESULT	cSpecialFirefly::SetParam(int index, LPCSTR szValue)
{
	CString strValue = szValue;

	switch (index)
	{
	case PARAM_IMAGE:
		{
			StringToValue(strValue,m_param.szImage);
		}
		break;
	case PARAM_NUMBER:
		{
			StringToValue(strValue,m_param.nNumber);
		}
		break;
	case PARAM_MINZ:
		StringToValue(strValue,m_param.fMinZ);
		break;
	case PARAM_MAXZ:
		StringToValue(strValue,m_param.fMaxZ);
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
	case PARAM_FOCUS:
		StringToValue(strValue,m_param.ptFocus);
		break;
	case PARAM_RANGE:
		StringToValue(strValue,m_param.rcRange);
		break;
	}
	InitParam();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//run
//////////////////////////////////////////////////////////////////////
void cSpecialFirefly::InitParam()
{
	m_aFly.resize(m_param.nNumber);
	cPoint pt = cPoint(m_param.rcRange.l,m_param.rcRange.t,m_param.fMinZ);
	pt += m_param.ptFocus;
	float fmul = 100.f;
	int width = m_param.rcRange.Width()*fmul;
	int height = m_param.rcRange.Height()*fmul;
	int z = (m_param.fMaxZ - m_param.fMinZ)*fmul;

	for (int i=0; i<m_param.nNumber; i++)
	{
		m_aFly[i].ptPos = cPoint(pt.x + rand() % width / fmul , pt.y + rand() % height / fmul , pt.z + (rand() % z)/fmul);
		m_aFly[i].nFlash = rand() % 5 + 5;
		m_aFly[i].nFrame = 0;
		m_aFly[i].bAddAlpha = TRUE;
		m_aFly[i].nAlpha = 0;
		m_aFly[i].nStatus = STATUS_MOVE_SLIGHT;
		m_aFly[i].fSpeed = (rand() % 2 + 2.f)/10.f;
		m_aFly[i].fSpeedVerMul = ((rand() % 3)/10.f + 0.2f)/10.f;
		m_aFly[i].fSpeedDiv = sqrt(m_aFly[i].fSpeedVerMul * m_aFly[i].fSpeedVerMul + 1);
	}

	FindTexture();
	CreateBuffer();
}

BOOL cSpecialFirefly::Active()
{
	cRect rc = GetGround()->GetRectGround();
	cRect r = rc & m_param.rcRange;
	m_bActive = r.IsEmpty();
	return m_bActive;
}

BOOL cSpecialFirefly::Run()
{
	m_ptFocus = GetCamera()->GetFocus();
	for (int i=0; i<m_aFly.size(); i++)
		RunSnow(m_aFly[i]);		
	return FALSE;
}

void cSpecialFirefly::RunSnow(stFly& fly)			//单个的snow运动
{
	switch (fly.nStatus)
	{
	case STATUS_MOVE_SLIGHT:
		fly.nFrame++;
		if (fly.nFrame >= fly.nFlash)
		{
			fly.nFrame = 0;
			if (fly.bAddAlpha)
			{
				fly.nAlpha += 16;
				if (fly.nAlpha >= 255)
				{
					fly.nAlpha = 255;
					fly.bAddAlpha = FALSE;
				}
			}
			else
			{
				fly.nAlpha -= 16;
				if (fly.nAlpha < 0)
				{
					fly.nAlpha = 0;
					fly.bAddAlpha = TRUE;
				}
			}
		}
		if (fly.bAddAlpha)
		{
			fly.ptPos.z += 0.002f;
		}
		else
		{
			fly.ptPos.z -= 0.002f;
		}		
		
		{
			cPoint pt = fly.ptPos - m_ptFocus;
			if (abs(pt.x) + abs(pt.y) < 2.f) 
			{
				fly.nFrame = 0;
				fly.nStatus = STATUS_MOVE;
				float a = ( rand() % 360 ) / 360.f * 2 * c_fPI;
				fly.ptSpeed = cPoint(fly.fSpeed*sin(a),fly.fSpeed*cos(a),0);
			}
		}
		break;
	case STATUS_MOVE:
		{
			fly.nFrame++;
			if (fly.nFrame % 2)
			{
				fly.nAlpha = 0;
			}
			else
				fly.nAlpha = 255;

			cPoint ptSpeed;
			//compute the speed by ptSpeed and verSpeed;
			{
				cPoint vVer = cPoint(fly.ptSpeed.y * fly.fSpeedVerMul,-fly.ptSpeed.x * fly.fSpeedVerMul,0);
				float dot = fly.ptSpeed.x * vVer.y - fly.ptSpeed.y * vVer.x;
				if (dot < 0)
				{
					vVer.x = -vVer.x;
					vVer.y = -vVer.y;
				}
				fly.ptSpeed = fly.ptSpeed + vVer;
				fly.ptSpeed.x /= fly.fSpeedDiv;
				fly.ptSpeed.y /= fly.fSpeedDiv;
				fly.ptSpeed.z = 0;
			}
			fly.ptPos += fly.ptSpeed;

			cPoint pt = fly.ptPos - fly.ptInitPos;
			if (fly.nFrame > 1000 || abs(pt.x) + abs(pt.y) > 16.f) 
			{
				fly.nStatus = STATUS_MOVE_SLIGHT;
				fly.ptInitPos = fly.ptPos;
				fly.nFrame = 0;
				fly.bAddAlpha = TRUE;
				fly.nAlpha = 0;
			}
		}
		break;
	}
}
//////////////////////////////////////////////////////////////////////
//draw
//////////////////////////////////////////////////////////////////////
cPoint cSpecialFirefly::GetDrawPoint()
{
	return m_param.ptFocus;
}

BOOL cSpecialFirefly::IsDraw(DrawEnum e) //是否绘制
{
	if ( e == SD_DRAWAFTERALL)
		return TRUE;
	return FALSE;
}

HRESULT	cSpecialFirefly::Draw(DrawEnum e)
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

	cColor cr = m_param.color.Value();
	for (int i=0; i<m_param.nNumber; i++)
	{
		cr.a = m_aFly[i].nAlpha;
		pVertex->color = cr.Value();
		pVertex->v.x = m_aFly[i].ptPos.x;
		pVertex->v.y = m_aFly[i].ptPos.y;
		pVertex->v.z = m_aFly[i].ptPos.z;
		pVertex++;

//		SetPoint(*pVertex,m_aFly[i].ptPos);
	}
	m_pVB->Unlock();

	GetGround()->SetTexture(m_pTextureSnow,0);

    if(FAILED(hr = pd3dDevice->DrawPrimitive( D3DPT_POINTLIST, 0, m_param.nNumber)))
		return hr;

    pd3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
    pd3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  FALSE );
	return S_OK;
}
