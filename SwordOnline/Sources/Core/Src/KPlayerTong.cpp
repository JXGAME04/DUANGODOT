//---------------------------------------------------------------------------
// Sword3 Engine (c) 2003 by Kingsoft
//
// File:	KPlayerTong.cpp
// Date:	2003.08.12
// Code:	±ß³ÇÀË×Ó
// Desc:	KPlayerTong Class
//---------------------------------------------------------------------------

#include	"KCore.h"
#include	"KNpc.h"
#include	"KPlayer.h"
#include	"KPlayerSet.h"
#include	"KPlayerTong.h"
#ifndef _SERVER
#include	"CoreShell.h"
#endif

//#define		defTONG_NAME_LENGTH			8
#define		defFuncShowNormalMsg(str)		\
	{										\
		KSystemMessage	sMsg;				\
		sMsg.eType = SMT_NORMAL;			\
		sMsg.byConfirmType = SMCT_NONE;		\
		sMsg.byPriority = 0;				\
		sMsg.byParamSize = 0;				\
		sprintf(sMsg.szMessage, str);		\
		CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);\
	}

#ifdef _SERVER
//nhung map khong set doi~ mau` currentcamp
static int g_TExceptMapID[] = 
{
	378,
	379,
	380,	//tong kim
	337,
	338,
	339,	//thuyen pld
	464,	//vuot ai
	465,
	466,
	467,
	468,
	469,
	470,
	471,
	480,
	481,
	482,
	483,
	484,
	485,
	486,
	487,
	488,
	489,
	490,
	491,
	492,
	493,
	494,
	495,
};

static bool g_IsExceptMap(int nMapID)
{
	for(int i=0;i < sizeof(g_TExceptMapID)/sizeof(int);++i)
	{
		if(nMapID == g_TExceptMapID[i])
			return true;
	}
	return false;
}
#endif
//-------------------------------------------------------------------------
//	khoi tao
//-------------------------------------------------------------------------
void	KPlayerTong::Init(int nPlayerIdx)
{
	m_nPlayerIndex = nPlayerIdx;

	Clear();
}

//-------------------------------------------------------------------------
//	xoa
//-------------------------------------------------------------------------
void	KPlayerTong::Clear()
{
	m_nFlag				= 0;
	m_nFigure			= enumTONG_FIGURE_MEMBER;
	m_nCamp				= 0;
	m_dwTongNameID		= 0;
	m_nWeekOffer		= 0;
	m_szName[0]			= 0;
	m_szTitle[0]		= 0;
	m_szMasterName[0]	= 0;
	m_nApplyTo			= 0;
	m_nNextTimeInfo		= 0;
	m_nNextTimeMemPage	= 0;
	m_nWeekGoalType		= 0;
	m_uRight			= 0;
}

#ifndef _SERVER
//-------------------------------------------------------------------------
//	gui lenh tao bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyCreateTong(int nCamp, char *lpszTongName)
{
	defFuncShowNormalMsg(MSG_TONG_APPLY_CREATE);
	if (!lpszTongName || !lpszTongName[0] || strlen(lpszTongName) > defTONG_NAME_MAX_LENGTH)
	{
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR01);
		return FALSE;
	}
	if(!(((BYTE)lpszTongName[0] >= 65 && (BYTE)lpszTongName[0] <= 90)
	|| ((BYTE)lpszTongName[0] >= 97 && (BYTE)lpszTongName[0] <= 122)))
	{//ky' tu dau tien chi~ co the la chu~
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR01);
		return FALSE;
	}
	for(int i=0;i<(int)strlen(lpszTongName);++i)
	{
		if((BYTE)lpszTongName[i] < 32
		|| (BYTE)lpszTongName[i] == '%'
		|| (BYTE)lpszTongName[i] == '"'
		|| (BYTE)lpszTongName[i] == '\\'
		|| (BYTE)lpszTongName[i] == '/')
		{
			defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR01);
			return FALSE;
		}
	}
	if (nCamp != camp_justice && nCamp != camp_evil && nCamp != camp_balance)
	{
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR02);
		return FALSE;
	}
	if (m_nFlag)
	{
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR03);
		return FALSE;
	}
	if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp != camp_free
	|| Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentCamp != camp_free)
	{
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR04);
		return FALSE;
	}
	if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Level < PlayerSet.m_sTongParam.m_nLevel)
	{
		char	szBuf[80];
		sprintf(szBuf, MSG_TONG_CREATE_ERROR05, PlayerSet.m_sTongParam.m_nLevel);
		defFuncShowNormalMsg(szBuf);
		return FALSE;
	}
	if ((int)Player[CLIENT_PLAYER_INDEX].m_dwLeadLevel < PlayerSet.m_sTongParam.m_nLeadLevel)
	{
		char	szBuf[80];
		sprintf(szBuf, MSG_TONG_CREATE_ERROR06, PlayerSet.m_sTongParam.m_nLeadLevel);
		defFuncShowNormalMsg(szBuf);
		return FALSE;
	}
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
	{
		defFuncShowNormalMsg(MSG_TONG_CREATE_ERROR08);
		return FALSE;
	}
	if (Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipmentMoney() < PlayerSet.m_sTongParam.m_nMoney)
	{
		char	szBuf[80];
		sprintf(szBuf, MSG_TONG_CREATE_ERROR07, PlayerSet.m_sTongParam.m_nMoney);
		defFuncShowNormalMsg(szBuf);
		return FALSE;
	}

	TONG_APPLY_CREATE_COMMAND	sApply;
	sApply.ProtocolType = c2s_extendtong;
	sApply.m_wLength = sizeof(TONG_APPLY_CREATE_COMMAND) - 1;
	sApply.m_btMsgId = enumTONG_COMMAND_ID_APPLY_CREATE;
	sApply.m_btCamp = (BYTE)nCamp;
	strcpy(sApply.m_szName, lpszTongName);

	if (g_pClient)
		g_pClient->SendPackToServer(&sApply, sApply.m_wLength + 1);

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	client gui lenh xin gia nhap bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyAddTong(UINT uDestID)
{
	defFuncShowNormalMsg(MSG_TONG_APPLY_ADD);
	if (m_nFlag)
	{
		defFuncShowNormalMsg(MSG_TONG_APPLY_ADD_ERROR1);
		return FALSE;
	}
	if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentCamp != camp_free)
	{
		defFuncShowNormalMsg(MSG_TONG_APPLY_ADD_ERROR2);
		return FALSE;
	}
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
	{
		defFuncShowNormalMsg(MSG_TONG_APPLY_ADD_ERROR3);
		return FALSE;
	}

	TONG_APPLY_ADD_COMMAND	sApply;
	sApply.ProtocolType = c2s_extendtong;
	sApply.m_wLength = sizeof(TONG_APPLY_ADD_COMMAND) - 1;
	sApply.m_btMsgId = enumTONG_COMMAND_ID_APPLY_ADD;
	sApply.m_uDestID = uDestID;

	if (g_pClient)
		g_pClient->SendPackToServer(&sApply, sizeof(TONG_APPLY_ADD_COMMAND));
	
	return TRUE;
}

BOOL KPlayerTong::ApplyRight(UINT uDestID, UINT uRight)
{
	if(!m_nFlag)
		return FALSE;
	
	TONG_APPLY_RIGHT_COMMAND	sApply;
	sApply.ProtocolType = c2s_extendtong;
	sApply.m_wLength = sizeof(TONG_APPLY_RIGHT_COMMAND) - 1;
	sApply.m_btMsgId = enumTONG_COMMAND_ID_APPLY_RIGHT;
	sApply.m_uDestID = uDestID;
	sApply.m_uRight = uRight;
	if (g_pClient)
		g_pClient->SendPackToServer(&sApply, sizeof(TONG_APPLY_RIGHT_COMMAND));
	
	return TRUE;
}

#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	dieu kien lap bang
//-------------------------------------------------------------------------
int		KPlayerTong::CheckCreateCondition(int nCamp, char *lpszTongName)
{
	if (Player[m_nPlayerIndex].m_nIndex <= 0)
		return FALSE;
	if (Player[m_nPlayerIndex].CheckTrading())
		return FALSE;
	if (!lpszTongName || !lpszTongName[0] || strlen(lpszTongName) > defTONG_NAME_MAX_LENGTH)
		return FALSE;
	if(!(((BYTE)lpszTongName[0] >= 65 && (BYTE)lpszTongName[0] <= 90)
	|| ((BYTE)lpszTongName[0] >= 97 && (BYTE)lpszTongName[0] <= 122)))
		return FALSE;
	for(int i=0;i<(int)strlen(lpszTongName);++i)
	{
		if((BYTE)lpszTongName[i] < 32
		|| (BYTE)lpszTongName[i] == '%'
		|| (BYTE)lpszTongName[i] == '"'
		|| (BYTE)lpszTongName[i] == '\\'
		|| (BYTE)lpszTongName[i] == '/')
			return FALSE;
	}
	if (nCamp != camp_justice && nCamp != camp_evil && nCamp != camp_balance)
		return FALSE;
	if (m_nFlag)
		return FALSE;
	if (Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp != camp_free ||
		Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp != camp_free)
		return FALSE;
	if (Npc[Player[m_nPlayerIndex].m_nIndex].m_Level < PlayerSet.m_sTongParam.m_nLevel || 
		(int)Player[m_nPlayerIndex].m_dwLeadLevel < PlayerSet.m_sTongParam.m_nLeadLevel)
		return FALSE;
	if (Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney() < PlayerSet.m_sTongParam.m_nMoney)
		return FALSE;
	if (Player[m_nPlayerIndex].m_ItemList.GetTaskItemNum(4,195,0) <= 0)
	{
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, "CÇn mang theo Nh¹c V­¬ng KiÕm ®Ó lËp bang héi");
		return FALSE;
	}
	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	ket qua tao bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::Create(int nCamp, char *lpszTongName)
{
	m_nFlag			= 1;
	m_nFigure		= enumTONG_FIGURE_MASTER;
	m_nCamp			= nCamp;
	strcpy(m_szName, lpszTongName);
	strcpy(m_szMasterName, Npc[Player[m_nPlayerIndex].m_nIndex].Name);
	m_dwTongNameID	= g_FileName2Id(m_szName);
	strcpy(m_szTitle, defTITLE_MASTER);
	m_uRight		= defRIGHT_FULL;
	
	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = m_nCamp;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = m_nCamp;
	Player[m_nPlayerIndex].m_ItemList.CostMoney(PlayerSet.m_sTongParam.m_nMoney);
	Player[m_nPlayerIndex].m_ItemList.RemoveTaskItem(4,195,0);
	// gui den client
	TONG_CREATE_SYNC	sCreate;
	sCreate.ProtocolType = s2c_tongcreate;
	sCreate.m_btCamp = nCamp;
	if (strlen(lpszTongName) < sizeof(sCreate.m_szName))
		strcpy(sCreate.m_szName, lpszTongName);
	else
	{
		memcpy(sCreate.m_szName, lpszTongName, sizeof(sCreate.m_szName) - 1);
		sCreate.m_szName[sizeof(sCreate.m_szName) - 1] = 0;
	}
	if (g_pServer)
		g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sCreate, sizeof(TONG_CREATE_SYNC));
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£ºµÃµ½·þÎñÆ÷Í¨Öª´´½¨°ï»á
//-------------------------------------------------------------------------
void	KPlayerTong::Create(TONG_CREATE_SYNC *psCreate)
{
	if (!psCreate)
		return;

	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp = psCreate->m_btCamp;
	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentCamp = psCreate->m_btCamp;

	m_nFlag			= 1;
	m_nFigure		= enumTONG_FIGURE_MASTER;
	m_nCamp			= psCreate->m_btCamp;
	m_uRight		= defRIGHT_FULL;
	m_szTitle[0]	= 0;
	memset(m_szName, 0, sizeof(m_szName));
	memcpy(m_szName, psCreate->m_szName, sizeof(psCreate->m_szName));
	strcpy(m_szMasterName, Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].Name);
	m_dwTongNameID	= g_FileName2Id(m_szName);

	// Í¨Öª½çÃæ°ï»á½¨Á¢³É¹¦
	defFuncShowNormalMsg(MSG_TONG_CREATE_SUCCESS);

	// Í¨Öª°ï»áÆµµÀ
	CoreDataChanged(GDCNI_PLAYER_BASE_INFO, 0, 0);

}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£ºÍ·ÉÏÊÇ·ñÐèÒª¶¥ÕÒÈË±êÖ¾
//-------------------------------------------------------------------------
BOOL	KPlayerTong::GetOpenFlag()
{
	return (m_nFlag && m_nFigure != enumTONG_FIGURE_MEMBER);
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	chuyen msg qua cho uDestID de xin gia nhap
//-------------------------------------------------------------------------
BOOL	KPlayerTong::TransferAddApply(UINT uDestID)
{
	if (m_nFlag)
		return FALSE;
	if (Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp != camp_free ||
		Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp != camp_free)
		return FALSE;
	if (Player[m_nPlayerIndex].CheckTrading())
		return FALSE;

	int	nTarget = PlayerSet.FindSame(uDestID);
	if (nTarget <= 0)
		return FALSE;
	if (!Player[nTarget].m_cTong.CheckAcceptAddApplyCondition())
		return FALSE;
	m_nApplyTo = nTarget;

	TONG_APPLY_ADD_SYNC	sAdd;
	sAdd.ProtocolType = s2c_extendtong;
	sAdd.m_btMsgId = enumTONG_SYNC_ID_TRANSFER_ADD_APPLY;
	sAdd.m_nPlayerIdx = m_nPlayerIndex;
	strcpy(sAdd.m_szName, Npc[Player[m_nPlayerIndex].m_nIndex].Name);
	sAdd.m_wLength = sizeof(TONG_APPLY_ADD_SYNC) - 1 - sizeof(sAdd.m_szName) + strlen(sAdd.m_szName);
	if (g_pServer)
		g_pServer->PackDataToClient(Player[nTarget].m_nNetConnectIdx, &sAdd, sAdd.m_wLength + 1);
	
	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£ºÅÐ¶ÏÊÇ·ñ¿ÉÒÔ×ª·¢±ðÈËµÄ¼ÓÈë°ï»áÉêÇë
//-------------------------------------------------------------------------
BOOL	KPlayerTong::CheckAcceptAddApplyCondition()
{
	if (!m_nFlag || m_nFigure == enumTONG_FIGURE_MEMBER)
		return FALSE;

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£ºÊÇ·ñ½ÓÊÜ³ÉÔ± bFlag == TRUE ½ÓÊÜ == FALSE ²»½ÓÊÜ
//-------------------------------------------------------------------------
void	KPlayerTong::AcceptMember(int nPlayerIdx, UINT dwNameID, BOOL bFlag)
{
	if (nPlayerIdx <= 0)
		return;

	TONG_ACCEPT_MEMBER_COMMAND	sAccept;
	sAccept.ProtocolType	= c2s_extendtong;
	sAccept.m_wLength		= sizeof(TONG_ACCEPT_MEMBER_COMMAND) - 1;
	sAccept.m_btMsgId		= enumTONG_COMMAND_ID_ACCEPT_ADD;
	sAccept.m_nPlayerIdx	= nPlayerIdx;
	sAccept.m_dwNameID		= dwNameID;
	sAccept.m_btFlag		= (bFlag != 0);

	if (g_pClient)
		g_pClient->SendPackToServer(&sAccept, sAccept.m_wLength + 1);
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	gui loi tu choi' gia nhap bang den muc tieu xin gia nhap
//-------------------------------------------------------------------------
void	KPlayerTong::SendRefuseMessage(int nPlayerIdx, UINT dwNameID)
{
	if (nPlayerIdx <= 0 || nPlayerIdx >= MAX_PLAYER)
		return;
	if (Player[nPlayerIdx].m_cTong.m_nApplyTo != m_nPlayerIndex ||
		Player[nPlayerIdx].m_nIndex <= 0 ||
		Player[m_nPlayerIndex].m_nIndex <= 0)
		return;
	if (Player[nPlayerIdx].m_dwID != dwNameID)
		return;
	
	int nLength = strlen(Npc[Player[m_nPlayerIndex].m_nIndex].Name);
	SHOW_MSG_SYNC	sMsg;
	sMsg.ProtocolType = s2c_msgshow;
	sMsg.m_wMsgID = enumMSG_ID_TONG_REFUSE_ADD;
	sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID) + nLength;
	sMsg.m_lpBuf = new BYTE[sMsg.m_wLength + 1];

	memcpy(sMsg.m_lpBuf, &sMsg, sizeof(SHOW_MSG_SYNC) - sizeof(LPVOID));
	memcpy((char*)sMsg.m_lpBuf + sizeof(SHOW_MSG_SYNC) - sizeof(LPVOID), Npc[Player[m_nPlayerIndex].m_nIndex].Name, nLength);

	if (g_pServer)
		g_pServer->PackDataToClient(Player[nPlayerIdx].m_nNetConnectIdx, sMsg.m_lpBuf, sMsg.m_wLength + 1);
}
#endif

void	KPlayerTong::GetTongName(char *lpszGetName)
{
	if (!lpszGetName)
		return;
	if (!m_nFlag)
	{
		lpszGetName[0] = 0;
		return;
	}

	strcpy(lpszGetName, m_szName);
}

void	KPlayerTong::GetTitle(char *lpszGetName)
{
	if (!lpszGetName)
		return;
	if (!m_nFlag)
	{
		lpszGetName[0] = 0;
		return;
	}

	strcpy(lpszGetName, m_szTitle);
}

#ifdef _SERVER
//-------------------------------------------------------------------------
//	dieu kien gia nhap bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::CheckAddCondition(int nPlayerIdx)
{
	if (!m_nFlag || m_nFigure == enumTONG_FIGURE_MEMBER)
		return FALSE;
	if (Player[nPlayerIdx].m_cTong.m_nApplyTo != this->m_nPlayerIndex)
		return FALSE;
	if (Player[nPlayerIdx].m_cTong.m_nFlag)
		return FALSE;
	if (Npc[Player[nPlayerIdx].m_nIndex].m_CurrentCamp != camp_free ||
		Npc[Player[nPlayerIdx].m_nIndex].m_Camp != camp_free)
		return FALSE;
	if (Player[nPlayerIdx].CheckTrading())
		return FALSE;

	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	ket qua sau khi add member
//-------------------------------------------------------------------------
BOOL	KPlayerTong::AddTong(int nCamp, char *lpszTongName, char *lpszMasterName, int nFigure)
{
	if (m_nFlag || !lpszTongName || !lpszMasterName)
		return FALSE;
	m_nFlag		= 1;
	m_nFigure	= nFigure;
	if(m_nFigure < enumTONG_FIGURE_MEMBER || m_nFigure >= enumTONG_FIGURE_MASTER)
		m_nFigure = enumTONG_FIGURE_MEMBER;
	m_nCamp		= nCamp;
	strcpy(this->m_szName, lpszTongName);
	strcpy(this->m_szMasterName, lpszMasterName);
	
	if(m_nFigure == enumTONG_FIGURE_MEMBER)
		strcpy(m_szTitle, defTITLE_MEMBER);
	else if(m_nFigure == enumTONG_FIGURE_MANAGER)
		strcpy(m_szTitle, defTITLE_MANAGER);
	else //if(m_nFigure == enumTONG_FIGURE_DIRECTOR)
		strcpy(m_szTitle, defTITLE_DIRECT);
	
	m_dwTongNameID	= g_FileName2Id(m_szName);
	m_nWeekOffer		= 0;
	m_nWeekGoalType		= 0;
	m_uRight			= 0;
	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = m_nCamp;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = m_nCamp;

	TONG_Add_SYNC	sAdd;
	sAdd.ProtocolType = s2c_extendtong;
	sAdd.m_wLength = sizeof(sAdd) - 1;
	sAdd.m_btMsgId = enumTONG_SYNC_ID_ADD;
	sAdd.m_btCamp = this->m_nCamp;
	sAdd.m_btFigure = m_nFigure;
	strcpy(sAdd.m_szTongName, m_szName);
	strcpy(sAdd.m_szMaster, m_szMasterName);

	if (g_pServer)
		g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sAdd, sAdd.m_wLength + 1);
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	sync client add
//-------------------------------------------------------------------------
BOOL	KPlayerTong::AddTong(int nCamp, char *lpszTongName, char *lpszMaster, int nFigure)
{
	m_nFlag		= 1;
	m_nFigure	= nFigure;
	m_nCamp		= nCamp;
	strcpy(m_szName, lpszTongName);
	if(m_nFigure == enumTONG_FIGURE_MEMBER)
		strcpy(m_szTitle, defTITLE_MEMBER);
	else if(m_nFigure == enumTONG_FIGURE_MANAGER)
		strcpy(m_szTitle, defTITLE_MANAGER);
	else //if(m_nFigure == enumTONG_FIGURE_DIRECTOR)
		strcpy(m_szTitle, defTITLE_DIRECT);
	strcpy(m_szMasterName, lpszMaster);
	m_dwTongNameID	= g_FileName2Id(m_szName);
	m_nWeekOffer		= 0;
	m_nWeekGoalType		= 0;
	m_uRight			= 0;
	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp = m_nCamp;
	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentCamp = m_nCamp;

	defFuncShowNormalMsg(MSG_TONG_ADD_SUCCESS);
	CoreDataChanged(GDCNI_PLAYER_BASE_INFO, 0, 0);
	
	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	gui lenh bo nhiem
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyInstate(UINT uDestID, int nNewFigure)
{
	if (!m_nFlag)
		return FALSE;

	TONG_APPLY_INSTATE_COMMAND	sApply;
	sApply.ProtocolType = c2s_extendtong;
	sApply.m_btMsgId = enumTONG_COMMAND_ID_APPLY_INSTATE;
	sApply.m_uDestID = uDestID;
	sApply.m_btNewFigure = nNewFigure;
	sApply.m_wLength = sizeof(sApply) - 1;
	if (g_pClient)
		g_pClient->SendPackToServer(&sApply, sApply.m_wLength + 1);

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£ºÉêÇëÌßÈË
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyKick(UINT uDestID)
{
	if (!m_nFlag)
		return FALSE;

	TONG_APPLY_KICK_COMMAND	sKick;
	sKick.ProtocolType		= c2s_extendtong;
	sKick.m_wLength			= sizeof(sKick) - 1;
	sKick.m_btMsgId			= enumTONG_COMMAND_ID_APPLY_KICK;
	sKick.m_uDestID			= uDestID;

	if (g_pClient)
		g_pClient->SendPackToServer(&sKick, sKick.m_wLength + 1);

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	gui lenh chuyen vi
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyChangeMaster(UINT uDestID)
{
	if (!m_nFlag || m_nFigure != enumTONG_FIGURE_MASTER)
		return FALSE;

	TONG_APPLY_CHANGE_MASTER_COMMAND	sChange;
	sChange.ProtocolType	= c2s_extendtong;
	sChange.m_wLength		= sizeof(sChange) - 1;
	sChange.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_CHANGE_MASTER;
	sChange.m_uDestID		= uDestID;
	if (g_pClient)
		g_pClient->SendPackToServer(&sChange, sChange.m_wLength + 1);

	return TRUE;
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	lenh phan boi, roi bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyLeave()
{
	if (!m_nFlag)
		return FALSE;

	TONG_APPLY_LEAVE_COMMAND	sLeave;
	sLeave.ProtocolType		= c2s_extendtong;
	sLeave.m_wLength		= sizeof(sLeave) - 1;
	sLeave.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_LEAVE;
	if (g_pClient)
		g_pClient->SendPackToServer(&sLeave, sLeave.m_wLength + 1);

	return TRUE;
}
//dong mo~ tuyen dung
void	KPlayerTong::ApplyChangeRecruit()
{
	if (!m_nFlag)
		return;

	TONG_APPLY_LEAVE_COMMAND	sRecruit;
	sRecruit.ProtocolType		= c2s_extendtong;
	sRecruit.m_wLength		= sizeof(sRecruit) - 1;
	sRecruit.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_RECRUIT;
	if (g_pClient)
		g_pClient->SendPackToServer(&sRecruit, sRecruit.m_wLength + 1);
}
//gui tien ngan quy~
void	KPlayerTong::ApplyContribMoney(int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_CONTRIBMONEY;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyWithdrawMoney(int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_WITHDRAWMONEY;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyStoreOffer(int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_STOREOFFER;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyDispenseOffer(UINT uDestID, int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_DISPENSEOFFER_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_DISPENSEOFFER;
	sCmd.m_uDestID		= uDestID;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyAssignMoney(int nMemberPoint, int nManagerPoint, int nDirectorPoint)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_ASSIGNFUND_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_ASSIGNMONEY;
	sCmd.nMemberPoint	= nMemberPoint;
	sCmd.nManagerPoint	= nManagerPoint;
	sCmd.nDirectorPoint	= nDirectorPoint;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyAssignOffer(int nMemberPoint, int nManagerPoint, int nDirectorPoint)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_ASSIGNFUND_COMMAND	sCmd;
	sCmd.ProtocolType		= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_ASSIGNOFFER;
	sCmd.nMemberPoint	= nMemberPoint;
	sCmd.nManagerPoint	= nManagerPoint;
	sCmd.nDirectorPoint	= nDirectorPoint;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyTransMoney(int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_TRANSMONEY;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyStoreBuildFund(int nMoney)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_STOREBUILDFUND;
	sCmd.m_nMoney		= nMoney;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyChangeAnnounce(const char* pString)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_ANNOUNCE_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_ANNOUNCE;
	strcpy(sCmd.m_szAnnounce, pString);
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyChangeTitle(UINT uDestID, BYTE nDestFigure, const char* pString)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CHANGETITLE_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_CHANGETITLE;
	sCmd.m_uDestID		= uDestID;
	sCmd.m_nDestFigure	= nDestFigure;
	strcpy(sCmd.m_szName, pString);
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyChangeTitleAll(BYTE nSex, const char* pString)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_CHANGETITLE_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_CHANGETITLEALL;
	sCmd.m_uDestID		= 0;
	sCmd.m_nDestFigure	= nSex;
	strcpy(sCmd.m_szName, pString);
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyChangeCamp(BYTE btCamp)
{
	if (!m_nFlag)
		return;
	if(m_nCamp == (int)btCamp)
	{
		defFuncShowNormalMsg(MSG_TONG_CHANGECAMP_SAME);
		return;
	}
	TONG_APPLY_CHANGECAMP_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_CHANGECAMP;
	sCmd.m_btCamp		= btCamp;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

void	KPlayerTong::ApplyAction(int nAction)
{
	if (!m_nFlag)
		return;
	TONG_APPLY_ACTION_COMMAND	sCmd;
	sCmd.ProtocolType	= c2s_extendtong;
	sCmd.m_wLength		= sizeof(sCmd) - 1;
	sCmd.m_btMsgId		= enumTONG_COMMAND_ID_APPLY_ACTION;
	sCmd.m_nAction		= nAction;
	if (g_pClient)
		g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
}

#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	gui den server yeu cau thong tin
//-------------------------------------------------------------------------
BOOL	KPlayerTong::ApplyInfo(int nInfoID, UINT nParam1, int nParam2, int nParam3)
{
	if (nInfoID < 0 || nInfoID >= enumTONG_APPLY_INFO_ID_NUM)
		return FALSE;

	TONG_APPLY_INFO_COMMAND	sInfo;
	sInfo.ProtocolType = c2s_extendtong;
	sInfo.m_btMsgId = enumTONG_COMMAND_ID_APPLY_INFO;
	sInfo.m_btInfoID = nInfoID;
	sInfo.m_nParam1 = nParam1;
	sInfo.m_nParam2 = nParam2;
	sInfo.m_nParam3 = nParam3;
	sInfo.m_wLength = sizeof(sInfo) - 1;
	if (g_pClient)
		g_pClient->SendPackToServer(&sInfo, sInfo.m_wLength + 1);
	return TRUE;
}
#endif

UINT	KPlayerTong::GetTongNameID()
{
	return (m_nFlag ? m_dwTongNameID : 0);
}

#ifdef _SERVER
//-------------------------------------------------------------------------
//	gui thong tin co ban tu sv den client
//-------------------------------------------------------------------------
void	KPlayerTong::SendSelfInfo()
{
	TONG_SELF_INFO_SYNC	sInfo;
	sInfo.ProtocolType = s2c_extendtong;
	sInfo.m_wLength = sizeof(sInfo) - 1;
	sInfo.m_btMsgId = enumTONG_SYNC_ID_SELF_INFO;
	sInfo.m_btJoinFlag = this->m_nFlag;
	sInfo.m_btFigure = this->m_nFigure;
	sInfo.m_btCamp = this->m_nCamp;
	sInfo.m_nWeekOffer = this->m_nWeekOffer;
	sInfo.m_uRight = this->m_uRight;
	strcpy(sInfo.m_szMaster, this->m_szMasterName);
	strcpy(sInfo.m_szTitle, this->m_szTitle);
	strcpy(sInfo.m_szTongName, this->m_szName);
	if (g_pServer)
		g_pServer->PackDataToClient(Player[this->m_nPlayerIndex].m_nNetConnectIdx, &sInfo, sInfo.m_wLength + 1);
}
#endif

#ifndef _SERVER
//-------------------------------------------------------------------------
//	nhan thong tin co ban~ tu server
//-------------------------------------------------------------------------
void	KPlayerTong::SetSelfInfo(TONG_SELF_INFO_SYNC *pInfo)
{
	if (pInfo->m_btJoinFlag == 0)
	{
		if (m_nFlag)
		{
			CoreDataChanged(GDCNI_TONG_CLOSEUI, 0, 0);
			Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp = camp_free;
		}
		Clear();

		CoreDataChanged(GDCNI_PLAYER_BASE_INFO, 0, 0);

		return;
	}

	if (m_nFlag == 1 && m_nFigure != pInfo->m_btFigure)
	{
		defFuncShowNormalMsg("Chøc vô cña b¹n trong bang héi ®· thay ®æi");
	}

	this->m_nFlag = 1;
	this->m_nFigure = pInfo->m_btFigure;
	this->m_nCamp = pInfo->m_btCamp;
	this->m_nWeekOffer = pInfo->m_nWeekOffer;
	this->m_uRight = pInfo->m_uRight;
	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp = m_nCamp;
	memcpy(this->m_szMasterName, pInfo->m_szMaster, sizeof(pInfo->m_szMaster));
	memcpy(this->m_szName, pInfo->m_szTongName, sizeof(pInfo->m_szTongName));
	memcpy(this->m_szTitle, pInfo->m_szTitle, sizeof(pInfo->m_szTitle));
	m_dwTongNameID	= g_FileName2Id(m_szName);

	CoreDataChanged(GDCNI_PLAYER_BASE_INFO, 0, 0);
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	server check sender
//-------------------------------------------------------------------------
BOOL	KPlayerTong::CheckInstatePower(TONG_APPLY_INSTATE_COMMAND *pApply)
{
	if (!pApply)
		return FALSE;
	if (!m_nFlag || (m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR))
		return FALSE;
	if (!(m_uRight & defRIGHT_DEPOSE))
		return FALSE;
	if (pApply->m_btNewFigure != enumTONG_FIGURE_DIRECTOR && pApply->m_btNewFigure != enumTONG_FIGURE_MANAGER
		&& pApply->m_btNewFigure != enumTONG_FIGURE_MEMBER)
		return FALSE;
	if(m_nFigure == enumTONG_FIGURE_DIRECTOR && pApply->m_btNewFigure == enumTONG_FIGURE_DIRECTOR)
		return FALSE;
	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	co quyen truc xuat khong
//-------------------------------------------------------------------------
BOOL	KPlayerTong::CheckKickPower(UINT uDestID)
{
	if (!m_nFlag || (m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR))
		return FALSE;
	if (uDestID == Player[m_nPlayerIndex].m_dwID)
		return FALSE;
	if(!(m_uRight & defRIGHT_KICKOUT))
		return FALSE;

	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	sau khi duoc bo nhiem chuc vu moi
//-------------------------------------------------------------------------
void	KPlayerTong::BeInstated(STONG_SERVER_TO_CORE_BE_INSTATED *pSync)
{
	if(!m_nFlag || pSync->m_dwTongNameID != m_dwTongNameID)
		return;
	m_nFigure = pSync->m_btFigure;
	m_uRight = 0;
	if(m_nFigure == enumTONG_FIGURE_DIRECTOR)
		strcpy(m_szTitle, defTITLE_DIRECT);
	else if(m_nFigure == enumTONG_FIGURE_MANAGER)
		strcpy(m_szTitle, defTITLE_MANAGER);
	else
		strcpy(m_szTitle, defTITLE_MEMBER);
	SendSelfInfo();
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	¹¦ÄÜ£º±»Ìß³ö°ï»á
//-------------------------------------------------------------------------
void	KPlayerTong::BeKicked()
{
	if(!m_nFlag)
		return;
	Clear();

	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = camp_free;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = camp_free;

	SendSelfInfo();

	SHOW_MSG_SYNC	sMsg;
	sMsg.ProtocolType = s2c_msgshow;
	sMsg.m_wMsgID = enumMSG_ID_TONG_BE_KICK;
	sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID);
	g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sMsg, sMsg.m_wLength + 1);
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	p->szTong[0] = 0;
	p->szTitle[0] = 0;
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);

}

void KPlayerTong::BeRight(UINT uRight)
{
	if(!m_nFlag)
		return;
	m_uRight = uRight;
	SendSelfInfo();
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	dieu kien roi bang
//-------------------------------------------------------------------------
BOOL	KPlayerTong::CheckLeavePower()
{
	if (!m_nFlag || m_nFigure == enumTONG_FIGURE_MASTER || m_nFigure == enumTONG_FIGURE_DIRECTOR)
		return FALSE;
	int nMoney = Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney()/10000;
	if(nMoney < defTONG_LEAVE_MONEY)
		return FALSE;
	Player[m_nPlayerIndex].m_ItemList.CostMoney(defTONG_LEAVE_MONEY*10000);
	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	ket qua sau khi roi bang
//-------------------------------------------------------------------------
void	KPlayerTong::Leave(STONG_SERVER_TO_CORE_LEAVE *pLeave)
{
	if (pLeave->m_btSuccessFlag)
	{
		Clear();
		Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = camp_free;
		if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
		&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
			Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = camp_free;
		SendSelfInfo();

		SHOW_MSG_SYNC	sMsg;
		sMsg.ProtocolType = s2c_msgshow;
		sMsg.m_wMsgID = enumMSG_ID_TONG_LEAVE_SUCCESS;
		sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID);
		g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sMsg, sMsg.m_wLength + 1);
		//broadcast danh hieu
		char szPack[8+sizeof(PNTONG_SYNC)];
		DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
		pCmd->ProtocolType = s2c_dynamic_structure;
		pCmd->nBranch = s2cdnmbr_broadnpctong;
		pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
		PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
		p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
		p->szTong[0] = 0;
		p->szTitle[0] = 0;
		int nMaxCount = MAX_BROADCAST_COUNT;
		Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
	}
	else
	{	//that bai, tra lai tien da tru`
		Player[m_nPlayerIndex].m_ItemList.AddMoney(room_equipment, defTONG_LEAVE_MONEY*10000);

		SHOW_MSG_SYNC	sMsg;
		sMsg.ProtocolType = s2c_msgshow;
		sMsg.m_wMsgID = enumMSG_ID_TONG_LEAVE_FAIL;
		sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID);
		g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sMsg, sMsg.m_wLength + 1);
	}
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	kiem tra dieu kien chuyen vi
//-------------------------------------------------------------------------
int	KPlayerTong::CheckChangeMasterPower(UINT uDestID)
{
	if (!m_nFlag || m_nFigure != enumTONG_FIGURE_MASTER)
		return 0;
	int nDstIdx = PlayerSet.FindSame(uDestID);
	if(!nDstIdx)
	{
		char szMsg[128];
		strcpy(szMsg, "Thµnh viªn kh«ng cã mÆt n¬i ®©y ®Ó nhËn chøc Bang chñ.");
		KPlayerChat::SendSystemInfo(1, m_nPlayerIndex, MESSAGE_SYSTEM_ANNOUCE_HEAD, (char *) szMsg, strlen(szMsg) );
		return 0;
	}
	if (m_dwTongNameID != Player[nDstIdx].m_cTong.GetTongNameID())
		return 0;
	if(Player[m_nPlayerIndex].m_dwID == Player[nDstIdx].m_dwID)
		return 0;
	if(!Player[nDstIdx].m_cTong.CheckGetMasterPower(m_dwTongNameID))
	{
		char szMsg[128];
		strcpy(szMsg, "Thµnh viªn kh«ng ®ñ ®iÒu kiÖn ®Ó nhËn chøc Bang chñ.");
		KPlayerChat::SendSystemInfo(1, m_nPlayerIndex, MESSAGE_SYSTEM_ANNOUCE_HEAD, (char *) szMsg, strlen(szMsg) );
		return 0;
	}
	return nDstIdx;
}
#endif

#ifdef _SERVER
BOOL	KPlayerTong::CheckGetMasterPower(UINT dwTongID)
{
	if (!m_nFlag || m_nFigure == enumTONG_FIGURE_MASTER)
		return FALSE;
	if (dwTongID != m_dwTongNameID)
		return FALSE;
	if (Npc[Player[m_nPlayerIndex].m_nIndex].m_Level < PlayerSet.m_sTongParam.m_nLevel || 
		(int)Player[m_nPlayerIndex].m_dwLeadLevel < PlayerSet.m_sTongParam.m_nLeadLevel)
		return FALSE;

	return TRUE;
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	sau khi chuyen~ vi bang chu~ cua tung nguoi
//-------------------------------------------------------------------------
void	KPlayerTong::ChangeAs(STONG_SERVER_TO_CORE_CHANGE_AS *pAs)
{
	if (!pAs)
		return;
	if (!m_nFlag)
		return;
	m_nFigure = pAs->m_btFigure;
	if(m_nFigure == enumTONG_FIGURE_MASTER)
	{
		strcpy(m_szTitle, defTITLE_MASTER);
		m_uRight = defRIGHT_FULL;
	}
	else
	{
		strcpy(m_szTitle, defTITLE_MEMBER);
		m_uRight = 0;
	}
	//this->SendSelfInfo(); //se~ duoc broadcast sau
	
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
	
	SHOW_MSG_SYNC	sMsg;

	sMsg.ProtocolType = s2c_msgshow;
	if (m_nFigure == enumTONG_FIGURE_MASTER)
		sMsg.m_wMsgID = enumMSG_ID_TONG_CHANGE_AS_MASTER;
	else
		sMsg.m_wMsgID = enumMSG_ID_TONG_CHANGE_AS_MEMBER;
	sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID);
	g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sMsg, sMsg.m_wLength + 1);

}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	broadcast master tung` player
//-------------------------------------------------------------------------
void	KPlayerTong::ChangeMaster(char *lpszMaster)
{
	if (!lpszMaster || !lpszMaster[0])
		return;
	strcpy(m_szMasterName, lpszMaster);

	this->SendSelfInfo();
}
#endif

#ifdef _SERVER
//-------------------------------------------------------------------------
//	ket qua sau khi check bang luc dang nhap
//-------------------------------------------------------------------------
void	KPlayerTong::Login(STONG_SERVER_TO_CORE_LOGIN *pLogin)
{
	if (pLogin->m_nFlag == 0)
	{
		Clear();
		Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = camp_free;
		if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
		&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
			Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = camp_free;
		this->SendSelfInfo();

		return;
	}

	m_nFlag			= 1;
	m_nFigure		= pLogin->m_nFigure;
	m_nCamp			= pLogin->m_nCamp;
	m_dwTongNameID	= g_FileName2Id(pLogin->m_szTongName);
	m_nWeekOffer	= pLogin->m_nWeekOffer;
	m_nWeekGoalType = pLogin->m_bWGType;
	m_uRight		= pLogin->m_uRight;
	m_nApplyTo		= 0;
	strcpy(m_szName, pLogin->m_szTongName);
	strcpy(m_szTitle, pLogin->m_szTitle);
	strcpy(m_szMasterName, pLogin->m_szMaster);

	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = m_nCamp;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = m_nCamp;
	this->SendSelfInfo();
	
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);

}

BOOL	KPlayerTong::CheckChangeRecruitPower()
{
	if(!m_nFlag)
		return FALSE;
	if(!(m_uRight & defRIGHT_RECRUIT))
		return FALSE;
	return TRUE;
}

void	KPlayerTong::Delete()
{
	Clear();
	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = camp_free;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = camp_free;
	
	SendSelfInfo();

	SHOW_MSG_SYNC	sMsg;
	sMsg.ProtocolType = s2c_msgshow;
	sMsg.m_wMsgID = enumMSG_ID_TONG_DELETE;
	sMsg.m_wLength = sizeof(SHOW_MSG_SYNC) - 1 - sizeof(LPVOID);
	g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, &sMsg, sMsg.m_wLength + 1);
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	p->szTong[0] = 0;
	p->szTitle[0] = 0;
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
}

BOOL	KPlayerTong::CheckContribMoney(int nMoney)
{
	if(!m_nFlag)
		return FALSE;
	if(nMoney <= 0)
		return FALSE;
	int nCurMoney = Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney()/10000;
	if (nCurMoney < nMoney)
		return FALSE;
	Player[m_nPlayerIndex].m_ItemList.CostMoney(nMoney*10000);
	return TRUE;
}

BOOL	KPlayerTong::CheckWithDrawMoney(int nMoney)
{
	if(!m_nFlag)
		return FALSE;
	if(nMoney <= 0)
		return FALSE;
	if(!(m_uRight & defRIGHT_FUNDMANAGER))
		return FALSE;
	int nCurMoney = Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney()/10000;
	int nMod = Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney()%10000;
	if((nMod && nCurMoney + nMoney >= 200000) || (nCurMoney + nMoney > 200000))
	{
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, "Sè tiÒn ng­¬i cÇn rót v­ît søc chøa trong hµnh trang.");
		return FALSE;
	}
	return TRUE;
}

BOOL	KPlayerTong::CheckStoreOffer(int nMoney)
{
	if(!m_nFlag)
		return FALSE;
	if(nMoney <= 0)
		return FALSE;
	int nCurMoney = Player[m_nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_OFFER);
	if (nCurMoney < nMoney)
		return FALSE;
	nCurMoney -= nMoney;
	Player[m_nPlayerIndex].m_cTask.SetSaveVal(TASKVALUE_OFFER, nCurMoney);
	char szPack[16];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_taskoffer;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
	UINT* p = (UINT*)(pCmd+1);
	*p = nCurMoney;
	g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
	
	return TRUE;
}

int	KPlayerTong::CheckDispenseOffer(UINT uDestID, int nOffer)
{
	if(!m_nFlag)
		return 0;
	if(!(m_uRight & defRIGHT_FUNDMANAGER))
		return 0;
	if(nOffer <= 0 || nOffer > defTONG_MAX_OFFER_DAYLIMIT)
		return 0;
	int	nTarget = PlayerSet.FindSame(uDestID);
	if(nTarget <= 0)
	{
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, "Thµnh viªn kh«ng trùc tuyÕn ®Ó ng­¬i ph¸t ®iÓm cèng hiÕn");
		return 0;
	}
	if(m_dwTongNameID != Player[nTarget].m_cTong.GetTongNameID())
		return 0;
	int nDestDayOffer = Player[nTarget].m_cTask.GetSaveVal(TASKVALUE_OFFER_ONDAY);
	if(nDestDayOffer + nOffer > defTONG_MAX_OFFER_DAYLIMIT)
	{
		nOffer = defTONG_MAX_OFFER_DAYLIMIT - nDestDayOffer;
		char szMsg[128];
		sprintf(szMsg, "Thµnh viªn %s chØ cßn nhËn ®iÓm cèng hiÕn trong ngµy lµ %d ®iÓm",
			Npc[Player[nTarget].m_nIndex].Name, nOffer);
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, szMsg);
		return 0;
	}

	return nTarget;
}

BOOL	KPlayerTong::CheckAssignMoney(int nMemberPoint, int nManagerPoint, int nDirectorPoint)
{
	if(!m_nFlag)
		return FALSE;
	if(m_nFigure != enumTONG_FIGURE_MASTER)
		return FALSE;
	if(nMemberPoint <= 0 && nManagerPoint <= 0 && nDirectorPoint <= 0)
		return FALSE;
	if(nMemberPoint > defTONG_MAX_OFFER_DAYLIMIT || nManagerPoint > defTONG_MAX_OFFER_DAYLIMIT
	|| nDirectorPoint > defTONG_MAX_OFFER_DAYLIMIT)
		return FALSE;
	return TRUE;
}

BOOL	KPlayerTong::CheckTransMoney(int nMoney)
{
	if(!m_nFlag)
		return FALSE;
	if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
		return FALSE;
	if(!(m_uRight & defRIGHT_FUNDMANAGER))
		return FALSE;
	if(nMoney <= 0)
		return FALSE;
	return TRUE;
}

BOOL	KPlayerTong::CheckStoreBuildFund(int nMoney)
{
	if(!m_nFlag)
		return FALSE;
	if(nMoney <= 0)
		return FALSE;
	int nCurMoney = Player[m_nPlayerIndex].m_ItemList.GetEquipmentMoney()/10000;
	if (nCurMoney < nMoney)
		return FALSE;
	Player[m_nPlayerIndex].m_ItemList.CostMoney(nMoney*10000);
	return TRUE;
}

void	KPlayerTong::StoreBuildFundRet(int nWeekOffer, int nAddOffer)
{
	if(!m_nFlag)
		return;
	m_nWeekOffer = nWeekOffer;
	if(nAddOffer > 0)
	{
		int nCurOffer = Player[m_nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_OFFER);
		nCurOffer += nAddOffer;
		Player[m_nPlayerIndex].m_cTask.SetSaveVal(TASKVALUE_OFFER, nCurOffer);
		char szPack[16];
		DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
		pCmd->ProtocolType = s2c_dynamic_structure;
		pCmd->nBranch = s2cdnmbr_taskoffer;
		pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
		UINT* p = (UINT*)(pCmd+1);
		*p = nCurOffer;
		g_pServer->PackDataToClient(Player[m_nPlayerIndex].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
	}
	SendSelfInfo();
}

void	KPlayerTong::ResetWeekTask(BYTE nWeekGoalType)
{
	if(!m_nFlag)
		return;
	m_nWeekGoalType = nWeekGoalType;
	m_nWeekOffer = 0;
	SendSelfInfo();
}

BOOL	KPlayerTong::CheckAnnouncePower()
{
	if(!m_nFlag)
		return FALSE;
	if(!(m_uRight & defRIGHT_RECORD))
		return FALSE;
	return TRUE;
}

void	KPlayerTong::SetWeekOffer(int nValue)
{
	m_nWeekOffer = nValue;
	SendSelfInfo();
}

BOOL	KPlayerTong::CheckChangeTitlePower(BYTE nDestFigure, char* pszTitle)
{
	if(!m_nFlag)
		return FALSE;
	if(!(m_uRight & defRIGHT_CHANGETITLE))
		return FALSE;
	if(m_nFigure <= (int)nDestFigure)
		return FALSE;
	if(!pszTitle[0])
		return FALSE;
	g_StrLower(pszTitle);
	if(strstr(pszTitle, "bang chñ") || strstr(pszTitle, "tr­ëng l·o"))
	{
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, "§æi danh hiÖu kh«ng hîp lÖ. H·y thö l¹i tªn kh¸c.");
		return FALSE;
	}
	return TRUE;
}

BOOL	KPlayerTong::CheckChangeTitleAllPower(char* pszTitle)
{
	if(!m_nFlag)
		return FALSE;
	if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
		return FALSE;
	if(!(m_uRight & defRIGHT_CHANGETITLE))
		return FALSE;
	if(!pszTitle[0])
		return FALSE;
	g_StrLower(pszTitle);
	if(strstr(pszTitle, "bang chñ") || strstr(pszTitle, "tr­ëng l·o"))
	{
		KPlayerChat::SendSystemMsg(m_nPlayerIndex, "§æi danh hiÖu kh«ng hîp lÖ. H·y thö l¹i tªn kh¸c.");
		return FALSE;
	}
	return TRUE;
}

void	KPlayerTong::ChangeTitle(const char* pszTitle)
{
	if(!m_nFlag)
		return;
	strcpy(m_szTitle, pszTitle);
	SendSelfInfo();
	//broadcast danh hieu
	char szPack[8+sizeof(PNTONG_SYNC)];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_broadnpctong;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(PNTONG_SYNC);
	PNTONG_SYNC* p = (PNTONG_SYNC*)(pCmd+1);
	p->ID = Npc[Player[m_nPlayerIndex].m_nIndex].m_dwID;
	strcpy(p->szTong, m_szName);
	strcpy(p->szTitle, m_szTitle);
	int nMaxCount = MAX_BROADCAST_COUNT;
	Npc[Player[m_nPlayerIndex].m_nIndex].BroadCast(pCmd, pCmd->m_wLength + 1, nMaxCount);
}

BOOL	KPlayerTong::CheckChangeCampPower(BYTE btCamp)
{
	if(!m_nFlag)
		return FALSE;
	if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
		return FALSE;
	if(!(m_uRight & defRIGHT_CHANGECAMP))
		return FALSE;
	if(btCamp < camp_justice || btCamp > camp_balance)
		return FALSE;
	if(m_nCamp == (int)btCamp)
		return FALSE;
	return TRUE;
}

void	KPlayerTong::ChangeCamp(int nCamp)
{
	if(!m_nFlag)
		return;
	m_nCamp = nCamp;
	Npc[Player[m_nPlayerIndex].m_nIndex].m_Camp = m_nCamp;
	if (!Player[m_nPlayerIndex].m_cTeam.m_nFlag
	&& !g_IsExceptMap(SubWorld[Npc[Player[m_nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID))
		Npc[Player[m_nPlayerIndex].m_nIndex].m_CurrentCamp = m_nCamp;
	SendSelfInfo();
}

BOOL	KPlayerTong::CheckCanAction(int nAction)
{
	switch(nAction)
	{
		case TONG_ACTION_UPBUILDLEVEL:
		{
			if(!m_nFlag)
				return FALSE;
			if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
				return FALSE;
			if(!(m_uRight & defRIGHT_BUILDLEVEL))
				return FALSE;
			return TRUE;
		}
		case TONG_ACTION_ENTERMAP:
		{
			if(!m_nFlag)
				return FALSE;
			return TRUE;
		}
		case TONG_ACTION_CREATEMAP:
		{
			if(!m_nFlag)
				return FALSE;
			if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
				return FALSE;
			if(!(m_uRight & defRIGHT_TONGMAP))
				return FALSE;
			return TRUE;
		}
		case TONG_ACTION_CONFIGMAP:
		{
			if(!m_nFlag)
				return FALSE;
			if(m_nFigure != enumTONG_FIGURE_MASTER && m_nFigure != enumTONG_FIGURE_DIRECTOR)
				return FALSE;
			if(!(m_uRight & defRIGHT_TONGMAP))
				return FALSE;
			return TRUE;
		}
	}
	return FALSE;
}

#endif

#ifndef _SERVER
void	KPlayerTong::OpenCreateInterface()
{
	CoreDataChanged(GDCNI_OPEN_TONG_CREATE_SHEET, 1, 0);
}
#endif





