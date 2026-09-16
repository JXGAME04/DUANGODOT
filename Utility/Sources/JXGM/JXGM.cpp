// JXGM.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "headers\jxgm_i.h"
#include "ksystemonline.h"

#include <objbase.h>
#include <initguid.h>
#pragma warning(disable : 4786)
#include "IClient.h"
#include "Library.h"
using OnlineGameLib::Win32::CLibrary;

/*
 * Global variable
 */
CLibrary g_theRainbowDll("rainbow.dll");

typedef HRESULT ( __stdcall *pfnCreateClientInterface)(REFIID riid, void **ppv);

pfnCreateClientInterface g_pFactroyFun = NULL;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	g_pFactroyFun = (pfnCreateClientInterface)g_theRainbowDll.GetProcAddress("CreateInterface");

    return TRUE;
}

STDAPI CreateSystemOnline(ISystemOnline** ppSystem)
{
	HRESULT hr = E_FAIL;
	if (ppSystem)
	{
		KSystemOnline* pKSystemOnline = KSystemOnline::Instance();
		if (pKSystemOnline)
		{
			*ppSystem = (ISystemOnline*)pKSystemOnline;
			hr = S_OK;
		}
	}
	return hr;
}

STDAPI DestroySystemOnline()
{
	HRESULT hr = S_OK;
	KSystemOnline::ReleaseInstance();
	return hr;
}

IClient* CreateClient(const size_t &maxFreeBuffers, const size_t &bufferSize)
{
	IClient *pClient = NULL;

	IClientFactory *pClientFactory = NULL;
 
	assert(g_pFactroyFun);

	if (SUCCEEDED(g_pFactroyFun(IID_IClientFactory, reinterpret_cast< void ** >(&pClientFactory))))
	{
		pClientFactory->SetEnvironment(maxFreeBuffers, bufferSize);
		
		pClientFactory->CreateClientInterface(IID_IESClient, reinterpret_cast< void ** >(&pClient));
		
		pClientFactory->Release();
	}

	return pClient;
}
