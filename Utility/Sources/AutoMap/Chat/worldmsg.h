/////////////////////////////////////////////////////////////
#include "iNet.h"

const int c_nPort = 10001;

enum
{
	Msg_World_Start = Msg_Net_End, 

	Msg_c2s_Account = Msg_World_Start,
	Msg_s2c_Account,

	Msg_c2s_Chat,
	Msg_s2c_Chat,

	Msg_c2s_Quit,

	Msg_World_End, 

	MsgNum,
};

//world
#define MAX_ACCOUNT 32
#define MAX_PASSWORD 32
#define MAX_WORD 128

struct stMsg_c2s_Account : public stMsg
{
	stMsg_c2s_Account() : stMsg(Msg_c2s_Account){}
	char szAccount[MAX_ACCOUNT];
	char szPassword[MAX_PASSWORD];
};

struct stMsg_s2c_Account : public stMsg
{
	stMsg_s2c_Account() : stMsg(Msg_s2c_Account){}
	// <0 error, =0 create, ==1 reconnect, == 2 grab the control
	long lResult;
};

struct stMsg_s2c_Chat: public stMsg
{
	stMsg_s2c_Chat() : stMsg(Msg_s2c_Chat){}
	char szChat[MAX_WORD];
	char szAccount[MAX_ACCOUNT];
};

struct stMsg_c2s_Chat: public stMsg
{
	stMsg_c2s_Chat() : stMsg(Msg_c2s_Chat){}
	char szChat[MAX_WORD];
	char szAccount[MAX_ACCOUNT];
};

struct stMsg_c2s_Quit : public stMsg
{
	stMsg_c2s_Quit() : stMsg(Msg_c2s_Quit){}
};
