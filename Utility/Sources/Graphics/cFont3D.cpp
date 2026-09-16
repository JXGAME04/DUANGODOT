// cFont3D3D.cpp: implementation of the cFont3D3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cFont3D.h"
#include "graphics.h"
#include "cGraphics.h"
#include "D3DFont.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cFont3D::cFont3D(cGraphics* p):cFont(p)
{
	m_pFont = NULL;	
}

cFont3D::~cFont3D()
{
	if (m_pFont)
		delete m_pFont;
}

/**************************************************************************/
//device 
/**************************************************************************/
LPDIRECT3DDEVICE8 cFont3D::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

HRESULT cFont3D::InitDeviceObjects()
{
	HRESULT hr = m_pFont->InitDeviceObjects(GetDevice());
	return hr;
}

HRESULT cFont3D::RestoreDeviceObjects()
{
	HRESULT hr = m_pFont->RestoreDeviceObjects();
	return hr;
}

HRESULT cFont3D::InvalidateDeviceObjects()
{
	HRESULT hr = m_pFont->InvalidateDeviceObjects();
	return hr;
}

HRESULT cFont3D::DeleteDeviceObjects()
{
	HRESULT hr = m_pFont->DeleteDeviceObjects();
	return hr;
}

/**************************************************************************/
//create	
/**************************************************************************/
HRESULT cFont3D::Create(LPCSTR strFontName, DWORD dwHeight, DWORD dwFlags)
{
	strcpy(m_strName,strFontName);
    m_pFont = new CD3DFont( m_strName, dwHeight, dwFlags );
	
	return S_OK;
}

/**************************************************************************/
//draw
/**************************************************************************/

HRESULT cFont3D::DrawText( const stParam& param, LPCSTR strText, int nLength)
{
	cColor color = param.crColor;
	DWORD cr = color.Value();
	return m_pFont->DrawText(param.ptPos.x,param.ptPos.y,cr,(LPSTR)strText,param.lFlag);
}
