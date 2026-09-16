/*****************************************************************************************
//	½çÃæ--×´Ì¬½çÃæ
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-2
*****************************************************************************************/
#include "KWin32.h"
#include "KIniFile.h"
#include "../Elem/WndMessage.h"
#include "../elem/wnds.h"
#include "UiParadeItem.h"
#include "../ShortcutKey.h"
#include "../UiSoundSetting.h"
#include "../../../core/src/coreshell.h"
#include "../UiBase.h"

extern iCoreShell*		g_pCoreShell;

#define	SCHEME_INI		"¿´¶Ô·½Íæ¼Ò×°±¸.ini"

KUiParadeItem* KUiParadeItem::m_pSelf = NULL;

// -------------------------------------------------------------------------
// ---> ½¨Á¢¿Ø¼þÓëUIEP_*ÒÔ¼°¿É½ÓÄÉÎïÆ·µÄÀàÐÍµÄ¶ÔÓ¦¹ØÏµ
static struct UE_CTRL_MAP
{
	int				nPosition;
	const char*		pIniSection;
}CtrlItemMap[_ITEM_COUNT] =
{
	{ UIEP_HEAD,		"Cap"		},	//nãn
	{ UIEP_HAND,		"Weapon"	},	//vò khÝ
	{ UIEP_NECK,		"Necklace"	},	//d©y chuyÒn
	{ UIEP_FINESSE,		"Bangle"	},	//bao tay
	{ UIEP_BODY,		"Cloth"		},	//¸o
	{ UIEP_WAIST,		"Sash"		},	//®ai l­ng
	{ UIEP_FINGER1,		"Ring1"		},	//nhÉn
	{ UIEP_FINGER2,		"Ring2"		},	//nhÉn
	{ UIEP_WAIST_DECOR,	"Pendant"	},	//ngäc béi
	{ UIEP_FOOT,		"Shoes"		},	//giµy
	{ UIEP_HORSE,		"Horse"		},	//ngùa
	{ UIEP_MASK,		"Mask"		},	//mÆt n¹
	{ UIEP_MANTLE,		"Mantle"	},	//phi phong
	{ UIEP_SIGNET,		"Signet"	},	//Ên
	{ UIEP_SHIPIN,		"Shipin"	},	//trang søc
};


//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÈç¹û´°¿ÚÕý±»ÏÔÊ¾£¬Ôò·µ»ØÊµÀýÖ¸Õë
//--------------------------------------------------------------------------
KUiParadeItem* KUiParadeItem::GetIfVisible()
{
	if (m_pSelf && m_pSelf->IsVisible())
		return m_pSelf;
	return NULL;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º´ò¿ª´°¿Ú£¬·µ»ØÎ¨Ò»µÄÒ»¸öÀà¶ÔÏóÊµÀý
//--------------------------------------------------------------------------
KUiParadeItem* KUiParadeItem::OpenWindow(KUiPlayerItem* pDest)
{
	if (m_pSelf == NULL)
	{
		m_pSelf = new KUiParadeItem;
		if (m_pSelf)
			m_pSelf->Initialize();
	}
	if (m_pSelf)
	{
		m_pSelf->UpdateData(pDest);
		
		char Scheme[256];
		g_UiBase.GetCurSchemePath(Scheme, 256);
		LoadScheme(Scheme);

		m_pSelf->BringToTop();
		UiSoundPlay(UI_SI_WND_OPENCLOSE);
		m_pSelf->Show();
	}
	return m_pSelf;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¹Ø±Õ´°¿Ú£¬Í¬Ê±¿ÉÒÔÑ¡ÔòÊÇ·ñÉ¾³ý¶ÔÏóÊµÀý
//--------------------------------------------------------------------------
void KUiParadeItem::CloseWindow(bool bDestroy)
{
	if (m_pSelf)
	{
		if (bDestroy == false)
			m_pSelf->Hide();
		else
		{
			m_pSelf->Destroy();
			m_pSelf = NULL;
		}

		if (g_pCoreShell)
			g_pCoreShell->OperationRequest(GOI_VIEW_PLAYERITEM_END, 0, 0);
	}
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º³õÊ¼»¯
//--------------------------------------------------------------------------
void KUiParadeItem::Initialize()
{
	AddChild(&m_Name);
	AddChild(&m_Title);

	for (int i = 0; i < _ITEM_COUNT; i ++)
	{
		m_EquipBox[i].SetObjectGenre(CGOG_ITEM);
		AddChild(&m_EquipBox[i]);
		m_EquipBox[i].SetContainerId((int)UOC_EQUIPTMENT);
	}

	AddChild(&m_Close);

	Wnd_AddWindow(this);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÔØÈë´°¿ÚµÄ½çÃæ·½°¸
//--------------------------------------------------------------------------
void KUiParadeItem::LoadScheme(const char* pScheme)
{
	if (m_pSelf)
	{
		char		Buff[128];
		KIniFile	Ini;
		sprintf(Buff, "%s\\%s", pScheme, SCHEME_INI);
		if (Ini.Load(Buff))
			m_pSelf->LoadScheme(&Ini);	
	}
}

//ÔØÈë½çÃæ·½°¸
void KUiParadeItem::LoadScheme(class KIniFile* pIni)
{
	if (g_pCoreShell->GetGameData(GDI_PLAYER_IS_MALE, 0, m_Dest.uId))
		Init(pIni, "Male");
	else
		Init(pIni, "Female");

	m_Face    .Init(pIni, "Face");
	m_Name    .Init(pIni, "Name");
	m_Title   .Init(pIni, "Title");

	m_Close	 .Init(pIni, "Close");

	for (int i = 0; i < _ITEM_COUNT; i ++)
	{
		m_EquipBox[i].Init(pIni, CtrlItemMap[i].pIniSection);
	}
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º´°¿Úº¯Êý
//--------------------------------------------------------------------------
int KUiParadeItem::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uMsg)
	{
	case WND_N_BUTTON_CLICK:
		if (uParam == (unsigned int)(KWndWindow*)&m_Close)
			Hide();
		break;
	default:
		nRet = KWndShowAnimate::WndProc(uMsg, uParam, nParam);
	}
	return nRet;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¸üÐÂ»ù±¾Êý¾Ý£¨ÈËÃûµÈ²»Ò×±äÊý¾Ý£©
//--------------------------------------------------------------------------
void KUiParadeItem::UpdateBaseData(KUiPlayerItem* pDest)
{
	if (!g_pCoreShell)
		return;

	KUiPlayerBaseInfo	Info;
	memset(&Info, 0, sizeof(KUiPlayerBaseInfo));
	g_pCoreShell->GetGameData(GDI_PLAYER_BASE_INFO, (int)&Info, pDest->uId);
	m_Name  .SetText(Info.Name);
	m_Title .SetText(Info.Title);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¸üÐÂÊý¾Ý
//--------------------------------------------------------------------------
void KUiParadeItem::UpdateData(KUiPlayerItem* pDest)
{
	if (pDest == NULL)
		return;

	UpdateBaseData(pDest);

	UpdateAllEquips(pDest);

	m_Dest = *pDest;
}

void KUiParadeItem::UpdateAllEquips(KUiPlayerItem* pDest)
{
	if (!g_pCoreShell)
		return;

	KUiObjAtRegion	Equips[_ITEM_COUNT];
	int nCount = g_pCoreShell->GetGameData(GDI_PARADE_EQUIPMENT, (unsigned int)&Equips, 0);
	int	i;
	for (i = 0; i < _ITEM_COUNT; i++)
		m_EquipBox[i].Celar();
	for (i = 0; i < nCount; i++)
	{
		if (Equips[i].Obj.uGenre != CGOG_NOTHING)
			UpdateEquip(&Equips[i], true);
	}
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º×°±¸±ä»¯¸üÐÂ
//--------------------------------------------------------------------------
void KUiParadeItem::UpdateEquip(KUiObjAtRegion* pEquip, int bAdd)
{
	if (pEquip)
	{
		for (int i = 0; i < _ITEM_COUNT; i++)
		{
			if (CtrlItemMap[i].nPosition == pEquip->Region.v)
			{
				if (bAdd)
					m_EquipBox[i].HoldObject(pEquip->Obj.uGenre, pEquip->Obj.uId,
						pEquip->Region.Width, pEquip->Region.Height);
				else
					m_EquipBox[i].HoldObject(CGOG_NOTHING, 0, 0, 0);
				break;
			}
		}
	}
}
