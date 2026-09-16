#ifndef ROLESTRUCT_H
#define ROLESTRUCT_H


#pragma pack( push, 1 )
struct TRoleBaseInfo
{
	//RoleBaseInfo
	unsigned long	dwId;
	char	szName[32];
	bool	bSex;
	char	szAlias[32];		
	char	caccname[32];
	unsigned char	nFirstSect;
	unsigned char	nSect;
	unsigned char	cFightMode;
	unsigned char	cUseRevive;
	unsigned char	cIsExchange;
// --------------------- add by spe 2003/07/19 -----------------------
	unsigned char	cPkStatus;
// -------------------------------------------------------------------
	int		ijoincount;
	int		isectrole;
	int		igroupcode;
	int		igrouprole;
	int		irevivalid;
	int		irevivalx;
	int		irevivaly;
	int		ientergameid;
	int		ientergamex;
	int		ientergamey;
	char	cpartnercode[32];
	int		isavemoney;
	int		imoney;
	int		ifiveprop;
	int		iteam;
	int		ifightlevel;
	int		fightexp;
	int		ileadlevel;
	int		ileadexp;
	int		iliveexp;
	int		ipower;
	int		iagility;
	int		iouter;
	int		iinside;
	int		iluck;
	int		imaxlife;
	int		imaxstamina;
	int		imaxinner;
// --------------------- add by spe 2003/07/19 -----------------------
	int		icurlife;
	int		icurstamina;
	int		icurinner;
	int		ipkvalue;
// -------------------------------------------------------------------
	int		ileftprop;
	int		ileftfight;
	int		ileftlife;
	int		ifinishgame;
	short	iarmorres;
	short	iweaponres;
	short	ihelmres;
};

struct TRoleData
{
	TRoleBaseInfo BaseInfo;
	bool	bBaseNeedUpdate;//通知是否需要更新
	//当存取时如果Offset等于-1，表示不更新该数据
	//Additional Info
	short	nFightSkillCount;
	short	nLiveSkillCount;
	unsigned char	nTaskCount;
	short	nItemCount;
	short	nFriendCount;
	int		dwTaskOffset;
	int		dwLSkillOffset;
	int		dwFSkillOffset;
	int		dwItemOffset;
	int		dwFriendOffset;
	int		dwDataLen;
	unsigned char	pBuffer[1];
};
#pragma pack( pop )

#endif //#define ROLESTRUCT_H