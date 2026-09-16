 // S3Client.cpp : Defines the entry point for the application.
//

#include "KWin32.h"
#include "KCore.h"
#include "S3Client.h"
#include "KWin32Wnd.h"
#include "../../Represent/iRepresent/iRepresentShell.h"
#include "Ui/UiShell.h"
#include "NetConnect/NetConnectAgent.h"
#include "TextCtrlCmd/TextCtrlCmd.h"
#include "KPakList.h"
#include "Ui/Elem/TextPic.h"
#include "Ui/Elem/UiCursor.h"
#include "Ui/Elem/SpecialFuncs.h"
#include "Ui/FilterTextLib.h"
#include "Ui/ChatFilter.h"
#include "Ui/uibase.h"
#include "ErrorCode.h"
#include "Ui/UICase/uioptions.h" //fdb
//for auto
#include "Ui/UiCase/UiPlayerBar.h"
#include "Ui/UiCase/UiMsgCentrePad.h"
#include "Ui/UiCase/UiFaceSelector.h"
#include "Ui/UiCase/UiInformation.h"
#include "Ui/UiCase/UiInit.h"
#include "Ui/UiCase/UiSelServer.h"
#include "Ui/UiCase/UiLogin.h"
#include "Ui/UiCase/UiSelPlayer.h"
#include "Ui/Elem/Wnds.h"
#include <shellapi.h>	//Shell_NotifyIcon
#define ClientVersion
KMyApp		MyApp;
HINSTANCE	hInst;
KPakList	g_PakList;
CFilterTextLib g_libFilterText;
CChatFilter g_ChatFilter;

#define	QUIT_QUESTION_ID	"22"
#define	GAME_TITLE			"23"

#define REPRESENT_MODULE_2			"Represent2.dll"
#define REPRESENT_MODULE_3			"Represent3.dll"
#define CREATE_REPRESENT_SHELL_FUN	"CreateRepresentShell"
#define	GAME_FPS			54


struct iRepresentShell*	g_pRepresentShell = NULL;
struct IInlinePicEngineSink* g_pIInlinePicSink = NULL;
iCoreShell*				g_pCoreShell = NULL;
KMusic*					g_pMusic = NULL;

#define	DYNAMIC_LINK_REPRESENT_LIBRARY

#ifdef DYNAMIC_LINK_REPRESENT_LIBRARY
	static HMODULE		l_hRepresentModule = NULL;
	int					g_bRepresent3 = false;
#endif
	
int					g_bScreen = true;
char				g_szGameName[32] = "½£ÏÀÇéÔµ¡¤ÍøÂç°æ";


KClientCallback g_ClientCallback;
//auto
const char* MMF_NAME_SERVER = "Local\\Auto_Name_MMFSV_";
const char* MMF_NAME_CLIENT = "Local\\Auto_Name_MMFCL_";
const char* EVT_CLRECV = "Local\\Auto_EventClientRecv_";
const char* EVT_SVRECV = "Local\\Auto_EventServerRecv_";
HANDLE g_hMap = NULL;
SharedState* g_pState = NULL;
HANDLE g_hEventRecv = NULL;
HANDLE g_hRepMap = NULL;
SharedState* g_pRepState = NULL;
HANDLE g_hRepEvent = NULL;
UINT g_CurNum = 0;
UINT g_CurSize = 0;
BYTE g_Buffer[SHARED_SIZE];
static int g_DrawVision = 0;
static UINT g_DrawVisionTime = 0;
static int g_ALGStep = 0;
static UINT g_AGLNextTime = 0;

/*
 * This macro is helper that can judge some legal character
 */
#define _private_IS_SPACE(c)   ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == 'x')
#define IS_SPACE(c)	_private_IS_SPACE(c)

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
//disable dpi aware mode of window
	/*HMODULE hUser32 = LoadLibrary("user32.dll");
if (hUser32) {
    typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
    
    SetProcessDPIAwareFunc setDPIAwareFunc = 
        (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
    
    if (setDPIAwareFunc) {
        setDPIAwareFunc();
    }
    
    FreeLibrary(hUser32);
}*/
 	// TODO: Place code here.

	/*
	 * Add this funtion by liupeng on 2003.3.20
	 * We can find some error when start a console tracer
	 */
#ifdef	TRUE

	bool bOpenTracer = false;

    while( lpCmdLine[0] == '-' || lpCmdLine[0] == '/' )
    {
        lpCmdLine++;
		
        switch ( *lpCmdLine++ )
        {
		case 'c':
        case 'C':
            bOpenTracer = true;
            break;
        }
		
        while( IS_SPACE( *lpCmdLine ) )
        {
            lpCmdLine++;
        }
    }
	
	if ( bOpenTracer ) 
	{
		AllocConsole();
	}

#endif // End of this function

	hInst = hInstance;
	if (MyApp.Init(hInstance))
		MyApp.Run();

#ifdef TRUE

	if ( bOpenTracer )
	{
		FreeConsole();
	}

#endif
	Error_Box();

	return 0;
}

KMyApp::KMyApp()
{
	m_pInlinePicSink = NULL;
}

BOOL InitRepresentShell(BOOL bFullScreen, int nWidth, int nHeight)
{
	Error_SetErrorString(g_bRepresent3 ? REPRESENT_MODULE_3 : REPRESENT_MODULE_2);
	if (g_pRepresentShell == NULL)
	{
#ifdef DYNAMIC_LINK_REPRESENT_LIBRARY
		if (l_hRepresentModule == NULL &&
			(l_hRepresentModule = LoadLibrary(g_bRepresent3 ? REPRESENT_MODULE_3 : REPRESENT_MODULE_2)) == NULL)
		{
			Error_SetErrorCode(ERR_T_LOAD_MODULE_FAILED);
			return FALSE;
		}
		fnCreateRepresentShell pCreate = (fnCreateRepresentShell)GetProcAddress(
			l_hRepresentModule, CREATE_REPRESENT_SHELL_FUN);
		if (pCreate == NULL || 
			(g_pRepresentShell = pCreate()) == NULL)
		{
			Error_SetErrorCode((pCreate == NULL) ? ERR_T_MODULE_UNCORRECT : ERR_T_MODULE_INIT_FAILED);
			return FALSE;
		}
#else
		g_pRepresentShell = CreateRepresentShell();
#endif
	}
	if(g_pRepresentShell->Create(nWidth, nHeight, bFullScreen != 0) == 0)
	{
		return TRUE;
	}
	else
	{
		Error_SetErrorCode(g_bRepresent3 ? ERR_T_REPRESENT3_INIT_FAILED : ERR_T_REPRESENT2_INIT_FAILED);
		return FALSE;
	}
}

void KMyApp::AddTrayIcon(HWND hWnd, LPCSTR tip)
{
    NOTIFYICONDATAA nid;
	memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WMAPP_TRAY;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(SWORD_ICON));
    strcpy(nid.szTip, tip);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void KMyApp::AddTrayIconHide(HWND hWnd, LPCSTR tip)
{
	g_bTrayActive = TRUE;
	strcpy(g_szTip, tip);
	AddTrayIcon(hWnd, tip);
	ShowWindow(hWnd, SW_HIDE);
}
#define	NEWINITSCREEN_W	948
#define NEWINITSCREEN_H	640
BOOL KMyApp::GameInit()
{
	g_bTrayActive = FALSE;
	g_uTaskbarCreated = RegisterWindowMessageA("TaskbarCreated");
	Error_SetErrorString("KMyApp::GameInit");
    #ifdef KUI_USE_HARDWARE_MOUSE
	
    ShowMouse(TRUE);
    
    #else   // KUI_USE_HARDWARE_MOUSE
	
    ShowMouse(FALSE);
    
    #endif

	g_SetRootPath(NULL);
	g_SetFilePath("\\");

	KIniFile*	pSetting = g_UiBase.GetCommConfigFile();
	if (pSetting)
	{
		pSetting->GetString("Main", "GameName", "½£ÏÀÇéÔµ¡¤ÍøÂç°æ", g_szGameName, sizeof(g_szGameName));
        SetWindowText(g_GetMainHWnd(), g_szGameName);
	}

//#ifdef _DEBUG
	g_FindDebugWindow("#32770","DebugWin");
//#endif
	//TSetFontType(TRUE); //set font TCVN3 default = TRUE
	KIniFile	IniFile;
	if (!IniFile.Load("\\config.ini"))
	{
		Error_SetErrorCode(ERR_T_FILE_NO_FOUND);
		Error_SetErrorString("\\config.ini");
		return FALSE;
	}
#ifdef _DEBUG
	BOOL		bCursor = FALSE;
	if (IniFile.GetInteger("Client", "ShowCursor", 0, &bCursor))
		ShowMouse(TRUE);
#endif

	IniFile.GetInteger("Client", "FullScreen", FALSE, &g_bScreen);

#ifdef DYNAMIC_LINK_REPRESENT_LIBRARY
	//IniFile.GetInteger("Client", "Represent", 2, &g_bRepresent3);
	//g_bRepresent3 = (g_bRepresent3 == 3);
#endif

	g_PakList.Open("\\package.ini");

	char	szPath[MAX_PATH];
	if (IniFile.GetString("Client", "CapPath", "", szPath, sizeof(szPath)))
	{
		if (szPath[0])
			SetScrPicPath(szPath);
	}

	IniFile.Clear();

	if (!g_libFilterText.Initialize()
		|| !g_ChatFilter.Initialize())
		return FALSE;

	if (!InitRepresentShell(g_bScreen, NEWINITSCREEN_W, NEWINITSCREEN_H))
	{
		return FALSE;
	}
	
	UiSetScreenSize(NEWINITSCREEN_W, NEWINITSCREEN_H);
	
	if (!UiInit())
	{
		Error_SetErrorCode(ERR_T_MODULE_INIT_FAILED);
		Error_SetErrorString("UiInit");
		return FALSE;
	}

	//[wxb 2003-6-23]
	m_pInlinePicSink = new KInlinePicSink;
    if (m_pInlinePicSink)
	{
		m_pInlinePicSink->Init(g_pRepresentShell);
		_ASSERT(NULL == g_pIInlinePicSink);
		g_pIInlinePicSink = m_pInlinePicSink;
	}


	UiPaint(0);

	// init dsound
	m_Sound.Init();

	SetMultiGame(TRUE);

	if ((g_pCoreShell = CoreGetShell()) == NULL)
	{
		Error_SetErrorCode(ERR_T_MODULE_INIT_FAILED);
		Error_SetErrorString("CoreGetShell");
		return false;
	}
	g_pCoreShell->SetRepresentShell(g_pRepresentShell);
	g_pCoreShell->SetMusicInterface((KMusic*)&m_Music);
	g_pCoreShell->SetCallDataChangedNofify(&g_ClientCallback);
	g_pCoreShell->SetRepresentAreaSize(NEWINITSCREEN_W, NEWINITSCREEN_H);
	
	g_pMusic = &m_Music;

	if (g_NetConnectAgent.Initialize() == 0)
	{
		Error_SetErrorCode(ERR_T_MODULE_INIT_FAILED);
		Error_SetErrorString("NetConnectAgent");
		return FALSE;
	}

	m_GameCounter = 0;
	m_PaintStep = GAME_FPS/18;
	m_Timer.Start();
	
	SetMouseHoverTime(150);
	if(UiStart())
	{
		if(!InitMapping())
			return FALSE;
		return TRUE;
	}
	else
	{
		Error_SetErrorCode(ERR_T_MODULE_INIT_FAILED);
		Error_SetErrorString("UiStart");
		return FALSE;
	}
}

BOOL KMyApp::GameExit()
{	
	if (m_pInlinePicSink)
	{
		//[wxb 2003-6-23]
		m_pInlinePicSink->UnInit();
		delete m_pInlinePicSink;
		m_pInlinePicSink = NULL;
		g_pIInlinePicSink = NULL;
	}

	UiExit();

	g_pMusic = NULL;
	if (g_pCoreShell)
	{
		g_pCoreShell->SetRepresentShell(NULL);
		g_pCoreShell->SetClient(NULL);
		g_pCoreShell->SetMusicInterface(NULL);
		g_pCoreShell->Release();
		g_pCoreShell = NULL;
	}

	if (g_pRepresentShell)
	{
		g_pRepresentShell->Release();
		g_pRepresentShell = NULL;
	}

	g_NetConnectAgent.Exit();

	m_Music.Close();
	m_Sound.Exit();

#ifdef DYNAMIC_LINK_REPRESENT_LIBRARY
	if (l_hRepresentModule)
	{
		FreeLibrary(l_hRepresentModule);
		l_hRepresentModule = NULL;
	}
#endif

	::ShowCursor(TRUE);

	g_ChatFilter.Uninitialize();
	g_libFilterText.Uninitialize();
	
//clear auto
	if (g_pState) { UnmapViewOfFile(g_pState); g_pState = NULL; }
	if (g_hMap) { CloseHandle(g_hMap); g_hMap = NULL; }
    if (g_hEventRecv) { CloseHandle(g_hEventRecv); g_hEventRecv = NULL; }
	if (g_pRepState) { UnmapViewOfFile(g_pRepState); g_pRepState = NULL; }
	if (g_hRepMap) { CloseHandle(g_hRepMap); g_hRepMap = NULL; }
    if (g_hRepEvent) { CloseHandle(g_hRepEvent); g_hRepEvent = NULL; }

	return TRUE;
}

bool KMyApp::InitMapping()
{
	char Buffer[128];
	UINT pid = GetCurrentProcessId();
	sprintf(Buffer, "%s%u", MMF_NAME_SERVER, pid);
	g_hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_SIZE, Buffer);
    if (!g_hMap)
		return false;
	g_pState = (SharedState*)MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0,0,0);
    if (!g_pState)
		return false;
    // init struct once
	sprintf(Buffer, "%s%u", EVT_SVRECV, pid);
    g_hEventRecv = CreateEvent(NULL, FALSE, FALSE, Buffer); // auto-reset
	return true;
}

void SendInfoToTool(const void * const pData, const size_t &datalength)
{
	if(!g_pRepState || datalength <= 0)
		return;
	if(g_CurSize + datalength + sizeof(UINT) > SHARED_SIZE)
		return;
	memcpy(&g_Buffer[g_CurSize], pData, datalength);
	g_CurSize += datalength;
	++g_CurNum;
}

void KMyApp::AppSendInfoToTool(const void * const pData, const size_t &datalength)
{
	SendInfoToTool(pData, datalength);
}

void KMyApp::SendAllCommand()
{
	if(!g_pRepState || g_CurSize <= 0)
		return;
	*(UINT*)g_pRepState = g_CurNum;
	memcpy((BYTE*)g_pRepState + sizeof(UINT), g_Buffer, g_CurSize);
	g_CurSize = 0;
	g_CurNum = 0;
	SetEvent(g_hRepEvent);
}

void KMyApp::ExtAutoLogin(const IPCAutoLogin* pALg)
{
	if(!g_pCoreShell)
		return;
	if(g_ALGStep >= 100)
		return;
	KUiInit* pInit = NULL;
	KUiSelServer* pSelsv = NULL;
	if(pInit = KUiInit::GetIfVisible())
	{
		g_ALGStep = 1;
		pInit->AutoLgNextStep();
	}
	else if(pSelsv = KUiSelServer::GetIfVisible())
	{
		g_ALGStep = 2;
		pSelsv->AutoLgNextStep(pALg->nSelSvGroup, pALg->nSelServer);
		if(!KUiSelServer::GetIfVisible())
		{
			g_AGLNextTime = timeGetTime() + 4000;
		}
	}
	else if(g_ALGStep == 2)
	{
		KUiLogin* pLogin = NULL;
		if(pLogin = KUiLogin::GetIfVisible())
		{
			g_ALGStep = 3;
			pLogin->AutoLgNextStep(pALg->szAccount, pALg->szPassword);
			g_AGLNextTime = timeGetTime() + 4000;
		}
		else if(g_AGLNextTime < timeGetTime())
		{
			g_ALGStep = 100;
			PostQuitMessage(0);
			return;
		}
	}
	else if(g_ALGStep == 3)
	{
		KUiSelPlayer* pSelP = NULL;
		if(pSelP = KUiSelPlayer::GetIfVisible())
		{
			g_ALGStep = 4;
			g_AGLNextTime = timeGetTime() + 4000;
			if(!pSelP->AutoLgNextStep(pALg->szName))
			{
				g_ALGStep = 100;
				PostQuitMessage(0);
				return;
			}
		}
		else if(g_AGLNextTime < timeGetTime())
		{
			g_ALGStep = 100;
			PostQuitMessage(0);
			return;
		}
	}
	else if(g_ALGStep == 4)
	{
		UINT uID;
		g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
		if(uID == pALg->dwID)
		{
			g_ALGStep = 100;
		}
		else if(g_AGLNextTime < timeGetTime())
		{
			g_ALGStep = 100;
			PostQuitMessage(0);
			return;
		}
	}
}

void KMyApp::ExtAutoLoop(const autoData* pApData)
{
	if(!g_pCoreShell)
		return;
	Wnd_SetPKKey(pApData->uFKey);
	g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_CHECKTIME, 0);
	if(pApData->bRevive)
	{
		if(PushReviveButton())
			return;
	}
	if(pApData->bOutWhenDis && !pApData->bOnPK)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_DISEXIT, 0))
		{
			PostQuitMessage(0);
			return;
		}
	}
	if(pApData->bOutTimer && !pApData->bOnPK)
	{
    	SYSTEMTIME	sTime;
		GetLocalTime(&sTime);
		if(sTime.wHour == pApData->nHour && sTime.wMinute == pApData->nMinute)
		{
			PostQuitMessage(0);
			return;
		}
	}
	if(!g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0))
		return;
	if(pApData->bOutWhenTP && !pApData->bOnPK)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_EXIT, 0))
		{
			PostQuitMessage(0);
			return;
		}
	}
	int nParam[4];
	if(pApData->bCheckiLife)
	{
		nParam[0] = pApData->nIlifeCell1;
		nParam[1] = pApData->nIlifeCell2;
		nParam[2] = pApData->nIlifeCell3;
		if(nParam[2] < 200)
			nParam[2] = 200;
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PUMPLIFE, (int)&nParam);
	}
	if(pApData->bCheckiMana)
	{
		nParam[0] = pApData->nImanaCell1;
		nParam[1] = pApData->nImanaCell2;
		nParam[2] = pApData->nImanaCell3;
		if(nParam[2] < 200)
			nParam[2] = 200;
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PUMPMANA, (int)&nParam);
	}
	if(pApData->bCheckTPLife)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_CHECKLIFE, pApData->nTPLife))
			return;
	}
	if(pApData->bCheckTPMana)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_CHECKMANA, pApData->nTPMana))
			return;
	}
	if(pApData->bCheckTPLifeGone)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_LIFEGONE, 0))
			return;
	}
	if(pApData->bCheckTPManaGone)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_MANAGONE, 0))
			return;
	}
	if(pApData->bCheckTPIBox)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_FULLITEM, pApData->nTPiboxSel))
			return;
	}
	if(pApData->bCheckTPMoney)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_FULLMONEY, pApData->nTPMoney))
			return;
	}
	if(pApData->bCheckTPIDmg)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_TP_DMGITEM, pApData->nTPDmgItem))
			return;
	}
	if(pApData->bChat)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_CANCHAT, pApData->nChatChann))
		{
			char	Buffer[256];
			int nMsgLength = KUiFaceSelector::ConvertFaceText(Buffer, pApData->szChat, strlen(pApData->szChat));
			nMsgLength = TEncodeText(Buffer, nMsgLength);
			if(pApData->nChatChann == 0)	//chat phu can
			{
				int nChannelDataCount = KUiMsgCentrePad::GetChannelCount();
				for(int i=0;i<nChannelDataCount;++i)
				{
					if(KUiMsgCentrePad::IsChannelType(i, KUiMsgCentrePad::ch_Screen))
					{
						DWORD nChannelID = KUiMsgCentrePad::GetChannelID(i);
						KUiPlayerBar::OnSendChannelMessage(
							nChannelID, Buffer, nMsgLength);
					}
				}
			}
			else	//kenh the gioi
			{
				int i = KUiMsgCentrePad::GetChannelIndex("CH_WORLD");
				if(i >= 0)
				{
					DWORD nChannelID = KUiMsgCentrePad::GetChannelID(i);
					KUiPlayerBar::OnSendChannelMessage(
						nChannelID, Buffer, nMsgLength);
				}
			}
		}
	}
	g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PICKUPSET, Wnd_IsLButtonDown());
	BOOL bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PICKUP, (int)pApData);
	if(bLaunch != 2)
		bLaunch = 0;
	if(pApData->bUseBuff && !bLaunch)
	{
		nParam[0] = pApData->nUseBuffVal;
		nParam[1] = pApData->bPTBuff;
		bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_BASEBUFF, (int)&nParam);
	}
	if(pApData->bCLBuff && !bLaunch)
	{
		bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_CLBUFF, pApData->bCLBuffCamp);
	}
	if(pApData->nSkillIdSP1 && !bLaunch)
	{
		bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_SUPPORTBUFF, pApData->nSkillIdSP1);
	}
	if(pApData->nSkillIdSP2 && !bLaunch)
	{
		bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_SUPPORTBUFF, pApData->nSkillIdSP2);
	}
	if(pApData->nSkillIdSP3 && !bLaunch)
	{
		bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_SUPPORTBUFF, pApData->nSkillIdSP3);
	}
	if(!pApData->bOnPK)
	{
		if(pApData->nSkillIdL)
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_LEFTSKILL, pApData->nSkillIdL);
		if(pApData->nSkillIdR)
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_RIGHTSKILL, pApData->nSkillIdR);
	}
	else if(pApData->bDrawVision)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_ISFIGHTMODE, 0))
		{
			g_DrawVision = pApData->nPKVision;
			if(g_DrawVision < 100)
				g_DrawVision = 100;
			else if(g_DrawVision > 1200)
				g_DrawVision = 1200;
			g_DrawVisionTime = timeGetTime() + 250;
		}
	}
	if(pApData->nSkillIdA1 || pApData->nSkillIdA2)
	{
		nParam[0] = pApData->nSkillIdA1;
		nParam[1] = pApData->nSkillIdA2;
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_CHANGEAURA, (int)&nParam);
	}
	if(!Wnd_IsLButtonDown())
	{
		if(pApData->bOnPK)
		{
			if(pApData->bUseFKey && !Wnd_IsPKKeyDown())
				g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_RESETNPCID, 0);
			if(!bLaunch && (!pApData->bUseFKey || Wnd_IsPKKeyDown()))
			bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PKFIGHT, (int)pApData);
		}
		else
		{
			if(!bLaunch)
			{
				BOOL bMoving = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_MOVE, (int)pApData);
				if(!bMoving && pApData->bFight)
				{
					bLaunch = g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_FIGHT, (int)pApData);
					if(bLaunch == 2)
					{
						PostQuitMessage(0);
						return;
					}
					if(!bLaunch)
						g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_RESETMOVE, 0);
				}
			}
		}
	}
	else if(pApData->bOnPK)
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_RESETNPCID, 0);
	
	if(pApData->bEatPoison)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_EATPOISON, 0))
			return;
	}
	if(pApData->bEatLifeFull)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_EATLIFEFULL, 0))
			return;
	}
	if(pApData->bEatExp)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_EATEXPX2, 0))
			return;
	}
	if(pApData->bEatSkill)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_EATSKILLX2, 0))
			return;
	}
	if(pApData->bOpenBag)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_OPENBAG, 0))
			return;
	}
	if(pApData->bArrangeI)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_ARRANGEITEM, 0))
			return;
	}
	if(pApData->bArrangeB)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_ARRANGEBOX, 0))
			return;
	}
	if(!Wnd_IsLButtonDown())
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_FILTER, (int)pApData))
			return;
	}
	if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PTPROC, (int)pApData))
		return;
	if(pApData->nSelInvitePt)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PTINVITE, (int)pApData))
			return;
	}
	if(pApData->nSelJoinPt)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_PTJOIN, (int)pApData))
			return;
	}
	if(pApData->bFRepair)
	{
		if(g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_REPAIRF, 0))
			return;
	}
	if(pApData->bReturn && !g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_ISFIGHTMODE, 0)
	&& !Wnd_IsLButtonDown())
	{
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_RETURN, (int)pApData);
	}
}

void KMyApp::ProcIpcCommand()
{
	if(!g_pState)
		return;
	if (WaitForSingleObject(g_hEventRecv, 0) == WAIT_OBJECT_0)
	{
		HWND hWnd = g_GetMainHWnd();
		UINT uCount = *(UINT*)g_pState;
		SharedState* p = (SharedState*)((BYTE*)g_pState + sizeof(UINT));
		for(UINT c = 0; c<uCount; ++c)
		{
			switch(p->CmdID)
			{
			case PRT_CONNECT:
			{
				char Buffer[128];
				UINT pid = GetCurrentProcessId();
				sprintf(Buffer, "%s%u", MMF_NAME_CLIENT, pid);
				g_hRepMap = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, Buffer);
				g_pRepState = (SharedState*)MapViewOfFile(g_hRepMap, FILE_MAP_READ | FILE_MAP_WRITE, 0,0,0);
				sprintf(Buffer, "%s%u", EVT_CLRECV, pid);
				g_hRepEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, Buffer);
				SharedState s;
				s.CmdID = PRG_REPLYRECVED;
				s.Size = sizeof(SharedState);
				SendInfoToTool(&s, sizeof(SharedState));
			}
			break;
			case PRT_GAMELOOP:
			{
				IPCGameLoop* pGL = (IPCGameLoop*)p;
				ExtAutoLoop(&pGL->setting);
			}
			break;
			case PRT_HIDEGAME:
			{
				IPCHideGame* pCmd = (IPCHideGame*)p;
				if(pCmd->bHide)
				{
					if(g_bTrayActive)
						break;
					KUiPlayerBaseInfo	Info;
					memset(&Info, 0, sizeof(KUiPlayerBaseInfo));
					g_pCoreShell->GetGameData(GDI_PLAYER_BASE_INFO, (int)&Info, 0);
					if(Info.Name[0])
						AddTrayIconHide(hWnd, Info.Name);
					else
						AddTrayIconHide(hWnd, "< >");
				}
				else
				{
					if(!g_bTrayActive)
						break;
					g_bTrayActive = FALSE;
					NOTIFYICONDATAA nid;
					memset(&nid, 0, sizeof(nid));
					nid.cbSize = sizeof(nid);
					nid.hWnd = hWnd;
					nid.uID = ID_TRAYICON;
					Shell_NotifyIcon(NIM_DELETE, &nid);
					ShowWindow(hWnd, SW_RESTORE);
					SetForegroundWindow(hWnd);
				}
			}
			break;
			case PRT_ISHIDE:
			{
				IPCHideGame s;
				s.CmdID = PRG_REPISHIDE;
				s.Size = sizeof(IPCHideGame);
				s.bHide = g_bTrayActive;
				SendInfoToTool(&s, sizeof(IPCHideGame));
			}
			break;
			case PRT_TICKSTART:
			{
				IPCHideGame* pCmd = (IPCHideGame*)p;
				g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_CLEAR, pCmd->bHide);
			}
			break;
			case PRT_RETONOFPK:
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0))
				{
					IPCHideGame* pCmd = (IPCHideGame*)p;
					char szInfo[64];
					if(pCmd->bHide)
						strcpy(szInfo, "BËt chÕ ®é AutoPK");
					else
						strcpy(szInfo, "ChuyÓn vÒ luyÖn c«ng, t¾t AutoPK");
					KUiMsgCentrePad::SystemMessageArrival(szInfo, (unsigned short)strlen(szInfo));
				}
			}
			break;
			case PRT_RETAUTOONOF:
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0))
				{
					IPCHideGame* pCmd = (IPCHideGame*)p;
					char szInfo[64];
					if(pCmd->bHide)
						strcpy(szInfo, "KÝch ho¹t Auto bªn ngoµi");
					else
						strcpy(szInfo, "Ng­ng ho¹t ®éng Auto bªn ngoµi");
					KUiMsgCentrePad::SystemMessageArrival(szInfo, (unsigned short)strlen(szInfo));
				}
			}
			break;
			case PRT_GETITEMNAME:
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0))
				{
					char szBuffer[4812];
					SharedState* pN = (SharedState*)&szBuffer[0];
					pN->CmdID = PRG_OPENNOPICK;
					int* pCount = (int*)(pN+1);
					char* pName = (char*)(pCount+1);
					*pCount = g_pCoreShell->OperationRequest(
						GOI_AUTOPLAY_ACTION, ATYPE_GETITEMNAME, (int)pName);
					pN->Size = sizeof(SharedState) + sizeof(int) + (*pCount)*80;
					SendInfoToTool(pN, pN->Size);
				}
			}
			break;
			case PRT_GETTEAMAROUND:
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0))
				{
					char szBuffer[3232];
					IPCHideGame* pCmd = (IPCHideGame*)p;
					SharedState* pN = (SharedState*)&szBuffer[0];
					pN->CmdID = PRG_TEAMNAMELIST;
					int* pCount = (int*)(pN+1);
					*pCount++ = pCmd->bHide;
					char* pName = (char*)(pCount+1);
					*pCount = g_pCoreShell->OperationRequest(
						GOI_AUTOPLAY_ACTION, ATYPE_GETAROUNDNAME, (int)pName);
					pN->Size = sizeof(SharedState) + sizeof(int)*2 + (*pCount)*32;
					SendInfoToTool(pN, pN->Size);
				}
			}
			break;
			case PRT_ACTAUTOLG:
			{
				IPCAutoLogin* pCmd = (IPCAutoLogin*)p;
				ExtAutoLogin(pCmd);
			}
			break;
			case PRT_QUITGAME:
			{
				PostQuitMessage(0);
				return;
			}
			break;
			}
			p = (SharedState*)((BYTE*)p + p->Size);
		}
	}
}
//bForceCoreLoop lµ resize cöa sæ míi x¶y ra
BOOL KMyApp::GameLoop(bool bForceCoreLoop)
{
	static int nGameFps = 0;
	static int nCurStep = 0;
	if(!nCurStep)
		g_NetConnectAgent.Breathe();
	if(g_DrawVisionTime < timeGetTime())
		g_DrawVision = 0;
	ProcIpcCommand();
	if (m_GameCounter * 1000 <= m_Timer.GetElapse() * GAME_FPS)
	{
		m_GameCounter++;
		UiUpdateTickTime();
		if(!nCurStep || bForceCoreLoop)
		{
			if(bForceCoreLoop)
			g_pCoreShell->OperationRequest(GOI_PROCFRAME_BREATHE, 2, 0);//ScenePlace set force lookat
			if (!g_pCoreShell->Breathe() || !UiHeartBeat())
				return false;
		}
		INT64 nElapse = m_Timer.GetElapse();
		if (nElapse)
		{
			nGameFps = m_GameCounter * 1000 / nElapse;
			if(nGameFps < GAME_FPS-4)
			{
				m_GameCounter = nElapse * GAME_FPS / 1000;
			}
		}
		g_pCoreShell->OperationRequest(GOI_PROCFRAME_BREATHE, 1, 0);//MessageLoop
		g_pCoreShell->OperationRequest(GOI_PROCFRAME_POSSHIFT, GAME_FPS, 18);
		g_pCoreShell->OperationRequest(GOI_PROCFRAME_BREATHE, 0, 0);//ScenePlace
		g_pCoreShell->OperationRequest(GOI_AUTOPLAY_ACTION, ATYPE_DRAWVISION, g_DrawVision);
		UiPaint(nGameFps);
		nCurStep++;
		if(nCurStep >= m_PaintStep)
			nCurStep = 0;
	}
	SendAllCommand();
	/*if (m_GameCounter * 1000 >= m_Timer.GetElapse() * GAME_FPS)
	{
		UiPaint(nGameFps);
		Sleep(1);
	}
	else if ((m_GameCounter % 8) == 0)
	{
		Sleep(1);
	}*/

	return true;
}

int KMyApp::HandleInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nRet = 0;
	if (uMsg != WM_CLOSE)
	{
		UiProcessInput(uMsg, wParam, lParam);
	}
	else if (g_bScreen == false && UiIsAlreadyQuit() == false)
	{
		KIniFile*	pSetting = g_UiBase.GetCommConfigFile();
		if (pSetting)
		{
			char	szMsg[128], szTitle[64];
			pSetting->GetString("InfoString", QUIT_QUESTION_ID, "", szMsg, sizeof(szMsg));
			pSetting->GetString("InfoString", GAME_TITLE, "", szTitle, sizeof(szTitle));
			if (szMsg[0] && szTitle[0])
			{
				nRet = (MessageBox(g_GetMainHWnd(), szMsg, szTitle,
					MB_YESNO | MB_ICONQUESTION) != IDYES);
			}
		}
	}
	return nRet;
}
