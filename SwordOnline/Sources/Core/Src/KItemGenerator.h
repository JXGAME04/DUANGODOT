//---------------------------------------------------------------------------
// Sword3 Core (c) 2002 by Kingsoft
//
// File:	KItemGenerator.h
// Date:	2002.08.26
// Code:	DongBo
// Desc:    header file. 本文件定义的类用于生成道具
//---------------------------------------------------------------------------

#ifndef	KItemGeneratorH
#define	KItemGeneratorH

#include "KBasPropTbl.h"
#include "KItem.h"

#define		IN
#define		OUT

#define	NUMOFCMA	150		// 经验值. 供每种装备使用的魔法总数不超过此数

//class KItem;

class KItemGenerator
{
public:
	KItemGenerator();
	~KItemGenerator();

// 以下是核心成员变量
protected:
	KLibOfBPT	m_BPTLib;

// 以下是辅助成员变量
	int			m_GMA_naryMA[2][NUMOFCMA];	// [0][x]: 前缀; [1][x]: 后缀
	int			m_GMA_nCount[2];
	int			m_GMA_naryLevel[2][NUMOFCMA];
	int			m_GMA_naryDropRate[2][NUMOFCMA];
	int			m_GMA_naryCandidateMA[NUMOFCMA];
	int			m_EquipNumOfEntries[equip_mask+1];
	int			m_MedNumOfEntries;
// 以下是对外接口
public:
	BOOL Init();
	BOOL Gen_Quest(IN int, IN OUT KItem*, IN int nCount = 0);
	BOOL Gen_TownPortal(IN OUT KItem*);
	BOOL Gen_Medicine(IN int, IN int, IN OUT KItem*);
	BOOL Gen_MAScript(IN int, IN int, IN int, IN int, IN int, IN const int*, IN OUT KItem*);
	BOOL Gen_Equipment(IN int, IN int, IN int, IN int, IN const int*, IN int, IN OUT KItem*);
	BOOL Gen_PurpleEquip(IN int, IN int, IN int, IN int, IN const int*, IN int, IN OUT KItem*);
	BOOL Gen_ExistEquipment(IN int, IN int, IN int, IN int, IN const int*, IN OUT KItem*);	
	BOOL Gen_GoldEquip(IN int, IN int, IN OUT KItem*, IN int* pnaryMALevel);
	const KBASICPROP_EQUIPMENT_GOLD* GetGoldItemRecord(IN int nIndex);
	int GetGoldItemNumber();
	const KEQCP_REQ* GetSuitACRecord(IN int nIndex)
	{
		return m_BPTLib.GetSuitACRecord(nIndex);
	}
	int GetSuitACNumber()
	{
		return m_BPTLib.GetSuitACNumber();
	}
	int FindGoldStartId(int nGroupId)
	{
		return m_BPTLib.FindGoldStartId(nGroupId);
	}
	const KBASICPROP_MASCRIPT* GetMAScriptRecord(IN int nIndex)
	{
		return m_BPTLib.GetMAScriptRecord(nIndex);
	}
private:
	BOOL Gen_MagicAttrib(int, const int*, int, int, KItemNormalAttrib*);
	const KMAGICATTRIB_TABFILE* GetMARecord(int) const;
	BOOL GMA_GetAvaliableMA(int);
	BOOL GMA_GetLevelAndDropRate(int);
	int  GMA_GetCandidateMA(int, int, int);
	void GMA_ChooseMA(int nPos, int nLevel, int nLucky, KItemNormalAttrib* pINA);
};

extern KItemGenerator	ItemGen;			//	装备生成器
#endif	// #ifndef	KItemGeneratorH
