// cSpecialSign.cpp: implementation of the cSpecialSign class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cSpecialSign.h"
#include "stringvalue.h"
#include "cGround.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cSpecialSign::cSpecialSign(cGraphics* p) : cSpecial(p)
{
	m_param.eElement = GT_SPECIAL_SIGN;	

	m_param.ptStart = cPoint(0,0,0);
	m_param.ptEnd = cPoint(0,0,0);
	m_param.crColor = cColor(255,255,255,255);
}

cSpecialSign::~cSpecialSign()
{

}

//////////////////////////////////////////////////////////////////////
//load binary
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialSign::LoadBinary(CFile& f)
{
	f.Read(&m_param,sizeof(m_param));
	return S_OK;
}

HRESULT	cSpecialSign::SaveBinary(CFile& f)
{
	f.Write(&m_param,sizeof(m_param));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
const CString c_aStrItem[] = 
{
	"elementtype",
	"sign的起始点","sign的终点","color",
	"",
};

const CString* cSpecialSign::GetItemStringArray()
{
	return c_aStrItem;
}

const CString c_aStrDescription[] = 
{
	"不能修改",
	"不能修改","cPoint","cColor",
	"",
};

const CString* cSpecialSign::GetDescriptionStringArray()
{
	return c_aStrDescription;
}

HRESULT cSpecialSign::GetValue(int e, CString& strValue)
{
	int n = GT_SPECIAL_SIGN;
	switch (e)
	{
	case PARAM_ELEMENTTYPE:
		ValueToString(n,strValue);
		break;
	case PARAM_START:
		ValueToString(m_param.ptStart,strValue);
		break;
	case PARAM_END:
		ValueToString(m_param.ptEnd,strValue);
		break;
	case PARAM_COLOR:
		ValueToString(m_param.crColor,strValue);
		break;
	}	
	return S_OK;
}

HRESULT cSpecialSign::SetParam(int e, LPCSTR szValue)
{
	CString strValue = szValue;
	switch (e)
	{
	case PARAM_START:
		StringToValue(szValue,m_param.ptStart);
		break;
	case PARAM_END:
		StringToValue(szValue,m_param.ptEnd);
		break;
	case PARAM_COLOR:
		StringToValue(szValue,m_param.crColor);
		break;
	}	
	return S_OK;
}

BOOL cSpecialSign::Active()
{
	m_bActive = TRUE;
	return m_bActive;
}

//draw
BOOL cSpecialSign::IsDraw(DrawEnum e)
{
	switch(e)
	{
	case SD_DRAWBEFOREALL:
		return TRUE;
	}
	return FALSE;
}

HRESULT	cSpecialSign::Draw(DrawEnum e)
{
	switch(e)
	{
	case SD_DRAWBEFOREALL:
		GetGround()->DrawLine(m_param.ptStart,m_param.ptEnd,m_param.crColor);
		break;
	}
	return S_OK;
}

cPoint	cSpecialSign::GetDrawPoint()
{
	return m_param.ptStart;
}
