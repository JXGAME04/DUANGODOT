#ifndef __SKILLDEF_H__
#define __SKILLDEF_H__

#define MAX_SKILL  2400
#define MAX_SKILLLEVEL 64
#define MaxMissleDir	64
#define MAXSKILLLEVELSETTINGNUM	20  //¹©ÌîĞ´¼¼ÄÜÉı¼¶Ê±×î´óµÄÏà¹ØÊı¾İÖÖÀà
#define MAX_MISSLESTYLE  1000
#define MISSLE_MIN_COLLISION_ZHEIGHT 0	  //×Óµ¯ÂäµØÅö×²µÄ¸ß¶È¡£
#define MISSLE_MAX_COLLISION_ZHEIGHT 20   //×Óµ¯¸ßÓÚ¸Ã¸ß¶ÈÊ±,²»¼ÆËãÅö×²	

//---------------------------------------------------------------------------
// MoveKind ÔË¶¯ÀàĞÍ
//---------------------------------------------------------------------------
enum eMissleMoveKind
{
	MISSLE_MMK_Stand,							//	Ô­µØ
		MISSLE_MMK_Line,							//	Ö±Ïß·ÉĞĞ
		MISSLE_MMK_Random,							//	Ëæ»ú·ÉĞĞ£¨°µºÚ¶şÅ®Î×µÄCharged Bolt£©
		MISSLE_MMK_Circle,							//	»·ĞĞ·ÉĞĞ£¨Î§ÈÆÔÚÉí±ß£¬°µºÚ¶ş´Ì¿ÍµÄ¼¯Æø£©
		MISSLE_MMK_Helix,							//	°¢»ùÃ×µÂÂİĞıÏß£¨°µºÚ¶şÓÎÏÀµÄBless Hammer£©
		MISSLE_MMK_Follow,							//	¸ú×ÙÄ¿±ê·ÉĞĞ
		MISSLE_MMK_Motion,							//	Íæ¼Ò¶¯×÷Àà
		MISSLE_MMK_Parabola,						//	Å×ÎïÏß
		MISSLE_MMK_SingleLine,						//	±ØÖĞµÄµ¥Ò»Ö±Ïß·ÉĞĞÄ§·¨
		MISSLE_MMK_RollBack = 100,					//  ×Óµ¥À´»Ø·ÉĞĞ
		MISSLE_MMK_Toss		,						//	×óÓÒÕğµ´
};

//---------------------------------------------------------------------------
// FollowKind ¸úËæÀàĞÍ	(Ö÷ÒªÊÇÕë¶ÔÔ­µØ¡¢»·ĞĞÓëÂİĞıÏß·ÉĞĞÓĞÒâÒå)
//---------------------------------------------------------------------------
enum eMissleFollowKind
{
	MISSLE_MFK_None,							//	²»¸úËæÈÎºÎÎï¼ş
	MISSLE_MFK_NPC,								//	¸úËæNPC»òÍæ¼Ò
	MISSLE_MFK_Missle,							//	¸úËæ×Óµ¯
};

#define	MAX_MISSLE_STATUS 4
enum eMissleStatus
{
	MS_DoWait,
	MS_DoFly,
	MS_DoVanish,
	MS_DoCollision,
};


enum eSkillLRInfo
{
	BothSkill,          //×óÓÒ¼ü½Ô¿É
	leftOnlySkill,		//×ó¼ü
	RightOnlySkill,		//ÓÒ¼ü
	NoneSkill,			//¶¼²»¿É
};

//--------------------------------------------------------Skill.h

//¼¼ÄÜ·¢ËÍÕßµÄÀàĞÍ
enum eGameActorType
{
	Actor_Npc,
	Actor_Obj,
	Actor_Missle,
	Actor_Sound,
	Actor_None,
};
enum eSkillLauncherType
{
	SKILL_SLT_Npc = 0,
	SKILL_SLT_Obj ,
	SKILL_SLT_Missle,
};


#ifndef _SERVER

struct	TOrginSkill
{
	int		nNpcIndex;				//	NpcµÄindex
	DWORD	nSkillId;				//	·¢ËÍµÄskillid
};

#endif


enum eSkillParamType
{
	SKILL_SPT_TargetIndex	= -1,
	SKILL_SPT_Direction		= -2,
};

//¼¼ÄÜµÄÀàĞÍ
enum eSKillStyle
{
	SKILL_SS_Missles = 0,			//	×Óµ¯Àà		±¾¼¼ÄÜÓÃÓÚ·¢ËÍ×Óµ¯Àà
		SKILL_SS_Melee,
		SKILL_SS_InitiativeNpcState,	//	Ö÷¶¯Àà		±¾¼¼ÄÜÓÃÓÚ¸Ä±äµ±Ç°NpcµÄÖ÷¶¯×´Ì¬
		SKILL_SS_PassivityNpcState,		//	±»¶¯Àà		±¾¼¼ÄÜÓÃÓÚ¸Ä±äNpcµÄ±»¶¯×´Ì¬
		SKILL_SS_CreateNpc,				//	²úÉúNpcÀà	±¾¼¼ÄÜÓÃÓÚÉú³ÉÒ»¸öĞÂµÄNpc
		SKILL_SS_BuildPoison,			//	Á¶¶¾Àà		±¾¼¼ÄÜÓÃÓÚÁ¶¶¾
		SKILL_SS_AddPoison,				//	¼Ó¶¾Àà		±¾¼¼ÄÜÓÃÓÚ¸øÎäÆ÷¼Ó¶¾ĞÔ
		SKILL_SS_GetObjDirectly,		//	È¡ÎïÀà		±¾¼¼ÄÜÓÃÓÚ¸ô¿ÕÈ¡Îï
		SKILL_SS_StrideObstacle ,		//	¿çÔ½Àà		±¾¼¼ÄÜÓÃÓÚ¿çÔ½ÕÏ°­
		SKILL_SS_BodyToObject,			//	±äÎïÀà		±¾¼¼ÄÜÓÃÓÚ½«Ê¬Ìå±ä³É±¦Ïä
		SKILL_SS_Mining,				//	²É¿óÀà		±¾¼¼ÄÜÓÃÓÚ²É¿óËæ»úÉú³É¿óÊ¯
		SKILL_SS_RepairWeapon,			//	ĞŞ¸´Àà		±¾¼¼ÄÜÓÃÓÚĞŞ¸´×°±¸
		SKILL_SS_Capture,				//	²¶×½Àà		±¾¼¼ÄÜÓÃÓÚ²¶×½¶¯ÎïNpc
		SKILL_SS_Thief,					//	ÍµÇÔÀà
};


//Í¬Ê±·¢³öµÄ¶à¸ö×Óµ¯µÄ·½ÏòÆğÊ¼¸ñÊ½
enum eMisslesForm
{
	SKILL_MF_Wall	= 0,	// 0´xÕp song song nh­ bøc t­êng, vİ dô 3 kiÕm Nga Mi, 4 rång C¸i Bang
	SKILL_MF_Line,			// 1 bay ®­êng th¼ng
	SKILL_MF_Spread,		// 2 bay ph©n t¸n, vİ dô 15 rång, 8 kiÕm Vâ §ang
	SKILL_MF_Circle,		// 3 ph©n t¸n trßn, vİ dô 5x bæng CB, 9x §M ná b¾n tia tÇng 2
	SKILL_MF_Random,		// 4 xuÊt hiÖn ngÉu nhiªn, vİ dô sĞt 9x C«n L«n
	SKILL_MF_Zone,			// 5 kh«ng thÊy sö dông
	SKILL_MF_AtTarget,		// 6 xuÊt hiÖn t¹i vŞ trİ môc tiªu, vİ dô V§ Thiªn §Şa VC 9x
	SKILL_MF_AtFirer,		// 7 xuÊt hiÖn t¹i vŞ trİ b¶n th©n, vİ dô 9x C«n TL, c¸c skill vßng s¸ng
	SKILL_MF_COUNT,
};

enum eMeleeForm
{
	Melee_AttackWithBlur = SKILL_MF_COUNT,
	Melee_Jump,
	Melee_JumpAndAttack,
	Melee_RunAndAttack,
	Melee_ManyAttack,
};


enum eSKillCostType
{
	SKILL_SCT_MANA		= 1,
		SKILL_SCT_LIFE		= 2,
		SKILL_SCT_STAMINA	= 8,
		SKILL_SCT_MONEY		= 16,
};

enum eMisslesGenerateStyle
{
	SKILL_MGS_NULL		= 0,
		SKILL_MGS_SAMETIME	,    //Í¬Ê±
		SKILL_MGS_ORDER		,	 //°´Ë³Ğò
		SKILL_MGS_RANDONORDER,
		SKILL_MGS_RANDONSAME,
		SKILL_MGS_CENTEREXTENDLINE,  //ÓÉÖĞ¼äÏòÁ½ÖÜÀ©É¢
};

typedef struct 
{
	int dx;
	int dy;
}TCollisionOffset;

typedef struct 
{
	int nRegion;
	int nMapX;
	int nMapY;
}
TMisslePos;

typedef struct 
{
	TCollisionOffset m_Offset [4];
}
TCollisionMatrix;
extern TCollisionMatrix g_CollisionMatrix[64];


typedef struct 
{
	int nLauncher;	
	DWORD dwLauncherID;			
	eSkillLauncherType eLauncherType; //·¢ËÍÕß£¬Ò»°ãÎªNpc


	int nParent;
	eSkillLauncherType eParentType;	  //Ä¸	 
	DWORD dwParentID;

	int nParam1;
	int nParam2;
	int nWaitTime;
	int nTargetId;
	DWORD dwTargetNpcID;
}
TOrdinSkillParam, * LPOrdinSkillParam;

#endif //__SKILLDEF_H__
