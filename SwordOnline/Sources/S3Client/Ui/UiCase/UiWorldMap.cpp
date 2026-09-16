/* 
 * File:     UiWorldMap.cpp
 * Desc:     世界地图
 * Author:   flying
 * Creation: 2003/7/22
 */
//-----------------------------------------------------------------------------
#include "KWin32.h"
#include "KIniFile.h"
#include "../elem/wnds.h"
#include "../Elem/WndMessage.h"
#include <crtdbg.h>
//#include "../ShortcutKey.h"
#include "UiWorldMap.h"
#include "../UiBase.h"
#include "../UiSoundSetting.h"
#include "../../../Represent/iRepresent/iRepresentShell.h"
#include "../../../Represent/iRepresent/KRepresentUnit.h"
#include "../../../core/src/CoreShell.h"
#include "../../../core/src/GameDataDef.h"

extern iRepresentShell*	g_pRepresentShell;
extern iCoreShell*		g_pCoreShell;

KUiWorldmap* KUiWorldmap::m_pSelf = NULL;

#define		SCHEME_INI_WORLD		"小地图_世界地图版.ini"
#define		WORLD_MAP_INFO_FILE		"\\Settings\\MapList.ini"

void MapToggleStatus();

static struct EMT_MAPTYPE
{
	int				nIndex;
	const char*		pszMapType;
}sMapType_Map[EMT_COUNT] =
{
	{ EMT_City,			"City"			},
	{ EMT_Capital,		"Capital"		},
	{ EMT_Cave,			"Cave"			},
	{ EMT_Battlefield,	"Battlefield"	},
	{ EMT_Field,		"Field"			},
	{ EMT_Country,		"Country"		},
	{ EMT_Tong,			"Tong"			},
	{ EMT_Others,		"Others"		},
};

KUiWorldmap* KUiWorldmap::OpenWindow()
{
	if (m_pSelf == NULL)
	{
		m_pSelf = new KUiWorldmap;
		if (m_pSelf)
			m_pSelf->Initialize();
	}

	if (m_pSelf)
	{
		UiSoundPlay(UI_SI_WND_OPENCLOSE);
		m_pSelf->UpdateData();
		m_pSelf->Show();
		m_pSelf->BringToTop();
		Wnd_SetExclusive(m_pSelf);
	}
	return m_pSelf;
}

void KUiWorldmap::CloseWindow()
{
	if (m_pSelf)
	{
		Wnd_ReleaseExclusive(m_pSelf);
		m_pSelf->Destroy();
		m_pSelf = NULL;
		MapToggleStatus();
	}
}

KUiWorldmap* KUiWorldmap::GetIfVisible()
{
	if (m_pSelf && m_pSelf->IsVisible())
		return m_pSelf;
	else
		return NULL;
}

//初始化
void KUiWorldmap::Initialize()
{
	AddChild(&m_Sign);
	AddChild(&m_CloseBtn);
	int i;
	for(i = 0; i < EMT_COUNT; ++i)
		AddChild(&m_TypeImg[i]);
	char szBuffer[128];
	g_UiBase.GetCurSchemePath(szBuffer, sizeof(szBuffer));
	strcat(szBuffer, "\\" SCHEME_INI_WORLD);
	KIniFile	Ini;
	if (Ini.Load(szBuffer))
	{
		Init(&Ini, "WorldMap");
		m_Sign.Init(&Ini, "Sign");
		m_CloseBtn.Init(&Ini, "CloseBtn");
		for(i = 0; i < EMT_COUNT; ++i)
			m_TypeImg[i].Init(&Ini, sMapType_Map[i].pszMapType);
	}

	Wnd_AddWindow(this, WL_TOPMOST);
	return;
}

int KUiWorldmap::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	int nResult = false;

	switch(uMsg)
	{
	case WND_N_BUTTON_CLICK:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_KEYDOWN:
		CloseWindow();
		nResult = true;
		break;
	default:
		nResult = KWndWindow::WndProc(uMsg, uParam, nParam);
		break;
	}
	return nResult;
}

void KUiWorldmap::UpdateData()
{
	m_Sign.Hide();
	int i;
	for(i = 0; i < EMT_COUNT; ++i)
		m_TypeImg[i].Hide();
	if (g_pCoreShell)
	{
		KIniFile	Ini;
		if (Ini.Load(WORLD_MAP_INFO_FILE))
		{
			//取得世界地图的图形文件明
			char	szBuffer[128];
			//if (Ini.GetString("List", "WorldMapImage", "", szBuffer, sizeof(szBuffer)))
			//{
				//SetImage(ISI_T_BITMAP16, szBuffer, true);

				int nAreaX = -1, nAreaY = 0;
				KUiSceneTimeInfo Info;
				g_pCoreShell->SceneMapOperation(GSMOI_SCENE_TIME_INFO, (unsigned int)&Info, 0);
				sprintf(szBuffer, "%d_MapPos", Info.nSceneId);
				Ini.GetInteger2("List", szBuffer, &nAreaX, &nAreaY);
				if (nAreaX != -1)
				{
					int nWidth, nHeight;
					m_Sign.GetSize(&nWidth,  &nHeight);
					m_Sign.SetPosition(nAreaX - nWidth / 2, nAreaY - nHeight / 2);
					m_Sign.Show();
					char szBuff[32];
					sprintf(szBuffer, "%d_MapType", Info.nSceneId);
					Ini.GetString("List", szBuffer, "", szBuff, sizeof(szBuff));
					int nMTId = 7; //default EMT_Others
					for(i = 0; i<EMT_COUNT;++i)
					{
						if(!strcmpi(szBuff, sMapType_Map[i].pszMapType))
						{
							nMTId = i;
							break;
						}
					}
					m_TypeImg[nMTId].GetSize(&nWidth,  &nHeight);
					m_TypeImg[nMTId].SetPosition(nAreaX+27 - nWidth / 2, nAreaY+20 - nHeight / 2);
					m_TypeImg[nMTId].Show();
				}
			//}
		}
	}
}

//活动函数
void KUiWorldmap::Breathe()
{
	if (m_Sign.IsVisible())
		m_Sign.NextFrame();
	for(int i = 0; i<EMT_COUNT;++i)
	{
		if (m_TypeImg[i].IsVisible())
		{
			m_TypeImg[i].NextFrame();
			break;
		}
	}
}