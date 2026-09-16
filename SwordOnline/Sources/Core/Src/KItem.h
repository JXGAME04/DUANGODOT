//---------------------------------------------------------------------------
// File:	KItem.h
// Editor:	Guve
// Desc:	Header File
//---------------------------------------------------------------------------

#ifndef	KItemH
#define	KItemH

#include	"KBasPropTbl.h"
#include	"KMagicAttrib.h"
#include	"GameDataDef.h"
#ifndef _SERVER
#include	 "../../Represent/iRepresent/KRepresentUnit.h"
#endif
#include	"KTabFile.h"
//#include	"KPlayer.h"
class KPlayer;
class KIniFile;

#define		IN
#define		OUT

#ifdef _SERVER
#define		MAX_ITEM	160000
#else
#define		MAX_ITEM	512
#endif
#define		KItemNormalAttrib KMagicAttrib
/*
//×°±¸ÊôÐÔ×÷ÓÃÓÚPlayerµÄºÎÖÖÊôÐÔ
#define	ID_LIFE		0
#define	ID_MAXLIFE	1
#define	ID_LIEFREPLENISH		2

#define	ID_MANA		3
#define	ID_MAXMANA	4
#define	ID_MANAREPLENISH		5
*/

enum ITEMGENRE
{
	item_equip = 0,			// trang bÞ trªn ng­êi
	item_medicine,			// 1 thuèc men
	item_mine,				// kh«ng xµi
	item_materials,			// kh«ng xµi
	item_task,				// 4 vËt phÈm nhiÖm vô
	item_townportal,		// 5 thæ ®Þa phï
	item_mascript,			// 6 vËt phÈm cã ma thuËt hoÆc script
	item_broken,			// 7 trang bÞ h­
	item_number,			// sè l­îng lo¹i
	item_gengold=16,		// dïng ®Ó add item hoµng kim
	item_genplatina,		// dïng ®Ó add item b¹ch kim
	item_genpurple,			// dïng ®Ó add item tÝm
};

enum EQUIPDETAILTYPE
{
/*0 */equip_meleeweapon = 0,
/*1 */equip_rangeweapon,
/*2 */equip_armor,
/*3 */equip_ring,
/*4 */equip_amulet,
/*5 */equip_boots,
/*6 */equip_belt,
/*7 */equip_helm,
/*8 */equip_cuff,
/*9 */equip_pendant,
/*10*/equip_horse,
/*11*/equip_mask,
/*12*/equip_mantle,
/*13*/equip_signet,
/*14*/equip_shipin,
/*15*/equip_detailnum,
};

enum MEDICINEDETAILTYPE
{
	medicine_blood = 0,
	medicine_mana,
	medicine_both,
	medicine_stamina,
	medicine_antipoison,
	medicine_detailnum,
};

typedef struct
{
	int		nItemGenre;				// KiÓu item (th«ng sè 1)
	int		nDetailType;			// th«ng sè 2
	int		nParticularType;		// th«ng sè 3
	int		nObjIdx;				// ID trong file obj
	int		nStack;					// sè xÕp chång
	int		nMaxStack;				// sè xÕp chång tèi ®a
	int		nWidth;					// réng «
	int		nHeight;				// cao «
	int		nPrice;					// gi¸
	int		nLevel;					// cÊp
	int		nSeries;				// ngò hµnh
	char	szItemName[80];			// tªn
#ifndef _SERVER
	char	szImageName[80];		// h×nh spr
	char	szIntro[240];			// ®o¹n m« t¶
#endif
} KItemCommonAttrib;


class KNpc;

class KItem
{
public:
	KItem();
	~KItem();

private:
	KItemCommonAttrib	m_CommonAttrib;			// thuéc tÝnh chung

public:
	KItemNormalAttrib	m_aryBaseAttrib[7];		// thuéc tÝnh c¬ b¶n
	KItemNormalAttrib	m_aryRequireAttrib[6];	// thuéc tÝnh yªu cÇu
	KItemNormalAttrib	m_aryMagicAttrib[6];	// thuéc tÝnh ma ph¸p
	KItemNormalAttrib	m_aryMagicAttribEx[2];	// thuéc tÝnh ma ph¸p më réng
private:
	UINT	m_dwID;								// ID t¹o ra t¨ng dÇn
	int		m_nCurrentDur;						// ®é bÒn hiÖn t¹i
	int		m_nExType;					//ph©n lo¹i më réng: th­êng, hoµng kim, tÝm, . . .
	int		m_nGenParam;					//th«ng sè khi t¹o (ID hoµng kim)
	int		m_nGroupId;					//sè bé hoµng kim, hoÆc skillid cña item magicscrippt
	int		m_nExGroupId;				//sè bé hoµng kim më réng
	int		m_nGroupSerial;				//sè thø tù tõng mãn trong 1 bé
#ifdef _SERVER
	BOOL	m_bCanSell;
#endif

#ifndef _SERVER
	KRUImage	m_Image;
	BOOL	m_bShowLevel;
	BOOL	m_bShowSeries;
	BOOL	m_bShortKey;
#endif
public:
	void	ApplyMagicAttribToNPC(IN KNpc*, IN int = 0, IN BOOL bActAll = FALSE) const;
	void	RemoveMagicAttribFromNPC(IN KNpc*, IN int = 0, IN BOOL bActAll = FALSE) const;
	void	ApplyHiddenMagicAttribToNPC(IN KNpc*, IN int) const;
	void	RemoveHiddenMagicAttribFromNPC(IN KNpc*, IN int) const;
	void	SetID(UINT dwID) { m_dwID = dwID; };
	UINT	GetID() const { return m_dwID; };
	void	SetExType(int nExType) {if(nExType >=0 && nExType < extype_number) m_nExType = nExType;};
	int		GetExType() {return m_nExType;};
	void	SetGenParam(int nParam) {m_nGenParam = nParam;};
	int		GetGenParam() {return m_nGenParam;};
	int		GetDetailType() const { return m_CommonAttrib.nDetailType; };
	int		GetGenre() const { return m_CommonAttrib.nItemGenre; };
	int		GetSeries() const { return m_CommonAttrib.nSeries; };
	int		GetParticular() const { return m_CommonAttrib.nParticularType; };
	int		GetLevel() const { return m_CommonAttrib.nLevel; };
	void	SetLevel(int nLevel) { m_CommonAttrib.nLevel = nLevel; };
	void	SetSeries(int nSeries) { m_CommonAttrib.nSeries = nSeries; };
	int		GetWidth() const { return m_CommonAttrib.nWidth; };
	int		GetHeight() const { return m_CommonAttrib.nHeight; };
	int		GetPrice() const { return m_CommonAttrib.nPrice; };
	char*	GetName() const { return (char *)m_CommonAttrib.szItemName; };
	int		GetObjIdx() const { return m_CommonAttrib.nObjIdx;};
	int		GetStack() const { return m_CommonAttrib.nStack;};
	int		GetMaxStack() const { return m_CommonAttrib.nMaxStack;};
	void	SetCount(int nCount)
	{
		if(nCount > 0 && m_CommonAttrib.nMaxStack > 0)
		{
			m_CommonAttrib.nStack = nCount;
			if(m_CommonAttrib.nStack > m_CommonAttrib.nMaxStack)
			{
				m_CommonAttrib.nStack = m_CommonAttrib.nMaxStack;
			}
		}
	}
	void*	GetRequirement(IN int);
	int		GetMaxDurability();
	int		GetTotalMagicLevel();
	int		GetRepairPrice();
	void	Remove();
	BOOL	SetBaseAttrib(IN const KItemNormalAttrib*);
	BOOL	SetRequireAttrib(IN const KItemNormalAttrib*);
	BOOL	SetMagicAttrib(IN const KItemNormalAttrib*);
	void	SetDurability(IN const int nDur) { m_nCurrentDur = nDur; };
	int		GetDurability() { return m_nCurrentDur; };
	int		Abrade(IN const int nRange);
	BOOL	CanBeRepaired();
#ifndef _SERVER
	void	Paint(int nX, int nY);
	void	GetDesc(char* pszMsg, bool bShowPrice = false, int nPriceScale = 1,
			int nActiveAttrib = 0, BOOL bActivateAll = FALSE, int nUiType = -1);
#endif
	void	SetAttrib_Extend(const KBASICPROP_EQUIPMENT_GOLD*);
	void	GetGroups(int& nGroupId, int& nExGroupId, int& nGroupSerial)
	{
		nGroupId = m_nGroupId;
		nExGroupId = m_nExGroupId;
		nGroupSerial = m_nGroupSerial;
	}
	void	SetGroupId(int nId)
	{
		m_nGroupId = nId;
	}
friend class	KItemGenerator;
friend class	KPlayer;
friend class	KItemList;
private:
	BOOL SetAttrib_CBR(IN const KBASICPROP_EQUIPMENT*);
	BOOL SetAttrib_MA(IN const KItemNormalAttrib* pMA, bool bExMagic = false);
	BOOL SetAttrib_MA(IN const KMACP*);
	void operator = (const KBASICPROP_EQUIPMENT&);
	void operator = (const KBASICPROP_MEDICINE&);
	void operator = (const KBASICPROP_QUEST&);
	void operator = (const KBASICPROP_TOWNPORTAL&);
	void operator = (const KBASICPROP_MASCRIPT&);
private:
	BOOL SetAttrib_Base(const KEQCP_BASIC*);
	BOOL SetAttrib_Req(const KEQCP_REQ*);
};

extern KItem Item[MAX_ITEM];

#endif
