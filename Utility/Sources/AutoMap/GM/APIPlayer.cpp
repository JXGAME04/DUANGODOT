//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIPlayer.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-7-7 19:21:53
//	文件说明		:	Player API 的工具类实现
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMCore.h"
#include "GMMessages.h"
#include "APIPlayer.h"
#include "APIRole.h"

KPlayerApiTool::KPlayerApiTool()
{
	m_pGMCoreRef = NULL;
}

void KPlayerApiTool::Init(KGameMasterCore* pGMC, LPCSTR strName)
{
	m_strAccountName = strName;
	m_pGMCoreRef = pGMC;
}

KPlayerApiTool::~KPlayerApiTool()
{
}

STDMETHODIMP KPlayerApiTool::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pVal = m_strAccountName.AllocSysString();

	return S_OK;
}

STDMETHODIMP KPlayerApiTool::get_ActiveRole(IKRole** pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	GAMEWORLD gw;
	CString strRole;
	int nX = -1, nY = -1, nWorldID = -1;
	HRESULT hr = m_pGMCoreRef->WhichGWIsPlayerInRealTime(gw, strRole, nX, nY, nWorldID, m_strAccountName);
	if (!SUCCEEDED(hr))
		strRole.Empty();
	else if (gw.strGameWorldName.GetLength())
	{
		CStringList lstRoles;
		KAPIChild* pChild = NULL;

		hr = m_pGMCoreRef->GetRoleList(m_strAccountName, gw.strGameWorldName, lstRoles);
		KS_CHECK(hr);

		{
			if (lstRoles.Find(strRole))
			{
				if (LookingForChild(strRole + "\n" + gw.strGameWorldName, (void**)&pChild) && pChild)
				{
					KRoleApiTool* pRT = ((KRoleApiTool*)pChild);
					hr = pRT->QueryInterface(IID_IKRole, (void**)pVal);
				}
				else
				{
					CComObjectNoLock<KRoleApiTool>* pPLTool = NULL;
					gCreateInstanceNoLock<KRoleApiTool>(&pPLTool);
					if (pPLTool)
					{
						pPLTool->Init(m_pGMCoreRef, m_strAccountName, strRole, gw.strGameWorldName);
						pPLTool->SetParentAPI(this);
						hr = pPLTool->QueryInterface(IID_IKRole, (void**)pVal);
					}
					else
						hr = E_FAIL;
				}
			}
			else
				hr = E_FAIL;
		}
	}

KS_EXIT:
	return hr;
}

STDMETHODIMP KPlayerApiTool::get_CountOfRolesInGW(BSTR strGameWorldName, long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (wcslen(strGameWorldName) == 0)
		return E_FAIL;

	CStringList lstRoles;
	CString strGWName(strGameWorldName);
	HRESULT hr = m_pGMCoreRef->GetRoleList(m_strAccountName, strGWName, lstRoles);
	if (SUCCEEDED(hr))
		*pVal = lstRoles.GetCount();
	return hr;
}

STDMETHODIMP KPlayerApiTool::ItemRoleByNameInGW(BSTR strGameWorldName, BSTR strRoleName, IKRole **prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	if (wcslen(strGameWorldName) == 0 || wcslen(strRoleName) == 0)
		return E_FAIL;

	CStringList lstRoles;
	CString strGWName(strGameWorldName), strRole(strRoleName);
	KAPIChild* pChild = NULL;

	HRESULT hr = m_pGMCoreRef->GetRoleList(m_strAccountName, strGWName, lstRoles);
	KS_CHECK(hr);

	{
		if (lstRoles.Find(strRole))
		{
			if (LookingForChild(strRole + "\n" + strGWName, (void**)&pChild) && pChild)
			{
				KRoleApiTool* pRT = ((KRoleApiTool*)pChild);
				hr = pRT->QueryInterface(IID_IKRole, (void**)prop);
			}
			else
			{
				CComObjectNoLock<KRoleApiTool>* pPLTool = NULL;
				gCreateInstanceNoLock<KRoleApiTool>(&pPLTool);
				if (pPLTool)
				{
					pPLTool->Init(m_pGMCoreRef, m_strAccountName, strRole, strGWName);
					pPLTool->SetParentAPI(this);
					hr = pPLTool->QueryInterface(IID_IKRole, (void**)prop);
				}
				else
					hr = E_FAIL;
			}
		}
		else
			hr = E_FAIL;
	}

KS_EXIT:
	return hr;
}

STDMETHODIMP KPlayerApiTool::ItemRoleByIndexInGW(BSTR strGameWorldName, long Index, IKRole **prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CStringList lstRoles;
	CString strGWName(strGameWorldName), strRole;
	KAPIChild* pChild = NULL;

	HRESULT hr = m_pGMCoreRef->GetRoleList(m_strAccountName, strGWName, lstRoles);
	POSITION pos = NULL;
	KS_CHECK(hr);

	{
		if (NULL != (pos = lstRoles.FindIndex(Index)))
		{
			strRole = lstRoles.GetAt(pos);
			if (LookingForChild(strRole + "\n" + strGWName, (void**)&pChild) && pChild)
			{
				KRoleApiTool* pRT = ((KRoleApiTool*)pChild);
				hr = pRT->QueryInterface(IID_IKRole, (void**)prop);
			}
			else
			{
				CComObjectNoLock<KRoleApiTool>* pPLTool = NULL;
				gCreateInstanceNoLock<KRoleApiTool>(&pPLTool);
				if (pPLTool)
				{
					pPLTool->Init(m_pGMCoreRef, m_strAccountName, strRole, strGWName);
					pPLTool->SetParentAPI(this);
					hr = pPLTool->QueryInterface(IID_IKRole, (void**)prop);
				}
				else
					hr = E_FAIL;
			}
		}
		else
			hr = E_FAIL;
	}

KS_EXIT:
	return hr;
}

STDMETHODIMP KPlayerApiTool::UnlockAccount(BSTR strPassword)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CString strPass(strPassword);
	return m_pGMCoreRef->UnlockPlayerAccount(m_strAccountName, strPass);
}

STDMETHODIMP KPlayerApiTool::KickOffActiveRole(long ForbidMinutes)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pGMCoreRef->ExecutePlayer(m_strAccountName, "ds KickOutSelf()");

	return S_OK;
}
