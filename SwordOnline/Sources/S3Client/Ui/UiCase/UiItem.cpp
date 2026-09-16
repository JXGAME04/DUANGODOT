// -------------------------------------------------------------------------
//	ÎÄ¼þÃû		£º	UiItem.cpp
//	Author		£º	ÂÀ¹ð»ª, Wooy(Wu yue)
//	´´½¨Ê±¼ä	£º	2002-9-16 11:26:43
//	¹¦ÄÜÃèÊö	£º	
// -------------------------------------------------------------------------
#include "KWin32.h"
#include "KIniFile.h"
#include "../elem/wnds.h"
#include "uitrade.h"
#include "uiitem.h"
#include "UiGetMoney.h"
#include "UiStoreBox.h"
#include "UiTradeConfirmWnd.h"
#include "UiShop.h"
#include "../../../core/src/coreshell.h"
#include "../../../core/src/GameDataDef.h"
#include "../UiBase.h"
#include "../ShortcutKey.h"
#include <crtdbg.h>
#include "../UiSoundSetting.h"
#include "../../../Engine/src/KDebug.h"
extern iCoreShell*		g_pCoreShell;

#define SCHEME_INI_ITEM	"ËæÉíÎïÆ·.ini"

KUiItem* KUiItem::m_pSelf = NULL;

enum WAIT_OTHER_WND_OPER_PARAM
{
	UIITEM_WAIT_GETMONEY,
};


//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÈç¹û´°¿ÚÕý±»ÏÔÊ¾£¬Ôò·µ»ØÊµÀýÖ¸Õë
//--------------------------------------------------------------------------
KUiItem* KUiItem::GetIfVisible()
{
	if (m_pSelf && m_pSelf->IsVisible())
		return m_pSelf;
	return NULL;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º´ò¿ª´°¿Ú£¬·µ»ØÎ¨Ò»µÄÒ»¸öÀà¶ÔÏóÊµÀý
//--------------------------------------------------------------------------
KUiItem* KUiItem::OpenWindow()
{
	if (m_pSelf == NULL)
	{
		m_pSelf = new KUiItem;
		if (m_pSelf)
			m_pSelf->Initialize();
	}
	if (m_pSelf)
	{
		UiSoundPlay(UI_SI_WND_OPENCLOSE);
		m_pSelf->UpdateData();
		m_pSelf->BringToTop();
		m_pSelf->Show();
	}
	return m_pSelf;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¹Ø±Õ´°¿Ú£¬Í¬Ê±¿ÉÒÔÑ¡ÔòÊÇ·ñÉ¾³ý¶ÔÏóÊµÀý
//--------------------------------------------------------------------------
void KUiItem::CloseWindow(bool bDestroy)
{
	if (m_pSelf)
	{
		KUiShop::CancelTrade();
		if (bDestroy == false)
			m_pSelf->Hide();
		else
		{
			m_pSelf->Destroy();
			m_pSelf = NULL;
		}
	}
}

// -------------------------------------------------------------------------
// ¹¦ÄÜ	: ³õÊ¼»¯
// -------------------------------------------------------------------------
void KUiItem::Initialize()
{
	AddChild(&m_Money);
	AddChild(&m_GetMoneyBtn);
	AddChild(&m_CloseBtn);
	AddChild(&m_ItemBox);
	AddChild(&m_OpenStatusPadBtn);
	AddChild(&m_MoneyIcon);
	AddChild(&m_EnergyIcon);
	AddChild(&m_GoldCoin);
	AddChild(&m_MakeAdvBtn);
	AddChild(&m_MarkPriceBtn);
	AddChild(&m_MakeStallBtn);
	
	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	m_ItemBox.SetContainerId((int)UOC_ITEM_TAKE_WITH);
	m_nMoney = 0;
	Wnd_AddWindow(this);
}

//»î¶¯º¯Êý
void KUiItem::Breathe()
{
	m_nMoney = g_pCoreShell->GetGameData(GDI_PLAYER_HOLD_MONEY, 0, 0);
	char szBuff[64];
	int nGroup = m_nMoney/10000;
	int nMod = m_nMoney % 10000;
	if(nGroup == 0)
		sprintf(szBuff, "%d l­îng", nMod);
	else if(nMod == 0)
		sprintf(szBuff, "%d v¹n l­îng", nGroup);
	else
		sprintf(szBuff, "%d v¹n %d l­îng", nGroup, nMod);
	m_Money.SetText(szBuff);
	int nCoin = 0; // ch­a cã code
	sprintf(szBuff, "%d Xu", nCoin);
	m_GoldCoin.SetText(szBuff);
}

void KUiItem::OnNpcTradeMode(bool bTrue)
{
	if (m_pSelf)
		m_pSelf->m_ItemBox.EnablePickPut(!bTrue);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¹¹Ôìº¯Êý
//--------------------------------------------------------------------------
void KUiItem::UpdateData()
{
	m_ItemBox.Clear();

	//m_nMoney = g_pCoreShell->GetGameData(GDI_PLAYER_HOLD_MONEY, 0, 0);
	//m_Money.SetIntText(m_nMoney);

	KUiObjAtRegion* pObjs = NULL;
	int nCount = g_pCoreShell->GetGameData(GDI_ITEM_TAKEN_WITH, 0, 0);
	if (nCount == 0)
		return;

	if (pObjs = (KUiObjAtRegion*)malloc(sizeof(KUiObjAtRegion) * nCount))
	{
		g_pCoreShell->GetGameData(GDI_ITEM_TAKEN_WITH, (unsigned int)pObjs, nCount);//µ¥Ïß³ÌÖ´ÐÐ£¬nCountÖµ²»±ä
		for (int i = 0; i < nCount; i++)
		{
			KUiDraggedObject no;
			no.uGenre = pObjs[i].Obj.uGenre;
			no.uId = pObjs[i].Obj.uId;
			no.DataX = pObjs[i].Region.h;
			no.DataY = pObjs[i].Region.v;
			no.DataW = pObjs[i].Region.Width;
			no.DataH = pObjs[i].Region.Height;
			m_ItemBox.AddObject(&no, 1);
		}
		free(pObjs);
		pObjs = NULL;
	}
}

// -------------------------------------------------------------------------
// ¹¦ÄÜ	: ÎïÆ·±ä»¯¸üÐÂ
// -------------------------------------------------------------------------
void KUiItem::UpdateItem(KUiObjAtRegion* pItem, int bAdd)
{
	if (pItem)
	{
		KUiDraggedObject Obj;
		Obj.uGenre = pItem->Obj.uGenre;
		Obj.uId = pItem->Obj.uId;
		Obj.DataX = pItem->Region.h;
		Obj.DataY = pItem->Region.v;
		Obj.DataW = pItem->Region.Width;
		Obj.DataH = pItem->Region.Height;
		if (bAdd)
			m_ItemBox.AddObject(&Obj, 1);
		else
			m_ItemBox.RemoveObject(&Obj);
		UiSoundPlay(UI_SI_PICKPUT_ITEM);
	}
	else
		UpdateData();
}

// -------------------------------------------------------------------------
// ¹¦ÄÜ	: ÔØÈë½çÃæ·½°¸
// -------------------------------------------------------------------------
void KUiItem::LoadScheme(const char* pScheme)
{
	char		Buff[128];
	KIniFile	Ini;
	sprintf(Buff, "%s\\%s", pScheme, SCHEME_INI_ITEM);
	if (m_pSelf && Ini.Load(Buff))
	{
		m_pSelf->Init(&Ini, "Main");
		m_pSelf->m_Money.Init(&Ini, "Money");
		m_pSelf->m_GetMoneyBtn.Init(&Ini, "GetMoneyBtn");
		m_pSelf->m_CloseBtn.Init(&Ini, "CloseBtn");
		m_pSelf->m_ItemBox.Init(&Ini, "ItemBox");
		m_pSelf->m_OpenStatusPadBtn.Init(&Ini, "OpenStatus");
		m_pSelf->m_ItemBox.EnableTracePutPos(true);
		m_pSelf->m_MoneyIcon.Init(&Ini, "MoneyIcon");
		m_pSelf->m_EnergyIcon.Init(&Ini, "EnergyIcon");
		m_pSelf->m_GoldCoin.Init(&Ini, "GoldCoin");
		m_pSelf->m_MakeAdvBtn.Init(&Ini, "MakeAdvBtn");
		m_pSelf->m_MarkPriceBtn.Init(&Ini, "MarkPriceBtn");
		m_pSelf->m_MakeStallBtn.Init(&Ini, "MakeStallBtn");
	}
}

void KUiItem::OnClickItem(KUiDraggedObject* pItem, bool bDoImmed)
{
	if (pItem == NULL || g_pCoreShell == NULL)
		return;
	KUiObjAtContRegion	Obj;
	Obj.Obj.uGenre = pItem->uGenre;
	Obj.Obj.uId = pItem->uId;
	Obj.Region.h = pItem->DataX;
	Obj.Region.v = pItem->DataY;
	Obj.Region.Width  = pItem->DataW;
	Obj.Region.Height = pItem->DataH;
	Obj.eContainer = UOC_ITEM_TAKE_WITH;

	if (bDoImmed == false)
	{
		KUiItemBuySelInfo	Price = { 0 };
		if (g_pCoreShell->GetGameData(GDI_TRADE_ITEM_PRICE,
			(unsigned int)(&Obj), (int)(&Price)))
		{
			KUiTradeConfirm::OpenWindow(&Obj, &Price, TCA_SALE);
		}
	}
	else
	{
		UISYS_STATUS eStatus = g_UiBase.GetStatus();
		if (eStatus == UIS_S_TRADE_SALE || eStatus == UIS_S_TRADE_NPC)
		{
			g_pCoreShell->OperationRequest(GOI_TRADE_NPC_SELL,
				(unsigned int)(&Obj), 0);
		}
		else if (g_UiBase.IsOperationEnable(UIS_O_USE_ITEM))
		{
			if (KUiStoreBox::GetIfVisible())
			{
				unsigned int uParam[2];
				uParam[0] = Obj.Obj.uId;
				uParam[1] = pos_equiproom;
				g_pCoreShell->OperationRequest(GOI_EXCHANGEITEM,
								(unsigned int)&uParam, pos_repositoryroom);
			}
			else
			{
				Obj.Region.Width = pos_equip;
				g_pCoreShell->OperationRequest(GOI_USE_ITEM,
					(unsigned int)(&Obj), UOC_ITEM_TAKE_WITH);
			}
		}
	}
}

void KUiItem::OnRepairItem(KUiDraggedObject* pItem)
{
	if (pItem == NULL || g_pCoreShell == NULL)
		return;
	KUiObjAtContRegion	Obj;
	Obj.Obj.uGenre = pItem->uGenre;
	Obj.Obj.uId = pItem->uId;
	Obj.Region.h = pItem->DataX;
	Obj.Region.v = pItem->DataY;
	Obj.Region.Width  = pItem->DataW;
	Obj.Region.Height = pItem->DataH;
	Obj.eContainer = UOC_ITEM_TAKE_WITH;

	KUiItemBuySelInfo	Price = { 0 };
	if (g_pCoreShell->GetGameData(GDI_REPAIR_ITEM_PRICE,
		(unsigned int)(&Obj), (int)(&Price)))
	{
		KUiTradeConfirm::OpenWindow(&Obj, &Price, TCA_REPAIR);
	}
}

// -------------------------------------------------------------------------
// ¹¦ÄÜ	: ´°¿Úº¯Êý
// -------------------------------------------------------------------------
int KUiItem::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	switch(uMsg)
	{
	case WND_N_LEFT_CLICK_ITEM:
		if (g_UiBase.GetStatus() == UIS_S_TRADE_SALE)
			OnClickItem((KUiDraggedObject*)uParam, true);
		else if (g_UiBase.GetStatus() == UIS_S_TRADE_NPC)
			OnClickItem((KUiDraggedObject*)uParam, false);
		else if (g_UiBase.GetStatus() == UIS_S_TRADE_REPAIR)
			OnRepairItem((KUiDraggedObject*)uParam);
		break;
	case WND_N_RIGHT_CLICK_ITEM:
		OnClickItem((KUiDraggedObject*)uParam, true);
		break;
	case WND_N_ITEM_PICKDROP:
		OnItemPickDrop((ITEM_PICKDROP_PLACE*)uParam, (ITEM_PICKDROP_PLACE*)nParam);
		break;
	case WND_N_BUTTON_CLICK:
		if (uParam == (unsigned int)(KWndWindow*)&m_CloseBtn)
		{
			KUiShop::CancelTrade();
			Hide();
		}
		else if (uParam == (unsigned int)(KWndWindow*)&m_OpenStatusPadBtn)
			KShortcutKeyCentre::ExcuteScript(SCK_SHORTCUT_STATUS);
		else if (uParam == (unsigned int)(KWndWindow*)&m_GetMoneyBtn)
		{
			if (KUiStoreBox::GetIfVisible())
				KUiGetMoney::OpenWindow(0, m_nMoney, this, UIITEM_WAIT_GETMONEY, &m_Money);
		}
		break;
	case WND_M_OTHER_WORK_RESULT:
		if (uParam == UIITEM_WAIT_GETMONEY)
			OnGetMoney(nParam);
		break;
	default:
		return KWndShowAnimate::WndProc(uMsg, uParam, nParam);
	}
	return 0;
}

void KUiItem::OnGetMoney(int nMoney)
{	
	if (nMoney > 0 && KUiStoreBox::GetIfVisible())
	{
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX,
			true, nMoney);
	}
}

void KUiItem::OnItemPickDrop(ITEM_PICKDROP_PLACE* pPickPos, ITEM_PICKDROP_PLACE* pDropPos)
{
	if (!g_UiBase.IsOperationEnable(UIS_O_MOVE_ITEM) && 
		!g_UiBase.IsOperationEnable(UIS_O_TRADE_ITEM))
		return;
	KUiObjAtContRegion	Pick, Drop;
	KUiDraggedObject	Obj;

	UISYS_STATUS eStatus = g_UiBase.GetStatus();
	if (pPickPos)
	{
		_ASSERT(pPickPos->pWnd);		
		((KWndObjectMatrix*)(pPickPos->pWnd))->GetObject(
			Obj, pPickPos->h, pPickPos->v);
		Pick.Obj.uGenre = Obj.uGenre;
		Pick.Obj.uId = Obj.uId;
		Pick.Region.Width = Obj.DataW;
		Pick.Region.Height = Obj.DataH;
		Pick.Region.h = Obj.DataX;
		Pick.Region.v = Obj.DataY;
		Pick.eContainer = UOC_ITEM_TAKE_WITH;

		if (eStatus == UIS_S_TRADE_SALE)
		{
			g_pCoreShell->OperationRequest(GOI_TRADE_NPC_SELL,
				(unsigned int)(&Pick), 0);
			return;
		}
		else if (eStatus == UIS_S_TRADE_REPAIR)
		{
			g_pCoreShell->OperationRequest(GOI_TRADE_NPC_REPAIR,
				(unsigned int)(&Pick), 0);
			return;
		}
		else if (eStatus == UIS_S_TRADE_BUY)
			return;
	}

	if (pDropPos)
	{
		Wnd_GetDragObj(&Obj);
		Drop.Obj.uGenre = Obj.uGenre;
		Drop.Obj.uId = Obj.uId;
		Drop.Region.Width = Obj.DataW;
		Drop.Region.Height = Obj.DataH;
		Drop.Region.h = pDropPos->h;
		Drop.Region.v = pDropPos->v;
		Drop.eContainer = UOC_ITEM_TAKE_WITH;	
	}
	
	g_pCoreShell->OperationRequest(GOI_SWITCH_OBJECT,
		pPickPos ? (unsigned int)&Pick : 0,
		pDropPos ? (int)&Drop : 0);
}

