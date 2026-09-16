// cScreen.cpp: implementation of the cScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cScreen.h"

#include "Graphics.h"
#include "cGraphics.h"
#include "cMap.h"
#include "cMapSmall.h"
#include "position.h"

#include "cPicture.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cScreen::cScreen(cGraphics* p)
{
	m_pGraphics = p;
	m_ulCount = 1;
	ZeroMemory(&m_param,sizeof(m_param));	
	m_pMap = NULL;
	m_pMapSmall = NULL;
}

cScreen::~cScreen()
{
}

/**************************************************************************/
//com
/**************************************************************************/
ULONG cScreen::AddRef() 
{
	return ++m_ulCount;
}

ULONG cScreen::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

iObj* cScreen::GetiMap()
{
	return m_pMap;
}

iObj* cScreen::GetiMapSmall()
{
	return m_pMapSmall;
}

/**************************************************************************/
//begin and end
/**************************************************************************/

HRESULT cScreen::Begin()
{
	GetGraphics()->CreateGraphicsElement(GT_MAP,&m_pMap);
	GetGraphics()->CreateGraphicsElement(GT_SMALLMAP,&m_pMapSmall);
	return 0;
}

HRESULT cScreen::End()
{
	RELEASE(m_pMap);
	RELEASE(m_pMapSmall);
	return 0;
}

/**************************************************************************/
//update screen's element 
/**************************************************************************/
void cScreen::AddcPicture(cPicture* p)
{
	ASSERT(p);
	if (!p)
		return;
	p->AddRef();
	p->SetInScreen(true);

	if (p->GetLayer() >= 0)
		m_listPicture.push_back(p);
	else
		m_listPictureBefore.push_back(p);
}

void cScreen::RemovecPicture(cPicture* p) 
{
	ASSERT(p);
	if (!p)
		return;
	if (p->GetLayer() >= 0)
		m_listPicture.remove(p);
	else
		m_listPictureBefore.remove(p);
	p->SetInScreen(false);
	p->Release();
}
/**************************************************************************/
//param
/**************************************************************************/
HRESULT cScreen::GetParam(stParam& param) 
{
	if (param.mask == 0)
		param = m_param;
	return 0;
}

HRESULT cScreen::SetParam(stParam& param, bool bRunNow)
{
	if (bRunNow)
		return setParam(param);
	AddRef();
	param.wParam = (long)this;
	param.lParam = 0;
	GetGraphics()->PushMessage(&param,sizeof(param),GT_SCREEN);
	return 0;
}

void cScreen::Trace()
{
}

HRESULT cScreen::setParam(const stParam& param) 
{
	if (param.mask & MASK_SCREENSIZE)
	{
		m_param.fWidth = param.fWidth;
		m_param.fHeight = param.fHeight;
	}
	if (param.mask & MASK_SELECTRECT)
	{
		m_param.rcSelect = param.rcSelect;
	}
	if (param.mask & MASK_COMMAND)
	{
		switch (param.eCommand)
		{
		case SCOM_ADDOBJ:
			AddcPicture(dynamic_cast<cPicture*>(param.pObj));
			param.pObj->Release();			
			break;
		case SCOM_REMOVEOBJ:
			RemovecPicture(dynamic_cast<cPicture*>(param.pObj));
			param.pObj->Release();
			break;
		}
	}
	return 0;
}
/**************************************************************************/
//draw
/**************************************************************************/
HRESULT cScreen::AddiObj(iObj* p)
{
	if (!p)
	{
		ASSERT(0);
		return -1;
	}
	p->AddRef();
	stParam param;
	param.mask = MASK_COMMAND;
	param.eCommand = SCOM_ADDOBJ;
	param.pObj = p;
	setParam(param);
	return 0;
}

HRESULT cScreen::RemoveiObj(iObj* p)
{
	if (!p)
	{
		ASSERT(0);
		return -1;
	}
	p->AddRef();
	stParam param;
	param.mask = MASK_COMMAND;
	param.eCommand = SCOM_REMOVEOBJ;
	param.pObj = p;
	setParam(param);
	return 0;
}

/**************************************************************************/
//paint
/**************************************************************************/
void cScreen::PaintBefore()
{
	if (m_listPictureBefore.empty())
		return;
	
	HRESULT hr; 
	hr = SetEnvironment();

	for_all(m_listPictureBefore,mem_fun<HRESULT,cPicture>(&cPicture::Draw));

	ClearEnvironment();
}

void cScreen::Paint()
{
	HRESULT hr; 
	hr = SetEnvironment();
	for_all(m_listPicture,mem_fun<HRESULT,cPicture>(&cPicture::Draw));
	m_pMapSmall->Draw();
	hr = SetEnvironment();
	m_pMap->Draw();
	PaintSelect();
	ClearEnvironment();
}

void cScreen::PaintSelect()
{
	CRect r = m_param.rcSelect;
	cRect rect = cRect(r.left,r.top,r.right,r.bottom);
	if (!r.IsRectEmpty())
	{
		stVertexScreen v[4];
		cRect rc(0,0,1,1);
		FillRectUV(v,rc);
		FillRect(v,rect);
		DrawRect(v,COLOR_GREEN);
	}	
}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/

HRESULT cScreen::FinalCleanup()
{
	HRESULT hr = S_OK;
	for_all(m_listPicture,mem_fun<HRESULT,cPicture>(&cPicture::FinalCleanup));
	for_all(m_listPicture,mem_fun<ULONG,cPicture>(&cPicture::Release));
	m_listPicture.clear();
	for_all(m_listPictureBefore,mem_fun<HRESULT,cPicture>(&cPicture::FinalCleanup));
	for_all(m_listPictureBefore,mem_fun<ULONG,cPicture>(&cPicture::Release));
	m_listPictureBefore.clear();
	return hr;
}

void cScreen::New()
{
	FinalCleanup();
}