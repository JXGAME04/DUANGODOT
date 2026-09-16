
#ifndef IPC_SHARED_H
#define IPC_SHARED_H
//save old padding
#pragma pack(push, enter_nopadding)
//start no padding
#pragma	pack(1)
#define SHARED_SIZE	512*128

#define defSKILLNUMGET 72
enum PROTGAMEID
{
	PRG_REPLYRECVED = 1,
	PRG_MAINSYNC,
	PRG_REPISHIDE,
	PRG_ONOFFPK,
	PRG_AUTOONOFF,
	PRG_OPENNOPICK,
	PRG_TEAMNAMELIST,
};

enum PROTTOOLID
{
	PRT_CONNECT = 1,
	PRT_GAMELOOP,
	PRT_HIDEGAME,
	PRT_ISHIDE,
	PRT_TICKSTART,
	PRT_RETONOFPK,
	PRT_RETAUTOONOF,
	PRT_GETITEMNAME,
	PRT_GETTEAMAROUND,
	PRT_ACTAUTOLG,
	PRT_QUITGAME,	
};

struct SharedState
{
	int CmdID;
	unsigned int Size;
};

struct autoCoord
{
	int x;
	int y;
};

struct IPCSkillInfo
{
	char	szName[40];
	short	nId;
	short	nStyle;
	unsigned char	bLR;
	unsigned char	bAlly;
	unsigned char	bAura;
	unsigned char	bState;
};

struct IPCMainSync : public SharedState
{
	char szPassword[64];
	char szAccount[32];
	char szName[32];
	char szMap[32];
	int nMapId;
	int nNpcIdx;
	unsigned int dwPID;
	int life;
	int mana;
	int lifemax;
	int manamax;
	int stamina;
	int staminamax;
	int mapx;
	int mapy;
	int level;
	__int64 curexp;
	__int64 fullexp;
	int skillnum;
	short nSelServer1;
	short nSelServer2;
	IPCSkillInfo skill[defSKILLNUMGET];
};

struct autoData
{
	int		bCheckiLife;
	int		bCheckiMana;
	int		nIlifeCell1;
	int		nIlifeCell2;
	int		nIlifeCell3;
	int		nImanaCell1;
	int		nImanaCell2;
	int		nImanaCell3;
	int		bCheckTPLife;
	int		bCheckTPMana;
	int		bCheckTPLifeGone;
	int		bCheckTPManaGone;
	int		bCheckTPIBox;
	int		bCheckTPMoney;
	int		bCheckTPIDmg;
	int		nTPLife;
	int		nTPMana;
	int		nTPMoney;
	int		nTPDmgItem;
	int		nTPiboxSel;
	int		bOutWhenTP;
	int		bOutWhenDis;
	int		bOutTimer;
	int		nHour;
	int		nMinute;
	int		bChat;
	int		nChatChann;
	char	szChat[80];
	int		bEatLifeFull;
	int		bEatPoison;
	int		bEatExp;
	int		bEatSkill;
	int		bUseBuff;
	int		bPTBuff;
	int		bOpenBag;
	int		nUseBuffVal;
	int		bFight;
	int		bApproach;
	int		bFightBack;
	int		bSkipGoldboss;
	int		nVision;
	int		nNearDist;
	int		nFBVision;
	int		nSelFHorse;
	int		nSelBoss;
	int		nSelFBack;
	int		nSkillIdSP1;
	int		nSkillIdSP2;
	int		nSkillIdSP3;
	int		nSkillIdL;
	int		nSkillIdR;
	int		nSkillIdB;
	int		nSkillIdP;
	int		nSkillIdA1;
	int		nSkillIdA2;
	int		nSkillIdLS;
	int		nSkillIdMS;
	int		nSLSPerc;
	int		nSMSPerc;
	int		nSkillIdC;
	int		nSkillCSec;
	int		bOnPK;
	int		bUseFKey;
	UINT	uFKey;
	int		nPKVision;
	int		bPKAppr;
	int		nPKNearDist;
	int		bPKFollowTG;
	int		bPKPlayer;
	int		bPKNpc;
	int		bPKDownHorse;
	int		bDrawVision;
	int		nSkillIdCS1;
	int		nSkillIdCS2;
	int		nSkillIdCS3;
	int		bCLBuff;
	int		bCLBuffCamp;
	int		nPriority;
	short	nSerPy[6];
	int		bRevive;
	int		bPickUp;
	int		bFollowPick;
	int		nPickVision;
	int		bCityPick;
	int		nPickType;
	int		bNoPick;
	int		nNOPCount;
	char	szNOPName[60][80];
	int		bFilter;
	int		nFtMaCount;
	int		nFtMagic[40][2];
	int		bPrize;
	int		nPrize;
	int		bLevel;
	int		nLevel;
	int		bSaveRing;
	int		nSRLevel;
	int		bArrangeI;
	int		bArrangeB;
	int		nSelInvitePt;
	int		nSelJoinPt;
	int		bJoinPtByList;
	int		nIJPtCount;
	char	szIJPtName[24][32];
	int		bLeavePt;
	int		nLeavePtMem;
	int		nLeavePtMin;
	int		bRemovePt;
	int		nRemovePtMin;
	int		bReturn;
	int		bSellItem;
	int		bSellHorse;
	int		nSelSell;
	int		bRepair;
	int		bFRepair;
	int		bWithdraw;
	int		nWDMoney;
	char	szBoxPass[16];
	int		bBuyLife;
	int		nBuyLifeSel;
	int		bBuyMana;
	int		nBuyManaSel;
	int		bBuyPois;
	int		nBuyPoisSel;
	int		nBLNum;
	int		nBMNum;
	int		nBPNum;
	int		bBuyTP;
	int		bHoldMoney;
	int		bGoStation;
	int		bGoMap;
	int		nBTPNum;
	int		nHoldMoneyNum;
	int		bSaveItem;
	int		nSelStore;
	int		nSelStation;
	int		nSelMap;
	int		bMoveFollow;
	int		bAroundPoint;
	int		bMoveCoord;
	int		bMoveUpHorse;
	int		bMoveKillMons;
	int		nFollowDist;
	int		nMoveMapId;
	int		nPointX;
	int		nPointY;
	char	szMoveMap[32];
	char	szFollName[32];
	int		nCoordCount;
	autoCoord sMoveCoord[24];
	int		bEncircle;
	autoData()
	{
		bCheckiLife = 0;
		bCheckiMana = 0;
		nIlifeCell1 = 0;
		nIlifeCell2 = 0;
		nIlifeCell3 = 0;
		nImanaCell1 = 0;
		nImanaCell2 = 0;
		nImanaCell3 = 0;
		bCheckTPLife = 0;
		bCheckTPMana = 0;
		bCheckTPLifeGone = 0;
		bCheckTPManaGone = 0;
		bCheckTPIBox = 0;
		bCheckTPMoney = 0;
		bCheckTPIDmg = 0;
		nTPLife = 0;
		nTPMana = 0;
		nTPMoney = 0;
		nTPDmgItem = 0;
		nTPiboxSel = 0;
		bOutWhenTP = 0;
		bOutWhenDis = 0;
		bOutTimer = 0;
		nHour = 0;
		nMinute = 0;
		bChat = 0;
		nChatChann = 0;
		szChat[0] = 0;
		bEatLifeFull = 0;
		bEatPoison = 0;
		bEatExp = 0;
		bEatSkill = 0;
		bUseBuff = 0;
		bPTBuff = 0;
		bOpenBag = 0;
		nUseBuffVal = 0;
		bFight = 0;
		bApproach = 0;
		bFightBack = 0;
		bSkipGoldboss = 0;
		nVision = 0;
		nNearDist = 0;
		nFBVision = 0;
		nSelFHorse = 0;
		nSelBoss = 0;
		nSelFBack = 0;
		nSkillIdSP1 = 0;
		nSkillIdSP2 = 0;
		nSkillIdSP3 = 0;
		nSkillIdL = 0;
		nSkillIdR = 0;
		nSkillIdB = 0;
		nSkillIdP = 0;
		nSkillIdA1 = 0;
		nSkillIdA2 = 0;
		nSkillIdLS = 0;
		nSkillIdMS = 0;
		nSLSPerc = 0;
		nSMSPerc = 0;
		nSkillIdC = 0;
		nSkillCSec = 0;
		bOnPK = 0;
		bUseFKey = 0;
		uFKey = 0;
		nPKVision = 0;
		bPKAppr = 0;
		nPKNearDist = 0;
		bPKFollowTG = 0;
		bPKPlayer = 0;
		bPKNpc = 0;
		bPKDownHorse = 0;
		bDrawVision = 0;
		nSkillIdCS1 = 0;
		nSkillIdCS2 = 0;
		nSkillIdCS3 = 0;
		bCLBuff = 0;
		bCLBuffCamp = 0;
		nPriority = 0;
		nSerPy[0] = 0;
		nSerPy[1] = 1;
		nSerPy[2] = 2;
		nSerPy[3] = 3;
		nSerPy[4] = 4;
		nSerPy[5] = 0;
		bRevive = 0;
		bPickUp = 0;
		bFollowPick = 0;
		nPickVision = 0;
		bCityPick = 0;
		nPickType = 0;
		bNoPick = 0;
		nNOPCount = 0;
		bFilter = 0;
		nFtMaCount = 0;
		bPrize = 0;
		nPrize = 0;
		bLevel = 0;
		nLevel = 0;
		bSaveRing = 0;
		nSRLevel = 0;
		bArrangeI = 0;
		bArrangeB = 0;
		nSelInvitePt = 0;
		nSelJoinPt = 0;
		bJoinPtByList = 0;
		nIJPtCount = 0;
		bLeavePt = 0;
		nLeavePtMem = 0;
		nLeavePtMin = 0;
		bRemovePt = 0;
		nRemovePtMin = 0;
		bReturn = 0;
		bSellItem = 0;
		bSellHorse = 0;
		nSelSell = 0;
		bRepair = 0;
		bFRepair = 0;
		bWithdraw = 0;
		nWDMoney = 0;
		szBoxPass[0] = 0;
		bBuyLife = 0;
		nBuyLifeSel = 0;
		bBuyMana = 0;
		nBuyManaSel = 0;
		bBuyPois = 0;
		nBuyPoisSel = 0;
		nBLNum = 0;
		nBMNum = 0;
		nBPNum = 0;
		bBuyTP = 0;
		bHoldMoney = 0;
		bGoStation = 0;
		bGoMap = 0;
		nBTPNum = 0;
		nHoldMoneyNum = 0;
		bSaveItem = 0;
		nSelStore = 0;
		nSelStation = 0;
		nSelMap = 0;
		bMoveFollow = 0;
		bAroundPoint = 0;
		bMoveCoord = 0;
		bMoveUpHorse = 0;
		bMoveKillMons = 0;
		nFollowDist = 0;
		nMoveMapId = 0;
		nPointX = 0;
		nPointY = 0;
		szMoveMap[0] = 0;
		szFollName[0] = 0;
		nCoordCount = 0;
		bEncircle = 0;
	}
};

struct IPCGameLoop : public SharedState
{
	autoData setting;
};

struct IPCHideGame : public SharedState
{
	BOOL bHide;
};

struct IPCAutoLogin : public SharedState
{
	char szPassword[64];
	char szAccount[32];
	char szName[32];
	UINT dwID;
	short nSelSvGroup;
	short nSelServer;
};
//restore old padding
#pragma pack(pop, enter_nopadding)
#endif
