#ifndef MESSAGE_H
#define MESSAGE_H

#include "ipublic.h"

typedef long MSGSIZE;
typedef long MSGTYPE;

enum eMsgType{
	Msg_Graphics,
	Msg_World,
	Msg_Net,
};

//sender

class iMsg 
{
public:
	//pack and code
	//unpack and uncode is autorun after create!
	enum eMsgPack{PACK_NULL,};
	enum eMsgCode{CODE_NULL,};
	enum eMsgMask{MMASK_TYPE = 0x01,
		MMASK_PACK = MMASK_TYPE*2, 
		MMASK_CODE = MMASK_PACK*2,
		MMASK_SIZE = MMASK_CODE*2,
		MMASK_BUFFER = MMASK_SIZE*2,
	};
	
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	struct stParam
	{
		MSGSIZE totalSize;
		MSGTYPE type;
		eMsgPack ePack;
		eMsgCode eCode;
		MSGSIZE size;
		void* buffer;
		long mask;
		long lParam,wParam;
	};

	//set and get
	virtual void SetParam(stParam& param) = 0;
	virtual void GetParam(stParam& param) = 0;
	virtual void SetData(void* buffer,MSGSIZE size,MSGTYPE type,eMsgPack pack = PACK_NULL,eMsgCode code = CODE_NULL) = 0;
	//整个消息的大小和位置
	virtual MSGSIZE GetDataSize() = 0;
	virtual void* GetData() = 0;
	virtual MSGTYPE GetType() = 0;
	virtual MSGSIZE GetSize() = 0;
	virtual void* GetBuffer() = 0;

	enum eSenderType{
		Sender_Net,
		Sender_Client,
		Sender_Server,
	};
	virtual void SetSender(void* p, eSenderType eType) = 0;
	virtual void GetSender(void*& p, eSenderType& eType) = 0;
	//use when a msg arrive 
	//return sz <= size! return 0 error!
	virtual MSGSIZE Create(void* buffer,MSGSIZE totalSize) = 0;
};

_declspec(dllexport) iMsg* Message_CreateMsg(eMsgType eType,long verson1 = 0,long verson2 = 0);

enum eMsgQueueType{
	MsgQueue_Graphics,
	MsgQueue_World,
	MsgQueue_Net,
	MsgQueue_Main,
};

class iMsgQueue
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual void Clear() = 0;
	virtual bool PushMsg(iMsg* pMsg) = 0;
	virtual iMsg* PopMsg() = 0;
	virtual ULONG WaitForMessageArrive(long time) = 0;
};

_declspec(dllexport) iMsgQueue* Message_CreateMsgQueue(eMsgQueueType eType,long verson1 = 0,long verson2 = 0);

#endif