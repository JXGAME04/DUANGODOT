#ifndef	KProtocolDefH
#define	KProtocolDefH


// Add by Freeway chen in 2003.7.1
// 定义协议兼容的版本，如果修改的协议，导致原有版本无法使用，需要修改下面的值

#define USE_KPROTOCOL_VERSION   1
//#undef USE_KPROTOCOL_VERSION

#define KPROTOCOL_VERSION   1
/*
 *
 */

/*
 * It was to judge a package type that 
 * it is a larger package or it is a small package
 */
const UINT g_nGlobalProtocolType = 31;

enum s2c_PROTOCOL
{
	s2c_roleserver_getroleinfo_result = 10,
	s2c_syncgamesvr_roleinfo_cipher,
	s2c_gamestatistic_bigpackage,
	/*
	 * This value must be equal to c2s_micropackbegin
	 */
	s2c_micropackbegin = g_nGlobalProtocolType,

	s2c_accountbegin = 32,
	s2c_accountlogin,
	s2c_gamelogin,
	s2c_accountlogout,
	s2c_gatewayverify,		//用于连接后第一个包
	s2c_gatewayverifyagain,		//用于重连后第一个包
	s2c_gatewayinfo,
	s2c_queryplayer,	//guve
	
	s2c_multiserverbegin = 48,
	s2c_querymapinfo,
	s2c_querygameserverinfo,
	s2c_identitymapping,
	s2c_notifyplayerlogin,
	s2c_notifyplayerexchange,
	s2c_notifysvrip,
	
	s2c_roleserver_getrolelist_result,
	s2c_roleserver_saverole_result,
	s2c_roleserver_createrole_result,
	s2c_roleserver_deleterole_result,
	s2c_logiclogout,
	s2c_gateway_broadcast,
	s2c_gamestatistic,
	
	s2c_clientbegin = 64,
/*65 */s2c_login,		//ref: ../../S3Client/Login/LoginDef.h
/*66 */s2c_logout,
/*67 */s2c_syncend,
/*68 */s2c_synccurplayer,
/*69 */s2c_synccurplayerskill,
/*70 */s2c_synccurplayernormal,
/*71 */s2c_newplayer,
/*72 */s2c_removeplayer,
/*73 */s2c_syncworld,
/*74 */s2c_syncplayer,
/*75 */s2c_syncplayermin,
/*76 */s2c_syncnpc,
/*77 */s2c_syncnpcmin,
/*78 */s2c_syncnpcminplayer,
/*79 */s2c_objadd,
/*80 */s2c_syncobjstate,
/*81 */s2c_syncobjdir,
/*82 */s2c_objremove,
/*83 */s2c_objTrapAct,
/*84 */s2c_npcremove,
/*85 */s2c_npcwalk,
/*86 */s2c_npcrun,
/*87 */s2c_dynamic_structure,
/*88 */s2c_npcmagic,
/*89 */s2c_npcjump,
/*90 */s2c_npctalk,
/*91 */s2c_npchurt,
/*92 */s2c_npcdeath,
/*93 */s2c_npcchgcurcamp,
/*94 */s2c_npcchgcamp,
/*95 */s2c_skillcast,
/*96 */s2c_playertalk,
/*97 */s2c_playerexp,
/*98 */s2c_teaminfo,
/*99 */s2c_teamselfinfo,
/*100*/s2c_teamapplyinfofalse,
/*101*/s2c_teamcreatesuccess,
/*102*/s2c_teamcreatefalse,
/*103*/s2c_teamopenclose,
/*104*/s2c_teamgetapply,
/*105*/s2c_teamaddmember,
/*106*/s2c_teamleave,
/*107*/s2c_teamchangecaptain,
/*108*/s2c_playerfactiondata,
/*109*/s2c_playerleavefaction,
/*110*/s2c_playerfactionskilllevel,
/*111*/s2c_playersendchat,
/*112*/s2c_playersyncleadexp,
/*113*/s2c_playerlevelup,
/*114*/s2c_teammatelevel,
/*115*/s2c_playersyncattribute,
/*116*/s2c_playerskilllevel,
/*117*/s2c_syncitem,
/*118*/s2c_removeitem,
/*119*/s2c_syncmoney,
/*120*/s2c_playermoveitem,
/*121*/s2c_scriptaction,
/*122*/s2c_chatapplyaddfriend,
/*123*/s2c_chataddfriend,
/*124*/s2c_chatrefusefriend,
/*125*/s2c_chataddfriendfail,
/*126*/s2c_chatloginfriendnoname,
/*127*/s2c_chatloginfriendname,
/*128*/s2c_chatonefrienddata,
/*129*/s2c_chatfriendonline,
/*130*/s2c_chatdeletefriend,
/*131*/s2c_chatfriendoffline,
/*132*/s2c_syncrolelist,
/*133*/s2c_tradechangestate,
/*134*/s2c_npcsetmenustate,
/*135*/s2c_trademoneysync,
/*136*/s2c_tradedecision,
/*137*/s2c_chatscreensingleerror,
/*138*/s2c_syncnpcstate,
/*139*/s2c_teaminviteadd,
/*140*/s2c_tradepressoksync,
/*141*/s2c_ping,
/*142*/s2c_npcsit,
/*143*/s2c_opensalebox,
/*144*/s2c_castskilldirectly,
/*145*/s2c_msgshow,
/*146*/s2c_syncstateeffect,
/*147*/s2c_openstorebox,
/*148*/s2c_playerrevive,
/*149*/s2c_requestnpcfail,
/*150*/s2c_tradeapplystart,
/*151*/s2c_rolenewdelresponse,	//新建与删除角色的结果返回,所带数据为结构tagNewDelRoleResponse
/*152*/s2c_ItemAutoMove,
/*153*/s2c_itemexchangefinish,
/*154*/s2c_changeweather,
/*155*/s2c_pksyncnormalflag,
/*156*/s2c_pksyncenmitystate,
/*157*/s2c_pksyncexercisestate,
/*158*/s2c_pksyncpkvalue,
/*159*/s2c_npcsleepmode,
/*160*/s2c_viewequip,
/*161*/s2c_ladderresult,
/*162*/s2c_ladderlist,
/*163*/s2c_tongcreate,
/*164*/s2c_replyclientping,
/*165*/s2c_npcgoldchange,
/*166*/s2c_itemdurabilitychange,

//	s2c_gmgateway2relaysvr,		//GM登陆后网关通知中转服务器有合法连接的协议

	s2c_extend = 250,
	s2c_extendchat = 251,
	s2c_extendfriend = 252,
	s2c_extendtong = 253,
	s2c_end,
};

enum c2s_PROTOCOL
{
	c2s_roleserver_saveroleinfo = 10,
	c2s_roleserver_createroleinfo,
	
	c2s_gmsvr2gateway_saverole,

	/*
	 * This value must be equal to s2c_micropackbegin
	 */
	c2s_micropackbegin = g_nGlobalProtocolType,

	c2s_accountbegin = 32,
	c2s_accountlogin,
	c2s_gamelogin,
	c2s_accountlogout,
	c2s_gatewayverify,
	c2s_gatewayverifyagain,
	c2s_gatewayinfo,

	c2s_multiserverbegin = 48,
	c2s_permitplayerlogin,
	c2s_updatemapinfo,
	c2s_updategameserverinfo,
	c2s_entergame,
	c2s_leavegame,
	c2s_registeraccount,

	c2s_requestsvrip,

	c2s_roleserver_getrolelist,
	c2s_roleserver_getroleinfo,
	c2s_roleserver_deleteplayer,
	c2s_gamestatistic,
	c2s_roleserver_lock,
	
	c2s_gameserverbegin = 64,
/*65 */ c2s_login,
/*66 */ c2s_logiclogin,
/*67 */ c2s_syncend,
/*68 */ c2s_loadplayer,
/*69 */ c2s_newplayer,
/*70 */ c2s_removeplayer,
/*71 */ c2s_requestworld,
/*72 */ c2s_requestplayer,
/*73 */ c2s_requestnpc,
/*74 */ c2s_requestobj,
/*75 */ c2s_npcwalk,
/*76 */ c2s_npcrun,
/*77 */ c2s_npcskill,
/*78 */ c2s_npcjump,
/*79 */ c2s_npctalk,
/*80 */ c2s_dynamic_structure,
/*81 */ c2s_npcdeath,
/*82 */ c2s_playertalk,
/*83 */ c2s_teamapplyinfo,
/*84 */ c2s_teamapplycreate,
/*85 */ c2s_teamapplyopenclose,
/*86 */ c2s_teamapplyadd,
/*87 */ c2s_teamacceptmember,
/*88 */ c2s_teamapplyleave,
/*89 */ c2s_teamapplykickmember,
/*90 */ c2s_teamapplychangecaptain,
/*91 */ c2s_teamapplydismiss,
/*92 */ c2s_playerapplysetpk,
/*93 */ c2s_playerapplyfactiondata,
/*94 */ c2s_playersendchat,
/*95 */ c2s_playeraddbaseattribute,
/*96 */ c2s_playerapplyaddskillpoint,
/*97 */ c2s_playereatitem,
/*98 */ c2s_playerpickupitem,
/*99 */ c2s_playermoveitem,
/*100*/ c2s_playersellitem,
/*101*/ c2s_playerbuyitem,
/*102*/ c2s_playerthrowawayitem,
/*103*/ c2s_playerselui,
/*104*/ c2s_chatsetchannel,
/*105*/ c2s_chatapplyaddfriend,
/*106*/ c2s_chataddfriend,
/*107*/ c2s_chatrefusefriend,
/*108*/ c2s_dbplayerselect,
/*109*/ c2s_chatapplyresendallfriendname,
/*110*/ c2s_chatapplysendonefriendname,
/*111*/ c2s_chatdeletefriend,
/*112*/ c2s_chatredeletefriend,
/*113*/ c2s_tradeapplystateopen,
/*114*/ c2s_tradeapplystateclose,
/*115*/ c2s_tradeapplystart,
/*116*/ c2s_trademovemoney,
/*117*/ c2s_tradedecision,
/*118*/ c2s_dialognpc,
/*119*/ c2s_teaminviteadd,
/*120*/ c2s_changeauraskill,
/*121*/ c2s_teamreplyinvite,
/*122*/ c2s_ping,
/*123*/ c2s_npcsit,
/*124*/ c2s_objmouseclick,
/*125*/ c2s_storemoney,
/*126*/ c2s_playerrevive,
/*127*/ c2s_tradereplystart,
/*128*/ c2s_pkapplychangenormalflag,
/*129*/ c2s_pkapplyenmity,
/*130*/ c2s_viewequip,
//	c2s_gmlogin,		//功效同c2s_login
/*131*/ c2s_ladderquery,
/*132*/ c2s_repairitem,
/*133*/ c2s_gmcommand,

	_c2s_begin_relay = 250,
	c2s_extend = _c2s_begin_relay,
	c2s_extendchat,
	c2s_extendfriend,
	_c2s_end_relay = c2s_extendfriend,
	c2s_extendtong,
	c2s_end,
};

enum c2c_PROTOCOL		//该协议族仅用于server和server之间
{
	c2c_transferroleinfo = 14,
	c2c_micropackbegin = g_nGlobalProtocolType,
	c2c_protocolbegin = 32,
	c2c_permitplayerexchangeout = 64,
	c2c_permitplayerexchangein,
	c2c_notifyexchange,

	s2s_broadcast = 96,		//用于从Relay到游戏世界的广播
	s2s_execute = 97,		//用于从Relay到游戏世界的执行脚本

	c2c_end,
};

//以下协议为c2s_extend协议的派生协议，参见KRelayProtocol.h
//扩展协议必须以EXTEND_HEADER打头

enum gm_PROTOCOL		//该协议族仅用于gm和server之间
{
	gm_begin = 32,
	gm_c2s_execute,				//以下协议由GM客户端以relay_c2c_askwaydata转发给游戏世界
	gm_c2s_disable,
	gm_c2s_enable,
	gm_c2s_tracking,
	gm_c2s_setrole,
	gm_c2s_getrole,	
	gm_c2s_findplayer,
	gm_c2s_unlock,
	gm_c2s_getrolelist,
	gm_c2s_broadcast_chat,

	gm_s2c_tracking,			//以下协议由游戏世界以relay_c2c_data转发给GM客户端
	gm_s2c_getrole,
	gm_s2c_findplayer,
	gm_s2c_getrolelist,

	gm_end,
};


enum relay_PROTOCOL		//该协议族仅用于server和relay之间
{
	relay_begin = 32,

	relay_c2c_data,

	relay_c2c_askwaydata,

	relay_s2c_loseway,

	relay_end,
};

enum chat_PROTOCOL
{
	chat_micropackbegin = g_nGlobalProtocolType,

	chat_someonechat,
	chat_channelchat,
	chat_feedback,

	chat_everyone,

	chat_groupman,
	chat_specman,

	chat_relegate,
	chat_filterplayer,
};

enum playercomm_PROTOCOL		//该协议族仅用于server和client之间,但是server可以转发给relay处理，因为relay承担了部分server的功能
{
	playercomm_begin = 32,

	playercomm_c2s_querychannelid,
	playercomm_s2c_notifychannelid,
	playercomm_c2s_freechannelid,

	playercomm_c2s_subscribe,
	
	playercomm_s2c_relegate,
	playercomm_c2s_rollback,

	playercomm_s2c_gmquerychannelid,
	playercomm_s2c_gmfreechannelid,
	playercomm_s2c_gmsubscribe,

	playercomm_c2s_someoneact,
	playercomm_c2s_channelact,

	playercomm_someonechat,
	playercomm_channelchat,
};

// game server 发给 s3client 帮会扩展协议 id
enum
{
	enumTONG_SYNC_ID_TRANSFER_ADD_APPLY,
	enumTONG_SYNC_ID_ADD,
	enumTONG_SYNC_ID_HEAD_INFO,
	enumTONG_SYNC_ID_SELF_INFO,
	enumTONG_SYNC_ID_MEMBER_INFO,
	enumTONG_SYNC_ID_TONGPAGE_INFO,
	enumTONG_SYNC_ID_RECORD_AFFAIRHISTORY,
	enumTONG_SYNC_ID_RECORD_ANNOUNCE,
	enumTONG_SYNC_ID_RECORD_WEEKTASK,
	enumTONG_SYNC_ID_INSTATE,
	enumTONG_SYNC_ID_KICK,
	enumTONG_SYNC_ID_SENDER_RIGHT,
	enumTONG_SYNC_ID_CHANGE_RECRUIT,
	enumTONG_SYNC_ID_ACCEPTMEMBER_FAIL,
	enumTONG_SYNC_ID_CONTRIBMONEY,
	enumTONG_SYNC_ID_STOREOFFER,
	enumTONG_SYNC_ID_ASSIGNMONEY,
	enumTONG_SYNC_ID_ASSIGNOFFER,
	enumTONG_SYNC_ID_TRANSMONEY,
	enumTONG_SYNC_ID_BUILDFUND,
	enumTONG_SYNC_ID_CHANGETITLE,
	enumTONG_SYNC_ID_CHANGETITLEALL,
	enumTONG_SYNC_ID_NUM,
};

// s3client 发给 game server 帮会扩展协议 id
enum
{
	enumTONG_COMMAND_ID_START = 0,
	enumTONG_COMMAND_ID_APPLY_CREATE,
	enumTONG_COMMAND_ID_APPLY_ADD,
	enumTONG_COMMAND_ID_ACCEPT_ADD,
	enumTONG_COMMAND_ID_APPLY_INFO,
	enumTONG_COMMAND_ID_APPLY_INSTATE,
	enumTONG_COMMAND_ID_APPLY_KICK,
	enumTONG_COMMAND_ID_APPLY_LEAVE,
	enumTONG_COMMAND_ID_APPLY_CHANGE_MASTER,
	enumTONG_COMMAND_ID_APPLY_RIGHT,
	enumTONG_COMMAND_ID_APPLY_RECRUIT,
	enumTONG_COMMAND_ID_APPLY_CONTRIBMONEY,
	enumTONG_COMMAND_ID_APPLY_WITHDRAWMONEY,
	enumTONG_COMMAND_ID_APPLY_STOREOFFER,
	enumTONG_COMMAND_ID_APPLY_DISPENSEOFFER,
	enumTONG_COMMAND_ID_APPLY_ASSIGNMONEY,
	enumTONG_COMMAND_ID_APPLY_ASSIGNOFFER,
	enumTONG_COMMAND_ID_APPLY_TRANSMONEY,
	enumTONG_COMMAND_ID_APPLY_STOREBUILDFUND,
	enumTONG_COMMAND_ID_APPLY_ANNOUNCE,
	enumTONG_COMMAND_ID_APPLY_CHANGETITLE,
	enumTONG_COMMAND_ID_APPLY_CHANGETITLEALL,
	enumTONG_COMMAND_ID_APPLY_CHANGECAMP,
	enumTONG_COMMAND_ID_APPLY_ACTION,
	enumTONG_COMMAND_ID_NUM,
};


#endif
