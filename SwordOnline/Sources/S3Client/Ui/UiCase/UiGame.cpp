/*****************************************************************************************
//	ß[‘ò´°¿ÚµÄ½Ó¿Ú
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-18
------------------------------------------------------------------------------------------
	ÏòÎÞÊý¿¹ÈÕÏÈÁÒ¡£ÖÂ¾´£¡³«Òé9.18È«¹úÃù¾¯±¨¡£
*****************************************************************************************/
#include "KWin32.h"

#include "Windows.h"
#include "../Elem/WndMessage.h"
#include "../../../core/Src/CoreShell.h"
#include "../../../core/Src/GameDataDef.h"
#include "UiGame.h"
#include "memory.h"

#include "UiPlayerBar.h"
#include "UiChatCentre.h"
#include "UiFaceSelector.h"
#include "UiTongManager.h"
#include "UiMiniMap.h"
#include "../UiChatPhrase.h"
#include "../Elem/PopupMenu.h"
#include "../../../Engine/src/Text.h"

#include "../../../core/Src/KPlayerMenuState.h"

#include "../Elem/Wnds.h"

#include "../ShortcutKey.h"
#include "CommCtrl.h"
#include "../../../Represent/iRepresent/iRepresentShell.h"
#include "../../../Engine/src/KDebug.h"
#include "../../S3Client.h"

extern iRepresentShell*	g_pRepresentShell;
extern iCoreShell*		g_pCoreShell;
extern KUiChatPhrase    g_UiChatPhrase;

KUiGameSpace	g_WndGameSpace;

#define SEL_ACTION_MENU 1
#define SEL_EMOTE_MENU 2

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÊäÈë´¦ÀíÏûÏ¢ÒÔ¼°Ò»Ð©ÌØ¶¨´°¿ÚÏûÏ¢µÄÏìÓ¦
//--------------------------------------------------------------------------
int KUiGameSpace::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	bool bDefault = true;
	int nKind = -1;
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			if(uMsg == WM_LBUTTONDOWN)
			{
				Wnd_LButtonDown();
				KUiMiniMap::Unflagging();
				g_pCoreShell->SceneMapOperation(GSMOI_SCENE_MAP_REMOVE_FLAG, 0, 0);
			}
			else if(uMsg == WM_RBUTTONDOWN)
			{
				int x, y, w, h;
				Wnd_GetCursorPos(&x, &y);
				Wnd_GetScreenSize(w, h);
				unsigned int uScrSize[2];
				int nPos[2];
				uScrSize[0] = w;
				uScrSize[1] = h;
				nPos[0] = x;
				nPos[1] = y;
				g_pCoreShell->SceneMapOperation(GSMOI_CLICK_2_SPACECOORD, (unsigned int)&uScrSize, (int)&nPos);
			}
			int nModifier = 0;
			if (uParam & MK_CONTROL)
				nModifier |= HOTKEYF_CONTROL;

			if (uParam & MK_SHIFT)
				nModifier |= HOTKEYF_SHIFT;

			if (GetKeyState(VK_MENU) & 0x8000)
				nModifier |= HOTKEYF_ALT;

			if (KShortcutKeyCentre::HandleMouseInput(uMsg == WM_LBUTTONDOWN ? VK_LBUTTON : VK_RBUTTON, nModifier, LOWORD(nParam), HIWORD(nParam)))
				bDefault = false;
		}
		break;
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		{
			int nModifier = 0;
			if (uParam & MK_CONTROL)
				nModifier |= HOTKEYF_CONTROL;

			if (uParam & MK_SHIFT)
				nModifier |= HOTKEYF_SHIFT;

			if (GetKeyState(VK_MENU) & 0x8000)
				nModifier |= HOTKEYF_ALT;

			if (KShortcutKeyCentre::HandleMouseInput(uMsg == WM_LBUTTONDBLCLK ? VK_LDBUTTON : VK_RDBUTTON, nModifier, LOWORD(nParam), HIWORD(nParam)))
				bDefault = false;
		}
		break;
	case WND_M_MENUITEM_SELECTED:
		if (uParam == (unsigned int)(KWndWindow*)this)
		{
			if (HIWORD(nParam) == SEL_ACTION_MENU && short(LOWORD(nParam)) >= 0)
			{
				ProcessPeople(&m_SelectPlayer, LOWORD(nParam));
				bDefault = false;
			}
			if (HIWORD(nParam) == SEL_EMOTE_MENU && short(LOWORD(nParam)) >= 0)
			{
				ProcessEmote(m_SelectPlayer.Name, "", LOWORD(nParam));
				bDefault = false;
			}
		}
		break;
	case WM_MOUSEMOVE:
	case WM_MOUSEHOVER:
		if (uParam & MK_LBUTTON)
		{
			if (g_pCoreShell)
				g_pCoreShell->GotoWhere(LOWORD(nParam), HIWORD(nParam), 0);
		}
		else if (uParam & MK_RBUTTON)
		{
			KShortcutKeyCentre::HandleMouseInput(0, VK_RBUTTON, LOWORD(nParam), HIWORD(nParam));
		}

		OnMouseMoveCursor(LOWORD(nParam), HIWORD(nParam));
		bDefault = false;
		break;
	case WM_KEYDOWN:
		if (g_pCoreShell)
		{
			if(uParam == 'F')
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0)) //AutoPK on-off
				{
					if (GetKeyState(VK_CONTROL) & 0x8000)
					{
						SharedState s;
						s.CmdID = PRG_ONOFFPK;
						s.Size = sizeof(SharedState);
						SendInfoToTool(&s, sizeof(SharedState));
					}
				}
			}
			else if(uParam == 'A')
			{
				if(g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, 0, 0)) //Auto tick on-off
				{
					if (GetKeyState(VK_CONTROL) & 0x8000)
					{
						SharedState s;
						s.CmdID = PRG_AUTOONOFF;
						s.Size = sizeof(SharedState);
						SendInfoToTool(&s, sizeof(SharedState));
					}
				}
			}
			else if(uParam == 'G')
			{
				g_pCoreShell->OperationRequest(GOI_EXEPROGRAM_TASK, 1, 0);
			}
			else if(uParam == 'L')
			{
				g_pCoreShell->OperationRequest(GOI_EXEPROGRAM_TASK, 1, TRUE);
			}
		}
		break;
	default:	
		break;
	}

	if (bDefault && g_pCoreShell)
		g_pCoreShell->ProcessInput(uMsg, uParam, nParam);

	return 0;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º»æÖÆÓÎÏ·ÊÀ½ç
//--------------------------------------------------------------------------
void KUiGameSpace::PaintWindow()
{
	g_pCoreShell->DrawGameSpace();
}

void KUiGameSpace::OnMouseMoveCursor(int x, int y)
{
	if (g_pCoreShell == NULL)
		return;

	KUiPlayerItem SelectPlayer;
	int nKind = -1;
	int nObjectIdx = -1;
	if (g_pCoreShell->FindSelectNPC(x, y, relation_all, true, &SelectPlayer, nKind))
	{
		if (nKind == kind_dialoger)
			Wnd_SwitchCursor(MOUSE_CURSOR_DIALOG);
		else if (g_pCoreShell->GetNPCRelation(SelectPlayer.nIndex) == relation_enemy)
			Wnd_SwitchCursor(MOUSE_CURSOR_FIGHT);
		else
			Wnd_SwitchCursor(MOUSE_CURSOR_NORMAL);
	}
	else if (g_pCoreShell->FindSelectObject(x, y, true, nObjectIdx, nKind))
	{
		if (nKind == Obj_Kind_MapObj)
			Wnd_SwitchCursor(MOUSE_CURSOR_DIALOG);
		else if (nKind == Obj_Kind_Item || nKind == Obj_Kind_Money)
			Wnd_SwitchCursor(MOUSE_CURSOR_PICK);
		else if (nKind == Obj_Kind_Prop)
			Wnd_SwitchCursor(MOUSE_CURSOR_USE);
		else if (nKind == Obj_Kind_Box)
			Wnd_SwitchCursor(CURSOR_POINT_TO_OBJ_NPC);
	}
	else
		Wnd_SwitchCursor(MOUSE_CURSOR_NORMAL);
}

void AddBlackList(const char* strName, const char* strGroup);

void ProcessPeople(KUiPlayerItem* pDest, int nAction)
{
	if (pDest == NULL || pDest->Name[0] == 0)
		return;
	switch(nAction)
	{
	case ACTION_JOINTEAM:	//ÒªÓÃnIndex
		if (g_pCoreShell && pDest->nData == PLAYER_MENU_STATE_TEAMOPEN && pDest->nIndex != -1)
			g_pCoreShell->ApplyAddTeam(pDest);
		break;
	case ACTION_TRADE:	//ÒªÓÃnIndex
		if (g_pCoreShell && pDest->nData == PLAYER_MENU_STATE_TRADEOPEN && pDest->nIndex != -1)
			g_pCoreShell->TradeApplyStart(pDest);
		break;
	case ACTION_CHAT:		//ÒªÓÃName
		KUiPlayerBar::InputNameMsg(false, pDest->Name, true);
		break;
	case ACTION_REVENGE:			//ÒªÓÃuId
		if (g_pCoreShell && pDest->uId != 0)
			g_pCoreShell->OperationRequest(GOI_REVENGE_SOMEONE, (unsigned int)pDest, 0);
		break;
	case ACTION_MAKEFRIEND:	//ÒªÓÃName
		if (g_pCoreShell && !KUiChatCentre::IsMyFriend(pDest->Name))
			g_pCoreShell->OperationRequest(GOI_CHAT_FRIEND_ADD,	(unsigned int)pDest, 0);
		break;
	case ACTION_INVITETEAM:			//ÒªÓÃuId
		if (g_pCoreShell && pDest->uId != 0)
		{
			KUiPlayerTeam	TeamInfo;
			TeamInfo.cNumMember = 0;
			g_pCoreShell->TeamOperation(TEAM_OI_GD_INFO, (unsigned int)&TeamInfo, 0);
			if (TeamInfo.cNumMember == 0)
				g_pCoreShell->TeamOperation(TEAM_OI_CREATE, 0, 0);
			g_pCoreShell->TeamOperation(TEAM_OI_INVITE, (unsigned int)pDest, 0);
		}
		break;
	case ACTION_FOLLOW:	//ÒªÓÃnIndex
		if (g_pCoreShell && pDest->nIndex != -1)
			g_pCoreShell->OperationRequest(GOI_FOLLOW_SOMEONE, (unsigned int)pDest, 0);
		break;
	case ACTION_VIEWITEM:			//ÒªÓÃuId
		if (g_pCoreShell && pDest->uId != 0)
			g_pCoreShell->OperationRequest(GOI_VIEW_PLAYERITEM, (unsigned int)pDest->uId, 0);
		break;
	case ACTION_BLACKLIST:	//ÒªÓÃName
		AddBlackList(pDest->Name, "");
		break;
	case ACTION_TONG:
		KUiTongManager::OpenWindow(pDest->Name);
		break;
	case ACTION_EXENPC:
		g_pCoreShell->OperationRequest(GOI_EXEPROGRAM_TASK, 0, (int)pDest->uId);
		break;
	}
}

void ProcessEmote(char* szDest, char *szDestChannel, int nEmoteIndex)
{
	if (g_pCoreShell && szDest)
	{
		KUiPlayerBaseInfo MyInfo;
		char szBuff[256];
		int nStringLen, nChannelIndex, nDestChannel;

		nDestChannel = KUiMsgCentrePad::GetChannelIndex(szDestChannel);
		if(nDestChannel == -1)
		{
			nChannelIndex = KUiPlayerBar::GetCurChannel();
		}
		else
		{
			nChannelIndex = nDestChannel;
		}
		if(nChannelIndex != -1)
		{
			int nChannelCount = KUiMsgCentrePad::GetChannelCount();
			int nChannelID    = KUiMsgCentrePad::GetChannelID(nChannelIndex);

			//¿ªÊ¼»ñÈ¡ºÍ×ª»»×Ö´®
			g_pCoreShell->GetGameData(GDI_PLAYER_BASE_INFO, (unsigned int)&MyInfo, 0);
       		if(szDest[0])
	            nStringLen = g_UiChatPhrase.GetEmote(nEmoteIndex, szBuff, sizeof(szBuff));
		    else
			    nStringLen = g_UiChatPhrase.GetEmote(nEmoteIndex, szBuff, sizeof(szBuff), 1);
			//»ñÈ¡ºÍ×ª»»×Ö´®ÍæÁË

			//·¢³öÈ¥
			if(KUiPlayerBar::IsCanSendMessage(szBuff, nStringLen, szDest, nChannelID))
			{
				char szBuffConverted[256];
				nStringLen = KUiFaceSelector::ConvertFaceText(szBuffConverted, szBuff, nStringLen);
				nStringLen = g_UiChatPhrase.ConvertEmoteString(szBuffConverted, nStringLen, MyInfo.Name, szDest);
    			if(nChannelIndex >= nChannelCount)
			    {
					if (szDest[0])
					{
    				    KUiPlayerBar::OnSendSomeoneMessage(szDest, szBuffConverted, nStringLen);
						int nIndex = KUiPlayerBar::AddRecentPlayer(szDest);
						KUiPlayerBar::SetCurrentChannel(nIndex);
					}
			    }
				else
				{
					KUiPlayerBar::OnSendChannelMessage(nChannelID, szBuffConverted, nStringLen);
				}
				///////////////////////////////////////////
			}
			/////////////////////////////
		}
		////////////////////
	}
	/////////////////
}

char g_ActionName[][32] = 
{
	"T¸n gÉu",
	"B¹n h÷u",
	"Giao dÞch",
	"Vµo nhãm",
	"Mêi nhãm",
	"Theo sau",
	"Cõu s¸t",
	"Tin tøc",
	"Sæ ®en",
	"Bang héi",
	"Ch¹y lÖnh",
};

bool IsInBlackName(char* strName);
#define defADMINMODE

void PopUpContextPeopleMenu(const KUiPlayerItem& SelectPlayer, int x, int y, int nKind)
{
	if (SelectPlayer.Name[0] == 0)
		return;
	if(nKind == kind_player)
	{
		if (KUiPlayerBar::IsSelfName((char*)SelectPlayer.Name))
			return;
	}
	else
	{
#ifndef defADMINMODE
		return;
#endif
	}
	int nbIsRecruit = g_pCoreShell->TongOperation(GTOI_TONG_IS_RECRUIT, (unsigned int)&SelectPlayer, 0);
	int nActionDataCount = sizeof(g_ActionName) / 32;

	KPopupMenuData* pMenuData = (KPopupMenuData*)malloc(MENU_DATA_SIZE(nActionDataCount));
	if (pMenuData == NULL)
		return;

	KPopupMenu::InitMenuData(pMenuData, nActionDataCount);
	pMenuData->usMenuFlag |= PM_F_AUTO_DEL_WHEN_HIDE;
	pMenuData->usMenuFlag &= ~PM_F_HAVE_HEAD_TAIL_IMG;

	KUiPlayerTeam TeamInfo;
	TeamInfo.nCaptainPower = 0;
	g_pCoreShell->TeamOperation(TEAM_OI_GD_INFO, (unsigned int)&TeamInfo, 0);
	int nMenuCount = 0;
	int i;
	pMenuData->nItemHeight = 0;
	BOOL bGM = g_pCoreShell->GetGameData(GDI_IS_PLAYER_GM, 0, 0);
	if(nKind == kind_dialoger || nKind == kind_normal)
	{
		if(bGM)
		{
			strcpy(pMenuData->Items[nMenuCount].szData, "Ch¹y lÖnh NPC");
			pMenuData->Items[nMenuCount].uDataLen = strlen(pMenuData->Items[nMenuCount].szData);
			pMenuData->Items[nMenuCount].uID = ACTION_EXENPC;
			nMenuCount++;
		}
		else
			return;
	}
	else
	{
		for (i = 0; i < nActionDataCount; i++)
		{
			if ((i == ACTION_JOINTEAM && SelectPlayer.nIndex != -1 && SelectPlayer.nData == PLAYER_MENU_STATE_TEAMOPEN) ||	//"ÉêÇëÈë¶Ó", ¶Ô·½Î´´ò¿ª¶ÓÎéÊ±²»ÄÜ¼ÓÈë
				(i == ACTION_TRADE && SelectPlayer.nIndex != -1 && SelectPlayer.nData == PLAYER_MENU_STATE_TRADEOPEN) ||	//"½»Ò×ÎïÆ·", ¶Ô·½Î´´ò¿ª½»Ò×Ê±²»ÄÜ¼ÓÈë
				(i == ACTION_MAKEFRIEND && !KUiChatCentre::IsMyFriend((char*)SelectPlayer.Name)) || //"¼ÓÎªºÃÓÑ", ¶Ô·½ÒÑÊÇÎÒµÄºÃÓÑÊ±²»ÄÜÔÙÑûÇë
				(i == ACTION_INVITETEAM && SelectPlayer.uId != 0 && TeamInfo.nCaptainPower > 0)	||	//"ÑûÇë¼ÓÈë", ¶ÓÎé²»¿ÉÒÔ¼ÓÈËÁË¾Í²»ÄÜÑûÇë¼ÓÈë
				(i == ACTION_FOLLOW && SelectPlayer.nIndex != -1) ||
				(i == ACTION_REVENGE && SelectPlayer.uId != 0) ||
				(i == ACTION_VIEWITEM && SelectPlayer.uId != 0) ||
				(i == ACTION_TONG && SelectPlayer.nIndex != -1 && nbIsRecruit) ||
				(i == ACTION_BLACKLIST && !IsInBlackName((char*)SelectPlayer.Name)) ||
				(i == ACTION_CHAT) ||
				(i == ACTION_EXENPC && bGM)
				)
			{
				strcpy(pMenuData->Items[nMenuCount].szData, g_ActionName[i]);
				pMenuData->Items[nMenuCount].uDataLen = strlen(g_ActionName[i]);
				pMenuData->Items[nMenuCount].uID = i;
				nMenuCount++;
			}
		}
	}
	pMenuData->nNumItem = nMenuCount;

	pMenuData->nX = x;
	pMenuData->nY = y;
	g_WndGameSpace.m_SelectPlayer = SelectPlayer;
	KPopupMenu::Popup(pMenuData, &g_WndGameSpace, SEL_ACTION_MENU);
}

void PopUpContextEmoteMenu(char* szDest, int x, int y)
{
	if (szDest == NULL || szDest[0] == 0)
		return;

	if (KUiPlayerBar::IsSelfName(szDest))
		return;

	int nEmoteCount = g_UiChatPhrase.GetEmoteCount((KUiChatPhrase::EMOTE_TYPE)(1));

	KPopupMenuData* pMenuData = (KPopupMenuData*)malloc(MENU_DATA_SIZE(nEmoteCount));
	if (pMenuData == NULL)
	{
		return;
	}

	KPopupMenu::InitMenuData(pMenuData, nEmoteCount);
	pMenuData->usMenuFlag |= PM_F_AUTO_DEL_WHEN_HIDE;
	pMenuData->usMenuFlag &= ~PM_F_HAVE_HEAD_TAIL_IMG;

	int i, nBuffLen;
	char szBuff[128];
	pMenuData->nItemHeight = 0;
	for (i = 0; i < nEmoteCount; i++)
	{
		nBuffLen = g_UiChatPhrase.GetMenuEmoteName(i, szBuff, sizeof(szBuff));
		memcpy(pMenuData->Items[i].szData, szBuff, nBuffLen);
		pMenuData->Items[i].uDataLen = nBuffLen;
		pMenuData->Items[i].uID = i;
	}
	pMenuData->nNumItem = nEmoteCount;

	pMenuData->nX = x;
	pMenuData->nY = y;
	strncpy(g_WndGameSpace.m_SelectPlayer.Name, szDest, 32);
	g_WndGameSpace.m_SelectPlayer.nIndex = -1;
	g_WndGameSpace.m_SelectPlayer.uId = 0;
	g_WndGameSpace.m_SelectPlayer.Name[31] = 0;
	KPopupMenu::Popup(pMenuData, &g_WndGameSpace, SEL_EMOTE_MENU);
}