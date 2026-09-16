//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIRole.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-7-7 19:21:53
//	文件说明		:	Role API 的工具类实现
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMCore.h"
#include "GMMessages.h"
#include "APIRole.h"

KRoleApiTool::KRoleApiTool()
{
	m_pGMCoreRef = NULL;
}

void KRoleApiTool::Init(KGameMasterCore* pGMCore, LPCSTR strAccountName,
						LPCSTR strRoleName, LPCSTR strGameWorldName)
{
	ASSERT(strlen(strAccountName));
	ASSERT(strlen(strRoleName));
	ASSERT(strlen(strGameWorldName));
	m_pGMCoreRef = pGMCore;
	m_strAccountName = strAccountName;
	m_strRoleName = strRoleName;
	m_strGameWorldName = strGameWorldName;
}

KRoleApiTool::~KRoleApiTool()
{
	//todo
}


STDMETHODIMP KRoleApiTool::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_strRoleName.AllocSysString();

	return S_OK;
}

STDMETHODIMP KRoleApiTool::SendMessageToThis(BSTR strMessage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return m_pGMCoreRef->SendMessage(m_strAccountName, m_strRoleName, CString(strMessage));
}


STDMETHODIMP KRoleApiTool::PutInJail(long Minutes)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return m_pGMCoreRef->PutInJail(m_strAccountName, m_strRoleName, m_strGameWorldName, Minutes);
}

STDMETHODIMP KRoleApiTool::FreeFromJail()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return m_pGMCoreRef->FreeFromJail(m_strAccountName, m_strRoleName, m_strGameWorldName);
}

STDMETHODIMP KRoleApiTool::ShutUp(long Minutes)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here

	return E_FAIL;
}

STDMETHODIMP KRoleApiTool::get_PlayerAccountName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_strAccountName.AllocSysString();

	return S_OK;
}

STDMETHODIMP KRoleApiTool::FreeChat()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here

	return E_FAIL;
}

STDMETHODIMP KRoleApiTool::get_RoleInGameWorldName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_strGameWorldName.AllocSysString();

	return S_OK;
}