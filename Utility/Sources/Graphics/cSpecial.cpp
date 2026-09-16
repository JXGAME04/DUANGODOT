// cSpecial.cpp: implementation of the cSpecial class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cSpecial.h"
#include "cGraphics.h"
#include "iItem.h"
#include "cGround.h"
#include "cCamera.h"
#include "position.h"

/*
#include "cScreen.h"
*/
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cSpecial::cSpecial(cGraphics* p):m_pGraphics(p)
{
	m_ulCount = 1;
	m_bActive = FALSE;
	m_bSelect = FALSE;
	time = 0;
}

cSpecial::~cSpecial()
{
	
}

//graphics
cScreen* cSpecial::GetScreen()
{
	return GetGraphics()->GetcScreen();
}

cGround* cSpecial::GetGround()
{
	return GetGraphics()->GetcGround();
}

cCamera* cSpecial::GetCamera()
{
	return GetGraphics()->GetcCamera();
}
//////////////////////////////////////////////////////////////////////
//com
//////////////////////////////////////////////////////////////////////
ULONG cSpecial::AddRef() 
{
	return ++m_ulCount;
}

ULONG cSpecial::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

//////////////////////////////////////////////////////////////////////
//load and save
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecial::Load(const stSection* p)
{
	int n = GetItemCount();
	for (int i=0; i<n; i++)
	{
		CString strItem;
		GetItem(i,strItem);
		CString strValue;
		if (GetSectionValue(p,strItem,strValue))
			SetParam(i,strValue);
	}
	return S_OK;
}

HRESULT	cSpecial::Save(stSection* p)
{
	int n = GetItemCount();
	for (int i=0; i<n; i++)
	{
		CString strItem;
		GetItem(i,strItem);
		CString strValue;
		GetValue(i,strValue);
		SetSectionValue(p,strItem,strValue);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecial::SetParam(LPCSTR szItem, LPCSTR szValue)
{
	int e = FindIndex(szItem);
	return SetParam(e,szValue);
}

HRESULT	cSpecial::GetParam(LPCSTR szItem, LPSTR szValue)
{
	CString strValue;
	GetParam(szItem,strValue);
	strcpy(szValue,strValue);
	return S_OK;
}

HRESULT	cSpecial::GetParam(LPCSTR szItem, CString& strValue)
{
	int e = FindIndex(szItem);
	return GetValue(e,strValue);
}


HRESULT cSpecial::GetParam(int index, CString& szItem, CString& szValue, CString& szDescription)
{
	if (index >= GetItemCount() || index < 0)
		return -1;
	GetDescription(index,szDescription);
	GetValue(index,szValue);
	GetItem(index,szItem);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
cPoint cSpecial::GetDrawPoint()
{
	return GetCamera()->GetFocus();
}

HRESULT	cSpecial::Draw()				//绘制自身为了编辑操作
{
	cPoint pt = GetDrawPoint();
	int alpha;
	if (m_bSelect)
		alpha = 255;
	else
		alpha = 128;
	cColor color = cColor(255,255,255,alpha);
	if (!m_bSelect)
		time = 0;
	if (time++ <= 6*2)
	{
		float f = 0.5f;
		cRect rc;
		rc.l = pt.x - f;
		rc.r = pt.x + f;
		rc.t = pt.y - f;
		rc.b = pt.y + f;
		GetGround()->DrawRect(rc,color);
	}
	else if (time > 8*2)
	{
		time = 0;
	}
	return S_OK;
}

BOOL cSpecial::Contain(cPoint ptGround)
{
	cPoint pt = GetDrawPoint();
	if (DistanceAdd(ptGround,pt) < 0.5f)
		return TRUE;
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
//string
//////////////////////////////////////////////////////////////////////
int cSpecial::FindIndex(LPCSTR szItem)
{
	const CString* aStr = GetItemStringArray();
	int i=0;
	while(!aStr[i].IsEmpty())
	{
		if (aStr[i] == szItem)
			return i;
		else
			i++;
	}
	return -1;
}

HRESULT	cSpecial::GetDescription(int index, CString& strDescription)
{
	const CString* aStr = GetDescriptionStringArray();
	strDescription = aStr[index];
	return S_OK;	
}

HRESULT	cSpecial::GetItem(int index, CString& strItem)
{
	const CString* aStr = GetItemStringArray();
	strItem = aStr[index];
	return S_OK;
}
