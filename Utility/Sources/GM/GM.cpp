//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMApp.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-5 14:57:44
//	文件说明		:	GameMaster 对象的实现
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMCore.h"
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

STDMETHODIMP CGMApp::get_OnlineCount(long* prop)
{
	METHOD_PROLOGUE_ATL;

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

STDMETHODIMP CGMApp::Beep()
{
	::Beep(1000,1000);

	return S_OK;
}
