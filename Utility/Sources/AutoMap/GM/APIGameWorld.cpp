//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIGameWorld.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-7-7 15:10:05
//	文件说明		:	GameWorld API 的工具类实现
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMCore.h"
#include "GMMessages.h"
#include "APIGameWorld.h"

KGameWorldApiTool::KGameWorldApiTool()
{
	m_pGMCoreRef = NULL;
}

void KGameWorldApiTool::Init(KGameMasterCore* pGMC, LPCSTR strGWName, DWORD dwIP)
{
	m_pGMCoreRef = pGMC;
	m_strGWName = strGWName;
	m_dwIP = dwIP;
}

KGameWorldApiTool::~KGameWorldApiTool()
{
	m_pGMCoreRef = NULL;
}

STDMETHODIMP KGameWorldApiTool::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_strGWName.AllocSysString();
	return S_OK;
}

STDMETHODIMP KGameWorldApiTool::get_IsRunning(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_pGMCoreRef && m_pGMCoreRef->IsConnectionOK();

	return S_OK;
}

STDMETHODIMP KGameWorldApiTool::get_OnlineCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_pGMCoreRef->GetOnlineCount(m_strGWName);

	return S_OK;
}

STDMETHODIMP KGameWorldApiTool::BroadCast(BSTR strMessage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CString str(strMessage);
	return m_pGMCoreRef->BroadCastChatToGameWorlds(m_strGWName, str);
}