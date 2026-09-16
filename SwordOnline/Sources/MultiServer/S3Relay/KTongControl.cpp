// KTongControl.cpp: implementation of the CTongControl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "S3Relay.h"
#include "Global.h"
#include "KTongControl.h"
#include "../../Engine/src/KWin32.h"
#include "../../Engine/src/KFilePath.h"
#include "../../Engine/src/KLuaScript.h"
#include "../../Engine/src/KDebug.h"
#include "time.h"

static KLuaScript* g_pTScript = NULL;

void	InitTScript()
{
	g_FindDebugWindow("#32770","DebugWin");
	g_SetRootPath(NULL);
	g_SetFilePath("\\");
	char szFile[256];
	g_GetFullPath(szFile, "\\script\\tong\\tonghead.lua");
	g_pTScript = new KLuaScript;
	g_pTScript->Init();
	if(g_pTScript->Load(szFile))
	{
		try
		{
			int nTopIndex = 0;
			g_pTScript->SafeCallBegin(&nTopIndex);
			g_pTScript->CallFunction("RandomSeed", 0, "d", ((unsigned)time( NULL ) ^ GetTickCount()));
			g_pTScript->SafeCallEnd(nTopIndex);
		}
		catch(...)
		{
			rTRACE("RandomSeed() error");
			return;
		}
		rTRACE("TScript loaded ok");
	}
	else
	{
		rTRACE("TScript loaded fail");
	}
}

void	UnInitTScript()
{
	if(g_pTScript)
	{
		delete g_pTScript;
		g_pTScript = NULL;
	}
}
//tao bang
CTongControl::CTongControl(int nCamp, char *lpszPlayerName, char *lpszTongName, BYTE nSex)
{
	m_nCamp			= nCamp;
	m_nMoney		= 0;
	m_nBuildMoney	= 0;
	m_nLevel		= 1;
	m_nBuilLevel	= 0;
	m_nOffer		= 0;
	m_bLockRecruit	= 0;
	m_bOrdeal		= 1; //khoi tao. thu thach
	m_uWeekCount	= 1; //vao tuan thu 1
	m_nExp			= 0;
	m_nTaskLevel	= 0;
	m_uUnionID		= 0;
	m_uPromoteID	= 0;
	m_uDemiseNextMin = 0;
	m_uAssignFundNextMin = 0;
	m_uChangeTitleAllNextMin = 0;
	m_uChangeCampNextMin = 0;
	m_szAnnounce[0] = 0;
	strcpy(m_szName, lpszTongName);
	m_dwNameID = g_String2Id(m_szName);
	strcpy(m_Master.m_szName, lpszPlayerName);
	m_Master.m_dwNameID = g_String2Id(m_Master.m_szName);
	strcpy(m_Master.m_szTitle, defTITLE_MASTER);
	UINT y, mt, d, h, m;
	g_GetCurDate(y, mt, d, h, m);
	m_uCreateDate = g_DateMinute(y, mt, d, h, m);
	m_Master.m_uOnlineDate = m_Master.m_uJoinDate = m_uCreateDate;
	m_Master.m_nFigure = enumTONG_FIGURE_MASTER;
	m_Master.m_uRight = defRIGHT_FULL;
	m_Master.m_nSex = nSex;
	char szMsg[128];
	sprintf(szMsg, "%02u-%02u-%04u %s t¹o lËp bang héi %s", d,mt,y, m_Master.m_szName, m_szName);
	m_HistoryMsg.push_back(string(szMsg));
}
//khoi tao. tam thoi, sau do load database
CTongControl::CTongControl(TTongList sList)
{
	strcpy(m_szName, sList.szName);
	m_dwNameID		= g_String2Id(m_szName);
	m_uDemiseNextMin = 0;
	m_uAssignFundNextMin = 0;
	m_uChangeTitleAllNextMin = 0;
	m_uChangeCampNextMin = 0;
	m_szAnnounce[0] = 0;
}

CTongControl::~CTongControl()
{
	m_AffairMsg.clear();
	m_HistoryMsg.clear();
}

BOOL	CTongControl::AddMember(char *lpszPlayerName, char *lpszTongName, BYTE btFigure, BYTE nSex, BOOL& bForce)
{
	if(!bForce && m_bLockRecruit)
	{
		bForce = TRUE;
		return FALSE;
	}
	bForce = FALSE;
	UINT uNameID = g_String2Id(lpszPlayerName);
	if(m_Master.m_dwNameID == uNameID)
		return FALSE;
	vector<STONG_MEMBER>::iterator it;
	STONG_MEMBER sMember;
	sMember.m_dwNameID = uNameID;
	it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it != m_Director.end())
		return FALSE;
	it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
	if(it != m_Manager.end())
		return FALSE;
	it = std::find(m_Member.begin(), m_Member.end(), sMember);
	if(it != m_Member.end())
		return FALSE;
	
	sMember.m_nFigure = btFigure;
	sMember.m_nSex = nSex;
	strcpy(sMember.m_szName, lpszPlayerName);
	sMember.m_uOnlineDate = sMember.m_uJoinDate = g_GetCurDateMin();
	if(btFigure == enumTONG_FIGURE_MEMBER)
	{
		strcpy(sMember.m_szTitle, defTITLE_MEMBER);
		m_Member.push_back(sMember);
	}
	else if(btFigure == enumTONG_FIGURE_MANAGER)
	{
		strcpy(sMember.m_szTitle, defTITLE_MANAGER);
		m_Manager.push_back(sMember);
	}
	else if(btFigure == enumTONG_FIGURE_DIRECTOR)
	{
		strcpy(sMember.m_szTitle, defTITLE_DIRECT);
		m_Director.push_back(sMember);
	}
	else
		return FALSE;
	if(lpszTongName)
		strcpy(lpszTongName, m_szName);
	//save database
	g_cTongSet.SaveMember(m_szName, sMember);
	return TRUE;
}

BOOL	CTongControl::GetTongHeadInfo(STONG_HEAD_INFO_SYNC *pInfo)
{
	if (!pInfo)
		return FALSE;

	pInfo->ProtocolFamily = pf_tong;
	pInfo->ProtocolID = enumS2C_TONG_HEAD_INFO;
	pInfo->m_nMoney = m_nMoney;
	pInfo->m_nBuildMoney = m_nBuildMoney;
	pInfo->m_btCamp = m_nCamp;
	pInfo->m_btLevel = m_nLevel;
	pInfo->m_nBuilLevel = m_nBuilLevel;
	pInfo->m_nOffer = m_nOffer;
	pInfo->m_bLockRecruit = m_bLockRecruit;
	strcpy(pInfo->m_szTongName, m_szName);
	strcpy(pInfo->m_szMaster, m_Master.m_szName);
	pInfo->m_szUnion[0] = 0;//lien minh trong'
	pInfo->m_MemberNum = 1 + m_Director.size() + m_Manager.size() + m_Member.size();
	int nExPerc = (m_nExp*100) / defTONG_EXP_PERLEVEL;
	if(nExPerc > 100)
		nExPerc = 100;
	pInfo->m_btExpPercent = nExPerc;
	pInfo->m_wLength = sizeof(STONG_HEAD_INFO_SYNC);

	return TRUE;
}

int	CTongControl::FindMemPage(int nPos)
{
	int nTargetPage = nPos/defTONG_ONE_PAGE_MAX_NUM;
	if(nTargetPage < 0)
		nTargetPage = 0;
	int nMemNum = 1 + m_Director.size() + m_Manager.size() + m_Member.size();
	int nPageNum = nMemNum/defTONG_ONE_PAGE_MAX_NUM;
	if(nMemNum % defTONG_ONE_PAGE_MAX_NUM)
		++nPageNum;
	if(nTargetPage >= nPageNum)
		nTargetPage = nPageNum - 1;
	return nTargetPage;
}

STONG_MEMBER*	CTongControl::FindMember(UINT uNameID)
{
	if(uNameID == 0)
		return NULL;
	if(m_Master.m_dwNameID == uNameID)
		return &m_Master;
	
	STONG_MEMBER sMember;
	sMember.m_dwNameID = uNameID;
	vector<STONG_MEMBER>::iterator it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it != m_Director.end())
	{
		int nPos = it - m_Director.begin();
		return &m_Director[nPos];
	}
	it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
	if(it != m_Manager.end())
	{
		int nPos = it - m_Manager.begin();
		return &m_Manager[nPos];
	}
	it = std::find(m_Member.begin(), m_Member.end(), sMember);
	if(it != m_Member.end())
	{
		int nPos = it - m_Member.begin();
		return &m_Member[nPos];
	}
	return NULL;
}

BOOL	CTongControl::GetTongMemberInfo(
			STONG_GET_MEMBER_INFO_COMMAND *pApply,
			STONG_MEMBER_INFO_SYNC *pInfo, CTongConnect* pConn, CNetConnectDup* pConndup)
{
	if (!pApply || !pInfo || (!pConn && !pConndup))
		return FALSE;
	pInfo->ProtocolFamily	= pf_tong;
	pInfo->ProtocolID		= enumS2C_TONG_MEMBER_INFO;
	pInfo->m_dwPID			= pApply->m_dwPID;
	pInfo->m_dwParam		= pApply->m_dwParam;
	pInfo->m_bBegin			= 1; //xoa memlist ui
	pInfo->m_nMemNum		= 0;
	int nTargetPage = pApply->m_nPage;
	if(nTargetPage < 0)
		nTargetPage = 0;
	int nMemNum = 1 + m_Director.size() + m_Manager.size() + m_Member.size();
	int nPageNum = nMemNum/defTONG_ONE_PAGE_MAX_NUM;
	if(nMemNum % defTONG_ONE_PAGE_MAX_NUM)
		++nPageNum;
	if(nTargetPage >= nPageNum)
		nTargetPage = nPageNum - 1;
	pInfo->m_nPage = nTargetPage;
	nMemNum = nTargetPage*defTONG_ONE_PAGE_MAX_NUM;
	nPageNum = defTONG_ONE_PAGE_MAX_NUM;
	int nOffset = 1;//tinh luon bang chu
	if(pInfo->m_nPage == 0)	//co' bang chu~
	{
		*(STONG_MEMBER*)&pInfo->sMem[pInfo->m_nMemNum++] = m_Master;
		if (g_HostServer.FindPlayerByID(m_Master.m_dwNameID))
			pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 1;
	}
	int i;
	for(i = 0; i < (int)m_Director.size();++i)
	{
		if(nOffset >= nMemNum)
		{
			*(STONG_MEMBER*)&pInfo->sMem[pInfo->m_nMemNum++] = m_Director[i];
			if (g_HostServer.FindPlayerByID(m_Director[i].m_dwNameID))
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 1;
			else
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 0;
			if(pInfo->m_nMemNum >= defTONG_MAX_MEMINFOSYNC)
			{
				if(nPageNum < defTONG_MAX_MEMINFOSYNC)
					pInfo->m_nMemNum = nPageNum;
				pInfo->m_wLength = sizeof(STONG_MEMBER_INFO_SYNC)
					- sizeof(STONG_MEMSUBINFO)*defTONG_MAX_MEMINFOSYNC
					+ sizeof(STONG_MEMSUBINFO)*pInfo->m_nMemNum;
				if(pConn)
				pConn->SendPackage((const void *)pInfo, pInfo->m_wLength);
				else
				pConndup->SendPackage((const void *)pInfo, pInfo->m_wLength);
				nPageNum -= pInfo->m_nMemNum;
				pInfo->m_nMemNum = 0;
				pInfo->m_bBegin = 0;
				if(nPageNum <= 0)
					return TRUE;
			}
		}
		else
			++nOffset;
	}
	for(i = 0; i < (int)m_Manager.size();++i)
	{
		if(nOffset >= nMemNum)
		{
			*(STONG_MEMBER*)&pInfo->sMem[pInfo->m_nMemNum++] = m_Manager[i];
			if (g_HostServer.FindPlayerByID(m_Manager[i].m_dwNameID))
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 1;
			else
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 0;
			if(pInfo->m_nMemNum >= defTONG_MAX_MEMINFOSYNC)
			{
				if(nPageNum < defTONG_MAX_MEMINFOSYNC)
					pInfo->m_nMemNum = nPageNum;
				pInfo->m_wLength = sizeof(STONG_MEMBER_INFO_SYNC)
					- sizeof(STONG_MEMSUBINFO)*defTONG_MAX_MEMINFOSYNC
					+ sizeof(STONG_MEMSUBINFO)*pInfo->m_nMemNum;
				if(pConn)
				pConn->SendPackage((const void *)pInfo, pInfo->m_wLength);
				else
				pConndup->SendPackage((const void *)pInfo, pInfo->m_wLength);
				nPageNum -= pInfo->m_nMemNum;
				pInfo->m_nMemNum = 0;
				pInfo->m_bBegin = 0;
				if(nPageNum <= 0)
					return TRUE;
			}
		}
		else
			++nOffset;
	}
	for(i = 0; i < (int)m_Member.size();++i)
	{
		if(nOffset >= nMemNum)
		{
			*(STONG_MEMBER*)&pInfo->sMem[pInfo->m_nMemNum++] = m_Member[i];
			if (g_HostServer.FindPlayerByID(m_Member[i].m_dwNameID))
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 1;
			else
				pInfo->sMem[pInfo->m_nMemNum-1].btOnline = 0;
			if(pInfo->m_nMemNum >= defTONG_MAX_MEMINFOSYNC)
			{
				if(nPageNum < defTONG_MAX_MEMINFOSYNC)
					pInfo->m_nMemNum = nPageNum;
				pInfo->m_wLength = sizeof(STONG_MEMBER_INFO_SYNC)
					- sizeof(STONG_MEMSUBINFO)*defTONG_MAX_MEMINFOSYNC
					+ sizeof(STONG_MEMSUBINFO)*pInfo->m_nMemNum;
				if(pConn)
				pConn->SendPackage((const void *)pInfo, pInfo->m_wLength);
				else
				pConndup->SendPackage((const void *)pInfo, pInfo->m_wLength);
				nPageNum -= pInfo->m_nMemNum;
				pInfo->m_nMemNum = 0;
				pInfo->m_bBegin = 0;
				if(nPageNum <= 0)
					return TRUE;
			}
		}
		else
			++nOffset;
	}
	if(pInfo->m_nMemNum > 0 && nPageNum > 0)
	{
		if(pInfo->m_nMemNum > nPageNum)
			pInfo->m_nMemNum = nPageNum;
		pInfo->m_wLength = sizeof(STONG_MEMBER_INFO_SYNC)
			- sizeof(STONG_MEMSUBINFO)*defTONG_MAX_MEMINFOSYNC
			+ sizeof(STONG_MEMSUBINFO)*pInfo->m_nMemNum;
		if(pConn)
		pConn->SendPackage((const void *)pInfo, pInfo->m_wLength);
		else
		pConndup->SendPackage((const void *)pInfo, pInfo->m_wLength);
	}
	return TRUE;
}

BOOL	CTongControl::Instate(STONG_INSTATE_COMMAND *pInstate, STONG_INSTATE_SYNC *pSync)
{
	if (!pInstate || !pSync)
		return FALSE;
	if (pInstate->m_btNewFigure != enumTONG_FIGURE_DIRECTOR && pInstate->m_btNewFigure != enumTONG_FIGURE_MANAGER
		&& pInstate->m_btNewFigure != enumTONG_FIGURE_MEMBER)
		return FALSE;
	if(pInstate->m_uDestID == m_Master.m_dwNameID)
		return FALSE;
	int nPos = 1;
	STONG_MEMBER sMember;
	sMember.m_dwNameID = pInstate->m_uDestID;
	vector<STONG_MEMBER>::iterator it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it == m_Director.end())
	{
		it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
		if(it == m_Manager.end())
		{
			it = std::find(m_Member.begin(), m_Member.end(), sMember);
			if(it == m_Member.end())
				return FALSE;
		}
	}
	if (it->m_nFigure == pInstate->m_btNewFigure)
		return FALSE;
	BYTE nSdrFigure;
	if(pInstate->m_uSenderID == m_Master.m_dwNameID)
		nSdrFigure = enumTONG_FIGURE_MASTER;
	else
	{
		sMember.m_dwNameID = pInstate->m_uSenderID;
		vector<STONG_MEMBER>::iterator itSdr = std::find(m_Director.begin(), m_Director.end(), sMember);
		if(itSdr == m_Director.end())
		{
			itSdr = std::find(m_Manager.begin(), m_Manager.end(), sMember);
			if(itSdr == m_Manager.end())
			{
				itSdr = std::find(m_Member.begin(), m_Member.end(), sMember);
				if(itSdr == m_Member.end())
					return FALSE;
			}
		}
		nSdrFigure = itSdr->m_nFigure;
	}
	if(nSdrFigure == enumTONG_FIGURE_DIRECTOR && (pInstate->m_btNewFigure == enumTONG_FIGURE_DIRECTOR
		|| it->m_nFigure == enumTONG_FIGURE_DIRECTOR))
		return FALSE;
	if((pInstate->m_btNewFigure == enumTONG_FIGURE_DIRECTOR && m_Director.size() >= defTONG_MAX_DIRECTOR)
	|| (pInstate->m_btNewFigure == enumTONG_FIGURE_MANAGER && m_Manager.size() >= defTONG_MAX_MANAGER))
	{
		pSync->ProtocolFamily	= pf_tong;
		pSync->ProtocolID		= enumS2C_TONG_INSTATE;
		pSync->m_btNewFigure	= pInstate->m_btNewFigure;
		pSync->m_uSenderID		= pInstate->m_uSenderID;
		pSync->m_nSenderPIdx	= pInstate->m_nSenderPIdx;
		pSync->m_dwTongNameID	= m_dwNameID;
		return FALSE;
	}
	sMember = *it;
	sMember.m_uRight = 0;
	if (it->m_nFigure == enumTONG_FIGURE_DIRECTOR)
	{
		m_Director.erase(it);
	}
	else if(it->m_nFigure == enumTONG_FIGURE_MANAGER)
	{
		m_Manager.erase(it);
	}
	else
	{
		m_Member.erase(it);
	}
	if (pInstate->m_btNewFigure == enumTONG_FIGURE_DIRECTOR)
	{
		sMember.m_nFigure = enumTONG_FIGURE_DIRECTOR;
		strcpy(sMember.m_szTitle, defTITLE_DIRECT);
		nPos += m_Director.size();
		m_Director.push_back(sMember);
	}
	else if(pInstate->m_btNewFigure == enumTONG_FIGURE_MANAGER)
	{
		sMember.m_nFigure = enumTONG_FIGURE_MANAGER;
		strcpy(sMember.m_szTitle, defTITLE_MANAGER);
		nPos += m_Director.size() + m_Manager.size();
		m_Manager.push_back(sMember);
	}
	else
	{
		sMember.m_nFigure = enumTONG_FIGURE_MEMBER;
		strcpy(sMember.m_szTitle, defTITLE_MEMBER);
		nPos += m_Director.size() + m_Manager.size() + m_Member.size();
		m_Member.push_back(sMember);
	}
	//gui page kha~ dung. ve cho sender
	STONG_GET_MEMBER_INFO_COMMAND	sGet;
	STONG_MEMBER_INFO_SYNC	sInfo;
	sGet.m_dwPID = pInstate->m_uSenderID;
	sGet.m_dwParam = pInstate->m_nSenderPIdx;
	sGet.m_nPage = FindMemPage(nPos);
	CNetConnectDup conndupret;
	if (g_HostServer.FindPlayerByID(sGet.m_dwPID, NULL, &conndupret))
	{
		CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndupret.GetIP());
		if (tongconndup.IsValid())
		{
			GetTongMemberInfo(&sGet, &sInfo, NULL, &tongconndup);
		}
	}
	
	{{
		CNetConnectDup conndup;
		UINT uNetID;
		if (g_HostServer.FindPlayerByID(sMember.m_dwNameID, NULL, &conndup, NULL, &uNetID))
		{
			CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndup.GetIP());
			if (tongconndup.IsValid())
			{
				STONG_BE_INSTATED_SYNC	sSync;
				sSync.ProtocolFamily = pf_tong;
				sSync.ProtocolID = enumS2C_TONG_BE_INSTATED;
				sSync.m_btFigure = pInstate->m_btNewFigure;
				sSync.m_dwPlayerNameID = sMember.m_dwNameID;
				sSync.m_uNetID = uNetID;
				sSync.m_dwTongNameID = m_dwNameID;
				tongconndup.SendPackage((const void *)&sSync, sizeof(sSync));
			}
		}
	}}

	char	szMsg[96];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "%s±®­îc bæ nhiÖm lµm %s", sMember.m_szName, sMember.m_szTitle);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	STONG_MEMBER* pMem = FindMember(pInstate->m_uSenderID);
	if(pMem)
	{
		sprintf(szMsg, "%s±chØ ®Þnh %s lµm %s", pMem->m_szName, sMember.m_szName, sMember.m_szTitle);
		AddAffairMsg(szMsg);
	}
	g_cTongSet.SaveMember(m_szName, sMember);

	return TRUE;
}

BOOL	CTongControl::Kick(STONG_KICK_COMMAND *pKick, STONG_KICK_SYNC *pSync)
{
	if (!pKick || !pSync)
		return FALSE;
	UINT dwNameID = pKick->m_uDestID;
	if (dwNameID == 0)
		return FALSE;
	int nPos = 1 + m_Director.size();
	STONG_MEMBER sMember;
	sMember.m_dwNameID = dwNameID;
	vector<STONG_MEMBER>::iterator it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
	if(it == m_Manager.end())
	{
		it = std::find(m_Member.begin(), m_Member.end(), sMember);
		if(it == m_Member.end())
			return FALSE;
		nPos += it - m_Member.begin() + m_Manager.size();
	}
	else
		nPos += it - m_Manager.begin();
	char szName[32];
	strcpy(szName, it->m_szName);
	
	pSync->ProtocolFamily	= pf_tong;
	pSync->ProtocolID		= enumS2C_TONG_KICK;
	pSync->m_uSenderID		= pKick->m_uSenderID;
	pSync->m_nSenderPIdx	= pKick->m_nSenderPIdx;
	pSync->m_dwTongNameID	= m_dwNameID;
	pSync->m_uDestID		= pKick->m_uDestID;
	
	int nDeductMoney = it->m_nTotalOffer/1000;
	if(nDeductMoney > defTONG_MAX_DEDUCTMONEY)
		nDeductMoney = defTONG_MAX_DEDUCTMONEY;
	pSync->m_nNeedMoney = nDeductMoney;
	if(nDeductMoney > m_nMoney)
		return FALSE;
	
	pSync->ProtocolID = 0;
	m_nMoney -= nDeductMoney;
	
	if(it->m_nFigure == enumTONG_FIGURE_MANAGER)
	{
		m_Manager.erase(it);
	}
	else //if (pKick->m_btFigure == enumTONG_FIGURE_MEMBER)
	{
		m_Member.erase(it);
	}
	//gui page kha~ dung. ve cho sender
	STONG_GET_MEMBER_INFO_COMMAND	sGet;
	STONG_MEMBER_INFO_SYNC	sInfo;
	sGet.m_dwPID = pKick->m_uSenderID;
	sGet.m_dwParam = pKick->m_nSenderPIdx;
	sGet.m_nPage = FindMemPage(nPos);
	CNetConnectDup conndupret;
	if (g_HostServer.FindPlayerByID(sGet.m_dwPID, NULL, &conndupret))
	{
		CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndupret.GetIP());
		if (tongconndup.IsValid())
		{
			GetTongMemberInfo(&sGet, &sInfo, NULL, &tongconndup);
		}
	}
	
	{{ //gui cho nguoi bi kick neu co
		CNetConnectDup conndup;
		UINT uNetID;
		if (g_HostServer.FindPlayerByID(dwNameID, NULL, &conndup, NULL, &uNetID))
		{
			CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndup.GetIP());
			if (tongconndup.IsValid())
			{
				STONG_BE_KICKED_SYNC	sSync;
				sSync.ProtocolFamily = pf_tong;
				sSync.ProtocolID = enumS2C_TONG_BE_KICKED;
				sSync.m_uNetID = uNetID;
				sSync.m_dwPlayerNameID = dwNameID;
				sSync.m_dwTongNameID = m_dwNameID;
				tongconndup.SendPackage((const void *)&sSync, sizeof(sSync));
			}
		}
	}}

	//xoa database
	g_cTongDB.DelMember(szName);
	// thong bao
	char	szMsg[96];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "%s±bÞ khai trõ khái bæn bang", szName);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	STONG_MEMBER* pMem = FindMember(pKick->m_uSenderID);
	if(pMem)
	{
		sprintf(szMsg, "%s±trôc xuÊt %s ra khái bang", pMem->m_szName, szName);
		AddAffairMsg(szMsg, true, nDeductMoney <= 0);
	}
	if(nDeductMoney > 0)
		g_cTongDB.ChangeTong(*this);
	return TRUE;
}

BOOL	CTongControl::Leave(STONG_LEAVE_COMMAND *pLeave, STONG_LEAVE_SYNC *pSync)
{
	if (!pLeave || !pSync)
		return FALSE;

	pSync->ProtocolFamily	= pf_tong;
	pSync->ProtocolID		= enumS2C_TONG_LEAVE;
	pSync->m_nPlayerIdx		= pLeave->m_nPlayerIdx;
	pSync->m_dwTongNameID		= pLeave->m_dwTongNameID;
	pSync->m_dwPlayerNameID		= pLeave->m_dwPlayerNameID;
	pSync->m_btSuccessFlag	= 0;
	
	STONG_MEMBER sMember;
	sMember.m_dwNameID = pLeave->m_dwPlayerNameID;
	vector<STONG_MEMBER>::iterator it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
	if(it == m_Manager.end())
	{
		it = std::find(m_Member.begin(), m_Member.end(), sMember);
		if(it == m_Member.end())
			return FALSE;
	}
	strcpy(sMember.m_szName, it->m_szName);
	if(it->m_nFigure == enumTONG_FIGURE_MANAGER)
	{
		m_Manager.erase(it);
	}
	else //if (it->m_nFigure == enumTONG_FIGURE_MEMBER)
	{
		m_Member.erase(it);
	}
	
	pSync->m_btSuccessFlag = 1;
	m_nMoney += defTONG_LEAVE_MONEY;
	g_cTongDB.ChangeTong(*this);
	g_cTongDB.DelMember(sMember.m_szName);

	char	szMsg[96];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "%sÅph¶n béi bæn bang", sMember.m_szName);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}

	return TRUE;
}

BOOL	CTongControl::AcceptMaster(STONG_ACCEPT_MASTER_COMMAND *pAccept, CTongConnect* pConn)
{
	if (!pAccept)
		return FALSE;
	if(pAccept->m_uSenderID != m_Master.m_dwNameID)
		return FALSE;
	STONG_MEMBER sMember;
	sMember.m_dwNameID = pAccept->m_uDestID;
	vector<STONG_MEMBER>::iterator it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it == m_Director.end())
	{
		it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
		if(it == m_Manager.end())
		{
			it = std::find(m_Member.begin(), m_Member.end(), sMember);
			if(it == m_Member.end())
				return FALSE;
		}
	}
	STONG_CHANGE_AS_SYNC	sChange;
	sChange.ProtocolFamily	= pf_tong;
	sChange.ProtocolID		= enumS2C_TONG_CHANGE_AS;
	sChange.m_dwTongNameID			= m_dwNameID;
	UINT uCurTimeMin = g_GetCurDateMin();
	if(m_uDemiseNextMin >= uCurTimeMin)
	{
		sChange.m_dwPlayerNameID = pAccept->m_uSenderID;
		sChange.m_nPlayerIdx	= pAccept->m_nSenderPIdx;
		sChange.m_btFigure		= -1;
		pConn->SendPackage((const void *)&sChange, sizeof(sChange));
		return FALSE;
	}
	m_uDemiseNextMin = uCurTimeMin + 5;
	
	STONG_MEMBER sOldMaster = m_Master;
	STONG_MEMBER sNewMaster = *it;
	strcpy(sOldMaster.m_szTitle, defTITLE_MEMBER);
	sOldMaster.m_nFigure = enumTONG_FIGURE_MEMBER;
	sOldMaster.m_uRight = 0;
	strcpy(sNewMaster.m_szTitle, defTITLE_MASTER);
	sNewMaster.m_nFigure = enumTONG_FIGURE_MASTER;
	sNewMaster.m_uRight = defRIGHT_FULL;
	if (it->m_nFigure == enumTONG_FIGURE_DIRECTOR)
	{
		m_Director.erase(it);
	}
	else if (it->m_nFigure == enumTONG_FIGURE_MANAGER)
	{
		m_Manager.erase(it);
	}
	else //if (it->m_nFigure == enumTONG_FIGURE_MEMBER)
	{
		m_Member.erase(it);
	}
	m_Master = sNewMaster;
	m_Member.push_back(sOldMaster);
	
//bang chu cu~
	sChange.m_dwPlayerNameID = pAccept->m_uSenderID;
	sChange.m_nPlayerIdx	= pAccept->m_nSenderPIdx;
	sChange.m_btFigure		= enumTONG_FIGURE_MEMBER;
	pConn->SendPackage((const void *)&sChange, sizeof(sChange));
// bang chu~ moi'
	sChange.m_dwPlayerNameID = pAccept->m_uDestID;
	sChange.m_nPlayerIdx	= pAccept->m_nDestPIdx;
	sChange.m_btFigure		= enumTONG_FIGURE_MASTER;
	pConn->SendPackage((const void *)&sChange, sizeof(sChange));
	
//gui page 0 ve cho sender
	STONG_GET_MEMBER_INFO_COMMAND	sGet;
	STONG_MEMBER_INFO_SYNC	sInfo;
	sGet.m_dwPID = pAccept->m_uSenderID;
	sGet.m_dwParam = pAccept->m_nSenderPIdx;
	sGet.m_nPage = 0;
	CNetConnectDup conndupret;
	if (g_HostServer.FindPlayerByID(sGet.m_dwPID, NULL, &conndupret))
	{
		CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndupret.GetIP());
		if (tongconndup.IsValid())
		{
			GetTongMemberInfo(&sGet, &sInfo, NULL, &tongconndup);
		}
	}
	
	STONG_CHANGE_MASTER_SYNC	sMaster;
	sMaster.ProtocolFamily	= pf_tong;
	sMaster.ProtocolID		= enumS2C_TONG_CHANGE_MASTER;
	sMaster.m_dwTongNameID	= m_dwNameID;
	strcpy(sMaster.m_szName, m_Master.m_szName);
	g_TongServer.BroadPackage((const void*)&sMaster, sizeof(sMaster));

	//bang chu~ moi'
	g_cTongSet.SaveMember(m_szName, sNewMaster);
	// bang chu~ cu~
	g_cTongSet.SaveMember(m_szName, sOldMaster);

	char	szMsg[96];
	sprintf(szMsg, "\\O%u", m_dwNameID);

	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	sprintf(szMsg, "%s chuyÓn vÞ bang chñ cho %s", sOldMaster.m_szName, sNewMaster.m_szName);
	if (channid != -1)
	{
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	
	AddAffairMsg(szMsg);
	
	return TRUE;
}

BOOL	CTongControl::GetLoginData(STONG_GET_LOGIN_DATA_COMMAND *pLogin, STONG_LOGIN_DATA_SYNC *pSync)
{
	UINT	dwNameID = g_String2Id(pLogin->m_szName);
	if (dwNameID == 0)
		return FALSE;

	pSync->m_btCamp		= this->m_nCamp;
	strcpy(pSync->m_szTongName, this->m_szName);
	strcpy(pSync->m_szMaster, m_Master.m_szName);
	if (m_Master.m_dwNameID == dwNameID)
	{
		pSync->m_btFigure	= enumTONG_FIGURE_MASTER;
		pSync->m_btFlag		= 1;
		strcpy(pSync->m_szTitle, m_Master.m_szTitle);
		pSync->m_nWeekOffer = m_Master.m_nWeekOffer;
		pSync->m_bWGType = m_Master.m_bWGType;
		pSync->m_uRight = m_Master.m_uRight;
		m_Master.m_uOnlineDate = g_GetCurDateMin();
		g_cTongSet.SaveMember(m_szName, m_Master);
		return TRUE;
	}
	
	STONG_MEMBER sMember;
	sMember.m_dwNameID = dwNameID;
	vector<STONG_MEMBER>::iterator it;
	it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it != m_Director.end())
	{
		pSync->m_btFigure	= enumTONG_FIGURE_DIRECTOR;
		pSync->m_btFlag		= 1;
		strcpy(pSync->m_szTitle, it->m_szTitle);
		pSync->m_nWeekOffer = it->m_nWeekOffer;
		pSync->m_bWGType = it->m_bWGType;
		pSync->m_uRight = it->m_uRight;
		it->m_uOnlineDate = g_GetCurDateMin();
		g_cTongSet.SaveMember(m_szName, *it);
		return TRUE;
	}
	
	it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
	if(it != m_Manager.end())
	{
		pSync->m_btFigure	= enumTONG_FIGURE_MANAGER;
		pSync->m_btFlag		= 1;
		strcpy(pSync->m_szTitle, it->m_szTitle);
		pSync->m_nWeekOffer = it->m_nWeekOffer;
		pSync->m_bWGType = it->m_bWGType;
		pSync->m_uRight = it->m_uRight;
		it->m_uOnlineDate = g_GetCurDateMin();
		g_cTongSet.SaveMember(m_szName, *it);
		return TRUE;
	}

	it = std::find(m_Member.begin(), m_Member.end(), sMember);
	if(it != m_Member.end())
	{
		pSync->m_btFigure	= enumTONG_FIGURE_MEMBER;
		pSync->m_btFlag		= 1;
		strcpy(pSync->m_szTitle, it->m_szTitle);
		pSync->m_nWeekOffer = it->m_nWeekOffer;
		pSync->m_bWGType = it->m_bWGType;
		pSync->m_uRight = it->m_uRight;
		it->m_uOnlineDate = g_GetCurDateMin();
		g_cTongSet.SaveMember(m_szName, *it);
		return TRUE;
	}

	return TRUE;
}

BOOL 	CTongControl::DistribRight(UINT uSenderID, UINT uDestID, UINT& uRight)
{
	BOOL bOK = FALSE;
	if (m_Master.m_dwNameID != uSenderID)
		return bOK;
	STONG_MEMBER sMember;
	sMember.m_dwNameID = uDestID;
	vector<STONG_MEMBER>::iterator it;
	it = std::find(m_Director.begin(), m_Director.end(), sMember);
	if(it != m_Director.end())
	{
		uRight &= defRIGHT_FULL;
		it->m_uRight = uRight;
		bOK = TRUE;
	}
	if(!bOK)
	{
		it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
		if(it != m_Manager.end())
		{
			if(uRight & defRIGHT_CHANGETITLE)
				it->m_uRight = defRIGHT_CHANGETITLE;
			else
				it->m_uRight = 0;
			uRight = it->m_uRight;
			bOK = TRUE;
		}
	}
	if(bOK) //gui cho nguoi duoc phan quyen
	{
		g_cTongSet.SaveMember(m_szName, *it); //save database
		CNetConnectDup conndup;
		UINT uNetID;
		if (g_HostServer.FindPlayerByID(uDestID, NULL, &conndup, NULL, &uNetID))
		{
			CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndup.GetIP());
			if (tongconndup.IsValid())
			{
				STONG_BE_RIGHT_SYNC	sSync;
				sSync.ProtocolFamily = pf_tong;
				sSync.ProtocolID = enumS2C_TONG_BE_RIGHT;
				sSync.m_uNetID = uNetID;
				sSync.m_dwPlayerNameID = uDestID;
				sSync.m_uRight = uRight;
				sSync.m_dwTongNameID = m_dwNameID;
				tongconndup.SendPackage((const void *)&sSync, sizeof(sSync));
			}
		}
	}
	return bOK;
}

BOOL	CTongControl::ChangeRecruit(UINT uSenderID, int& nLockRecruit)
{
	if(uSenderID)
	{
		STONG_MEMBER* pMem = FindMember(uSenderID);
		if(!pMem)
			return FALSE;
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if(m_bLockRecruit)
			sprintf(szMsg, "%s më chøc n¨ng tuyÓn dông thµnh viªn", pMem->m_szName);
		else
			sprintf(szMsg, "%s ®ãng chøc n¨ng tuyÓn dông thµnh viªn", pMem->m_szName);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg, true, false);
	}
	m_bLockRecruit = !m_bLockRecruit;
	nLockRecruit = m_bLockRecruit;
	g_cTongDB.ChangeTong(*this);
	return TRUE;
}

BOOL	CTongControl::Update()
{
	UINT uCurDate = g_GetCurDateMin();
	UINT uCurWeek = (uCurDate - m_uCreateDate)/10080 + 1;
	BOOL bSave = FALSE;
	UINT uMemNum = 1 + m_Director.size() + m_Manager.size() + m_Member.size();
	if(m_uWeekCount != uCurWeek)
	{
		bSave = TRUE;
		m_uWeekCount = uCurWeek;
		int i;
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if(m_bOrdeal)
		{
			if(uMemNum < defTONG_CONDITION_MINMEM
			|| m_nMoney < defTONG_CONDITION_MINMONEY)
			{
				g_cTongDB.DelTong(m_szName);
				for(i = 0; i < (int)m_Director.size();++i)
				{
					g_cTongDB.DelMember(m_Director[i].m_szName);
				}
				for(i = 0; i < (int)m_Manager.size();++i)
				{
					g_cTongDB.DelMember(m_Manager[i].m_szName);
				}
				for(i = 0; i < (int)m_Member.size();++i)
				{
					g_cTongDB.DelMember(m_Member[i].m_szName);
				}
				g_cTongDB.DelMember(m_Master.m_szName);
				STONG_DELETE_SYNC	sDelTong;
				sDelTong.ProtocolFamily	= pf_tong;
				sDelTong.ProtocolID		= enumS2C_TONG_DELETE;
				sDelTong.m_dwTongNameID	= m_dwNameID;
				g_TongServer.BroadPackage((const void*)&sDelTong, sizeof(sDelTong));
				return FALSE;
			}
			m_bOrdeal = 0;
			sprintf(szMsg, "Bang héi ®· thµnh c«ng v­ît qua giai ®o¹n thö th¸ch");
			if (channid != -1)
			{
				g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("Th«ng b¸o"), std::string(szMsg));
			}
			AddAffairMsg(szMsg, true, false);
		}
		else
		{
			if(uMemNum < defTONG_CONDITION_MINMEM
			|| m_nMoney < defTONG_CONDITION_MINMONEY)
			{
				m_bOrdeal = 1;
				if (channid != -1)
				{
					sprintf(szMsg, "Bang héi b­íc vµo thêi gian thö th¸ch v× kh«ng ®ñ ®iÒu kiÖn tèi thiÓu: %d ng­êi, ng©n quü %d v¹n l­îng",
						defTONG_CONDITION_MINMEM, defTONG_CONDITION_MINMONEY);
					g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("Th«ng b¸o"), std::string(szMsg));
				}
				sprintf(szMsg, "Bang héi b­íc vµo thêi gian thö th¸ch");
				AddAffairMsg(szMsg, true, false);
			}
		}
		//reset nhiem vu muc tieu tuan
		ResetTask(false);
	}
	if(m_nLevel < defTONG_MAX_LEVEL)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if(m_nMoney >= defTONG_EXP_COSTMONEY)
		{
			bSave = TRUE;
			m_nMoney -= defTONG_EXP_COSTMONEY;
			if(m_nLevel <= 0)
				m_nLevel = 1;
			int nAddExp = defTONG_EXP_PERLEVEL/(m_nLevel*80);
			if(nAddExp <= 0)
				nAddExp = 1;
			m_nExp += nAddExp;
			if(m_nExp >= defTONG_EXP_PERLEVEL)
			{
				++m_nLevel;
				m_nExp = 0;
				sprintf(szMsg, "Bang héi ®· ®¹t ®Õn ®¼ng cÊp %d", m_nLevel);
				if (channid != -1)
				{
					g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("Th«ng b¸o"), std::string(szMsg));
				}
				if(!(m_nLevel % 10))
				{
					AddHistoryMsg(szMsg, true, false);
				}
			}
		}
		else
		{
			if (channid != -1)
			{
				sprintf(szMsg, "Ng©n quü kh«ng ®ñ %d v¹n l­îng ®Ó t¨ng kinh nghiÖm bang héi", defTONG_EXP_COSTMONEY);
				g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("Th«ng b¸o"), std::string(szMsg));
			}
		}
	}
	if(m_nMoney < defTONG_CONDITION_MINMONEY)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if (channid != -1)
		{
			sprintf(szMsg, "§Ó bang héi duy tr× ho¹t ®éng, ng©n quü cÇn tèi thiÓu %d v¹n l­îng", defTONG_CONDITION_MINMONEY);
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("HÖ thèng"), std::string(szMsg));
		}
	}
	else if(uMemNum < defTONG_CONDITION_MINMEM)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if (channid != -1)
		{
			sprintf(szMsg, "§Ó bang héi duy tr× ho¹t ®éng, nh©n sè tèi thiÓu ph¶i cã %d ng­êi", defTONG_CONDITION_MINMEM);
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("HÖ thèng"), std::string(szMsg));
		}
	}
	if(bSave)
		g_cTongDB.ChangeTong(*this);
	return TRUE;
}

BOOL	CTongControl::AddMoney(UINT uSenderID, int& nMoney)
{
	if(nMoney <= 0)
		return FALSE;
	m_nMoney += nMoney;
	g_cTongDB.ChangeTong(*this);
	STONG_MEMBER* pMem = FindMember(uSenderID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if (channid != -1)
		{
			sprintf(szMsg, "%s ®ãng gãp vµo ng©n quü bang héi %d v¹n l­îng", pMem->m_szName, nMoney);
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
	}
	nMoney = m_nMoney; //tra lai ket qua
	return TRUE;
}

BOOL	CTongControl::WithDrawMoney(UINT uSenderID, int& nMoney)
{
	if(nMoney <= 0)
		return FALSE;
	if(m_nMoney < nMoney)
	{
		nMoney = -1;
		return TRUE;
	}
	m_nMoney -= nMoney;
	STONG_MEMBER* pMem = FindMember(uSenderID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		sprintf(szMsg, "%s rót tõ ng©n quü bang héi %d v¹n l­îng", pMem->m_szName, nMoney);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
	nMoney = m_nMoney; //tra lai ket qua
	return TRUE;
}

BOOL	CTongControl::AddOffer(UINT uSenderID, int& nOffer)
{
	if(nOffer <= 0)
		return FALSE;
	if(m_nOffer + nOffer < 0 || m_nOffer + nOffer > 2000000000L)
		m_nOffer = 2000000000L;
	else
		m_nOffer += nOffer;
	g_cTongDB.ChangeTong(*this);
	STONG_MEMBER* pMem = FindMember(uSenderID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if (channid != -1)
		{
			sprintf(szMsg, "%s göi %d ®iÓm vµo cèng hiÕn dù tr÷ bang", pMem->m_szName, nOffer);
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
	}
	nOffer = m_nOffer;
	return TRUE;
}

BOOL	CTongControl::DispenseOffer(UINT uSenderID, UINT uDestID, int& nOffer)
{
	if(nOffer <= 0)
		return FALSE;
	if(m_nOffer < nOffer)
	{
		nOffer = -1;
		return TRUE;
	}
	m_nOffer -= nOffer;
	STONG_MEMBER* pMem = FindMember(uSenderID);
	STONG_MEMBER* pDstMem = FindMember(uDestID);
	if(pMem && pDstMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		sprintf(szMsg, "%s ph¸t cho %s %d ®iÓm tõ cèng hiÕn dù tr÷ bang", pMem->m_szName, pDstMem->m_szName, nOffer);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
	nOffer = m_nOffer;
	return TRUE;
}

void	CTongControl::AssignMoney(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_ASSIGNFUND_SYNC sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_ASSIGN_MONEY_RET;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
	sSync.nDirectorPoint = pInfo->nDirectorPoint > 0?pInfo->nDirectorPoint:0;
	sSync.nManagerPoint = pInfo->nManagerPoint > 0?pInfo->nManagerPoint:0;
	sSync.nMemberPoint = pInfo->nMemberPoint > 0?pInfo->nMemberPoint:0;
	UINT uCurTimeMin = g_GetCurDateMin();
	if(m_uAssignFundNextMin >= uCurTimeMin)
	{
		sSync.m_nMoney = -1;
		sSync.m_bSuccess = 0;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	UINT i;
	int nCost = 0, nDirectorNum = 0, nManagerNum = 0, nMemberNum = 0;
	if(pInfo->nDirectorPoint > 0)
	{
		for(i = 0; i < m_Director.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Director[i].m_dwNameID))
			{
				nCost += pInfo->nDirectorPoint;
				++nDirectorNum;
			}
		}
	}
	if(pInfo->nManagerPoint > 0)
	{
		for(i = 0; i < m_Manager.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Manager[i].m_dwNameID))
			{
				nCost += pInfo->nManagerPoint;
				++nManagerNum;
			}
		}
	}
	if(pInfo->nMemberPoint > 0)
	{
		for(i = 0; i < m_Member.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Member[i].m_dwNameID))
			{
				nCost += pInfo->nMemberPoint;
				++nMemberNum;
			}
		}
	}
	if(nCost == 0)
		return;
	if(m_nMoney < nCost)
	{
		sSync.m_nMoney = nCost;
		sSync.m_bSuccess = 0;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	m_uAssignFundNextMin = uCurTimeMin + 5;
	
	m_nMoney -= nCost;
	sSync.m_nMoney = m_nMoney;
	sSync.m_bSuccess = 1;
	pConn->SendPackage((const void *)&sSync, sizeof(sSync));
	
	char szMsg[256];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "Bang chñ ph¸t ng©n quü: Tr­ëng l·o %d v¹n - %d ng­êi, §éi tr­ëng %d v¹n - %d ng­êi, §Ö tö %d v¹n - %d ng­êi. Tiªu hao quü bang: %d v¹n",
				sSync.nDirectorPoint, nDirectorNum,
				sSync.nManagerPoint, nManagerNum,
				sSync.nMemberPoint, nMemberNum,
				nCost);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	if(pInfo->m_dwPlayerNameID == m_Master.m_dwNameID)
	{
		sprintf(szMsg, "%s ph¸t tiÒn cho bang chóng tiªu hao ng©n quü: %d v¹n l­îng", m_Master.m_szName, nCost);
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
}

void	CTongControl::AssignOffer(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_ASSIGNFUND_SYNC sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_ASSIGN_OFFER_RET;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
	sSync.nDirectorPoint = pInfo->nDirectorPoint > 0?pInfo->nDirectorPoint:0;
	sSync.nManagerPoint = pInfo->nManagerPoint > 0?pInfo->nManagerPoint:0;
	sSync.nMemberPoint = pInfo->nMemberPoint > 0?pInfo->nMemberPoint:0;
	UINT uCurTimeMin = g_GetCurDateMin();
	if(m_uAssignFundNextMin >= uCurTimeMin)
	{
		sSync.m_nMoney = -1;
		sSync.m_bSuccess = 0;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	UINT i;
	int nCost = 0, nDirectorNum = 0, nManagerNum = 0, nMemberNum = 0;
	if(pInfo->nDirectorPoint > 0)
	{
		for(i = 0; i < m_Director.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Director[i].m_dwNameID))
			{
				nCost += pInfo->nDirectorPoint;
				++nDirectorNum;
			}
		}
	}
	if(pInfo->nManagerPoint > 0)
	{
		for(i = 0; i < m_Manager.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Manager[i].m_dwNameID))
			{
				nCost += pInfo->nManagerPoint;
				++nManagerNum;
			}
		}
	}
	if(pInfo->nMemberPoint > 0)
	{
		for(i = 0; i < m_Member.size();++i)
		{
			if (g_HostServer.FindPlayerByID(m_Member[i].m_dwNameID))
			{
				nCost += pInfo->nMemberPoint;
				++nMemberNum;
			}
		}
	}
	if(nCost == 0)
		return;
	
	if(m_nOffer < nCost)
	{
		sSync.m_nMoney = nCost;
		sSync.m_bSuccess = 0;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	m_uAssignFundNextMin = uCurTimeMin + 5;
	
	m_nOffer -= nCost;
	sSync.m_nMoney = m_nOffer;
	sSync.m_bSuccess = 1;
	pConn->SendPackage((const void *)&sSync, sizeof(sSync));
	
	char szMsg[256];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "Bang chñ ph¸t cèng hiÕn: Tr­ëng l·o %d ®iÓm - %d ng­êi, §éi tr­ëng %d ®iÓm - %d ng­êi, §Ö tö %d ®iÓm - %d ng­êi. Tiªu hao cèng hiÕn dù tr÷: %d ®iÓm",
				sSync.nDirectorPoint, nDirectorNum,
				sSync.nManagerPoint, nManagerNum,
				sSync.nMemberPoint, nMemberNum,
				nCost);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	if(pInfo->m_dwPlayerNameID == m_Master.m_dwNameID)
	{
		sprintf(szMsg, "%s ph¸t cèng hiÕn dù tr÷ cho bang chóng tiªu hao: %d ®iÓm", m_Master.m_szName, nCost);
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
}

BOOL	CTongControl::TransMoney(UINT uSenderID, int& nMoney, int& nBuildFund)
{
	if(nMoney <= 0)
		return FALSE;
	if(m_nMoney < nMoney)
	{
		nMoney = -1;
		nBuildFund = -1;
		return TRUE;
	}
	m_nMoney -= nMoney;
	m_nBuildMoney += nMoney;
	STONG_MEMBER* pMem = FindMember(uSenderID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		sprintf(szMsg, "%s chuyÓn ng©n quü vµo ng©n s¸ch kiÕn thiÕt %d v¹n l­îng", pMem->m_szName, nMoney);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
	nMoney = m_nMoney; //tra lai ket qua
	nBuildFund = m_nBuildMoney;
	return TRUE;
}

BOOL	CTongControl::AddBuildFund(STONG_CONTRIBMONEY_COMMAND	*pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return FALSE;
	if(pInfo->m_nMoney == 0)
		return FALSE;
	if(pInfo->m_nMoney < 0)
	{
		if(m_nBuildMoney + pInfo->m_nMoney < 0)
		{
			if(m_nBuildMoney == 0)
				return FALSE;
			pInfo->m_nMoney = -m_nBuildMoney;
		}
	}
	m_nBuildMoney += pInfo->m_nMoney;
	g_cTongDB.ChangeTong(*this);
	if(pInfo->m_nMoney > 0)
	{
		STONG_MEMBER* pMem = FindMember(pInfo->m_dwPlayerNameID);
		if(pMem)
		{
			char szMsg[128];
			sprintf(szMsg, "\\O%u", m_dwNameID);
			DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
			if (channid != -1)
			{
				sprintf(szMsg, "%s göi vµo ng©n s¸ch kiÕn thiÕt %d v¹n l­îng", pMem->m_szName, pInfo->m_nMoney);
				g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
			}
			int nOffer = pInfo->m_nMoney/defTONG_MONEY2OFFER_RATE;
			if(nOffer > 0)
			{
				if(pMem->m_nWeekOffer < defTONG_MAX_WEEK_OFFER)
				{
					int nRemain = defTONG_MAX_WEEK_OFFER - pMem->m_nWeekOffer;
					if(nRemain < nOffer)
						nOffer = nRemain;
					pMem->m_nWeekOffer += nOffer;
					pMem->m_nTotalOffer += nOffer;
					g_cTongSet.SaveMember(m_szName, *pMem);
				}
				else
					nOffer = 0;
			}
			STONG_STOREBUILDFUND_SYNC sSync;
			sSync.ProtocolFamily = pf_tong;
			sSync.ProtocolID = enumS2C_TONG_STOREBUILDFUND;
			sSync.m_dwTongNameID = m_dwNameID;
			sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
			sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
			sSync.m_nWeekOffer = pMem->m_nWeekOffer;
			sSync.m_nBuildFund = m_nBuildMoney;
			sSync.m_nAddOffer = nOffer;
			pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		}
	}
	return TRUE;
}

BOOL	CTongControl::GeneratesTask(int nLevel, UINT nMembers, bool bSync /*= false*/)
{
	if(!g_pTScript)
		return FALSE;
	if(nLevel < 0)
		nLevel = 0;
	else if(nLevel > 2)
		nLevel = 2;
	try
	{
		int nTopIndex = 0;
		g_pTScript->SafeCallBegin(&nTopIndex);
		g_pTScript->CallFunction("GenerateTask", 5, "dd", nLevel, nMembers);
		m_NewWeekGoal.wWeekGoalType = (WORD) Lua_ValueToNumber(g_pTScript->m_LuaState, 1);
		m_NewWeekGoal.nMWeekGoal = (int) Lua_ValueToNumber(g_pTScript->m_LuaState, 2);
		m_NewWeekGoal.nTWeekGoal = (int) Lua_ValueToNumber(g_pTScript->m_LuaState, 3);
		m_NewWeekGoal.nMWeeGoalPrice = (int) Lua_ValueToNumber(g_pTScript->m_LuaState, 4);
		m_NewWeekGoal.nTWeeGoalPrice = (int) Lua_ValueToNumber(g_pTScript->m_LuaState, 5);
		m_NewWeekGoal.nTaskLevel = nLevel;
		g_pTScript->SafeCallEnd(nTopIndex);
	}
	catch(...)
	{
		rTRACE("GenerateTask() error");
		return FALSE;
	}
	//rTRACE("task %d-%d-%d-%d-%d", m_NewWeekGoal.wWeekGoalType, m_NewWeekGoal.nMWeekGoal,
	//	m_NewWeekGoal.nTWeekGoal, m_NewWeekGoal.nMWeeGoalPrice, m_NewWeekGoal.nTWeeGoalPrice);
	return TRUE;
}

BOOL	CTongControl::GetRecordWeekTask(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	if (!pApply || !pConn)
		return FALSE;
	STONG_MEMBER* pMem = FindMember(pApply->m_dwPID);
	if(!pMem)
		return FALSE;
	STONG_RECORD_WEEKTASK_SYNC sInfo;
	sInfo.ProtocolFamily	= pf_tong;
	sInfo.ProtocolID		= enumS2C_TONG_RECORD_WEEKTASK;
	sInfo.m_dwPlayerNameID	= pApply->m_dwPID;
	sInfo.m_nPlayerIdx		= pApply->m_dwParam;
	sInfo.m_dwTongNameID	= pApply->m_dwTongNameID;
	sInfo.m_OldWeekGoal		= m_OldWeekGoal;
	sInfo.m_NewWeekGoal		= m_NewWeekGoal;
	sInfo.m_uWeekCount		= m_uWeekCount;
	sInfo.m_bOrdeal			= m_bOrdeal;
	UINT uCurDate = g_GetCurDateMin();
	sInfo.m_DayNum = ((uCurDate - m_uCreateDate)%10080)/1440 + 1;
	if(sInfo.m_DayNum > 7)
		sInfo.m_DayNum = 7;
	sInfo.m_nTaskLevel		= m_nTaskLevel;
	sInfo.m_nOldWGCompleted = pMem->m_nOldWGCompleted;
	sInfo.m_nNewWGCompleted = pMem->m_nNewWGCompleted;
	sInfo.m_bWGType			= pMem->m_bWGType;
	sInfo.m_bGetPrice		= pMem->m_bGetPrice;
	pConn->SendPackage((const void *)&sInfo, sizeof(sInfo));
	
	return TRUE;
}

BOOL	CTongControl::GetRecordAnnounce(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	if (!pApply || !pConn)
		return FALSE;
	
	STONG_RECORD_ANNOUNCE_SYNC sInfo;
	sInfo.ProtocolFamily	= pf_tong;
	sInfo.ProtocolID		= enumS2C_TONG_RECORD_ANNOUNCE;
	sInfo.m_dwPlayerNameID	= pApply->m_dwPID;
	sInfo.m_nPlayerIdx		= pApply->m_dwParam;
	sInfo.m_dwTongNameID	= pApply->m_dwTongNameID;
	strcpy(sInfo.m_szAnnounce, m_szAnnounce);
	pConn->SendPackage((const void *)&sInfo, sizeof(sInfo));
	
	return TRUE;
}

BOOL	CTongControl::GetRecordAffair(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	if (!pApply || !pConn)
		return FALSE;
	int nCount = (int)m_AffairMsg.size();
	if(nCount == 0)
		return TRUE;
	
	STONG_RECORD_AFFAIRHISTORY_SYNC sInfo;
	sInfo.ProtocolFamily	= pf_tong;
	sInfo.ProtocolID		= enumS2C_TONG_RECORD_AFFAIRHISTORY;
	sInfo.m_dwPlayerNameID	= pApply->m_dwPID;
	sInfo.m_nPlayerIdx		= pApply->m_dwParam;
	sInfo.m_dwTongNameID	= pApply->m_dwTongNameID;
	sInfo.m_nMsgCount		= 0;
	for(int i = 0; i < nCount; ++i)
	{
		strcpy(sInfo.m_szMsg[sInfo.m_nMsgCount++], m_AffairMsg[i].c_str());
		if(sInfo.m_nMsgCount >= 4)
		{
			sInfo.m_wLength = sizeof(STONG_RECORD_AFFAIRHISTORY_SYNC);
			pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
			sInfo.m_nMsgCount = 0;
		}
	}
	if(sInfo.m_nMsgCount > 0)
	{
		sInfo.m_wLength = sizeof(STONG_RECORD_AFFAIRHISTORY_SYNC) - sizeof(sInfo.m_szMsg)
						+ sizeof(sInfo.m_szMsg[0])*sInfo.m_nMsgCount;
		pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
	}
	return TRUE;
}

BOOL	CTongControl::GetRecordHistory(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	if (!pApply || !pConn)
		return FALSE;
	int nCount = (int)m_HistoryMsg.size();
	if(nCount == 0)
		return TRUE;
	
	STONG_RECORD_AFFAIRHISTORY_SYNC sInfo;
	sInfo.ProtocolFamily	= pf_tong;
	sInfo.ProtocolID		= enumS2C_TONG_RECORD_AFFAIRHISTORY;
	sInfo.m_dwPlayerNameID	= pApply->m_dwPID;
	sInfo.m_nPlayerIdx		= pApply->m_dwParam;
	sInfo.m_dwTongNameID	= pApply->m_dwTongNameID;
	sInfo.m_nMsgCount		= 0;
	for(int i = 0; i < nCount; ++i)
	{
		strcpy(sInfo.m_szMsg[sInfo.m_nMsgCount++], m_HistoryMsg[i].c_str());
		if(sInfo.m_nMsgCount >= 4)
		{
			sInfo.m_wLength = sizeof(STONG_RECORD_AFFAIRHISTORY_SYNC);
			pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
			sInfo.m_nMsgCount = 0;
		}
	}
	if(sInfo.m_nMsgCount > 0)
	{
		sInfo.m_wLength = sizeof(STONG_RECORD_AFFAIRHISTORY_SYNC) - sizeof(sInfo.m_szMsg)
						+ sizeof(sInfo.m_szMsg[0])*sInfo.m_nMsgCount;
		pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
	}
	return TRUE;
}

BOOL	CTongControl::SetAnnounce(STONG_ANNOUNCE_COMMAND* pInfo)
{
	if (!pInfo)
		return FALSE;
	strcpy(m_szAnnounce, pInfo->m_szAnnounce);
	g_cTongDB.ChangeTong(*this);
	STONG_MEMBER* pMem = FindMember(pInfo->m_dwPlayerNameID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if (channid != -1)
		{
			sprintf(szMsg, "%s cËp nhËt C«ng C¸o cña bæn bang", pMem->m_szName);
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
	}
	return TRUE;
}

void	CTongControl::AddAffairMsg(const char* pMsg, bool bDate, bool bSave)
{
	if(m_AffairMsg.size() >= defTONG_MAX_MSG)
	{
		m_AffairMsg.erase(m_AffairMsg.begin());
	}
	
	char Buff[128];
	int nLen = 0;
	if(bDate)
	{
		UINT y, mt, d, h, m;
		g_GetCurDate(y, mt, d, h, m);
		nLen = sprintf(Buff, "%02u-%02u-%04u ", d,mt,y);
	}
	int nMsgLen = strlen(pMsg);
	if(sizeof(Buff) - nLen - 1 < nMsgLen)
		nMsgLen = sizeof(Buff) - nLen - 1;
	memcpy(&Buff[nLen], pMsg, nMsgLen);
	Buff[nLen+nMsgLen] = 0;
	
	m_AffairMsg.push_back(string(Buff));
	if(bSave)
		g_cTongDB.ChangeTong(*this);
}

void	CTongControl::AddHistoryMsg(const char* pMsg, bool bDate, bool bSave)
{
	if(m_HistoryMsg.size() >= defTONG_MAX_MSG)
	{
		m_HistoryMsg.erase(m_HistoryMsg.begin());
	}
	
	char Buff[128];
	int nLen = 0;
	if(bDate)
	{
		UINT y, mt, d, h, m;
		g_GetCurDate(y, mt, d, h, m);
		nLen = sprintf(Buff, "%02u-%02u-%04u ", d,mt,y);
	}
	int nMsgLen = strlen(pMsg);
	if(sizeof(Buff) - nLen - 1 < nMsgLen)
		nMsgLen = sizeof(Buff) - nLen - 1;
	memcpy(&Buff[nLen], pMsg, nMsgLen);
	Buff[nLen+nMsgLen] = 0;
	
	m_HistoryMsg.push_back(string(Buff));
	if(bSave)
		g_cTongDB.ChangeTong(*this);
}

const char g_TTaskName[][64] =
{
	"NhiÖm vô kh«ng x¸c ®Þnh",
	"ChiÕn tr­êng Tèng Kim",
	"NhiÖm vô TÝn Sø",
	"Th¸ch thøc thêi gian",
	"Chuçi nhiÖm vô D· TÈu",
};

void	CTongControl::ResetTask(bool bSave)
{
	m_OldWeekGoal = m_NewWeekGoal;
	memset(&m_NewWeekGoal, 0, sizeof(TTongWeekGoal));
	GeneratesTask(m_nTaskLevel, 1 + m_Director.size() + m_Manager.size() + m_Member.size());
	//sync reset gs
	STONG_WEEKLYRESET_SYNC sSync;
	sSync.ProtocolFamily	= pf_tong;
	sSync.ProtocolID		= enumS2C_TONG_WEEKLYRESET;
	sSync.m_dwTongNameID	= m_dwNameID;
	sSync.m_nWeekGoalType	= (BYTE)m_NewWeekGoal.wWeekGoalType;
	g_TongServer.BroadPackage((const void*)&sSync, sizeof(sSync));
	//reset database
	m_Master.m_bWGType = (BYTE)m_NewWeekGoal.wWeekGoalType;
	m_Master.m_nOldWGCompleted = m_Master.m_nNewWGCompleted;
	m_Master.m_nNewWGCompleted = 0;
	m_Master.m_nWeekOffer = 0;
	m_Master.m_bGetPrice = 0;
	g_cTongSet.SaveMember(m_szName, m_Master);
	int i;
	for(i = 0; i < (int)m_Director.size();++i)
	{
		m_Director[i].m_bWGType = (BYTE)m_NewWeekGoal.wWeekGoalType;
		m_Director[i].m_nOldWGCompleted = m_Director[i].m_nNewWGCompleted;
		m_Director[i].m_nNewWGCompleted = 0;
		m_Director[i].m_nWeekOffer = 0;
		m_Director[i].m_bGetPrice = 0;
		g_cTongSet.SaveMember(m_szName, m_Director[i]);
	}
	for(i = 0; i < (int)m_Manager.size();++i)
	{
		m_Manager[i].m_bWGType = (BYTE)m_NewWeekGoal.wWeekGoalType;
		m_Manager[i].m_nOldWGCompleted = m_Manager[i].m_nNewWGCompleted;
		m_Manager[i].m_nNewWGCompleted = 0;
		m_Manager[i].m_nWeekOffer = 0;
		m_Manager[i].m_bGetPrice = 0;
		g_cTongSet.SaveMember(m_szName, m_Manager[i]);
	}
	for(i = 0; i < (int)m_Member.size();++i)
	{
		m_Member[i].m_bWGType = (BYTE)m_NewWeekGoal.wWeekGoalType;
		m_Member[i].m_nOldWGCompleted = m_Member[i].m_nNewWGCompleted;
		m_Member[i].m_nNewWGCompleted = 0;
		m_Member[i].m_nWeekOffer = 0;
		m_Member[i].m_bGetPrice = 0;
		g_cTongSet.SaveMember(m_szName, m_Member[i]);
	}
	char szMsg[128];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	sprintf(szMsg, "Môc tiªu trong tuÇn cña bang: %s", g_TTaskName[m_NewWeekGoal.wWeekGoalType]);
	if (channid != -1)
	{
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	AddAffairMsg(szMsg, true, false);
	if(bSave)
		g_cTongDB.ChangeTong(*this);
}

void	CTongControl::AddWeekGoal(UINT dwPlayerNameID, int nValue)
{
	if(!m_NewWeekGoal.wWeekGoalType || !m_NewWeekGoal.nTWeekGoal || !m_NewWeekGoal.nMWeekGoal)
		return;
	int nOldDoneWG = m_NewWeekGoal.nTCompletedWG;
	STONG_MEMBER* pMem = FindMember(dwPlayerNameID);
	if(pMem)
	{
		if(nValue <= 0)
			return;
		if(!pMem->m_bWGType)
			return;
		if(pMem->m_nNewWGCompleted >= m_NewWeekGoal.nMWeekGoal)
			return;
		if(nValue + pMem->m_nNewWGCompleted > m_NewWeekGoal.nMWeekGoal)
			nValue = m_NewWeekGoal.nMWeekGoal - pMem->m_nNewWGCompleted;
		pMem->m_nNewWGCompleted += nValue;
		pMem->m_nTotalOffer += nValue;
		pMem->m_nWeekOffer += nValue;
		if(pMem->m_nWeekOffer > defTONG_MAX_WEEK_OFFER)
			pMem->m_nWeekOffer = defTONG_MAX_WEEK_OFFER;
		g_cTongSet.SaveMember(m_szName, *pMem);
		m_NewWeekGoal.nTCompletedWG += nValue;
		if(m_NewWeekGoal.nTCompletedWG > m_NewWeekGoal.nTWeekGoal)
			m_NewWeekGoal.nTCompletedWG = m_NewWeekGoal.nTWeekGoal;
		//send add offer cho doi tuong
		CNetConnectDup conndup;
		UINT uNetID;
		if (g_HostServer.FindPlayerByID(dwPlayerNameID, NULL, &conndup, NULL, &uNetID))
		{
			CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndup.GetIP());
			if (tongconndup.IsValid())
			{
				STONG_ADDWEEKGOAL_SYNC sSync;
				sSync.ProtocolFamily = pf_tong;
				sSync.ProtocolID = enumS2C_TONG_ADDWEEKGOAL_RET;
				sSync.m_dwTongNameID = m_dwNameID;
				sSync.m_dwPlayerNameID = dwPlayerNameID;
				sSync.m_uNetID = uNetID;
				sSync.m_nWeekOffer = pMem->m_nWeekOffer;
				sSync.m_nAddOffer = nValue;
				tongconndup.SendPackage((const void *)&sSync, sizeof(sSync));
			}
		}
	}
	else
	{
		m_NewWeekGoal.nTCompletedWG += nValue;
		if(m_NewWeekGoal.nTCompletedWG < 0)
			m_NewWeekGoal.nTCompletedWG = 0;
		else if(m_NewWeekGoal.nTCompletedWG > m_NewWeekGoal.nTWeekGoal)
			m_NewWeekGoal.nTCompletedWG = m_NewWeekGoal.nTWeekGoal;
	}
	if(nOldDoneWG != m_NewWeekGoal.nTCompletedWG)
		g_cTongDB.ChangeTong(*this);
}

void	CTongControl::SetTaskLevel(UINT dwPlayerNameID, int nValue)
{
	m_nTaskLevel = nValue;
	char szMsg[128];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "§é khã môc tiªu tuÇn hiÖn t¹i lµ: %d cÊp", m_nTaskLevel);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	STONG_MEMBER* pMem = FindMember(dwPlayerNameID);
	if(pMem)
	{
		sprintf(szMsg, "%s thiÕt lËp ®é khã môc tiªu tuÇn lµ %d cÊp", pMem->m_szName, m_nTaskLevel);
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
}

void	CTongControl::ReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_RECEIVEPRICE_SYNC sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_TONG_RECEIVEPRICE_RET;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nValue;
	sSync.m_btSuccessFlag = 0;
	if(m_OldWeekGoal.bTGetPrice)
	{
		if(sSync.m_dwPlayerNameID)
		{
			pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		}
		return;
	}
	if(!m_OldWeekGoal.wWeekGoalType || !m_OldWeekGoal.nTWeekGoal)
	{
		sSync.m_btSuccessFlag = 1;
		if(sSync.m_dwPlayerNameID)
		{
			pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		}
		return;
	}
	if(m_OldWeekGoal.nTCompletedWG < m_OldWeekGoal.nTWeekGoal)
	{
		sSync.m_btSuccessFlag = 2;
		if(sSync.m_dwPlayerNameID)
		{
			pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		}
		return;
	}
	m_OldWeekGoal.bTGetPrice = 1;
	sSync.m_btSuccessFlag = 3;
	m_nOffer += m_OldWeekGoal.nTWeeGoalPrice;
	if(sSync.m_dwPlayerNameID)
	{
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
	}
	char szMsg[128];
	sprintf(szMsg, "\\O%u", m_dwNameID);
	DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
	if (channid != -1)
	{
		sprintf(szMsg, "Bang héi ®· nhËn th­ëng môc tiªu tuÇn: %d ®iÓm cèng hiÕn dù tr÷.", m_OldWeekGoal.nTWeeGoalPrice);
		g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
	}
	STONG_MEMBER* pMem = FindMember(sSync.m_dwPlayerNameID);
	if(pMem)
	{
		sprintf(szMsg, "%s nhËn th­ëng môc tiªu tuÇn bang héi: %d cèng hiÕn dù tr÷.", pMem->m_szName, m_OldWeekGoal.nTWeeGoalPrice);
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
}

void	CTongControl::PlayerReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_PLAYERPRICE_SYNC sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_TONG_PLAYERPRICE_RET;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nValue;
	sSync.m_nPrice = 0;
	sSync.m_btSuccessFlag = 1;
	
	STONG_MEMBER* pMem = FindMember(sSync.m_dwPlayerNameID);
	if(!pMem)
		return;
	if(!m_OldWeekGoal.wWeekGoalType || !m_OldWeekGoal.nMWeekGoal)
	{
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	if(pMem->m_bGetPrice)
	{
		sSync.m_btSuccessFlag = 2;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	if(pMem->m_nOldWGCompleted == 0)
	{
		sSync.m_btSuccessFlag = 3;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	if(pMem->m_nOldWGCompleted < m_OldWeekGoal.nMWeekGoal)
	{
		sSync.m_btSuccessFlag = 4;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	pMem->m_bGetPrice = 1;
	g_cTongSet.SaveMember(m_szName, *pMem);
	sSync.m_btSuccessFlag = 0;
	sSync.m_nPrice = m_OldWeekGoal.nMWeeGoalPrice;
	pConn->SendPackage((const void *)&sSync, sizeof(sSync));
}

void	CTongControl::ChangeTitle(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	if(pInfo->m_nDestFigure >= enumTONG_FIGURE_MASTER)
		return;
	vector<STONG_MEMBER>::iterator it;
	STONG_MEMBER sMember;
	sMember.m_dwNameID = pInfo->m_uDestID;
	if(pInfo->m_nDestFigure == enumTONG_FIGURE_MEMBER)
	{
		it = std::find(m_Member.begin(), m_Member.end(), sMember);
		if(it == m_Member.end())
			return;
	}
	else if(pInfo->m_nDestFigure == enumTONG_FIGURE_MANAGER)
	{
		it = std::find(m_Manager.begin(), m_Manager.end(), sMember);
		if(it == m_Manager.end())
			return;
	}
	else
	{
		it = std::find(m_Director.begin(), m_Director.end(), sMember);
		if(it == m_Director.end())
			return;
	}
	strcpy(it->m_szTitle, pInfo->m_szName);
	g_cTongSet.SaveMember(m_szName, *it);
	STONG_MEMBER* pMem = FindMember(pInfo->m_dwPlayerNameID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		sprintf(szMsg, "%s ®æi danh hiÖu %s lµ %s", pMem->m_szName, it->m_szName, it->m_szTitle);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg);
		//sync cho sender
		STONG_CHANGETITLE_SYNC sSync;
		sSync.ProtocolFamily = pf_tong;
		sSync.ProtocolID = enumS2C_TONG_CHANGETITLE_UI;
		sSync.m_dwTongNameID = m_dwNameID;
		sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
		sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
		sSync.m_uDestID = pInfo->m_uDestID;
		strcpy(sSync.m_szName, pInfo->m_szName);
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
	}
	//sync cho nguoi duoc doi~ ten neu co
	CNetConnectDup conndup;
	UINT uNetID;
	if (g_HostServer.FindPlayerByID(pInfo->m_uDestID, NULL, &conndup, NULL, &uNetID))
	{
		CNetConnectDup tongconndup = g_TongServer.FindTongConnectByIP(conndup.GetIP());
		if (tongconndup.IsValid())
		{
			STONG_CHANGETITLE_SYNC	sSync;
			sSync.ProtocolFamily = pf_tong;
			sSync.ProtocolID = enumS2C_TONG_BE_CHANGETITLE;
			sSync.m_dwPlayerNameID = pInfo->m_uDestID;
			sSync.m_nPlayerIdx = uNetID;
			sSync.m_dwTongNameID = m_dwNameID;
			sSync.m_uDestID = 0;
			strcpy(sSync.m_szName, pInfo->m_szName);
			tongconndup.SendPackage((const void *)&sSync, sizeof(sSync));
		}
	}
}

void	CTongControl::ChangeTitleAll(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_CHANGETITLE_SYNC	sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_TONG_CHANGETITLEALL;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_uDestID = -1;
	strcpy(sSync.m_szName, pInfo->m_szName);
	UINT uCurTimeMin = g_GetCurDateMin();
	if(m_uChangeTitleAllNextMin >= uCurTimeMin)
	{
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	m_uChangeTitleAllNextMin = uCurTimeMin + 5;
	int i;
	for(i=0;i<(int)m_Manager.size();++i)
	{
		if(pInfo->m_nDestFigure == m_Manager[i].m_nSex)
		{
			strcpy(m_Manager[i].m_szTitle, pInfo->m_szName);
			g_cTongSet.SaveMember(m_szName, m_Manager[i]);
		}
	}
	for(i=0;i<(int)m_Member.size();++i)
	{
		if(pInfo->m_nDestFigure == m_Member[i].m_nSex)
		{
			strcpy(m_Member[i].m_szTitle, pInfo->m_szName);
			g_cTongSet.SaveMember(m_szName, m_Member[i]);
		}
	}
	STONG_MEMBER* pMem = FindMember(pInfo->m_dwPlayerNameID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if(pInfo->m_nDestFigure)
		sprintf(szMsg, "%s ®æi danh hiÖu thµnh viªn n÷ lµ %s", pMem->m_szName, pInfo->m_szName);
		else
		sprintf(szMsg, "%s ®æi danh hiÖu thµnh viªn nam lµ %s", pMem->m_szName, pInfo->m_szName);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg);
	}
	sSync.m_uDestID = pInfo->m_nDestFigure;
	g_TongServer.BroadPackage((const void*)&sSync, sizeof(sSync));
}

void	CTongControl::ChangeCamp(STONG_CHANGECAMP_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	STONG_CHANGECAMP_SYNC	sSync;
	sSync.ProtocolFamily = pf_tong;
	sSync.ProtocolID = enumS2C_TONG_CHANGECAMP;
	sSync.m_dwPlayerNameID = pInfo->m_dwPlayerNameID;
	sSync.m_nPlayerIdx = pInfo->m_nPlayerIdx;
	sSync.m_dwTongNameID = m_dwNameID;
	sSync.m_nCamp = -1;
	sSync.m_nMoney = 0;
	UINT uCurTimeMin = g_GetCurDateMin();
	if(m_uChangeCampNextMin >= uCurTimeMin)
	{
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	if(m_nMoney < defTONG_MONEY2CHANGECAMP)
	{
		sSync.m_nCamp = 0;
		sSync.m_nMoney = defTONG_MONEY2CHANGECAMP;
		pConn->SendPackage((const void *)&sSync, sizeof(sSync));
		return;
	}
	m_uChangeCampNextMin = uCurTimeMin + 5;
	m_nCamp = pInfo->m_btCamp;
	if(m_nCamp < camp_justice || m_nCamp > camp_balance)
		m_nCamp = camp_justice;
	m_nMoney -= defTONG_MONEY2CHANGECAMP;
	STONG_MEMBER* pMem = FindMember(pInfo->m_dwPlayerNameID);
	if(pMem)
	{
		char szMsg[128];
		sprintf(szMsg, "\\O%u", m_dwNameID);
		DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
		if(m_nCamp == camp_justice)
		sprintf(szMsg, "%s thay ®æi phe ph¸i thµnh ChÝnh ph¸i", pMem->m_szName);
		else if(m_nCamp == camp_evil)
		sprintf(szMsg, "%s thay ®æi phe ph¸i thµnh Tµ ph¸i", pMem->m_szName);
		else
		sprintf(szMsg, "%s thay ®æi phe ph¸i thµnh Trung lËp", pMem->m_szName);
		if (channid != -1)
		{
			g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
		}
		AddAffairMsg(szMsg, true, false);
	}
	g_cTongDB.ChangeTong(*this);
	sSync.m_nCamp = m_nCamp;
	sSync.m_nMoney = m_nMoney;
	g_TongServer.BroadPackage((const void*)&sSync, sizeof(sSync));
}
