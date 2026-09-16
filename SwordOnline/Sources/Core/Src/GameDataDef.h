/*****************************************************************************************
//	Íâ½ç·ÃÎÊCoreÓÃµ½Êı¾İ½á¹¹µÈµÄ¶¨Òå
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-12
------------------------------------------------------------------------------------------
	Ò»Ğ©¶¨Òå¿ÉÄÜ´¦ÓÚÔÚÓÎÏ·ÊÀ½ç¸÷Ä£¿éµÄÍ·ÎÄ¼şÖĞ£¬ÇëÔÚ´Ë´¦°üº¬ÄÇ¸öÍ·ÎÄ¼ş£¬²¢ÇëÄÇÑùµÄÍ·ÎÄ¼ş
²»Òª°üº¬Ò»Ğ©ÓëÓÎÏ·ÊÀ½ç¶ÔÍâÎŞ¹ØµÄÄÚÈİ¡£
    ¿ª·¢¹ı³ÌÖĞÓÎÏ·ÊÀ½çµÄÍâ²¿¿Í»§ÔÚÎ´»ñµÃÓÎÏ·ÊÀ½ç½Ó¿ÚÍêÕû¶¨ÒåµÄÇé¿öÏÂ£¬»áÏÖÏÈÖ±½ÓÔÚ´ËÎÄ¼ş
¶¨ÒåËüĞèÒªµÄÊı¾İ¶¨Òå£¬ÓÎÏ·ÊÀ½ç¸÷Ä£¿é¿É¸ù¾İ×ÔÉíĞèÒªÓëÉè¼Æ°Ñ¶¨Òå×÷ĞŞ¸Ä»òÒÆ¶¯µ½Ä£¿éÄÚµÄ¶ÔÍâ
Í·ÎÄ¼ş£¬²¢ÔÚ´Ë½¨Á¢³äÒªµÄ°üº¬¡£
*****************************************************************************************/
#ifndef GAMEDATADEF_H
#define GAMEDATADEF_H

#include "CoreObjGenreDef.h"
#include "CoreUseNameDef.h"

#define		ITEM_VERSION						1

#define		_CHAT_SCRIPT_OPEN

#define		MAX_TEAM_MEMBER						7		// ×î´ó¶ÓÔ±ÊıÁ¿(²»°üÀ¨¶Ó³¤)
#define		MAX_SENTENCE_LENGTH					256		// ÁÄÌìÃ¿¸öÓï¾ä×î´ó³¤¶È

#define		FILE_NAME_LENGTH					80
#define		PLAYER_PICKUP_CLIENT_DISTANCE		63
#define		defMAX_EXEC_OBJ_SCRIPT_DISTANCE		200
#define		defMAX_PLAYER_SEND_MOVE_FRAME		5
#define		PLAYER_PICKUP_SERVER_DISTANCE		40000
#define		MAX_INT								0x7fffffff

#define		ROLE_NO								2
#define		PLAYER_MALE_NPCTEMPLATEID			-1
#define		PLAYER_FEMALE_NPCTEMPLATEID			-2

#define		PLAYER_SHARE_EXP_DISTANCE			768

#define		MAX_DEATH_PUNISH_PK_VALUE			10		// PK´¦·££¬PKÖµ´Ó 0 µ½ 10

enum ITEM_PART
{
	itempart_head = 0,
	itempart_body,
	itempart_belt,
	itempart_weapon,
	itempart_foot,
	itempart_cuff,
	itempart_amulet,
	itempart_ring1,
	itempart_ring2,
	itempart_pendant,
	itempart_horse,
	itempart_mask,
	itempart_mantle,
	itempart_signet,
	itempart_shipin,
	itempart_num,
};

enum ITEMEXTENDTYPE
{
	extype_normal = 0,		// trang bŞ b×nh th­êng
	extype_gold,			// hoµng kim 1
	extype_platina,			// b¹ch kim 2
	extype_purple,			// ®å tİm 3
	extype_number,			// sè l­îng lo¹i
};

typedef struct
{
	int		nIdx;
	int		nPlace;
	int		nX;
	int		nY;
} PlayerItem;

enum INVENTORY_ROOM
{
	room_equipment = 0,	// ×°±¸À¸
	room_repository,	// ÖüÎïÏä
	room_trade,			// ½»Ò×À¸
	room_tradeback,		// ½»Ò×¹ı³ÌÖĞ×°±¸À¸µÄ±¸·İ
	room_trade1,		// ½»Ò×¹ı³ÌÖĞ¶Ô·½µÄ½»Ò×À¸
	room_immediacy,		// ¿ì½İÎïÆ·
	room_num,			// ¿Õ¼äÊıÁ¿
};

enum ITEM_POSITION
{
	pos_hand = 1,		// ÊÖÉÏ
	pos_equip,			// ×°±¸×ÅµÄ
	pos_equiproom,		// µÀ¾ßÀ¸
	pos_repositoryroom,	// ÖüÎïÏä
	pos_traderoom,		// ½»Ò×À¸
	pos_trade1,			// ½»Ò×¹ı³ÌÖĞ¶Ô·½µÄ½»Ò×À¸
	pos_immediacy,		// ¿ì½İÎïÆ·
};

#define		MAX_HAND_ITEM				1
#define		EQUIPMENT_ROOM_WIDTH		6
#define		EQUIPMENT_ROOM_HEIGHT		10
#define		MAX_EQUIPMENT_ITEM			(EQUIPMENT_ROOM_WIDTH * EQUIPMENT_ROOM_HEIGHT)
#define		REPOSITORY_ROOM_WIDTH		6
#define		REPOSITORY_ROOM_HEIGHT		10
#define		MAX_REPOSITORY_ITEM			(REPOSITORY_ROOM_WIDTH * REPOSITORY_ROOM_HEIGHT)
#define		TRADE_ROOM_WIDTH			10
#define		TRADE_ROOM_HEIGHT			4
#define		MAX_TRADE_ITEM				(TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT)
#define		MAX_TRADE1_ITEM				MAX_TRADE_ITEM
#define		IMMEDIACY_ROOM_WIDTH		3
#define		IMMEDIACY_ROOM_HEIGHT		1
#define		MAX_IMMEDIACY_ITEM			(IMMEDIACY_ROOM_WIDTH * IMMEDIACY_ROOM_HEIGHT)
#define		MAX_PLAYER_ITEM_RESERVED	32
#define		MAX_PLAYER_ITEM				(MAX_EQUIPMENT_ITEM + MAX_REPOSITORY_ITEM + MAX_TRADE_ITEM + MAX_TRADE1_ITEM + MAX_IMMEDIACY_ITEM + itempart_num + MAX_HAND_ITEM + MAX_PLAYER_ITEM_RESERVED)


#define		REMOTE_REVIVE_TYPE			0
#define		LOCAL_REVIVE_TYPE			1

#define		MAX_MELEE_WEAPON			9
#define		MAX_RANGE_WEAPON			4
#define		MAX_ARMOR					14
#define		MAX_HELM					14
#define		MAX_RING					1
#define		MAX_BELT					2
#define		MAX_PENDANT					2
#define		MAX_AMULET					2
#define		MAX_CUFF					2
#define		MAX_BOOT					4
#define		MAX_HORSE					6

#define		MAX_NPC_TYPE	300
#define		MAX_NPC_LEVEL	101


#define		MAX_NPC_DIR		64
#define		MAX_WEAPON		MAX_MELEE_WEAPON + MAX_RANGE_WEAPON
#define		MAX_SKILL_STATE 10
#define		MAX_NPC_HEIGHT	128
#define		MAX_RESIST		95
#define		MAX_HIT_PERCENT	95
#define		MIN_HIT_PERCENT	5
#define		MAX_NPC_RECORDER_STATE 8 //×î´óNpc¿ÉÒÔ¼ÇÂ¼ÏÂÀ´µÄ×´Ì¬ÊıÁ¿;

#define		PLAYER_MOVE_DO_NOT_MANAGE_DISTANCE	5

#define	NORMAL_NPC_PART_NO		5		// ÆÕÍ¨npcÍ¼ÏñÄ¬ÈÏÎªÖ»ÓĞÒ»¸ö²¿¼ş£¬ÕâÊÇµÚ¼¸¸ö

#ifndef _SERVER
#define		C_REGION_X(x)	(LOWORD(SubWorld[0].m_Region[ (x) ].m_RegionID))
#define		C_REGION_Y(y)	(HIWORD(SubWorld[0].m_Region[ (y) ].m_RegionID))
#endif

enum
{
	CHAT_S_STOP = 0,						// ·ÇÁÄÌì×´Ì¬
	CHAT_S_SCREEN,							// ÓëÍ¬ÆÁÄ»Íæ¼ÒÁÄÌì
	CHAT_S_SINGLE,							// ÓëÍ¬·şÎñÆ÷Ä³Íæ¼ÒË½ÁÄ
	CHAT_S_TEAM,							// Óë¶ÓÎéÈ«Ìå³ÉÔ±½»Ì¸
	CHAT_S_NUM,								// ÁÄÌì×´Ì¬ÖĞÀàÊı
};

enum PLAYER_INSTANT_STATE
{
	enumINSTANT_STATE_LEVELUP = 0,
	enumINSTANT_STATE_REVIVE,
	enumINSTANT_STATE_CREATE_TEAM,
	enumINSTANT_STATE_LOGIN,
	enumINSTANT_STATE_NUM,
};

enum CHAT_STATUS
{
	CHAT_S_ONLINE = 0,		//ÔÚÏß
	CHAT_S_BUSY,			//Ã¦Âµ
	CHAT_S_HIDE,			//ÒşÉí
	CHAT_S_LEAVE,			//Àë¿ª
	CHAT_S_DISCONNECT,		//µôÏß
};

// ×¢Òâ£º´ËÃ¶¾Ù²»ÔÊĞí¸ü¸Ä(by zroc)
enum OBJ_ATTRIBYTE_TYPE
{
	series_metal = 0,		//	kim
	series_wood,			//	méc
	series_water,			//	thñy
	series_fire,			//	háa
	series_earth,			//	thæ
	series_num,				//	5
};

enum OBJ_GENDER
{
	OBJ_G_MALE	= 0,	//ĞÛĞÔ£¬ÄĞµÄ
	OBJ_G_FEMALE,		//´ÆµÄ£¬Å®µÄ
};

enum NPCCAMP
{
	camp_begin,				// ĞÂÊÖÕóÓª£¨¼ÓÈëÃÅÅÉÇ°µÄÍæ¼Ò£©
	camp_justice,			// ÕıÅÉÕóÓª
	camp_evil,				// Ğ°ÅÉÕóÓª
	camp_balance,			// ÖĞÁ¢ÕóÓª
	camp_free,				// É±ÊÖÕóÓª£¨³öÊ¦ºóµÄÍæ¼Ò£©
	camp_animal,			// Ò°ÊŞÕóÓª
	camp_event,				// Â·ÈËÕóÓª
	camp_num,				// ÕóÓªÊı
};

enum ITEM_IN_ENVIRO_PROP
{
	IIEP_NORMAL = 0,	//Ò»°ã/Õı³£/¿ÉÓÃ
	IIEP_NOT_USEABLE,	//²»¿ÉÓÃ/²»¿É×°Åä
	IIEP_SPECIAL,		//ÌØ¶¨µÄ²»Í¬Çé¿ö
};

#define	GOD_MAX_OBJ_TITLE_LEN	1536	//128ÁÙÊ±¸ÄÎª1024ÎªÁË¼æÈİ¾É´úÂë to be modified
#define	GOD_MAX_OBJ_PROP_LEN	516
#define	GOD_MAX_OBJ_DESC_LEN	516

//==================================
//	ÓÎÏ·¶ÔÏóµÄÃèÊö
//==================================
struct KGameObjDesc
{
	char	szTitle[GOD_MAX_OBJ_TITLE_LEN];	//±êÌâ£¬Ãû³Æ
	//char	szProp[GOD_MAX_OBJ_PROP_LEN];	//ÊôĞÔ£¬Ã¿ĞĞ¿ÉÒÔtab»®·ÖÎª¿¿×óÓë¿¿ÓÒ¶ÔÆëÁ½²¿·Ö
	//char	szDesc[GOD_MAX_OBJ_DESC_LEN];	//ÃèÊö
};

//==================================
//	ÎÊÌâÓë¿ÉÑ¡´ğ°¸
//==================================
struct KUiAnswer
{
	char	AnswerText[256];	//¿ÉÑ¡´ğ°¸ÎÄ×Ö£¨¿ÉÒÔ°üº¬¿ØÖÆ·û£©
	int		AnswerLen;			//¿ÉÑ¡´ğ°¸´æ´¢³¤¶È£¨°üÀ¨¿ØÖÆ·û£¬²»°üº¬½áÊø·û£©
};

struct KUiQuestionAndAnswer
{
	char		Question[512];	//ÎÊÌâÎÄ×Ö£¨¿ÉÒÔ°üº¬¿ØÖÆ·û£©
	int			QuestionLen;	//ÎÊÌâÎÄ×Ö´æ´¢³¤¶È£¨°üÀ¨¿ØÖÆ·û£¬²»°üº¬½áÊø·û£©
	int			AnswerCount;	//¿ÉÑ¡´ğ°¸µÄÊıÄ¿
	KUiAnswer	Answer[1];		//ºòÑ¡´ğ°¸
};

//==================================
//	¼òÂÔ±íÊ¾ÓÎÏ·¶ÔÏóµÄ½á¹¹
//==================================
struct KUiGameObject
{
	unsigned int uGenre;	//¶ÔÏóÀàÊô
	unsigned int uId;		//¶ÔÏóid
//	int			 nData;		//Óë¶ÔÏóÊµÀıÏà¹ØµÄÄ³Êı¾İ
};

//==================================
//	ÒÔ×ø±ê±íÊ¾µÄÒ»¸öÇøÓò·¶Î§
//==================================
struct KUiRegion
{
	int		h;		//×óÉÏ½ÇÆğµãºá×ø±ê
	int		v;		//×óÉÏ½ÇÆğµã×İ×ø±ê
	int		Width;	//ÇøÓòºá¿í
	int		Height;	//ÇøÓò×İ¿í
};

//==================================
//	¿ÉÒÔÓÎÏ·¶ÔÏóÈİÄÉµÄµØ·½
//==================================
enum UIOBJECT_CONTAINER
{
	UOC_IN_HAND	= 1,		//ÊÖÖĞÄÃ×Å
	UOC_GAMESPACE,			//ÓÎÏ·´°¿Ú
	UOC_IMMEDIA_ITEM,		//¿ì½İÎïÆ·
	UOC_IMMEDIA_SKILL,		//¿ì½İÎä¹¦0->ÓÒ¼üÎä¹¦£¬1,2...-> F1,F2...¿ì½İÎä¹¦
	UOC_ITEM_TAKE_WITH,		//ËæÉíĞ¯´ø
	UOC_TO_BE_TRADE,		//Òª±»ÂòÂô£¬ÂòÂôÃæ°åÉÏ
	UOC_OTHER_TO_BE_TRADE,	//ÂòÂôÃæ°åÉÏ£¬±ğÈËÒªÂô¸ø×Ô¼ºµÄ£¬
	UOC_EQUIPTMENT,			//ÉíÉÏ×°±¸
	UOC_NPC_SHOP,			//npcÂòÂô³¡Ëù
	UOC_STORE_BOX,			//´¢ÎïÏä
	UOC_SKILL_LIST,			//ÁĞ³öÈ«²¿ÓµÓĞ¼¼ÄÜµÄ´°¿Ú£¬¼¼ÄÜ´°¿Ú
	UOC_SKILL_TREE,			//×ó¡¢ÓÒ¿ÉÓÃ¼¼ÄÜÊ÷
};

//==================================
// iCoreShell::GetGameDataº¯Êıµ÷ÓÃ,uDataIdÈ¡ÖµÎªGDI_TRADE_DATAÊ±£¬
// uParamµÄĞí¿ÉÈ¡ÖµÁĞ±í
// ×¢ÊÍÖĞµÄReturn:ĞĞ±íÊ¾Ïà¹ØµÄGetGameDataµ÷ÓÃµÄ·µ»ØÖµµÄº¬Òå
//==================================
enum UI_TRADE_OPER_DATA
{
	UTOD_IS_WILLING,		//ÊÇ·ñ½»Ò×ÒâÏò(½ĞÂôÖĞ)
	//Return: ·µ»Ø×Ô¼ºÊÇ·ñ´¦ÓÚ½ĞÂôÖĞµÄ²¼¶ûÖµ
	UTOD_IS_LOCKED,			//×Ô¼ºÊÇ·ñ´¦ÓÚÒÑËø¶¨×´Ì¬
	//Return: ·µ»Ø×Ô¼ºÊÇ·ñ´¦ÓÚÒÑËø¶¨×´Ì¬µÄ²¼¶ûÖµ
	UTOD_IS_TRADING,		//ÊÇ·ñ¿ÉÒÔÕıÔÚµÈ´ı½»Ò×²Ù×÷£¨½»Ò×ÊÇ·ñÒÑÈ·¶¨£©
	//Return: ·µ»ØÊÇ·ñÕıÔÚµÈ´ı½»Ò×²Ù×÷£¨½»Ò×ÊÇ·ñÒÑÈ·¶¨£©
	UTOD_IS_OTHER_LOCKED,	//¶Ô·½ÊÇ·ñÒÑ¾­´¦ÓÚËø¶¨×´Ì¬
	//Return: ·µ»Ø¶Ô·½ÊÇ·ñÒÑ¾­´¦ÓÚËø¶¨×´Ì¬µÄ²¼¶ûÖµ
};

//==================================
//	ÂòÂôÎïÆ·
//==================================
struct KUiItemBuySelInfo
{
	char			szItemName[64];	//ÎïÆ·Ãû³Æ
	int				nPrice;			//ÂòÂô¼ÛÇ®£¬ÕıÖµÎªÂô¼Û¸ñ£¬¸ºÖµ±íÊ¾ÂòÈëµÄ¼Û¸ñÎª(-nPrice)
};

//==================================
//	±íÊ¾Ä³¸öÓÎÏ·¶ÔÏóÔÚ×ø±êÇøÓò·¶Î§µÄĞÅÏ¢½á¹¹
//==================================
struct KUiObjAtRegion
{
	KUiGameObject	Obj;
	KUiRegion		Region;
};

struct KUiObjAtContRegion : public KUiObjAtRegion
{
	union
	{
		UIOBJECT_CONTAINER	eContainer; 
		int					nContainer;
	};
};

struct KUiMsgParam
{
	unsigned char	eGenre;	//È¡Öµ·¶Î§ÎªÃ¶¾ÙÀàĞÍMSG_GENRE_LIST,¼ûMsgGenreDef.hÎÄ¼ş
	unsigned char	cChatPrefixLen;
	unsigned short	nMsgLength;
	char			szName[32];
#define	CHAT_MSG_PREFIX_MAX_LEN	16
	unsigned char	cChatPrefix[CHAT_MSG_PREFIX_MAX_LEN];
};

struct KUiInformationParam
{
	char	sInformation[256];	//ÏûÏ¢ÎÄ×ÖÄÚÈİ
	char	sConfirmText[64];	//È·ÈÏÏûÏ¢(°´Å¥)µÄ±êÌâÎÄ×Ö
	short	nInforLen;			//ÏûÏ¢ÎÄ×ÖÄÚÈİµÄ´æ´¢³¤¶È
	bool	bNeedConfirmNotify;	//ÊÇ·ñÒª·¢»ØÈ·ÈÏÏûÏ¢(¸øcore)
	bool	bReserved;			//±£Áô£¬Öµ¹Ì¶¨Îª0
};

enum PLAYER_ACTION_LIST
{
	PA_NONE = 0,	//ÎŞ¶¯×÷
	PA_RUN  = 0x01,	//ÅÜ
	PA_SIT  = 0x02,	//´ò×ø
	PA_RIDE = 0x04,	//Æï£¨Âí£©
};

//==================================
//	ÏµÍ³ÏûÏ¢·ÖÀà
//==================================
enum SYS_MESSAGE_TYPE
{
	SMT_NORMAL = 0,	//²»²Î¼Ó·ÖÀàµÄÏûÏ¢
	SMT_SYSTEM,		//ÏµÍ³£¬Á¬½ÓÏà¹Ø
	SMT_PLAYER,		//Íæ¼ÒÏà¹Ø
	SMT_TEAM,		//×é¶ÓÏà¹Ø
	SMT_FRIEND,		//ÁÄÌìºÃÓÑÏà¹Ø
	SMT_MISSION,	//ÈÎÎñÏà¹Ø
	SMT_CLIQUE,		//°ïÅÉÏà¹Ø
};

//==================================
//	ÏµÍ³ÏûÏ¢ÏìÓ¦·½Ê½
//==================================
enum SYS_MESSAGE_CONFIRM_TYPE
{
	SMCT_NONE,				//ÔÚ¶Ô»°ÏûÏ¢´°¿ÚÖ±½ÓÂÓ¹ı£¬²»ĞèÒªÏìÓ¦¡£
	SMCT_CLICK,				//µã»÷Í¼±êºóÁ¢¼´É¾³ı¡£
	SMCT_MSG_BOX,			//µã»÷Í¼±êºóµ¯³öÏûÏ¢¿ò¡£
	SMCT_UI_RENASCENCE,		//Ñ¡ÔñÖØÉú
	SMCT_UI_ATTRIBUTE,		//´ò¿ªÊôĞÔÒ³Ãæ
	SMCT_UI_SKILLS,			//´ò¿ª¼¼ÄÜÒ³Ãæ
	SMCT_UI_ATTRIBUTE_SKILLS,//´ò¿ªÊôĞÔÒ³Ãæ¼¼ÄÜÒ³Ãæ
	SMCT_UI_TEAM_INVITE,	//´ğÓ¦»ò¾Ü¾ø¼ÓÈë¶ÓÎéµÄÑûÇë,
	//						pParamBuf Ö¸ÏòÒ»¸öKUiPlayerItem½á¹¹µÄÊı¾İ£¬±íÊ¾ÑûÇéÈË(¶Ó³¤)
	SMCT_UI_TEAM_APPLY,		//´ğÓ¦»ò¾Ü¾ø¼ÓÈë¶ÓÎéµÄÉêÇë,
	//						pParamBuf Ö¸ÏòÒ»¸öKUiPlayerItem½á¹¹µÄÊı¾İ£¬±íÊ¾ÉêÇëÈË
	SMCT_UI_TEAM,			//´ò¿ª¶ÓÎé¹ÜÀíÃæ°å
	SMCT_UI_INTERVIEW,		//´ò¿ªÁÄÌì¶Ô»°½çÃæ,
	//						pParamBuf Ö¸ÏòÒ»¸öKUiPlayerItem½á¹¹µÄÊı¾İ£¬±íÊ¾·¢À´ÏûÏ¢µÄºÃÓÑ
	SMCT_UI_FRIEND_INVITE,	//Åú×¼»ò¾Ü¾ø±ğÈË¼Ó×Ô¼ºÎªºÃÓÑ
	//						pParamBuf Ö¸ÏòÒ»¸öKUiPlayerItem½á¹¹µÄÊı¾İ£¬±íÊ¾·¢³öºÃÓÑÑûÇëµÄÈË
	SMCT_UI_TRADE,			//´ğÓ¦»ò¾Ü¾ø½»Ò×µÄÇëÇó,
	//						pParamBuf Ö¸ÏòÒ»¸öKUiPlayerItem½á¹¹µÄÊı¾İ£¬±íÊ¾·¢³ö½»Ò×ÑûÇëµÄÈË
	SMCT_DISCONNECT,		//¶ÏÏß
	SMCT_UI_TONG_JOIN_APPLY,//´ğÓ¦»ò¾Ü¾ø¼ÓÈë°ï»áµÄÉêÇë
};

//==================================
//	ÏµÍ³ÏûÏ¢
//==================================
struct KSystemMessage
{
	char			szMessage[128];	//thong tin guve
	unsigned int	uReservedForUi;	//½çÃæÊ¹ÓÃµÄÊı¾İÓò,coreÀïÌî0¼´¿É
	unsigned char	eType;			//ÏûÏ¢·ÖÀàÈ¡ÖµÀ´×ÔÃ¶¾ÙÀàĞÍ SYS_MESSAGE_TYPE
	unsigned char	byConfirmType;	//ÏìÓ¦ÀàĞÍ
	unsigned char	byPriority;		//ÓÅÏÈ¼¶,ÊıÖµÔ½´ó£¬±íÊ¾ÓÅÏÈ¼¶Ô½¸ß
	unsigned char	byParamSize;	//°éËæGDCNI_SYSTEM_MESSAGEÏûÏ¢µÄpParamBufËùÖ¸²ÎÊı»º³åÇø¿Õ¼äµÄ´óĞ¡¡£
};

//==================================
//	ÁÄÌìÆµµÀµÄÃèÊö
//==================================
struct KUiChatChannel
{
	int			 nChannelNo;
	unsigned int uChannelId;
	union
	{
		int		 nChannelIndex;
		int		 nIsSubscibed;	//ÊÇ·ñ±»¶©ÔÄ
	};
	char		 cTitle[32];
};

//==================================
//	ÁÄÌìºÃÓÑµÄÒ»¸ö·Ö×éµÄĞÅÏ¢
//==================================
struct KUiChatGroupInfo
{
	char	szTitle[32];	//·Ö×éµÄÃû³Æ
	int		nNumFriend;		//×éÄÚºÃÓÑµÄÊıÄ¿
};

//==================================
//	ºÃÓÑ·¢À´µÄÁÄÌì»°Óï
//==================================
struct KUiChatMessage
{
	unsigned int uColor;
	short	nContentLen;
	char	szContent[256];
};

//==================================
//	Ö÷½ÇµÄÒ»Ğ©²»Ò×±äµÄÊı¾İ
//==================================
struct KUiPlayerBaseInfo
{
	char	Agname[32];	//´ÂºÅ
	char	Name[32];	//Ãû×Ö
	char	Title[32];	//³ÆºÅ
	int		nCurFaction;// µ±Ç°¼ÓÈëÃÅÅÉ id £¬Èç¹ûÎª -1 £¬µ±Ç°Ã»ÓĞÔÚÃÅÅÉÖĞ
	int		nRankInWorld;//½­ºşÅÅÃûÖµ,ÖµÎª0±íÊ¾Î´ÉÏÅÅÃû°å
	unsigned int nCurTong;// µ±Ç°¼ÓÈë°ïÅÉname id £¬Èç¹ûÎª 0 £¬µ±Ç°Ã»ÓĞÔÚ°ïÅÉÖĞ
};

//==================================
//	Ö÷½ÇµÄÒ»Ğ©Ò×±äµÄÊı¾İ
//==================================
struct KUiPlayerRuntimeInfo
{
	int		nLifeFull;			//ÉúÃüÂúÖµ
	int		nLife;				//ÉúÃü
	int		nManaFull;			//ÄÚÁ¦ÂúÖµ
	int		nMana;				//ÄÚÁ¦
	int		nStaminaFull;		//ÌåÁ¦ÂúÖµ
	int		nStamina;			//ÌåÁ¦
	int		nAngryFull;			//Å­ÂúÖµ
	int		nAngry;				//Å­
	int		nExperienceFull;	//¾­ÑéÂúÖµ
	int		nExperience;		//µ±Ç°¾­ÑéÖµ
	int		nCurLevelExperience;//µ±Ç°¼¶±ğÉı¼¶ĞèÒªµÄ¾­ÑéÖµ

	unsigned char	byActionDisable;//ÊÇ·ñ²»¿É½øĞĞ¸÷ÖÖ¶¯×÷£¬ÎªÃ¶¾ÙPLAYER_ACTION_LISTÈ¡ÖµµÄ×éºÏ
	unsigned char	byAction;	//ÕıÔÚ½øĞĞµÄĞĞÎª¶¯×÷£¬ÎªÃ¶¾ÙPLAYER_ACTION_LISTÈ¡ÖµµÄ×éºÏ
	unsigned short	wReserved;	//±£Áô
};

//==================================
//	Ö÷½ÇµÄÒ»Ğ©ÊôĞÔÊı¾İË÷Òı
//==================================
enum UI_PLAYER_ATTRIBUTE
{
	UIPA_STRENGTH = 0,			//Á¦Á¿
	UIPA_DEXTERITY,				//Ãô½İ
	UIPA_VITALITY,				//»îÁ¦
	UIPA_ENERGY,				//¾«Á¦
};

//==================================
//	Ö÷½ÇµÄÒ»Ğ©Ò×±äµÄÊôĞÔÊı¾İ
//==================================
struct KUiPlayerAttribute
{
	int		nMoney;				//ÒøÁ½
	int		nLevel;				//µÈ¼¶
	char	StatusDesc[16];		//×´Ì¬ÃèÊö

	int		nBARemainPoint;		//»ù±¾ÊôĞÔÊ£ÓàµãÊı
	int		nStrength;			//Á¦Á¿
	int		nDexterity;			//Ãô½İ
	int		nVitality;			//»îÁ¦
	int		nEnergy;			//¾«Á¦

	int		nKillMAX;			//×î´óÉ±ÉËÁ¦
	int		nKillMIN;			//×îĞ¡É±ÉËÁ¦
	int		nRightKillMax;		//ÓÒ¼ü×î´óÉ±ÉËÁ¦
	int		nRightKillMin;		//ÓÒ¼ü×îĞ¡É±ÉËÁ¦

	int		nAttack;			//¹¥»÷Á¦
	int		nDefence;			//·ÀÓùÁ¦
	int		nMoveSpeed;			//ÒÆ¶¯ËÙ¶È
	int		nAttackSpeed;		//¹¥»÷ËÙ¶È

	int		nPhyDef;			//ÎïÀí·ÀÓù
	int		nCoolDef;			//±ù¶³·ÀÓù
	int		nLightDef;			//ÉÁµç·ÀÓù
	int		nFireDef;			//»ğÑæ·ÀÓù
	int		nPoisonDef;			//¶¾ËØ·ÀÓù
};

//==================================
//	Ö÷½ÇµÄÁ¢¼´Ê¹ÓÃÎïÆ·ÓëÎä¹¦
//==================================
struct KUiPlayerImmedItemSkill
{
	KUiGameObject	ImmediaItem[3];
	KUiGameObject	IMmediaSkill[2];
};

//==================================
//	Ö÷½Ç×°±¸°²»»µÄÎ»ÖÃ
//==================================
enum UI_EQUIPMENT_POSITION
{
	UIEP_HEAD = 0,		 //nãn
	UIEP_HAND = 1,		 //vò khİ
	UIEP_NECK = 2,		 //d©y chuyÒn
	UIEP_FINESSE = 3,	 //bao tay
	UIEP_BODY = 4,		 //¸o
	UIEP_WAIST = 5,		 //®ai l­ng
	UIEP_FINGER1 = 6,	 //nhÉn
	UIEP_FINGER2 = 7,	 //nhÉn
	UIEP_WAIST_DECOR = 8,//ngäc béi
	UIEP_FOOT = 9,		 //giµy
	UIEP_HORSE = 10,	 //ngùa
	UIEP_MASK = 11,		//mÆt n¹
	UIEP_MANTLE = 12,	//phi phong
	UIEP_SIGNET = 13,	//Ên
	UIEP_SHIPIN = 14,	//trang søc
};

//==================================
//	Ö÷½ÇµÄÉú»î¼¼ÄÜÊı¾İ
//==================================
struct KUiPlayerLiveSkillBase
{
	int		nRemainPoint;			//Ê£Óà¼¼ÄÜµãÊı
	int		nLiveExperience;		//µ±Ç°¼¼ÄÜ¾­ÑéÖµ
	int		nLiveExperienceFull;	//Éıµ½ÏÂ¼¶ĞèÒªµÄ¾­ÑéÖµ
};

//==================================
//	µ¥Ïî¼¼ÄÜÊı¾İ
//==================================
struct KUiSkillData : public KUiGameObject
{
	union
	{
		int		nLevel;
		int		nData;
	};
};

//==================================
//	Ò»¸ö¶ÓÎéÖĞ×î¶à°üº¬³ÉÔ±µÄÊıÄ¿
//==================================
#define	PLAYER_TEAM_MAX_MEMBER	8

//==================================
//	Í³Ë§ÄÜÁ¦Ïà¹ØµÄÊı¾İ
//==================================
struct KUiPlayerLeaderShip
{
	int		nLeaderShipLevel;			//Í³Ë§Á¦µÈ¼¶
	int		nLeaderShipExperience;		//Í³Ë§Á¦¾­ÑéÖµ
	int		nLeaderShipExperienceFull;	//Éıµ½ÏÂ¼¶ĞèÒªµÄ¾­ÑéÖµ
};

//==================================
//	Ò»¸öÍæ¼Ò½ÇÉ«Ïî
//==================================
struct KUiPlayerItem
{
	char			Name[32];	//Íæ¼Ò½ÇÉ«ĞÕÃû
	unsigned int	uId;		//Íæ¼Ò½ÇÉ«id
	int				nIndex;		//Íæ¼Ò½ÇÉ«Ë÷Òı
	int				nData;		//´ËÍæ¼ÒÏà¹ØµÄÒ»ÏîÊıÖµ£¬º¬ÒåÓë¾ßÌåµÄÓ¦ÓÃÎ»ÖÃÓĞ¹Ø
};

//==================================
//	×é¶ÓĞÅÏ¢µÄÃèÊö
//==================================
struct KUiTeamItem
{
	KUiPlayerItem	Leader;
};

//==================================
//	¶ÓÎéĞÅÏ¢
//==================================
struct KUiPlayerTeam
{
	bool			bTeamLeader;			//Íæ¼Ò×Ô¼ºÊÇ·ñ¶Ó³¤
	char			cNumMember;				//¶ÓÔ±ÊıÄ¿
	char			cNumTojoin;				//Óû¼ÓÈëµÄÈËÔ±µÄÊıÄ¿
	bool			bOpened;				//¶ÓÎéÊÇ·ñÔÊĞíÆäËûÈË¼ÓÈë
	int				nTeamServerID;			//¶ÓÎéÔÚ·şÎñÆ÷ÉÏµÄid£¬ÓÃÓÚ±êÊ¶¸Ã¶ÓÎé£¬-1 Îª¿Õ
	int				nCaptainPower;
};

//==================================
//	Ä§·¨ÊôĞÔ
//==================================
#ifndef MAGICATTRIB
#define MAGICATTRIB
struct KMagicAttrib
{
	int				nAttribType;					//ÊôĞÔÀàĞÍ
	int				nValue[3];						//ÊôĞÔ²ÎÊı
	KMagicAttrib(){nValue[0] = nValue[1] = nValue[2] = nAttribType = 0;};
};
#else
struct KMagicAttrib;
#endif

/* ÕâÊÇ¾ÉµÄ´úÂë£¬ĞÂµÄÒÑ¾­·ÅÔÚKNpcGoldÀïÃæ´¦ÀíÁË
//==================================
//	NPC¼ÓÇ¿
//==================================
struct KNpcEnchant
{
	int		nExp;					// ¾­Ñé
	int		nLife;					// ÉúÃü
	int		nLifeReplenish;			// »ØÑª
	int		nAttackRating;			// ÃüÖĞ
	int		nDefense;				// ·ÀÓù
	int		nMinDamage;
	int		nMaxDamage;

	int		TreasureNumber;				// ×°±¸
	int		AuraSkill;					// ¹â»·
	int		DamageEnhance;				// ÉËº¦
	int		SpeedEnhance;				// ËÙ¶È
	int		SelfResist;					// ×ÔÉí¿¹ĞÔ
	int		ConquerResist;				// ÏàÉú¿¹ĞÔ
#ifndef _SERVER
	char	NameModify[32];				// ¸ÄÃû
#endif
};

//==================================
//	NPCµ¥Ïî¼ÓÇ¿
//==================================
struct KNpcSpeicalEnchant
{
	int		ValueModify;
	char	NameModify[16];
};
*/

struct KMapPos
{
	int		nSubWorld;
	int		nRegion;
	int		nMapX;
	int		nMapY;
	int		nOffX;
	int		nOffY;
};

//==================================
//	Ñ¡ÏîÉèÖÃÏî
//==================================
enum OPTIONS_LIST
{
	OPTION_PERSPECTIVE,		//Í¸ÊÓÄ£Ê½  nParam = (int)(bool)bEnable ÊÇ·ñ¿ªÆô
	OPTION_DYNALIGHT,		//¶¯Ì¬¹âÓ°	nParam = (int)(bool)bEnable ÊÇ·ñ¿ªÆô
	OPTION_MUSIC_VALUE,		//ÒôÀÖÒôÁ¿	nParam = ÒôÁ¿´óĞ¡£¨È¡ÖµÎª0µ½-10000£©
	OPTION_SOUND_VALUE,		//ÒôĞ§ÒôÁ¿	nParam = ÒôÁ¿´óĞ¡£¨È¡ÖµÎª0µ½-10000£©
	OPTION_BRIGHTNESS,		//ÁÁ¶Èµ÷½Ú	nParam = ÁÁ¶È´óĞ¡£¨È¡ÖµÎª0µ½-100£©
	OPTION_WEATHER,			//ÌìÆøĞ§¹û¿ª¹Ø nParam = (int)(bool)bEnable ÊÇ·ñ¿ªÆô
};

//==================================
//	Ëù´¦µÄµØÓòÊ±¼ä»·¾³ĞÅÏ¢
//==================================
struct KUiSceneTimeInfo
{
	char	szSceneName[32];		//³¡¾°Ãû
	int		nSceneId;				//³¡¾°id
	int		nScenePos0;				//³¡¾°µ±Ç°×ø±ê£¨¶«£©
	int		nScenePos1;				//³¡¾°µ±Ç°×ø±ê£¨ÄÏ£©
	int		nGameSpaceTime;			//ÒÔ·ÖÖÓÎªµ¥Î»
};

//==================================
//	¹âÔ´ĞÅÏ¢
//==================================
//ÕûÊı±íÊ¾µÄÈıÎ¬µã×ø±ê
struct KPosition3
{
	int nX;
	int nY;
	int nZ;
};

struct KLightInfo
{
	KPosition3 oPosition;			// ¹âÔ´Î»ÖÃ
	DWORD dwColor;					// ¹âÔ´ÑÕÉ«¼°ÁÁ¶È
	long  nRadius;					// ×÷ÓÃ°ë¾¶
};


//Ğ¡µØÍ¼µÄÏÔÊ¾ÄÚÈİÏî
enum SCENE_PLACE_MAP_ELEM
{ 
	SCENE_PLACE_MAP_ELEM_NONE		= 0x00,		//ÎŞ¶«Î÷
	SCENE_PLACE_MAP_ELEM_PIC		= 0x01,		//ÏÔÊ¾ËõÂÔÍ¼
	SCENE_PLACE_MAP_ELEM_CHARACTER	= 0x02,		//ÏÔÊ¾ÈËÎï
	SCENE_PLACE_MAP_ELEM_PARTNER	= 0x04,		//ÏÔÊ¾Í¬¶ÓÎéÈË
};

//³¡¾°µÄµØÍ¼ĞÅÏ¢
struct KSceneMapInfo
{
	int	nScallH;		//ÕæÊµ³¡¾°Ïà¶ÔÓÚµØÍ¼µÄºáÏò·Å´ó±ÈÀı
	int nScallV;		//ÕæÊµ³¡¾°Ïà¶ÔÓÚµØÍ¼µÄ×İÏò·Å´ó±ÈÀı
	int	nFocusMinH;
	int nFocusMinV;
	int nFocusMaxH;
	int nFocusMaxV;
	int nOrigFocusH;
	int nOrigFocusV;
	int nFocusOffsetH;
	int nFocusOffsetV;
};

enum NPC_RELATION
{
	relation_none	= 1,
	relation_self	= 2,
	relation_ally	= 4,
	relation_enemy	= 8,
	relation_dialog	= 16,
	relation_all	= relation_none | relation_ally | relation_enemy | relation_self | relation_dialog,	
	relation_num,
};

enum NPCKIND
{
	kind_normal = 0	,		
	kind_player,
	kind_partner,
	kind_dialoger,	//¶Ô»°Õß
	kind_bird,
	kind_mouse,
	/*kind_melee	= 0x0004,
	kind_range	= 0x0008,
	kind_escape	= 0x0010,
	kind_bird	= 0x0020,
	*/
    kind_num
};

enum	// Îï¼şÀàĞÍ
{
	Obj_Kind_MapObj = 0,		// µØÍ¼Îï¼ş£¬Ö÷ÒªÓÃÓÚµØÍ¼¶¯»­
	Obj_Kind_Body,				// npc µÄÊ¬Ìå
	Obj_Kind_Box,				// ±¦Ïä
	Obj_Kind_Item,				// µôÔÚµØÉÏµÄ×°±¸
	Obj_Kind_Money,				// µôÔÚµØÉÏµÄÇ®
	Obj_Kind_LoopSound,			// Ñ­»·ÒôĞ§
	Obj_Kind_RandSound,			// Ëæ»úÒôĞ§
	Obj_Kind_Light,				// ¹âÔ´£¨3DÄ£Ê½ÖĞ·¢¹âµÄ¶«Î÷£©
	Obj_Kind_Door,				// ÃÅÀà
	Obj_Kind_Trap,				// ÏİÚå
	Obj_Kind_Prop,				// Ğ¡µÀ¾ß£¬¿ÉÖØÉú
	Obj_Kind_Num,				// Îï¼şµÄÖÖÀàÊı
};

//Ö÷½ÇÉí·İµØÎ»µÈÒ»Ğ©¹Ø¼üÊôĞÔÏî
enum PLAYER_BRIEF_PROP
{
	PBP_LEVEL = 1,	//µÇ¼¶±ä»¯	nParam±íÊ¾µ±Ç°µÈ¼¶
	PBP_FACTION,	//ÃÅÅÉ		nParam±íÊ¾ÃÅÅÉÊôĞÔ£¬Èç¹ûnParamÎª-1±íÊ¾Ã»ÓĞÃÅÅÉ
	PBP_CLIQUE,		//°ïÅÉ		nParamÎª·Ç0Öµ±íÊ¾ÈëÁË°ïÅÉ£¬0Öµ±íÊ¾ÍÑÀëÁË°ïÅÉ
};

//ĞÂÎÅÏûÏ¢µÄÀàĞÍ¶¨Òå
enum NEWS_MESSAGE_TYPE
{
	NEWSMESSAGE_NORMAL,			//Ò»°ãÏûÏ¢£¬ÏÔÊ¾£¨Ò»´Î£©¾ÍÏûÏ¢ÏûÍöÁË
								//ÎŞÊ±¼ä²ÎÊı
	NEWSMESSAGE_COUNTING,		//µ¹¼Æ£¨Ãë£©ÊıÏûÏ¢£¬¼ÆÊıµ½0Ê±£¬¾ÍÏûÏ¢¾ÍÏûÍöÁË¡£
								//Ê±¼ä²ÎÊıÖĞµÄÊı¾İ½á¹¹ÖĞ½öÃëÊı¾İÓĞĞ§£¬µ¹¼ÆÊıÒÔÃëÎªµ¥Î»¡£
	NEWSMESSAGE_TIMEEND,		//¶¨Ê±ÏûÏ¢£¬¶¨Ê±µ½Ê±£¬ÏûÏ¢¾ÍÏûÍêÁË£¬·ñÔòÃ¿°ë·ÖÖÓÏÔÊ¾Ò»´Î¡£
								//Ê±¼ä²ÎÊı±íÊ¾ÏûÍöµÄÖ¸¶¨Ê±¼ä¡£
};

#define MAX_MESSAGE_LENGTH 512

struct KNewsMessage
{
	int		nType;						//ÏûÏ¢ÀàĞÍ
	char	sMsg[MAX_MESSAGE_LENGTH];	//ÏûÏ¢ÄÚÈİ
	int		nMsgLen;					//ÏûÏ¢ÄÚÈİ´æ´¢³¤¶È
};

struct KRankIndex
{
	bool			bValueAppened;	//Ã¿Ò»ÏîÊÇ·ñÓĞÃ»ÓĞ¶îÍâÊı¾İ
	bool			bSortFlag;		//Ã¿Ò»ÏîÊÇ·ñÓĞÃ»ÓĞÉı½µ±ê¼Ç
	unsigned short	usIndexId;		//ÅÅÃûÏîIDÊıÖµ
};

#define MAX_RANK_MESSAGE_STRING_LENGTH 128

struct KRankMessage
{
	char szMsg[MAX_RANK_MESSAGE_STRING_LENGTH];	// ÎÄ×ÖÄÚÈİ
	unsigned short		usMsgLen;				// ÎÄ×ÖÄÚÈİµÄ³¤¶È
	short				cSortFlag;				// Æì±êÖµ£¬QOO_RANK_DATAµÄÊ±ºò±íÊ¾³öÉı½µ£¬¸ºÖµ±íÊ¾½µ£¬ÕıÖµ±íÊ¾Éı£¬0Öµ±íÊ¾Î»ÖÃÎ´±ä
	int					nValueAppend;			// ´ËÏî¸½´øµÄÖµ

};

struct KMissionRecord
{
	char			sContent[256];	//´æ´¢ÈÎÎñÌáÊ¾ĞÅÏ¢µÄ»º³åÇø£¬£¨×Ö·û´®Îª¿ØÖÆ·ûÒÑ¾­±àÂëµÄ×Ö·û´®£©
	int				nContentLen;	//sContentÄÚÓĞĞ§ÄÚÈİµÄ³¤¶È(µ¥Î»£º×Ö½Ú)£¬³¤¶È×î´óÒ»¶¨²»³¬¹ı256×Ö½Ú
	unsigned int	uValue;			//¹ØÁªÊıÖµ
};

//---------------------------- bang hoi ------------------------

#define		defTONG_MAX_DIRECTOR				7
#define		defTONG_MAX_MANAGER					56
#define		defTONG_ONE_PAGE_MAX_NUM			25

#define		defTONG_STR_LENGTH					32
//guve
#define		defTONG_NAME_MAX_LENGTH				15
#define		defTONG_MAX_MEMINFOSYNC				5
#define		defTONG_MAX_PAGEINFOSYNC			10

#define defTITLE_MASTER		"Bang chñ"
#define defTITLE_DIRECT		"Tr­ëng l·o"
#define defTITLE_MANAGER	"§éi tr­ëng"
#define defTITLE_MEMBER		"M«n ®Ö"

#define defRIGHT_DEPOSE			0x01
#define defRIGHT_CHANGECAMP		0x02
#define defRIGHT_CHANGETITLE	0x04
#define defRIGHT_KICKOUT		0x08
#define defRIGHT_RECORD			0x10
#define defRIGHT_LEAGUE			0x20
#define defRIGHT_BUILDLEVEL		0x40
#define defRIGHT_FORCERETIRE	0x80
#define defRIGHT_TONGMAP		0x100
#define defRIGHT_WORKSHOP		0x200
#define defRIGHT_CLAIMWAR		0x400
#define defRIGHT_FUNDMANAGER	0x800
#define defRIGHT_WEEKGOAL		0x1000
#define defRIGHT_RECRUIT		0x2000
#define defRIGHT_FULL			0x3FFF

#define defTONG_MAX_DEDUCTMONEY		50
#define defTONG_LEAVE_MONEY			5
#define defTONG_CONDITION_MINMEM	16
#define defTONG_CONDITION_MINMONEY	100
#define defTONG_MAX_LEVEL			100
#define defTONG_EXP_PERLEVEL		20000
#define defTONG_EXP_COSTMONEY		1
#define defTONG_MAX_OFFER_DAYLIMIT	1000
#define defTONG_MAX_WEEK_OFFER		22400
#define defTONG_MONEY2OFFER_RATE	10
#define defTONG_MAX_MSG				50
#define defTONG_MONEY2CHANGECAMP	100

enum TONG_MEMBER_FIGURE
{
	enumTONG_FIGURE_MEMBER,				// mon de
	enumTONG_FIGURE_MANAGER,			// doi truong
	enumTONG_FIGURE_DIRECTOR,			// truong lao
	enumTONG_FIGURE_MASTER,				// bang chu
	enumTONG_FIGURE_NUM,
};

enum
{
	enumTONG_APPLY_INFO_ID_TONG_HEAD,
	enumTONG_APPLY_INFO_MEMBERPAGE,
	enumTONG_APPLY_INFO_TONGPAGE,
	enumTONG_APPLY_INFO_UNIONPAGE,
	enumTONG_APPLY_INFO_RECORD,
	enumTONG_APPLY_INFO_ID_NUM,
};

//guve
enum TONG_ACTION_TYPE
{
	TONG_ACTION_DISMISS,	//kick
	TONG_ACTION_ASSIGN,		//bo nhiem
	TONG_ACTION_DEMISE,		//chuyen vi
	TONG_ACTION_LEAVE,		//roi` bang
	TONG_ACTION_RECRUIT,	//dong mo tuyen dung
	TONG_ACTION_APPLY,		//xin vao bang
	TONG_ACTION_RIGHT,		//phan quyen
	TONG_ACTION_CONTRIBMONEY,	//gui tien ngan quy~
	TONG_ACTION_WITHDRAWMONEY,	//rut tien ngan quy~
	TONG_ACTION_STOREOFFER,	//gui cong hien du tru
	TONG_ACTION_DISPENSEOFFER,	//phat cong hien ca nhan
	TONG_ACTION_ASSIGNMONEY,	//phat ngan quy~ all mem
	TONG_ACTION_ASSIGNOFFER,	//phat cong hien du tru all mem
	TONG_ACTION_TRANSMONEY,		//chuyen ngan sach kien thiet
	TONG_ACTION_STOREBUILDFUND,		//gui tien vao ngan sach kien thiet
	TONG_ACTION_ANNOUNCE,	//thay doi thong cao'
	TONG_ACTION_CHANGETITLE,	//doi ten ca nhan
	TONG_ACTION_CHANGETITLE_MALE,	//doi ten thanh vien nam
	TONG_ACTION_CHANGETITLE_FEMALE,	//doi ten thanh vien nu
	TONG_ACTION_CHANGECAMP,	//doi phe
	TONG_ACTION_UPBUILDLEVEL,	//nang cap kien thiet
	TONG_ACTION_ENTERMAP,	//vao khu vuc bang
	TONG_ACTION_CREATEMAP,	//tao ban do
	TONG_ACTION_CONFIGMAP,	//thiet lap ban do
};

struct TTongWeekGoal
{
	int nTWeekGoal;
	int nMWeekGoal;
	int nTCompletedWG;
	int nTWeeGoalPrice;
	int nMWeeGoalPrice;
	WORD wWeekGoalType;
	BYTE bTGetPrice;
	BYTE nTaskLevel;
	TTongWeekGoal()
	{
		nTWeekGoal = 0;
		nMWeekGoal = 0;
		nTCompletedWG = 0;
		nTWeeGoalPrice = 0;
		nMWeeGoalPrice = 0;
		wWeekGoalType = 0;
		bTGetPrice = 0;
		nTaskLevel = 0;
	}
};

struct STONG_MEMBER
{
	char	m_szName[defTONG_STR_LENGTH];
	char	m_szTitle[defTONG_STR_LENGTH];
	UINT	m_dwNameID;
	UINT	m_uJoinDate;	//ngay gia nhap
	UINT	m_uOnlineDate;	//ngay online gan nhat
	UINT	m_uRight;		//quyen`
	int		m_nTotalOffer;	//tong cong hien
	int		m_nWeekOffer;	//cong hien tuan
	int		m_nOldWGCompleted; //hoan thanh muc tieu
	int		m_nNewWGCompleted; //hoan thanh muc tieu
	BYTE	m_nSex;		//0:nam
	BYTE	m_nFigure;	//chuc vu
	BYTE	m_bWGType;	//duoc nhan nhiem vu
	BYTE	m_bGetPrice;	//da nhan thuong~
	BYTE	m_bRetired;		//da quy an~
	STONG_MEMBER()
	{
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
	bool operator==(const STONG_MEMBER& dst) const
	{
		return m_dwNameID == dst.m_dwNameID;
	}
};

struct STONG_MEMSUBINFO : public STONG_MEMBER
{
	BYTE	btOnline;
	STONG_MEMSUBINFO()
	{
		btOnline = 0;
	}
};

struct STONG_PAGEMEM
{
	char	m_szName[defTONG_STR_LENGTH];
	int		m_nLevel;
};

//chua' thong tin lenh chuc nang bang
struct KTongOperationParam
{
	int                 nData[4];
	char				Name[32];		//chua' chuoi~
};

//Ä³¸öÍæ¼ÒÓëXXµÄ¹ØÏµ£¬(XX¿ÉÒÔÊÇ°ï»á£¬¶ÓÎéµÈµÈ)
struct KUiPlayerRelationWithOther : KUiPlayerItem
{
	int		nRelation;
	int		nParam;
};

//Í¨ÓÃµÄ´øÃû³ÆÃèÊöÓÎÏ·¶ÔÏóµÄ½á¹¹
struct KUiGameObjectWithName
{
	char			szName[32];
	int				nData;
	int				nParam;
	unsigned int 	uParam;
};

//-------------------------- °ï»áÏà¹Ø end ----------------------

#endif
