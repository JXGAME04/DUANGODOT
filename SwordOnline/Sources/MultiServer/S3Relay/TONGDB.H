// TongDB.h: interface for the CTongDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TONGDB_H__8C487F07_C490_4562_AD01_77953CCCBCE2__INCLUDED_)
#define AFX_TONGDB_H__8C487F07_C490_4562_AD01_77953CCCBCE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "DBTable.h"
#include "KTongControl.h"

#define MAX_TONG_COUNT 10000

typedef struct
{
	int			nCamp;
	DWORD		dwMoney;
	int			nCredit;
	int			nLevel;
	char		szName[defTONG_STR_LENGTH];
	char		szPassword[defTONG_STR_LENGTH];
	char		szMasterTitle[defTONG_STR_LENGTH];
	char		szDirectorTitle[defTONG_MAX_DIRECTOR][defTONG_STR_LENGTH];
	char		szManagerTitle[defTONG_MAX_MANAGER][defTONG_STR_LENGTH];
	char		szNormalTitle[defTONG_STR_LENGTH];
}TTongStruct;

typedef struct
{
	char		szName[defTONG_STR_LENGTH];
	char		szTong[defTONG_STR_LENGTH];
	TONG_MEMBER_FIGURE		MemberClass;
	int			nTitleIndex;
}TMemberStruct;

struct TTongStruct_V1
{
	char		m_szName[defTONG_STR_LENGTH];
	int			m_nCamp;
	int			m_nMoney;
	int			m_nBuildMoney;
	int			m_nLevel;
	int			m_nBuilLevel;
	int			m_nOffer;
	UINT		m_uCreateDate;
	UINT		m_uWeekCount;
	int			m_nExp;
	int			m_bLockRecruit;
	int			m_bOrdeal;
	int			m_nTaskLevel;
	UINT		m_uUnionID;
	UINT		m_uPromoteID;
	UINT		m_uAffairCount;
	UINT		m_uHistoryCount;
	UINT		m_uWorkshopCount;
	char		m_szAnnounce[128];
	TTongWeekGoal m_OldWeekGoal;
	TTongWeekGoal m_NewWeekGoal;
};

struct TMemberStruct_V1
{
	char	m_szName[defTONG_STR_LENGTH];		
	char	szTong[defTONG_STR_LENGTH];	
	char	m_szTitle[defTONG_STR_LENGTH];
	UINT	m_uJoinDate;
	UINT	m_uOnlineDate;
	UINT	m_uRight;
	int		m_nTotalOffer;
	int		m_nWeekOffer;
	int		m_nOldWGCompleted;
	int		m_nNewWGCompleted;
	BYTE	m_nSex;
	BYTE	m_nFigure;
	BYTE	m_bWGType;
	BYTE	m_bGetPrice;
	BYTE	m_bRetired;
	TMemberStruct_V1()
	{
		m_szName[0] = 0;
		szTong[0] = 0;	
		m_szTitle[0] = 0;
		m_uRight = 0;
		m_nTotalOffer = 0;
		m_nWeekOffer = 0;
		m_nOldWGCompleted = 0;
		m_nNewWGCompleted = 0;
		m_nSex = 0;
		m_nFigure = 0;
		m_bWGType = 0;
		m_bGetPrice = 0;
		m_bRetired = 0;
		m_uJoinDate = 0;
		m_uOnlineDate = 0;
	}
};

class CTongDB  
{
private:
	ZDBTable* m_TongTable;						//帮会数据表
	ZDBTable* m_MemberTable;					//帮众数据表
	
	TTongList m_TongList[MAX_TONG_COUNT];	//帮会列表
	int m_TongListCount;						//帮会列表数
public:
	CTongDB();
	virtual ~CTongDB(){};

	bool Open();
	void Close();
	
	bool ChangeTong(CTongControl& aTongControl);	//save bang
	bool DelTong(char* aTongName);	//xoa bang
	bool SearchTong(char* aTongName, CTongControl& aTongControl);	//load data
	int GetTongCount();							//so luong bang
	int GetTongList(TTongList* aTongList, int aMaxNum);	//danh sach ten bang

	bool ChangeMember(TMemberStruct_V1 aMember);//save thanh vien
	bool DelMember(char* aMemberName);		//xoa thanh vien
	
};

#endif // !defined(AFX_TONGDB_H__8C487F07_C490_4562_AD01_77953CCCBCE2__INCLUDED_)
