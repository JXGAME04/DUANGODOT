// -------------------------------------------------------------------------
//	文件名		：	ksystemonline.cpp
//	创建者		：	万里
//	创建时间	：	2003-6-2 19:06:56
//	功能描述	：	
//
// -------------------------------------------------------------------------

#include "stdafx.h"

#include "ksystemonline.h"
#include "KSystemConnect.h"

#include "IClient.h"

KSystemOnline* KSystemOnline::m_pInstance = NULL;

KSystemOnline::KSystemOnline()
{
	m_pSystemConnect = NULL;
}

KSystemOnline::~KSystemOnline()
{
	Close();
}

KSystemOnline* KSystemOnline::Instance()
{
	if (m_pInstance == NULL)
	{
		m_pInstance = new KSystemOnline;
	}

	return m_pInstance;
}

void KSystemOnline::ReleaseInstance()
{
	if (m_pInstance)
	{
		m_pInstance->Close();
		delete m_pInstance;
	}
	m_pInstance = NULL;
}

void KSystemOnline::Close()
{
	if (m_pSystemConnect)
	{
		m_pSystemConnect->Stop();
		delete m_pSystemConnect;
	}
	m_pSystemConnect = NULL;
}

HRESULT STDMETHODCALLTYPE KSystemOnline::OpenSystemConnect(DWORD nIP, short nPort, ISystemConnect** ppConnect)
{
	if (ppConnect == NULL)
		return E_INVALIDARG;

	if (m_pSystemConnect)
	{
		*ppConnect = m_pSystemConnect;
		return S_OK;
	}

	KSystemConnect* pSystemConnect = new KSystemConnect;
	if (pSystemConnect == NULL)
		return E_OUTOFMEMORY;
	
	HRESULT hr = E_FAIL;
	if (pSystemConnect->InitClient(50, 8192) && pSystemConnect->ConnectTo(nIP, nPort))
	{
		pSystemConnect->Start();
		m_pSystemConnect = pSystemConnect;
		*ppConnect = m_pSystemConnect;
		hr = S_OK;
	}
	else
		delete pSystemConnect;
	
	return hr;
}

