// cLight.cpp: implementation of the cLight class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cLight.h"
//#include "cTile.h"
#include "cGround.h"

//#include "cLine.h"
//#include "cPlane.h"
#include "cGraphics.h"
#include "cCamera.h"

#include "cBNode.h"
#include "iItem.h"

#include "position.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cLight::cLight(cGraphics* p) : cGroundObj(p)
{
	m_nFrame = 0;
	m_bEnable = TRUE;
	m_bEnlarge = TRUE;
	m_bOut = TRUE;
}

cLight::~cLight()
{
}

void cLight::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stLightParam;	
	cGroundObj::Init(false);
	m_pParam->eElement = GT_LIGHT;
}

/**************************************************************************/
//save and load	
/**************************************************************************/
HRESULT	cLight::LoadPack(void** pParam)
{
	HRESULT hr = cGroundObj::LoadPack(pParam);
	if (FAILED(hr))
		return hr;
	m_fRangeSquare = GetRange() * GetRange();
	m_fInitRange = GetRange();
	return S_OK;
}

HRESULT cLight::LoadBinary(CFile& file)
{
	HRESULT hr = cGroundObj::LoadBinary(file);
	if (FAILED(hr))
		return hr;
	m_fRangeSquare = GetRange() * GetRange();
	m_fInitRange = GetRange();
	return S_OK;
}

HRESULT cLight::Load(stSection* p)
{
	cGroundObj::Load(p);

	if (p != NULL)
	{
		stLightParam& m_param = *GetLightParam();
		GetSectionValue(p,"range",m_param.fRange);
		GetSectionValue(p,"mainlight",m_param.bMainlight);

		GetSectionValue(p,"varrangemax",m_param.fRangeVarMax);
		GetSectionValue(p,"varrangespeed",m_param.fRangeVarSpeed);
		GetSectionValue(p,"varrangeframe",m_param.fRangeVarFrame);

		GetSectionValue(p,"posjumpmax",m_param.fPosJumpMax);
		GetSectionValue(p,"posjumpspeed",m_param.ptPosJumpSpeed);
		GetSectionValue(p,"posjumpframe",m_param.fPosJumpFrame);

		GetSectionValue(p,"frameflash",m_param.fFlashFrame);
		GetSectionValue(p,"framedark",m_param.nFlashDarkFrame);
	}
	m_fRangeSquare = GetRange() * GetRange();
	m_fInitRange = GetRange();
	
	return S_OK;
}

HRESULT cLight::Save(stSection* p)
{
	cPoint pt = GetOffset();
	SetOffset(m_ptInitPos);
	cGroundObj::Save(p);
	SetOffset(pt);

	stLightParam& m_param = *GetLightParam();
	SetSectionValue(p,"range",m_fInitRange);
	SetSectionValue(p,"mainlight",m_param.bMainlight);

	SetSectionValue(p,"varrangemax",m_param.fRangeVarMax);
	SetSectionValue(p,"varrangespeed",m_param.fRangeVarSpeed);
	SetSectionValue(p,"varrangeframe",m_param.fRangeVarFrame);

	SetSectionValue(p,"posjumpmax",m_param.fPosJumpMax);
	SetSectionValue(p,"posjumpspeed",m_param.ptPosJumpSpeed);
	SetSectionValue(p,"posjumpframe",m_param.fPosJumpFrame);

	SetSectionValue(p,"frameflash",m_param.fFlashFrame);
	SetSectionValue(p,"framedark",m_param.nFlashDarkFrame);

	return S_OK;
}

/**************************************************************************/
//param	
/**************************************************************************/
HRESULT cLight::OnCommand(long eCommand, stObjParam& param)
{
	cGroundObj::OnCommand(eCommand,param);
	return 0;
}


HRESULT cLight::setParam(stObjParam& pm)
{
	HRESULT hr = cGroundObj::setParam(pm);

	if (!pm.IsEMask(MASK_LIGHT))
		return hr;

	stLightParam& param = *((stLightParam* )(&pm));
	stLightParam& m_param = *GetLightParam();

	if (param.IsSetMask(stLightParam::MASK_LIGHT_RANGE))
	{
		m_param.fRange = param.fRange ;
		if (m_param.fRange > MAX_LIGHT_RADIUS)
			m_param.fRange = MAX_LIGHT_RADIUS;
		m_fRangeSquare = m_param.fRange*m_param.fRange;
		m_fInitRange = m_param.fRange;
	}
	if (param.IsSetMask(stLightParam::MASK_LIGHT_VARRANGEMAX))
		m_param.fRangeVarMax = param.fRangeVarMax;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_VARRANGESPEED))
		m_param.fRangeVarSpeed = param.fRangeVarSpeed;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_VARRANGEFRAME))
		m_param.fRangeVarFrame = param.fRangeVarFrame;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_POSJUMPMAX))
		m_param.fPosJumpMax = param.fPosJumpMax;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_POSJUMPSPEED))
		m_param.ptPosJumpSpeed = param.ptPosJumpSpeed;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_POSJUMPFRAME))
		m_param.fPosJumpFrame = param.fPosJumpFrame;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_FLASHDARKFRAME))
		m_param.nFlashDarkFrame = param.nFlashDarkFrame;
	if (param.IsSetMask(stLightParam::MASK_LIGHT_FLASHFRAME))
		m_param.fFlashFrame = param.fFlashFrame;
	return S_OK;
}

void cLight::GetSortPoint(cPoint& pt)
{
	pt = GetOffset();
}

/**************************************************************************/
//光照计算
/**************************************************************************/

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/

/**************************************************************************/
//utility
/**************************************************************************/
bool cLight::CheckDisplay(const cRect* pRc)
{
	cRect rc = *pRc;
	cRect r;
	GetLightRect(r);
	cRect rect = r & rc;
	m_bDisplay = !(rect.IsEmpty());
	return m_bDisplay;
}

BOOL cLight::IsShow()
{
	return 	(GetGraphics()->IsShow(iGraphics::GS_LIGHT) && IsDisplay());	
}

HRESULT cLight::Draw()
{
	cPoint pt = GetOffset();
	cRect r;
	const float f = 0.1f;
	r.l = pt.x - f;
	r.t = pt.y - f*2;
	r.r = pt.x + f;
	r.b = pt.y + f*2;
	cColor cr = GetColor();
	long brightness = 125;
	if (GetMapedit() & Mapedit_Select)
		brightness = 255;
	cr.a = brightness;

	GetGround()->DrawRect(r,cr,GetOffset().z);
	if (GetMapedit() & Mapedit_Trace)
	{
		char sMsg[128];
		sprintf(sMsg,"(F5)Light: pos(%.1f,%.1f,%.1f) Color(%d,%d,%d,%d) Range(%.1f)",
			GetOffset().x,GetOffset().y,GetOffset().z,
			GetColor().r,GetColor().g,GetColor().b,GetColor().a,
			GetRange()
			);
		GetGraphics()->Trace(sMsg);
	}
	return S_OK;
}

void cLight::GetLightRect(cRect& r)
{
	r = cRect(GetOffset().x - GetRange(), GetOffset().y - GetRange(),
		GetOffset().x + GetRange(), GetOffset().y + GetRange()
		);
}

///////////////////////////////////////////////////////
//Contain
///////////////////////////////////////////////////////

BOOL cLight::Contain(POINT ptScreen)
{
	if (!IsShow())
		return FALSE;
	cPoint ptGround(ptScreen.x,ptScreen.y,GetOffset().z);
	GetCamera()->ScreenToGround(ptGround);
	return Contain(ptGround);
}

BOOL cLight::Contain(cPoint ptGround)
{
	if (!IsShow())
		return FALSE;
	return Distance(ptGround,GetOffset()) < .2f;
}

BOOL cLight::ContainedScreen(RECT rc)
{
	return FALSE;
}

BOOL cLight::ContainedGround(cRect rc)
{
	return FALSE;
}

BOOL cLight::ContainedDiamond(cRect rc)
{
	if (!IsShow())
		return FALSE;
	cPoint p1 = GetOffset();
	GroundToTile(p1);
	TileToDiamond(p1);
	if (!IsPointInRect(p1,rc))
		return FALSE;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//light
//////////////////////////////////////////////////////////////////////////////
bool cLight::BeginLighting(cColor cr)
{
	m_nFrame++;
	stLightParam& m_param = *GetLightParam();
	if (m_param.fRangeVarFrame != 0)
	{
		int n = m_param.fRangeVarFrame;
		if (m_nFrame % n == 0)
		{
			if (m_bEnlarge)
			{
				m_param.fRange += m_param.fRangeVarSpeed;
				if (m_param.fRange >= MAX_LIGHT_RADIUS)
				{
					m_bEnlarge = FALSE;
					m_param.fRange = MAX_LIGHT_RADIUS;
				}
				if (m_param.fRange >= m_param.fRangeVarMax)
					m_bEnlarge = FALSE;
			}
			else
			{
				m_param.fRange -= m_param.fRangeVarSpeed;
				if (m_param.fRange <= m_fInitRange)
					m_bEnlarge = TRUE;
			}
		}
	}
	if (m_param.fPosJumpFrame != 0)
	{
		int n = m_param.fPosJumpFrame;
		if (m_nFrame % n == 0)
		{
			if (m_bOut)
			{
				m_param.ptOffset += m_param.ptPosJumpSpeed;
				if (DistanceMax(m_param.ptOffset, m_ptInitPos) > m_param.fPosJumpMax)
					m_bOut = FALSE;
			}
			else
			{
				m_param.ptOffset -= m_param.ptPosJumpSpeed;
				if (DistanceMax(m_param.ptOffset, m_ptInitPos) < 0.1f)
					m_bOut = TRUE;
			}
		}
	}
	
	if (m_param.fFlashFrame != 0)
	{
		if (IsEnable())
		{
			int n = m_param.fFlashFrame;
			if (m_nFrame % n == 0)
			{
				m_nDarkStartFrame = m_nFrame;
				EnableLight(FALSE);
			}		
		}
		else
		{
			if (m_nDarkStartFrame + m_param.nFlashDarkFrame < m_nFrame)
			{
				EnableLight(TRUE);
			}
		}
	}

	//light ai
	return true;
}

bool cLight::Lighting()
{
	return true;
}

bool cLight::EndLighting()
{
	return true;
}

bool cLight::CrossAbstacle(const cPoint& pt, BOOL bStrict)
{
	return false;
}

bool cLight::InRange(const cPoint& p)
{
	return DistanceSquare(p,GetOffset()) < GetRangeSquare();
}

bool cLight::CanLight(const cPoint& p, BOOL bStrict)
{
	if (!InRange(p))
		return false;
	if (CrossAbstacle(p,bStrict))
		return false;
	return true;
}

bool cLight::CanLight(const stVertexGround& v, BOOL bStrict)
{
	cPoint p = GetPoint(v);
	return CanLight(p,bStrict);
}

void cLight::Lighting(const cPoint& p, cColor& cr)
{
	float fDis = DistanceSquare(p,GetOffset());
	if (fDis < m_fRangeSquare)
	{
		cr += GetColor() * (1 - fDis / (m_fRangeSquare) ); 
		cr.Format();
	}
}

void cLight::Lighting(const stVertexGround& v, cColor& cr)
{
	cPoint p = GetPoint(v);
	Lighting(p,cr);
}

bool cLight::Light(const cPoint& p, cColor& cr, BOOL bStrict)
{
	if (!CanLight(p,bStrict))
		return false;
	//光线强度的计算公式，忘记了
	Lighting(p,cr);
	return true;
}

bool cLight::Light(const stVertexGround& v, cColor& cr, BOOL bStrict)
{
	cPoint p = GetPoint(v);
	return Light(p,cr,bStrict);
}

