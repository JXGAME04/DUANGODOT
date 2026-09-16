//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMApp.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-5 14:57:44
//	文件说明		:	GameMaster 对象的实现
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMCore.h"
#include "APIGameWorld.h"
#include "APIPlayer.h"
#include "GMMessages.h"

/////////////////////////////////////////////////////////////////////////////
// CGMApp

BEGIN_MESSAGE_MAP(CGMApp, CWinApp)
	//{{AFX_MSG_MAP(CGMApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// VBA Implementation

STDMETHODIMP CGMApp::get_TotalOnlineCount(long* prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	TRY{
		*prop = m_pGameMaster->GetOnlineCount("");
	}
	CATCH_ALL(e){
		TRACE("%s(%u): Failed.\n", __FILE__, __LINE__);
		return E_FAIL;
	}
	END_CATCH_ALL
	return S_OK;
}

STDMETHODIMP CGMApp::get_TotalRegisteredCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	TRY{
		*pVal = m_pGameMaster->GetRegisterCount();
	}
	CATCH_ALL(e){
		TRACE("%s(%u): Failed.\n", __FILE__, __LINE__);
		return E_FAIL;
	}
	END_CATCH_ALL

	return S_OK;
}

STDMETHODIMP CGMApp::get_IsGMSystemRunning(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	TRY{
		*pVal = m_pGameMaster->IsConnectionOK();
	}
	CATCH_ALL(e){
		TRACE("%s(%u): Failed.\n", __FILE__, __LINE__);
		return E_FAIL;
	}
	END_CATCH_ALL

	return S_OK;
}

STDMETHODIMP CGMApp::FindPlayer(BSTR strAccount, BSTR *pstrInGameWorld)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;
	TRY{
		GAMEWORLD gw;
		CString strAcc(strAccount);
		hr = m_pGameMaster->WhichGWIsPlayerIn(gw, strAcc);
		if (SUCCEEDED(hr))
			*pstrInGameWorld = gw.strGameWorldName.AllocSysString();
	}
	CATCH_ALL(e){
		TRACE("%s(%u): Failed.\n", __FILE__, __LINE__);
		return E_FAIL;
	}
	END_CATCH_ALL

	return hr;
}

STDMETHODIMP CGMApp::get_CountOfGameWorlds(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	TRY{
		*pVal = m_pGameMaster->CountOfGameWorlds();
	}
	CATCH_ALL(e){
		TRACE("%s(%u): Failed.\n", __FILE__, __LINE__);
		return E_FAIL;
	}
	END_CATCH_ALL

	return S_OK;
}

STDMETHODIMP CGMApp::ItemGameWorldByName(BSTR strGWName, IKGameWorld **prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	CString strGW(strGWName);
	GAMEWORLD gw;
	KAPIChild* pChild = NULL;

	KS_CHECK_BOOLEX(wcslen(strGWName), hr = E_FAIL);
	KS_CHECK_BOOLEX(m_pGameMaster->GetGWByName(gw, strGW), hr = E_FAIL);
	KS_CHECK_BOOLEX(gw.strGameWorldName == strGW, hr = E_FAIL);

	if (LookingForChild(strGW, (void**)&pChild) && pChild)
	{
		KGameWorldApiTool* pGW = ((KGameWorldApiTool*)pChild);
		hr = pGW->QueryInterface(IID_IKGameWorld, (void**)prop);
	}
	else
	{
		CComObjectNoLock<KGameWorldApiTool>* pGWTool = NULL;
		gCreateInstanceNoLock<KGameWorldApiTool>(&pGWTool);
		if (pGWTool)
		{
			pGWTool->Init(m_pGameMaster, gw.strGameWorldName, gw.dwIP);
			pGWTool->SetParentAPI(this);
			hr = pGWTool->QueryInterface(IID_IKGameWorld, (void**)prop);
		}
		else
			hr = E_FAIL;
	}


KS_EXIT:
	return hr;
}

STDMETHODIMP CGMApp::ItemGameWorldByIndex(long Index, IKGameWorld **prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;
	GAMEWORLD gw;
	KAPIChild* pChild = NULL;

	KS_CHECK_BOOLEX(Index >= 0 && Index < m_pGameMaster->CountOfGameWorlds(), hr = E_FAIL);
	KS_CHECK_BOOLEX(m_pGameMaster->GetGWByIndex(gw, Index), hr = E_FAIL);
	KS_CHECK_BOOLEX(gw.strGameWorldName.GetLength(), hr = E_FAIL);

	if (LookingForChild(gw.strGameWorldName, (void**)&pChild) && pChild)
	{
		KGameWorldApiTool* pGW = ((KGameWorldApiTool*)pChild);
		hr = pGW->QueryInterface(IID_IKGameWorld, (void**)prop);
	}
	else
	{
		CComObjectNoLock<KGameWorldApiTool>* pGWTool = NULL;
		gCreateInstanceNoLock<KGameWorldApiTool>(&pGWTool);
		if (pGWTool)
		{
			pGWTool->Init(m_pGameMaster, gw.strGameWorldName, gw.dwIP);
			pGWTool->SetParentAPI(this);
			hr = pGWTool->QueryInterface(IID_IKGameWorld, (void**)prop);
		}
		else
			hr = E_FAIL;
	}

	
KS_EXIT:
	return hr;
}

STDMETHODIMP CGMApp::ItemPlayerByName(BSTR strAccount, IKPlayer **prop)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (0 == wcslen(strAccount))
		return E_FAIL;

	HRESULT hr = S_OK;

	CString strAcc(strAccount);
	KAPIChild* pChild = NULL;

	if (m_Players.LookingForChild(strAcc, (void**)&pChild) && pChild)
	{
		KPlayerApiTool* pGW = ((KPlayerApiTool*)pChild);
		hr = pGW->QueryInterface(IID_IKPlayer, (void**)prop);
	}
	else
	{
		CComObjectNoLock<KPlayerApiTool>* pPLTool = NULL;
		gCreateInstanceNoLock<KPlayerApiTool>(&pPLTool);
		if (pPLTool)
		{
			pPLTool->Init(m_pGameMaster, strAcc);
			pPLTool->SetParentAPI(&m_Players);
			hr = pPLTool->QueryInterface(IID_IKPlayer, (void**)prop);
		}
		else
			hr = E_FAIL;
	}


KS_EXIT:
	return hr;
}

STDMETHODIMP CGMApp::FindPlayerInRealTime(BSTR strAccount, BSTR *pstrInGameWorld)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (0 == wcslen(strAccount))
		return E_FAIL;

	GAMEWORLD gw;
	CString strRole, strAcc(strAccount);
	int nX = -1, nY = -1, nWorldID = -1;
	HRESULT hr = m_pGameMaster->WhichGWIsPlayerInRealTime(gw, strRole, nX, nY, nWorldID, strAcc);
	if (SUCCEEDED(hr))
		*pstrInGameWorld = gw.strGameWorldName.AllocSysString();
	return hr;
}

STDMETHODIMP CGMApp::KickOffPlayer(BSTR strAccount)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (0 == wcslen(strAccount))
		return E_FAIL;
	return m_pGameMaster->ExecutePlayer(CString(strAccount), "ds KickOutSelf()");
}

STDMETHODIMP CGMApp::SpeakToTheWorld(BSTR strMessage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	CString str(strMessage);
	return m_pGameMaster->BroadCastChatToGameWorlds(NULL, str);
}