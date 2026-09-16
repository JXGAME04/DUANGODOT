#include "stdafx.h"
#include "cselect.h"

#include "Graphics.h"
#include "cGround.h"
#include "cCamera.h"

#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//construction
//////////////////////////////////////////////////////////////////////////

cSelect::cSelect(cGraphics* p) : cGroundObj(p)
{
}

cSelect::~cSelect()
{

}

void cSelect::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stSelectParam;
	cGroundObj::Init(false);
}

HRESULT cSelect::setParam(stObjParam& pm)
{
	HRESULT hr = cGroundObj::setParam(pm);
	if (!pm.IsEMask(MASK_SELECT))
		return hr;

	stSelectParam& param = *((stSelectParam* )&pm);
	stSelectParam& m_param = GetSelectParam();

	if (param.IsSetMask(stSelectParam::MASK_SELECT_SIZE))
	{
		m_param.wSelect = param.wSelect;
		m_param.hSelect = param.hSelect;		
	}
	if (param.IsSetMask(stSelectParam::MASK_SELECT_OFFSETEND))
		m_param.ptOffsetEnd = param.ptOffsetEnd;
	if (param.IsSetMask(stSelectParam::MASK_SELECT_TYPE))
		m_param.eSelect = param.eSelect;
	return 0;
}

bool cSelect::CheckDisplay(const cRect* rc) 
{
	m_bDisplay = true;
	return m_bDisplay;
}

HRESULT cSelect::Draw()
{
	if (GetType() == SELECT_NULL)
		return 0;

	//get the four vertex!
	stVertexGround v[4];
	for (int i=0; i<4; i++)
	{
		v[i].color = 0xffffffff;
		v[i].sz = GetOffset().z;
	}
	switch (GetType())
	{
	case SELECT_SQUARE:
		{
//offset ÎªÖÐÐÄ
			cRect r(GetOffset().x - GetSelectWidth(),
				GetOffset().y - GetSelectHeight(),
				GetOffset().x + GetSelectWidth(),
				GetOffset().y + GetSelectHeight()
				);
			float x0 = GetOffset().x - GetSelectWidth();
			float y0 = GetOffset().y - GetSelectHeight();
			float x1 = GetOffset().x + GetSelectWidth();
			float y1 = GetOffset().y + GetSelectHeight();

			GetGround()->DrawRect(r,GetColor(),GetOffset().z);
		}
		break;
	case SELECT_DIAMOND:
		{
			float x = GetOffset().x ;
			float y = GetOffset().y ;

			v[0].sx = x - GetSelectWidth();
			v[0].sy = y;
			v[1].sx = x ;
			v[1].sy = y - GetSelectHeight() ;
			v[2].sx = x ;
			v[2].sy = y + GetSelectHeight() ;
			v[3].sx = x + GetSelectWidth();
			v[3].sy = y;

			v[0].tu = 0.f; 
			v[0].tv = 0.f;
			v[1].tu = 0.f;
			v[1].tv = 1.f;
			v[2].tu = 1.f;
			v[2].tv = 0.f;
			v[3].tu = 1.f;
			v[3].tv = 1.f;			

//			GetGraphics()->SetCommonTexture(cGraphics::TEX_SELECT);
//			GetGround()->DrawQuadrangle(v);
			GetGround()->DrawRect(v,GetColor());
		}
		break;
	case SELECT_PARALLELOGRAM:
		{
			v[0].sx = GetOffset().x;
			v[3].sx = GetOffsetEnd().x;
			v[0].sy = GetOffset().y;
			v[3].sy = GetOffsetEnd().y;
			if (fabs(v[3].sy - v[0].sy) > fabs(v[3].sx - v[0].sx))
			{
				if (v[3].sy < v[0].sy)
				{
					v[3].sy -= 1.f;
					v[0].sy += 1.f;
				}
				else if (v[3].sy > v[0].sy)
				{
					v[3].sy += 1.f;
					v[0].sy -= 1.f;
				}
			}
			else
			{
				if (v[3].sx < v[0].sx)
				{
					v[3].sx -= 1.f;
					v[0].sx += 1.f;
				}
				else //if (v[3].sx > v[0].sx)
				{
					v[3].sx += 1.f;
					v[0].sx -= 1.f;
				}
			}
			v[1].sx = ( (v[0].sx + v[0].sy) + (v[3].sx - v[3].sy) ) / 2;
			v[1].sy = ( (v[0].sx + v[0].sy) - (v[3].sx - v[3].sy) ) / 2;
			v[2].sx = ( (v[3].sx + v[3].sy) + (v[0].sx - v[0].sy) ) / 2;
			v[2].sy = ( (v[3].sx + v[3].sy) - (v[0].sx - v[0].sy) ) / 2;
			GetGround()->DrawRect(v,GetColor());
		}
		break;
	}
	return 0;
}

