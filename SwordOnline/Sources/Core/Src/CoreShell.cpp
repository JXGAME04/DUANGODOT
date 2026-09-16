/*****************************************************************************************
//	Íâ½ç·ÃÎÊCore½Ó¿Ú·½·¨
//	Copyright : Kingsoft 2002
//	Author	:   
//	CreateTime:	2002-9-12
------------------------------------------------------------------------------------------
*****************************************************************************************/
#include "KCore.h"
#include "GameDataDef.h"
#include "CoreShell.h"

#include "CoreDrawGameObj.h"
#include "ImgRef.h"

#include "KPlayer.h"
#include "KPlayerSet.h"
#include "KItemList.h"
#include "KSubWorldSet.h"
#include "KProtocolProcess.h"

#include "KNpcResList.h"
#include "Scene/KScenePlaceC.h"
#include "kskills.h"
#include "GameDataDef.h"
#include "MsgGenreDef.h"
#include "KOption.h"
#include "KSubWorld.h"
#include "KViewItem.h"
#include "KTongProtocol.h"
#include "malloc.h"

#include "KBuySell.h"
#include "KThiefSkill.h"
#include "KMissleSet.h"
#include "KMath.h"
#include "KObjSet.h"
#define	NPC_TRADE_BOX_WIDTH		6
#define	NPC_TRADE_BOX_HEIGHT	10
#define	MAX_TRADE_ITEM_WIDTH	2
#define	MAX_TRADE_ITEM_HEIGHT	4
IClientCallback* l_pDataChangedNotifyFunc = 0;
int KNpc::g_DrawVision;
int KNpc::g_DrawVisionSkill;
class KCoreShell : public iCoreShell
{
public:
	int	 GetProtocolSize(BYTE byProtocol);
	int	 Debug(unsigned int uDataId, unsigned int uParam, int nParam);
	int	 OperationRequest(unsigned int uOper, unsigned int uParam, int nParam);
	void ProcessInput(unsigned int uMsg, unsigned int uParam, int nParam);
	int	 FindSelectNPC(int x, int y, int nRelation, bool bSelect, void* pReturn, int& nKind);
	int FindSelectObject(int x, int y, bool bSelect, int& nObjectIdx, int& nKind);
	int FindSpecialNPC(char* Name, void* pReturn, int& nKind);
	int ChatSpecialPlayer(void* pPlayer, const char* pMsgBuff, unsigned short nMsgLength);
	void ApplyAddTeam(void* pPlayer);
	void TradeApplyStart(void* pPlayer);
	int UseSkill(int x, int y, int nSkillID);
	int LockSomeoneUseSkill(int nTargetIndex, int nSkillID);
	int LockSomeoneAction(int nTargetIndex);
	int LockObjectAction(int nTargetIndex);
	void GotoWhere(int x, int y, int mode);	//mode 0 is auto, 1 is walk, 2 is run
	void Goto(int nDir, int mode);	//nDir 0~63, mode 0 is auto, 1 is walk, 2 is run
	void Turn(int nDir);	//nDir 0 is left, 1 is right, 2 is back
	int ThrowAwayItem();
	int GetNPCRelation(int nIndex);
	
	//ÓëµØÍ¼Ïà¹ØµÄ²Ù×÷,uOperµÄÈ¡ÖµÀ´×Ô GAME_SCENE_MAP_OPERATION_INDEX
	int	SceneMapOperation(unsigned int uOper, unsigned int uParam, int nParam);
	//Óë°ï»áÏà¹ØµÄ²Ù×÷, uOperµÄÈ¡ÖµÀ´×Ô GAME_TONG_OPERATION_INDEX
	int	TongOperation(unsigned int uOper, unsigned int uParam, int nParam);
	//Óë×é¶ÓÏà¹ØµÄ²Ù×÷£¬uOperµÄÈ¡ÖµÀ´×Ô GAME_TEAM_OPERATION_INDEX
	int TeamOperation(unsigned int uOper, unsigned int uParam, int nParam);

	int	 GetGameData(unsigned int uDataId, unsigned int uParam, int nParam);

	void DrawGameObj(unsigned int uObjGenre, unsigned int uId, int x, int y, int Width, int Height, int nParam);
	void DrawGameSpace();
	DWORD GetPing();
	//void SendPing();
	int	 SetCallDataChangedNofify(IClientCallback* pNotifyFunc);
	void NetMsgCallbackFunc(void* pMsgData);
	void SetRepresentShell(struct iRepresentShell* pRepresent);
	void SetMusicInterface(void* pMusicInterface);
	void SetRepresentAreaSize(int nWidth, int nHeight);
	int  Breathe();
	void Release();	//ÊÍ·Å½Ó¿Ú¶ÔÏó
	void SetClient(LPVOID pClient);
	void SendNewDataToServer(void* pData, int nLength);
};

struct sStation
{
	int x;
	int y;
};
sStation s53MedList[] =
{
	{51150, 102700},
};
sStation s53ShopList[] =
{
	{51205, 101480},
};
sStation s53MoveList[] =
{
	{50517, 103568},
};
sStation s53Center = {51892, 101854};
//-----------
sStation s20MedList[] =
{
	{111034, 197262},
};
sStation s20ShopList[] =
{
	{107837, 200166},
};
sStation s20MoveList[] =
{
	{110670, 195540},
};
sStation s20Center = {113435, 198520};
//-----------
sStation s99MedList[] =
{
	{51054, 103276},
};
sStation s99ShopList[] =
{
	{51405, 105034},
};
sStation s99MoveList[] =
{
	{52171, 105754},
	{51440, 101176},
};
sStation s99Center = {52159, 102392};
//-----------
sStation s101MedList[] =
{
	{53734, 102290},
};
sStation s101ShopList[] =
{
	{52455, 100606},
};
sStation s101MoveList[] =
{
	{51942, 99132},
};
sStation s101Center = {54111, 100866};
//-----------
sStation s100MedList[] =
{
	{53038, 99966},
};
sStation s100ShopList[] =
{
	{52512, 100036},
};
sStation s100MoveList[] =
{
	{53707, 99260},
	{51543, 99112},
	{51307, 101368},
	{55022, 102942},
};
sStation s100Center = {52759, 100768};
//-----------
sStation s121MedList[] =
{
	{61906, 145656},
};
sStation s121ShopList[] =
{
	{61266, 144794},
};
sStation s121MoveList[] =
{
	{61578, 141662},
	{63385, 146996},
};
sStation s121Center = {62496, 144256};
//-----------
sStation s153MedList[] =
{
	{51224, 102696},
};
sStation s153ShopList[] =
{
	{52311, 103142},
};
sStation s153MoveList[] =
{
	{52181, 101696},
	{53928, 103822},
};
sStation s153Center = {52180, 103726};
//-----------
sStation s174MedList[] =
{
	{50323, 104126},
};
sStation s174ShopList[] =
{
	{50127, 102480},
};
sStation s174MoveList[] =
{
	{52306, 102274},
};
sStation s174Center = {51571, 102806};
//-----------
sStation s01MedList[] =
{
	{51280, 102082},
};
sStation s01ShopList[] =
{
	{49902, 102572},
};
sStation s01MoveList[] =
{
	{48596, 103334},
	{49892, 99436},
	{52694, 101276},
	{52894, 104962},
};
sStation s01Center = {51059, 102406};
//-----------
sStation s78MedList[] =
{
	{51584, 103800},
};
sStation s78ShopList[] =
{
	{52140, 104334},
};
sStation s78MoveList[] =
{
	{54146, 103434},
	{51138, 108056},
	{45996, 102746},
	{48309, 100412},
};
sStation s78Center = {50499, 103440};
//-----------
sStation s11MedList[] =
{
	{100535, 164300},
};
sStation s11ShopList[] =
{
	{98992, 164338},
};
sStation s11MoveList[] =
{
	{102323, 166194},
	{96561, 162830},
	{96886, 158658},
	{104396, 160110},
};
sStation s11Center = {100425, 162088};
//-----------
sStation s162MedList[] =
{
	{47983, 102686},
};
sStation s162ShopList[] =
{
	{49138, 102416},
};
sStation s162MoveList[] =
{
	{47032, 104606},
	{53366, 100000},
	{54355, 104858},
};
sStation s162Center = {50818, 100638};
//-----------
sStation s37MedList[] =
{
	{56804, 98886},
};
sStation s37ShopList[] =
{
	{57170, 99270},
};
sStation s37MoveList[] =
{
	{54561, 103124},
	{52176, 101964},
	{51024, 95796},
	{59569, 93640},
};
sStation s37Center = {55757, 98198};
//-----------
sStation s80MedList[] =
{
	{56778, 98524},
};
sStation s80ShopList[] =
{
	{54484, 96666},
};
sStation s80MoveList[] =
{
	{53478, 95684},
	{58383, 98012},
	{54982, 102332},
	{50970, 102168},
};
sStation s80Center = {56569, 97092};
//-----------
sStation s176MedList[] =
{
	{51725, 95238},
	{49423, 94814},
	{53938, 106180},
	{47228, 107542},
};
sStation s176ShopList[] =
{
	{42868, 101256},
};
sStation s176MoveList[] =
{
	{43279, 96462},
	{43889, 106572},
	{54287, 105342},
	{51250, 93178},
};
sStation s176Center = {50416, 94524};
//-----------
typedef std::vector<sStation>	StationVector;
typedef std::map<int, StationVector> MapStation;
static MapStation g_MedicineStation;
static MapStation g_ShopStation;
static MapStation g_MoveStation;
typedef std::map<int, sStation> MapOneStation;
static MapOneStation g_CenterStation;
static int g_GoMapID[] = 
{
	875,
	322,
	321,
	75,
	225,
	226,
	227,
	336,
	340,
	144,
	93,
	124,
	152,
	224,
	198,
	320,
	181,
	319,
	123,
	206,
	79,
	56,
	166,
	182,
	164,
	21,
	167,
	193,
	170,
	19,
	7,
};

struct BOTTLE_CTRL_MAP
{
	int				nDetail;
	int				nLevel;
};

static BOTTLE_CTRL_MAP g_LifeBottle[] =
{
	{0, 1},
	{0, 2},
	{0, 3},
	{0, 4},
	{0, 5},
	{2, 1},
	{2, 2},
	{2, 3},
	{2, 4},
	{2, 5},
};

static BOTTLE_CTRL_MAP g_ManaBottle[] =
{
	{1, 1},
	{1, 2},
	{1, 3},
	{1, 4},
	{1, 5},
	{2, 1},
	{2, 2},
	{2, 3},
	{2, 4},
	{2, 5},
};

static BOTTLE_CTRL_MAP g_PoisonBottle[] =
{
	{4, 1},
	{4, 2},
	{4, 3},
	{4, 4},
	{4, 5},
};

static KCoreShell	g_CoreShell;

CORE_API void g_InitCore();
#ifndef CORE_STATIC
#ifndef _STANDALONE
extern "C" __declspec(dllexport)
#endif
#else
extern "C"
#endif

iCoreShell* CoreGetShell()
{
	g_InitCore();
	//add auto station pos
	int count,i;
	count = sizeof(s53MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[53].push_back(s53MedList[i]);
	}
	count = sizeof(s53ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[53].push_back(s53ShopList[i]);
	}
	count = sizeof(s53MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[53].push_back(s53MoveList[i]);
	}
	g_CenterStation[53] = s53Center;
	//---------
	count = sizeof(s20MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[20].push_back(s20MedList[i]);
	}
	count = sizeof(s20ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[20].push_back(s20ShopList[i]);
	}
	count = sizeof(s20MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[20].push_back(s20MoveList[i]);
	}
	g_CenterStation[20] = s20Center;
	//---------
	count = sizeof(s99MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[99].push_back(s99MedList[i]);
	}
	count = sizeof(s99ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[99].push_back(s99ShopList[i]);
	}
	count = sizeof(s99MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[99].push_back(s99MoveList[i]);
	}
	g_CenterStation[99] = s99Center;
	//---------
	count = sizeof(s101MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[101].push_back(s101MedList[i]);
	}
	count = sizeof(s101ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[101].push_back(s101ShopList[i]);
	}
	count = sizeof(s101MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[101].push_back(s101MoveList[i]);
	}
	g_CenterStation[101] = s101Center;
	//---------
	count = sizeof(s100MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[100].push_back(s100MedList[i]);
	}
	count = sizeof(s100ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[100].push_back(s100ShopList[i]);
	}
	count = sizeof(s100MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[100].push_back(s100MoveList[i]);
	}
	g_CenterStation[100] = s100Center;
	//---------
	count = sizeof(s121MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[121].push_back(s121MedList[i]);
	}
	count = sizeof(s121ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[121].push_back(s121ShopList[i]);
	}
	count = sizeof(s121MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[121].push_back(s121MoveList[i]);
	}
	g_CenterStation[121] = s121Center;
	//---------
	count = sizeof(s153MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[153].push_back(s153MedList[i]);
	}
	count = sizeof(s153ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[153].push_back(s153ShopList[i]);
	}
	count = sizeof(s153MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[153].push_back(s153MoveList[i]);
	}
	g_CenterStation[153] = s153Center;
	//---------
	count = sizeof(s174MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[174].push_back(s174MedList[i]);
	}
	count = sizeof(s174ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[174].push_back(s174ShopList[i]);
	}
	count = sizeof(s174MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[174].push_back(s174MoveList[i]);
	}
	g_CenterStation[174] = s174Center;
	//---------
	count = sizeof(s01MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[1].push_back(s01MedList[i]);
	}
	count = sizeof(s01ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[1].push_back(s01ShopList[i]);
	}
	count = sizeof(s01MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[1].push_back(s01MoveList[i]);
	}
	g_CenterStation[1] = s01Center;
	//---------
	count = sizeof(s78MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[78].push_back(s78MedList[i]);
	}
	count = sizeof(s78ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[78].push_back(s78ShopList[i]);
	}
	count = sizeof(s78MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[78].push_back(s78MoveList[i]);
	}
	g_CenterStation[78] = s78Center;
	//---------
	count = sizeof(s11MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[11].push_back(s11MedList[i]);
	}
	count = sizeof(s11ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[11].push_back(s11ShopList[i]);
	}
	count = sizeof(s11MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[11].push_back(s11MoveList[i]);
	}
	g_CenterStation[11] = s11Center;
	//---------
	count = sizeof(s162MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[162].push_back(s162MedList[i]);
	}
	count = sizeof(s162ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[162].push_back(s162ShopList[i]);
	}
	count = sizeof(s162MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[162].push_back(s162MoveList[i]);
	}
	g_CenterStation[162] = s162Center;
	//---------
	count = sizeof(s37MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[37].push_back(s37MedList[i]);
	}
	count = sizeof(s37ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[37].push_back(s37ShopList[i]);
	}
	count = sizeof(s37MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[37].push_back(s37MoveList[i]);
	}
	g_CenterStation[37] = s37Center;
	//---------
	count = sizeof(s80MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[80].push_back(s80MedList[i]);
	}
	count = sizeof(s80ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[80].push_back(s80ShopList[i]);
	}
	count = sizeof(s80MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[80].push_back(s80MoveList[i]);
	}
	g_CenterStation[80] = s80Center;
	//---------
	count = sizeof(s176MedList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MedicineStation[176].push_back(s176MedList[i]);
	}
	count = sizeof(s176ShopList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_ShopStation[176].push_back(s176ShopList[i]);
	}
	count = sizeof(s176MoveList) / sizeof(sStation);
	for (i = 0; i < count; ++i)
	{
		g_MoveStation[176].push_back(s176MoveList[i]);
	}
	g_CenterStation[176] = s176Center;
	////////////
	return &g_CoreShell;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º·¢³öÓÎÏ·ÊÀ½çÊý¾Ý¸Ä±äµÄÍ¨Öªº¯Êý
//	·µ»Ø£ºÈçÎ´±»×¢²áÍ¨Öªº¯Êý£¬ÔòÖ±½Ó·µ»Ø0£¬·ñÔò·µ»ØÍ¨Öªº¯ÊýÖ´ÐÐ½á¹û¡£
//--------------------------------------------------------------------------
int CoreDataChanged(unsigned int uDataId, unsigned int uParam, int nParam)
{
	if (l_pDataChangedNotifyFunc)
		return l_pDataChangedNotifyFunc->CoreDataChanged(uDataId, uParam, nParam);
	return 0;
}

void SendDataToTool(const void * const pData, const size_t &datalength)
{
	if (l_pDataChangedNotifyFunc)
		l_pDataChangedNotifyFunc->SendDataToTool(pData, datalength);
}

void KCoreShell::Release()
{
	g_ReleaseCore();
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º½ÓÊÜÓë·ÖÅÉ´¦ÀíÍøÂçÏûÏ¢
//--------------------------------------------------------------------------
void KCoreShell::NetMsgCallbackFunc(void* pMsgData)
{
	g_ProtocolProcess.ProcessNetMsg((BYTE *)pMsgData);
}
//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÉèÖÃÓÎÏ·ÊÀ½çÊý¾Ý¸Ä±äµÄÍ¨Öªº¯Êý
//	²ÎÊý£ºfnCoreDataChangedCallback pNotifyFunc --> Í¨Öªº¯ÊýµÄÖ¸Õë¡£
//	·µ»Ø£º·µ»ØÖµÎª·Ç0Öµ±íÊ¾×¢²á³É¹¦£¬·ñÔò±íÊ¾Ê§°Ü¡£
//--------------------------------------------------------------------------
int	KCoreShell::SetCallDataChangedNofify(IClientCallback* pNotifyFunc)
{
	l_pDataChangedNotifyFunc = pNotifyFunc;
	return true;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º´ÓÓÎÏ·ÊÀ½ç»ñÈ¡Êý¾Ý
//	²ÎÊý£ºunsigned int uDataId --> ±íÊ¾»ñÈ¡ÓÎÏ·Êý¾ÝµÄÊý¾ÝÏîÄÚÈÝË÷Òý£¬ÆäÖµÎªÃ·¾ÙÀàÐÍ
//							GAMEDATA_INDEXµÄÈ¡ÖµÖ®Ò»¡£
//		  unsigned int uParam  --> ÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//		  int nParam --> ÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//	·µ»Ø£ºÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨¡£
//--------------------------------------------------------------------------
int	KCoreShell::GetGameData(unsigned int uDataId, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uDataId)
	{
	case GDI_PLAYER_IS_MALE:
		{
			int nIndex = 0;
			if (nParam == 0)
				nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;
			else
				nIndex = NpcSet.SearchID(nParam);

			if (nIndex)
				nRet = (Npc[nIndex].m_NpcSettingIdx == PLAYER_MALE_NPCTEMPLATEID);
			else
				nRet = 1;	//³ö´íÊ±ÊÇÄÐÐÔ
		}
		break;
	case GDI_REPAIR_ITEM_PRICE:
		if (uParam)
		{
			KUiObjAtContRegion *pObj = (KUiObjAtContRegion *)uParam;
			KItem*	pItem = NULL;

			switch(pObj->Obj.uGenre)
			{
			case CGOG_ITEM:
				{
					if (pObj->Obj.uId > 0)
					{
						pItem = &Item[pObj->Obj.uId];
					}
				}
				break;
			default:
				break;
			}

			if (!pItem)
				break;

			KUiItemBuySelInfo *pInfo = (KUiItemBuySelInfo *)nParam;
		
			if (pObj->eContainer == UOC_NPC_SHOP)
				break;
			pInfo->nPrice = pItem->GetRepairPrice();
			strcpy(pInfo->szItemName, pItem->GetName());
			nRet = pItem->CanBeRepaired();
		}
		else
		{
			nParam = 0;
			nRet = 0;
		}
		break;
	case GDI_TRADE_ITEM_PRICE:
		if (uParam)
		{
			KUiObjAtContRegion *pObj = (KUiObjAtContRegion *)uParam;
			KItem*	pItem = NULL;

			switch(pObj->Obj.uGenre)
			{
			case CGOG_ITEM:
				{
					if (pObj->Obj.uId > 0)
					{
						pItem = &Item[pObj->Obj.uId];
					}
				}
				break;
			case CGOG_NPCSELLITEM:
				{
					int	nBuyIdx = Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx;
					if (nBuyIdx != -1)
					{
						int nIndex = BuySell.GetItemIndex(nBuyIdx, pObj->Obj.uId);
						if (nIndex >= 0)
							pItem = BuySell.GetItem(nIndex);
					}
				}
				break;
			default:
				break;
			}

			if (!pItem)
				break;

			KUiItemBuySelInfo *pInfo = (KUiItemBuySelInfo *)nParam;
		
			if (pObj->eContainer == UOC_NPC_SHOP)
				pInfo->nPrice = pItem->GetPrice();
			else
				pInfo->nPrice = pItem->GetPrice() / BUY_SELL_SCALE;
			strcpy(pInfo->szItemName, pItem->GetName());
			nRet = (pItem->GetGenre() != item_task);
		}
		else
		{
			nParam = 0;
			nRet = 0;
		}
		break;
	//ÓÎÏ·¶ÔÏóÃèÊöËµÃ÷ÎÄ±¾´®
	//uParam = (KUiGameObject*) ÃèÊöÓÎÏ·¶ÔÏóµÄ½á¹¹Êý¾ÝµÄÖ¸Õë
	//nParam = (char*) Ö¸ÏòÒ»¸ö»º³åÇøµÄÖ¸Õë£¬Æä¿Õ¼ä²»ÉÙÓÚ256×Ö½Ú¡£
	case GDI_GAME_OBJ_DESC_INCLUDE_REPAIRINFO:
	case GDI_GAME_OBJ_DESC_INCLUDE_TRADEINFO:
		if (nParam && uParam)
		{
			KUiObjAtContRegion* pObj = (KUiObjAtContRegion *)uParam;
			char* pszDescript = (char *)nParam;
			pszDescript[0] = 0;
			switch(pObj->Obj.uGenre)
			{
			case CGOG_ITEM:
				{
					if (pObj->eContainer == UOC_EQUIPTMENT)
					{
						int nActive = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetActiveAttribNum(pObj->Obj.uId);
						Item[pObj->Obj.uId].GetDesc(pszDescript, true, BUY_SELL_SCALE, nActive,
							Player[CLIENT_PLAYER_INDEX].m_ItemList.IsActivateAll(), UOC_EQUIPTMENT);
					}
					else
						Item[pObj->Obj.uId].GetDesc(pszDescript, true, BUY_SELL_SCALE);
				}
				break;			
			case CGOG_NPCSELLITEM:
				{
					int nIdx = -1;
					if (-1 == Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx)
						break;
					nIdx = BuySell.GetItemIndex(Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx, pObj->Obj.uId);

					KItem* pItem = NULL;
					if (nIdx < 0)
						break;
					pItem = BuySell.GetItem(nIdx);

					if (!pItem)
						break;
					pItem->GetDesc(pszDescript, true);
				}
				break;
			}
		}
		break;
	case GDI_GAME_OBJ_DESC:
		if (nParam && uParam)
		{
			KUiObjAtContRegion* pObj = (KUiObjAtContRegion *)uParam;
			char* pszDescript = (char *)nParam;
			pszDescript[0] = 0;
			switch(pObj->Obj.uGenre)
			{
			case CGOG_ITEM:
				{
					if (pObj->eContainer == UOC_EQUIPTMENT)
					{
						int nActive = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetActiveAttribNum(pObj->Obj.uId);
						Item[pObj->Obj.uId].GetDesc(pszDescript, false, 1, nActive,
							Player[CLIENT_PLAYER_INDEX].m_ItemList.IsActivateAll(), UOC_EQUIPTMENT);
					}
					else
						Item[pObj->Obj.uId].GetDesc(pszDescript);
				}
				break;
			case CGOG_SKILL:
			case CGOG_SKILL_FIGHT:
			case CGOG_SKILL_LIVE:
			case CGOG_SKILL_SHORTCUT:
				{
					int nLevel = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_SkillList.GetLevel(pObj->Obj.uId);
					_ASSERT(nLevel >= 0);
					if (pObj->Obj.uId >0)
					{
						
						ISkill * pISkill = g_SkillManager.GetSkill(pObj->Obj.uId, 1);
						if (!pISkill)
							break;
						eSkillStyle eStyle = (eSkillStyle) pISkill->GetSkillStyle();
						
						switch(eStyle)
						{
						case SKILL_SS_Missles:			//	×Óµ¯Àà		±¾¼¼ÄÜÓÃÓÚ·¢ËÍ×Óµ¯Àà
						case SKILL_SS_Melee:
						case SKILL_SS_InitiativeNpcState:	//	Ö÷¶¯Àà		±¾¼¼ÄÜÓÃÓÚ¸Ä±äµ±Ç°NpcµÄÖ÷¶¯×´Ì¬
						case SKILL_SS_PassivityNpcState:		//	±»¶¯Àà		±¾¼¼ÄÜÓÃÓÚ¸Ä±äNpcµÄ±»¶¯×´Ì¬
							{
								KSkill::GetDesc(
									pObj->Obj.uId,
									nLevel,
									pszDescript,
									Player[CLIENT_PLAYER_INDEX].m_nIndex,
									(pObj->Obj.uGenre == CGOG_SKILL_SHORTCUT)?false:true
									);
							}
							break;
							
						case SKILL_SS_Thief:
							{
								((KThiefSkill *)pISkill)->GetDesc(
									pObj->Obj.uId,
									nLevel,
									pszDescript,
									Player[CLIENT_PLAYER_INDEX].m_nIndex,
									(pObj->Obj.uGenre == CGOG_SKILL_SHORTCUT)?false:true
								);
			
							}break;
							
						}
					}
					
				}
				break;
			
			case CGOG_PLAYER_FACE:
				break;
			case CGOG_NPCSELLITEM:
				{
					int nIdx = -1;
					if (-1 == Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx)
						break;
					nIdx = BuySell.GetItemIndex(Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx, pObj->Obj.uId);

					KItem* pItem = NULL;
					if (nIdx < 0)
						break;
					pItem = BuySell.GetItem(nIdx);

					if (!pItem)
						break;
					pItem->GetDesc(pszDescript);
				}
				break;
			}
		}
		break;
	//Ö÷½ÇµÄÒ»Ð©²»Ò×±äµÄÊý¾Ý
	//uParam = (KUiPlayerBaseInfo*)pInfo	
	case GDI_PLAYER_BASE_INFO:
		if (uParam)
		{
			KUiPlayerBaseInfo* pInfo = (KUiPlayerBaseInfo*)uParam;
			int nIndex = 0;
			if (nParam == 0)
			{
				nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;
				pInfo->nCurFaction = Player[CLIENT_PLAYER_INDEX].m_cFaction.m_nCurFaction;
				pInfo->nCurTong = Player[CLIENT_PLAYER_INDEX].m_cTong.GetTongNameID();
			}
			else
			{
				nIndex = NpcSet.SearchID(nParam);
				pInfo->nCurFaction = -1;
				pInfo->nCurTong = 0;
			}
			if (nIndex)
			{
				strcpy(pInfo->Name, Npc[nIndex].Name);
				//to do:no implements in this version
				pInfo->Agname[0] = 0;
				pInfo->Title[0] = 0;
				if (Npc[nIndex].m_btRankId)
				{
					char szRankId[5];
					itoa(Npc[nIndex].m_btRankId, szRankId, 10);
					g_RankTabSetting.GetString(szRankId, "RANKSTR", "", pInfo->Title, 32);
				}
				pInfo->nRankInWorld = Player[CLIENT_PLAYER_INDEX].m_nWorldStat;
			}
		}
		break;

	//Ö÷½ÇµÄÒ»Ð©Ò×±äµÄÊý¾Ý
	//uParam = (KUiPlayerRuntimeInfo*)pInfo
	case GDI_PLAYER_RT_INFO:
		if (uParam)
		{
			KUiPlayerRuntimeInfo* pInfo = (KUiPlayerRuntimeInfo*)uParam;
			pInfo->nLifeFull = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentLifeMax;		//ÉúÃüÂúÖµ
			pInfo->nLife = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentLife;				//ÉúÃü
			pInfo->nManaFull = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentManaMax;		//ÄÚÁ¦ÂúÖµ
			pInfo->nMana = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentMana;				//ÄÚÁ¦
			pInfo->nStaminaFull = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentStaminaMax;//ÌåÁ¦ÂúÖµ
			pInfo->nStamina = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_CurrentStamina;		//ÌåÁ¦

			pInfo->nAngryFull = 0;		//Å­ÂúÖµ
			pInfo->nAngry = 0;			//Å­
			
			pInfo->nExperienceFull = Player[CLIENT_PLAYER_INDEX].m_nNextLevelExp;		//¾­ÑéÂúÖµ
			pInfo->nExperience = Player[CLIENT_PLAYER_INDEX].m_nExp;					//¾­Ñé
			pInfo->nCurLevelExperience = Player[CLIENT_PLAYER_INDEX].m_nNextLevelExp;
			
			pInfo->byActionDisable = 0;
			//to do	¸øpInfo->bActionDisable¡¢¸³ÓèºÏÊÊµÄÖµ

			pInfo->byAction = PA_NONE;

			if (Player[CLIENT_PLAYER_INDEX].m_RunStatus)
				pInfo->byAction |= PA_RUN;

			if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Doing == do_sit)
				pInfo->byAction |= PA_SIT;
			if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_bRideHorse)
				pInfo->byAction |= PA_RIDE;
			pInfo->wReserved = 0;
		}
		break;

	//Ö÷½ÇµÄÒ»Ð©Ò×±äµÄÊôÐÔÊý¾Ý
	//uParam = (KUiPlayerAttribute*)pInfo
	case GDI_PLAYER_RT_ATTRIBUTE:
		if (uParam)
		{
			KUiPlayerAttribute* pInfo = (KUiPlayerAttribute*)uParam;
			KNpc*	pNpc = &Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex];
			pInfo->nMoney = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetMoney(room_equipment);				//ÒøÁ½
			pInfo->nBARemainPoint = Player[CLIENT_PLAYER_INDEX].m_nAttributePoint;						//»ù±¾ÊôÐÔÊ£ÓàµãÊý
			pInfo->nStrength = Player[CLIENT_PLAYER_INDEX].m_nCurStrength;								//Á¦Á¿
			pInfo->nDexterity = Player[CLIENT_PLAYER_INDEX].m_nCurDexterity;								//Ãô½Ý
			pInfo->nVitality = Player[CLIENT_PLAYER_INDEX].m_nCurVitality;								//»îÁ¦
			pInfo->nEnergy = Player[CLIENT_PLAYER_INDEX].m_nCurEngergy;									//¾«Á¦

			Player[CLIENT_PLAYER_INDEX].GetEchoDamage(&pInfo->nKillMIN, &pInfo->nKillMAX, 0);				//×î´ó×îÐ¡É±ÉËÁ¦
			Player[CLIENT_PLAYER_INDEX].GetEchoDamage(&pInfo->nRightKillMin , &pInfo->nRightKillMax, 1);
			pInfo->nAttack = pNpc->m_CurrentAttackRating;				//¹¥»÷Á¦
			pInfo->nDefence = pNpc->m_CurrentDefend;					//·ÀÓùÁ¦
			pInfo->nMoveSpeed = pNpc->m_CurrentRunSpeed;				//ÒÆ¶¯ËÙ¶È
			pInfo->nAttackSpeed = pNpc->m_CurrentAttackSpeed;			//¹¥»÷ËÙ¶È
			//ÎïÀí·ÀÓù
			if (pNpc->m_CurrentPhysicsResistMax >= pNpc->m_CurrentPhysicsResist)
				pInfo->nPhyDef = pNpc->m_CurrentPhysicsResist;
			else
				pInfo->nPhyDef = pNpc->m_CurrentPhysicsResistMax;
			//±ù¶³·ÀÓù
			if (pNpc->m_CurrentColdResistMax >= pNpc->m_CurrentColdResist)
				pInfo->nCoolDef = pNpc->m_CurrentColdResist;
			else
				pInfo->nCoolDef = pNpc->m_CurrentColdResistMax;
			//ÉÁµç·ÀÓù
			if (pNpc->m_CurrentLightResistMax >= pNpc->m_CurrentLightResist)
				pInfo->nLightDef = pNpc->m_CurrentLightResist;
			else
				pInfo->nLightDef = pNpc->m_CurrentLightResistMax;
			//»ðÑæ·ÀÓù
			if (pNpc->m_CurrentFireResistMax >= pNpc->m_CurrentFireResist)
				pInfo->nFireDef = pNpc->m_CurrentFireResist;
			else
				pInfo->nFireDef = pNpc->m_CurrentFireResistMax;
			//¶¾ËØ·ÀÓù
			if (pNpc->m_CurrentPoisonResistMax >= pNpc->m_CurrentPoisonResist)
				pInfo->nPoisonDef = pNpc->m_CurrentPoisonResist;
			else
				pInfo->nPoisonDef = pNpc->m_CurrentPoisonResistMax;
			
			pInfo->nLevel = pNpc->m_Level;

			// ¸ù¾ÝÍæ¼Ò×´Ì¬ÏÔÊ¾ ×´Ì¬ÎÄ×Ö not end ²»Ó¦¸ÃÊÇ×´Ì¬£¬Ó¦¸ÃÊÇÎåÐÐÊôÐÔ spe
			memset(pInfo->StatusDesc, 0, sizeof(pInfo->StatusDesc));
			switch(pNpc->m_Series)
			{
			case series_water:
				strcpy(pInfo->StatusDesc, "HÖ Thñy");
				break;
			case series_wood:
				strcpy(pInfo->StatusDesc, "HÖ Méc");
				break;
			case series_metal:
				strcpy(pInfo->StatusDesc, "HÖ Kim");
				break;
			case series_fire:
				strcpy(pInfo->StatusDesc, "HÖ Háa");
				break;
			case series_earth:
				strcpy(pInfo->StatusDesc, "HÖ Thæ");
				break;
			}
		}
		break;

	//Ö÷½ÇµÄÁ¢¼´Ê¹ÓÃÎïÆ·ÓëÎä¹¦
	//uParam = (KUiPlayerImmedItemSkill*)pInfo
	case GDI_PLAYER_IMMED_ITEMSKILL:
		if (uParam)
		{
			KUiPlayerImmedItemSkill* pInfo = (KUiPlayerImmedItemSkill*)uParam;
			memset(pInfo,0,sizeof(KUiPlayerImmedItemSkill));
			pInfo->IMmediaSkill[0].uGenre	= CGOG_SKILL_SHORTCUT;
			pInfo->IMmediaSkill[0].uId		= Player[CLIENT_PLAYER_INDEX].GetLeftSkill();
			pInfo->IMmediaSkill[1].uGenre	= CGOG_SKILL_SHORTCUT;
			pInfo->IMmediaSkill[1].uId		= Player[CLIENT_PLAYER_INDEX].GetRightSkill();

			for (int i = 0; i < MAX_IMMEDIACY_ITEM; i++)
			{
				pInfo->ImmediaItem[i].uId = Player[CLIENT_PLAYER_INDEX].m_ItemList.m_Room[room_immediacy].FindItem(i, 0);
				if (pInfo->ImmediaItem[i].uId > 0)
				{
					pInfo->ImmediaItem[i].uGenre = CGOG_ITEM;
				}
				else
				{
					pInfo->ImmediaItem[i].uGenre = CGOG_NOTHING;
				}
			}
		}
		break;

	//Ö÷½ÇËæÉíÐ¯´øµÄÇ®
	//nRet = Ö÷½ÇËæÉíÐ¯´øµÄÇ®
	case GDI_PLAYER_HOLD_MONEY:	
		nRet = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetMoney(room_equipment);
		break;
	//Ö÷½ÇËæÉíÐ¯´øµÄÎïÆ·µÈ
	//uParam = (KUiObjAtRegion*) pInfo -> KUiObjAtRegion½á¹¹Êý×éµÄÖ¸Õë£¬KUiObjAtRegion
	//				½á¹¹ÓÃÓÚ´æ´¢ÎïÆ·»òÕßÎä¹¦µÄÊý¾Ý¼°Æä·ÅÖÃÇøÓòÎ»ÖÃÐÅÏ¢¡£
	//nParam = pInfoÊý×éÖÐ°üº¬KUiObjAtRegion½á¹¹µÄÊýÄ¿
	//Return = Èç¹û·µ»ØÖµÐ¡ÓÚµÈÓÚ´«Èë²ÎÊýnParam£¬ÆäÖµ±íÊ¾pInfoÊý×éÖÐµÄÇ°¶àÉÙ¸öKUiObjAtRegion
	//			½á¹¹±»Ìî³äÁËÓÐÐ§µÄÊý¾Ý£»·ñÔò±íÊ¾ÐèÒª´«Èë°üº¬¶àÉÙ¸öKUiObjAtRegion½á¹¹µÄÊý×é
	//			²Å¹»´æ´¢È«²¿µÄËæÉíÐ¯´øµÄÎïÆ·ÐÅÏ¢¡£

	// flyingÌí¼ÓÁËÒ»¸öÊÇ·ñ¿ÉÒÔÆïÂíµÄÇëÇó£¬ÔÚÕâÀïÊµÏÖ£¬gg¿ÉÀÖÒ»¹Þ£¬mmÇë³ÔKFCÅ¶
	case GDI_GET_PLAYERNPC_INDEX:
		nRet = Player[CLIENT_PLAYER_INDEX].m_nIndex;
		if (uParam)
		{
			UINT* pP = (UINT*)uParam;
			*pP = Player[CLIENT_PLAYER_INDEX].m_dwID;
		}
		break;

	case GDI_ITEM_TAKEN_WITH:
		nRet = 0;
		if (uParam)
		{
			int nCount = 0;
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*)uParam;
			PlayerItem* pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetFirstItem();
			if (pItem && pItem->nPlace == pos_equiproom)
			{
				pInfo->Obj.uGenre = CGOG_ITEM;
				pInfo->Obj.uId = pItem->nIdx;
				pInfo->Region.h = pItem->nX;
				pInfo->Region.v = pItem->nY;
				pInfo->Region.Width = Item[pItem->nIdx].GetWidth();
				pInfo->Region.Height = Item[pItem->nIdx].GetHeight();
				nCount++;
				pInfo++;
			}
			while(pItem)
			{
				pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetNextItem();
				if (pItem && pItem->nPlace == pos_equiproom)
				{
					pInfo->Obj.uGenre = CGOG_ITEM;
					pInfo->Obj.uId = pItem->nIdx;		
					pInfo->Region.h = pItem->nX;
					pInfo->Region.v = pItem->nY;
					pInfo->Region.Width = Item[pItem->nIdx].GetWidth();
					pInfo->Region.Height = Item[pItem->nIdx].GetHeight();
					nCount++;
					pInfo++;
				}
				if (nCount > nParam)
					break;
			}
			nRet = nCount;
		}
		else
		{
			int nCount = 0;
			PlayerItem* pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetFirstItem();
			if (pItem && pItem->nPlace == pos_equiproom)
				nCount++;
			while(pItem)
			{
				pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetNextItem();
				if (pItem && pItem->nPlace == pos_equiproom)
					nCount++;
			}
			nRet = nCount;
		}
		break;

	//Ö÷½Ç×°±¸ÎïÆ·
	//uParam = (KUiObjAtRegion*)pInfo -> °üº¬10¸öÔªËØµÄKUiObjAtRegion½á¹¹Êý×éÖ¸Õë£¬
	//			KUiObjAtRegion½á¹¹ÓÃÓÚ´æ´¢×°±¸µÄÊý¾ÝºÍ·ÅÖÃÎ»ÖÃÐÅÏ¢¡£
	//			KUiObjAtRegion::Region::h ±íÊ¾ÊôÓÚµÚ¼¸Ì××°±¸
	//			KUiObjAtRegion::Region::v ±íÊ¾ÊôÓÚÄÄ¸öÎ»ÖÃµÄ×°±¸,ÆäÖµÎªÃ·¾ÙÀàÐÍ
	//			UI_EQUIPMENT_POSITIONµÄÈ¡ÖµÖ®Ò»¡£Çë²Î¿´UI_EQUIPMENT_POSITIONµÄ×¢ÊÍ¡£
	//nParam =	Òª»ñÈ¡µÄÊÇµÚ¼¸Ì××°±¸ÐÅÏ¢
	//Return =  ÆäÖµ±íÊ¾pInfoÊý×éÖÐµÄÇ°¶àÉÙ¸öKUiObjAtRegion½á¹¹±»Ìî³äÁËÓÐÐ§µÄÊý¾Ý¡£
	case GDI_EQUIPMENT:
		nRet = 0;
		if (uParam)
		{
			// TODO£ºÔÝÊ±Ã»ÓÐ×öµÚ¶þÌ××°±¸
			if (nParam == 1)
				break;

			int PartConvert[itempart_num] = 
			{
				UIEP_HEAD,		UIEP_BODY,
				UIEP_WAIST,		UIEP_HAND,
				UIEP_FOOT,		UIEP_FINESSE,
				UIEP_NECK,		UIEP_FINGER1,
				UIEP_FINGER2,	UIEP_WAIST_DECOR,
				UIEP_HORSE,
				UIEP_MASK,		//mÆt n¹
				UIEP_MANTLE,	//phi phong
				UIEP_SIGNET,	//Ên
				UIEP_SHIPIN,	//trang søc
			};

			int nCount = 0;
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*)uParam;

			for (int i = 0; i < itempart_num; i++)
			{
				pInfo->Obj.uId = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipment(i);
				if (pInfo->Obj.uId)
				{
					pInfo->Obj.uGenre = CGOG_ITEM;
				
					pInfo->Region.Width = Item[pInfo->Obj.uId].GetWidth();
					pInfo->Region.Height = Item[pInfo->Obj.uId].GetHeight();
					pInfo->Region.h = 0;
					pInfo->Region.v = PartConvert[i];
				}
				else
				{
					pInfo->Obj.uGenre = CGOG_NOTHING;
				}
				nCount++;
				pInfo++;
			}
			nRet = nCount;
		}
		break;
	case GDI_PARADE_EQUIPMENT:
		nRet = 0;
		if (uParam)
		{
			// TODO£ºÔÝÊ±Ã»ÓÐ×öµÚ¶þÌ××°±¸
			if (nParam == 1)
				break;

			int PartConvert[itempart_num] = 
			{
				UIEP_HEAD,		UIEP_BODY,
				UIEP_WAIST,		UIEP_HAND,
				UIEP_FOOT,		UIEP_FINESSE,
				UIEP_NECK,		UIEP_FINGER1,
				UIEP_FINGER2,	UIEP_WAIST_DECOR,
				UIEP_HORSE,
				UIEP_MASK,		//mÆt n¹
				UIEP_MANTLE,	//phi phong
				UIEP_SIGNET,	//Ên
				UIEP_SHIPIN,	//trang søc
			};

			int nCount = 0;
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*)uParam;

			for (int i = 0; i < itempart_num; i++)
			{
				pInfo->Obj.uId = g_cViewItem.m_sItem[i];
				if (pInfo->Obj.uId)
				{
					pInfo->Obj.uGenre = CGOG_ITEM;
				
					pInfo->Region.Width = Item[pInfo->Obj.uId].GetWidth();
					pInfo->Region.Height = Item[pInfo->Obj.uId].GetHeight();
					pInfo->Region.h = 0;
					pInfo->Region.v = PartConvert[i];
				}
				else
				{
					pInfo->Obj.uGenre = CGOG_NOTHING;
				}
				nCount++;
				pInfo++;
			}
			nRet = nCount;
		}
		break;
		
	//½»Ò×²Ù×÷Ïà¹ØµÄÊý¾Ý
	//uParam = (UI_TRADE_OPER_DATA)eOper ¾ßÌåº¬Òå¼ûUI_TRADE_OPER_DATA
	//nParam ¾ßÌåÓ¦ÓÃÓëº¬ÒåÓÉuParamµÄÈ¡Öµ×´¿ö¾ö¶¨,¼ûUI_TRADE_OPER_DATAµÄËµÃ÷
	//Return ¾ßÌåº¬ÒåÓÉuParamµÄÈ¡Öµ×´¿ö¾ö¶¨,¼ûUI_TRADE_OPER_DATAµÄËµÃ÷
	//Ö÷½ÇµÄÉú»î¼¼ÄÜÊý¾Ý
	case GDI_TRADE_OPER_DATA:
		if (uParam == UTOD_IS_LOCKED)
			nRet = Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeLock;
		else if (uParam == UTOD_IS_TRADING)
			nRet = Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeState;
		else if (uParam == UTOD_IS_OTHER_LOCKED)
			nRet = Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeDestLock;
		else if (uParam == UTOD_IS_WILLING)
		{
			nRet = (Player[CLIENT_PLAYER_INDEX].m_cMenuState.m_nState == PLAYER_MENU_STATE_TRADEOPEN);
		}
		break;

	//uParam = (KUiPlayerLiveSkillBase*) pInfo -> Ö÷½ÇµÄÉú»î¼¼ÄÜÊý¾Ý
	case GDI_LIVE_SKILL_BASE:
		if (uParam)
		{
			KUiPlayerLiveSkillBase* pInfo = (KUiPlayerLiveSkillBase*)uParam;
			//to do:no implements in this version;
			pInfo->nLiveExperience = 0 ;
			pInfo->nRemainPoint = 0 ;
			pInfo->nLiveExperienceFull = 0 ;
			memset(pInfo,0,sizeof(KUiPlayerLiveSkillBase));
		}
		break;

	//Ö÷½ÇÕÆÎÕµÄ¸÷ÏîÉú»î¼¼ÄÜ
	//uParam = (unsigned int*) pSkills -> °üº¬10¸öunsigned intµÄÊý×éÓÃÓÚ´æ´¢¸÷ÏîÉú»î¼¼ÄÜµÄid¡£
	case GDI_LIVE_SKILLS:
		if (uParam)
		{
			//to do:no implements in this version;
			KUiSkillData* pInfo = (KUiSkillData*)uParam;
			memset(pInfo,0,sizeof(KUiSkillData)*10);
		}
		break;

	//Ê£ÓàÕ½¶·¼¼ÄÜµãÊý
	//Return = Ê£ÓàÕ½¶·¼¼ÄÜµãÊý
	case GDI_FIGHT_SKILL_POINT:
		//to do:no implements in this version;
		nRet = Player[CLIENT_PLAYER_INDEX].m_nSkillPoint;
		break;

	//Ö÷½ÇÕÆÎÕµÄ¸÷ÏîÕ½¶·¼¼ÄÜ
	//uParam = (unsigned int*) pSkills -> °üº¬30¸öunsigned intµÄÊý×éÓÃÓÚ´æ´¢¸÷ÏîÕ½¶·¼¼ÄÜµÄid¡£
	case GDI_FIGHT_SKILLS:
		if (uParam)
		{
			KUiSkillData* pSkills = (KUiSkillData*)uParam;
			Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_SkillList.GetSkillSortList(pSkills);
		}
		break;
	//ÏÔÊ¾×ó¼ü¼¼ÄÜÁÐ±í
	//uParam = (KUiSkillData*) pSkills -> °üº¬65¸öKUiSkillDataµÄÊý×éÓÃÓÚ´æ´¢¸÷¼¼ÄÜµÄÊý¾Ý¡£
	//								KUiSkillData::nLevelÓÃÀ´±íÊö¼¼ÄÜÏÔÊ¾ÔÚµÚ¼¸ÐÐ
	//Return = ·µ»ØÓÐÐ§Êý¾ÝµÄSkillsµÄÊýÄ¿
	case GDI_LEFT_ENABLE_SKILLS:
		{
			KUiSkillData * pSkills = (KUiSkillData*)uParam;
			int nCount = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_SkillList.GetLeftSkillSortList(pSkills);
			return nCount;

		}
		break;
	//ÏÔÊ¾ÓÒ¼ü¼¼ÄÜÁÐ±í
	//uParam = (KUiSkillData*) pSkills -> °üº¬65¸öKUiSkillDataµÄÊý×éÓÃÓÚ´æ´¢¸÷¼¼ÄÜµÄÊý¾Ý¡£
	//								KUiSkillData::nLevelÓÃÀ´±íÊö¼¼ÄÜÏÔÊ¾ÔÚµÚ¼¸ÐÐ
	//Return = ·µ»ØÓÐÐ§Êý¾ÝµÄSkillsµÄÊýÄ¿
	case GDI_RIGHT_ENABLE_SKILLS:
		{
			KUiSkillData * pSkills = (KUiSkillData*)uParam;
			int nCount = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_SkillList.GetRightSkillSortList(pSkills);
			return nCount;
		}
		break;
	//Ö÷½ÇµÄ×Ô´´Îä¹¦
	//uParam = (unsigned int*) pSkills -> °üº¬5¸öunsigned intµÄÊý×éÓÃÓÚ´æ´¢¸÷Ïî×Ô´´Îä¹¦µÄid¡£
	case GDI_CUSTOM_SKILLS:
		if (uParam)
		{
			//to do: no implements in this version;
			KUiSkillData* pSkills = (KUiSkillData*)uParam;
			memset(pSkills,0,sizeof(KUiSkillData)*5);
		}
		break;
	//»ñÈ¡ÖÜÎ§Íæ¼ÒµÄÁÐ±í
	//uParam = (KUiPlayerItem*)pList -> ÈËÔ±ÐÅÏ¢ÁÐ±í
	//			KUiPlayerItem::nData = 0
	//nParam = pListÊý×éÖÐ°üº¬KUiPlayerItem½á¹¹µÄÊýÄ¿
	//Return = Èç¹û·µ»ØÖµÐ¡ÓÚµÈÓÚ´«Èë²ÎÊýnParam£¬ÆäÖµ±íÊ¾pListÊý×éÖÐµÄÇ°¶àÉÙ¸öKUiPlayerItem
	//			½á¹¹±»Ìî³äÁËÓÐÐ§µÄÊý¾Ý£»·ñÔò±íÊ¾ÐèÒª´«Èë°üº¬¶àÉÙ¸öKUiPlayerItem½á¹¹µÄÊý×é
	//			²Å¹»´æ´¢È«²¿ÈËÔ±ÐÅÏ¢¡£
	case GDI_NEARBY_PLAYER_LIST:
		nRet = NpcSet.GetAroundPlayer((KUiPlayerItem*)uParam, nParam);
		break;

	//»ñÈ¡ÖÜÎ§¹Âµ¥¿ÉÊÜÑûÇëµÄÍæ¼ÒµÄÁÐ±í
	//²ÎÊýº¬ÒåÍ¬GDI_NEARBY_PLAYER_LIST
	case GDI_NEARBY_IDLE_PLAYER_LIST:
		nRet = NpcSet.GetAroundPlayerForTeamInvite((KUiPlayerItem*)uParam, nParam);
		break;

	//Ö÷½ÇÍ³Ë§ÄÜÁ¦Ïà¹ØµÄÊý¾Ý
	//uParam = (KUiPlayerLeaderShip*) -> Ö÷½ÇÍ³Ë§ÄÜÁ¦Ïà¹ØµÄÊý¾Ý½á¹¹Ö¸Õë
	case GDI_PLAYER_LEADERSHIP:
		if (uParam)
		{
			KUiPlayerLeaderShip* pInfo = (KUiPlayerLeaderShip*)uParam ;
			pInfo->nLeaderShipExperience = Player[CLIENT_PLAYER_INDEX].m_dwLeadExp ;		//Í³Ë§Á¦¾­ÑéÖµ
			//to do: waiting for...;
			pInfo->nLeaderShipExperienceFull = Player[CLIENT_PLAYER_INDEX].m_dwNextLevelLeadExp;//Í³Ë§Á¦¾­ÑéÖµ
			pInfo->nLeaderShipLevel = Player[CLIENT_PLAYER_INDEX].m_dwLeadLevel ;			//Í³Ë§Á¦µÈ¼¶
		}
		break;
	//»ñµÃÎïÆ·ÔÚÄ³¸ö»·¾³Î»ÖÃµÄÊôÐÔ×´Ì¬
	//uParam = (KUiGameObject*)pObj£¨µ±nParam==0Ê±£©ÎïÆ·µÄÐÅÏ¢
	//uParam = (KUiObjAtContRegion*)pObj£¨µ±nParam!=0Ê±£©ÎïÆ·µÄÐÅÏ¢
	//nParam = (int)(bool)bJustTry  ÊÇ·ñÖ»ÊÇ³¢ÊÔ·ÅÖÃ
	//Return = (ITEM_IN_ENVIRO_PROP)eProp ÎïÆ·µÄÊôÐÔ×´Ì¬
	case GDI_ITEM_IN_ENVIRO_PROP:
		{
			if (!nParam)
			{
				KUiGameObject *pObj = (KUiGameObject *)uParam;
				if (pObj->uGenre != CGOG_ITEM && pObj->uGenre != CGOG_NPCSELLITEM)
					break;

				KItem* pItem = NULL;

				if (pObj->uGenre == CGOG_ITEM && pObj->uId > 0 && pObj->uId < MAX_ITEM)
				{
					pItem = &Item[pObj->uId];
				}
				else if (pObj->uGenre == CGOG_NPCSELLITEM)
				{
					int nIdx = BuySell.GetItemIndex(Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx, pObj->uId);
					pItem = BuySell.GetItem(nIdx);
				}

				_ASSERT(pItem);
				if (!pItem || pItem->GetGenre() != item_equip)
					break;

				if (Player[CLIENT_PLAYER_INDEX].m_ItemList.CanEquip(pItem))
				{
					nRet = IIEP_NORMAL;
				}
				else
				{
					nRet = IIEP_NOT_USEABLE;
				}
			}
			else
			{
				KUiObjAtContRegion *pObj = (KUiObjAtContRegion *)uParam;
				if (pObj->Obj.uGenre != CGOG_ITEM || pObj->Obj.uId >= MAX_ITEM)
					break;

				int PartConvert[itempart_num] = 
				{
					itempart_head,		itempart_weapon,
					itempart_amulet,	itempart_cuff,
					itempart_body,		itempart_belt,
					itempart_ring1,		itempart_ring2,
					itempart_pendant,	itempart_foot,
					itempart_horse,
					itempart_mask,
					itempart_mantle,
					itempart_signet,
					itempart_shipin,
				};

				_ASSERT(pObj->eContainer < itempart_num);
				if (pObj->eContainer >= itempart_num || pObj->eContainer < 0)
					break;

				if (Item[pObj->Obj.uId].GetGenre() != item_equip)
					break;

				int nPlace = PartConvert[pObj->eContainer];

				if (Player[CLIENT_PLAYER_INDEX].m_ItemList.CanEquip(pObj->Obj.uId, nPlace))
				{
					nRet = IIEP_NORMAL;
				}
				else
				{
					nRet = IIEP_NOT_USEABLE;
				}
			}
		}
		break;
	case GDI_GET_ITEM_INFO:
		{
			if(!uParam)
			{
				if(nParam > 0 && nParam < MAX_ITEM)
					nRet = Item[nParam].GetGenre();
				else
					nRet = -1;
			}
			else if(uParam == 1)
			{
				if(nParam > 0 && nParam < MAX_ITEM)
					nRet = Item[nParam].GetExType();
			}
		}break;
	case GDI_GET_GAME_TIME:
		nRet = g_SubWorldSet.GetGameTime();
		break;
	case GDI_IMMEDIATEITEM_NUM:
		if (uParam >= 0 && uParam < 3)
			nRet = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetSameDetailItemNum(uParam);
		break;
	//ÓëNPCµÄÂòÂô
	//uParam = (KUiObjAtContRegion*) pInfo -> KUiObjAtRegion½á¹¹Êý×éµÄÖ¸Õë£¬KUiObjAtRegion
	//				½á¹¹ÓÃÓÚ´æ´¢ÎïÆ·µÄÊý¾Ý¼°Æä·ÅÖÃÇøÓòÎ»ÖÃÐÅÏ¢¡£
	//				ÆäÖÐKUiObjAtContRegion::nContainerÖµ±íÊ¾µÚ¼¸Ò³µÄÎïÆ·
	//nParam = pInfoÊý×éÖÐ°üº¬KUiObjAtRegion½á¹¹µÄÊýÄ¿
	//Return = Èç¹û·µ»ØÖµÐ¡ÓÚµÈÓÚ´«Èë²ÎÊýnParam£¬ÆäÖµ±íÊ¾pInfoÊý×éÖÐµÄÇ°¶àÉÙ¸öKUiObjAtRegion
	//			½á¹¹±»Ìî³äÁËÓÐÐ§µÄÊý¾Ý£»·ñÔò±íÊ¾ÐèÒª´«Èë°üº¬¶àÉÙ¸öKUiObjAtRegion½á¹¹µÄÊý×é
	//			²Å¹»´æ´¢È«²¿µÄnpcÁÐ³öÀ´½»Ò×µÄÎïÆ·ÐÅÏ¢¡£
	case GDI_TRADE_NPC_ITEM:
		nRet = 0;
		if (uParam)
		{
			int nCount = 0;
			int nPage = 0;
			int nIndex = 0;
			int	nBuyIdx = Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx;
			KUiObjAtContRegion* pInfo = (KUiObjAtContRegion *)uParam;

			if (nBuyIdx == -1)
				break;
			if (nBuyIdx >= BuySell.GetHeight())
				break;
			if (!BuySell.m_pShopRoom)
				break;
			BuySell.m_pShopRoom->Clear();
			for (int i = 0; i < BuySell.GetWidth(); i++)
			{
				nIndex = BuySell.GetItemIndex(nBuyIdx, i);
				KItem* pItem = BuySell.GetItem(nIndex);
				
				if (nIndex >= 0 && pItem)
				{
					// Set pInfo->Obj.uGenre
					pInfo->Obj.uGenre = CGOG_NPCSELLITEM;
					// Set pInfo->Obj.uId
					pInfo->Obj.uId = i;

					POINT	Pos;
					if (BuySell.m_pShopRoom->FindRoom(pItem->GetWidth(), pItem->GetHeight(), &Pos))
					{
						// nIndex + 1±£Ö¤²»Îª0
						BuySell.m_pShopRoom->PlaceItem(Pos.x, Pos.y, nIndex + 1, pItem->GetWidth(), pItem->GetHeight());
					}
					else
					{
						nPage++;
						BuySell.m_pShopRoom->Clear();
						// ClearÍê³Éºó±ØÈ»³É¹¦£¬ËùÒÔÃ»ÓÐÅÐ¶Ï
						BuySell.m_pShopRoom->FindRoom(pItem->GetWidth(), pItem->GetHeight(), &Pos);
						BuySell.m_pShopRoom->PlaceItem(Pos.x, Pos.y, nIndex + 1, pItem->GetWidth(), pItem->GetHeight());
					}
					pInfo->Region.h = Pos.x;
					pInfo->Region.v = Pos.y;
					pInfo->Region.Width = pItem->GetWidth();
					pInfo->Region.Height = pItem->GetHeight();
					pInfo->nContainer = nPage;
					nCount++;
					pInfo++;
				}
			}			
			nRet = nCount;
		}
		else
		{
			int nCount = 0;
			int nIndex = 0;
			int	nBuyIdx = Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx;
			if (nBuyIdx == -1)
				break;
			if (nBuyIdx >= BuySell.GetHeight())
				break;
			for (int i = 0; i < BuySell.GetWidth(); i++)
			{
				nIndex = BuySell.GetItemIndex(nBuyIdx, i);
				KItem* pItem = BuySell.GetItem(nIndex);
				
				if (nIndex >= 0 && pItem)
				{
					nCount++;
				}
			}
			nRet = nCount;
		}
		break;
	case GDI_ITEM_IN_STORE_BOX:
		nRet = 0;
		if (uParam)
		{
			int nCount = 0;
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*)uParam;
			pInfo->Obj.uGenre = CGOG_MONEY;
			pInfo->Obj.uId = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetMoney(room_repository);
			nCount++;
			pInfo++;
			PlayerItem* pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetFirstItem();
			if (pItem && pItem->nPlace == pos_repositoryroom)
			{
				pInfo->Obj.uGenre = CGOG_ITEM;
				pInfo->Obj.uId = pItem->nIdx;
				pInfo->Region.h = pItem->nX;
				pInfo->Region.v = pItem->nY;
				pInfo->Region.Width = Item[pItem->nIdx].GetWidth();
				pInfo->Region.Height = Item[pItem->nIdx].GetHeight();
				nCount++;
				pInfo++;
			}
			while(pItem)
			{
				pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetNextItem();
				if (pItem && pItem->nPlace == pos_repositoryroom)
				{
					pInfo->Obj.uGenre = CGOG_ITEM;
					pInfo->Obj.uId = pItem->nIdx;		
					pInfo->Region.h = pItem->nX;
					pInfo->Region.v = pItem->nY;
					pInfo->Region.Width = Item[pItem->nIdx].GetWidth();
					pInfo->Region.Height = Item[pItem->nIdx].GetHeight();
					nCount++;
					pInfo++;
				}
				if (nCount > nParam)
					break;
			}
			nRet = nCount;
		}
		else
		{
			int nCount = 0;
			// µÚÒ»¸öÊÇÇ®
			nCount++;
			// ºóÃæÊÇ×°±¸
			PlayerItem* pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetFirstItem();
			if (pItem && pItem->nPlace == pos_repositoryroom)
				nCount++;
			while(pItem)
			{
				pItem = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetNextItem();
				if (pItem && pItem->nPlace == pos_repositoryroom)
					nCount++;
			}
			nRet = nCount;
		}
		break;
	case GDI_PK_SETTING:					//»ñÈ¡pkÉèÖÃ
		nRet = Player[CLIENT_PLAYER_INDEX].m_cPK.GetNormalPKState();
		break;
	case GDI_SHOW_PLAYERS_NAME:			//»ñÈ¡ÏÔÊ¾¸÷Íæ¼ÒÈËÃû
		nRet = NpcSet.CheckShowName();
		break;
	case GDI_SHOW_PLAYERS_LIFE:			//»ñÈ¡ÏÔÊ¾¸÷Íæ¼ÒÉúÃü
		nRet = NpcSet.CheckShowLife();
		break;
	case GDI_SHOW_PLAYERS_MANA:			//»ñÈ¡ÏÔÊ¾¸÷Íæ¼ÒÄÚÁ¦
		nRet = NpcSet.CheckShowMana();
		break;
	case GDI_IS_PLAYER_GM:
		nRet = Player[CLIENT_PLAYER_INDEX].m_bGM;
		break;
	}
	return nRet;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÏòÓÎÏ··¢ËÍ²Ù×÷
//	²ÎÊý£ºunsigned int uDataId --> CoreÍâ²¿¿Í»§¶ÔcoreµÄ²Ù×÷ÇëÇóµÄË÷Òý¶¨Òå
//							ÆäÖµÎªÃ·¾ÙÀàÐÍGAMEOPERATION_INDEXµÄÈ¡ÖµÖ®Ò»¡£
//		  unsigned int uParam  --> ÒÀ¾ÝuOperIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//		  int nParam --> ÒÀ¾ÝuOperIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//	·µ»Ø£ºÈç¹û³É¹¦·¢ËÍ²Ù×÷ÇëÇó£¬º¯Êý·µ»Ø·Ç0Öµ£¬·ñÔò·µ»Ø0Öµ¡£
//--------------------------------------------------------------------------
int	KCoreShell::OperationRequest(unsigned int uOper, unsigned int uParam, int nParam)
{
	int nRet = 1;
	switch(uOper)
	{
	case GOI_QUERY_RANK_INFORMATION:
		SendClientCmdQueryLadder(uParam);
		break;
	//uParam = (const char*)pszFileName
	case GOI_PLAY_SOUND:
		if (uParam)
		{
			static KCacheNode* pSndNode = NULL;
			KWavSound* pSound = NULL;
			pSndNode	= (KCacheNode*)g_SoundCache.GetNode((char *)uParam, (KCacheNode * )pSndNode);
			pSound		= (KWavSound*)pSndNode->m_lpData;
			if (pSound)
			{
				if (pSound->IsPlaying())
					break;
				pSound->Play(0, -10000 + Option.GetSndVolume() * 100, 0);
			}
		}
		break;
	case GOI_PLAYER_RENASCENCE:
		{
			int nReviveType;
			if (nParam)	// bBackTown
			{
				nReviveType = REMOTE_REVIVE_TYPE;
			}
			else
			{
				nReviveType = LOCAL_REVIVE_TYPE;
			}
//			SendClientCmdRevive(nReviveType);
			SendClientCmdRevive();
		}
		break;
	case GOI_MONEY_INOUT_STORE_BOX:
		{
			BOOL	bIn = (BOOL)uParam;
			int		nMoney = nParam;
			int		nSrcRoom, nDesRoom;


			if (bIn)
			{
				nSrcRoom = room_equipment;
				nDesRoom = room_repository;
			}
			else
			{
				nDesRoom = room_equipment;
				nSrcRoom = room_repository;
			}
			Player[CLIENT_PLAYER_INDEX].m_ItemList.ExchangeMoney(nSrcRoom, nDesRoom, nMoney);
		}
		break;
		//Àë¿ªÓÎÏ·
	case GOI_EXIT_GAME:
		g_SubWorldSet.Close();
		g_ScenePlace.ClosePlace();
		break;
	case GOI_GAMESPACE_DISCONNECTED:
		Player[CLIENT_PLAYER_INDEX].m_sExtAuto.nHomeStep = 0;
		Player[CLIENT_PLAYER_INDEX].m_sExtAuto.nSubStep = 0;
		g_SubWorldSet.Close();
		break;
	case GOI_TRADE_NPC_BUY:
		{
			KUiObjAtContRegion* pObject1 = (KUiObjAtContRegion*)uParam;
			if (CGOG_NPCSELLITEM != pObject1->Obj.uGenre)
					break;

			int nIdx = 0;
			KItem* pItem = NULL;

			nIdx = BuySell.GetItemIndex(Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx, pObject1->Obj.uId);
			pItem = BuySell.GetItem(nIdx);

			int nWidth, nHeight;
			ItemPos	Pos;

			nWidth = pItem->GetWidth();
			nHeight = pItem->GetHeight();
			if (!Player[CLIENT_PLAYER_INDEX].m_ItemList.SearchPosition(nWidth, nHeight, &Pos))
			{
				nRet = 0;
				break;
			}
			if (Pos.nPlace != pos_equiproom)
			{
				nRet = 0;

				KSystemMessage	sMsg;
				
				strcpy(sMsg.szMessage, MSG_SHOP_NO_ROOM);
				sMsg.eType = SMT_SYSTEM;
				sMsg.byConfirmType = SMCT_CLICK;
				sMsg.byPriority = 1;
				sMsg.byParamSize = 0;
				CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
				break;
			}

			if (Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
			{
				nRet = 0;
				KSystemMessage	sMsg;
				
				strcpy(sMsg.szMessage, MSG_SHOP_NO_MONEY);
				sMsg.eType = SMT_SYSTEM;
				sMsg.byConfirmType = SMCT_CLICK;
				sMsg.byPriority = 1;
				sMsg.byParamSize = 0;
				CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
				break;
			}
			SendClientCmdBuy(pObject1->Obj.uId, pos_equiproom, Pos.nX, Pos.nY);
		}
		break;
	case GOI_TRADE_NPC_SELL:
		{
			KUiObjAtContRegion* pObject1 = (KUiObjAtContRegion*)uParam;

			if (CGOG_ITEM != pObject1->Obj.uGenre)
				break;
			//·ÅÏÂÈ¥µÄ¶«Î÷²»Îª¿Õ£¬ËùÒÔÊÇÂô¶«Î÷
			int nIdx = pObject1->Obj.uId;	//Player[CLIENT_PLAYER_INDEX].m_ItemList.Hand();
			if (nIdx > 0 && nIdx < MAX_ITEM)
			{
				if (Item[nIdx].GetGenre() == item_task)
				{
					KSystemMessage	sMsg;
					sprintf(sMsg.szMessage, MSG_TRADE_TASK_ITEM);
					sMsg.eType = SMT_NORMAL;
					sMsg.byConfirmType = SMCT_NONE;
					sMsg.byPriority = 0;
					sMsg.byParamSize = 0;
					CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
					return 0;
				}
				SendClientCmdSell(Item[nIdx].GetID());
				return 1;
			}
			else
			{
				return 0;
			}
		}
		break;
	case GOI_TRADE_NPC_REPAIR:
		{
			KUiObjAtContRegion* pObject1 = (KUiObjAtContRegion*)uParam;

			if (CGOG_ITEM != pObject1->Obj.uGenre)
				break;
			//·ÅÏÂÈ¥µÄ¶«Î÷²»Îª¿Õ£¬ËùÒÔÊÇÂô¶«Î÷
			int nIdx = pObject1->Obj.uId;	//Player[CLIENT_PLAYER_INDEX].m_ItemList.Hand();
			if (nIdx > 0 && nIdx < MAX_ITEM)
			{		
				if (Item[nIdx].GetGenre() != item_equip)
				{
/*					KSystemMessage	sMsg;
					sprintf(sMsg.szMessage, MSG_TRADE_TASK_ITEM);
					sMsg.eType = SMT_NORMAL;
					sMsg.byConfirmType = SMCT_NONE;
					sMsg.byPriority = 0;
					sMsg.byParamSize = 0;
					CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);*/
					return 0;
				}
				else if (Item[nIdx].GetDurability() == -1 || Item[nIdx].GetDurability() == Item[nIdx].GetMaxDurability())
				{
					return 0;
				}
				else if (Item[nIdx].GetRepairPrice() <= Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipmentMoney())
				{
					SendClientCmdRepair(Item[nIdx].GetID());
				}
				else
				{
					KSystemMessage	sMsg;
					sprintf(sMsg.szMessage, MSG_SHOP_NO_MONEY);
					sMsg.eType = SMT_NORMAL;
					sMsg.byConfirmType = SMCT_NONE;
					sMsg.byPriority = 0;
					sMsg.byParamSize = 0;
					CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
					return 0;
				}
				
				return 1;
			}
			else
			{
				return 0;
			}
		}
		break;
	case GOI_SWITCH_OBJECT:
		//uParam = (KUiObjAtContRegion*)pObject1 -> ÄÃÆðµÄÎïÆ·²Ù×÷Ç°µÄÐÅÏ¢
		//nParam = (KUiObjAtContRegion*)pObject2 -> ·ÅÏÂµÄÎïÆ·²Ù×÷ºóµÄÐÅÏ¢
		{
			ItemPos	P1, P2;
			int PartConvert[itempart_num] = 
			{
				itempart_head,		itempart_weapon,
				itempart_amulet,	itempart_cuff,
				itempart_body,		itempart_belt,
				itempart_ring1,		itempart_ring2,
				itempart_pendant,	itempart_foot,
				itempart_horse,
				itempart_mask,
				itempart_mantle,
				itempart_signet,
				itempart_shipin,
			};
			KUiObjAtContRegion* pObject1 = (KUiObjAtContRegion*)uParam;
			KUiObjAtContRegion* pObject2 = (KUiObjAtContRegion*)nParam;
			
			if (!pObject1 && !pObject2)
				break;
			
			if (pObject1)
			{
				switch(pObject1->eContainer)
				{
				case UOC_STORE_BOX:
					P1.nPlace = pos_repositoryroom;
					P1.nX = pObject1->Region.h;
					P1.nY = pObject1->Region.v;
					break;
				case UOC_IMMEDIA_ITEM:
					P1.nPlace = pos_immediacy;
					P1.nX = pObject1->Region.h;
					P1.nY = pObject1->Region.v;
					break;
				case UOC_ITEM_TAKE_WITH:
					P1.nPlace = pos_equiproom;
					P1.nX = pObject1->Region.h;
					P1.nY = pObject1->Region.v;
					break;
				case UOC_EQUIPTMENT:
					{
						// TODO:ÔÝÊ±Ã»ÓÐµÚ¶þ×°±¸
						if (pObject1->Region.h == 1)
							break;
						P1.nPlace = pos_equip;
						P1.nX = PartConvert[pObject1->Region.v];
					}
					break;
				case UOC_TO_BE_TRADE:
					P1.nPlace = pos_traderoom;
					P1.nX = pObject1->Region.h;
					P1.nY = pObject1->Region.v;
					break;
				case UOC_NPC_SHOP:
					if (CGOG_NPCSELLITEM != pObject1->Obj.uGenre)
						break;

					int nIdx = 0;
					KItem* pItem = NULL;
					
					nIdx = BuySell.GetItemIndex(Player[CLIENT_PLAYER_INDEX].m_BuyInfo.m_nBuyIdx, pObject1->Obj.uId);
					pItem = BuySell.GetItem(nIdx);
					
					int nWidth, nHeight;
					ItemPos	Pos;
					
					nWidth = pItem->GetWidth();
					nHeight = pItem->GetHeight();
					if (!Player[CLIENT_PLAYER_INDEX].m_ItemList.SearchPosition(nWidth, nHeight, &Pos))
					{
						nRet = 0;
						KSystemMessage	sMsg;
						
						strcpy(sMsg.szMessage, MSG_SHOP_NO_ROOM);
						sMsg.eType = SMT_SYSTEM;
						sMsg.byConfirmType = SMCT_CLICK;
						sMsg.byPriority = 1;
						sMsg.byParamSize = 0;
						CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
						break;
					}
					if (Pos.nPlace != pos_equiproom)
					{
						nRet = 0;
						KSystemMessage	sMsg;
						
						strcpy(sMsg.szMessage, MSG_SHOP_NO_ROOM);
						sMsg.eType = SMT_SYSTEM;
						sMsg.byConfirmType = SMCT_CLICK;
						sMsg.byPriority = 1;
						sMsg.byParamSize = 0;
						CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
						break;
					}
					if (Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
					{
						nRet = 0;
						KSystemMessage	sMsg;
						
						strcpy(sMsg.szMessage, MSG_SHOP_NO_MONEY);
						sMsg.eType = SMT_SYSTEM;
						sMsg.byConfirmType = SMCT_CLICK;
						sMsg.byPriority = 1;
						sMsg.byParamSize = 0;
						CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
						break;
					}
					// ÄÃÆðÀ´µÄ¶«Î÷²»Îª¿Õ£¬ËùÒÔÊÇÂò¶«Î÷
					SendClientCmdBuy(pObject1->Obj.uId, pos_equiproom, Pos.nX, Pos.nY);
					break;
				}
			}
			
			if (pObject2)
			{
				switch(pObject2->eContainer)
				{
				case UOC_STORE_BOX:
					P2.nPlace = pos_repositoryroom;
					P2.nX = pObject2->Region.h;
					P2.nY = pObject2->Region.v;
					break;
				case UOC_IMMEDIA_ITEM:
					P2.nPlace = pos_immediacy;
					P2.nX = pObject2->Region.h;
					P2.nY = pObject2->Region.v;
					break;
				case UOC_ITEM_TAKE_WITH:
					P2.nPlace = pos_equiproom;
					P2.nX = pObject2->Region.h;
					P2.nY = pObject2->Region.v;
					break;
				case UOC_EQUIPTMENT:
					{
						// TODO:ÔÝÊ±Ã»ÓÐµÚ¶þ×°±¸
						if (pObject2->Region.h == 1)
							break;
						P2.nPlace = pos_equip;
						P2.nX = PartConvert[pObject2->Region.v];
					}
					break;
				case UOC_TO_BE_TRADE:
					P2.nPlace = pos_traderoom;
					P2.nX = pObject2->Region.h;
					P2.nY = pObject2->Region.v;
					break;
				case UOC_NPC_SHOP:
					break;
				}
			}
			if (!pObject1)
			{
				memcpy(&P1, &P2, sizeof(P1));
			}
			if (!pObject2)
			{
				memcpy(&P2, &P1, sizeof(P1));
			}
			if(P1.nPlace == pos_equiproom)
			{
				Player[CLIENT_PLAYER_INDEX].m_sExtAuto.uFtNextTime = timeGetTime() + 5000;
			}
			Player[CLIENT_PLAYER_INDEX].MoveItem(P1, P2);
		}
		break;

	//Íæ¼ÒµãÍê¶Ô»°¿ò
	case GOI_INFORMATION_CONFIRM_NOTIFY:
	{
		PLAYER_SELECTUI_COMMAND command;
		command.nSelectIndex = 0;
		Player[CLIENT_PLAYER_INDEX].OnSelectFromUI(&command, UI_TALKDIALOG);
		break;
	}
	
	//ÎÊÌâÑ¡Ôñ´ð°¸
	//nParma = nAnswerIndex
	case GOI_QUESTION_CHOOSE:
		if (g_bUISelLastSelCount == 0 )
			break;	
		{
			PLAYER_SELECTUI_COMMAND command;
			command.nSelectIndex = nParam;
			Player[CLIENT_PLAYER_INDEX].OnSelectFromUI(&command, UI_SELECTDIALOG);
		}
		break;

	//Ê¹ÓÃÎïÆ·
	//uParam = (KUiObjAtRegion*)pInfo -> ÎïÆ·µÄÊý¾ÝÒÔ¼°ÎïÆ·Ô­À´°Ú·ÅµÄÎ»ÖÃ
	//nParam = ÎïÆ·Ê¹ÓÃÇ°·ÅÖÃµÄÎ»ÖÃ£¬È¡ÖµÎªÃ¶¾ÙÀàÐÍUIOBJECT_CONTAINER¡£
	case GOI_USE_ITEM:
		//to do: waiting for...
		if (uParam)
		{
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*) uParam;
			int nPlace = nParam;
			ItemPos	Pos;
			switch(nPlace)
			{
			case UOC_ITEM_TAKE_WITH:
				Pos.nPlace = pos_equiproom;
				break;
			case UOC_IMMEDIA_ITEM:
				Pos.nPlace = pos_immediacy;
				break;
			case UOC_EQUIPTMENT:
				Pos.nPlace = pos_equip;
				break;
			default:
				Pos.nPlace = -1;
				break;
			}
			Pos.nX = pInfo->Region.h;
			Pos.nY = pInfo->Region.v;
			if (pInfo->Obj.uGenre == CGOG_ITEM && pInfo->Obj.uId > 0 && Pos.nPlace != -1)
				Player[CLIENT_PLAYER_INDEX].ApplyUseItem(pInfo->Obj.uId, Pos, pInfo->Region.Width);
		}
		break;

	//´©ÉÏ×°±¸
	//uParam = (KUiObjAtRegion*)pInfo -> ×°±¸µÄÊý¾ÝºÍ·ÅÖÃÎ»ÖÃÐÅÏ¢
	//			KUiObjAtRegion::Region::h ±íÊ¾ÊôÓÚµÚ¼¸Ì××°±¸
	//			KUiObjAtRegion::Region::v ±íÊ¾ÊôÓÚÄÄ¸öÎ»ÖÃµÄ×°±¸,ÆäÖµÎªÃ·¾ÙÀàÐÍ
	//			UI_EQUIPMENT_POSITIONµÄÈ¡ÖµÖ®Ò»¡£Çë²Î¿´UI_EQUIPMENT_POSITIONµÄ×¢ÊÍ¡£
	case GOI_WEAR_EQUIP:
		//to do: waiting for...
		if (uParam)
		{
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*) uParam;
		}
		break;

	//Ê©Õ¹Îä¹¦/¼¼ÄÜ
	//uParam = (KUiGameObject*)pInfo -> ¼¼ÄÜÊý¾Ý
	case GOI_USE_SKILL:	
		if (uParam)
		{
			//to do:wating for...
			KUiGameObject* pInfo = (KUiGameObject*)uParam;
		}
		break;

	//ÉèÖÃÁ¢¼´¼¼ÄÜ
	//uParam = (KUiGameObject*)pSKill, ¼¼ÄÜÐÅÏ¢
	//nParam = Á¢¼´Î»ÖÃ£¬0±íÊ¾ÎªÓÒ¼ü¼¼ÄÜ£¬1ÖÁ4±íÊ¾ÎªF1ÖÁF4¼¼ÄÜ
	case GOI_SET_IMMDIA_SKILL:
		if (uParam)
		{
			KUiGameObject* pSkill = (KUiGameObject*)uParam;
			if (nParam == 0)
				//to do : modify;
			{
				if ( (int)pSkill->uId > 0 )
					Player[CLIENT_PLAYER_INDEX].SetLeftSkill((int)pSkill->uId);
				else if ((int)(pSkill->uId) == -1) //ÉèÖÃµ±Ç°ÎïÀí¼¼ÄÜÎª×ó¼¼ÄÜ
				{
					int nDetailType = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetWeaponType();
					int nParticularType = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetWeaponParticular();
					
					//½üÉíÎäÆ÷
					if (nDetailType == 0)
					{
						Player[CLIENT_PLAYER_INDEX].SetLeftSkill(g_nMeleeWeaponSkill[nParticularType]);
					}//Ô¶³ÌÎäÆ÷
					else if (nDetailType == 1)
					{
						Player[CLIENT_PLAYER_INDEX].SetLeftSkill(g_nRangeWeaponSkill[nParticularType]);
					}//¿ÕÊÖ
					else if (nDetailType == -1)
					{
						Player[CLIENT_PLAYER_INDEX].SetLeftSkill(g_nHandSkill);
					}
					
				}
			}
			else if (nParam == 1)
			{
				if ((int)pSkill->uId > 0)
					Player[CLIENT_PLAYER_INDEX].SetRightSkill((int)pSkill->uId);
				else if ((int)(pSkill->uId) == -1)
				{
					int nDetailType = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetWeaponType();
					int nParticularType = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetWeaponParticular();
					
					if (nDetailType == 0)
					{
						Player[CLIENT_PLAYER_INDEX].SetRightSkill(g_nMeleeWeaponSkill[nParticularType]);
					}
					else if (nDetailType == 1)
					{
						Player[CLIENT_PLAYER_INDEX].SetRightSkill(g_nRangeWeaponSkill[nParticularType]);
					}
					else if (nDetailType == -1)
					{
						Player[CLIENT_PLAYER_INDEX].SetRightSkill(g_nHandSkill);
					}
				}
			}
		}
		break;

	//ÔöÇ¿Ò»ÖÖ¼¼ÄÜ£¬£¬Ò»´Î¼ÓÒ»µã
	//uParam = ¼¼ÄÜÀàÊô
	//nParam = (uint)¼¼ÄÜid
	case GOI_TONE_UP_SKILL:
		Player[CLIENT_PLAYER_INDEX].ApplyAddSkillLevel((int)nParam, 1);
		break;

	//ÔöÇ¿Ò»Ð©ÊôÐÔµÄÖµ£¬Ò»´Î¼ÓÒ»µã
	//uParam = ±íÊ¾ÒªÔöÇ¿µÄÊÇÄÄ¸öÊôÐÔ£¬È¡ÖµÎªUI_PLAYER_ATTRIBUTEµÄÃ·¾ÙÖµÖ®Ò»
	case GOI_TONE_UP_ATTRIBUTE:
		switch (uParam)
		{
		case UIPA_STRENGTH:		//Á¦Á¿
			Player[CLIENT_PLAYER_INDEX].ApplyAddBaseAttribute(0, 1);
			break;
		case UIPA_DEXTERITY:	//Ãô½Ý
			Player[CLIENT_PLAYER_INDEX].ApplyAddBaseAttribute(1, 1);
			break;		
		case UIPA_VITALITY:		//»îÁ¦
			Player[CLIENT_PLAYER_INDEX].ApplyAddBaseAttribute(2, 1);
			break;
		case UIPA_ENERGY:		//¾«Á¦
			Player[CLIENT_PLAYER_INDEX].ApplyAddBaseAttribute(3, 1);
			break;		
		}
		break;

	//´ðÓ¦/¾Ü¾ø½»Ò×ÇëÇó
	//uParam = (KUiPlayerItem*)pRequestPlayer ·¢³öÇëÇóµÄÍæ¼Ò
	//nParam = (int)(bool)bAccept ÊÇ·ñ½ÓÊÜÇëÇó
	case GOI_TRADE_INVITE_RESPONSE:
		if (uParam)
		{
			KTrade::ReplyInvite(((KUiPlayerItem*)uParam)->nIndex, nParam);
		}
		break;

	//Ôö¼õÒ»¸öÓûÂô³öµÄÎïÆ·
	//uParam = (KUiObjAtRegion*) pObject -> ÎïÆ·ÐÅÏ¢£¬ÆäÖÐ×ø±êÐÅÏ¢ÎªÔÚ½»Ò×½çÃæÖÐµÄ×ø±ê
	//nParam = bAdd -> 0Öµ±íÊ¾¼õÉÙ£¬1Öµ±íÊ¾Ôö¼Ó
	//Remark : Èç¹ûÎïÆ·ÊÇ½ðÇ®µÄ»°£¬ÔòKUiObjAtRegion::Obj::uId±íÊ¾°Ñ½ðÇ®¶îµ÷ÕûÎªÕâ¸öÖµ£¬ÇÒnParamÎÞÒâÒå¡£
	case GOI_TRADE_DESIRE_ITEM:
		if (uParam)
		{
			KUiObjAtRegion* pInfo = (KUiObjAtRegion*) uParam;
			if (pInfo->Obj.uGenre != CGOG_MONEY)
				break;
			Player[CLIENT_PLAYER_INDEX].TradeMoveMoney(pInfo->Obj.uId);
		}
		break;

	//ÓÐÎÞ½»Ò×ÒâÏò
	//nParam = bWilling
	case GOI_TRADE_WILLING:
		if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].GetMenuState() == PLAYER_MENU_STATE_TRADEOPEN)
		{
			Player[CLIENT_PLAYER_INDEX].TradeApplyClose();
		}
		else
		{
			Player[CLIENT_PLAYER_INDEX].TradeApplyOpen((char*)uParam, nParam);
		}
		break;

	//Ëø¶¨½»Ò×
	//nParam = (int)(book)bLock ÊÇ·ñËø¶¨
	case GOI_TRADE_LOCK:
		if ( !Player[CLIENT_PLAYER_INDEX].CheckTrading() )
			break;
		if (Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeLock)
			Player[CLIENT_PLAYER_INDEX].TradeApplyLock(0);
		else
			Player[CLIENT_PLAYER_INDEX].TradeApplyLock(1);
		break;

	//½»Ò×
	case GOI_TRADE:
		if ( !Player[CLIENT_PLAYER_INDEX].CheckTrading() )
			break;
		if (Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeLock != 1 || Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeDestLock != 1)
			break;
		if (Player[CLIENT_PLAYER_INDEX].m_cTrade.m_nTradeState == 0)
		{
			Player[CLIENT_PLAYER_INDEX].TradeDecision(1);		// ½»Ò×È·¶¨
		}
		else
		{
			Player[CLIENT_PLAYER_INDEX].TradeDecision(2);		// ½»Ò×È·¶¨È¡Ïû
		}
		break;

	//½»Ò×È¡Ïû
	case GOI_TRADE_CANCEL:
		if ( !Player[CLIENT_PLAYER_INDEX].CheckTrading() )
			break;
		Player[CLIENT_PLAYER_INDEX].TradeDecision(0);		// ½»Ò×È¡Ïû
		break;

	//²éÑ¯ÊÇ·ñ¿ÉÒÔ¶ªÄ³¸ö¶«Î÷µ½ÓÎÏ·´°¿Ú
	//uParam = (KUiGameObject*)pObject -> ÎïÆ·ÐÅÏ¢
	//nParam = ±»ÍÏ¶¯¶«Î÷µÄµ±Ç°×ø±ê£¨¾ø¶Ô×ø±ê£©£¬ºá×ø±êÔÚµÍ16Î»£¬×Ý×ø±êÔÚ¸ß16Î»¡£(ÏñËØµã×ø±ê)
	//Return = ÊÇ·ñ¿ÉÒÔ·ÅÏÂ
	case GOI_AUTOPLAY_ACTION:
		{
			nRet = 0;
			int nNpcIdx = Player[CLIENT_PLAYER_INDEX].m_nIndex;
			int nPlayerIdx = CLIENT_PLAYER_INDEX;
			UINT uCurTime = timeGetTime();
			switch(uParam)
			{
				case ATYPE_PUMPLIFE:
				{
					int* pValue = (int*)nParam;
					if(Player[nPlayerIdx].m_sExtAuto.uLTime1 < uCurTime)
					{
						if(Npc[nNpcIdx].m_CurrentLife < Npc[nNpcIdx].m_CurrentLifeMax
						 && Npc[nNpcIdx].m_CurrentLife < pValue[0])
						{
							Player[nPlayerIdx].m_sExtAuto.uLTime1 = uCurTime + pValue[2];
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 0 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										ItemPos	Pos;
										Pos.nPlace = pos_equiproom;
										Pos.nX = j;
										Pos.nY = i;
										Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
										return 1;
									}
								}
							}
						}
					}
					if(Player[nPlayerIdx].m_sExtAuto.uLTime2 < uCurTime)
					{
						if(Npc[nNpcIdx].m_CurrentLife < Npc[nNpcIdx].m_CurrentLifeMax
						 && Npc[nNpcIdx].m_CurrentLife < pValue[1])
						{
							Player[nPlayerIdx].m_sExtAuto.uLTime2 = uCurTime + pValue[2];
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 0 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										ItemPos	Pos;
										Pos.nPlace = pos_equiproom;
										Pos.nX = j;
										Pos.nY = i;
										Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
										return 1;
									}
								}
							}
						}
					}
					break;
				}
				case ATYPE_PUMPMANA:
				{
					int* pValue = (int*)nParam;
					if(Player[nPlayerIdx].m_sExtAuto.uMTime1 < uCurTime)
					{
						if(Npc[nNpcIdx].m_CurrentMana < Npc[nNpcIdx].m_CurrentManaMax
						 && Npc[nNpcIdx].m_CurrentMana < pValue[0])
						{
							Player[nPlayerIdx].m_sExtAuto.uMTime1 = uCurTime + pValue[2];
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 1 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										ItemPos	Pos;
										Pos.nPlace = pos_equiproom;
										Pos.nX = j;
										Pos.nY = i;
										Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
										return 1;
									}
								}
							}
						}
					}
					if(Player[nPlayerIdx].m_sExtAuto.uMTime2 < uCurTime)
					{
						if(Npc[nNpcIdx].m_CurrentMana < Npc[nNpcIdx].m_CurrentManaMax
						 && Npc[nNpcIdx].m_CurrentMana < pValue[1])
						{
							Player[nPlayerIdx].m_sExtAuto.uMTime2 = uCurTime + pValue[2];
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 1 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										ItemPos	Pos;
										Pos.nPlace = pos_equiproom;
										Pos.nX = j;
										Pos.nY = i;
										Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
										return 1;
									}
								}
							}
						}
					}
					break;
				}
				case ATYPE_TP_CHECKLIFE:
				case ATYPE_TP_CHECKMANA:
				case ATYPE_TP_LIFEGONE:
				case ATYPE_TP_MANAGONE:
				case ATYPE_TP_FULLITEM:
				case ATYPE_TP_FULLMONEY:
				case ATYPE_TP_DMGITEM:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					bool bUseTP = false;
					if(uParam == ATYPE_TP_CHECKLIFE && Npc[nNpcIdx].m_CurrentLife < nParam)
						bUseTP = true;
					else if(uParam == ATYPE_TP_CHECKMANA && Npc[nNpcIdx].m_CurrentMana < nParam)
						bUseTP = true;
					else if(uParam == ATYPE_TP_LIFEGONE)
					{
						bool bFound = false;
						for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
						{
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 0 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										bFound = true;
										break;
									}
								}
							}
							if(bFound)
								break;
						}
						if(!bFound)
							bUseTP = true;
					}
					else if(uParam == ATYPE_TP_MANAGONE)
					{
						bool bFound = false;
						for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
						{
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									if(nGenre == item_medicine
									&& (nDetail == 1 || nDetail == 2
									|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
									{
										bFound = true;
										break;
									}
								}
							}
							if(bFound)
								break;
						}
						if(!bFound)
							bUseTP = true;
					}
					else if(uParam == ATYPE_TP_FULLITEM)
					{
						int x, y;
						if(nParam == 0)
						{
							if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(1, 1, &x, &y))
								bUseTP = true;
						}
						else if(nParam == 1)
						{
							if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(2, 2, &x, &y))
								bUseTP = true;
						}
						else if(nParam == 2)
						{
							if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(2, 3, &x, &y))
								bUseTP = true;
						}
						else if(nParam == 3)
						{
							if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(2, 4, &x, &y))
								bUseTP = true;
						}
					}
					else if(uParam == ATYPE_TP_FULLMONEY)
					{
						if(Player[nPlayerIdx].m_ItemList.GetMoney(room_equipment) > nParam*10000)
							bUseTP = true;
					}
					else if(uParam == ATYPE_TP_DMGITEM)
					{
						if(nParam > 0)
						{
							for (int k = 0; k < itempart_horse; ++k)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.GetEquipment(k);
								if(nIdx > 0)
								{
									int nDur = Item[nIdx].GetDurability();
									if(nDur > 0 && nDur < nParam)
									{
										bUseTP = true;
										break;
									}
								}
							}
						}
					}
					if(bUseTP)
					{
						for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
						for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
						{
							int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
							if(nIdx > 0)
							{
								int nGenre = Item[nIdx].GetGenre();
								int nDetail = Item[nIdx].GetDetailType();
								int nPart = Item[nIdx].GetParticular();
								if(nGenre == item_townportal
								|| (nGenre == item_mascript && nDetail == 1 && nPart == 438))
								{
									Player[nPlayerIdx].m_sExtAuto.bJustTP = TRUE;
									ItemPos	Pos;
									Pos.nPlace = pos_equiproom;
									Pos.nX = j;
									Pos.nY = i;
									Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
									return 1;
								}
							}
						}
					}
					break;
				}
				case ATYPE_CLEAR:
				{
					g_ScenePlace.RemoveFlag();
					Player[nPlayerIdx].m_mAutoExcludeNpcID.clear();
					Player[nPlayerIdx].m_mAutoIDObj.clear();
					Player[nPlayerIdx].m_mAutoIDTeam.clear();
					Player[nPlayerIdx].m_mAutoTeamRecv.clear();
					Player[nPlayerIdx].m_vAutoTeamKick.clear();
					memset(&Player[nPlayerIdx].m_sExtAuto, 0, sizeof(ExtAuto));
					Player[nPlayerIdx].m_sExtAuto.uChatTime = uCurTime + 5*1000;
					if(!nParam && nNpcIdx > 0)	//t¾t auto
					{
						int nX, nY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						Npc[nNpcIdx].SendCommand(do_run, nX, nY);
						SendClientCmdRun(nX, nY);
					}
					break;
				}
				case ATYPE_CHECKTIME:
				{
					if(!Player[nPlayerIdx].m_sExtAuto.uUnFightTime)
					{
						Player[nPlayerIdx].m_sExtAuto.uUnFightTime = uCurTime;
						if(nNpcIdx > 0)
							Player[nPlayerIdx].m_sExtAuto.bPrevFightState = Npc[nNpcIdx].m_FightMode;
					}
					else
					{
						if(nNpcIdx > 0)
						{
							Player[nPlayerIdx].m_sExtAuto.bJustDis = FALSE;
							if(Player[nPlayerIdx].m_sExtAuto.bPrevFightState != Npc[nNpcIdx].m_FightMode)
							{
								Player[nPlayerIdx].m_sExtAuto.bPrevFightState = Npc[nNpcIdx].m_FightMode;
								Player[nPlayerIdx].m_sExtAuto.uUnFightTime = uCurTime;
								if(Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
								{
									CoreDataChanged(GDCNI_UI_ACT, 1, 0);
									Player[nPlayerIdx].m_sExtAuto.bJustTP = FALSE;
									Player[nPlayerIdx].m_sExtAuto.nTempX = 0;
									Player[nPlayerIdx].m_sExtAuto.nTempY = 0;
									Player[nPlayerIdx].m_sExtAuto.bReachDes = FALSE;
									Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 8000;
								}
								else
								{
									Player[nPlayerIdx].m_sExtAuto.nHomeStep = 0;
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								}
							}
						}
						else if(!Player[nPlayerIdx].m_sExtAuto.bJustDis)
						{
							Player[nPlayerIdx].m_sExtAuto.bJustDis = TRUE;
							Player[nPlayerIdx].m_sExtAuto.uUnFightTime = uCurTime;
						}
					}
					
					for (std::map<UINT,UINT>::iterator it = Player[nPlayerIdx].m_mAutoExcludeNpcID.begin();
						it != Player[nPlayerIdx].m_mAutoExcludeNpcID.end();)
					{
						UINT uTime = it->second;
						if(uTime < uCurTime)
						{
							Player[nPlayerIdx].m_mAutoExcludeNpcID.erase(it++);
						}
						else
						{
							++it;
						}
					}
					break;
				}
				case ATYPE_TP_EXIT:
				{
					if(Player[nPlayerIdx].m_sExtAuto.bJustTP && !Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
						return 1;
					break;
				}
				case ATYPE_DISEXIT:
				{
					if(Player[nPlayerIdx].m_sExtAuto.bJustDis
					|| (nNpcIdx > 0 && !Player[nPlayerIdx].m_sExtAuto.bPrevFightState))
					{
						if(uCurTime - Player[nPlayerIdx].m_sExtAuto.uUnFightTime > 14*60*1000)
							return 1;
					}
					break;
				}
				case ATYPE_CANCHAT:
				{
					if(Player[nPlayerIdx].m_sExtAuto.uChatTime < uCurTime)
					{
						if(nParam == 0)
							Player[nPlayerIdx].m_sExtAuto.uChatTime = uCurTime + 12*1000;
						else
							Player[nPlayerIdx].m_sExtAuto.uChatTime = uCurTime + 62*1000;
						return 1;
					}
					break;
				}
				case ATYPE_EATLIFEFULL:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					int x, y;
					if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(2, 3, &x, &y))
					{
						for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
						for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
						{
							int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
							if(nIdx > 0)
							{
								int nGenre = Item[nIdx].GetGenre();
								int nDetail = Item[nIdx].GetDetailType();
								if(nGenre == item_medicine
								&& (nDetail == 0 || nDetail == 1 || nDetail == 2 || nDetail == 4
								|| (nDetail == 8 && Item[nIdx].GetLevel() == 4)))
								{
									ItemPos	Pos;
									Pos.nPlace = pos_equiproom;
									Pos.nX = j;
									Pos.nY = i;
									Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
									return 1;
								}
							}
						}
					}
					break;
				}
				case ATYPE_EATPOISON:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uPoisonTime < uCurTime)
					{
						Player[nPlayerIdx].m_sExtAuto.uPoisonTime = uCurTime + 500;
						if(Npc[nNpcIdx].m_PoisonState.nTime > 0)
						{
						for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
						for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
						{
							int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
							if(nIdx > 0)
							{
								int nGenre = Item[nIdx].GetGenre();
								int nDetail = Item[nIdx].GetDetailType();
								if(nGenre == item_medicine && nDetail == 4)
								{
									ItemPos	Pos;
									Pos.nPlace = pos_equiproom;
									Pos.nX = j;
									Pos.nY = i;
									Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
									return 1;
								}
							}
						}}
					}
					break;
				}
				case ATYPE_EATEXPX2:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uExp2Time >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uExp2Time = uCurTime + 5000;
					KStateNode* pNode = (KStateNode *)Npc[nNpcIdx].m_StateSkillList.GetHead();
					while(pNode)
					{
						if(pNode->m_SkillID == 440)
							return 0;
						pNode = (KStateNode *)pNode->GetNext();
					}
					for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
					for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
					{
						int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
						if(nIdx > 0)
						{
							int nGenre = Item[nIdx].GetGenre();
							int nDetail = Item[nIdx].GetDetailType();
							int nPart = Item[nIdx].GetParticular();
							if(nGenre == item_mascript && nDetail == 1
							&& (nPart == 71 || nPart == 1181))
							{
								ItemPos	Pos;
								Pos.nPlace = pos_equiproom;
								Pos.nX = j;
								Pos.nY = i;
								Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
								return 1;
							}
						}
					}
					break;
				}
				case ATYPE_EATSKILLX2:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uSkillExp2Time >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uSkillExp2Time = uCurTime + 6000;
					break;
				}
				case ATYPE_BASEBUFF:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uBaseBuffTime >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uBaseBuffTime = uCurTime + 500;
					int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(93);
					if(!nSkillIdx)
						return 0;
					int* pValue = (int*)nParam;
					if((Npc[nNpcIdx].m_CurrentLife < Npc[nNpcIdx].m_CurrentLifeMax)
					&& (Npc[nNpcIdx].m_CurrentLife <= Npc[nNpcIdx].m_CurrentLifeMax - pValue[0]))
					{//Nga my buff minh truoc
						Npc[nNpcIdx].SendCommand(do_skill, 93, -1, nNpcIdx);
						SendClientCmdSkill(93, -1, Npc[nNpcIdx].m_dwID);
						return 1;
					}
					if(pValue[1]) //buff pt sau
					{
						KSkill* pSkill = (KSkill*)g_SkillManager.GetSkill(93,
											Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].SkillLevel);
						if(!pSkill || !Player[nPlayerIdx].m_cTeam.m_nFlag)
							return 0;
						int nX, nY, dX, dY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						int nIndex = NpcSet.SearchID(g_Team[0].m_nCaptain);
						if(nIndex > 0 && Npc[nIndex].m_RegionIndex >= 0) //buff doi truong
						{
							Npc[nIndex].GetMpsPos(&dX, &dY);
							if(g_GetDistance(nX, nY, dX, dY) < pSkill->GetAttackRadius())
							{
								if((Npc[nIndex].m_CurrentLife < Npc[nIndex].m_CurrentLifeMax)
								&& (Npc[nIndex].m_CurrentLife <= Npc[nIndex].m_CurrentLifeMax - pValue[0]))
								{
									Npc[nNpcIdx].SendCommand(do_skill, 93, -1, nIndex);
									SendClientCmdSkill(93, -1, Npc[nIndex].m_dwID);
									return 1;
								}
							}
						}
						for (int i = 0; i < MAX_TEAM_MEMBER; ++i) //buff mem
						{
							nIndex = NpcSet.SearchID(g_Team[0].m_nMember[i]);
							if(nIndex > 0 && Npc[nIndex].m_RegionIndex >= 0)
							{
								Npc[nIndex].GetMpsPos(&dX, &dY);
								if(g_GetDistance(nX, nY, dX, dY) < pSkill->GetAttackRadius())
								{
									if((Npc[nIndex].m_CurrentLife < Npc[nIndex].m_CurrentLifeMax)
									&& (Npc[nIndex].m_CurrentLife <= Npc[nIndex].m_CurrentLifeMax - pValue[0]))
									{
										Npc[nNpcIdx].SendCommand(do_skill, 93, -1, nIndex);
										SendClientCmdSkill(93, -1, Npc[nIndex].m_dwID);
										return 1;
									}
								}
							}
						}
					}
					break;
				}
				case ATYPE_CLBUFF:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uCLBuffTime >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uCLBuffTime = uCurTime + 500;
					int nX, nY;
					Npc[nNpcIdx].GetMpsPos(&nX, &nY);
					static int arID[3] = {171,173,178};
					for(int k=0;k<3;++k)
					{
						int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(arID[k]);
						if(!nSkillIdx)
							continue;
						KSkill* pSkill = (KSkill*)g_SkillManager.GetSkill(arID[k],
											Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].SkillLevel);
						if(!pSkill)
							continue;
						int nStateId = pSkill->GetStateSpecailId();
						if(!nStateId)
							continue;
						int nSkillRad = pSkill->GetAttackRadius();
						if(nParam)	//camp
						{
							int nIdx = 0;
							int x,y;
							while(nIdx = NpcSet.GetNextIdx(nIdx))
							{
								if(nIdx == nNpcIdx || !Npc[nIdx].m_dwID || Npc[nIdx].m_RegionIndex < 0
								|| Npc[nIdx].m_Doing == do_death || Npc[nIdx].m_Doing == do_revive)
									continue;
								if(NpcSet.GetRelation(nNpcIdx, nIdx) == relation_enemy)
									continue;
								if(Npc[nIdx].m_Kind != kind_player)
									continue;
								if(Npc[nIdx].m_CurrentCamp != Npc[nNpcIdx].m_CurrentCamp)
									continue;
								Npc[nIdx].GetMpsPos(&x, &y);
								int nDist = g_GetDistance(nX, nY, x, y);
								if(nDist < nSkillRad)
								{
									bool bCastExist = false;
									KStateNode* pNode = (KStateNode *)Npc[nIdx].m_StateSkillList.GetHead();
									while(pNode)
									{
										if(pNode->m_StateGraphics == nStateId)
										{
											bCastExist = true;
											break;
										}
										pNode = (KStateNode *)pNode->GetNext();
									}
									if(bCastExist)
										continue;
									Npc[nNpcIdx].SendCommand(do_skill, arID[k], -1, nIdx);
									SendClientCmdSkill(arID[k], -1, Npc[nIdx].m_dwID);
									return 1;
								}
							}
						}
						else	//team
						{
							int dX, dY;
							int nIndex = NpcSet.SearchID(g_Team[0].m_nCaptain);
							if(nIndex > 0 && Npc[nIndex].m_RegionIndex >= 0) //buff doi truong
							{
								Npc[nIndex].GetMpsPos(&dX, &dY);
								if(g_GetDistance(nX, nY, dX, dY) < nSkillRad)
								{
									bool bCastExist = false;
									KStateNode* pNode = (KStateNode *)Npc[nIndex].m_StateSkillList.GetHead();
									while(pNode)
									{
										if(pNode->m_StateGraphics == nStateId)
										{
											bCastExist = true;
											break;
										}
										pNode = (KStateNode *)pNode->GetNext();
									}
									if(!bCastExist)
									{
										Npc[nNpcIdx].SendCommand(do_skill, arID[k], -1, nIndex);
										SendClientCmdSkill(arID[k], -1, Npc[nIndex].m_dwID);
										return 1;
									}
								}
							}
							for (int i = 0; i < MAX_TEAM_MEMBER; ++i) //buff mem
							{
								nIndex = NpcSet.SearchID(g_Team[0].m_nMember[i]);
								if(nIndex > 0 && Npc[nIndex].m_RegionIndex >= 0)
								{
									Npc[nIndex].GetMpsPos(&dX, &dY);
									if(g_GetDistance(nX, nY, dX, dY) < nSkillRad)
									{
										bool bCastExist = false;
										KStateNode* pNode = (KStateNode *)Npc[nIndex].m_StateSkillList.GetHead();
										while(pNode)
										{
											if(pNode->m_StateGraphics == nStateId)
											{
												bCastExist = true;
												break;
											}
											pNode = (KStateNode *)pNode->GetNext();
										}
										if(!bCastExist)
										{
											Npc[nNpcIdx].SendCommand(do_skill, arID[k], -1, nIndex);
											SendClientCmdSkill(arID[k], -1, Npc[nIndex].m_dwID);
											return 1;
										}
									}
								}
							}
						}
					}
					break;
				}
				case ATYPE_OPENBAG:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uOpenBagTime >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uOpenBagTime = uCurTime + 3000;
					break;
				}
				case ATYPE_SUPPORTBUFF:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					KStateNode* pNode = (KStateNode *)Npc[nNpcIdx].m_StateSkillList.GetHead();
					while(pNode)
					{
						if(pNode->m_SkillID == nParam)
							return 0;
						pNode = (KStateNode *)pNode->GetNext();
					}
					int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(nParam);
					if(!nSkillIdx)
						return 0;
					if(Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].NextCastTime > SubWorld[0].m_dwCurrentTime)
						return 0;
					Npc[nNpcIdx].SendCommand(do_skill, nParam, -1, nNpcIdx);
					SendClientCmdSkill(nParam, -1, Npc[nNpcIdx].m_dwID);
					return 1;
				}
				case ATYPE_LEFTSKILL:
				{
					if(Player[nPlayerIdx].GetLeftSkill() == nParam)
						return 0;
					KUiGameObject	Skill;
					Skill.uGenre = CGOG_SKILL_FIGHT;
					Skill.uId = nParam;
					OperationRequest(GOI_SET_IMMDIA_SKILL,
						(unsigned int)&Skill, 0);
					return 1;
				}
				case ATYPE_RIGHTSKILL:
				{
					if(Player[nPlayerIdx].GetRightSkill() == nParam)
						return 0;
					KUiGameObject	Skill;
					Skill.uGenre = CGOG_SKILL_FIGHT;
					Skill.uId = nParam;
					OperationRequest(GOI_SET_IMMDIA_SKILL,
						(unsigned int)&Skill, 1);
					return 1;
				}
				case ATYPE_CHANGEAURA:
				{
					int nTime = g_SubWorldSet.GetGameTime();
					if(Player[nPlayerIdx].m_sExtAuto.nAuraTime > nTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.nAuraTime = nTime + 10;
					int* pValue = (int*)nParam;
					Player[nPlayerIdx].m_sExtAuto.bChangeAura = !Player[nPlayerIdx].m_sExtAuto.bChangeAura;
					if(Player[nPlayerIdx].m_sExtAuto.bChangeAura)
					{
						if(pValue[0] <= 0 || Player[nPlayerIdx].GetRightSkill() == pValue[0])
							return 0;
						KUiGameObject	Skill;
						Skill.uGenre = CGOG_SKILL_FIGHT;
						Skill.uId = pValue[0];
						OperationRequest(GOI_SET_IMMDIA_SKILL,
							(unsigned int)&Skill, 1);
					}
					else
					{
						if(pValue[1] <= 0 || Player[nPlayerIdx].GetRightSkill() == pValue[1])
							return 0;
						KUiGameObject	Skill;
						Skill.uGenre = CGOG_SKILL_FIGHT;
						Skill.uId = pValue[1];
						OperationRequest(GOI_SET_IMMDIA_SKILL,
							(unsigned int)&Skill, 1);
					}
					return 1;
				}
				case ATYPE_RESETMOVE:
				{
					Player[nPlayerIdx].m_sExtAuto.uTFollMove2 = 0;
					break;
				}
				case ATYPE_MOVE:
				{
					Player[nPlayerIdx].m_sExtAuto.nCurMoveRet = 0;
					if(!Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
						return 0;
					const autoData* pApData = (autoData*)nParam;
					if(!pApData->bMoveFollow && !pApData->bAroundPoint && !pApData->bMoveCoord)
						return 0;
					if(pApData->bMoveFollow)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uTFollMove2 > uCurTime)
							return 0;
						int nIdx = 0;
						while (nIdx = NpcSet.GetNextIdx(nIdx))
						{
							if (Npc[nIdx].m_Kind != kind_player)
								continue;
							if (nIdx == Player[nPlayerIdx].m_nIndex)
								continue;
							if (Npc[nIdx].m_RegionIndex < 0)
								continue;
							if(!strcmp(pApData->szFollName, Npc[nIdx].Name))
							{
								int nDist = NpcSet.GetDistance(nNpcIdx, nIdx);
								if((!pApData->bFight && nDist > 75) || (nDist > 75
								&& (Player[nPlayerIdx].m_sExtAuto.uTFollMove1 > uCurTime
								|| nDist > pApData->nFollowDist)))
								{
									g_ScenePlace.RemoveFlag();
									if(pApData->bMoveUpHorse && !Npc[nNpcIdx].m_bRideHorse)
									{
										if(!pApData->bFight || Player[nPlayerIdx].FindTargetNpc(
											pApData->nVision, pApData->bFightBack, pApData->nFBVision,
											pApData->nSelBoss) <= 0)
										OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
									}
									int x,y;
									Npc[nIdx].GetMpsPos(&x, &y);
									if (!Player[nPlayerIdx].m_RunStatus)
									{
										Npc[nNpcIdx].SendCommand(do_walk, x, y);
										SendClientCmdWalk(x, y);
									}
									else
									{
										Npc[nNpcIdx].SendCommand(do_run, x, y);
										SendClientCmdRun(x, y);
									}
									if(nDist > pApData->nFollowDist)
										Player[nPlayerIdx].m_sExtAuto.uTFollMove1 = uCurTime + nDist;
									Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
									return 1;
								}
								else
								{
									Player[nPlayerIdx].m_mAutoExcludeNpcID.clear();
									if(pApData->bFight)
									Player[nPlayerIdx].m_sExtAuto.uTFollMove2 = uCurTime + 1000;
									else
									Player[nPlayerIdx].m_sExtAuto.uTFollMove2 = 0;
								}
								return 0;
							}
						}
					}
					if(pApData->bAroundPoint)
					{
						if(pApData->nMoveMapId == SubWorld[0].m_SubWorldID
						&& pApData->nPointX > 0 && pApData->nPointY > 0)
						{
							Player[nPlayerIdx].m_sExtAuto.nCurMoveRet = 1;
							int x,y;
							Npc[nNpcIdx].GetMpsPos(&x, &y);
							int nVision = pApData->nVision;
							if(nVision < 100)
								nVision = 100;
							else if(nVision > 1200)
								nVision = 1200;
							int nDist = g_GetDistance(x, y, pApData->nPointX, pApData->nPointY);
							if(nDist >= nVision || nDist > 75)
							{
								if(nDist >= nVision)
								{
									Player[nPlayerIdx].m_sExtAuto.uTOutMove = uCurTime + 750;
									Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
								}
								else if(Player[nPlayerIdx].m_sExtAuto.uTOutMove < uCurTime)
								{
									g_ScenePlace.RemoveFlag();
									return 0;
								}
								if(!SubWorld[0].HaveTarget(x, y))
								{
									SubWorld[0].FindPath(pApData->nPointX, pApData->nPointY);
								}
								else
								{
									if(pApData->nPointX != x || pApData->nPointY != y)
									{
										g_ScenePlace.RemoveFlag();
										SubWorld[0].FindPath(pApData->nPointX, pApData->nPointY);
									}
								}
								return 1;
							}
							else
								g_ScenePlace.RemoveFlag();
							return 0;
						}
					}
					if(pApData->bMoveCoord)
					{
						if(pApData->nCoordCount > 0 && pApData->nMoveMapId == SubWorld[0].m_SubWorldID)
						{
							Player[nPlayerIdx].m_sExtAuto.nCurMoveRet = 2;
							int nX,nY;
							Npc[nNpcIdx].GetMpsPos(&nX, &nY);
							int nVision = pApData->nVision;
							if(nVision < 100)
								nVision = 100;
							else if(nVision > 1200)
								nVision = 1200;
							if(Player[nPlayerIdx].m_sExtAuto.nCoordStep >= pApData->nCoordCount)
								Player[nPlayerIdx].m_sExtAuto.nCoordStep = 0;
							int nDist = g_GetDistance(nX, nY,
								pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x,
								pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y);
							if(pApData->bEncircle
							&& Player[nPlayerIdx].m_sExtAuto.uTEncircle > uCurTime)
							{
								g_ScenePlace.RemoveFlag();
								int x = Player[nPlayerIdx].m_sExtAuto.sEncircle[8].x;
								int y = Player[nPlayerIdx].m_sExtAuto.sEncircle[8].y;
								int nVS = pApData->nVision;
								if(nVS < 600)
									nVS = 600;
								int nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
									nVS, pApData->bFightBack, pApData->nFBVision,
									pApData->nSelBoss, TRUE, NULL, pApData->bMoveFollow, x, y);
								if(nTGNpcIdx > 0)
								{
									UINT i = Player[nPlayerIdx].m_sExtAuto.nCurEncircle;
									x = Player[nPlayerIdx].m_sExtAuto.sEncircle[i].x;
									y = Player[nPlayerIdx].m_sExtAuto.sEncircle[i].y;
									UINT uRemain = 9000 - (Player[nPlayerIdx].m_sExtAuto.uTEncircle - uCurTime);
									if(g_GetDistance(nX, nY, x, y) < 64
									|| uRemain > (i+1)*1000)
									{
										++i;
										if(i >= 9)
										{
											Player[nPlayerIdx].m_sExtAuto.uTEncircle = 0;
											return 1;
										}
										Player[nPlayerIdx].m_sExtAuto.nCurEncircle = i;
										x = Player[nPlayerIdx].m_sExtAuto.sEncircle[i].x;
										y = Player[nPlayerIdx].m_sExtAuto.sEncircle[i].y;
									}
									if (!Player[nPlayerIdx].m_RunStatus)
									{
										Npc[nNpcIdx].SendCommand(do_walk, x, y);
										SendClientCmdWalk(x, y);
									}
									else
									{
										Npc[nNpcIdx].SendCommand(do_run, x, y);
										SendClientCmdRun(x, y);
									}
									return 1;
								}
								else
								{
									Player[nPlayerIdx].m_sExtAuto.uTEncircle = 0;
									return 1;
								}
							}
							if(nDist >= nVision || nDist > 75)
							{
								if(!(pApData->bMoveKillMons && pApData->bFight)
								|| !Player[nPlayerIdx].m_sExtAuto.nTempX)
								{
									if(nDist >= nVision)
									{
										Player[nPlayerIdx].m_sExtAuto.uTOutMove = uCurTime + 750;
										Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
									}
									else if(Player[nPlayerIdx].m_sExtAuto.uTOutMove < uCurTime)
									{
										g_ScenePlace.RemoveFlag();
										if(!pApData->bFight)
										{
											++Player[nPlayerIdx].m_sExtAuto.nCoordStep;
											if(Player[nPlayerIdx].m_sExtAuto.nCoordStep >= pApData->nCoordCount)
												Player[nPlayerIdx].m_sExtAuto.nCoordStep = 0;
											Player[nPlayerIdx].m_sExtAuto.nTempX = 0;
											Player[nPlayerIdx].m_sExtAuto.nTempY = 0;
											Player[nPlayerIdx].m_sExtAuto.bReachDes = FALSE;
										}
										return 0;
									}
									int x,y;
									if(!SubWorld[0].HaveTarget(x, y))
									{
										SubWorld[0].FindPath(
										pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x,
										pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y);
										if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
											Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 10000;
										else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
											Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 8000;
										else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
											Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 6000;
										else
											Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 4000;
									}
									else
									{
										if(pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x != x
										|| pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y != y)
										{
											g_ScenePlace.RemoveFlag();
											SubWorld[0].FindPath(
											pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x,
											pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y);
											if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
												Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 10000;
											else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
												Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 8000;
											else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
												Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 6000;
											else
												Player[nPlayerIdx].m_sExtAuto.uTJustMove = uCurTime + 4000;
										}
									}
								}
								if(pApData->bMoveKillMons && pApData->bFight && nDist >= nVision)
								{
									if(!Player[nPlayerIdx].m_sExtAuto.nTempX)
									{
										if(Player[nPlayerIdx].m_sExtAuto.uTJustMove < uCurTime)
										{
											int nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
											pApData->nVision, pApData->bFightBack, pApData->nFBVision,
											pApData->nSelBoss);
											if(nTGNpcIdx > 0)
											{
												g_ScenePlace.RemoveFlag();
												Player[nPlayerIdx].m_sExtAuto.nCurMoveRet = 3;
												Player[nPlayerIdx].m_sExtAuto.nTempX = nX;
												Player[nPlayerIdx].m_sExtAuto.nTempY = nY;
												return 0;
											}
										}
									}
									else
									{
										Player[nPlayerIdx].m_sExtAuto.nCurMoveRet = 3;
										return 0;
									}
								}
								else
								{
									Player[nPlayerIdx].m_sExtAuto.nTempX = 0;
									Player[nPlayerIdx].m_sExtAuto.nTempY = 0;
								}
								if(pApData->bMoveUpHorse)
								{
									if(nDist > 1500 && !Npc[nNpcIdx].m_bRideHorse)
										OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
								}
								return 1;
							}
							else
							{
								g_ScenePlace.RemoveFlag();
								if(!pApData->bFight)
								{
									++Player[nPlayerIdx].m_sExtAuto.nCoordStep;
									if(Player[nPlayerIdx].m_sExtAuto.nCoordStep >= pApData->nCoordCount)
										Player[nPlayerIdx].m_sExtAuto.nCoordStep = 0;
									Player[nPlayerIdx].m_sExtAuto.nTempX = 0;
									Player[nPlayerIdx].m_sExtAuto.nTempY = 0;
									Player[nPlayerIdx].m_sExtAuto.bReachDes = FALSE;
								}
								else if(pApData->bEncircle
								&& !Player[nPlayerIdx].m_sExtAuto.bReachDes)
								{
									int x = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x;
									int y = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y;
									int nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
									pApData->nVision, pApData->bFightBack, pApData->nFBVision,
									pApData->nSelBoss, TRUE, NULL, pApData->bMoveFollow, x, y);
									if(nTGNpcIdx > 0)
									{
										Player[nPlayerIdx].m_sExtAuto.bReachDes = TRUE;
										Player[nPlayerIdx].m_sExtAuto.nCurEncircle = 0;
										Player[nPlayerIdx].m_sExtAuto.uTEncircle = uCurTime + 9000;
										int i;
										for(i = 0; i < 4; ++i)
										{
											Player[nPlayerIdx].m_sExtAuto.sEncircle[i].x =
												x + ((500 * g_DirCos(i*15, 64)) >> 10);
											Player[nPlayerIdx].m_sExtAuto.sEncircle[i].y =
												y + ((500 * g_DirSin(i*15, 64)) >> 10);
										}
										for(i = 0; i < 4; ++i)
										{
											Player[nPlayerIdx].m_sExtAuto.sEncircle[i+4].x =
												x + ((250 * g_DirCos(i*15, 64)) >> 10);
											Player[nPlayerIdx].m_sExtAuto.sEncircle[i+4].y =
												y + ((250 * g_DirSin(i*15, 64)) >> 10);
										}
										Player[nPlayerIdx].m_sExtAuto.sEncircle[8].x = x;
										Player[nPlayerIdx].m_sExtAuto.sEncircle[8].y = y;
										return 1;
									}
								}
							}
							return 0;
						}
					}
					break;
				}
				case ATYPE_FIGHT:
				{
					if(!Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
						return 0;
					const autoData* pApData = (autoData*)nParam;
					int nX, nY, x, y;
					BOOL bNewFound = FALSE;
					Npc[nNpcIdx].GetMpsPos(&nX, &nY);
					int nTGNpcIdx = 0;
					if(Player[nPlayerIdx].m_sExtAuto.uNpcID)
					{
						nTGNpcIdx = NpcSet.SearchID(Player[nPlayerIdx].m_sExtAuto.uNpcID);
						if(!nTGNpcIdx || Npc[nTGNpcIdx].m_RegionIndex < 0
						|| Npc[nTGNpcIdx].m_Doing == do_death || Npc[nTGNpcIdx].m_Doing == do_revive
							|| !(NpcSet.GetRelation(nNpcIdx, nTGNpcIdx) == relation_enemy))
							Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
					}
					if(!Player[nPlayerIdx].m_sExtAuto.uNpcID)
					{
						int Ox = 0,Oy = 0;
						if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 1)
						{
							Ox = pApData->nPointX;
							Oy = pApData->nPointY;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 2)
						{
							Ox = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x;
							Oy = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 3)
						{
							Ox = Player[nPlayerIdx].m_sExtAuto.nTempX;
							Oy = Player[nPlayerIdx].m_sExtAuto.nTempY;
						}
						nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
						pApData->nVision, pApData->bFightBack, pApData->nFBVision,
						pApData->nSelBoss, TRUE, NULL, pApData->bMoveFollow, Ox, Oy);
						if(!nTGNpcIdx)
						{
							if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 2)
							{
								++Player[nPlayerIdx].m_sExtAuto.nCoordStep;
								if(Player[nPlayerIdx].m_sExtAuto.nCoordStep >= pApData->nCoordCount)
									Player[nPlayerIdx].m_sExtAuto.nCoordStep = 0;
								Player[nPlayerIdx].m_sExtAuto.bReachDes = FALSE;
							}
							Player[nPlayerIdx].m_sExtAuto.nTempX = 0;
							Player[nPlayerIdx].m_sExtAuto.nTempY = 0;
							return 0;
						}
						Player[nPlayerIdx].m_sExtAuto.uNpcID = Npc[nTGNpcIdx].m_dwID;
						bNewFound = TRUE;
					}
					if(pApData->bSkipGoldboss)
					{
						if(Npc[nTGNpcIdx].m_nBossType == 2)
						{
							Player[nPlayerIdx].
								m_mAutoExcludeNpcID[Player[nPlayerIdx].m_sExtAuto.uNpcID]
								= uCurTime + 30000;
							Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
							return 0;
						}
					}
					if(pApData->nSelFHorse == 1)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uHorseTime < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uHorseTime = uCurTime + 2000;
							if(!Npc[nNpcIdx].m_bRideHorse)
							{
								OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
							}
						}
					}
					else if(pApData->nSelFHorse == 2)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uHorseTime < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uHorseTime = uCurTime + 2000;
							if(Npc[nNpcIdx].m_bRideHorse)
							{
								OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
							}
						}
					}
					Npc[nTGNpcIdx].GetMpsPos(&x, &y);
					int nDist = g_GetDistance(nX, nY, x, y);
					if(Npc[nTGNpcIdx].m_Kind != kind_player)
					{
						if(bNewFound)
						{
							int nSpeed = Npc[nNpcIdx].m_CurrentRunSpeed;
							if(nSpeed <= 0)
								nSpeed = 10;
							Player[nPlayerIdx].m_sExtAuto.uFDelayTime =
								(UINT)(nDist/(float)nSpeed)*56 + 2500;
							Player[nPlayerIdx].m_sExtAuto.uFDelayTime += uCurTime;
							Player[nPlayerIdx].m_sExtAuto.nOldLife = Npc[nTGNpcIdx].m_CurrentLife;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.uFDelayTime < uCurTime)
						{
							if(Player[nPlayerIdx].m_sExtAuto.nOldLife == Npc[nTGNpcIdx].m_CurrentLife)
							{
								Player[nPlayerIdx].
									m_mAutoExcludeNpcID[Player[nPlayerIdx].m_sExtAuto.uNpcID]
									= uCurTime + 30000;
								Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
								return 0;
							}
							else
							{
								Player[nPlayerIdx].m_sExtAuto.uFDelayTime = uCurTime + 2500;
								Player[nPlayerIdx].m_sExtAuto.nOldLife = Npc[nTGNpcIdx].m_CurrentLife;
							}
						}
					}
					int nMainSkill = Player[nPlayerIdx].GetLeftSkill();
					if(pApData->nSkillIdC)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uChSkillTime < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uChSkillTime = uCurTime + 1000*pApData->nSkillCSec;
							Player[nPlayerIdx].m_sExtAuto.bChSkill = !Player[nPlayerIdx].m_sExtAuto.bChSkill;
						}
						if(Player[nPlayerIdx].m_sExtAuto.bChSkill)
						{
							int nIdx = Npc[nNpcIdx].m_SkillList.FindSame(pApData->nSkillIdC);
							if(nIdx && Npc[nNpcIdx].m_SkillList.m_Skills[nIdx].NextCastTime
									<= SubWorld[0].m_dwCurrentTime)
								nMainSkill = pApData->nSkillIdC;
						}
					}
					bool bChecked = false;
					if(pApData->nSkillIdB && Npc[nTGNpcIdx].m_Kind == kind_normal && Npc[nTGNpcIdx].m_nBossType > 0)
						nMainSkill = pApData->nSkillIdB;
					if(pApData->nSkillIdLS)
					{
						int nAPerc = pApData->nSLSPerc;
						if(nAPerc < 0)
							nAPerc = 0;
						else if(nAPerc > 100)
							nAPerc = 100;
						int nPPercent = (int)((double)Npc[nNpcIdx].m_CurrentLife
										*100.0/Npc[nNpcIdx].m_CurrentLifeMax);
						if(nPPercent < nAPerc)
						{
							bChecked = true;
							nMainSkill = pApData->nSkillIdLS;
						}
					}
					if(!bChecked && pApData->nSkillIdMS)
					{
						int nAPerc = pApData->nSMSPerc;
						if(nAPerc < 0)
							nAPerc = 0;
						else if(nAPerc > 100)
							nAPerc = 100;
						int nPPercent = (int)((double)Npc[nNpcIdx].m_CurrentMana
										*100.0/Npc[nNpcIdx].m_CurrentManaMax);
						if(nPPercent < nAPerc)
						{
							bChecked = true;
							nMainSkill = pApData->nSkillIdMS;
						}
					}
					
					if(Npc[nTGNpcIdx].m_Kind == kind_player && pApData->bFightBack)
					{
						if(pApData->nSelFBack == 1)
						{
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									int nGenre = Item[nIdx].GetGenre();
									int nDetail = Item[nIdx].GetDetailType();
									int nPart = Item[nIdx].GetParticular();
									if(nGenre == item_townportal
									|| (nGenre == item_mascript && nDetail == 1 && nPart == 438))
									{
										Player[nPlayerIdx].m_sExtAuto.bJustTP = TRUE;
										ItemPos	Pos;
										Pos.nPlace = pos_equiproom;
										Pos.nX = j;
										Pos.nY = i;
										Player[nPlayerIdx].ApplyUseItem(nIdx, Pos);
										return 1;
									}
								}
							}
						}
						else if(pApData->nSelFBack == 2)
							return 2;
						if(pApData->nSkillIdP)
							nMainSkill = pApData->nSkillIdP;
					}
					int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(nMainSkill);
					if(!nSkillIdx)
						return 0;
					KSkill* pSkill = (KSkill*)g_SkillManager.GetSkill(nMainSkill,
										Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].SkillLevel);
					if(!pSkill)
						return 0;
					if(pApData->nSelFHorse == 0)
					{
						if(pSkill->IsNeedDownHorse())
						{
							if(Npc[nNpcIdx].m_bRideHorse)
								OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
						}
						else
						{
							if(!Npc[nNpcIdx].m_bRideHorse)
								OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
						}
					}
					int nSkillRadius = pSkill->GetAttackRadius();
					if(pApData->bApproach)
					{
						int nNearDist = pApData->nNearDist;
						if(nNearDist < 75)
							nNearDist = 75;
						if(nSkillRadius > nNearDist)
							nSkillRadius = nNearDist;
					}
					g_ScenePlace.RemoveFlag();
					if(nDist < nSkillRadius)
					{
						Npc[nNpcIdx].SendCommand(do_skill, nMainSkill, -1, nTGNpcIdx);
						SendClientCmdSkill(nMainSkill, -1, Npc[nTGNpcIdx].m_dwID);
					}
					else
					{
						if (!Player[nPlayerIdx].m_RunStatus)
						{
							Npc[nNpcIdx].SendCommand(do_walk, x, y);
							SendClientCmdWalk(x, y);
						}
						else
						{
							Npc[nNpcIdx].SendCommand(do_run, x, y);
							SendClientCmdRun(x, y);
						}
					}
					return 1;
				}
				case ATYPE_RESETNPCID:
				{
					Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
					return 0;
				}
				case ATYPE_ISFIGHTMODE:
				{
					return Player[nPlayerIdx].m_sExtAuto.bPrevFightState;
				}
				case ATYPE_DRAWVISION:
				{
					KNpc::g_DrawVision = nParam;
					if(KNpc::g_DrawVision)
					{
						KNpc::g_DrawVisionSkill = KNpc::g_DrawVision;
						int nMainSkill = Player[nPlayerIdx].GetLeftSkill();
						int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(nMainSkill);
						if(!nSkillIdx)
							return 0;
						KSkill* pSkill = (KSkill*)g_SkillManager.GetSkill(nMainSkill,
										Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].SkillLevel);
						if(!pSkill)
							return 0;
						KNpc::g_DrawVisionSkill = pSkill->GetAttackRadius();
					}
					return 0;
				}
				case ATYPE_PKFIGHT:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					const autoData* pApData = (autoData*)nParam;
					int nX, nY, x, y;
					BOOL bNewFound = FALSE;
					Npc[nNpcIdx].GetMpsPos(&nX, &nY);
					int nTGNpcIdx = 0;
					if(Player[nPlayerIdx].m_sExtAuto.uNpcID)
					{
						nTGNpcIdx = NpcSet.SearchID(Player[nPlayerIdx].m_sExtAuto.uNpcID);
						if(!nTGNpcIdx || Npc[nTGNpcIdx].m_RegionIndex < 0
						|| Npc[nTGNpcIdx].m_Doing == do_death || Npc[nTGNpcIdx].m_Doing == do_revive
							|| !(NpcSet.GetRelation(nNpcIdx, nTGNpcIdx) == relation_enemy))
							Player[nPlayerIdx].m_sExtAuto.uNpcID = 0;
					}
					if(!Player[nPlayerIdx].m_sExtAuto.uNpcID)
					{
						if(pApData->nPriority)
						nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
						pApData->nPKVision, pApData->bPKPlayer,
						pApData->nPKVision, 0, pApData->bPKNpc, &pApData->nSerPy[0]);
						else
						nTGNpcIdx = Player[nPlayerIdx].FindTargetNpc(
						pApData->nPKVision, pApData->bPKPlayer,
						pApData->nPKVision, 0, pApData->bPKNpc);
						if(!nTGNpcIdx)
							return 0;
						Player[nPlayerIdx].m_sExtAuto.uNpcID = Npc[nTGNpcIdx].m_dwID;
						bNewFound = TRUE;
					}
					if(pApData->bPKDownHorse)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uHorseTime < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uHorseTime = uCurTime + 2000;
							if(Npc[nNpcIdx].m_bRideHorse)
							{
								OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
							}
						}
					}
					Npc[nTGNpcIdx].GetMpsPos(&x, &y);
					int nDist = g_GetDistance(nX, nY, x, y);
					int nMainSkill = Player[nPlayerIdx].GetLeftSkill();
					if(pApData->nSkillIdC)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uChSkillTime < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uChSkillTime = uCurTime + 1000*pApData->nSkillCSec;
							Player[nPlayerIdx].m_sExtAuto.bChSkill = !Player[nPlayerIdx].m_sExtAuto.bChSkill;
						}
						if(Player[nPlayerIdx].m_sExtAuto.bChSkill)
						{
							int nIdx = Npc[nNpcIdx].m_SkillList.FindSame(pApData->nSkillIdC);
							if(nIdx && Npc[nNpcIdx].m_SkillList.m_Skills[nIdx].NextCastTime
									<= SubWorld[0].m_dwCurrentTime)
								nMainSkill = pApData->nSkillIdC;
						}
					}
					bool bCastState = false;
					if(pApData->nSkillIdCS1)
					{
						KSkill* pStateSkill = (KSkill*)g_SkillManager.GetSkill(pApData->nSkillIdCS1, 1);
						if(pStateSkill)
						{
							int nStateId = pStateSkill->GetStateSpecailId();
							if(nStateId)
							{
								bool bCastExist = false;
								KStateNode* pNode = (KStateNode *)Npc[nTGNpcIdx].m_StateSkillList.GetHead();
								while(pNode)
								{
									if(pNode->m_StateGraphics == nStateId)
									{
										bCastExist = true;
										break;
									}
									pNode = (KStateNode *)pNode->GetNext();
								}
								if(!bCastExist)
								{
									bCastState = true;
									nMainSkill = pApData->nSkillIdCS1;
								}
							}
						}
					}
					if(pApData->nSkillIdCS2 && !bCastState)
					{
						KSkill* pStateSkill = (KSkill*)g_SkillManager.GetSkill(pApData->nSkillIdCS2, 1);
						if(pStateSkill)
						{
							int nStateId = pStateSkill->GetStateSpecailId();
							if(nStateId)
							{
								bool bCastExist = false;
								KStateNode* pNode = (KStateNode *)Npc[nTGNpcIdx].m_StateSkillList.GetHead();
								while(pNode)
								{
									if(pNode->m_StateGraphics == nStateId)
									{
										bCastExist = true;
										break;
									}
									pNode = (KStateNode *)pNode->GetNext();
								}
								if(!bCastExist)
								{
									bCastState = true;
									nMainSkill = pApData->nSkillIdCS2;
								}
							}
						}
					}
					if(pApData->nSkillIdCS3 && !bCastState)
					{
						KSkill* pStateSkill = (KSkill*)g_SkillManager.GetSkill(pApData->nSkillIdCS3, 1);
						if(pStateSkill)
						{
							int nStateId = pStateSkill->GetStateSpecailId();
							if(nStateId)
							{
								bool bCastExist = false;
								KStateNode* pNode = (KStateNode *)Npc[nTGNpcIdx].m_StateSkillList.GetHead();
								while(pNode)
								{
									if(pNode->m_StateGraphics == nStateId)
									{
										bCastExist = true;
										break;
									}
									pNode = (KStateNode *)pNode->GetNext();
								}
								if(!bCastExist)
								{
									bCastState = true;
									nMainSkill = pApData->nSkillIdCS3;
								}
							}
						}
					}
					int nSkillIdx = Npc[nNpcIdx].m_SkillList.FindSame(nMainSkill);
					if(!nSkillIdx)
						return 0;
					KSkill* pSkill = (KSkill*)g_SkillManager.GetSkill(nMainSkill,
										Npc[nNpcIdx].m_SkillList.m_Skills[nSkillIdx].SkillLevel);
					if(!pSkill)
						return 0;
					g_ScenePlace.RemoveFlag();
					int nSkillRadius = pSkill->GetAttackRadius();
					if(pApData->bPKFollowTG)
					{
						if(pApData->bPKAppr && !bCastState)
						{
							int nNearDist = pApData->nPKNearDist;
							if(nNearDist < 75)
								nNearDist = 75;
							if(nSkillRadius > nNearDist)
								nSkillRadius = nNearDist;
						}
						if(nDist < nSkillRadius)
						{
							Npc[nNpcIdx].SendCommand(do_skill, nMainSkill, -1, nTGNpcIdx);
							SendClientCmdSkill(nMainSkill, -1, Npc[nTGNpcIdx].m_dwID);
						}
						else
						{
							if (!Player[nPlayerIdx].m_RunStatus)
							{
								Npc[nNpcIdx].SendCommand(do_walk, x, y);
								SendClientCmdWalk(x, y);
							}
							else
							{
								Npc[nNpcIdx].SendCommand(do_run, x, y);
								SendClientCmdRun(x, y);
							}
						}
					}
					else
					{
						if(nDist <= nSkillRadius)
						{
							Npc[nNpcIdx].SendCommand(do_skill, nMainSkill, -1, nTGNpcIdx);
							SendClientCmdSkill(nMainSkill, -1, Npc[nTGNpcIdx].m_dwID);
						}
						else
						{
							if(bCastState)
								return 0;
							int nOverDist = nDist - nSkillRadius;
							nOverDist += nSkillRadius/2;
							int nDir = g_GetDirIndex(x, y, nX, nY);
							x = x + ((nOverDist * g_DirCos(nDir, 64)) >> 10);
							y = y + ((nOverDist * g_DirSin(nDir, 64)) >> 10);
							Npc[nNpcIdx].SendCommand(do_skill, nMainSkill, x, y);
							SendClientCmdSkill(nMainSkill, x, y);
						}
					}
					return 1;
				}
				case ATYPE_PICKUPSET:
				{
					Player[nPlayerIdx].m_sExtAuto.bLBObjDown = nParam;
					for (std::map<int,ExtAutoObjTime>::iterator itt = Player[nPlayerIdx].m_mAutoIDObj.begin();
						itt != Player[nPlayerIdx].m_mAutoIDObj.end();)
					{
						ExtAutoObjTime s = itt->second;
						if(uCurTime - s.nTotalTime >= 5*60000) //5 minutes
						{
							Player[nPlayerIdx].m_mAutoIDObj.erase(itt++);
						}
						else
						{
							++itt;
						}
					}
					break;
				}
				case ATYPE_GETITEMNAME:
				{
					char* pName = (char*)nParam;
					std::map<std::string, int> mapName;
					for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
					for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
					{
						int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
						if(nIdx > 0)
						{
							std::string str = Item[nIdx].GetName();
							if(mapName.find(str) == mapName.end())
							{
								mapName[str] = 1;
								strcpy(pName, Item[nIdx].GetName());
								pName += 80;
								++nRet;
							}
						}
					}
					break;
				}
				case ATYPE_PICKUP:
				{
					const autoData* pApData = (autoData*)nParam;
					if(!pApData->bPickUp)
						return 0;
					if(!Npc[nNpcIdx].m_FightMode && !pApData->bCityPick)
						return 0;
					int nGameLoop = g_SubWorldSet.GetGameTime();
					if(Player[nPlayerIdx].m_sExtAuto.nCurObjLoop == nGameLoop)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.nCurObjLoop = nGameLoop;
					int i = ObjSet.GetNext(0);
					int nX,nY,dX,dY;
					Npc[nNpcIdx].GetMpsPos(&nX, &nY);
					int nVision = 200;
					while(i)
					{
						if(Object[i].m_nID > 0 &&
						(Object[i].m_nKind == Obj_Kind_Item
						|| Object[i].m_nKind == Obj_Kind_Money))
						{
							int nExist = -1;
							for (std::map<int,ExtAutoObjTime>::iterator it = Player[nPlayerIdx].m_mAutoIDObj.begin();
							it != Player[nPlayerIdx].m_mAutoIDObj.end();++it)
							{
								ExtAutoObjTime& s = it->second;
								if((s.bItem && Object[i].m_nKind == Obj_Kind_Item
								&& Object[i].m_nItemDataID == s.nID)
								||
								(!s.bItem && Object[i].m_nKind == Obj_Kind_Money
								&& Object[i].m_nID == s.nID)
								)
								{
									nExist = it->first;
									break;
								}
							}
							if(nExist >= 0)
							{
								ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
								if(s.nChecked >= 3 || s.nPickTime > uCurTime)
								{
									i = ObjSet.GetNext(i);
									continue;
								}
							}
							if(pApData->bNoPick && pApData->nNOPCount)
							{
								bool bCont = false;
								for(int c=0;c < pApData->nNOPCount;++c)
								{
									if(!strcmp(Object[i].m_szName, pApData->szNOPName[c]))
									{
										bCont = true;
										break;
									}
								}
								if(bCont)
								{
									i = ObjSet.GetNext(i);
									continue;
								}
							}
							if(Object[i].m_nKind == Obj_Kind_Item)
							{
								if(pApData->nPickType == 1) //®Æc phÈm
								{
									if(Object[i].m_nItemGenre == item_equip)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
								}
								else if(pApData->nPickType == 2) //®å mµu
								{
									if(Object[i].m_nItemGenre == item_equip
									&& !Object[i].m_nColorID)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
								}
								else if(pApData->nPickType == 3) //1 « mµu
								{
									if(Object[i].m_nItemWidth != 1 || Object[i].m_nItemHeight != 1)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
									if(Object[i].m_nItemGenre == item_equip
									&& !Object[i].m_nColorID)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
								}
								else if(pApData->nPickType == 4) //1-4 « mµu
								{
									if(Object[i].m_nItemWidth > 2 || Object[i].m_nItemHeight > 2)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
									if(Object[i].m_nItemGenre == item_equip
									&& !Object[i].m_nColorID)
									{
										i = ObjSet.GetNext(i);
										continue;
									}
								}
								else if(pApData->nPickType == 5) //tiÒn
								{
									i = ObjSet.GetNext(i);
									continue;
								}
							}
							Object[i].GetMpsPos(&dX,&dY);
							if(g_GetDistance(nX, nY, dX, dY) < nVision)
							{
								int x, y;
								if(Object[i].m_nKind == Obj_Kind_Money
									|| Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
										Object[i].m_nItemWidth, Object[i].m_nItemHeight, &x, &y))
								{
									if(nExist < 0)
									{
										ExtAutoObjTime s;
										s.nTotalTime = uCurTime;
										s.nChecked = 1;
										s.nPickTime = uCurTime + 120;
										if(Object[i].m_nKind == Obj_Kind_Item)
											s.nID = Object[i].m_nItemDataID;
										else
											s.nID = Object[i].m_nID;
										s.bItem = (Object[i].m_nKind == Obj_Kind_Item);
										Player[nPlayerIdx].m_mAutoIDObj[Player[nPlayerIdx].m_sExtAuto.umObjIncId++] = s;
									}
									else
									{
										ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
										s.nChecked++;
										s.nPickTime = uCurTime + 120;
									}
									Player[nPlayerIdx].CheckObject(i);
									nRet = 1;
									break;
								}
							}
						}
						i = ObjSet.GetNext(i);
					}
					if(pApData->bFollowPick && !pApData->bOnPK && Npc[nNpcIdx].m_FightMode
					&& !Player[nPlayerIdx].m_sExtAuto.bLBObjDown)
					{
						if(pApData->bMoveFollow)
						{
							bool bFoundFol = false;
							int nIdx = 0;
							while (nIdx = NpcSet.GetNextIdx(nIdx))
							{
								if (Npc[nIdx].m_Kind != kind_player)
									continue;
								if (nIdx == Player[nPlayerIdx].m_nIndex)
									continue;
								if (Npc[nIdx].m_RegionIndex < 0)
									continue;
								if(!strcmp(pApData->szFollName, Npc[nIdx].Name))
								{
									bFoundFol = true;
									break;
								}
							}
							if(bFoundFol)
								break;
						}
						int nFollowObj = 0;
						if(Player[nPlayerIdx].m_sExtAuto.nCurObjID)
						{
							nFollowObj = ObjSet.FindID(Player[nPlayerIdx].m_sExtAuto.nCurObjID);
							if(nFollowObj > 0)
							{
								int nExist = -1;
								for (std::map<int,ExtAutoObjTime>::iterator it = Player[nPlayerIdx].m_mAutoIDObj.begin();
								it != Player[nPlayerIdx].m_mAutoIDObj.end();++it)
								{
									ExtAutoObjTime& s = it->second;
									if((s.bItem && Object[nFollowObj].m_nKind == Obj_Kind_Item
									&& Object[nFollowObj].m_nItemDataID == s.nID)
									||
									(!s.bItem && Object[nFollowObj].m_nKind == Obj_Kind_Money
									&& Object[nFollowObj].m_nID == s.nID)
									)
									{
										nExist = it->first;
										break;
									}
								}
								if(nExist >= 0)
								{
									ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
									if(s.nChecked >= 3)
									{
										nFollowObj = 0;
										Player[nPlayerIdx].m_sExtAuto.nCurObjID = 0;
									}
								}
								if(nFollowObj > 0)
								{
									int x, y;
									if(!(Object[nFollowObj].m_nKind == Obj_Kind_Money
											|| Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
											Object[nFollowObj].m_nItemWidth, Object[nFollowObj].m_nItemHeight, &x, &y)))
									{
										nFollowObj = 0;
										Player[nPlayerIdx].m_sExtAuto.nCurObjID = 0;
									}
								}
							}
						}
						nVision = pApData->nPickVision;
						if(nVision < 200)
							nVision = 200;
						else if(nVision > 800)
							nVision = 800;
						if(!nFollowObj)
						{
							i = ObjSet.GetNext(0);
							while(i)
							{
								if(Object[i].m_nID > 0 &&
								(Object[i].m_nKind == Obj_Kind_Item
								|| Object[i].m_nKind == Obj_Kind_Money))
								{
									int x, y;
									if(!(Object[i].m_nKind == Obj_Kind_Money
											|| Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
											Object[i].m_nItemWidth, Object[i].m_nItemHeight, &x, &y)))
									{
										i = ObjSet.GetNext(i);
										continue;
									}
									int nExist = -1;
									for (std::map<int,ExtAutoObjTime>::iterator it = Player[nPlayerIdx].m_mAutoIDObj.begin();
									it != Player[nPlayerIdx].m_mAutoIDObj.end();++it)
									{
										ExtAutoObjTime& s = it->second;
										if((s.bItem && Object[i].m_nKind == Obj_Kind_Item
										&& Object[i].m_nItemDataID == s.nID)
										||
										(!s.bItem && Object[i].m_nKind == Obj_Kind_Money
										&& Object[i].m_nID == s.nID)
										)
										{
											nExist = it->first;
											break;
										}
									}
									if(nExist >= 0)
									{
										ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
										if(s.nChecked >= 3)
										{
											i = ObjSet.GetNext(i);
											continue;
										}
									}
									if(pApData->bNoPick && pApData->nNOPCount)
									{
										bool bCont = false;
										for(int c=0;c < pApData->nNOPCount;++c)
										{
											if(!strcmp(Object[i].m_szName, pApData->szNOPName[c]))
											{
												bCont = true;
												break;
											}
										}
										if(bCont)
										{
											i = ObjSet.GetNext(i);
											continue;
										}
									}
									if(Object[i].m_nKind == Obj_Kind_Item)
									{
										if(pApData->nPickType == 1) //®Æc phÈm
										{
											if(Object[i].m_nItemGenre == item_equip)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
										}
										else if(pApData->nPickType == 2) //®å mµu
										{
											if(Object[i].m_nItemGenre == item_equip
											&& !Object[i].m_nColorID)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
										}
										else if(pApData->nPickType == 3) //1 « mµu
										{
											if(Object[i].m_nItemWidth != 1 || Object[i].m_nItemHeight != 1)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
											if(Object[i].m_nItemGenre == item_equip
											&& !Object[i].m_nColorID)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
										}
										else if(pApData->nPickType == 4) //1-4 « mµu
										{
											if(Object[i].m_nItemWidth > 2 || Object[i].m_nItemHeight > 2)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
											if(Object[i].m_nItemGenre == item_equip
											&& !Object[i].m_nColorID)
											{
												i = ObjSet.GetNext(i);
												continue;
											}
										}
										else if(pApData->nPickType == 5) //tiÒn
										{
											i = ObjSet.GetNext(i);
											continue;
										}
									}
									Object[i].GetMpsPos(&dX,&dY);
									if(g_GetDistance(nX, nY, dX, dY) < nVision)
									{
										if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet > 0)
										{
											int Ox = 0,Oy = 0;
											if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 1)
											{
												Ox = pApData->nPointX;
												Oy = pApData->nPointY;
											}
											else if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 2)
											{
												Ox = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].x;
												Oy = pApData->sMoveCoord[Player[nPlayerIdx].m_sExtAuto.nCoordStep].y;
											}
											else if(Player[nPlayerIdx].m_sExtAuto.nCurMoveRet == 3)
											{
												Ox = Player[nPlayerIdx].m_sExtAuto.nTempX;
												Oy = Player[nPlayerIdx].m_sExtAuto.nTempY;
											}
											int nFVision = pApData->nVision;
											if(nFVision < 100)
												nFVision = 100;
											else if(nFVision > 1200)
												nFVision = 1200;
											int nDist = g_GetDistance(Ox, Oy, dX, dY);
											if(nDist < nFVision)
											{
												nFollowObj = i;
												Player[nPlayerIdx].m_sExtAuto.nCurObjID = Object[i].m_nID;
												break;
											}
										}
										else
										{
											nFollowObj = i;
											Player[nPlayerIdx].m_sExtAuto.nCurObjID = Object[i].m_nID;
											break;
										}
									}
								}
								i = ObjSet.GetNext(i);
							}
						}
						if(nFollowObj)
						{
							g_ScenePlace.RemoveFlag();
							Object[nFollowObj].GetMpsPos(&dX,&dY);
							if (!Player[nPlayerIdx].m_RunStatus)
							{
								Npc[nNpcIdx].SendCommand(do_walk, dX, dY);
								SendClientCmdWalk(dX, dY);
							}
							else
							{
								Npc[nNpcIdx].SendCommand(do_run, dX, dY);
								SendClientCmdRun(dX, dY);
							}
							nRet = 2;
						}
					}
					break;
				}
				case ATYPE_FILTER:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uFtNextTime >= uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uFtNextTime = uCurTime + 300;
					const autoData* pApData = (autoData*)nParam;
					if(!pApData->bFilter && !pApData->bPrize && !pApData->bLevel && !pApData->bSaveRing)
						return 0;
					int nIdx = Player[nPlayerIdx].m_ItemList.Hand();
					if(nIdx)
					{
						bool bThrow = true;
						if(Item[nIdx].GetGenre() != item_equip)
							bThrow = false;
						if(bThrow && Item[nIdx].GetExType() != extype_normal)
							bThrow = false;
						if(bThrow && Item[nIdx].GetDetailType() >= equip_horse)
							bThrow = false;
						if(pApData->bPrize && bThrow)
						{
							int nPrize = Item[nIdx].GetPrice() / BUY_SELL_SCALE;
							if(nPrize > pApData->nPrize)
								bThrow = false;
						}
						if(pApData->bLevel && bThrow)
						{
							if(Item[nIdx].GetLevel() > pApData->nLevel)
								bThrow = false;
						}
						if(pApData->bSaveRing && bThrow)
						{
							if((Item[nIdx].GetDetailType() == equip_ring
							|| Item[nIdx].GetDetailType() == equip_amulet
							|| Item[nIdx].GetDetailType() == equip_pendant)
							&& Item[nIdx].GetLevel() > pApData->nSRLevel)
								bThrow = false;
						}
						if(pApData->bFilter && pApData->nFtMaCount && bThrow)
						{
							for(int i=0;i<pApData->nFtMaCount;++i)
							{
								if(!bThrow)
									break;
								for(int m=0;m<6;++m)
								{
									if(Item[nIdx].m_aryMagicAttrib[m].nAttribType == 139)
									{
										bThrow = false;
										break;
									}
									if(Item[nIdx].m_aryMagicAttrib[m].nAttribType == 0)
										break;
									if(pApData->nFtMagic[i][0] == Item[nIdx].m_aryMagicAttrib[m].nAttribType)
									{
										if(pApData->nFtMagic[i][0] == magic_indestructible_b
										|| Item[nIdx].m_aryMagicAttrib[m].nValue[0] >= pApData->nFtMagic[i][1])
										{
											bThrow = false;
											break;
										}
									}
								}
							}
						}
						if(bThrow)
						{
							int nExist = -1;
							UINT uID = Item[nIdx].GetID();
							for (std::map<int,ExtAutoObjTime>::iterator it = Player[nPlayerIdx].m_mAutoIDObj.begin();
							it != Player[nPlayerIdx].m_mAutoIDObj.end();++it)
							{
								ExtAutoObjTime& s = it->second;
								if(s.bItem && uID == s.nID)
								{
									nExist = it->first;
									break;
								}
							}
							if(nExist < 0)
							{
								ExtAutoObjTime s;
								s.nTotalTime = uCurTime;
								s.nChecked = 3;
								s.nPickTime = uCurTime + 120;
								s.nID = uID;
								s.bItem = 1;
								Player[nPlayerIdx].m_mAutoIDObj[Player[nPlayerIdx].m_sExtAuto.umObjIncId++] = s;
							}
							else
							{
								ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
								s.nTotalTime = uCurTime;
								s.nChecked = 3;
							}
							Player[nPlayerIdx].ThrowAwayItem();
							return 1;
						}
						int x, y;
						if(Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
										Item[nIdx].GetWidth(), Item[nIdx].GetHeight(), &x, &y))
						{
							ItemPos	P1, P2;
							P1.nPlace = P2.nPlace = pos_equiproom;
							P1.nX = P2.nX = x;
							P1.nY = P2.nY = y;
							Player[nPlayerIdx].MoveItem(P1, P2);
							return 1;
						}
					}
					else
					{
						for(int h=0;h<EQUIPMENT_ROOM_HEIGHT;++h)
						for(int w=0;w<EQUIPMENT_ROOM_WIDTH;++w)
						{
							nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(w, h);
							if(nIdx > 0)
							{
								if(Item[nIdx].GetGenre() != item_equip)
									continue;
								if(Item[nIdx].GetExType() != extype_normal)
									continue;
								if(Item[nIdx].GetDetailType() >= equip_horse)
									continue;
								if(pApData->bPrize)
								{
									int nPrize = Item[nIdx].GetPrice() / BUY_SELL_SCALE;
									if(nPrize > pApData->nPrize)
										continue;
								}
								if(pApData->bLevel)
								{
									if(Item[nIdx].GetLevel() > pApData->nLevel)
										continue;
								}
								if(pApData->bSaveRing)
								{
									if((Item[nIdx].GetDetailType() == equip_ring
									|| Item[nIdx].GetDetailType() == equip_amulet
									|| Item[nIdx].GetDetailType() == equip_pendant)
									&& Item[nIdx].GetLevel() > pApData->nSRLevel)
										continue;
								}
								bool bPick = true;
								if(pApData->bFilter && pApData->nFtMaCount)
								{
									for(int i=0;i<pApData->nFtMaCount;++i)
									{
										if(!bPick)
											break;
										for(int m=0;m<6;++m)
										{
											if(Item[nIdx].m_aryMagicAttrib[m].nAttribType == 139)
											{
												bPick = false;
												break;
											}
											if(Item[nIdx].m_aryMagicAttrib[m].nAttribType == 0)
												break;
											if(pApData->nFtMagic[i][0] == Item[nIdx].m_aryMagicAttrib[m].nAttribType)
											{
												if(pApData->nFtMagic[i][0] == magic_indestructible_b
												|| Item[nIdx].m_aryMagicAttrib[m].nValue[0] >= pApData->nFtMagic[i][1])
												{
													bPick = false;
													break;
												}
											}
										}
									}
								}
								if(bPick && pApData->bSaveRing && pApData->bSellItem
								&& !pApData->bFilter && !pApData->bPrize && !pApData->bLevel)
									bPick = false;
								if(bPick)
								{
									ItemPos	P1, P2;
									P1.nPlace = P2.nPlace = pos_equiproom;
									P1.nX = P2.nX = w;
									P1.nY = P2.nY = h;
									Player[nPlayerIdx].MoveItem(P1, P2);
									return 1;
								}
							}
						}
					}
					break;
				}
				case ATYPE_ARRANGEITEM:
				{
					if(!Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uARTimeItem > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uARTimeItem = uCurTime + 20000;
					DYNAMIC_COMMAND sCmd;
					sCmd.ProtocolType = c2s_dynamic_structure;
					sCmd.nBranch = c2sdnmbr_arrangeitem;
					sCmd.m_wLength = sizeof(DYNAMIC_COMMAND) - 1;
					if(g_pClient)
					g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
					return 1;
				}
				case ATYPE_ARRANGEBOX:
				{
					if(Player[nPlayerIdx].m_sExtAuto.bPrevFightState)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uARTimeBox > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uARTimeBox = uCurTime + 60000;
					DYNAMIC_COMMAND sCmd;
					sCmd.ProtocolType = c2s_dynamic_structure;
					sCmd.nBranch = c2sdnmbr_arrangebox;
					sCmd.m_wLength = sizeof(DYNAMIC_COMMAND) - 1;
					if(g_pClient)
					g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
					return 1;
				}
				case ATYPE_GETAROUNDNAME:
				{
					char* pName = (char*)nParam;
					int nIdx = 0;
					while (nIdx = NpcSet.GetNextIdx(nIdx))
					{
						if (Npc[nIdx].m_Kind != kind_player)
							continue;
						if (nIdx == Player[nPlayerIdx].m_nIndex)
							continue;
						if (Npc[nIdx].m_RegionIndex < 0)
							continue;
						strcpy(pName, Npc[nIdx].Name);
						pName += 32;
						nRet++;
						if(nRet >= 100)
							break;
					}
					break;
				}
				case ATYPE_PTPROC:
				{
					if(Player[nPlayerIdx].m_sExtAuto.uTNextProc > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uTNextProc = uCurTime + 400;
					for (std::map<UINT,UINT>::iterator it = Player[nPlayerIdx].m_mAutoIDTeam.begin();
						it != Player[nPlayerIdx].m_mAutoIDTeam.end();)
					{
						UINT uTime = it->second;
						if(uCurTime - uTime >= 15*1000) //15s
						{
							Player[nPlayerIdx].m_mAutoIDTeam.erase(it++);
						}
						else
						{
							++it;
						}
					}
					for (std::map<int, ExtAutoTeamRecv>::iterator itt = Player[nPlayerIdx].m_mAutoTeamRecv.begin();
						itt != Player[nPlayerIdx].m_mAutoTeamRecv.end();)
					{
						ExtAutoTeamRecv& s = itt->second;
						if(uCurTime - s.uTime >= 1500) //1.5s
						{
							Player[nPlayerIdx].m_mAutoTeamRecv.erase(itt++);
						}
						else
						{
							++itt;
						}
					}
					if(!Player[nPlayerIdx].m_vAutoTeamKick.empty())
					{
						UINT uNpcID = Player[nPlayerIdx].m_vAutoTeamKick.back();
						Player[nPlayerIdx].m_vAutoTeamKick.pop_back();
						Player[nPlayerIdx].TeamKickMember(uNpcID);
						return 1;
					}
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					const autoData* pApData = (autoData*)nParam;
					int nLeavePtMem = pApData->nLeavePtMem;
					UINT nLeavePtMin = pApData->nLeavePtMin;
					UINT nRemovePtMin = pApData->nRemovePtMin;
					if(nLeavePtMem < 2)
						nLeavePtMem = 2;
					else if(nLeavePtMem > 8)
						nLeavePtMem = 8;
					if(nLeavePtMin < 1)
						nLeavePtMin = 1;
					if(nRemovePtMin < 1)
						nRemovePtMin = 1;
					if(Player[nPlayerIdx].m_cTeam.m_nFlag)
					{
						if(!Player[nPlayerIdx].m_sExtAuto.uTNextLeave)
							Player[nPlayerIdx].m_sExtAuto.uTNextLeave = uCurTime + nLeavePtMin*60000;
						if(!Player[nPlayerIdx].m_sExtAuto.uTNextRemove)
							Player[nPlayerIdx].m_sExtAuto.uTNextRemove = uCurTime + nRemovePtMin*60000;
					}
					if(pApData->bLeavePt)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uTNextLeave < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uTNextLeave = uCurTime + nLeavePtMin*60000;
							if(!Player[nPlayerIdx].m_cTeam.m_nFlag)
								return 0;
							int nCount = 0;
							int nIdx = NpcSet.SearchID(g_Team[0].m_nCaptain);
							if(nIdx > 0 && Npc[nIdx].m_RegionIndex >= 0)
								++nCount;
							for (int i = 0; i < MAX_TEAM_MEMBER; ++i)
							{
								if(g_Team[0].m_nMember[i] > 0)
								{
									nIdx = NpcSet.SearchID(g_Team[0].m_nMember[i]);
									if(nIdx > 0 && Npc[nIdx].m_RegionIndex >= 0)
									{
										++nCount;
									}
								}
							}
							if(nCount < nLeavePtMem)
							{
								Player[nPlayerIdx].LeaveTeam();
								return 1;
							}
						}
					}
					if(pApData->bRemovePt)
					{
						if(Player[nPlayerIdx].m_sExtAuto.uTNextRemove < uCurTime)
						{
							Player[nPlayerIdx].m_sExtAuto.uTNextRemove = uCurTime + nRemovePtMin*60000;
							if(!Player[nPlayerIdx].m_cTeam.m_nFlag
							|| (int)Npc[nNpcIdx].m_dwID != g_Team[0].m_nCaptain)
								return 0;
							for (int i = 0; i < MAX_TEAM_MEMBER; ++i)
							{
								if(g_Team[0].m_nMember[i] > 0)
								{
									int nIdx = NpcSet.SearchID(g_Team[0].m_nMember[i]);
									if(!nIdx || Npc[nIdx].m_RegionIndex < 0)
									{
										Player[nPlayerIdx].m_vAutoTeamKick.push_back(g_Team[0].m_nMember[i]);
									}
								}
							}
						}
					}
					break;
				}
				case ATYPE_PTINVITE:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uTNextInvite > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uTNextInvite = uCurTime + 600;
					const autoData* pApData = (autoData*)nParam;
					if(Player[nPlayerIdx].m_cTeam.m_nFlag)
					{
						if((int)Npc[nNpcIdx].m_dwID != g_Team[0].m_nCaptain)
							return 0;
						bool bFull = true;
						for (int i = 0; i < MAX_TEAM_MEMBER; ++i)
						{
							if(g_Team[0].m_nMember[i] <= 0)
							{
								bFull = false;
								break;
							}
						}
						if(bFull)
							return 0;
					}
					UINT nLeavePtMin = pApData->nLeavePtMin;
					UINT nRemovePtMin = pApData->nRemovePtMin;
					if(nLeavePtMin < 1)
						nLeavePtMin = 1;
					if(nRemovePtMin < 1)
						nRemovePtMin = 1;
					int nIdx = 0;
					while (nIdx = NpcSet.GetNextIdx(nIdx))
					{
						if (Npc[nIdx].m_Kind != kind_player)
							continue;
						if (nIdx == Player[nPlayerIdx].m_nIndex)
							continue;
						if (Npc[nIdx].m_RegionIndex < 0)
							continue;
						if(Player[nPlayerIdx].m_mAutoIDTeam.find(Npc[nIdx].m_dwID)
							!= Player[nPlayerIdx].m_mAutoIDTeam.end())
							continue;
						if(pApData->bJoinPtByList && pApData->nIJPtCount)
						{	//chØ mêi cã tªn trong list
							bool bNamefound = false;
							for(int i=0;i<pApData->nIJPtCount;++i)
							{
								if(!strcmp(Npc[nIdx].Name, pApData->szIJPtName[i]))
								{
									bNamefound = true;
									break;
								}
							}
							if(!bNamefound)
								continue;
						}
						if(!Player[nPlayerIdx].m_cTeam.m_nFlag)
						{
							Player[nPlayerIdx].m_sExtAuto.uTNextLeave = uCurTime + nLeavePtMin*60000;
							Player[nPlayerIdx].m_sExtAuto.uTNextRemove = uCurTime + nRemovePtMin*60000;
							Player[nPlayerIdx].ApplyCreateTeam();
						}
						else
						{
							bool bFoundInTeam = false;
							for (int i = 0; i < MAX_TEAM_MEMBER; ++i)
							{
								if((int)Npc[nIdx].m_dwID == g_Team[0].m_nMember[i])
								{
									bFoundInTeam = true;
									break;
								}
							}
							if(bFoundInTeam)
								continue;
						}
						Player[nPlayerIdx].TeamInviteAdd(Npc[nIdx].m_dwID);
						Player[nPlayerIdx].m_mAutoIDTeam[Npc[nIdx].m_dwID] = uCurTime;
						nRet = 1;
						break;
					}
					break;
				}
				case ATYPE_PTJOIN:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uTNextJoin > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uTNextJoin = uCurTime + 500;
					const autoData* pApData = (autoData*)nParam;
					if(Player[nPlayerIdx].m_cTeam.m_nFlag)
						return 0;
					UINT nLeavePtMin = pApData->nLeavePtMin;
					if(nLeavePtMin < 1)
						nLeavePtMin = 1;
					for (std::map<int, ExtAutoTeamRecv>::iterator it
							= Player[nPlayerIdx].m_mAutoTeamRecv.begin();
						it != Player[nPlayerIdx].m_mAutoTeamRecv.end();)
					{	//t×m trong list ®· mêi
						ExtAutoTeamRecv& s = it->second;
						int nIdx = 0;
						while (nIdx = NpcSet.GetNextIdx(nIdx))
						{
							if (Npc[nIdx].m_Kind != kind_player)
								continue;
							if (nIdx == Player[nPlayerIdx].m_nIndex)
								continue;
							if (Npc[nIdx].m_RegionIndex < 0)
								continue;
							if(pApData->bJoinPtByList && pApData->nIJPtCount)
							{	//nÕu nhËn theo list th× tra cã tªn
								bool bNamefound = false;
								for(int i=0;i<pApData->nIJPtCount;++i)
								{
									if(!strcmp(Npc[nIdx].Name, pApData->szIJPtName[i]))
									{
										bNamefound = true;
										break;
									}
								}
								if(!bNamefound)
									continue;
							}
							if(!strcmp(Npc[nIdx].Name, s.szName))
								break;
						}
						if(nIdx)
						{
							Player[nPlayerIdx].m_sExtAuto.uTNextLeave = uCurTime + nLeavePtMin*60000;
							Player[nPlayerIdx].m_cTeam.ReplyInvite(it->first, 1);
							Player[nPlayerIdx].m_mAutoTeamRecv.erase(it++);
							return 1;
						}
						else
						{
							Player[nPlayerIdx].m_mAutoTeamRecv.erase(it++);
						}
					}
					break;
				}
				case ATYPE_REPAIRF:
				{
					if(!Npc[nNpcIdx].m_FightMode)
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uTNextRepair > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uTNextRepair = uCurTime + 1800;
					int nRepairIdx = 0;
					for (int k = 0; k < itempart_horse; ++k)
					{
						int nIdx = Player[nPlayerIdx].m_ItemList.GetEquipment(k);
						if(nIdx > 0)
						{
							int nDur = Item[nIdx].GetDurability();
							int nMaxDur = Item[nIdx].GetMaxDurability();
							if(nDur > 0 && nMaxDur > 0 && nDur < nMaxDur)
							{
								nRepairIdx = nIdx;
								break;
							}
						}
					}
					if(nRepairIdx)
					{
						int nMoney = Player[nPlayerIdx].m_ItemList.GetMoney(room_equipment);
						int nRepair = Item[nRepairIdx].GetRepairPrice();
						if(nMoney >= nRepair)
						{
							SendClientCmdRepair(Item[nRepairIdx].GetID());
							return 1;
						}
					}
					break;
				}
				case ATYPE_RETURN:
				{
					if(Player[nPlayerIdx].CheckTrading())
						return 0;
					if(Player[nPlayerIdx].m_sExtAuto.uTNextReturn > uCurTime)
						return 0;
					Player[nPlayerIdx].m_sExtAuto.uTNextReturn = uCurTime + 300;
					if(Player[nPlayerIdx].m_sExtAuto.uHorseTime < uCurTime)
					{
						Player[nPlayerIdx].m_sExtAuto.uHorseTime = uCurTime + 2000;
						if(!Npc[nNpcIdx].m_bRideHorse)
						{
							OperationRequest(GOI_PLAYER_ACTION, PA_RIDE, 0);
						}
					}
					const autoData* pApData = (autoData*)nParam;
					if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 0)
					{	//cÊt hoÆc qu¨ng mãn trªn tay nÕu cã
						g_ScenePlace.RemoveFlag();
						++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						int nIdx = Player[nPlayerIdx].m_ItemList.Hand();
						if(nIdx > 0)
						{
							int x, y;
							if(Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
											Item[nIdx].GetWidth(), Item[nIdx].GetHeight(), &x, &y))
							{
								ItemPos	P1, P2;
								P1.nPlace = P2.nPlace = pos_equiproom;
								P1.nX = P2.nX = x;
								P1.nY = P2.nY = y;
								Player[nPlayerIdx].MoveItem(P1, P2);
							}
							else
								Player[nPlayerIdx].ThrowAwayItem();
							return 1;
						}
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 1)
					{	//b¸n r¸c
						if(pApData->bSellItem)
						{
							int nSelIdx = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							{
								if(nSelIdx)
									break;
								for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
								{
									int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
									if(nIdx > 0)
									{
										if(Item[nIdx].GetGenre() != item_equip)
											continue;
										if(Item[nIdx].GetExType() != extype_normal)
											continue;
										int nDetail = Item[nIdx].GetDetailType();
										if(!pApData->bSellHorse)
										{
											if(nDetail >= equip_horse)
												continue;
											nSelIdx = nIdx;
										}
										else if(nDetail == equip_horse || nDetail == equip_mask)
										{
											nSelIdx = nIdx;
											break;
										}
										else if(nDetail < equip_horse)
										{
											nSelIdx = nIdx;
										}
										if(nSelIdx)
										{
											if(pApData->bSaveRing)
											{
												if((nDetail == equip_ring
												|| nDetail == equip_amulet
												|| nDetail == equip_pendant)
												&& Item[nSelIdx].GetLevel() > pApData->nSRLevel)
												{
													nSelIdx = 0;
													continue;
												}
											}
											if(!pApData->nSelSell && pApData->nFtMaCount)
											{
												bool bSave = false;
												for(int k=0;k<pApData->nFtMaCount;++k)
												{
													if(bSave)
														break;
													for(int m=0;m<6;++m)
													{
														if(Item[nSelIdx].m_aryMagicAttrib[m].nAttribType == 139)
														{
															bSave = true;
															break;
														}
														if(Item[nSelIdx].m_aryMagicAttrib[m].nAttribType == 0)
															break;
														if(pApData->nFtMagic[k][0] == Item[nSelIdx].m_aryMagicAttrib[m].nAttribType)
														{
															if(pApData->nFtMagic[k][0] == magic_indestructible_b
															|| Item[nSelIdx].m_aryMagicAttrib[m].nValue[0] >= pApData->nFtMagic[k][1])
															{
																bSave = true;
																break;
															}
														}
													}
												}
												if(bSave)
												{
													nSelIdx = 0;
													continue;
												}
											}
										}
									}
								}
							}
							if(nSelIdx)
							{
								SendClientCmdSell(Item[nSelIdx].GetID());
								return 1;
							}
							else
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						}
						else
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 2)
					{	//mËt khÈu
						++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 3)
					{	//rót tiÒn
						if(pApData->bWithdraw && pApData->nWDMoney)
						{
							int nWDMoney = pApData->nWDMoney*10000;
							int nMoney = Player[nPlayerIdx].m_ItemList.GetMoney(room_repository);
							if(nMoney < nWDMoney)
								nWDMoney = nMoney;
							OperationRequest(GOI_MONEY_INOUT_STORE_BOX, false, nWDMoney);
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							return 1;
						}
						else
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 4)
					{	//söa ®å trong thµnh
						int nRepairIdx = 0;
						if(pApData->bRepair)
						{
							for (int k = 0; k < itempart_horse; ++k)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.GetEquipment(k);
								if(nIdx > 0)
								{
									int nDur = Item[nIdx].GetDurability();
									int nMaxDur = Item[nIdx].GetMaxDurability();
									if(nDur > 0 && nMaxDur > 0 && nDur < nMaxDur)
									{
										nRepairIdx = nIdx;
										break;
									}
								}
							}
						}
						if(nRepairIdx)
						{
							int nMoney = Player[nPlayerIdx].m_ItemList.GetMoney(room_equipment);
							int nRepair = Item[nRepairIdx].GetRepairPrice();
							if(nMoney < nRepair)
							{
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 0;
							}
							SendClientCmdRepair(Item[nRepairIdx].GetID());
							return 1;
						}
						else
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 5)
					{	//cÊt ®å
						if(pApData->bSaveItem)
						{
							if(pApData->nSelStore == 0) {
							int nSaveIdx = 0, nDstPos = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							{
								if(nSaveIdx)
									break;
								for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
								{
									int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
									if(nIdx > 0)
									{
										if(Item[nIdx].GetGenre() != item_equip)
											continue;
										int x, y;
										if(Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(
											Item[nIdx].GetWidth(), Item[nIdx].GetHeight(), &x, &y, room_repository))
										{
											nDstPos = pos_repositoryroom;
											nSaveIdx = nIdx;
											break;
										}
									}
								}
							}
							if(nSaveIdx)
							{
								unsigned int uSrcPr[2];
								uSrcPr[0] = nSaveIdx;
								uSrcPr[1] = pos_equiproom;
								OperationRequest(GOI_EXCHANGEITEM,
								(unsigned int)&uSrcPr, nDstPos);
								return 1;
							}
							else
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							}
							else
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						}
						else
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 6)
					{	//mua thuèc
						MapStation::iterator it = g_MedicineStation.find(SubWorld[0].m_SubWorldID);
						if(it == g_MedicineStation.end())
						{
							Player[nPlayerIdx].m_sExtAuto.nHomeStep = 100;
							return 0;
						}
						if(!pApData->bBuyLife && !pApData->bBuyMana && !pApData->bBuyPois)
						{
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							return 0;
						}
						int nX,nY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						StationVector& v = ( *it ).second;
						if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 0)
						{	//t×m shop
							int i,j;
							int nLNum = 0, nMNum = 0, nPNum = 0;
							for( i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for( j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_LifeBottle[pApData->nBuyLifeSel].nDetail
									&& Item[nIdx].GetLevel() == g_LifeBottle[pApData->nBuyLifeSel].nLevel)
									{
										++nLNum;
									}
									else if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_ManaBottle[pApData->nBuyManaSel].nDetail
									&& Item[nIdx].GetLevel() == g_ManaBottle[pApData->nBuyManaSel].nLevel)
									{
										++nMNum;
									}
									else if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_PoisonBottle[pApData->nBuyPoisSel].nDetail
									&& Item[nIdx].GetLevel() == g_PoisonBottle[pApData->nBuyPoisSel].nLevel)
									{
										++nPNum;
									}
								}
							}
							if((!pApData->bBuyLife || nLNum >= pApData->nBLNum)
							&& (!pApData->bBuyMana || nMNum >= pApData->nBMNum)
							&& (!pApData->bBuyPois || nPNum >= pApData->nBPNum))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 0;
							}
							int nDist = -1, nPos = 0;
							for( i=0;i < (int)v.size();++i)
							{
								int nCurDist = g_GetDistance(nX, nY, v[i].x, v[i].y);
								if(nDist < 0)
									nDist = nCurDist;
								if(nCurDist < nDist)
								{
									nPos = i;
									nDist = nCurDist;
								}
							}
							if(SubWorld[0].FindPath(v[nPos].x, v[nPos].y) > 0)
							{
								Player[nPlayerIdx].m_sExtAuto.nCurShop = nPos;
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
								Player[nPlayerIdx].m_sExtAuto.nSubStep += 2;
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 1)
						{	//trªn ®­êng quay l¹i trung t©m
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
							}
							else
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								if(c.x != x || c.y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 2)
						{	//trªn ®­êng ®Õn shop
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							else
							{
								if(v[Player[nPlayerIdx].m_sExtAuto.nCurShop].x != x
								|| v[Player[nPlayerIdx].m_sExtAuto.nCurShop].y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							int nPos = Player[nPlayerIdx].m_sExtAuto.nCurShop;
							int nDist = g_GetDistance(nX, nY, v[nPos].x, v[nPos].y);
							if(nDist < 300)
							{
								if(CoreDataChanged(GDCNI_UI_ACT, 0, 0))
								{
									++Player[nPlayerIdx].m_sExtAuto.nSubStep;
									return 0;
								}
								if(!Player[nPlayerIdx].m_sExtAuto.uSyncTime)
								{
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
									return 0;
								}
								else if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								{
									sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
									if(SubWorld[0].FindPath(c.x, c.y) > 0)
									{
										Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
										Player[nPlayerIdx].m_sExtAuto.nSubStep = 1;
									}
									return 0;
								}
								int nIdx = 0;
								int dX,dY;
								char szBuff[32];
								while (nIdx = NpcSet.GetNextIdx(nIdx))
								{
									if (Npc[nIdx].m_Kind != kind_dialoger)
										continue;
									if (Npc[nIdx].m_RegionIndex < 0)
										continue;
									Npc[nIdx].GetMpsPos(&dX, &dY);
									if(g_GetDistance(nX, nY, dX, dY) < 128)
									{
										strcpy(szBuff, Npc[nIdx].Name);
										g_StrLower(szBuff);
										if(strstr(szBuff, "d­îc") || strstr(szBuff, "thuèc")
										|| strstr(szBuff, "thÇn y"))
										{
											Player[nPlayerIdx].DialogNpc(nIdx);
											return 1;
										}
									}
								}
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 3)
						{	//lùa chän khung dialog
							if(!CoreDataChanged(GDCNI_UI_ACT, 0, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							CoreDataChanged(GDCNI_UI_ACT, 1, 0);
							OperationRequest(GOI_QUESTION_CHOOSE, 0, 0);
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 1;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 4)
						{	//mua m¸u
							if(!CoreDataChanged(GDCNI_UI_ACT, 2, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							if(!pApData->bBuyLife || pApData->nBLNum <= 0)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							int nBuyNum = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_LifeBottle[pApData->nBuyLifeSel].nDetail
									&& Item[nIdx].GetLevel() == g_LifeBottle[pApData->nBuyLifeSel].nLevel)
									{
										++nBuyNum;
									}
								}
							}
							if(nBuyNum >= pApData->nBLNum)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							for(int b = 0; b < BuySell.GetWidth(); ++b)
							{
								KItem* pItem = BuySell.GetItem(BuySell.GetItemIndex(Player[nPlayerIdx].m_BuyInfo.m_nBuyIdx, b));
								if(!pItem)
									break;
								if(pItem->GetGenre() == item_medicine
								&& pItem->GetDetailType() == g_LifeBottle[pApData->nBuyLifeSel].nDetail
								&& pItem->GetLevel() == g_LifeBottle[pApData->nBuyLifeSel].nLevel)
								{
									if (Player[nPlayerIdx].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									int x,y;
									if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(1, 1, &x, &y))
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									SendClientCmdBuy(b, pos_equiproom, x, y);
									return 1;
								}
							}
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 5)
						{	//mua mana
							if(!CoreDataChanged(GDCNI_UI_ACT, 2, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							if(!pApData->bBuyMana || pApData->nBMNum <= 0)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							int nBuyNum = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_ManaBottle[pApData->nBuyManaSel].nDetail
									&& Item[nIdx].GetLevel() == g_ManaBottle[pApData->nBuyManaSel].nLevel)
									{
										++nBuyNum;
									}
								}
							}
							if(nBuyNum >= pApData->nBMNum)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							for(int b = 0; b < BuySell.GetWidth(); ++b)
							{
								KItem* pItem = BuySell.GetItem(BuySell.GetItemIndex(Player[nPlayerIdx].m_BuyInfo.m_nBuyIdx, b));
								if(!pItem)
									break;
								if(pItem->GetGenre() == item_medicine
								&& pItem->GetDetailType() == g_ManaBottle[pApData->nBuyManaSel].nDetail
								&& pItem->GetLevel() == g_ManaBottle[pApData->nBuyManaSel].nLevel)
								{
									if (Player[nPlayerIdx].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									int x,y;
									if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(1, 1, &x, &y))
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									SendClientCmdBuy(b, pos_equiproom, x, y);
									return 1;
								}
							}
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 6)
						{	//mua gi¶i ®éc
							if(!CoreDataChanged(GDCNI_UI_ACT, 2, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							if(!pApData->bBuyPois || pApData->nBPNum <= 0)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							int nBuyNum = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_medicine
									&& Item[nIdx].GetDetailType() == g_PoisonBottle[pApData->nBuyPoisSel].nDetail
									&& Item[nIdx].GetLevel() == g_PoisonBottle[pApData->nBuyPoisSel].nLevel)
									{
										++nBuyNum;
									}
								}
							}
							if(nBuyNum >= pApData->nBPNum)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							for(int b = 0; b < BuySell.GetWidth(); ++b)
							{
								KItem* pItem = BuySell.GetItem(BuySell.GetItemIndex(Player[nPlayerIdx].m_BuyInfo.m_nBuyIdx, b));
								if(!pItem)
									break;
								if(pItem->GetGenre() == item_medicine
								&& pItem->GetDetailType() == g_PoisonBottle[pApData->nBuyPoisSel].nDetail
								&& pItem->GetLevel() == g_PoisonBottle[pApData->nBuyPoisSel].nLevel)
								{
									if (Player[nPlayerIdx].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									int x,y;
									if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(1, 1, &x, &y))
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									SendClientCmdBuy(b, pos_equiproom, x, y);
									return 1;
								}
							}
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 7)
						{
							CoreDataChanged(GDCNI_UI_ACT, 3, 0);
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						}
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 7)
					{	//mua phï
						MapStation::iterator it = g_ShopStation.find(SubWorld[0].m_SubWorldID);
						if(it == g_ShopStation.end())
						{
							Player[nPlayerIdx].m_sExtAuto.nHomeStep = 100;
							return 0;
						}
						if(!pApData->bBuyTP)
						{
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							return 0;
						}
						int nX,nY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						StationVector& v = ( *it ).second;
						if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 0)
						{	//t×m shop
							int i,j;
							int nTPNum = 0;
							for( i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for( j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_townportal)
									{
										++nTPNum;
									}
								}
							}
							if(!pApData->bBuyTP || nTPNum >= pApData->nBTPNum)
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 0;
							}
							int nDist = -1, nPos = 0;
							for( i=0;i < (int)v.size();++i)
							{
								int nCurDist = g_GetDistance(nX, nY, v[i].x, v[i].y);
								if(nDist < 0)
									nDist = nCurDist;
								if(nCurDist < nDist)
								{
									nPos = i;
									nDist = nCurDist;
								}
							}
							if(SubWorld[0].FindPath(v[nPos].x, v[nPos].y) > 0)
							{
								Player[nPlayerIdx].m_sExtAuto.nCurShop = nPos;
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
								Player[nPlayerIdx].m_sExtAuto.nSubStep += 2;
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 1)
						{	//trªn ®­êng quay l¹i trung t©m
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
							}
							else
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								if(c.x != x || c.y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 2)
						{	//trªn ®­êng ®Õn shop
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							else
							{
								if(v[Player[nPlayerIdx].m_sExtAuto.nCurShop].x != x
								|| v[Player[nPlayerIdx].m_sExtAuto.nCurShop].y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							int nPos = Player[nPlayerIdx].m_sExtAuto.nCurShop;
							int nDist = g_GetDistance(nX, nY, v[nPos].x, v[nPos].y);
							if(nDist < 300)
							{
								if(CoreDataChanged(GDCNI_UI_ACT, 0, 0))
								{
									++Player[nPlayerIdx].m_sExtAuto.nSubStep;
									return 0;
								}
								if(!Player[nPlayerIdx].m_sExtAuto.uSyncTime)
								{
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
									return 0;
								}
								else if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								{
									sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
									if(SubWorld[0].FindPath(c.x, c.y) > 0)
									{
										Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
										Player[nPlayerIdx].m_sExtAuto.nSubStep = 1;
									}
									return 0;
								}
								int nIdx = 0;
								int dX,dY;
								char szBuff[32];
								while (nIdx = NpcSet.GetNextIdx(nIdx))
								{
									if (Npc[nIdx].m_Kind != kind_dialoger)
										continue;
									if (Npc[nIdx].m_RegionIndex < 0)
										continue;
									Npc[nIdx].GetMpsPos(&dX, &dY);
									if(g_GetDistance(nX, nY, dX, dY) < 128)
									{
										strcpy(szBuff, Npc[nIdx].Name);
										g_StrLower(szBuff);
										if(strstr(szBuff, "t¹p h"))
										{
											Player[nPlayerIdx].DialogNpc(nIdx);
											return 1;
										}
									}
								}
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 3)
						{	//lùa chän khung dialog
							if(!CoreDataChanged(GDCNI_UI_ACT, 0, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							CoreDataChanged(GDCNI_UI_ACT, 1, 0);
							OperationRequest(GOI_QUESTION_CHOOSE, 0, 0);
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 1;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 4)
						{	//mua phï
							if(!CoreDataChanged(GDCNI_UI_ACT, 2, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							if(!pApData->bBuyTP || pApData->nBTPNum <= 0)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							int nBuyNum = 0;
							for(int i=0;i<EQUIPMENT_ROOM_HEIGHT;++i)
							for(int j=0;j<EQUIPMENT_ROOM_WIDTH;++j)
							{
								int nIdx = Player[nPlayerIdx].m_ItemList.m_Room[room_equipment].FindItem(j, i);
								if(nIdx > 0)
								{
									if(Item[nIdx].GetGenre() == item_townportal)
									{
										++nBuyNum;
									}
								}
							}
							if(nBuyNum >= pApData->nBTPNum)
							{
								++Player[nPlayerIdx].m_sExtAuto.nSubStep;
								return 0;
							}
							for(int b = 0; b < BuySell.GetWidth(); ++b)
							{
								KItem* pItem = BuySell.GetItem(BuySell.GetItemIndex(Player[nPlayerIdx].m_BuyInfo.m_nBuyIdx, b));
								if(!pItem)
									break;
								if(pItem->GetGenre() == item_townportal)
								{
									if (Player[nPlayerIdx].m_ItemList.GetEquipmentMoney() < pItem->GetPrice())
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									int x,y;
									if(!Player[nPlayerIdx].m_ItemList.CheckCanPlaceInEquipment(1, 1, &x, &y))
									{
										++Player[nPlayerIdx].m_sExtAuto.nSubStep;
										return 0;
									}
									SendClientCmdBuy(b, pos_equiproom, x, y);
									return 1;
								}
							}
							++Player[nPlayerIdx].m_sExtAuto.nSubStep;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 5)
						{
							CoreDataChanged(GDCNI_UI_ACT, 3, 0);
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						}
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 8)
					{	//gi÷ tiÒn
						int nCurMoney = Player[nPlayerIdx].m_ItemList.GetEquipmentMoney();
						if(!pApData->bHoldMoney || pApData->nHoldMoneyNum <= 0)
						{
							if(nCurMoney > 0)
							{
								OperationRequest(GOI_MONEY_INOUT_STORE_BOX, true, nCurMoney);
								nRet = 1;
							}
						}
						else
						{
							int nNeed = pApData->nHoldMoneyNum*10000;
							nCurMoney -= nNeed;
							if(nCurMoney > 0)
							{
								OperationRequest(GOI_MONEY_INOUT_STORE_BOX, true, nCurMoney);
								nRet = 1;
							}
							else if(nCurMoney < 0)
							{
								OperationRequest(GOI_MONEY_INOUT_STORE_BOX, false, -nCurMoney);
								nRet = 1;
							}
						}
						++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 9)
					{	//®i xa phu
						MapStation::iterator it = g_MoveStation.find(SubWorld[0].m_SubWorldID);
						if(it == g_MoveStation.end())
						{
							Player[nPlayerIdx].m_sExtAuto.nHomeStep = 100;
							return 0;
						}
						if(!pApData->bGoStation)
						{
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
							return 0;
						}
						int nX,nY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						StationVector& v = ( *it ).second;
						if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 0)
						{	//t×m shop
							int nDist = -1, nPos = 0;
							for(int i=0;i < (int)v.size();++i)
							{
								int nCurDist = g_GetDistance(nX, nY, v[i].x, v[i].y);
								if(nDist < 0)
									nDist = nCurDist;
								if(nCurDist < nDist)
								{
									nPos = i;
									nDist = nCurDist;
								}
							}
							if(SubWorld[0].FindPath(v[nPos].x, v[nPos].y) > 0)
							{
								Player[nPlayerIdx].m_sExtAuto.nCurShop = nPos;
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
								Player[nPlayerIdx].m_sExtAuto.nSubStep += 2;
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 1)
						{	//trªn ®­êng quay l¹i trung t©m
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
							}
							else
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								if(c.x != x || c.y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 2)
						{	//trªn ®­êng ®Õn shop
							int x,y;
							if(!SubWorld[0].HaveTarget(x, y))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							else
							{
								if(v[Player[nPlayerIdx].m_sExtAuto.nCurShop].x != x
								|| v[Player[nPlayerIdx].m_sExtAuto.nCurShop].y != y)
								{
									g_ScenePlace.RemoveFlag();
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									return 0;
								}
							}
							int nPos = Player[nPlayerIdx].m_sExtAuto.nCurShop;
							int nDist = g_GetDistance(nX, nY, v[nPos].x, v[nPos].y);
							if(nDist < 300)
							{
								if(CoreDataChanged(GDCNI_UI_ACT, 0, 0))
								{
									++Player[nPlayerIdx].m_sExtAuto.nSubStep;
									return 0;
								}
								if(!Player[nPlayerIdx].m_sExtAuto.uSyncTime)
								{
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
									return 0;
								}
								else if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
								{
									sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
									if(SubWorld[0].FindPath(c.x, c.y) > 0)
									{
										Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000;
										Player[nPlayerIdx].m_sExtAuto.nSubStep = 1;
									}
									return 0;
								}
								int nIdx = 0;
								int dX,dY;
								char szBuff[32];
								while (nIdx = NpcSet.GetNextIdx(nIdx))
								{
									if (Npc[nIdx].m_Kind != kind_dialoger)
										continue;
									if (Npc[nIdx].m_RegionIndex < 0)
										continue;
									Npc[nIdx].GetMpsPos(&dX, &dY);
									if(g_GetDistance(nX, nY, dX, dY) < 128)
									{
										strcpy(szBuff, Npc[nIdx].Name);
										g_StrLower(szBuff);
										if(strstr(szBuff, "xa phu"))
										{
											Player[nPlayerIdx].DialogNpc(nIdx);
											return 1;
										}
									}
								}
							}
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 3)
						{	//lùa chän khung dialog
							if(!CoreDataChanged(GDCNI_UI_ACT, 0, 0))
							{
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								return 0;
							}
							char szBuff[256];
							for(int i=0;i < CoreDataChanged(GDCNI_UI_ACT, 4, 0);++i)
							{
								CoreDataChanged(GDCNI_UI_ACT, 5, i);
								CoreDataChanged(GDCNI_UI_ACT, 6, (int)&szBuff);
								g_StrLower(szBuff);
								if((pApData->nSelStation == 0 && strstr(szBuff, "l¹i"))
								|| (pApData->nSelStation == 4 && strstr(szBuff, "n¬i lµm")))
								{
									CoreDataChanged(GDCNI_UI_ACT, 1, 0);
									OperationRequest(GOI_QUESTION_CHOOSE, 0, i);
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 2000;
									return 1;
								}
								else if(pApData->nSelStation > 0 && pApData->nSelStation < 4
								&& strstr(szBuff, "n¬i ®·"))
								{
									CoreDataChanged(GDCNI_UI_ACT, 1, 0);
									OperationRequest(GOI_QUESTION_CHOOSE, 0, i);
									++Player[nPlayerIdx].m_sExtAuto.nSubStep;
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 2000;
									return 1;
								}
							}
							CoreDataChanged(GDCNI_UI_ACT, 1, 0);
							Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
							return 0;
						}
						else if(Player[nPlayerIdx].m_sExtAuto.nSubStep == 4)
						{	//lùa chän n¬i ®· ®i qua
							if(CoreDataChanged(GDCNI_UI_ACT, 0, 0))
							{
								int nSel = pApData->nSelStation - 1;
								if(nSel >= 0 && nSel < CoreDataChanged(GDCNI_UI_ACT, 4, 0))
								{
									CoreDataChanged(GDCNI_UI_ACT, 1, 0);
									OperationRequest(GOI_QUESTION_CHOOSE, 0, nSel);
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 2000;
									return 1;
								}
								else
								{
									CoreDataChanged(GDCNI_UI_ACT, 1, 0);
									Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
									++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
									Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
									return 0;
								}
							}
							if(Player[nPlayerIdx].m_sExtAuto.uSyncTime < uCurTime)
							{
								CoreDataChanged(GDCNI_UI_ACT, 1, 0);
								Player[nPlayerIdx].m_sExtAuto.nSubStep = 0;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = 0;
							}
						}
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 10)
					{	//®i b¶n ®å b»ng thÇn hµnh phï
						if(g_MoveStation.find(SubWorld[0].m_SubWorldID) == g_MoveStation.end())
						{
							Player[nPlayerIdx].m_sExtAuto.nHomeStep = 100;
							return 0;
						}
						if(!pApData->bGoMap)
						{
							++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
							Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 3000;
							return 0;
						}
						if(Player[nPlayerIdx].m_sExtAuto.uSyncTime > uCurTime)
							return 0;
						Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 1000;
						//di chuyÓn b»ng THP
						char szPack[16];
						DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
						pCmd->ProtocolType = c2s_dynamic_structure;
						pCmd->nBranch = c2sdnmbr_movemapid;
						pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
						int* pMapID = (int*)(pCmd+1);
						*pMapID = g_GoMapID[pApData->nSelMap];
						if (g_pClient)
							g_pClient->SendPackToServer(pCmd, pCmd->m_wLength + 1);
						++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
						Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 3000;
						return 1;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 11)
					{	//check cã ®øng gÇn tr¹m
						if(Player[nPlayerIdx].m_sExtAuto.uSyncTime > uCurTime)
							return 0;
						MapStation::iterator it = g_MedicineStation.find(SubWorld[0].m_SubWorldID);
						if(it == g_MedicineStation.end())
						{
							Player[nPlayerIdx].m_sExtAuto.nHomeStep = 100;
							return 0;
						}
						StationVector v = ( *it ).second;
						int nX,nY;
						Npc[nNpcIdx].GetMpsPos(&nX, &nY);
						int i;
						for( i=0;i < (int)v.size();++i)
						{
							if(g_GetDistance(nX, nY, v[i].x, v[i].y) < 200)
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000 + g_Random(10)*1000;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 1;
							}
						}
						it = g_ShopStation.find(SubWorld[0].m_SubWorldID);
						v = ( *it ).second;
						for( i=0;i < (int)v.size();++i)
						{
							if(g_GetDistance(nX, nY, v[i].x, v[i].y) < 200)
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000 + g_Random(10)*1000;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 1;
							}
						}
						it = g_MoveStation.find(SubWorld[0].m_SubWorldID);
						v = ( *it ).second;
						for( i=0;i < (int)v.size();++i)
						{
							if(g_GetDistance(nX, nY, v[i].x, v[i].y) < 200)
							{
								sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
								SubWorld[0].FindPath(c.x, c.y);
								Player[nPlayerIdx].m_sExtAuto.uSyncTime = uCurTime + 5000 + g_Random(10)*1000;
								++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
								return 1;
							}
						}
						Player[nPlayerIdx].m_sExtAuto.nHomeStep += 2;
					}
					else if(Player[nPlayerIdx].m_sExtAuto.nHomeStep == 12)
					{	//®ang ch¹y vÒ trung t©m
						int x,y;
						if(!SubWorld[0].HaveTarget(x, y))
						{
							sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
							SubWorld[0].FindPath(c.x, c.y);
						}
						else
						{
							sStation& c = g_CenterStation[SubWorld[0].m_SubWorldID];
							if(c.x != x || c.y != y)
							{
								g_ScenePlace.RemoveFlag();
								SubWorld[0].FindPath(c.x, c.y);
							}
						}
						if(Player[nPlayerIdx].m_sExtAuto.uSyncTime > uCurTime)
							return 0;
						g_ScenePlace.RemoveFlag();
						++Player[nPlayerIdx].m_sExtAuto.nHomeStep;
					}
					break;
				}
				case ATYPE_SETSELSV1:
				{
					PlayerSet.m_nSelSvGroup = nParam;
					break;
				}
				case ATYPE_SETSELSV2:
				{
					PlayerSet.m_nSelServer = nParam;
					break;
				}
				case ATYPE_SETACC:
				{
					strcpy(PlayerSet.m_szAccount, (char*)nParam);
					break;
				}
				case ATYPE_SETPASS:
				{
					strcpy(PlayerSet.m_szPassword, (char*)nParam);
					for (int i = 0; i < (int)strlen(PlayerSet.m_szPassword); ++i)
					{
						if(PlayerSet.m_szPassword[i] != -1)
						PlayerSet.m_szPassword[i] = ~PlayerSet.m_szPassword[i];
					}
					break;
				}
			}
		}
		break;

	//ÐÂÌí¼ÓÁÄÌìºÃÓÑ
	//uParam = (KUiPlayerItem*)pFriend
	//			KUiPlayerItem::nData = 0
	case GOI_CHAT_FRIEND_ADD:
		{
			if (g_pClient)
			{
				size_t pckgsize = sizeof(tagExtendProtoHeader) + sizeof(ASK_ADDFRIEND_CMD);

				tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
				pExHeader->ProtocolType = c2s_extendfriend;
				pExHeader->wLength = (WORD)(pckgsize - 1);

				ASK_ADDFRIEND_CMD* pAafCmd = (ASK_ADDFRIEND_CMD*)(pExHeader + 1);
				pAafCmd->ProtocolFamily = pf_friend;
				pAafCmd->ProtocolID = friend_c2c_askaddfriend;
				pAafCmd->pckgid = -1;
				strncpy(pAafCmd->dstrole, ((KUiPlayerItem*)uParam)->Name, _NAME_LEN);
				g_pClient->SendPackToServer(pExHeader, pckgsize);
		
				KSystemMessage	sMsg;
				sprintf(sMsg.szMessage, MSG_CHAT_APPLY_ADD_FRIEND, ((KUiPlayerItem*)uParam)->Name);
				sMsg.eType = SMT_NORMAL;
				sMsg.byConfirmType = SMCT_NONE;
				sMsg.byPriority = 0;
				sMsg.byParamSize = 0;
				CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
			}
		}
		break;
	//¶Ô±ðÈËÒª¼Ó×Ô¼ºÎªºÃÓÑµÄ»Ø¸´
	//uParam = (KUiPlayerItem*)pRequestPlayer ·¢³öÇëÇóµÄÍæ¼Ò
	//nParam = (int)(bool)bAccept ÊÇ·ñ½ÓÊÜÇëÇó
	case GOI_CHAT_FRIEND_INVITE:
		if (uParam)
		{
			if (g_pClient)
			{
				size_t pckgsize = sizeof(tagExtendProtoHeader) + sizeof(REP_ADDFRIEND_CMD);

				tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
				pExHeader->ProtocolType = c2s_extendfriend;
				pExHeader->wLength = (WORD)(pckgsize - 1);
				
				REP_ADDFRIEND_CMD* pRafCmd = (REP_ADDFRIEND_CMD*)(pExHeader + 1);
				pRafCmd->ProtocolFamily = pf_friend;
				pRafCmd->ProtocolID = friend_c2c_repaddfriend;
				pRafCmd->pckgid = -1;
				strncpy(pRafCmd->dstrole, ((KUiPlayerItem*)uParam)->Name, _NAME_LEN);
				pRafCmd->answer = nParam ? answerAgree : answerDisagree;
				g_pClient->SendPackToServer(pExHeader, pckgsize);

			}
		}
		break;

	case GOI_OPTION_SETTING:			//Ñ¡ÏîÉèÖÃ
		if (uParam == OPTION_DYNALIGHT)
		{
			g_ScenePlace.EnableDynamicLights(nParam != 0);
			if (g_pRepresent)
				g_pRepresent->SetOption(DYNAMICLIGHT, nParam != 0);
		}
		else if (uParam == OPTION_PERSPECTIVE)
		{
			if (g_pRepresent)
				g_pRepresent->SetOption(PERSPECTIVE, nParam != 0);
		}
		else if (uParam == OPTION_MUSIC_VALUE)
			Option.SetMusicVolume(nParam);
		else if (uParam == OPTION_SOUND_VALUE)
			Option.SetSndVolume(nParam);
		else if (uParam == OPTION_BRIGHTNESS)
			Option.SetGamma(nParam);
		else if (uParam == OPTION_WEATHER)
			g_ScenePlace.EnableWeather(false/*nParam*/);//fdb
		break;

	case GOI_VIEW_PLAYERITEM:
		{
			g_cViewItem.ApplyViewEquip(uParam);
//			if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_FightMode == 0)
//				g_cViewItem.ApplyViewEquip(uParam);
//			else
//			{
//				KSystemMessage	sMsg;
//				sprintf(sMsg.szMessage, MSG_CAN_NOT_VIEW_ITEM);
//				sMsg.eType = SMT_NORMAL;
//				sMsg.byConfirmType = SMCT_NONE;
//				sMsg.byPriority = 0;
//				sMsg.byParamSize = 0;
//				CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
//			}
		}
		break;
	case GOI_VIEW_PLAYERITEM_END:
		g_cViewItem.DeleteAll();
		break;

	case GOI_PLAYER_ACTION:
		{
			switch(uParam)
			{
			case PA_RUN:
				Player[CLIENT_PLAYER_INDEX].m_RunStatus = !Player[CLIENT_PLAYER_INDEX].m_RunStatus;
				break;
			case PA_SIT:
				if (!Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_bRideHorse)
				{
					if (Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Doing != do_sit)
					{
						Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].SendCommand(do_sit);
						SendClientCmdSit(TRUE);
					}
					else
					{
						Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].SendCommand(do_stand);
						SendClientCmdSit(FALSE);
					}
				}
				else
				{
					KSystemMessage	Msg;
					Msg.byConfirmType = SMCT_CLICK;
					Msg.eType = SMT_PLAYER;
					Msg.byPriority = 1;
					Msg.byParamSize = 0;
					strcpy(Msg.szMessage, "§ang c­ìi ngùa kh«ng thÓ ngåi thiÒn");
					CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&Msg, 0);
				}
				break;
			case PA_RIDE:
				{
					DYNAMIC_COMMAND sCmd;
					sCmd.ProtocolType = c2s_dynamic_structure;
					sCmd.nBranch = c2sdnmbr_updownhorse;
					sCmd.m_wLength = sizeof(DYNAMIC_COMMAND) - 1;
					g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
				}
				break;
			}
		}
		break;
	case GOI_PK_SETTING:		//ÉèÖÃPK
		//nParam = (int)(bool)bEnable	ÊÇ·ñÔÊÐípk
		Player[CLIENT_PLAYER_INDEX].m_cPK.ApplySetNormalPKState(nParam);
		break;
	//³ðÉ±Ä³ÈË
	//uParam = (KUiPlayerItem*) pTarget	³ðÉ±Ä¿±ê
	case GOI_REVENGE_SOMEONE:
		if (uParam)
		{
			KUiPlayerItem	*pTarget = (KUiPlayerItem*)uParam;
			Player[CLIENT_PLAYER_INDEX].m_cPK.ApplyEnmityPK(pTarget->uId);
		}
		break;
	//¸úËæÄ³ÈË
	//uParam = (KUiPlayerItem*) pTarget	¸úËæÄ¿±ê
	case GOI_FOLLOW_SOMEONE:
		if (uParam)
		{
			KUiPlayerItem	*pTarget = (KUiPlayerItem*)uParam;
			if (Npc[pTarget->nIndex].m_Kind == kind_player)
				Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_nPeopleIdx = pTarget->nIndex;
		}
		break;

	//ÏÔÊ¾¸÷Íæ¼ÒÈËÃû
	//nParam = (int)(bool)bShow	ÊÇ·ñÏÔÊ¾
	case GOI_SHOW_PLAYERS_NAME:
		NpcSet.SetShowNameFlag(nParam);
		break;
	//ÏÔÊ¾¸÷Íæ¼ÒÉúÃü
	//nParam = (int)(bool)bShow	ÊÇ·ñÏÔÊ¾
	case GOI_SHOW_PLAYERS_LIFE:
		NpcSet.SetShowLifeFlag(nParam);
		break;
	//ÏÔÊ¾¸÷Íæ¼ÒÄÚÁ¦
	//nParam = (int)(bool)bShow	ÊÇ·ñÏÔÊ¾
	case GOI_SHOW_PLAYERS_MANA:
		NpcSet.SetShowManaFlag(nParam);
		break;
	case GOI_PROCFRAME_BREATHE:
		if(uParam == 0)
			g_ScenePlace.Breathe();
		else if(uParam == 1)
			g_SubWorldSet.MessageLoop();
		else
			g_ScenePlace.EnableForceLookAt();
		break;
	case GOI_PROCFRAME_POSSHIFT:
		{
			int nPerStep = uParam/nParam;
			int	nIdx = 0;
			while(nIdx = NpcSet.GetNextIdx(nIdx))
			{
				if(!Npc[nIdx].m_bProcPosShift || Npc[nIdx].m_RegionIndex < 0)
					continue;
				Npc[nIdx].NewFPSMove(nPerStep);
			}
			nIdx = 0;
			while(nIdx = MissleSet.GetNextIdx(nIdx))
			{
				Missle[nIdx].OnFlyFPS(nPerStep);
			}
		}
		break;
	case GOI_EXEPROGRAM_TASK:
		{
			int nNpcIdx = Player[CLIENT_PLAYER_INDEX].m_nIndex;
			if(nNpcIdx <= 0)
				break;
			if(!uParam) //chuot phai~ len npc
			{
				char szPack[16];
				DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
				pCmd->ProtocolType = c2s_dynamic_structure;
				pCmd->nBranch = c2sdnmbr_gmexenpc;
				pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(UINT);
				UINT* pPos = (UINT*)(pCmd+1);
				*pPos = (UINT)nParam;
				if (g_pClient)
					g_pClient->SendPackToServer(pCmd, pCmd->m_wLength + 1);
			}
			else if(uParam == 1) //chay script GM
			{
				GM_COMMAND sCmd;
				sCmd.ProtocolType = c2s_gmcommand;
				sCmd.m_wLength = sizeof(GM_COMMAND) - 1;
				sCmd.bReload = nParam;
				sCmd.nParam = 789;
				g_pClient->SendPackToServer(&sCmd, sCmd.m_wLength + 1);
			}
		}
		break;
	case GOI_EXCHANGEITEM:
		{
			UINT* pParam = (UINT*)uParam;
			int nItemID = *pParam;
			int nSrcPos = *(pParam+1);
			if(nItemID <= 0 || nItemID >= MAX_ITEM)
				break;
			if(nParam <= 0)
				break;
			char szPack[16];
			DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
			pCmd->ProtocolType = c2s_dynamic_structure;
			pCmd->nBranch = c2sdnmbr_exchangeitem;
			pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + 2*sizeof(BYTE) + sizeof(int);
			BYTE* pPos = (BYTE*)(pCmd+1);
			*pPos = nSrcPos;
			pPos++;
			*pPos = nParam;
			pPos++;
			*(int*)pPos = Item[nItemID].GetID();
			if (g_pClient)
				g_pClient->SendPackToServer(pCmd, pCmd->m_wLength + 1);
		}
		break;
	default:
		nRet = 0;
		break;
	}

	return nRet;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º·¢ËÍÊäÈëÉè±¸µÄÊäÈë²Ù×÷ÏûÏ¢
//--------------------------------------------------------------------------
void KCoreShell::ProcessInput(unsigned int uMsg, unsigned int uParam, int nParam)
{
	Player[CLIENT_PLAYER_INDEX].ProcessInputMsg(uMsg, uParam, nParam);
}

int KCoreShell::FindSelectNPC(int x, int y, int nRelation, bool bSelect, void* pReturn, int& nKind)
{
	Player[CLIENT_PLAYER_INDEX].FindSelectNpc(x, y, nRelation);
	int nT = Player[CLIENT_PLAYER_INDEX].GetTargetNpc();

	if (!bSelect)
		Player[CLIENT_PLAYER_INDEX].SetTargetNpc(0);

	if (nT > 0)
	{
		if (pReturn)
		{
			KUiPlayerItem* p = (KUiPlayerItem*)pReturn;
			strncpy(p->Name, Npc[nT].Name, 32);
			p->nIndex = Npc[nT].m_Index;
			p->uId = Npc[nT].m_dwID;
			p->nData = Npc[nT].GetMenuState();
		}
		nKind = Npc[nT].m_Kind;
		return true;
	}
	return false;
}

int KCoreShell::FindSelectObject(int x, int y, bool bSelect, int& nObjectIdx, int& nKind)
{
	Player[CLIENT_PLAYER_INDEX].FindSelectObject(x, y);
	int nT = Player[CLIENT_PLAYER_INDEX].GetTargetObj();
	
	if (!bSelect)
		Player[CLIENT_PLAYER_INDEX].SetTargetObj(0);

	if (nT > 0)
	{
		nObjectIdx = nT;
		nKind = Object[nT].m_nKind;
		return true;
	}
	return false;
}


int KCoreShell::FindSpecialNPC(char* Name, void* pReturn, int& nKind)
{
	if (Name == NULL || Name[0] == 0)
		return false;
	for (int nT = 0; nT < MAX_NPC; nT++)
	{
		if	(strcmp(Npc[nT].Name, Name) == 0)
		{
			if (pReturn)
			{
				KUiPlayerItem* p = (KUiPlayerItem*)pReturn;
				strncpy(p->Name, Npc[nT].Name, 32);
				p->nIndex = Npc[nT].m_Index;
				p->uId = Npc[nT].m_dwID;
				p->nData = Npc[nT].GetMenuState();
			}
			nKind = Npc[nT].m_Kind;
			return true;
		}
	}
	return false;
}

int KCoreShell::ChatSpecialPlayer(void* pPlayer, const char* pMsgBuff, unsigned short nMsgLength)
{
	KUiPlayerItem* p = (KUiPlayerItem*)pPlayer;
	if (p)
	{
		if (p->nIndex >= 0 && p->nIndex < MAX_NPC)
		{
			int nTalker = p->nIndex;
			if (Npc[nTalker].m_Kind == kind_player &&
				Npc[nTalker].m_dwID == p->uId)
			{
				Npc[nTalker].SetChatInfo(p->Name, pMsgBuff, nMsgLength);
				return true;
			}
		}
	}

	return false;
}

void KCoreShell::ApplyAddTeam(void* pPlayer)
{
	KUiPlayerItem* p = (KUiPlayerItem*)pPlayer;
	if (p)
	{
		if (p->nIndex >= 0 && p->nIndex < MAX_NPC && !Player[CLIENT_PLAYER_INDEX].CheckTrading())
		{
			Player[CLIENT_PLAYER_INDEX].ApplyAddTeam(p->nIndex);
		}
	}
}

void KCoreShell::TradeApplyStart(void* pPlayer)
{
	KUiPlayerItem* p = (KUiPlayerItem*)pPlayer;
	if (p)
	{
		if (p->nIndex >= 0 && p->nIndex < MAX_NPC && !Player[CLIENT_PLAYER_INDEX].CheckTrading())
		{
			Player[CLIENT_PLAYER_INDEX].TradeApplyStart(p->nIndex);
		}
	}
}
//nhan' chuot. phai~, hoac shift+chuot trai, hoac phim' tat'
int KCoreShell::UseSkill(int x, int y, int nSkillID)
{
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
		return 0;
	
	int nX = x;
	int nY = y;
	int nZ = 0;
	g_ScenePlace.ViewPortCoordToSpaceCoord(nX, nY, nZ);
	int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	if (Npc[nIndex].IsCanInput())
	{
		int nIdx = 0;
		
		nIdx = Npc[nIndex].m_SkillList.FindSame(nSkillID);
		g_DebugLog("coreshell[skill]Active");
		Npc[nIndex].SetActiveSkill(nIdx);
	}
	else
	{
		g_DebugLog("coreshell[skill]return");
		return 0;
	}

	if (Npc[nIndex].m_ActiveSkillID > 0)
	{
		int nCurLevel = Npc[nIndex].m_SkillList.GetCurrentLevel(Npc[nIndex].m_ActiveSkillID);
		if(nCurLevel <= 0)
			return 0;
		KSkill * pISkill =  (KSkill*)g_SkillManager.GetSkill(Npc[nIndex].m_ActiveSkillID, nCurLevel);
		if (!pISkill) 
            return 0;

		if (pISkill->IsAura())
			return 0;

		int nTargetIdx = 0;
		
		if (pISkill->IsTargetAlly()) //dong minh cung phe
		{
			Player[CLIENT_PLAYER_INDEX].FindSelectNpc(x, y, relation_ally);
			if (Player[CLIENT_PLAYER_INDEX].GetTargetNpc())
			{
				nTargetIdx = Player[CLIENT_PLAYER_INDEX].GetTargetNpc();
			}
		}
		if (pISkill->IsTargetEnemy()) //danh dich thu~
		{
			Player[CLIENT_PLAYER_INDEX].FindSelectNpc(x, y, relation_enemy);
			if (Player[CLIENT_PLAYER_INDEX].GetTargetNpc())
			{
				nTargetIdx = Player[CLIENT_PLAYER_INDEX].GetTargetNpc();
			}
		}
		//loai buff bua` khi co' muc tieu moi xuat skill
		if (pISkill->IsTargetOnly() && !nTargetIdx)
        {
			Npc[nIndex].m_nPeopleIdx = 0;
			Player[CLIENT_PLAYER_INDEX].SetTargetNpc(0);
			return 0;
		}
		//dich. thu~ trung` chinh' minh
		if (nIndex == nTargetIdx)
		{
			Npc[nIndex].m_nPeopleIdx = 0;
			Player[CLIENT_PLAYER_INDEX].SetTargetNpc(0);
			return 0;
		}

		if ((!Npc[nIndex].m_SkillList.CanCast(Npc[nIndex].m_ActiveSkillID, SubWorld[Npc[nIndex].m_SubWorldIndex].m_dwCurrentTime))
			||
			(!Npc[nIndex].Cost(pISkill->GetSkillCostType(), pISkill->GetSkillCost(&Npc[nIndex]), TRUE))
			)
		{
			Npc[nIndex].m_nPeopleIdx = 0;
			Player[CLIENT_PLAYER_INDEX].SetTargetNpc(0);
			return 0;
		}
		//du~ dieu kien xuat skill
		if (!nTargetIdx) //click toa. do.
		{
			int nRange = pISkill->GetAttackRadius();
			if(nRange > 0)
			{
				int pX, pY;
				Npc[nIndex].GetMpsPos(&pX, &pY);
				int nDistance = g_GetDistance(pX, pY, nX, nY);
				if(nDistance > nRange)
				{
					int nDir = g_GetDirIndex(nX, nY, pX, pY);
					nDistance -= nRange;
					nX = nX + ((nDistance * g_DirCos(nDir, 64)) >> 10);
					nY = nY + ((nDistance * g_DirSin(nDir, 64)) >> 10);
				}
			}
			Npc[nIndex].SendCommand(do_skill, Npc[nIndex].m_ActiveSkillID, nX, nY);
			SendClientCmdSkill(Npc[nIndex].m_ActiveSkillID, nX, nY);
		}
		else //click trung' muc tieu
		{
			int nRange = pISkill->GetAttackRadius();
			if (pISkill->IsTargetOnly())
			{
				if (NpcSet.GetDistance(nIndex , nTargetIdx) > nRange)
				{//khong danh', muc tieu nay` khong di'
					Npc[nIndex].m_nPeopleIdx = 0;
					Player[CLIENT_PLAYER_INDEX].SetTargetNpc(0);
					return 0;
				}
				Npc[nIndex].SendCommand(do_skill, Npc[nIndex].m_ActiveSkillID, -1, nTargetIdx);
				SendClientCmdSkill(Npc[nIndex].m_ActiveSkillID, -1, Npc[nTargetIdx].m_dwID);
			}
			else
			{
				int pX, pY;
				Npc[nIndex].GetMpsPos(&pX, &pY);
				Npc[nTargetIdx].GetMpsPos(&nX, &nY);
				int nDistance = g_GetDistance(pX, pY, nX, nY);
				if(nDistance <= nRange)
				{
					Npc[nIndex].SendCommand(do_skill, Npc[nIndex].m_ActiveSkillID, -1, nTargetIdx);
					SendClientCmdSkill(Npc[nIndex].m_ActiveSkillID, -1, Npc[nTargetIdx].m_dwID);
				}
				else
				{
					int nDir = g_GetDirIndex(nX, nY, pX, pY);
					nDistance -= nRange;
					nX = nX + ((nDistance * g_DirCos(nDir, 64)) >> 10);
					nY = nY + ((nDistance * g_DirSin(nDir, 64)) >> 10);
					Npc[nIndex].SendCommand(do_skill, Npc[nIndex].m_ActiveSkillID, nX, nY);
					SendClientCmdSkill(Npc[nIndex].m_ActiveSkillID, nX, nY);
				}
			}
		}
	}
	Npc[nIndex].m_nPeopleIdx = 0;
	return 1;
}

int KCoreShell::LockSomeoneUseSkill(int nTargetIndex, int nSkillID)
{
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
		return 0;
	
	int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	if (nTargetIndex == nIndex)
		return 0;

	if (Npc[nIndex].IsCanInput())
	{
		int nIdx = 0;
		
		nIdx = Npc[nIndex].m_SkillList.FindSame(nSkillID);
		g_DebugLog("coreshell[skill]Active left");

		Npc[nIndex].SetActiveSkill(nIdx);
	}
	else
	{
		g_DebugLog("[skill]return");
		return 0;
	}

	int nRelation = NpcSet.GetRelation(nIndex, nTargetIndex);
	if (nRelation == relation_enemy)
	{
		Npc[nIndex].m_nPeopleIdx = nTargetIndex;
		return 1;
	}

	return 0;
}

int KCoreShell::LockSomeoneAction(int nTargetIndex)
{
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
		return 0;
	
	int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	if (nTargetIndex == nIndex)
		return 0;
	if (nTargetIndex <= 0 || nTargetIndex >= MAX_NPC)	//È¡ÏûLock
	{
		Npc[nIndex].m_nPeopleIdx = 0;
		return 1;
	}

	int nRelation = NpcSet.GetRelation(nIndex, nTargetIndex);
	if (nRelation != relation_enemy)
	{
		Npc[nIndex].m_nPeopleIdx = nTargetIndex;
		return 1;
	}

	return 0;
}

int KCoreShell::LockObjectAction(int nTargetIndex)
{
	if (Player[CLIENT_PLAYER_INDEX].CheckTrading())
		return 0;
	
	int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	if (nTargetIndex <= 0)	//È¡ÏûLock
		Npc[nIndex].m_nObjectIdx = 0;
	else
		Npc[nIndex].m_nObjectIdx = nTargetIndex;

	return 1;
}

void KCoreShell::GotoWhere(int x, int y, int mode)
{
	if (mode < 0 || mode > 2)
		return;

	if (Player[CLIENT_PLAYER_INDEX].m_nSendMoveFrames >= defMAX_PLAYER_SEND_MOVE_FRAME)
	{
		int bRun = false;

		if ((mode == 0 && Player[CLIENT_PLAYER_INDEX].m_RunStatus) ||
			mode == 2)
			bRun = true;

		int nX = x;
		int nY = y;
		int nZ = 0;
		g_ScenePlace.ViewPortCoordToSpaceCoord(nX, nY, nZ);
		int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

		if (!bRun)
		{
			Npc[nIndex].SendCommand(do_walk, nX, nY);
			// Send to Server
			SendClientCmdWalk(nX, nY);
		}
		else
		{
			Npc[nIndex].SendCommand(do_run, nX, nY);
			// Send to Server
			SendClientCmdRun(nX, nY);
		}
		Player[CLIENT_PLAYER_INDEX].m_nSendMoveFrames = 0;
	}
}

void KCoreShell::Goto(int nDir, int mode)
{
	if (nDir < 0 || nDir > 63)
		return;

	if (mode < 0 || mode > 2)
		return;

	int bRun = false;

	if ((mode == 0 && Player[CLIENT_PLAYER_INDEX].m_RunStatus) ||
		mode == 2)
		bRun = true;

	int nIndex = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	int nSpeed;
	if (bRun)
		nSpeed = Npc[nIndex].m_CurrentRunSpeed;
	else
		nSpeed = Npc[nIndex].m_CurrentWalkSpeed;

	Player[CLIENT_PLAYER_INDEX].Walk(nDir, nSpeed);

	Player[CLIENT_PLAYER_INDEX].m_nSendMoveFrames = 0;
}

void KCoreShell::Turn(int nDir)
{
	if (nDir < 0 || nDir > 3)
		return;

	if (nDir == 0)
		Player[CLIENT_PLAYER_INDEX].TurnLeft();
	else if (nDir == 1)
		Player[CLIENT_PLAYER_INDEX].TurnRight();
	else
		Player[CLIENT_PLAYER_INDEX].TurnBack();
}

int KCoreShell::ThrowAwayItem()
{
	if(Player[CLIENT_PLAYER_INDEX].m_sExtAuto.uUnFightTime)
	{
		int nPlayerIdx = CLIENT_PLAYER_INDEX;
		int nIdx = Player[nPlayerIdx].m_ItemList.Hand();
		if(nIdx > 0)
		{
			int nExist = -1;
			UINT uID = Item[nIdx].GetID();
			for (std::map<int,ExtAutoObjTime>::iterator it = Player[nPlayerIdx].m_mAutoIDObj.begin();
			it != Player[nPlayerIdx].m_mAutoIDObj.end();++it)
			{
				ExtAutoObjTime& s = it->second;
				if(s.bItem && uID == s.nID)
				{
					nExist = it->first;
					break;
				}
			}
			UINT uCurTime = timeGetTime();
			if(nExist < 0)
			{
				ExtAutoObjTime s;
				s.nTotalTime = uCurTime;
				s.nChecked = 3;
				s.nPickTime = uCurTime + 120;
				s.nID = uID;
				s.bItem = 1;
				Player[nPlayerIdx].m_mAutoIDObj[Player[nPlayerIdx].m_sExtAuto.umObjIncId++] = s;
			}
			else
			{
				ExtAutoObjTime& s = Player[nPlayerIdx].m_mAutoIDObj[nExist];
				s.nTotalTime = uCurTime;
				s.nChecked = 3;
			}
		}
	}
	return Player[CLIENT_PLAYER_INDEX].ThrowAwayItem();
}

int KCoreShell::GetNPCRelation(int nIndex)
{
	return NpcSet.GetRelation(Player[CLIENT_PLAYER_INDEX].m_nIndex, nIndex);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º»æÖÆÓÎÏ·¶ÔÏó
//--------------------------------------------------------------------------
void KCoreShell::DrawGameObj(unsigned int uObjGenre, unsigned int uId, int x, int y, int Width, int Height, int nParam)
{
	if (g_pRepresent)
		CoreDrawGameObj(uObjGenre, uId, x, y, Width, Height, nParam);
}

#include "../../Represent/iRepresent/iRepresentshell.h"

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º»æÖÆÓÎÏ·ÊÀ½ç
//--------------------------------------------------------------------------
void KCoreShell::DrawGameSpace()
{
	if (g_pRepresent)
	{
		g_ScenePlace.Paint();
		//SubWorld[0].Paint();
		//Player[CLIENT_PLAYER_INDEX].DrawSelectInfo();
	}
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÉèÖÃ»æÍ¼½Ó¿ÚÊµÀýµÄÖ¸Õë
//--------------------------------------------------------------------------
void KCoreShell::SetRepresentShell(struct iRepresentShell* pRepresent)
{
	g_pRepresent = pRepresent;
	g_ScenePlace.RepresentShellReset();
	if (g_pAdjustColorTab && g_ulAdjustColorCount && g_pRepresent)
		g_pRepresent->SetAdjustColorList(g_pAdjustColorTab, g_ulAdjustColorCount);
}

void KCoreShell::SetMusicInterface(void* pMusicInterface)
{
	g_pMusic = (KMusic*)pMusicInterface;
	Option.SetMusicVolume(Option.GetMusicVolume());
}

//ÈÕ³£»î¶¯£¬coreÈç¹ûÒªÊÙÖÕÕýÇÞÔò·µ»Ø0£¬·ñÔò·µ»Ø·Ç0Öµ
int KCoreShell::Breathe()
{
	g_SubWorldSet.MessageLoop();
	g_SubWorldSet.MainLoop();
	//g_ScenePlace.Breathe();
	return true;
}

int KCoreShell::GetProtocolSize(BYTE byProtocol)
{
	if (byProtocol <= s2c_clientbegin || byProtocol >= s2c_end)
		return -1;
	return g_nProtocolSize[byProtocol - s2c_clientbegin - 1];
}

#ifdef SWORDONLINE_SHOW_DBUG_INFO
extern int		g_bShowObstacle;
extern bool		g_bShowGameInfo;	//ÊÇ·ñÏÔÊ¾ÓÎÏ·£¨³¡¾°£©ÐÅÏ¢
#endif
int KCoreShell::Debug(unsigned int uDataId, unsigned int uParam, int nParam)
{
#ifdef SWORDONLINE_SHOW_DBUG_INFO
	switch(uDataId)
	{
	case DEBUG_SHOWINFO:
		Player[CLIENT_PLAYER_INDEX].m_DebugMode = !Player[CLIENT_PLAYER_INDEX].m_DebugMode;
		g_bShowGameInfo = !g_bShowGameInfo;
		break;
	case DEBUG_SHOWOBSTACLE:
		g_bShowObstacle = !g_bShowObstacle;
		break;
	}
#endif
	return 0;
}

DWORD KCoreShell::GetPing()
{
	return g_SubWorldSet.GetPing();
}

//void KCoreShell::SendPing()
//{
//	SendClientCmdPing();
//}

void KCoreShell::SetRepresentAreaSize(int nWidth, int nHeight)
{
	g_ScenePlace.SetRepresentAreaSize(nWidth, nHeight);
}

void KCoreShell::SetClient(LPVOID pClient)
{
	g_SetClient(pClient);
}

void KCoreShell::SendNewDataToServer(void* pData, int nLength)
{
	if (g_pClient)
		g_pClient->SendPackToServer(pData, nLength);
}

//ÓëµØÍ¼Ïà¹ØµÄ²Ù×÷
int	KCoreShell::SceneMapOperation(unsigned int uOper, unsigned int uParam, int nParam)
{
	//g_DebugLog("scene 1 [%u]", uOper);
	int nRet = 0;
	switch(uOper)
	{
	case GSMOI_SCENE_TIME_INFO:
		if (uParam)
		{
			KUiSceneTimeInfo* pInfo = (KUiSceneTimeInfo*)uParam;
			g_ScenePlace.GetSceneNameAndFocus(pInfo->szSceneName, pInfo->nSceneId,
				pInfo->nScenePos0, pInfo->nScenePos1);
			pInfo->nGameSpaceTime = (SubWorld[0].m_dwCurrentTime / 100) % 1440;
		}
		break;
	case GSMOI_SCENE_MAP_INFO:
		nRet = g_ScenePlace.GetMapInfo((KSceneMapInfo*)uParam);
		break;
	case GSMOI_IS_SCENE_MAP_SHOWING:
		g_ScenePlace.SetMapParam(uParam, nParam);
		break;
	case GSMOI_PAINT_SCENE_MAP:
		g_ScenePlace.PaintMap(uParam, nParam);
		break;
	case GSMOI_SCENE_MAP_FOCUS_OFFSET:
		g_ScenePlace.SetMapFocusPositionOffset((int)uParam, nParam);
		break;
	case GSMOI_SCENE_FOLLOW_WITH_MAP:	//ÉèÖÃ³¡¾°ÊÇ·ñËæ×ÅµØÍ¼µÄÒÆ¶¯¶øÒÆ¶¯
		g_ScenePlace.FollowMapMove(nParam);
		break;
	case GSMOI_SCENE_MAP_FLAG_ON_TARGET:
		g_ScenePlace.FlagOnTarget(uParam, nParam);
		break;
	case GSMOI_SCENE_MAP_REMOVE_FLAG:
		g_ScenePlace.RemoveFlag();
		break;
	case GSMOI_IS_SCENE_MAP_FLAGIMG:
		g_ScenePlace.SetFlagImage((char*)uParam, nParam);
		break;
	case GSMOI_SCENE_MAP_GET_FLAGPOS:
		nRet = g_ScenePlace.GetCurFlagPos(uParam, nParam);
		break;
	case GSMOI_CLICK_2_SPACECOORD:
	{
		if(Player[CLIENT_PLAYER_INDEX].m_nIndex > 0)
		{
			unsigned int* puPr = (unsigned int*)uParam;
			int* pnPr = (int*)nParam;
			g_ScenePlace.GetClickPos(*puPr, *(puPr+1), *pnPr, *(pnPr+1));
		}
	}
		break;
	}
	//g_DebugLog("scene 2 [%u]", uOper);
	return nRet;
}

//Óë°ï»áÏà¹ØµÄ²Ù×÷, uOperµÄÈ¡ÖµÀ´×Ô GAME_TONG_OPERATION_INDEX
int	KCoreShell::TongOperation(unsigned int uOper, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uOper)
	{
	case GTOI_TONG_CREATE:		//gui lenh tao bang
		Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyCreateTong(nParam, (char *)uParam);
		break;
	case GTOI_TONG_IS_RECRUIT:		//²éÑ¯Ä³ÈËµÄÕÐÈË¿ª¹Ø
		//uParam = (KUiPlayerItem*) Òª²éË­
		//Return = (int)(bool)		ÊÇ·ñ¿ª×ÅµÄÕÐÈË¿ª¹Ø
		if (uParam)
		{
			KUiPlayerItem	*pItem = (KUiPlayerItem*)uParam;
			nRet = Npc[pItem->nIndex].m_nTongFlag;
		}
		break;
	case GTOI_TONG_RECRUIT:     //ÕÐÈË¿ª¹Ø
		//uParam = (int)(bool)bRecruit ÊÇ·ñÔ¸ÒâÕÐÈË
		break;
	case GTOI_TONG_ACTION:         //gui lenh thuc hien chuc nang bang
		{
		    switch(nParam)
		    {
		    case TONG_ACTION_DISMISS:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyKick(uParam);
    			break;
		    case TONG_ACTION_ASSIGN:
			{
				KTongOperationParam *Oper = (KTongOperationParam *)uParam;
    			Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInstate((UINT)Oper->nData[0], Oper->nData[1]);
			    break;
			}
		    case TONG_ACTION_DEMISE:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeMaster(uParam);
    			break;
		    case TONG_ACTION_LEAVE:
    			Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyLeave();
    			break;
		    case TONG_ACTION_RECRUIT:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeRecruit();
    			break;
		    case TONG_ACTION_APPLY:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyAddTong((UINT)Oper->nData[0]);
					break;
				}
			case TONG_ACTION_RIGHT:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyRight((UINT)Oper->nData[0], (UINT)Oper->nData[1]);
					break;
				}
			case TONG_ACTION_CONTRIBMONEY:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyContribMoney(uParam);
    			break;
			case TONG_ACTION_WITHDRAWMONEY:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyWithdrawMoney(uParam);
    			break;
			case TONG_ACTION_STOREOFFER:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyStoreOffer(uParam);
				break;
			case TONG_ACTION_DISPENSEOFFER:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyDispenseOffer((UINT)Oper->nData[0], Oper->nData[1]);
					break;
				}
			case TONG_ACTION_ASSIGNMONEY:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyAssignMoney(Oper->nData[0], Oper->nData[1], Oper->nData[2]);
					break;
				}
			case TONG_ACTION_ASSIGNOFFER:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyAssignOffer(Oper->nData[0], Oper->nData[1], Oper->nData[2]);
					break;
				}
			case TONG_ACTION_TRANSMONEY:
				{
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyTransMoney(uParam);
					break;
				}
			case TONG_ACTION_STOREBUILDFUND:
				{
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyStoreBuildFund(uParam);
					break;
				}
			case TONG_ACTION_ANNOUNCE:
				{
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeAnnounce((char*)uParam);
					break;
				}
			case TONG_ACTION_CHANGETITLE:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeTitle((UINT)Oper->nData[0], Oper->nData[1], Oper->Name);
					break;
				}
			case TONG_ACTION_CHANGETITLE_MALE:
				{
					KTongOperationParam *Oper = (KTongOperationParam *)uParam;
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeTitleAll((BYTE)Oper->nData[0], Oper->Name);
					break;
				}
			case TONG_ACTION_CHANGECAMP:
				{
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyChangeCamp(uParam);
					break;
				}
			case TONG_ACTION_UPBUILDLEVEL:
			case TONG_ACTION_ENTERMAP:
			case TONG_ACTION_CREATEMAP:
			case TONG_ACTION_CONFIGMAP:
				{
					Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyAction(nParam);
					break;
				}
		    }
		}
		break;

	//tra loi chap nhan cho vao bang hay khong
	case GTOI_TONG_JOIN_REPLY:
		if (uParam)
		{
			KUiPlayerItem	*pItem = (KUiPlayerItem*)uParam;
			Player[CLIENT_PLAYER_INDEX].m_cTong.AcceptMember(pItem->nIndex, g_FileName2Id(pItem->Name), nParam);
		}
		break;

	case GTOI_REQUEST_PLAYER_TONG:	//lay thong tin bang
		if (uParam)
		{
			KUiPlayerItem	*pItem = (KUiPlayerItem*)uParam;
			Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInfo(nParam, pItem->uId, pItem->nIndex, pItem->nData);
		}
		break;
	case GTOI_REQUEST_TONG_DATA:     //ÒªÇóÄ³¸ö°ï»áµÄ¸÷ÖÖ×ÊÁÏ
	//uParam = (KUiGameObjectWithName*)pTong Òª²éÑ¯µÄ°ï»á
	//KUiGameObjectWithName::szName °ï»áµÄÃû×Ö
	//KUiGameObjectWithName::nData ×ÊÁÏµÄÖÖÀà£¬ÖµÈ¡×ÔÃ¶¾ÙTONG_MEMBER_FIGURE
	//			ÁÐ±íµÄÖÖÀàÊÇenumTONG_FIGURE_MASTERµÄ»°´ú±íÒªÇóµÄÊÇ°ï»áµÄ×ÊÑ¶¡£
	//KUiGameObjectWithName::nParam ¿ªÊ¼µÄË÷Òý
		/*if (uParam)
		{
			if (Player[CLIENT_PLAYER_INDEX].m_cTong.CheckIn() == 0)
				break;

			KUiGameObjectWithName	*pObj = (KUiGameObjectWithName*)uParam;
			char	szTongName[32];
			DWORD	dwTongNameID;

			szTongName[0] = 0;
			Player[CLIENT_PLAYER_INDEX].m_cTong.GetTongName(szTongName);
			if (!szTongName[0])
				break;

			// Ö»ÄÜ²éÑ¯×Ô¼º°ï»áÐÅÏ¢
			dwTongNameID = g_FileName2Id(pObj->szName);
			if (dwTongNameID != Player[CLIENT_PLAYER_INDEX].m_cTong.GetTongNameID())
				break;

			switch (pObj->nData)
			{
			case enumTONG_FIGURE_MASTER:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInfo(
					enumTONG_APPLY_INFO_ID_TONG_HEAD,
					Player[CLIENT_PLAYER_INDEX].m_nIndex, 0, 0);
				break;
			case enumTONG_FIGURE_DIRECTOR:
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInfo(
					enumTONG_APPLY_INFO_ID_TONG_HEAD,
					Player[CLIENT_PLAYER_INDEX].m_nIndex, 0, 0);
				break;
			case enumTONG_FIGURE_MANAGER:
				if (!Player[CLIENT_PLAYER_INDEX].m_cTong.CanGetManagerInfo(dwTongNameID))
					break;
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInfo(
					enumTONG_APPLY_INFO_ID_MANAGER,
					dwTongNameID, nParam, defTONG_ONE_PAGE_MAX_NUM);
				break;
			case enumTONG_FIGURE_MEMBER:
				if (!Player[CLIENT_PLAYER_INDEX].m_cTong.CanGetMemberInfo(dwTongNameID))
					break;
				Player[CLIENT_PLAYER_INDEX].m_cTong.ApplyInfo(
					enumTONG_APPLY_INFO_ID_MEMBER,
					dwTongNameID, nParam, defTONG_ONE_PAGE_MAX_NUM);
				break;
			}
		}*/
		break;
	case GTOI_GETTONGID_NPC:
		{
			if(uParam > 0 && uParam < MAX_NPC)
			{
				nRet = Npc[uParam].m_TongID;
			}
			else
			{
				nRet = Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_TongID;
			}
		}
		break;
	case GTOI_GETTONG_FIGURE:
		{
			nRet = -1;
			if(Player[CLIENT_PLAYER_INDEX].m_cTong.CheckIn())
				nRet = Player[CLIENT_PLAYER_INDEX].m_cTong.GetFigure();
		}
		break;
	case GTOI_GET_WEEKOFFER:
		{
			nRet = Player[CLIENT_PLAYER_INDEX].m_cTong.GetWeekOffer();
		}
		break;
	case GTOI_GET_PERSONALOFFER:
		{
			nRet = Player[CLIENT_PLAYER_INDEX].m_nOffer;
		}
		break;
	case GTOI_GETTONG_RIGHT:
		{
			nRet = (int)Player[CLIENT_PLAYER_INDEX].m_cTong.GetRight();
		}
		break;
	}
	return nRet;
}

//Óë×é¶ÓÏà¹ØµÄ²Ù×÷£¬uOperµÄÈ¡ÖµÀ´×Ô GAME_TEAM_OPERATION_INDEX
int KCoreShell::TeamOperation(unsigned int uOper, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uOper)
	{
	case TEAM_OI_GD_INFO:		//Ö÷½ÇËùÔÚµÄ¶ÓÎéÐÅÏ¢
		if (uParam)
		{
			KUiPlayerTeam* pTeam = (KUiPlayerTeam*)uParam;
			nRet = Player[CLIENT_PLAYER_INDEX].m_cTeam.GetInfo(pTeam);
		}
		break;
	case TEAM_OI_GD_MEMBER_LIST://»ñÈ¡Ö÷½ÇËùÔÚ¶ÓÎé³ÉÔ±ÁÐ±í
		nRet = g_Team[0].GetMemberInfo((KUiPlayerItem *)uParam, nParam);
		break;
	case TEAM_OI_GD_REFUSE_INVITE_STATUS://»ñÈ¡¾Ü¾øÑûÇëµÄ×´Ì¬
		nRet = Player[CLIENT_PLAYER_INDEX].m_cTeam.GetAutoRefuseState();
		break;
	case TEAM_OI_COLLECT_NEARBY_LIST://»ñÈ¡ÖÜÎ§¶ÓÎéµÄÁÐ±í
		NpcSet.GetAroundOpenCaptain(Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_Camp);
		break;
	case TEAM_OI_APPLY:			//ÉêÇë¼ÓÈëËýÈË¶ÓÎé
		if (uParam)
			Player[CLIENT_PLAYER_INDEX].ApplyAddTeam(((KUiTeamItem*)uParam)->Leader.nIndex);
		break;
	case TEAM_OI_CREATE:		//ÐÂ×é¶ÓÎé
		Player[CLIENT_PLAYER_INDEX].ApplyCreateTeam();//(char*)uParam);
		break;
	case TEAM_OI_APPOINT:		//ÈÎÃü¶Ó³¤£¬Ö»ÓÐ¶Ó³¤µ÷ÓÃ²ÅÓÐÐ§¹û
		Player[CLIENT_PLAYER_INDEX].ApplyTeamChangeCaptain(((KUiPlayerItem*)uParam)->uId);		
		break;
	case TEAM_OI_INVITE:		//ÑûÇë±ðÈË¼ÓÈë¶ÓÎé£¬Ö»ÓÐ¶Ó³¤µ÷ÓÃ²ÅÓÐÐ§¹û
		if (uParam)
		{
			Player[CLIENT_PLAYER_INDEX].TeamInviteAdd(((KUiPlayerItem*)uParam)->uId);

			KSystemMessage	sMsg;
			sprintf(sMsg.szMessage, MSG_TEAM_SEND_INVITE, ((KUiPlayerItem*)uParam)->Name);
			sMsg.eType = SMT_NORMAL;
			sMsg.byConfirmType = SMCT_NONE;
			sMsg.byPriority = 0;
			sMsg.byParamSize = 0;
			CoreDataChanged(GDCNI_SYSTEM_MESSAGE, (unsigned int)&sMsg, 0);
		}
		break;
	case TEAM_OI_KICK:			//Ìß³ý¶ÓÀïµÄÒ»¸ö¶ÓÔ±£¬Ö»ÓÐ¶Ó³¤µ÷ÓÃ²ÅÓÐÐ§¹û
		Player[CLIENT_PLAYER_INDEX].TeamKickMember(((KUiPlayerItem*)uParam)->uId);
		break;
	case TEAM_OI_LEAVE:			//Àë¿ª¶ÓÎé
		Player[CLIENT_PLAYER_INDEX].LeaveTeam();
		break;
	case TEAM_OI_CLOSE:			//¹Ø±Õ×é¶Ó£¬Ö»ÓÐ¶Ó³¤µ÷ÓÃ²ÅÓÐÐ§¹û
		Player[CLIENT_PLAYER_INDEX].ApplyTeamOpenClose(nParam);
		break;
	case TEAM_OI_REFUSE_INVITE:		//¾Ü¾ø±ðÈËÑûÇë×Ô¼º¼ÓÈë¶ÓÎé
		Player[CLIENT_PLAYER_INDEX].m_cTeam.SetAutoRefuseInvite(nParam);
		break;
	case TEAM_OI_APPLY_RESPONSE:		//Åú×¼ËûÈË¼ÓÈë¶ÓÎé£¬Ö»ÓÐ¶Ó³¤µ÷ÓÃ²ÅÓÐÐ§¹û
		if (uParam)
		{
			if (nParam)
			{
				Player[CLIENT_PLAYER_INDEX].AcceptTeamMember(((KUiPlayerItem*)uParam)->uId);
			}
			else
			{
				Player[CLIENT_PLAYER_INDEX].m_cTeam.DeleteOneFromApplyList(((KUiPlayerItem*)uParam)->uId);
//				Player[CLIENT_PLAYER_INDEX].m_cTeam.UpdateInterface();
			}
		}
		break;
	case TEAM_OI_INVITE_RESPONSE://¶Ô×é¶ÓÑûÇëµÄ»Ø¸´
		if (uParam)
			Player[CLIENT_PLAYER_INDEX].m_cTeam.ReplyInvite(((KUiPlayerItem*)uParam)->nIndex, nParam);
		break;
	}
	return nRet;
}