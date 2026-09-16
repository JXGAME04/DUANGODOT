#ifndef KPROTOCOL_H
#define KPROTOCOL_H

#ifndef __linux
#ifdef _STANDALONE
#include "GameDataDef.h"
#else
#include "../Sources/Core/src/GameDataDef.h"
#endif
#else
#include "GameDataDef.h"
#include <string.h>
#endif

#include "KProtocolDef.h"
#include "KRelayProtocol.h"

#pragma pack(push, enter_protocol)
#pragma	pack(1)

#define	PROTOCOL_MSG_TYPE	BYTE
#define PROTOCOL_MSG_SIZE	(sizeof(PROTOCOL_MSG_TYPE))
#define	MAX_PROTOCOL_NUM	200

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	bReload;
	int		nParam;
}GM_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	BYTE	WalkSpeed;
	BYTE	RunSpeed;
	BYTE	AttackSpeed;
	BYTE	CastSpeed;
	short	HelmType;
	short	ArmorType;
	short	WeaponType;
	short	MantleType;
	BYTE	HorseType;
	BYTE	RankID;
	UINT	ID;
	BYTE	m_btSomeFlag;	// 0x01 PKFlag 0x02 FightModeFlag 0x04 SleepModeFlag
//	BYTE	m_btPKFlag;
//	BYTE	m_btFightMode;
//	BYTE	m_SleepMode;
//	char	Name[32];	
	char	szTong[defTONG_NAME_MAX_LENGTH+1];
	char	szTitle[defTONG_NAME_MAX_LENGTH+1];
} PLAYER_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		Mana;
	int		ManaMax;
	BYTE	WalkSpeed;			// ÊÇ·ñÖ»Òª´«Ò»·İ¾Í¹»ÁË£¨Ö»¸Ä±äÒ»¸ö»òÊÇÍ¬Ê±¸Ä±ä£©
	BYTE	RunSpeed;
	BYTE	AttackSpeed;
	BYTE	CastSpeed;			// ÊÇ·ñ¿¼ÂÇ²»¸Ä±äÊ©·¨ËÙ¶È£¬»òÊ©·¨ËÙ¶ÈµÈÓÚ¹¥»÷ËÙ¶È
	short	HelmType;
	short	ArmorType;
	short	WeaponType;
	short	MantleType;
	BYTE	HorseType;
	BYTE	RankID;
	BYTE	m_btSomeFlag;	// 0x01 PKFlag 0x02 FightModeFlag 0x04 SleepModeFlag 0x08 TongOpenFlag
//	BYTE	FightMode;
//	BYTE	m_btPKFlag;
//	BYTE	m_btSleepMode;
} PLAYER_NORMAL_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	Camp;				// ÕóÓª
	BYTE	CurrentCamp;		// µ±Ç°ÕóÓª
	BYTE	m_bySeries;			// ÎåĞĞÏµ
	int		Life;
	int		LifeMax;
	BYTE	m_btMenuState;		// ×é¶Ó¡¢½»Ò×µÈ×´Ì¬
	BYTE	m_Doing;			// ĞĞÎª
	BYTE	m_btKind;			// npcÀàĞÍ
	UINT	MapX;				// Î»ÖÃĞÅÏ¢
	UINT	MapY;				// Î»ÖÃĞÅÏ¢
	UINT	ID;					// NpcµÄÎ¨Ò»ID
	int		NpcSettingIdx;		// ¿Í»§¶ËÓÃÓÚ¼ÓÔØÍæ¼Ò×ÊÔ´¼°»ù´¡ÊıÖµÉè¶¨
	WORD	NpcEnchant;			// ¼ÓÇ¿µÄNPC£¨ºóÃæµÄbit±íÊ¾¼ÓÇ¿ÀàĞÍ£¬Ãû×ÖÊÇ·ñ½ğÉ«Ö®ÀàµÄÓÉ¼ÓÇ¿µÄÊıÄ¿ÔÚ¿Í»§¶ËÈ·¶¨£©
	char	m_szName[32];		// Ãû×Ö
} NPC_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	UINT	MapX;
	UINT	MapY;
	int		Life;
	int		LifeMax;
	BYTE	Camp;
	BYTE	Doing;
	BYTE	State;
} NPC_NORMAL_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	m_dwNpcID;
	UINT	m_dwMapX;
	UINT	m_dwMapY;
	int		MapID;
	BYTE	m_byDoing;
	BYTE	m_btCamp;
	BYTE	m_btLifePerCent;	
} NPC_PLAYER_TYPE_NORMAL_SYNC;
                 
typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
} NPC_REMOVE_SYNC, NPC_SIT_SYNC, NPC_DEATH_SYNC, NPC_REQUEST_COMMAND, NPC_REQUEST_FAIL;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		nMpsX;
	int		nMpsY;
} NPC_WALK_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	BYTE	Type;
} NPC_REVIVE_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		nMpsX;
	int		nMpsY;
} NPC_JUMP_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		nMpsX;
	int		nMpsY;
} NPC_RUN_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		nFrames;
	int		nX;
	int		nY;
} NPC_HURT_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	BYTE	Camp;
} NPC_CHGCURCAMP_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	BYTE	Camp;
} NPC_CHGCAMP_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	ID;
	int		nSkillID;
	int		nSkillLevel;
	int		nMpsX;
	int		nMpsY;
} NPC_SKILL_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	int		nSkillID;
	int		nMpsX;
	int		nMpsY;
} NPC_SKILL_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	int		nMpsX;
	int		nMpsY;
} NPC_WALK_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
//	BYTE	ReviveType;
} NPC_REVIVE_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	int		nMpsX;
	int		nMpsY;
} NPC_RUN_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	int		nShopIndex;
} SALE_BOX_SYNC;

typedef struct 
{
	BYTE	ProtocolType;
	int		nNpcId;
} PLAYER_DIALOG_NPC_COMMAND; //Ö÷½ÇÓënNpcId¶Ô»°µÄÇëÇó

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	int		m_nExp;				// µ±Ç°¾­Ñé
} PLAYER_EXP_SYNC;				// Íæ¼ÒÍ¬²½¾­Ñé

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
} PLAYER_APPLY_CREATE_TEAM;		// ¿Í»§¶ËÍæ¼Ò´´½¨¶ÓÎé£¬Ïò·şÎñÆ÷·¢ÇëÇó

struct PLAYER_SEND_CREATE_TEAM_SUCCESS
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	nTeamServerID;		// ¶ÓÎéÔÚ·şÎñÆ÷ÉÏµÄÎ¨Ò»±êÊ¶
	PLAYER_SEND_CREATE_TEAM_SUCCESS() {nTeamServerID = -1;}
};	// ·şÎñÆ÷Í¨ÖªÍæ¼Ò¶ÓÎé´´½¨³É¹¦

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btErrorID;		// ¶ÓÎé´´½¨²»³É¹¦Ô­Òò£º0 Í¬Ãû 1 Íæ¼Ò±¾ÉíÒÑ¾­ÊôÓÚÄ³Ò»Ö§¶ÓÎé 3 µ±Ç°´¦ÓÚ²»ÄÜ×é¶Ó×´Ì¬
} PLAYER_SEND_CREATE_TEAM_FALSE;// ·şÎñÆ÷Í¨Öª¿Í»§¶Ë¶ÓÎé´´½¨²»³É¹¦

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwTarNpcID;		// ²éÑ¯Ä¿±ê npc id
} PLAYER_APPLY_TEAM_INFO;		// ¿Í»§¶ËÏò·şÎñÆ÷ÉêÇë²éÑ¯Ä³¸önpcµÄ×é¶ÓÇé¿ö

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
} PLAYER_APPLY_TEAM_INFO_FALSE;	// ·şÎñÆ÷¸æÖª¿Í»§¶ËÉêÇë²éÑ¯Ä³¸önpcµÄ×é¶ÓÇé¿öÊ§°Ü

typedef struct PLAYER_SEND_TEAM_INFO_DATA
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	int		m_nCaptain;			// ¶Ó³¤ npc id
	int		m_nMember[MAX_TEAM_MEMBER];	// ËùÓĞ¶ÓÔ± npc id
	UINT	nTeamServerID;		// ¶ÓÎéÔÚ·şÎñÆ÷ÉÏµÄÎ¨Ò»±êÊ¶
	PLAYER_SEND_TEAM_INFO_DATA() {nTeamServerID = -1;};
} PLAYER_SEND_TEAM_INFO;		// ·şÎñÆ÷Ïò¿Í»§¶Ë·¢ËÍÄ³¸ö¶ÓÎéµÄĞÅÏ¢Êı¾İ

typedef struct PLAYER_SEND_SELF_TEAM_INFO_DATA
{
	BYTE	ProtocolType;							// Ğ­ÒéÃû³Æ
	BYTE	m_btState;								// ¶ÓÎé×´Ì¬
	UINT	m_dwNpcID[MAX_TEAM_MEMBER + 1];			// Ã¿Ãû³ÉÔ±µÄnpc id £¨¶Ó³¤·ÅÔÚµÚÒ»Î»£©
	char	m_szNpcName[MAX_TEAM_MEMBER + 1][32];	// Ã¿Ãû³ÉÔ±µÄÃû×Ö£¨¶Ó³¤·ÅÔÚµÚÒ»Î»£©
	UINT	nTeamServerID;							// ¶ÓÎéÔÚ·şÎñÆ÷ÉÏµÄÎ¨Ò»±êÊ¶
	UINT	m_dwLeadExp;							// Íæ¼ÒµÄÍ³ÂÊÁ¦¾­Ñé
	BYTE	m_btLevel[MAX_TEAM_MEMBER + 1];			// Ã¿Ãû³ÉÔ±µÄµÈ¼¶£¨¶Ó³¤·ÅÔÚµÚÒ»Î»£©
	PLAYER_SEND_SELF_TEAM_INFO_DATA() {memset(m_szNpcName, 0, 32 * (MAX_TEAM_MEMBER + 1)); nTeamServerID = -1;};
} PLAYER_SEND_SELF_TEAM_INFO;						// ·şÎñÆ÷Ïò¿Í»§¶Ë·¢ËÍ¿Í»§¶Ë×ÔÉí¶ÓÎéµÄĞÅÏ¢Êı¾İ

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btOpenClose;		// ´ò¿ª»ò¹Ø±Õ
} PLAYER_TEAM_OPEN_CLOSE;		// ¶ÓÎé¶Ó³¤Ïò·şÎñÆ÷ÉêÇë¿ª·Å¡¢¹Ø±Õ¶ÓÎéÊÇ·ñÔÊĞí½ÓÊÕ³ÉÔ±×´Ì¬

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwTarNpcID;		// Ä¿±ê¶ÓÎé¶Ó³¤npc id »òÕß ÉêÇëÈË npc id
} PLAYER_APPLY_ADD_TEAM;		// Íæ¼ÒÏò·şÎñÆ÷ÉêÇë¼ÓÈëÄ³¸ö¶ÓÎé»òÕß·şÎñÆ÷ÏòÄ³¸ö¶Ó³¤×ª·¢Ä³¸öÍæ¼ÒµÄ¼ÓÈëÉêÇë

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwNpcID;			// ±»½ÓÊÜÈë¶ÓÎéµÄnpc id
} PLAYER_ACCEPT_TEAM_MEMBER;	// Íæ¼ÒÍ¨Öª·şÎñÆ÷½ÓÊÜÄ³¸öÍæ¼ÒÈë¶ÓÎé

typedef struct PLAYER_TEAM_ADD_MEMBER_DATA
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btLevel;			// ¼ÓÈëÕßµÈ¼¶
	UINT	m_dwNpcID;			// ¼ÓÈëÕßnpc id
	char	m_szName[32];		// ¼ÓÈëÕßĞÕÃû
	PLAYER_TEAM_ADD_MEMBER_DATA() {memset(m_szName, 0, 32);};
} PLAYER_TEAM_ADD_MEMBER;		// ·şÎñÆ÷Í¨Öª¶ÓÎéÖĞµÄ¸÷¸öÍæ¼ÒÓĞĞÂ³ÉÔ±¼ÓÈë

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
} PLAYER_APPLY_LEAVE_TEAM;		// ¿Í»§¶ËÍæ¼ÒÉêÇëÀë¶Ó

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwNpcID;			// Àë¶Ónpc id
} PLAYER_LEAVE_TEAM;			// ·şÎñÆ÷Í¨Öª¸÷¶ÓÔ±Ä³ÈËÀë¶Ó

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwNpcID;			// Àë¶Ónpc id
} PLAYER_TEAM_KICK_MEMBER;		// ¶Ó³¤Ìß³ıÄ³¸ö¶ÓÔ±

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwNpcID;			// Ä¿±ênpc id
} PLAYER_APPLY_TEAM_CHANGE_CAPTAIN;// ¶Ó³¤Ïò·şÎñÆ÷ÉêÇë°Ñ×Ô¼ºµÄ¶Ó³¤Éí·İ½»¸ø±ğµÄ¶ÓÔ±

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwCaptainID;		// ĞÂ¶Ó³¤npc id
	UINT	m_dwMemberID;		// ĞÂ¶ÓÔ±npc id
} PLAYER_TEAM_CHANGE_CAPTAIN;	// ·şÎñÆ÷Í¨Öª¸÷¶ÓÔ±¸ü»»¶Ó³¤

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
} PLAYER_APPLY_TEAM_DISMISS;	// Ïò·şÎñÆ÷ÉêÇë½âÉ¢¶ÓÎé

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btPKFlag;			// pk ¿ª¹Ø
} PLAYER_SET_PK;				// Ïò·şÎñÆ÷ÉêÇë´ò¿ª¡¢¹Ø±ÕPK

typedef struct
{
	BYTE	ProtocolType;			// Ğ­ÒéÃû³Æ
	BYTE	m_btCamp;				// ĞÂÕóÓª
	BYTE	m_btCurFaction;			// µ±Ç°ÃÅÅÉ
	BYTE	m_btFirstFaction;		// Ê×´Î¼ÓÈëÃÅÅÉ
	int		m_nAddTimes;			// ¼ÓÈëÃÅÅÉ´ÎÊı
} PLAYER_FACTION_DATA;				// ·şÎñÆ÷·¢¸ø¿Í»§¶ËÃÅÅÉĞÅÏ¢

typedef struct
{
	BYTE	ProtocolType;			// Ğ­ÒéÃû³Æ
} PLAYER_LEAVE_FACTION;				// ·şÎñÆ÷Í¨ÖªÍæ¼ÒÀë¿ªÃÅÅÉ

typedef struct
{
	BYTE	ProtocolType;			// Ğ­ÒéÃû³Æ
	BYTE	m_btCurFactionID;		// µ±Ç°ÃÅÅÉid
	BYTE	m_btLevel;				// ¼¼ÄÜ¿ª·ÅµÈ¼¶
} PLAYER_FACTION_SKILL_LEVEL;		// ·şÎñÆ÷Í¨ÖªÍæ¼Ò¿ª·Åµ±Ç°ÃÅÅÉ¼¼ÄÜµ½Ä³¸öµÈ¼¶

typedef struct
{
	BYTE	ProtocolType;			// Ğ­ÒéÃû³Æ
} PLAYER_APPLY_FACTION_DATA;		// ¿Í»§¶ËÉêÇë¸üĞÂÃÅÅÉÊı¾İ

typedef struct PLAYER_SEND_CHAT_DATA_COMMAND
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	WORD	m_wLength;
	BYTE	m_btCurChannel;		// µ±Ç°ÁÄÌìÆµµÀ
	BYTE	m_btType;			// MSG_G_CHAT »ò MSG_G_CMD »ò¡­¡­
	BYTE	m_btChatPrefixLen;	// ¸ñÊ½¿ØÖÆ×Ö·û³¤¶È
	WORD	m_wSentenceLen;		// ÁÄÌìÓï¾ä³¤¶È
	UINT	m_dwTargetID;		// ÁÄÌì¶ÔÏó id
	int		m_nTargetIdx;		// ÁÄÌì¶ÔÏóÔÚ·şÎñÆ÷¶ËµÄ idx
	char	m_szSentence[MAX_SENTENCE_LENGTH + CHAT_MSG_PREFIX_MAX_LEN];	// ÁÄÌìÓï¾äÄÚÈİ
	PLAYER_SEND_CHAT_DATA_COMMAND() {memset(m_szSentence, 0, sizeof(m_szSentence));};
} PLAYER_SEND_CHAT_COMMAND;		// ¿Í»§¶ËÁÄÌìÄÚÈİ·¢ËÍ¸ø·şÎñÆ÷

typedef struct PLAYER_SEND_CHAT_DATA_SYNC
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	WORD	m_wLength;
	BYTE	m_btCurChannel;		// µ±Ç°ÁÄÌì×´Ì¬
	BYTE	m_btNameLen;		// Ãû×Ö³¤¶È
	BYTE	m_btChatPrefixLen;	// ¿ØÖÆ×Ö·û³¤¶È
	WORD	m_wSentenceLen;		// ÁÄÌìÓï¾ä³¤¶È
	UINT	m_dwSourceID;		// 
	char	m_szSentence[32 + CHAT_MSG_PREFIX_MAX_LEN + MAX_SENTENCE_LENGTH];	// ÁÄÌìÓï¾äÄÚÈİ
	PLAYER_SEND_CHAT_DATA_SYNC() { memset(m_szSentence, 0, sizeof(m_szSentence)); };
} PLAYER_SEND_CHAT_SYNC;		// ¿Í»§¶ËÁÄÌìÄÚÈİ·¢ËÍ¸ø·şÎñÆ÷

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btState;
	int		m_nID;
	int		m_nDataID;
	int		m_nXpos;
	int		m_nYpos;
	int		m_nMoneyNum;
	int		m_nItemID;
	BYTE	m_btDir;
	BYTE	m_btItemWidth;
	WORD	m_wCurFrame;
	BYTE	m_btItemHeight;
	BYTE	m_btColorID;
	BYTE	m_btFlag;
	char	m_szName[80];
} OBJ_ADD_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	BYTE	m_btState;
	int		m_nID;
} OBJ_SYNC_STATE;

typedef struct
{
	BYTE	ProtocolType;
	BYTE	m_btDir;
	int		m_nID;
} OBJ_SYNC_DIR;

typedef struct
{
	BYTE	ProtocolType;
	int		m_nID;
	BYTE	m_btSoundFlag;
} OBJ_SYNC_REMOVE;

typedef struct
{
	BYTE	ProtocolType;
	int		m_nID;
	int		m_nTarX;
	int		m_nTarY;
} OBJ_SYNC_TRAP_ACT;

typedef struct
{
	BYTE	ProtocolType;
	int		m_nID;
} OBJ_CLIENT_SYNC_ADD;

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	UINT	m_dwLeadExp;		// Í³ÂÊÁ¦¾­ÑéÖµ
} PLAYER_LEAD_EXP_SYNC;			// Í¬²½Í³ÂÊÁ¦¾­ÑéÖµ

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btLevel;			// µ±Ç°µÈ¼¶
	int		m_nExp;				// µ±Ç°¾­Ñé
	int		m_nAttributePoint;	// Ê£ÓàÊôĞÔµã
	int		m_nSkillPoint;		// Ê£Óà¼¼ÄÜµã
	int		m_nBaseLifeMax;		// µ±Ç°×î´óÉúÃüÖµ
	int		m_nBaseStaminaMax;	// µ±Ç°×î´óÌåÁ¦Öµ
	int		m_nBaseManaMax;		// µ±Ç°×î´óÄÚÁ¦Öµ
} PLAYER_LEVEL_UP_SYNC;			// Íæ¼ÒÉı¼¶

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btLevel;			// µ±Ç°µÈ¼¶
	UINT	m_dwTeammateID;		// ¶ÓÓÑ npc id
} PLAYER_TEAMMATE_LEVEL_SYNC;	// Íæ¼ÒÉı¼¶µÄÊ±ºòÍ¨Öª¶ÓÓÑ

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btAttribute;		// ÊôĞÔ(0=Strength 1=Dexterity 2=Vitality 3=Engergy)
	int		m_nAddNo;			// ¼ÓµÄµãÊı
} PLAYER_ADD_BASE_ATTRIBUTE_COMMAND;	// Íæ¼ÒÌí¼Ó»ù±¾ÊôĞÔµã

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	int		m_nSkillID;			// ¼¼ÄÜid
	int		m_nAddPoint;		// Òª¼ÓµÄµãÊı
} PLAYER_ADD_SKILL_POINT_COMMAND;// Íæ¼ÒÉêÇëÔö¼ÓÄ³¸ö¼¼ÄÜµÄµãÊı

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btAttribute;		// ÊôĞÔ(0=Strength 1=Dexterity 2=Vitality 3=Engergy)
	int		m_nBasePoint;		// »ù±¾µãÊı
	int		m_nCurPoint;		// µ±Ç°µãÊı
	int		m_nLeavePoint;		// Ê£ÓàÎ´·ÖÅäÊôĞÔµã
} PLAYER_ATTRIBUTE_SYNC;		// Íæ¼ÒÍ¬²½ÊôĞÔµã

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	int		m_nSkillID;			// ¼¼ÄÜid
	int		m_nSkillLevel;		// ¼¼ÄÜµÈ¼¶
	int		m_nLeavePoint;		// Ê£ÓàÎ´·ÖÅä¼¼ÄÜµã
} PLAYER_SKILL_LEVEL_SYNC;		// Íæ¼ÒÍ¬²½¼¼ÄÜµã

//typedef struct
//{
//	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
//	int		m_nItemID;			// ÎïÆ·id
//	int		m_nSourcePos;		// À´Ô´Î»ÖÃ
//	int		m_nTargetPos;		// Ä¿µÄÎ»ÖÃ
//} PLAYER_EQUIP_ITEM_COMMAND;	// Íæ¼ÒÊó±êÓÒ¼üµã»÷Ê¹ÓÃÎïÆ·(×°±¸)

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	BYTE	m_btPlace;			// Ò©Æ·Î»ÖÃ
	BYTE	m_btX;				// Ò©Æ·Î»ÖÃ
	BYTE	m_btY;				// Ò©Æ·Î»ÖÃ
	int		m_nItemID;			// ÎïÆ·id
} PLAYER_EAT_ITEM_COMMAND;		// Íæ¼ÒÊó±êÓÒ¼üµã»÷Ê¹ÓÃÎïÆ·(³ÔÒ©)

typedef struct
{
	BYTE	ProtocolType;		// Ğ­ÒéÃû³Æ
	int		m_nObjID;			// Êó±êµã»÷µÄobjµÄid
	BYTE	m_btPosType;		// Î»ÖÃÀàĞÍ
	BYTE	m_btPosX;			// ×ø±ê x
	BYTE	m_btPosY;			// ×ø±ê y
} PLAYER_PICKUP_ITEM_COMMAND;		// Íæ¼Ò»ñµÃÎïÆ·£¨Êó±êµã»÷µØÍ¼ÉÏµÄobj£©

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_ID;				// ÎïÆ·µÄID
	WORD			m_Detail;			// ÎïÆ·µÄÀà±ğ
	WORD			m_Particur;			// ÎïÆ·µÄÏêÏ¸Àà
	short			m_Durability;		// ÄÍ¾Ã¶È
	WORD			m_Count;
	BYTE			m_Genre;			// ÎïÆ·µÄÀàĞÍ
	char			m_Series;			// ÎïÆ·µÄÎåĞĞ
	BYTE			m_Level;			// ÎïÆ·µÄµÈ¼¶
	BYTE			m_btPlace;			// ×ø±ê
	BYTE			m_btX;				// ×ø±ê
	BYTE			m_btY;				// ×ø±ê
	int				m_MagicLevel[16];	// Éú³É²ÎÊı
} ITEM_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_ID;				// ÎïÆ·µÄID
} ITEM_REMOVE_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_ID;				// ÎïÆ·µÄID
} PLAYER_SELL_ITEM_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_BuyIdx;			// ÂòµÚ¼¸¸ö¶«Î÷
	BYTE			m_Place;			// ·ÅÔÚÉíÉÏÄÄ¸öµØ·½
	BYTE			m_X;				// ×ø±êX
	BYTE			m_Y;				// ×ø±êY
} PLAYER_BUY_ITEM_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nMoney1;			// ×°±¸À¸
	int				m_nMoney2;			// ÖüÎïÏä
	int				m_nMoney3;			// ½»Ò×À¸
} PLAYER_MONEY_SYNC;					// ·şÎñÆ÷Í¨Öª¿Í»§¶ËÇ®µÄÊıÁ¿

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btDownPos;
	BYTE			m_btDownX;
	BYTE			m_btDownY;
	BYTE			m_btUpPos;
	BYTE			m_btUpX;
	BYTE			m_btUpY;
} PLAYER_MOVE_ITEM_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btDownPos;
	BYTE			m_btDownX;
	BYTE			m_btDownY;
	BYTE			m_btUpPos;
	BYTE			m_btUpX;
	BYTE			m_btUpY;
} PLAYER_MOVE_ITEM_SYNC;

// s2c_ItemAutoMove
typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btSrcPos;
	BYTE			m_btSrcX;
	BYTE			m_btSrcY;
	BYTE			m_btDestPos;
	BYTE			m_btDestX;
	BYTE			m_btDestY;
} ITEM_AUTO_MOVE_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
} PLAYER_THROW_AWAY_ITEM_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nLife;
	int				m_nStamina;
	int				m_nMana;
	int				m_nMaxLife;
	int				m_nMaxStamina;
	int				m_nMaxMana;
	BYTE			m_btTeamData;
} CURPLAYER_NORMAL_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	// npc²¿·Ö
	UINT			m_dwID;				// NpcµÄID
	BYTE			m_btLevel;			// NpcµÄµÈ¼¶
	BYTE			m_btSex;			// ĞÔ±ğ
	BYTE			m_btKind;			// NpcµÄÀàĞÍ
	BYTE			m_btSeries;			// NpcµÄÎåĞĞÏµ
	WORD			m_wLifeMax;			// NpcµÄ×î´óÉúÃü
	WORD			m_wStaminaMax;		// NpcµÄ×î´óÌåÁ¦
	WORD			m_wManaMax;			// NpcµÄ×î´óÄÚÁ¦
	int				m_HeadImage;
	// player ²¿·Ö
	WORD			m_wAttributePoint;	// Î´·ÖÅäÊôĞÔµã
	WORD			m_wSkillPoint;		// Î´·ÖÅä¼¼ÄÜµã
	WORD			m_wStrength;		// Íæ¼ÒµÄ»ù±¾Á¦Á¿£¨¾ö¶¨»ù±¾ÉËº¦£©
	WORD			m_wDexterity;		// Íæ¼ÒµÄ»ù±¾Ãô½İ£¨¾ö¶¨ÃüÖĞ¡¢ÌåÁ¦£©
	WORD			m_wVitality;		// Íæ¼ÒµÄ»ù±¾»îÁ¦£¨¾ö¶¨ÉúÃü¡¢ÌåÁ¦£©
	WORD			m_wEngergy;			// Íæ¼ÒµÄ»ù±¾¾«Á¦£¨¾ö¶¨ÄÚÁ¦£©
	WORD			m_wLucky;			// Íæ¼ÒµÄ»ù±¾ĞÒÔËÖµ
	int				m_nExp;				// µ±Ç°¾­ÑéÖµ(µ±Ç°µÈ¼¶ÔÚnpcÉíÉÏ)
	UINT			m_dwLeadExp;		// Í³ÂÊÁ¦¾­ÑéÖµ

	// ÃÅÅÉ
	BYTE			m_btCurFaction;		// µ±Ç°ÃÅÅÉ
	BYTE			m_btFirstFaction;	// µÚÒ»´Î¼ÓÈëµÄÊÇÄÄ¸öÃÅÅÉ
	int				m_nFactionAddTimes;	// ¼ÓÈë¸÷ÖÖÃÅÅÉµÄ×Ü´ÎÊı
	
	// ÅÅÃû
	WORD			m_wWorldStat;		// ÊÀ½çÅÅÃû
	WORD			m_wSectStat;		// ÃÅÅÉÅÅÃû

	// Ç®
	int				m_nMoney1;
	int				m_nMoney2;
	BYTE			m_bGM;
} CURPLAYER_SYNC;

#define MAX_SCIRPTACTION_BUFFERNUM 300

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wProtocolLong;
	BYTE	m_nOperateType;				//²Ù×÷ÀàĞÍ
	BYTE	m_bUIId, m_bOptionNum, m_bParam1, m_bParam2;// m_bParam1,Ö÷ĞÅÏ¢ÊÇÊı×Ö±êÊ¶»¹ÊÇ×Ö·û´®±êÊ¶, m_bParam2,ÊÇ·ñÊÇÓë·şÎñÆ÷½»»¥µÄÑ¡Ôñ½çÃæ
	int		m_nParam;
	int		m_nBufferLen;
	char	m_pContent[MAX_SCIRPTACTION_BUFFERNUM];				//´ø¿ØÖÆ·û
} PLAYER_SCRIPTACTION_SYNC;

typedef struct
{
	WORD	SkillId;
	BYTE	SkillLevel;
} SKILL_SEND_ALL_SYNC_DATA;

typedef struct
{
	BYTE						ProtocolType;
	WORD						m_wProtocolLong;
	SKILL_SEND_ALL_SYNC_DATA	m_sAllSkill[80];
} SKILL_SEND_ALL_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	BYTE	WeatherID;
} SYNC_WEATHER;

typedef struct defWORLD_SYNC
{
	BYTE	ProtocolType;
	int		SubWorld;
	int		Region;
	BYTE	Weather;
	UINT	Frame;
} WORLD_SYNC;

typedef struct 
{
	BYTE	ProtocolType;
	int		nSelectIndex;
}PLAYER_SELECTUI_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwTakeChannel;	// ¶©ÔÄÆµµÀ
} CHAT_SET_CHANNEL_COMMAND;				// Éè¶¨¶©ÔÄÆµµÀ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;
	UINT			m_dwTargetNpcID;	// Ä¿±ê npc id
	char			m_szInfo[MAX_SENTENCE_LENGTH];// ¸ø¶Ô·½µÄ»°
} CHAT_APPLY_ADD_FRIEND_COMMAND;		// ÁÄÌìÌí¼ÓºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nSrcPlayerIdx;	// À´Ô´ player idx
	char			m_szSourceName[32];	// À´Ô´Íæ¼ÒÃû×Ö
	char			m_szInfo[MAX_SENTENCE_LENGTH];// ¶Ô·½¸øµÄ»°
} CHAT_APPLY_ADD_FRIEND_SYNC;			// ÁÄÌìÌí¼ÓºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nTargetPlayerIdx;	// ±»½ÓÊÜplayer idx
} CHAT_ADD_FRIEND_COMMAND;				// Ìí¼ÓÄ³Íæ¼ÒÎªÁÄÌìºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nTargetPlayerIdx;	// ±»¾Ü¾øplayer idx
} CHAT_REFUSE_FRIEND_COMMAND;			// ¾Ü¾øÌí¼ÓÄ³Íæ¼ÒÎªÁÄÌìºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ĞÂÌí¼ÓºÃÓÑµÄ id
	int				m_nIdx;				// ĞÂÌí¼ÓºÃÓÑÔÚ player Êı×éÖĞµÄÎ»ÖÃ
	char			m_szName[32];		// ĞÂÌí¼ÓºÃÓÑµÄÃû×Ö
} CHAT_ADD_FRIEND_SYNC;					// Í¨Öª¿Í»§¶Ë³É¹¦Ìí¼ÓÒ»¸öÁÄÌìºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;
	char			m_szName[32];		// ¾Ü¾øÕßÃû×Ö
} CHAT_REFUSE_FRIEND_SYNC;				// Í¨Öª¿Í»§¶ËÌí¼ÓÁÄÌìºÃÓÑµÄÉêÇë±»¾Ü¾ø

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nTargetPlayerIdx;	// ³ö´í player idx (Ò»°ã¿ÉÄÜÊÇ´ËplayerÏÂÏß»òÕß»»·şÎñÆ÷ÁË)
} CHAT_ADD_FRIEND_FAIL_SYNC;			// Í¨Öª¿Í»§¶ËÌí¼ÓÁÄÌìºÃÓÑÊ§°Ü

typedef struct
{
	BYTE			ProtocolType;	// c2s_viewequip
	UINT			m_dwNpcID;
} VIEW_EQUIP_COMMAND;

//´Ë½á¹¹ÒÑ¾­±»tagDBSelPlayer½á¹¹Ìæ»»
/*typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nSelect;			// 
} DB_PLAYERSELECT_COMMAND;*/

/*
 * { Add by liupeng 2003.05.10
 *
 * #pragma pack( push, 1 )
*/

/*
 * Nonstandard extension used : zero-sized array in struct/union
 */
#pragma warning(disable: 4200)

#define KSG_PASSWORD_MAX_SIZE   64

typedef struct tagKSG_PASSWORD
{
    char szPassword[KSG_PASSWORD_MAX_SIZE];    // ÏÖÔÚ²ÉÓÃMD5µÄ×Ö·û´®£¬ÓÉÓÚÊÇ32¸ö×Ö·û£¬¼ÓÉÏÄ©Î²'\0'£¬ĞèÒªÖÁÉÙ33¸ö¿Õ¼ä£¬Òò´ËÊ¹ÓÃ64
} KSG_PASSWORD;

#define _NAME_LEN	32

struct tagProtoHeader
{
	BYTE	cProtocol;
};

struct tagResult : public tagProtoHeader
{
	BYTE	cResult;
};

struct tagDBSelPlayer : public tagProtoHeader
{
	char	szRoleName[_NAME_LEN];
};

struct tagDBDelPlayer : public tagProtoHeader
{
	char	        szAccountName[_NAME_LEN];
    KSG_PASSWORD    Password;
	char	        szRoleName[_NAME_LEN];
};

//É¾³ıÓëĞÂ½¨½ÇÉ«µÄ·µ»ØÏûÏ¢´øµÄÊı¾İ
struct tagNewDelRoleResponse : public tagDBSelPlayer
{
	bool	bSucceeded;		//ÊÇ·ñ³É¹¦
};

// 2003.05.11
struct tagDBSyncPlayerInfo : public tagProtoHeader
{
	size_t	dataLength;
	char	szData[0];
};

/*
 * 2003.06.27
 * s2c_gateway_broadcast
*/

#define	AP_WARNING_ALL_PLAYER_QUIT			1
#define	AP_NOTIFY_GAMESERVER_SAFECLOSE		2
#define	AP_NOTIFY_ALL_PLAYER				3
#define	MAX_GATEWAYBROADCAST_LEN	260
struct tagGatewayBroadCast : public tagProtoHeader
{
	UINT	uCmdType;
	char	szData[MAX_GATEWAYBROADCAST_LEN];
};

/*
 * 2003.05.22
 * s2c_syncgamesvr_roleinfo_cipher
*/
struct tagGuidableInfo : public tagProtoHeader
{
	GUID guid;
	WORD nExtPoint;			//¿ÉÓÃµÄ¸½ËÍµã
	WORD nChangePoint;		//±ä»¯µÄ¸½ËÍµã
	size_t	datalength;
	char	szData[0];

};

/*
 * c2s_permitplayerlogin
 */
struct tagPermitPlayerLogin : public tagProtoHeader
{
	GUID guid;

	BYTE szRoleName[_NAME_LEN];

	/*
	 * Succeeded : true
	 * Failed	 : false
	 */
	bool bPermit;
};

struct tagPermitPlayerExchange
{
	BYTE cProtocol;
	GUID guid;
	UINT dwIp;
	WORD wPort;
	bool bPermit;
};
/*
 * c2s_notifyplayerlogin
 */
struct tagNotifyPlayerLogin : public tagPermitPlayerLogin
{
	UINT			nIPAddr;
	unsigned short	nPort;
};

/*
 * s2c_querymapinfo
 */
struct tagQueryMapInfo : public tagProtoHeader
{	
};

/*
 * s2c_querygameserverinfo
 */
struct tagQueryGameSvrInfo : public tagProtoHeader
{	
};

/*
 * s2c_notifysvrip
 */
struct tagNotifySvrIp : public tagProtoHeader
{
	WORD	pckgID;

	BYTE	cIPType;
	UINT	dwMapID;

	UINT	dwSvrIP;
};

/*
 * s2c_notifyplayerexchange
 */
struct tagNotifyPlayerExchange : public tagProtoHeader
{
	GUID			guid;
	UINT			nIPAddr;
	unsigned short	nPort;
};

/*
 * c2s_requestsvrip
 */

/*
 * BYTE	cIPType
 */
#define INTRANER_IP	0
#define INTERNET_IP 1

struct tagRequestSvrIp : public tagProtoHeader
{
	WORD	pckgID;

	BYTE	cIPType;
	UINT	dwMapID;
};

/*
 * c2c_notifyexchange
 */
struct tagSearchWay : public tagProtoHeader
{
	int		lnID;
	int		nIndex;
	UINT	dwPlayerID;
};

/*
 * c2s_updatemapinfo
 */
struct tagUpdateMapID : public tagProtoHeader
{
	/*
	 * For example : Are your clear older information when it 
	 *		update local informatin
	 */
	BYTE cReserve;

	BYTE cMapCount;

	USHORT szMapID[0];	// C4200 warning
};

/*
 * c2s_updategameserverinfo
 */
struct tagGameSvrInfo : public tagProtoHeader
{
	UINT			nIPAddr_Intraner;
	UINT			nIPAddr_Internet;

	unsigned short	nPort;
	WORD			wCapability;
};

/*
 * s2c_identitymapping
 */
struct tagIdentityMapping : public tagGameSvrInfo
{
	GUID guid;
};

/*
 * c2s_logiclogin
 * s2c_gmgateway2relaysvr
 * s2c_gmnotify
 */
struct tagLogicLogin : public tagProtoHeader
{
	GUID guid;
};

/*
 * s2c_logiclogout
 */
struct tagLogicLogout : public tagProtoHeader
{
	BYTE szRoleName[_NAME_LEN];
};

/*
 * c2s_registeraccount
 */
struct tagRegisterAccount : public tagProtoHeader
{
	BYTE szAccountName[_NAME_LEN];
};

/*
 * c2s_entergame
 */
struct tagEnterGame : public tagProtoHeader
{
	/*
	 * Succeeded : content is account name
	 * Failed	 : content is null
	 */

	BYTE szAccountName[_NAME_LEN];
};

struct tagEnterGame2 : public EXTEND_HEADER
{
	char szAccountName[_NAME_LEN];
	char szCharacterName[_NAME_LEN];
	UINT	dwNameID;
	unsigned long	lnID;
};

/*
 * c2s_leavegame
 */

/*
 * BYTE cCmdType
 */
#define NORMAL_LEAVEGAME	0x0		// lock account
#define HOLDACC_LEAVEGAME	0x1A	// clear resource but don't to unlock account 

struct tagLeaveGame : public tagProtoHeader
{
	BYTE cCmdType;
	WORD nExtPoint;        //½«Òª¿Û³ıµÄ¸½ËÍµã
	/*
	 * Succeeded : content is account name
	 * Failed	 : content is null
	 */

	char szAccountName[_NAME_LEN];
};

struct tagLeaveGame2 : public EXTEND_HEADER
{
	BYTE cCmdType;
	char szAccountName[_NAME_LEN];
};
/*
*  c2s_registerfamily
*/
struct tagRegisterFamily : public tagProtoHeader
{
	BYTE bRegister;		//1 is Register, 0 is unRegister
	BYTE nFamily;
	BYTE RelayMethod;
};

/*
 * c2s_gmsvr2gateway_saverole
 */
struct tagGS2GWSaveRole : public tagProtoHeader
{
	size_t	datalength;
	BYTE	szData[0];
};

/*
 * #pragma pack( pop )
 *
 * } End of the struct define
 */

typedef struct
{
	char	szName[32];
	BYTE	Sex;
	BYTE	Series;
//	BYTE	HelmType;
//	BYTE	ArmorType;
//	BYTE	WeaponType;
	BYTE	Level;
} RoleBaseInfo/* client */, S3DBI_RoleBaseInfo /* server */;

typedef struct
{
	BYTE				ProtocolType;
	RoleBaseInfo		m_RoleList[MAX_PLAYER_IN_ACCOUNT];
} ROLE_LIST_SYNC;

//ÒÆ×ÔRoleDBManager/kroledbheader.h
//ÓÃÀ´Ìæ»»ÉÏÃæµÄROLE_LIST_SYNC,ROLE_LIST_SYNC½á¹¹²»ÔÙĞèÒªÁË
struct TProcessData
{
	unsigned char	nProtoId;
	size_t			nDataLen;//TRoleNetMsgÊ±±íÊ¾¸ÃBlockµÄÊµ¼ÊÊı¾İ³¤¶È,TProcessDataÊ±±íÊ¾StreamµÄÊµ¼ÊÊı¾İ³¤¶È
	unsigned long	ulIdentity;
	bool			bLeave;
	char			pDataBuffer[1];//Êµ¼ÊµÄÊı¾İ
};

struct tagRoleEnterGame
{
	BYTE			ProtocolType;
	bool			bLock;
	char			Name[_NAME_LEN];
};

//ĞÂ½¨½ÇÉ«µÄĞÅÏ¢½á¹¹
//×¢ÊÍ£ºĞÂ½¨¾öÏûÏ¢c2s_newplayer£¬´«ËÍµÄ²ÎÊıÎªTProcessData½á¹¹ÃèÊöµÄÊı¾İ£¬ÆäÖĞTProcessData::pDataBufferÒªÀ©Õ¹ÎªNEW_PLAYER_COMMAND
struct NEW_PLAYER_COMMAND
{
	BYTE			m_btRoleNo;			// ½ÇÉ«±àºÅ
	BYTE			m_btSeries;			// ÎåĞĞÏµ
	unsigned short	m_NativePlaceId;	//³öÉúµØID
	char			m_szName[32];		// ĞÕÃû
};


typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ºÃÓÑ id
	int				m_nPlayerIdx;		// ºÃÓÑ player index
} CHAT_LOGIN_FRIEND_NONAME_SYNC;		// Íæ¼ÒµÇÂ¼Ê±·¢ËÍÍæ¼ÒÁÄÌìºÃÓÑÊı¾İ£¨²»´øÃû×Ö£©

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;
	UINT			m_dwID;				// ºÃÓÑ id
	int				m_nPlayerIdx;		// ºÃÓÑ player index
	char			m_szName[32];		// ºÃÓÑÃû×Ö
} CHAT_LOGIN_FRIEND_NAME_SYNC;			// Íæ¼ÒµÇÂ¼Ê±·¢ËÍÍæ¼ÒÁÄÌìºÃÓÑÊı¾İ£¨´øÃû×Ö£©

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
} CHAT_APPLY_RESEND_ALL_FRIEND_NAME_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ºÃÓÑ id
} CHAT_APPLY_SEND_ONE_FRIEND_NAME_COMMAND;	// ÉêÇëµÃµ½Íæ¼ÒÄ³¸öÁÄÌìºÃÓÑµÄÍêÕûÊı¾İ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ºÃÓÑ id
	int				m_nPlayerIdx;		// ºÃÓÑ player index
	char			m_szName[32];		// ºÃÓÑÃû×Ö
} CHAT_ONE_FRIEND_DATA_SYNC;			// ·¢ËÍÍæ¼ÒÄ³Ò»¸öÁÄÌìºÃÓÑÊı¾İ£¨´øÃû×Ö£©

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ºÃÓÑ id
	int				m_nPlayerIdx;		// ºÃÓÑ player index
} CHAT_FRIEND_ONLINE_SYNC;				// Í¨Öª¿Í»§¶ËÓĞºÃÓÑÉÏÏß

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ±»É¾³ıid
} CHAT_DELETE_FRIEND_COMMAND;			// É¾³ıÄ³¸öÁÄÌìºÃÓÑ

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// É¾³ıid
} CHAT_DELETE_FRIEND_SYNC;				// ±»Ä³¸öÁÄÌìºÃÓÑÉ¾³ı

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ±»É¾³ıid
} CHAT_REDELETE_FRIEND_COMMAND;			// É¾³ıÄ³¸öÁÄÌìºÃÓÑ

typedef struct 
{
	BYTE			ProtocolType;
	BYTE			m_LogoutType;
} LOGOUT_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	BYTE			szAccName[32];
} LOGIN_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;				// ºÃÓÑ id
} CHAT_FRIEND_OFFLINE_SYNC;				// Í¨Öª¿Í»§¶ËÓĞºÃÓÑÏÂÏß

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;
	char			m_szSentence[MAX_SENTENCE_LENGTH];
} TRADE_APPLY_OPEN_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
} TRADE_APPLY_CLOSE_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btState;			// if == 0 close if == 1 open if == 2 trading
	UINT			m_dwNpcID;			// Èç¹ûÊÇ¿ªÊ¼½»Ò×£¬¶Ô·½µÄ npc id
} TRADE_CHANGE_STATE_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;
	UINT			m_dwID;
	BYTE			m_btState;
	char			m_szSentence[MAX_SENTENCE_LENGTH];
} NPC_SET_MENU_STATE_SYNC;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwID;
} TRADE_APPLY_START_COMMAND;

// ·şÎñÆ÷×ª·¢½»Ò×ÉêÇë
typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nDestIdx;			// ÉêÇëÕßÔÚ·şÎñÆ÷¶ËµÄplayer idx
	UINT			m_dwNpcId;			// ÉêÇëÕßµÄ npc id
} TRADE_APPLY_START_SYNC;

// ½ÓÊÜ»ò¾Ü¾ø±ğÈËµÄ½»Ò×ÉêÇë
typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_bDecision;		// Í¬Òâ 1 ²»Í¬Òâ 0
	int				m_nDestIdx;			// ½»Ò×¶Ô·½ÔÚ·şÎñÆ÷¶ËµÄplayer idx
} TRADE_REPLY_START_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nMoney;
} TRADE_MOVE_MONEY_COMMAND;		// c2s_trademovemoney

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	int				m_nMoney;
} TRADE_MONEY_SYNC;				// s2c_trademoneysync

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btDecision;		// È·¶¨½»Ò× 1  ÍË³ö½»Ò× 0  È¡ÏûÈ·¶¨ 4  Ëø¶¨½»Ò× 2  È¡ÏûËø¶¨ 3
} TRADE_DECISION_COMMAND;				// ½»Ò×Ö´ĞĞ»òÈ¡Ïû c2s_tradedecision

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btDecision;		// ½»Ò×ok 1  ½»Ò×È¡Ïû 0  Ëø¶¨ 2  È¡ÏûËø¶¨ 3
} TRADE_DECISION_SYNC;					// s2c_tradedecision

typedef struct
{
	BYTE			ProtocolType;		
	BYTE			m_byDir;			// È¡Ç®µÄ·½Ïò£¨0´æ£¬1È¡£©
	UINT			m_dwMoney;			// Ç®Êı
} STORE_MONEY_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;			// ³¤¶È
	BYTE			m_btError;			// ´íÎóÀàĞÍ	0 ¶Ô·½¹Ø±ÕÁË´ËÆµµÀ£¬1 ÕÒ²»µ½¶Ô·½
	char			m_szName[32];		// ¶Ô·½Ãû×Ö
} CHAT_SCREENSINGLE_ERROR_SYNC;

typedef struct 
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	BYTE			m_btStateInfo[MAX_NPC_RECORDER_STATE];
	UINT			m_ID;				// NpcµÄGID
}	NPC_SYNC_STATEINFO;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	UINT			m_dwNpcID;
} TEAM_INVITE_ADD_COMMAND;

typedef struct
{
	BYTE			ProtocolType;		// Ğ­ÒéÀàĞÍ
	WORD			m_wLength;			// ³¤¶È
	int				m_nIdx;
	char			m_szName[32];
} TEAM_INVITE_ADD_SYNC;

typedef struct
{
	BYTE			ProtocolType;		//
	int				m_nAuraSkill;
} SKILL_CHANGEAURASKILL_COMMAND;		//¸ü»»¹â»·¼¼ÄÜ

typedef struct
{
	BYTE			ProtocolType;
	BYTE			m_btResult;
	int				m_nIndex;
} TEAM_REPLY_INVITE_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	BYTE			m_btSelfLock;
	BYTE			m_btDestLock;
	BYTE			m_btSelfOk;
	BYTE			m_btDestOk;
} TRADE_STATE_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	WORD			m_wLength;
	UINT			m_dwSkillID;		// ¼¼ÄÜ
	int				m_nLevel;
	int				m_nTime;			// Ê±¼ä
	KMagicAttrib	m_MagicAttrib[MAX_SKILL_STATE];
} STATE_EFFECT_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	UINT			m_dwTime;
} PING_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	UINT			m_dwReplyServerTime;
	UINT			m_dwClientTime;
} PING_CLIENTREPLY_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	BYTE			m_btSitFlag;
} NPC_SIT_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	int				nMpsX;
	int				nMpsY;
} NPC_JUMP_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	int				m_dwRegionID;
	int				m_nObjID;
} OBJ_MOUSE_CLICK_SYNC;

typedef struct tagSHOW_MSG_SYNC
{
	BYTE			ProtocolType;
	WORD			m_wLength;
	WORD			m_wMsgID;
	LPVOID			m_lpBuf;
	tagSHOW_MSG_SYNC() {m_lpBuf = NULL;};
	~tagSHOW_MSG_SYNC() {Release();}
	void	Release() {if (m_lpBuf) delete []m_lpBuf; m_lpBuf = NULL;}
} SHOW_MSG_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	BYTE			m_btFlag;
} PK_APPLY_NORMAL_FLAG_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	BYTE			m_btFlag;
} PK_NORMAL_FLAG_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	UINT			m_dwNpcID;
} PK_APPLY_ENMITY_COMMAND;

typedef struct
{
	BYTE			ProtocolType;
	WORD			m_wLength;
	BYTE			m_btState;
	UINT			m_dwNpcID;
	char			m_szName[32];
} PK_ENMITY_STATE_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	WORD			m_wLength;
	BYTE			m_btState;
	UINT			m_dwNpcID;
	char			m_szName[32];
} PK_EXERCISE_STATE_SYNC;

typedef struct
{
	BYTE			ProtocolType;
	int				m_nPKValue;
} PK_VALUE_SYNC;

typedef struct
{
	int		m_nID;				// ÎïÆ·µÄID
	BYTE	m_btGenre;			// ÎïÆ·µÄÀàĞÍ
	WORD	m_btDetail;			// ÎïÆ·µÄÀà±ğ
	WORD	m_btParticur;		// ÎïÆ·µÄÏêÏ¸À
	WORD	m_Count;
	char	m_btSeries;			// ÎïÆ·µÄÎåĞĞ
	BYTE	m_btLevel;			// ÎïÆ·µÄµÈ¼¶
	int		m_btMagicLevel[16];	// Éú³É²ÎÊı
} SViewItemInfo;

typedef struct
{
	BYTE			ProtocolType;
	UINT			m_dwNpcID;
	BYTE			nPart;
	SViewItemInfo	m_sInfo;
} VIEW_EQUIP_SYNC;				// s2c_viewequip

typedef struct//¸Ã½á¹¹ÊÇËùÍ³¼ÆµÄÍæ¼ÒµÄ»ù±¾Êı¾İ
{
	char	Name[20];
	int		nValue;
	BYTE	bySort;
}TRoleList;

// ÓÎÏ·Í³¼Æ½á¹¹
typedef struct
{
	TRoleList MoneyStat[10];			//½ğÇ®×î¶àÅÅÃûÁĞ±í£¨Ê®¸öÍæ¼Ò£¬×î¶à¿É´ïµ½100¸ö£©
	TRoleList LevelStat[10];			//¼¶±ğ×î¶àÅÅÃûÁĞ±í£¨Ê®¸öÍæ¼Ò£¬×î¶à¿É´ïµ½100¸ö£©
	TRoleList KillerStat[10];			//É±ÈË×î¶àÅÅÃûÁĞ±í
	
	//[ÃÅÅÉºÅ][Íæ¼ÒÊı]£¬ÆäÖĞ[0]ÊÇÃ»ÓĞ¼ÓÈëÃÅÅÉµÄÍæ¼Ò
	TRoleList MoneyStatBySect[11][10];	//¸÷ÃÅÅÉ½ğÇ®×î¶àÅÅÃûÁĞ±í
	TRoleList LevelStatBySect[11][10];	//¸÷ÃÅÅÉ¼¶±ğ×î¶àÅÅÃûÁĞ±í

	//[ÃÅÅÉºÅ]£¬ÆäÖĞ[0]ÊÇÃ»ÓĞ¼ÓÈëÃÅÅÉµÄÍæ¼Ò
	int SectPlayerNum[11];				//¸÷¸öÃÅÅÉµÄÍæ¼ÒÊı
	int SectMoneyMost[11];				//²Æ¸»ÅÅÃûÇ°Ò»°ÙÍæ¼ÒÖĞ¸÷ÃÅÅÉËùÕ¼±ÈÀıÊı
	int SectLevelMost[11];				//¼¶±ğÅÅÃûÇ°Ò»°ÙÍæ¼ÒÖĞ¸÷ÃÅÅÉËùÕ¼±ÈÀıÊı
}  TGAME_STAT_DATA;

typedef struct
{
	BYTE	ProtocolType;
	BYTE	bSleep;
	UINT	NpcID;
} NPC_SLEEP_SYNC;

//////////////
//ÅÅÃûÏà¹Ø
typedef struct
{
	BYTE		ProtocolType;
	UINT		dwLadderID;
	TRoleList	StatData[10];
} LADDER_DATA;

typedef struct
{
	BYTE		ProtocolType;
	WORD		wSize;
	int			nCount;
	UINT		dwLadderID[0];
} LADDER_LIST;

typedef struct
{
	BYTE		ProtocolType;
	UINT		dwLadderID;
} LADDER_QUERY;


///////////////////
//chat Ïà¹Ø

typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	UINT	packageID;
	char	someone[_NAME_LEN];
	BYTE	sentlen;
} CHAT_SOMEONECHAT_CMD, CHAT_SOMEONECHAT_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	UINT	packageID;
	BYTE	filter;
	UINT	channelid;
	BYTE	cost;	//0: ÎŞÏŞÖÆ£¬1: 10Ôª/¾ä£¬2: <10Lv ? ²»ÄÜËµ : MaxMana/2/¾ä, 3: MaxMana/10/¾ä,4: <20Lv ? ²»ÄÜËµ : MaxMana*4/5/¾ä
	BYTE	sentlen;
} CHAT_CHANNELCHAT_CMD;

typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	UINT	packageID;
	char	someone[_NAME_LEN];
	UINT	channelid;
	BYTE	sentlen;
} CHAT_CHANNELCHAT_SYNC;

enum {codeSucc, codeFail, codeStore};
typedef struct
{
	BYTE	ProtocolType;
	UINT	packageID;
	BYTE	code;
} CHAT_FEEDBACK;

typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	WORD	wChatLength;
} CHAT_EVERYONE;


typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	WORD	wChatLength;
	BYTE	byHasIdentify;
	WORD	wPlayerCount;
} CHAT_GROUPMAN;


typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	UINT	nameid;
	unsigned long lnID;
	WORD	wChatLength;
} CHAT_SPECMAN;


enum { tgtcls_team, tgtcls_fac, tgtcls_tong, tgtcls_scrn, tgtcls_bc};
typedef struct
{
	BYTE	ProtocolType;
	WORD	wSize;
	UINT	nFromIP;
	UINT	nFromRelayID;
	UINT	channelid;
	BYTE	TargetCls;
	UINT	TargetID;
	WORD	routeDateLength;
} CHAT_RELEGATE;


///////////////////////////////////////
// tong Ïà¹Ø

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
} S2C_TONG_HEAD;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
} STONG_PROTOCOL_HEAD;

typedef struct
{
	int		m_nPlayerIdx;
	int		m_nCamp;
	char	m_szTongName[defTONG_NAME_MAX_LENGTH + 1];
} STONG_SERVER_TO_CORE_APPLY_CREATE;

typedef struct
{
	int		m_nPlayerIdx;
	UINT	m_uDestID;
} STONG_SERVER_TO_CORE_APPLY_ADD;

typedef struct
{
	int		m_nSelfIdx;
	int		m_nTargetIdx;
	UINT	m_dwNameID;
} STONG_SERVER_TO_CORE_CHECK_ADD_CONDITION;

typedef struct
{
	int		m_nCamp;
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	char	m_szTongName[32];
} STONG_SERVER_TO_CORE_CREATE_SUCCESS;

typedef struct
{
	int		m_nSelfIdx;
	int		m_nTargetIdx;
	UINT	m_dwNameID;
} STONG_SERVER_TO_CORE_REFUSE_ADD;

typedef struct
{
	int		m_nSelfIdx;
	int		m_nInfoID;
	UINT	m_nParam1;
	int		m_nParam2;
	int		m_nParam3;
} STONG_SERVER_TO_CORE_GET_INFO;

typedef struct
{
	int		m_nPlayerIdx;
	int		m_nCamp;
	UINT	m_dwPlayerNameID;
	int		m_nFigure;
	char	m_szTongName[32];
	char	m_szMasterName[32];
} STONG_SERVER_TO_CORE_ADD_SUCCESS;

typedef struct
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	BYTE	m_btFigure;
} STONG_SERVER_TO_CORE_BE_INSTATED;

typedef struct
{
	UINT	m_dwTongNameID;
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
} STONG_SERVER_TO_CORE_BE_KICKED;

typedef struct
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	BYTE	m_btSuccessFlag;
} STONG_SERVER_TO_CORE_LEAVE;

typedef struct
{
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	char	m_btFigure;
} STONG_SERVER_TO_CORE_CHANGE_AS;

typedef struct
{
	UINT	m_dwParam;
	UINT	m_uRight;
	BYTE	m_nFlag;
	BYTE	m_nCamp;
	BYTE	m_nFigure;
	BYTE	m_bWGType;
	int		m_nWeekOffer;
	char	m_szTongName[32];
	char	m_szTitle[32];
	char	m_szMaster[32];
} STONG_SERVER_TO_CORE_LOGIN;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	UINT	m_uRight;
} TONG_APPLY_RIGHT_COMMAND;
// Íæ¼ÒÉêÇë½¨Á¢°ï»á ÓÃÀ©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btCamp;
	char	m_szName[defTONG_NAME_MAX_LENGTH + 1];
} TONG_APPLY_CREATE_COMMAND;

// Íæ¼ÒÉêÇë¼ÓÈë°ï»á ÓÃÀ©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
} TONG_APPLY_ADD_COMMAND;

// Íæ¼ÒÉêÇë¼ÓÈë°ï»á ÓÃÀ©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	BYTE	m_btNewFigure;
} TONG_APPLY_INSTATE_COMMAND;

// ×ª·¢¼ÓÈë°ï»áÉêÇë À©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nPlayerIdx;
	char	m_szName[32];
} TONG_APPLY_ADD_SYNC;

// Í¨ÖªÍæ¼Ò½¨Á¢°ï»á³É¹¦ ÓÃÆÕÍ¨Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	BYTE	m_btCamp;
	char	m_szName[defTONG_NAME_MAX_LENGTH + 1];
} TONG_CREATE_SYNC;

// Í¨ÖªÍæ¼Ò¼ÓÈë°ï»á ÓÃÀ©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btCamp;
	BYTE	m_btFigure;
	char	m_szTongName[32];
	char	m_szMaster[32];
} TONG_Add_SYNC;

// Íæ¼ÒÉêÇë½âÉ¢°ï»á ÓÃÀ©Õ¹Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
} TONG_APPLY_DISMISS_COMMAND;

// Íæ¼Ò°ï»á±»½âÉ¢ ÓÃÆÕÍ¨Ğ­Òé
typedef struct
{
	BYTE	ProtocolType;
} TONG_DISMISS_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nPlayerIdx;
	UINT	m_dwNameID;
	BYTE	m_btFlag;
} TONG_ACCEPT_MEMBER_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btInfoID;
	UINT	m_nParam1;
	int		m_nParam2;
	int		m_nParam3;
} TONG_APPLY_INFO_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMoney;
	int		m_nBuildMoney;
	int		m_nOffer;
	UINT	m_MemberNum;
	BYTE	m_btExpPercent;
	BYTE	m_nBuilLevel;
	BYTE	m_btCamp;
	BYTE	m_btLevel;
	BYTE	m_bLockRecruit;
	BYTE	m_bSelf;
	char	m_szTongName[defTONG_STR_LENGTH];
	char	m_szMaster[defTONG_STR_LENGTH];
	char	m_szUnion[defTONG_STR_LENGTH];
} TONG_HEAD_INFO_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nPage;
	int		m_nMemNum;
	BYTE	m_bBegin;
	STONG_MEMSUBINFO sMem[defTONG_MAX_MEMINFOSYNC];
} TONG_MEMBER_INFO_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nPage;
	int		m_nMemNum;
	BYTE	m_bBegin;
	STONG_PAGEMEM sMem[defTONG_MAX_PAGEINFOSYNC];
} TONG_PAGE_INFO_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btJoinFlag;
	BYTE	m_btFigure;
	BYTE	m_btCamp;
	int		m_nWeekOffer;
	UINT	m_uRight;
	char	m_szTongName[32];
	char	m_szTitle[32];
	char	m_szMaster[32];
} TONG_SELF_INFO_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btNewFigure;
} TONG_INSTATE_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
} TONG_APPLY_KICK_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	int		m_nNeedMoney;
} TONG_KICK_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
} TONG_APPLY_LEAVE_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
} TONG_APPLY_CHANGE_MASTER_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	UINT	m_uRight;
} TONG_RIGHT_SENDER_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_bLockRecruit;
} TONG_CHANGE_RECRUIT_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
} TONG_ACCEPTMEMBER_FAIL_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMoney;
} TONG_APPLY_CONTRIBMONEY_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMoney;
} TONG_CONTRIBMONEY_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	int		m_nMoney;
} TONG_APPLY_DISPENSEOFFER_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		nMemberPoint;
	int		nManagerPoint;
	int		nDirectorPoint;
} TONG_APPLY_ASSIGNFUND_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMoney;
	BYTE	m_bSuccess;
} TONG_ASSIGNFUND_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMoney;
	int		m_nBuildFund;
} TONG_TRANSMONEY_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	char	m_szAnnounce[128];
} TONG_RECORD_ANNOUNCE_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	char	m_szAnnounce[128];
} TONG_APPLY_ANNOUNCE_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nMsgCount;
	char	m_szMsg[4][128];
} TONG_RECORD_AFFAIRHISTORY_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	TTongWeekGoal m_OldWeekGoal;
	TTongWeekGoal m_NewWeekGoal;
	UINT	m_uWeekCount;
	int		m_DayNum;
	int		m_nOldWGCompleted;
	int		m_nNewWGCompleted;
	BYTE	m_bOrdeal;
	BYTE	m_nTaskLevel;
	BYTE	m_bWGType;
	BYTE	m_bGetPrice;
} TONG_RECORD_WEEKTASK_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	BYTE	m_nDestFigure;
	char	m_szName[32];
} TONG_APPLY_CHANGETITLE_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	UINT	m_uDestID;
	char	m_szName[32];
} TONG_CHANGETITLE_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	BYTE	m_btCamp;
} TONG_APPLY_CHANGECAMP_COMMAND;

typedef struct
{
	BYTE	ProtocolType;
	WORD	m_wLength;
	BYTE	m_btMsgId;
	int		m_nAction;
} TONG_APPLY_ACTION_COMMAND;
// tong end
typedef struct
{
	BYTE	ProtocolType;
	UINT	m_dwNpcID;
	WORD	m_wGoldFlag;
} NPC_GOLD_CHANGE_SYNC;

typedef struct
{
	BYTE	ProtocolType;
	UINT	dwItemID;
	int		nChange;
} ITEM_DURABILITY_CHANGE;

typedef struct
{
	BYTE	ProtocolType;
	UINT	dwItemID;
} ITEM_REPAIR;

enum
{
	c2sdnmbr_updownhorse,
	c2sdnmbr_exchangeitem,
	c2sdnmbr_gmexenpc,
	c2sdnmbr_arrangeitem,
	c2sdnmbr_arrangebox,
	c2sdnmbr_movemapid,
};
enum
{
	s2cdnmbr_msginfo,
	s2cdnmbr_removehorsema,
	s2cdnmbr_itemstack,
	s2cdnmbr_updatepos,
	s2cdnmbr_broadnpctong,
	s2cdnmbr_taskoffer,
};


typedef struct
{
	BYTE			ProtocolType;
	WORD			m_wLength;
	BYTE			nBranch;
} DYNAMIC_COMMAND;

typedef struct
{
	UINT	ID;
	char	szTong[defTONG_NAME_MAX_LENGTH+1];
	char	szTitle[defTONG_NAME_MAX_LENGTH+1];
} PNTONG_SYNC;

// ÔÚµ÷ÓÃÕâÖ§º¯ÊıÖ®Ç°±ØĞëÅĞ¶ÏÊÇ·ñ´¦ÓÚ½»Ò××´Ì¬£¬Èç¹ûÕıÔÚ½»Ò×£¬²»ÄÜµ÷ÓÃÕâÖ§º¯Êı
void SendClientCmdSell(int nID);
// ÔÚµ÷ÓÃÕâÖ§º¯ÊıÖ®Ç°±ØĞëÅĞ¶ÏÊÇ·ñ´¦ÓÚ½»Ò××´Ì¬£¬Èç¹ûÕıÔÚ½»Ò×£¬²»ÄÜµ÷ÓÃÕâÖ§º¯Êı
void SendClientCmdBuy(int nBuyIdx, int nPlace, int nX, int nY);
// ÔÚµ÷ÓÃÕâÖ§º¯ÊıÖ®Ç°±ØĞëÅĞ¶ÏÊÇ·ñ´¦ÓÚ½»Ò××´Ì¬£¬Èç¹ûÕıÔÚ½»Ò×£¬²»ÄÜµ÷ÓÃÕâÖ§º¯Êı
void SendClientCmdRun(int nX, int nY);
// ÔÚµ÷ÓÃÕâÖ§º¯ÊıÖ®Ç°±ØĞëÅĞ¶ÏÊÇ·ñ´¦ÓÚ½»Ò××´Ì¬£¬Èç¹ûÕıÔÚ½»Ò×£¬²»ÄÜµ÷ÓÃÕâÖ§º¯Êı
void SendClientCmdWalk(int nX, int nY);
// ÔÚµ÷ÓÃÕâÖ§º¯ÊıÖ®Ç°±ØĞëÅĞ¶ÏÊÇ·ñ´¦ÓÚ½»Ò××´Ì¬£¬Èç¹ûÕıÔÚ½»Ò×£¬²»ÄÜµ÷ÓÃÕâÖ§º¯Êı
void SendClientCmdSkill(int nSkillID, int nX, int nY);
//void SendClientCmdPing();
void SendClientCmdSit(int nSitFlag);
void SendClientCmdMoveItem(void* pDownPos, void* pUpPos);
void SendClientCmdQueryLadder(UINT	dwLadderID);
void SendClientCmdRequestNpc(int nID);
void SendClientCmdJump(int nX, int nY);
void SendClientCmdStoreMoney(int nDir, int nMoney);
//void SendClientCmdRevive(int nReviveType);
void SendClientCmdRevive();
void SendObjMouseClick(int nObjID, UINT dwRegionID);
void SendClientCmdRepair(UINT dwID);

extern	int	g_nProtocolSize[MAX_PROTOCOL_NUM];
#pragma pack(pop, enter_protocol)
#endif
