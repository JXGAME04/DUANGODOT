#pragma once

#include "iPublic.h"
#include "stMsg.h"

const int c_nMaxLink = 1000;

#define MAX_MSGSIZE 1024

class iServerLink;
class iNetMsg;

enum {
	MsgIdle,

	Msg_Net_Start,

	//客户端连入
	Msg_Net_ClientConnect = Msg_Net_Start,
	//客户端断线
	Msg_Net_ClientDown,
	//客户端断线的时间过长了
	Msg_Net_Dead, 
	
	//连入服务器
	Msg_Net_ServerConnect,
	//服务器down
	Msg_Net_ServerDown,

	Msg_Net_End,
};

/////////////////////////////////////////////////////////////
struct stMsgIdle : public stMsg
{
public:
	stMsgIdle():stMsg(MsgIdle){};
};

/////////////////////////////////////////////////////////////
//net 和 gameworld 同样通过 popMessage 来通讯
//net
struct stMsg_Net_Dead: public stMsg
{
	stMsg_Net_Dead() : stMsg(Msg_Net_Dead){}
};

//net
struct stMsg_Net_ClientConnect: public stMsg
{
	stMsg_Net_ClientConnect() : stMsg(Msg_Net_ClientConnect){}
};

//异常断线
struct stMsg_Net_ClientDown : public stMsg
{
	stMsg_Net_ClientDown() : stMsg(Msg_Net_ClientDown){}
	long lReason;
};

struct stMsg_Net_ServerConnect : public stMsg
{
	stMsg_Net_ServerConnect() : stMsg(Msg_Net_ServerConnect){}
	BOOL bConnect; //connected or not connnect
};

struct stMsg_Net_ServerDown: public stMsg
{
	stMsg_Net_ServerDown() : stMsg(Msg_Net_ServerDown){}
	long lReason;
};

