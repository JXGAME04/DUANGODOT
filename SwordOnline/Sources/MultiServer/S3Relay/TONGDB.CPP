// TongDB.cpp: implementation of the CTongDB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Global.h"
#include "db.h"
#include "DBTable.h"
#include "TongDB.h"
//#include "S3Relay.h" //debug rTRACE

CTongDB		g_cTongDB;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int get_TongName(DB *db, const DBT *pkey, const DBT *pdata, DBT *ikey) 
{
	memset( ikey, 0, sizeof( DBT ) );
	TMemberStruct_V1* pMemberStruct = (TMemberStruct_V1*)pdata->data;
	ikey->data = pMemberStruct->szTong;
	ikey->size = strlen( pMemberStruct->szTong ) + 1;
	return 0;
}

CTongDB::CTongDB()
{//初始化变量
	m_TongListCount = 0;
	memset(m_TongList,0,sizeof(TTongList) * MAX_TONG_COUNT);
}

bool CTongDB::Open()
{
	m_TongTable = new ZDBTable("TongDB","TongTable");
	if(!m_TongTable->open())return false;

	m_MemberTable = new ZDBTable("TongMemberDB","MemberTable");
	m_MemberTable->addIndex(get_TongName);
	if(!m_MemberTable->open())
	{
		m_TongTable->close();
		return false;
	}
	return true;
}
void CTongDB::Close()
{
	m_TongTable->close();
	m_MemberTable->close();
	delete m_TongTable;
	delete m_MemberTable;
}

char g_szBuffer[32000];
bool CTongDB::ChangeTong(CTongControl& aTongControl)
{//save database thong tin bang
	bool aResult = true;
	int aKeySize = strlen(aTongControl.m_szName) + 1;
	UINT aSize = sizeof(TTongStruct_V1);
	TTongStruct_V1* aTongStruct = (TTongStruct_V1*)&g_szBuffer;
	strcpy(aTongStruct->m_szName, aTongControl.m_szName);
	aTongStruct->m_nCamp = aTongControl.m_nCamp;
	aTongStruct->m_nMoney = aTongControl.m_nMoney;
	aTongStruct->m_nBuildMoney = aTongControl.m_nBuildMoney;
	aTongStruct->m_nLevel = aTongControl.m_nLevel;
	aTongStruct->m_nBuilLevel = aTongControl.m_nBuilLevel;
	aTongStruct->m_nOffer = aTongControl.m_nOffer;
	aTongStruct->m_uCreateDate = aTongControl.m_uCreateDate;
	aTongStruct->m_uWeekCount = aTongControl.m_uWeekCount;
	aTongStruct->m_nExp = aTongControl.m_nExp;
	aTongStruct->m_bLockRecruit = aTongControl.m_bLockRecruit;
	aTongStruct->m_bOrdeal = aTongControl.m_bOrdeal;
	aTongStruct->m_nTaskLevel = aTongControl.m_nTaskLevel;
	aTongStruct->m_uUnionID = aTongControl.m_uUnionID;
	aTongStruct->m_uPromoteID = aTongControl.m_uPromoteID;
	aTongStruct->m_OldWeekGoal = aTongControl.m_OldWeekGoal;
	aTongStruct->m_NewWeekGoal = aTongControl.m_NewWeekGoal;
	strcpy(aTongStruct->m_szAnnounce, aTongControl.m_szAnnounce);
	char* pNextData = (char*)(aTongStruct + 1);
	aTongStruct->m_uAffairCount = aTongControl.m_AffairMsg.size();
	if(aTongStruct->m_uAffairCount)
	{
		TTongMsg* pMsg = (TTongMsg*)pNextData;
		for(UINT i=0;i<aTongStruct->m_uAffairCount;++i,++pMsg)
		{
			strcpy(pMsg->szMsg, aTongControl.m_AffairMsg[i].c_str());
		}
		pNextData += sizeof(TTongMsg)*aTongStruct->m_uAffairCount;
		aSize += sizeof(TTongMsg)*aTongStruct->m_uAffairCount;
	}
	aTongStruct->m_uHistoryCount = aTongControl.m_HistoryMsg.size();
	if(aTongStruct->m_uHistoryCount)
	{
		TTongMsg* pMsg = (TTongMsg*)pNextData;
		for(UINT i=0;i<aTongStruct->m_uHistoryCount;++i,++pMsg)
		{
			strcpy(pMsg->szMsg, aTongControl.m_HistoryMsg[i].c_str());
		}
		pNextData += sizeof(TTongMsg)*aTongStruct->m_uHistoryCount;
		aSize += sizeof(TTongMsg)*aTongStruct->m_uHistoryCount;
	}
	aTongStruct->m_uWorkshopCount = 0;
	
	aResult = m_TongTable->add(aTongControl.m_szName, aKeySize,(char*)aTongStruct, aSize);

	return aResult;
}

bool CTongDB::DelTong(char* aTongName)
{
	int aKeySize = strlen(aTongName) + 1;
	return m_TongTable->remove(aTongName, aKeySize);
}

bool CTongDB::SearchTong(char* aTongName, CTongControl& aTongControl)
{//fill full thong tin bang va member
	bool aResult = true;
	int aSize;
	int aKeySize = strlen(aTongName) + 1;
	char* aBuffer = m_TongTable->search(aTongName, aKeySize, aSize);
	if(!aBuffer) return false;
	UINT uCurDate = g_GetCurDateMin();
	bool bOldVersion = false;
	if(aSize == sizeof(TTongStruct)) //version cu~
	{
		bOldVersion = true;
		TTongStruct* pTongStruct = (TTongStruct*)aBuffer;
		strcpy(aTongControl.m_szName, pTongStruct->szName);
		aTongControl.m_nCamp = pTongStruct->nCamp;
		aTongControl.m_nMoney = pTongStruct->dwMoney/10000;
		aTongControl.m_nBuildMoney = 0;
		aTongControl.m_nLevel = pTongStruct->nLevel;
		aTongControl.m_nBuilLevel = 0;
		aTongControl.m_nOffer = 0;
		aTongControl.m_uCreateDate = uCurDate;
		aTongControl.m_uWeekCount = 1;
		aTongControl.m_nExp = 0;
		aTongControl.m_bLockRecruit = 0;
		aTongControl.m_bOrdeal = 0;
		aTongControl.m_nTaskLevel = 0;
		aTongControl.m_uUnionID = 0;
		aTongControl.m_uPromoteID = 0;
	}
	else
	{//new version
		TTongStruct_V1* pTongStruct = (TTongStruct_V1*)aBuffer;
		strcpy(aTongControl.m_szName, pTongStruct->m_szName);
		aTongControl.m_nCamp = pTongStruct->m_nCamp;
		aTongControl.m_nMoney = pTongStruct->m_nMoney;
		aTongControl.m_nBuildMoney = pTongStruct->m_nBuildMoney;
		aTongControl.m_nLevel = pTongStruct->m_nLevel;
		aTongControl.m_nBuilLevel = pTongStruct->m_nBuilLevel;
		aTongControl.m_nOffer = pTongStruct->m_nOffer;
		aTongControl.m_uCreateDate = pTongStruct->m_uCreateDate;
		aTongControl.m_uWeekCount = pTongStruct->m_uWeekCount;
		aTongControl.m_nExp = pTongStruct->m_nExp;
		aTongControl.m_bLockRecruit = pTongStruct->m_bLockRecruit;
		aTongControl.m_bOrdeal = pTongStruct->m_bOrdeal;
		aTongControl.m_nTaskLevel = pTongStruct->m_nTaskLevel;
		aTongControl.m_uUnionID = pTongStruct->m_uUnionID;
		aTongControl.m_uPromoteID = pTongStruct->m_uPromoteID;
		aTongControl.m_OldWeekGoal = pTongStruct->m_OldWeekGoal;
		aTongControl.m_NewWeekGoal = pTongStruct->m_NewWeekGoal;
		strcpy(aTongControl.m_szAnnounce, pTongStruct->m_szAnnounce);
		UINT uOffset = sizeof(TTongStruct_V1);
		if(pTongStruct->m_uAffairCount)
		{
			TTongMsg* pMsg = (TTongMsg*)(aBuffer + uOffset);
			for(UINT i=0;i<pTongStruct->m_uAffairCount;++i, ++pMsg)
			{
				aTongControl.AddAffairMsg(pMsg->szMsg, false, false);
			}
			uOffset += sizeof(TTongMsg)*pTongStruct->m_uAffairCount;
		}
		if(pTongStruct->m_uHistoryCount)
		{
			TTongMsg* pMsg = (TTongMsg*)(aBuffer + uOffset);
			for(UINT i=0;i<pTongStruct->m_uHistoryCount;++i, ++pMsg)
			{
				aTongControl.AddHistoryMsg(pMsg->szMsg, false, false);
			}
			uOffset += sizeof(TTongMsg)*pTongStruct->m_uHistoryCount;
		}
		if(pTongStruct->m_uWorkshopCount)
		{
			
		}
	}
	delete [] aBuffer;
	STONG_MEMBER sMember;
	aBuffer = m_MemberTable->search(aTongName, aKeySize, aSize, 0);
	if(bOldVersion) //version cu~
	{
		while(aBuffer)
		{
			TMemberStruct* pMemberStruct = (TMemberStruct*)aBuffer;
			switch(pMemberStruct->MemberClass)
			{
			case enumTONG_FIGURE_MASTER:
				{
				strcpy(aTongControl.m_Master.m_szName, pMemberStruct->szName);
				aTongControl.m_Master.m_dwNameID = g_String2Id(pMemberStruct->szName);
				strcpy(aTongControl.m_Master.m_szTitle, defTITLE_MASTER);
				aTongControl.m_Master.m_nFigure = enumTONG_FIGURE_MASTER;
				aTongControl.m_Master.m_uJoinDate = uCurDate;
				aTongControl.m_Master.m_uOnlineDate = uCurDate;
				aTongControl.m_Master.m_uRight = defRIGHT_FULL;
				char szMsg[128];
				UINT y, mt, d, h, m;
				g_GetCurDate(y, mt, d, h, m);
				sprintf(szMsg, "%02u-%02u-%04u %s t筼 l藀 bang h閕 %s", d,mt,y, aTongControl.m_Master.m_szName, aTongControl.m_szName);
				aTongControl.m_HistoryMsg.push_back(string(szMsg));
				}
				break;
				
			case enumTONG_FIGURE_DIRECTOR:
				strcpy(sMember.m_szName, pMemberStruct->szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->szName);
				strcpy(sMember.m_szTitle, defTITLE_DIRECT);
				sMember.m_nFigure = enumTONG_FIGURE_DIRECTOR;
				sMember.m_uJoinDate = uCurDate;
				sMember.m_uOnlineDate = uCurDate;
				aTongControl.m_Director.push_back(sMember);
				break;
				
			case enumTONG_FIGURE_MANAGER:
				strcpy(sMember.m_szName, pMemberStruct->szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->szName);
				strcpy(sMember.m_szTitle, defTITLE_MANAGER);
				sMember.m_nFigure = enumTONG_FIGURE_MANAGER;
				sMember.m_uJoinDate = uCurDate;
				sMember.m_uOnlineDate = uCurDate;
				aTongControl.m_Manager.push_back(sMember);
				break;
				
			case enumTONG_FIGURE_MEMBER:
				strcpy(sMember.m_szName, pMemberStruct->szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->szName);
				strcpy(sMember.m_szTitle, defTITLE_MEMBER);
				sMember.m_nFigure = enumTONG_FIGURE_MEMBER;
				sMember.m_uJoinDate = uCurDate;
				sMember.m_uOnlineDate = uCurDate;
				aTongControl.m_Member.push_back(sMember);
				break;
			}
			delete [] aBuffer;
			aBuffer = m_MemberTable->next(aSize);
		}
		//tao muc tieu tuan` va save thanh` newversion
		aTongControl.ResetTask(true);
	}
	else
	{//new version
		while(aBuffer)
		{
			TMemberStruct_V1* pMemberStruct = (TMemberStruct_V1*)aBuffer;
			switch(pMemberStruct->m_nFigure)
			{
			case enumTONG_FIGURE_MASTER:
				strcpy(aTongControl.m_Master.m_szName, pMemberStruct->m_szName);
				aTongControl.m_Master.m_dwNameID = g_String2Id(pMemberStruct->m_szName);
				strcpy(aTongControl.m_Master.m_szTitle, pMemberStruct->m_szTitle);
				aTongControl.m_Master.m_nFigure = pMemberStruct->m_nFigure;
				aTongControl.m_Master.m_uJoinDate = pMemberStruct->m_uJoinDate;
				aTongControl.m_Master.m_uOnlineDate = pMemberStruct->m_uOnlineDate;
				aTongControl.m_Master.m_uRight = pMemberStruct->m_uRight;
				aTongControl.m_Master.m_nTotalOffer = pMemberStruct->m_nTotalOffer;
				aTongControl.m_Master.m_nWeekOffer = pMemberStruct->m_nWeekOffer;
				aTongControl.m_Master.m_nOldWGCompleted = pMemberStruct->m_nOldWGCompleted;
				aTongControl.m_Master.m_nNewWGCompleted = pMemberStruct->m_nNewWGCompleted;
				aTongControl.m_Master.m_bWGType = pMemberStruct->m_bWGType;
				aTongControl.m_Master.m_bGetPrice = pMemberStruct->m_bGetPrice;
				aTongControl.m_Master.m_bRetired = pMemberStruct->m_bRetired;
				aTongControl.m_Master.m_nSex = pMemberStruct->m_nSex;
				break;
				
			case enumTONG_FIGURE_DIRECTOR:
				strcpy(sMember.m_szName, pMemberStruct->m_szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->m_szName);
				strcpy(sMember.m_szTitle, pMemberStruct->m_szTitle);
				sMember.m_nFigure = pMemberStruct->m_nFigure;
				sMember.m_uJoinDate = pMemberStruct->m_uJoinDate;
				sMember.m_uOnlineDate = pMemberStruct->m_uOnlineDate;
				sMember.m_uRight = pMemberStruct->m_uRight;
				sMember.m_nTotalOffer = pMemberStruct->m_nTotalOffer;
				sMember.m_nWeekOffer = pMemberStruct->m_nWeekOffer;
				sMember.m_nOldWGCompleted = pMemberStruct->m_nOldWGCompleted;
				sMember.m_nNewWGCompleted = pMemberStruct->m_nNewWGCompleted;
				sMember.m_bWGType = pMemberStruct->m_bWGType;
				sMember.m_bGetPrice = pMemberStruct->m_bGetPrice;
				sMember.m_bRetired = pMemberStruct->m_bRetired;
				sMember.m_nSex = pMemberStruct->m_nSex;
				aTongControl.m_Director.push_back(sMember);
				break;
				
			case enumTONG_FIGURE_MANAGER:
				strcpy(sMember.m_szName, pMemberStruct->m_szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->m_szName);
				strcpy(sMember.m_szTitle, pMemberStruct->m_szTitle);
				sMember.m_nFigure = pMemberStruct->m_nFigure;
				sMember.m_uJoinDate = pMemberStruct->m_uJoinDate;
				sMember.m_uOnlineDate = pMemberStruct->m_uOnlineDate;
				sMember.m_uRight = pMemberStruct->m_uRight;
				sMember.m_nTotalOffer = pMemberStruct->m_nTotalOffer;
				sMember.m_nWeekOffer = pMemberStruct->m_nWeekOffer;
				sMember.m_nOldWGCompleted = pMemberStruct->m_nOldWGCompleted;
				sMember.m_nNewWGCompleted = pMemberStruct->m_nNewWGCompleted;
				sMember.m_bWGType = pMemberStruct->m_bWGType;
				sMember.m_bGetPrice = pMemberStruct->m_bGetPrice;
				sMember.m_bRetired = pMemberStruct->m_bRetired;
				sMember.m_nSex = pMemberStruct->m_nSex;
				aTongControl.m_Manager.push_back(sMember);
				break;
				
			case enumTONG_FIGURE_MEMBER:
				strcpy(sMember.m_szName, pMemberStruct->m_szName);
				sMember.m_dwNameID = g_String2Id(pMemberStruct->m_szName);
				strcpy(sMember.m_szTitle, pMemberStruct->m_szTitle);
				sMember.m_nFigure = pMemberStruct->m_nFigure;
				sMember.m_uJoinDate = pMemberStruct->m_uJoinDate;
				sMember.m_uOnlineDate = pMemberStruct->m_uOnlineDate;
				sMember.m_uRight = pMemberStruct->m_uRight;
				sMember.m_nTotalOffer = pMemberStruct->m_nTotalOffer;
				sMember.m_nWeekOffer = pMemberStruct->m_nWeekOffer;
				sMember.m_nOldWGCompleted = pMemberStruct->m_nOldWGCompleted;
				sMember.m_nNewWGCompleted = pMemberStruct->m_nNewWGCompleted;
				sMember.m_bWGType = pMemberStruct->m_bWGType;
				sMember.m_bGetPrice = pMemberStruct->m_bGetPrice;
				sMember.m_bRetired = pMemberStruct->m_bRetired;
				sMember.m_nSex = pMemberStruct->m_nSex;
				aTongControl.m_Member.push_back(sMember);
				break;
			}
			delete [] aBuffer;
			aBuffer = m_MemberTable->next(aSize);
		}
	}

	return aResult;
}

int CTongDB::GetTongCount()
{//lay tong so bang
	int aCount = 0;
	int aTongSize;
	int aTongKeySize;
	char aTongKeyBuffer[defTONG_STR_LENGTH];
	char* aTongBuffer = m_TongTable->GetRecordEx(aTongSize, aTongKeyBuffer, aTongKeySize, ZDBTable::cpFirst);
	while(aTongBuffer)
	{
		aCount++;
		delete [] aTongBuffer;
		aTongBuffer = m_TongTable->GetRecordEx(aTongSize, aTongKeyBuffer, aTongKeySize, ZDBTable::cpNext);
	}

	return aCount;
}

int CTongDB::GetTongList(TTongList* aTongList, int aMaxNum)
{//lay ten cua tat ca bang
	int aCount = 0;
	int aTongSize;
	int aTongKeySize;
	char aTongKeyBuffer[defTONG_STR_LENGTH];
	char* aTongBuffer = m_TongTable->GetRecordEx(aTongSize, aTongKeyBuffer, aTongKeySize, ZDBTable::cpFirst);
	if(aTongBuffer && aTongSize == sizeof(TTongStruct)) //version cu~
	{
		while(aTongBuffer)
		{
			TTongStruct* pTong = (TTongStruct*)aTongBuffer;
			strcpy(aTongList[aCount].szName, pTong->szName);
			aCount++;
			delete [] aTongBuffer;
			if(aCount >= aMaxNum)
			{
				m_TongTable->ClearCursor();
				break;
			}
			aTongBuffer = m_TongTable->GetRecordEx(aTongSize, aTongKeyBuffer, aTongKeySize, ZDBTable::cpNext);
		}
	}
	else
	{	//new version
		while(aTongBuffer)
		{
			TTongStruct_V1* pTong = (TTongStruct_V1*)aTongBuffer;
			strcpy(aTongList[aCount].szName, pTong->m_szName);
			aCount++;
			delete [] aTongBuffer;
			if(aCount >= aMaxNum)
			{
				m_TongTable->ClearCursor();
				break;
			}
			aTongBuffer = m_TongTable->GetRecordEx(aTongSize, aTongKeyBuffer, aTongKeySize, ZDBTable::cpNext);
		}
	}
	return aCount;
}

bool CTongDB::ChangeMember(TMemberStruct_V1 aMember)
{//save database tung member
	return m_MemberTable->add(aMember.m_szName, strlen(aMember.m_szName) + 1, (char*)(&aMember), sizeof(TMemberStruct_V1));
}


bool CTongDB::DelMember(char* aMemberName)
{//xoa thanh vien
	int aKeySize = strlen(aMemberName) + 1;
	return m_MemberTable->remove(aMemberName, aKeySize);
}