// KTongSet.cpp: implementation of the CTongSet class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "S3Relay.h"
#include "Global.h"
#include "TongDB.h"
#include "KTongSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTongSet::CTongSet()
{
	m_pcTong = NULL;
	m_nTongPointSize = 0;
	Init();
}

CTongSet::~CTongSet()
{
	AUTOLOCKWRITE(m_lockTArray);
	DeleteAll();
}

void	CTongSet::Init()
{
	AUTOLOCKWRITE(m_lockTArray);
	DeleteAll();

	m_pcTong = (CTongControl**)new LPVOID[defTONG_SET_INIT_POINT_NUM];
	m_nTongPointSize = defTONG_SET_INIT_POINT_NUM;
	for (int i = 0; i < m_nTongPointSize; i++)
	{
		m_pcTong[i] = NULL;
	}
}

void	CTongSet::DeleteAll()
{
	if (m_pcTong)
	{
		for (int i = 0; i < m_nTongPointSize; i++)
		{
			if (m_pcTong[i])
			{
				delete m_pcTong[i];
				m_pcTong[i] = NULL;
			}
		}
		delete [] m_pcTong;
		m_pcTong = NULL;
	}
	m_nTongPointSize = 0;
}

BOOL	CTongSet::InitFromDB()
{
	InitTScript();
	int nTongNum = g_cTongDB.GetTongCount();
	if (nTongNum < 0)
		return FALSE;
	if (nTongNum == 0)
		return TRUE;
	AUTOLOCKWRITE(m_lockTArray);
	int i;
	if (nTongNum > m_nTongPointSize)
	{
		DeleteAll();
		m_pcTong = (CTongControl**)new LPVOID[nTongNum + defTONG_SET_INIT_POINT_NUM];
		m_nTongPointSize = nTongNum + defTONG_SET_INIT_POINT_NUM;
		for ( i = 0; i < m_nTongPointSize; ++i)
		{
			m_pcTong[i] = NULL;
		}
	}

	TTongList	*pList = new TTongList[nTongNum];
	//memset(pList, 0, sizeof(TTongList) * nTongNum);
	int nGetNum = g_cTongDB.GetTongList(pList, nTongNum);
	for (i = 0; i < nGetNum; ++i)
	{
		m_pcTong[i] = new CTongControl(pList[i]);
		g_cTongDB.SearchTong(m_pcTong[i]->m_szName, *(m_pcTong[i]));
	}

	delete [] pList;
	pList = NULL;
	//sap xep level bang
	if(nGetNum > 0)
	{
		CTongControl** pTSort = (CTongControl**)new LPVOID[nGetNum];
		for (i = 0; i < nGetNum; ++i)
		{
			pTSort[i] = NULL;
		}
		for (i = 0; i < nGetNum; ++i)
		{
			for(int n = 0; n < nGetNum; ++n)
			{
				if(!pTSort[n])
				{
					pTSort[n] = m_pcTong[i];
					break;
				}
				if((m_pcTong[i]->GetLevel() > pTSort[n]->GetLevel())
				|| (m_pcTong[i]->GetLevel() == pTSort[n]->GetLevel()
				&& m_pcTong[i]->GetExp() > pTSort[n]->GetExp()))
				{
					memmove(&pTSort[n+1], &pTSort[n], sizeof(CTongControl*)*(nGetNum-n-1));
					pTSort[n] = m_pcTong[i];
					break;
				}
			}
		}
		for (i = 0; i < nGetNum; ++i)
		{
			m_pcTong[i] = pTSort[i];
		}
		delete [] pTSort;
	}
	return TRUE;
}

void	CTongSet::SaveMember(const char* szTongName, STONG_MEMBER& sCtrlMem)
{
	TMemberStruct_V1	sMember;
	strcpy(sMember.m_szName, sCtrlMem.m_szName);
	strcpy(sMember.szTong, szTongName);
	strcpy(sMember.m_szTitle, sCtrlMem.m_szTitle);
	sMember.m_nFigure = sCtrlMem.m_nFigure;
	sMember.m_uJoinDate = sCtrlMem.m_uJoinDate;
	sMember.m_uOnlineDate = sCtrlMem.m_uOnlineDate;
	sMember.m_uRight = sCtrlMem.m_uRight;
	sMember.m_nTotalOffer = sCtrlMem.m_nTotalOffer;
	sMember.m_nWeekOffer = sCtrlMem.m_nWeekOffer;
	sMember.m_nOldWGCompleted = sCtrlMem.m_nOldWGCompleted;
	sMember.m_nNewWGCompleted = sCtrlMem.m_nNewWGCompleted;
	sMember.m_bWGType = sCtrlMem.m_bWGType;
	sMember.m_bGetPrice = sCtrlMem.m_bGetPrice;
	sMember.m_bRetired = sCtrlMem.m_bRetired;
	sMember.m_nSex = sCtrlMem.m_nSex;
	
	g_cTongDB.ChangeMember(sMember);
}

int		CTongSet::Create(int nCamp, char *lpszPlayerName, char *lpszTongName, BYTE nSex)
{
	AUTOLOCKWRITE(m_lockTArray);
	int nRet = 0xff;
	if (!m_pcTong || m_nTongPointSize <= 0) // khong tao duoc list bang trong'
		return nRet;
	if (!lpszPlayerName || !lpszTongName)
		return nRet;
	if (strlen(lpszTongName) >= defTONG_STR_LENGTH)
		return nRet;
	UINT dwTongNameID = g_String2Id(lpszTongName);
	if(dwTongNameID == 0)
		return nRet;
	UINT dwPlayerNameID = g_String2Id(lpszPlayerName);
	int i;
	// tim xem bang co san~ hay khong
	for (i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID
		|| m_pcTong[i]->m_Master.m_dwNameID == dwPlayerNameID))
			break;
	}
	// bang da ton tai
	if (i < m_nTongPointSize)
		return 1;

	// tim slot bang con trong'
	int nPos = -1;
	for (i = 0; i < m_nTongPointSize; ++i)
	{
		if (!m_pcTong[i])
		{
			nPos = i;
			break;
		}
	}
	// het slot trong, mo~ rong ra them
	if (nPos < 0)
	{
		CTongControl** pTemp = (CTongControl**)new LPVOID[m_nTongPointSize];
		for (i = 0; i < m_nTongPointSize; ++i)
			pTemp[i] = m_pcTong[i];
		delete [] m_pcTong;
		m_pcTong = NULL;
		m_pcTong = (CTongControl**)new LPVOID[m_nTongPointSize * 2];
		for (i = 0; i < m_nTongPointSize; ++i)
			m_pcTong[i] = pTemp[i];
		delete [] pTemp;
		pTemp = NULL;
		m_nTongPointSize *= 2;
		for (i = m_nTongPointSize / 2; i < m_nTongPointSize; ++i)
			m_pcTong[i] = NULL;
		nPos = m_nTongPointSize / 2;
	}
	// khoi~ tao. bang
	m_pcTong[nPos] = new CTongControl(nCamp, lpszPlayerName, lpszTongName, nSex);
	// save database
	g_cTongDB.ChangeTong(*m_pcTong[nPos]);
	SaveMember(lpszTongName, m_pcTong[nPos]->m_Master);
	return 0;
}

//----------------------------------------------------------------------
//	¹¦ÄÜ£ºÌí¼ÓÒ»¸ö°ï»á³ÉÔ±£¬if return == 0 ³É¹¦ else return error id
//----------------------------------------------------------------------
int		CTongSet::AddMember(char *lpszPlayerName, UINT dwTongNameID, char *lpszTongName, BYTE btFigure, BYTE nSex, BOOL& bForce)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return -1;
	if (!lpszPlayerName || !lpszPlayerName[0])
		return -1;

	int		i;
	// tim bang
	for (i = 0; i < m_nTongPointSize; ++i)
	{
		// add mem vao
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			if (!m_pcTong[i]->AddMember(lpszPlayerName, lpszTongName, btFigure, nSex, bForce))
				return -1;
			else
			{
				// thong bao kenh bang
				char	szMsg[96];
				sprintf(szMsg, "\\O%u", m_pcTong[i]->m_dwNameID);

				DWORD channid = g_ChannelMgr.GetChannelID(szMsg, 0);
				if (channid != -1)
				{
					sprintf(szMsg, "%s gia nhËp bæn bang", lpszPlayerName);
					g_ChannelMgr.SayOnChannel(channid, TRUE, std::string(), std::string("C«ng bè"), std::string(szMsg));
				}

				return i;
			}
		}
	}

	return -1;
}

//----------------------------------------------------------------------
//	lay phe cua bang
//----------------------------------------------------------------------
int		CTongSet::GetTongCamp(int nTongIdx)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return -1;
	if (nTongIdx < 0 || nTongIdx >= m_nTongPointSize)
		return -1;

	if (m_pcTong[nTongIdx])
		return m_pcTong[nTongIdx]->m_nCamp;

	return -1;
}

BOOL	CTongSet::GetMasterName(int nTongIdx, char *lpszName)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!lpszName)
		return FALSE;
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	if (nTongIdx < 0 || nTongIdx >= m_nTongPointSize)
		return FALSE;
	if (!m_pcTong[nTongIdx])
		return FALSE;
	strcpy(lpszName, m_pcTong[nTongIdx]->m_Master.m_szName);
	return TRUE;
}

BOOL	CTongSet::GetTongHeadInfo(UINT dwTongNameID, STONG_HEAD_INFO_SYNC *pInfo)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0 || dwTongNameID == 0)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == dwTongNameID)
		{
			return m_pcTong[i]->GetTongHeadInfo(pInfo);
		}
	}

	return FALSE;
}

BOOL	CTongSet::GetTongMemberInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, STONG_MEMBER_INFO_SYNC *pInfo, CTongConnect* pConn)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pApply || !pInfo)
		return FALSE;
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pApply->m_dwTongNameID)
		{
			return m_pcTong[i]->GetTongMemberInfo(pApply, pInfo, pConn);
		}
	}

	return FALSE;
}

BOOL	CTongSet::GetTongPageInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	if (!pApply || !pConn)
		return FALSE;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;

	STONG_TONGPAGE_INFO_SYNC sInfo;
	sInfo.ProtocolFamily	= pf_tong;
	sInfo.ProtocolID		= enumS2C_TONG_PAGE_INFO;
	sInfo.m_dwPID			= pApply->m_dwPID;
	sInfo.m_dwParam			= pApply->m_dwParam;
	sInfo.m_bBegin			= 1; //xoa memlist ui
	sInfo.m_nMemNum			= 0;
	int nTCount = 0;
	int i;
	for (i = 0; i < m_nTongPointSize; ++i)
	{
		if (!m_pcTong[i])
			break;
		++nTCount;
	}
	if(nTCount == 0)
	{
		sInfo.m_nPage = 0;
		sInfo.m_wLength = sizeof(STONG_TONGPAGE_INFO_SYNC)
			- sizeof(STONG_PAGEMEM)*defTONG_MAX_PAGEINFOSYNC;
		pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
		return TRUE;
	}
	int nTargetPage = pApply->m_nPage;
	if(nTargetPage < 0)
		nTargetPage = 0;
	int nPageNum = nTCount/defTONG_ONE_PAGE_MAX_NUM;
	if(nTCount % defTONG_ONE_PAGE_MAX_NUM)
		++nPageNum;
	if(nTargetPage >= nPageNum)
		nTargetPage = nPageNum - 1;
	sInfo.m_nPage = nTargetPage;
	int nMemNum = nTargetPage*defTONG_ONE_PAGE_MAX_NUM;
	nPageNum = defTONG_ONE_PAGE_MAX_NUM;
	int nOffset = 0;
	for (i = 0; i < nTCount; ++i)
	{
		if(nOffset >= nMemNum)
		{
			strcpy(sInfo.sMem[sInfo.m_nMemNum].m_szName, m_pcTong[i]->GetName());
			sInfo.sMem[sInfo.m_nMemNum].m_nLevel = m_pcTong[i]->GetLevel();
			++sInfo.m_nMemNum;
			if(sInfo.m_nMemNum >= defTONG_MAX_PAGEINFOSYNC)
			{
				if(nPageNum < defTONG_MAX_PAGEINFOSYNC)
					sInfo.m_nMemNum = nPageNum;
				sInfo.m_wLength = sizeof(STONG_TONGPAGE_INFO_SYNC)
					- sizeof(STONG_PAGEMEM)*defTONG_MAX_PAGEINFOSYNC
					+ sizeof(STONG_PAGEMEM)*sInfo.m_nMemNum;
				pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
				nPageNum -= sInfo.m_nMemNum;
				sInfo.m_nMemNum = 0;
				sInfo.m_bBegin = 0;
				if(nPageNum <= 0)
					return TRUE;
			}
		}
		else
			++nOffset;
	}
	if(sInfo.m_nMemNum > 0 && nPageNum > 0)
	{
		if(sInfo.m_nMemNum > nPageNum)
			sInfo.m_nMemNum = nPageNum;
		sInfo.m_wLength = sizeof(STONG_TONGPAGE_INFO_SYNC)
			- sizeof(STONG_PAGEMEM)*defTONG_MAX_PAGEINFOSYNC
			+ sizeof(STONG_PAGEMEM)*sInfo.m_nMemNum;
		pConn->SendPackage((const void *)&sInfo, sInfo.m_wLength);
	}
	return FALSE;
}

BOOL	CTongSet::GetRecordInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pApply || !pConn)
		return FALSE;
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pApply->m_dwTongNameID)
		{
			if(pApply->m_nPage == 0) //muc tieu tuan
			{
				return m_pcTong[i]->GetRecordWeekTask(pApply, pConn);
			}
			else if(pApply->m_nPage == 1) //cong cao'
			{
				return m_pcTong[i]->GetRecordAnnounce(pApply, pConn);
			}
			else if(pApply->m_nPage == 2) //su kien
			{
				return m_pcTong[i]->GetRecordAffair(pApply, pConn);
			}
			else	//lich su
			{
				return m_pcTong[i]->GetRecordHistory(pApply, pConn);
			}
		}
	}

	return FALSE;
}

BOOL	CTongSet::Instate(STONG_INSTATE_COMMAND *pInstate, STONG_INSTATE_SYNC *pSync)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pInstate || !pSync)
		return FALSE;
	if (!m_pcTong)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; i++)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pInstate->m_dwTongNameID)
		{
			return m_pcTong[i]->Instate(pInstate, pSync);
		}
	}

	return FALSE;
}

BOOL	CTongSet::Kick(STONG_KICK_COMMAND *pKick, STONG_KICK_SYNC *pSync)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pKick || !pSync)
		return FALSE;
	if (!m_pcTong)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; i++)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pKick->m_dwTongNameID)
		{
			return m_pcTong[i]->Kick(pKick, pSync);
		}
	}

	return FALSE;
}

BOOL	CTongSet::Leave(STONG_LEAVE_COMMAND *pLeave, STONG_LEAVE_SYNC *pSync)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pLeave || !pSync)
		return FALSE;
	if (!m_pcTong)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; i++)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pLeave->m_dwTongNameID)
		{
			return m_pcTong[i]->Leave(pLeave, pSync);
		}
	}

	return FALSE;
}

BOOL	CTongSet::AcceptMaster(STONG_ACCEPT_MASTER_COMMAND *pAccept, CTongConnect* pConn)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pAccept)
		return FALSE;
	if (!m_pcTong)
		return FALSE;

	for (int i = 0; i < m_nTongPointSize; i++)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pAccept->m_dwTongNameID)
		{
			return m_pcTong[i]->AcceptMaster(pAccept, pConn);
		}
	}

	return FALSE;
}

BOOL	CTongSet::GetLoginData(STONG_GET_LOGIN_DATA_COMMAND *pLogin, STONG_LOGIN_DATA_SYNC *pSync)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!pLogin || !pSync)
		return FALSE;

	pSync->ProtocolFamily	= pf_tong;
	pSync->ProtocolID		= enumS2C_TONG_LOGIN_DATA;
	pSync->m_btFlag			= 0;
	pSync->m_dwParam		= pLogin->m_dwParam;

	if (!m_pcTong)
		return TRUE;

	// Ñ°ÕÒ°ï»á
	for (int i = 0; i < m_nTongPointSize; i++)
	{
		if (m_pcTong[i] && m_pcTong[i]->m_dwNameID == pLogin->m_dwTongNameID)
		{
			return m_pcTong[i]->GetLoginData(pLogin, pSync);
		}
	}

	return TRUE;
}

BOOL	CTongSet::DistribRight(UINT dwTongNameID, UINT uSenderID, UINT uDestID, UINT& uRight)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->DistribRight(uSenderID, uDestID, uRight);
		}
	}
	return FALSE;
}

BOOL	CTongSet::ChangeRecruit(UINT dwTongNameID, UINT uSenderID, int& nLockRecruit)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->ChangeRecruit(uSenderID, nLockRecruit);
		}
	}
	return FALSE;
}

int	CTongSet::Update(int nTongIdx)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return -1;
	if(nTongIdx >= m_nTongPointSize)
		return -1;
	if (!m_pcTong[nTongIdx])
		return -1;
	if(!m_pcTong[nTongIdx]->Update()) //FALSE is Delete
	{
		delete m_pcTong[nTongIdx];
		m_pcTong[nTongIdx] = NULL;
		for (int i = nTongIdx; i < m_nTongPointSize-1; ++i)
			m_pcTong[i] = m_pcTong[i+1];
		m_pcTong[m_nTongPointSize-1] = NULL;
		--nTongIdx;
	}
	return (nTongIdx+1);
}

BOOL	CTongSet::AddMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->AddMoney(uSenderID, nMoney);
		}
	}
	return FALSE;
}

BOOL	CTongSet::WithDrawMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->WithDrawMoney(uSenderID, nMoney);
		}
	}
	return FALSE;
}

BOOL	CTongSet::AddOffer(UINT dwTongNameID, UINT uSenderID, int& nOffer)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->AddOffer(uSenderID, nOffer);
		}
	}
	return FALSE;
}

BOOL	CTongSet::DispenseOffer(UINT dwTongNameID, UINT uSenderID, UINT uDestID, int& nOffer)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->DispenseOffer(uSenderID, uDestID, nOffer);
		}
	}
	return FALSE;
}

void	CTongSet::AssignMoney(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->AssignMoney(pInfo, pConn);
			return;
		}
	}
}

void	CTongSet::AssignOffer(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->AssignOffer(pInfo, pConn);
			return;
		}
	}
}

BOOL	CTongSet::TransMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney, int& nBuildFund)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			return m_pcTong[i]->TransMoney(uSenderID, nMoney, nBuildFund);
		}
	}
	return FALSE;
}

BOOL	CTongSet::AddBuildFund(STONG_CONTRIBMONEY_COMMAND	*pInfo, CTongConnect* pConn)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			return m_pcTong[i]->AddBuildFund(pInfo, pConn);
		}
	}
	return FALSE;
}

BOOL	CTongSet::SetAnnounce(STONG_ANNOUNCE_COMMAND* pInfo)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return FALSE;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			return m_pcTong[i]->SetAnnounce(pInfo);
		}
	}
	return FALSE;
}

void	CTongSet::ResetTask(UINT dwTongNameID)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			m_pcTong[i]->ResetTask(true);
			return;
		}
	}
}

void	CTongSet::AddWeekGoal(UINT dwTongNameID, UINT dwPlayerNameID, int nValue)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			m_pcTong[i]->AddWeekGoal(dwPlayerNameID, nValue);
			return;
		}
	}
}

void	CTongSet::SetTaskLevel(UINT dwTongNameID, UINT dwPlayerNameID, int nValue)
{
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == dwTongNameID))
		{
			m_pcTong[i]->SetTaskLevel(dwPlayerNameID, nValue);
			return;
		}
	}
}

void	CTongSet::ReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->ReceivePrice(pInfo, pConn);
			return;
		}
	}
}

void	CTongSet::PlayerReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->PlayerReceivePrice(pInfo, pConn);
			return;
		}
	}
}

void	CTongSet::ChangeTitle(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->ChangeTitle(pInfo, pConn);
			return;
		}
	}
}

void	CTongSet::ChangeTitleAll(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->ChangeTitleAll(pInfo, pConn);
			return;
		}
	}
}

void	CTongSet::ChangeCamp(STONG_CHANGECAMP_COMMAND *pInfo, CTongConnect* pConn)
{
	if(!pInfo || !pConn)
		return;
	AUTOLOCKWRITE(m_lockTArray);
	if (!m_pcTong || m_nTongPointSize <= 0)
		return;
	for (int i = 0; i < m_nTongPointSize; ++i)
	{
		if (m_pcTong[i] && (m_pcTong[i]->m_dwNameID == pInfo->m_dwTongNameID))
		{
			m_pcTong[i]->ChangeCamp(pInfo, pConn);
			return;
		}
	}
}
