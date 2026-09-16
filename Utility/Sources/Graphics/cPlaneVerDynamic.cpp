// cPlaneVerDynamic.cpp: implementation of the cPlaneVerDynamic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPlaneVerDynamic.h"
#include "cGround.h"
#include "cGraphics.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlaneVerDynamic::cPlaneVerDynamic(cGraphics* p) : cPlaneVer(p)
{
	for (int i=0; i<MAX_RESOURCE; i++)
		m_aTexture[i] = NULL;
}

cPlaneVerDynamic::~cPlaneVerDynamic()
{

}

void cPlaneVerDynamic::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneVerDynamicParam;
	cPlaneVer::Init(false);
	m_pParam->eElement = GT_PLANEVERDYNAMIC;
}

iTexture*	cPlaneVerDynamic::GetTexture(int i)
{
	if (m_aTexture[i] == NULL)
	{
		FindTexture();
		FillTextureUV();
	}
	return m_aTexture[i];
}

stVertexGround*	cPlaneVerDynamic::GetVG(int i)
{
	return m_aaVG[i];
}

HRESULT cPlaneVerDynamic::Draw()
{
	ASSERT(IsLightingPoint());
	DWORD value = GetFinalColor().Value();
	if (GetResNum() == 0)		//没有设置RES的参数
	{
		ASSERT(0);
		return -1;
	}
	for (int j=0; j<GetResNum(); j++)
	{
		stVertexGround* aVG = GetVG(j);
		for (int i=0; i<4; i++)
			aVG[i].color = value;
		GetGround()->SetTexture(GetTexture(j),GetiFrame(j));
		GetGround()->DrawQuadrangle(aVG);
	}
	return S_OK;
}

void cPlaneVerDynamic::GetBottomSeam(cPoint& p1, cPoint& p2)
{
	p1 = GetOffset();
	p2 = GetOffset();
}

float cPlaneVerDynamic::GetBottomSeamLengthSquare()
{
	return 0;
}

HRESULT cPlaneVerDynamic::UpdateVertexInfo()
{
	HRESULT hr;
	for (int i=0; i<GetResArray().nResource; i++)
	{
		stTexInfo info;
		GetTexInfo(info,i);		
		hr = FillVertexInfoVerAngle0(info,i);
		if (FAILED(hr))
			return hr;
	}	
	return S_OK;
}

void cPlaneVerDynamic::FindTexture()
{
	stPlaneVerDynamicParam& m_param = *GetPlaneVerDynamicParam();
	stResourceA stResA;
	stResA.stDevice = m_param.stResArray.stDevice;
	for (int i=0; i<m_param.stResArray.nResource; i++)
	{
		stResA.stRes = m_param.stResArray.aResource[i];
		if (m_aTexture[i] == NULL)
			m_aTexture[i] = GetGraphics()->FindTexture(stResA);
	}
}

HRESULT cPlaneVerDynamic::setParam(stObjParam& pm)
{
	HRESULT hr = cPlaneVer::setParam(pm);
	if (!pm.IsEMask(MASK_PLANE))
		return hr;

	stPlaneVerDynamicParam& param = *((stPlaneVerDynamicParam* )&pm);
	stPlaneVerDynamicParam& m_param = *GetPlaneVerDynamicParam();

	if (param.IsSetMask(stPlaneParam::MASK_PLANE_RESOURCEARRAY))
	{
		ClearTexture();
		m_param.stResArray = param.stResArray;
		FindTexture();
	}
	return S_OK;
}

HRESULT cPlaneVerDynamic::DrawSub(cPoint p1,cPoint p2)
{
	return Draw();
}
