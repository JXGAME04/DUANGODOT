//---------------------------------------------------------------------------
// File:	KBasPropTbl.h
// Editor: Guve
//---------------------------------------------------------------------------

#ifndef	KBasPropTblH
#define	KBasPropTblH

#define		SZBUFLEN_0	80		// µäÐÍµÄ×Ö·û´®»º³åÇø³¤¶È
#define		SZBUFLEN_1	240		// µäÐÍµÄ×Ö·û´®»º³åÇø³¤¶È

#define		MAX_MAGIC_PREFIX	20
#define		MAX_MAGIC_SUFFIX	20

// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊöÒ©Æ·ÊôÐÔµÄÌØÐÔ£ºÊýÖµÓëÊ±¼ä
typedef struct
{
	int			nAttrib;
	int			nValue;
	int			nTime;
} KMEDATTRIB;

// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊöÒ©Æ·µÄ»ù±¾ÊôÐÔ. Ïà¹ØÊôÐÔÓÉÅäÖÃÎÄ¼þ(tab file)Ìá¹©
// ÊÊÓÃÓÚÒÔÏÂÒ©Æ·: ÉúÃü²¹³äÀà,ÄÚÁ¦²¹³äÀà,ÌåÁ¦²¹³äÀà,¶¾Ò©Àà,½â¶¾Àà,
//					½âÈ¼ÉÕÀà,½â±ù¶³Àà
typedef struct
{
	char		m_szName[SZBUFLEN_0];		// Ãû³Æ
	int			m_nItemGenre;				// µÀ¾ßÖÖÀà
	int			m_nDetailType;				// ¾ßÌåÀà±ð
	int			m_nParticularType;			// ÏêÏ¸Àà±ð
	char		m_szImageName[SZBUFLEN_0];	// ½çÃæÖÐµÄ¶¯»­ÎÄ¼þÃû
	int			m_nObjIdx;					// ¶ÔÓ¦Îï¼þË÷Òý
	int			m_nWidth;					// µÀ¾ßÀ¸ÖÐËùÕ¼¿í¶È
	int			m_nHeight;					// µÀ¾ßÀ¸ÖÐËùÕ¼¸ß¶È
	char		m_szIntro[SZBUFLEN_1];		// ËµÃ÷ÎÄ×Ö
	int			m_nPrice;					// ¼Û¸ñ
	int			m_nLevel;					// µÈ¼¶
	KMEDATTRIB	m_aryAttrib[2];				// Ò©Æ·µÄÊôÐÔ
} KBASICPROP_MEDICINE;

// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊöÒ»¶Ô×î´ó,×îÐ¡Öµ
typedef struct
{
	int			nMin;
	int			nMax;
} KMINMAXPAIR;

// ÒÔÏÂ½á¹¹ÓÃÓÚ¸ø³ö×°±¸µÄºËÐÄ²ÎÊý: »ù´¡ÊôÐÔ
typedef struct
{
	int			nType;						// ÊôÐÔÀàÐÍ
	KMINMAXPAIR	sRange;						// È¡Öµ·¶Î§
} KEQCP_BASIC;	// Equipment_CorePara_Basic

// ÒÔÏÂ½á¹¹ÓÃÓÚ¸ø³ö×°±¸µÄºËÐÄ²ÎÊý: ÐèÇóÊôÐÔ
typedef struct
{
	int			nType;						// ÊôÐÔÀàÐÍ
	int			nPara;						// ÊýÖµ
} KEQCP_REQ;	// Equipment_CorePara_Requirment

// ÒÔÏÂ½á¹¹ÓÃÓÚ¸ø³öÄ§·¨µÄºËÐÄ²ÎÊý
typedef struct
{
	int			nPropKind;					// ÐÞ¸ÄµÄÊôÐÔÀàÐÍ£¨¶ÔÍ¬Ò»¸öÊýÖµ¼Ó°Ù·Ö±ÈºÍ¼ÓµãÊý±»ÈÏÎªÊÇÁ½¸öÊôÐÔ£©
	KMINMAXPAIR	aryRange[3];				// ÐÞ¸ÄÊôÐÔËùÐèµÄ¼¸¸ö²ÎÊý
} KMACP;	// MagicAttrib_CorePara

// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊöÅäÖÃÎÄ¼þÖÐ¸ø³öµÄÄ§·¨ÊôÐÔ. Ïà¹ØÊôÐÔÓÉÅäÖÃÎÄ¼þ(tab file)Ìá¹©
// Add by Freeway Chen in 2003.5.30
#define			MATF_CBDR		    10      // ÎïÆ·ÀàÐÍ type(ÏÖÔÚµÄÖµÎª equip_detailnum)
#define         MATF_PREFIXPOSFIX   2       // Ç°×ººó×º
#define         MATF_SERIES         5       // ÎåÐÐ
#define         MATF_LEVEL          10      // ×î¶àÓÐ10¸ö¼¶±ð
#define         MAX_GOLD_GROUP		1000    // guve sè l­îng group hoµng kim
typedef struct
{
	int			m_nPos;						// Ç°×º»¹ÊÇºó×º
	char		m_szName[SZBUFLEN_0];		// Ãû³Æ
	int			m_nClass;					// ÎåÐÐÒªÇó
	int			m_nLevel;					// µÈ¼¶ÒªÇó
	char		m_szIntro[SZBUFLEN_1];		// ËµÃ÷ÎÄ×Ö
	KMACP		m_MagicAttrib;				// ºËÐÄ²ÎÊý
	int			m_DropRate[MATF_CBDR];		// ³öÏÖ¸ÅÂÊ
    int         m_nUseFlag;                 // ¸ÃÄ§·¨ÊÇ·ñ±»Ê¹ÓÃ¹ý
} KMAGICATTRIB_TABFILE;

// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊö×°±¸µÄ³õÊ¼ÊôÐÔ. Ïà¹ØÊý¾ÝÓÉÅäÖÃÎÄ¼þ(tab file)Ìá¹©
typedef struct
{
	char		m_szName[SZBUFLEN_0];		// Ãû³Æ
	int			m_nItemGenre;				// µÀ¾ßÖÖÀà (ÎäÆ÷? Ò©Æ·? ¿óÊ¯?)
	int			m_nDetailType;				// ¾ßÌåÀà±ð
	int			m_nParticularType;			// ÏêÏ¸Àà±ð
	char		m_szImageName[SZBUFLEN_0];	// ½çÃæÖÐµÄ¶¯»­ÎÄ¼þÃû
	int			m_nObjIdx;					// ¶ÔÓ¦Îï¼þË÷Òý
	int			m_nWidth;					// µÀ¾ßÀ¸ÖÐËùÕ¼¿í¶È
	int			m_nHeight;					// µÀ¾ßÀ¸ÖÐËùÕ¼¸ß¶È
	char		m_szIntro[SZBUFLEN_1];		// ËµÃ÷ÎÄ×Ö
	int			m_nSeries;					// ÎåÐÐÊôÐÔ
	int			m_nPrice;					// ¼Û¸ñ
	int			m_nLevel;					// µÈ¼¶
	KEQCP_BASIC	m_aryPropBasic[7];			// »ù´¡ÊôÐÔ
	KEQCP_REQ	m_aryPropReq[6];			// ÐèÇóÊôÐÔ
} KBASICPROP_EQUIPMENT;


// ÒÔÏÂ½á¹¹ÓÃÓÚÃèÊö»Æ½ð×°±¸µÄ³õÊ¼ÊôÐÔ. Ïà¹ØÊý¾ÝÓÉÅäÖÃÎÄ¼þ(tab file)Ìá¹©
// flying ¸ù¾Ý²ß»®ÐèÇóÐÞ¸Ä×ÔKBASICPROP_EQUIPMENT_UNIQUEÀàÐÍ
struct KBASICPROP_EQUIPMENT_GOLD : KBASICPROP_EQUIPMENT
{
	int			m_aryMagicId[6];
	int			m_aryMagicEx[2];
	int			m_nGroupId;
	int			m_nExGroupId;
	int			m_nGroupSerial;
};

typedef struct
{
	KMACP	m_GoldMA;
} MAATTRIB_GOLDEQUIP;

typedef struct
{
	char		m_szName[SZBUFLEN_0];		// Ãû³Æ
	int			m_nItemGenre;				// µÀ¾ßÖÖÀà
	int			m_nDetailType;				// ¾ßÌåÀà±ð
	char		m_szImageName[SZBUFLEN_0];	// ½çÃæÖÐµÄ¶¯»­ÎÄ¼þÃû
	int			m_nObjIdx;					// ¶ÔÓ¦Îï¼þË÷Òý
	int			m_nWidth;					// µÀ¾ßÀ¸ÖÐËùÕ¼¿í¶È
	int			m_nHeight;					// µÀ¾ßÀ¸ÖÐËùÕ¼¸ß¶È
	int			m_bCanSell;
	int			m_nMaxStack;
	char		m_szIntro[SZBUFLEN_1];		// ËµÃ÷ÎÄ×Ö
} KBASICPROP_QUEST;

typedef struct
{
	char		m_szName[SZBUFLEN_0];
	int			m_nItemGenre;
	int			m_nDetailType;
	int			m_nParticularType;
	char		m_szImageName[SZBUFLEN_0];
	int			m_nObjIdx;
	int			m_nWidth;
	int			m_nHeight;
	char		m_szIntro[SZBUFLEN_1];
	int			m_nPrice;
	char		m_szScript[128];
	int			m_nSkillID;
	int			m_bShowLevel;
	int			m_bShortKey;
	int			m_nMaxStack;
	int			m_bRegSeries;
} KBASICPROP_MASCRIPT;

typedef struct
{
	char		m_szName[SZBUFLEN_0];		// Ãû³Æ
	int			m_nItemGenre;				// µÀ¾ßÖÖÀà
	char		m_szImageName[SZBUFLEN_0];	// ½çÃæÖÐµÄ¶¯»­ÎÄ¼þÃû
	int			m_nObjIdx;					// ¶ÔÓ¦Îï¼þË÷Òý
	int			m_nWidth;					// µÀ¾ßÀ¸ÖÐËùÕ¼¿í¶È
	int			m_nHeight;					// µÀ¾ßÀ¸ÖÐËùÕ¼¸ß¶È
	int			m_nPrice;					// ¼Û¸ñ
	char		m_szIntro[SZBUFLEN_1];		// ËµÃ÷ÎÄ×Ö
} KBASICPROP_TOWNPORTAL;
//=============================================================================

class KBasicPropertyTable			// ËõÐ´: BPT,ÓÃÓÚÅÉÉúÀà
{
public:
	KBasicPropertyTable();
	~KBasicPropertyTable();

// ÒÔÏÂÊÇºËÐÄ³ÉÔ±±äÁ¿
protected:
	void*		m_pBuf;						// Ö¸ÏòÊôÐÔ±í»º³åÇøµÄÖ¸Õë
											// ÊôÐÔ±íÊÇÒ»¸ö½á¹¹Êý×é,
											// Æä¾ßÌåÀàÐÍÓÉÅÉÉúÀà¾ö¶¨
	int			m_nNumOfEntries;			// ÊôÐÔ±íº¬ÓÐ¶àÉÙÏîÊý¾Ý

// ÒÔÏÂÊÇ¸¨ÖúÐÔµÄ³ÉÔ±±äÁ¿
    int         m_nSizeOfEntry;				// Ã¿ÏîÊý¾ÝµÄ´óÐ¡(¼´½á¹¹µÄ´óÐ¡)
	char		m_szTabFile[MAX_PATH];		// tabfileµÄÎÄ¼þÃû

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	virtual BOOL Load();					// ´ÓtabfileÖÐ¶Á³ö³õÊ¼ÊôÐÔÖµ, ÌîÈëÊôÐÔ±í
	int NumOfEntries() const { return m_nNumOfEntries; }

// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	BOOL GetMemory();
	void ReleaseMemory();
	void SetCount(int);
	virtual BOOL LoadRecord(int i, KTabFile* pTF) = 0;
};

// =====>Ò©Æ·<=====
class KBPT_Medicine : public KBasicPropertyTable
{
public:
	KBPT_Medicine();
	~KBPT_Medicine();

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	const KBASICPROP_MEDICINE* GetRecord(IN int) const;

// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

// =====>ÈÎÎñÎïÆ·<=====
class KBPT_Quest : public KBasicPropertyTable
{
public:
	KBPT_Quest();
	~KBPT_Quest();

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	const KBASICPROP_QUEST* GetRecord(IN int) const;

protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class KBPT_TownPortal : public KBasicPropertyTable
{
public:
	KBPT_TownPortal();
	~KBPT_TownPortal();

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	const KBASICPROP_TOWNPORTAL* GetRecord(IN int) const;

protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class KBPT_MagicScript : public KBasicPropertyTable
{
public:
	KBPT_MagicScript();
	~KBPT_MagicScript();

public:
	const KBASICPROP_MASCRIPT* GetRecord(IN int) const;
	int FindRecord(IN int, IN int);
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class KBPT_Equipment : public KBasicPropertyTable
{
public:
	KBPT_Equipment();
	~KBPT_Equipment();

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	const KBASICPROP_EQUIPMENT* GetRecord(IN int) const;
	void Init(IN int);
// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

// flying modify this class
// »Æ½ð×°±¸
class KBPT_Equipment_Gold : public KBasicPropertyTable
{
public:
	KBPT_Equipment_Gold();
	virtual ~KBPT_Equipment_Gold();

// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	const KBASICPROP_EQUIPMENT_GOLD* GetRecord(IN int) const;
	int GetRecordCount() const {return KBasicPropertyTable::NumOfEntries();};
// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class VMA_Magic_GoldEquip : public KBasicPropertyTable
{
public:
	VMA_Magic_GoldEquip();
	virtual ~VMA_Magic_GoldEquip();
public:
	const MAATTRIB_GOLDEQUIP* GetRecord(IN int) const;
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class VMA_Suit_ActivateCount : public KBasicPropertyTable
{
public:
	VMA_Suit_ActivateCount();
	virtual ~VMA_Suit_ActivateCount();
public:
	const KEQCP_REQ* GetRecord(IN int) const;
	int GetRecordCount() const {return KBasicPropertyTable::NumOfEntries();};
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
};

class KBPT_MagicAttrib_TF : public KBasicPropertyTable
{
public:
	KBPT_MagicAttrib_TF();
	~KBPT_MagicAttrib_TF();

// ÒÔÏÂÊÇ¸¨Öú³ÉÔ±±äÁ¿
protected:
	int m_naryMACount[2][MATF_CBDR];	// Ã¿ÖÖ×°±¸¿ÉÊÊÓÃµÄÄ§·¨ÊýÄ¿,·ÖÇ°ºó×º½øÐÐÍ³¼Æ
										// ¹²ÓÐMATF_CBDRÖÖ×°±¸¿ÉÒÔ¾ß±¸Ä§·¨
// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	void GetMACount(int*) const;
	const KMAGICATTRIB_TABFILE* GetRecord(IN int) const;

// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	virtual BOOL LoadRecord(int i, KTabFile* pTF);
	void Init();
};
/*
class KBPT_MagicAttrib : public KBasicPropertyTable
{
public:
	KBPT_MagicAttrib();
	~KBPT_MagicAttrib();

// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
};
*/

//============================================================================

// Add by Freeway Chen in 2003.5.30
class KBPT_ClassMAIT    // Magic Item Index Table
{
public:
	KBPT_ClassMAIT();
	~KBPT_ClassMAIT();

// ÒÔÏÂÊÇºËÐÄ³ÉÔ±±äÁ¿
protected:
	int*	m_pnTable;				// »º³åÇøÖ¸Õë, Ëù´æÊý¾ÝÎª
									// KBPT_MagicAttrib_TF::m_pBufÊý×éµÄÏÂ±ê
	int		m_nSize;				// »º³åÇøÄÚº¬¶àÉÙÏîÊý¾Ý(²¢·Ç×Ö½ÚÊý)

// ÒÔÏÂÊÇ¸¨Öú³ÉÔ±±äÁ¿
	int		m_nNumOfValidData;		// »º³åÇøÖÐÓÐÐ§Êý¾ÝµÄ¸öÊý
									// ³õÊ¼»¯¹¤×÷Íê³Éºóm_nNumOfValidData < m_nSize
// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
    BOOL Clear();
	BOOL Insert(int nItemIndex);
	int  Get(int i) const;
    int  GetCount() const { return m_nNumOfValidData; }
};

//============================================================================

class KBPT_ClassifiedMAT
{
public:
	KBPT_ClassifiedMAT();
	~KBPT_ClassifiedMAT();

// ÒÔÏÂÊÇºËÐÄ³ÉÔ±±äÁ¿
protected:
	int*	m_pnTable;				// »º³åÇøÖ¸Õë, Ëù´æÊý¾ÝÎª
									// KBPT_MagicAttrib_TF::m_pBufÊý×éµÄÏÂ±ê
	int		m_nSize;				// »º³åÇøÄÚº¬¶àÉÙÏîÊý¾Ý(²¢·Ç×Ö½ÚÊý)

// ÒÔÏÂÊÇ¸¨Öú³ÉÔ±±äÁ¿
	int		m_nNumOfValidData;		// »º³åÇøÖÐÓÐÐ§Êý¾ÝµÄ¸öÊý
									// ³õÊ¼»¯¹¤×÷Íê³Éºóm_nNumOfValidData==m_nSize
// ÒÔÏÂÊÇ¶ÔÍâ½Ó¿Ú
public:
	BOOL GetMemory(int);
	BOOL Set(int);
	int Get(int) const;
	BOOL GetAll(int*, int*) const;

// ÒÔÏÂÊÇ¸¨Öúº¯Êý
protected:
	void ReleaseMemory();
};

class KLibOfBPT
{
public:
	KLibOfBPT();
	~KLibOfBPT();

protected:
	KBPT_Medicine			m_BPTMedicine;
	KBPT_TownPortal			m_BPTTownPortal;
	KBPT_Quest				m_BPTQuest;
	KBPT_MagicScript		m_BPTMAScript;
	KBPT_Equipment			m_BPTHorse;
	KBPT_Equipment			m_BPTMask;
	KBPT_Equipment			m_BPTMeleeWeapon;
	KBPT_Equipment			m_BPTRangeWeapon;
	KBPT_Equipment			m_BPTArmor;
	KBPT_Equipment			m_BPTHelm;
	KBPT_Equipment			m_BPTBoot;
	KBPT_Equipment			m_BPTBelt;
	KBPT_Equipment			m_BPTAmulet;
	KBPT_Equipment			m_BPTRing;
	KBPT_Equipment			m_BPTCuff;
	KBPT_Equipment			m_BPTPendant;
	
    KBPT_MagicAttrib_TF		m_BPTMagicAttrib;
	KBPT_Equipment_Gold		m_GoldItem;
	VMA_Magic_GoldEquip		m_GoldMagic;
	VMA_Suit_ActivateCount	m_SuitActCount;
    KBPT_ClassMAIT          m_CMAIT[MATF_PREFIXPOSFIX][MATF_CBDR][MATF_SERIES][MATF_LEVEL];
	KBPT_ClassifiedMAT		m_CMAT[2][MATF_CBDR];
	int						m_GoldStartId[MAX_GOLD_GROUP];
public:
	BOOL Init();

	const KMAGICATTRIB_TABFILE* GetMARecord(IN int) const;
	const int					GetMARecordNumber() const;
    const KBPT_ClassMAIT*       GetCMIT(IN int nPrefixPostfix, IN int nType, IN int nSeries, int nLevel) const;
	const KBPT_ClassifiedMAT*	GetCMAT(IN int, int) const;
	const KBASICPROP_EQUIPMENT_GOLD*	GetGoldItemRecord(IN int nIndex) const;
	const int							GetGoldItemNumber() const;
	const KEQCP_REQ*				GetSuitACRecord(IN int nIndex) const;
	const int						GetSuitACNumber() const;
	const MAATTRIB_GOLDEQUIP*		GetGoldMagicRecord(IN int nIndex) const;
	const KBASICPROP_EQUIPMENT*		GetMeleeWeaponRecord(IN int) const;
	const int						GetMeleeWeaponRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetRangeWeaponRecord(IN int) const;
	const int						GetRangeWeaponRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetArmorRecord(IN int) const;
	const int						GetArmorRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetHelmRecord(IN int) const;
	const int						GetHelmRecordNumber() const;
	const KBASICPROP_EQUIPMENT* 	GetBootRecord(IN int) const;
	const int						GetBootRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetBeltRecord(IN int) const;
	const int						GetBeltRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetAmuletRecord(IN int) const;
	const int						GetAmuletRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetRingRecord(IN int) const;
	const int						GetRingRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetCuffRecord(IN int) const;
	const int						GetCuffRecordNumber() const;
	const KBASICPROP_EQUIPMENT*		GetPendantRecord(IN int) const;
	const int						GetPendantRecordNumber() const;
	const KBASICPROP_EQUIPMENT* 	GetHorseRecord(IN int) const;
	const int						GetHorseRecordNumber() const;
	const KBASICPROP_EQUIPMENT* 	GetMaskRecord(IN int) const;
	const int						GetMaskRecordNumber() const;
	const KBASICPROP_MEDICINE*		GetMedicineRecord(IN int) const;
	const int						GetMedicineRecordNumber() const;
	const KBASICPROP_QUEST*			GetQuestRecord(IN int) const;
	const int						GetQuestRecordNumber() const;
	const KBASICPROP_TOWNPORTAL*	GetTownPortalRecord(IN int) const;
	const int						GetTownPortalRecordNumber() const;
	int								FindGoldStartId(int nGroupId);
	const KBASICPROP_MASCRIPT*		GetMAScriptRecord(IN int) const;
	const int						GetMAScriptRecordNumber() const;
	int								FindMAScriptRecord(IN int, IN int);
protected:
	BOOL InitMALib();
    BOOL InitMAIT();
};
#endif		// #ifndef KBasPropTblH
