// kMsg.cpp: implementation of the kMsg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "kMsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

kMsg::kMsg()
{
	m_ulCount = 1;
	m_pBuffer = NULL;
	memset(&m_param,0,sizeof(m_param));
	m_pSender = NULL;
	m_eSenderType = Sender_Net;
}

kMsg::~kMsg()
{
	if (m_pBuffer)
	{
		delete m_pBuffer;
		m_pBuffer= NULL;
	}
}

void kMsg::SetData(void* buffer,MSGSIZE size,MSGTYPE type,eMsgPack pack ,eMsgCode code )
{
	MSGSIZE sz = sizeof(stAParam) + size ;
	m_pBuffer = new char[sz];
	stParam* pMsg = (stParam*)m_pBuffer;
	pMsg->totalSize = sz;
	pMsg->type = type;
	pMsg->ePack = pack;
	pMsg->eCode = code;
	pMsg->size = size;
	void* buf = &pMsg->buffer;
	memcpy(buf,buffer,size);
	
	//pack and code!
}

void kMsg::SetParam(stParam& param) 
{
	if (param.mask & MMASK_TYPE)
	{
		m_param.type = param.type;
	}
	if (param.mask & MMASK_CODE)
	{
		m_param.eCode = param.eCode;
	}	
	if (param.mask & MMASK_SIZE)
	{
		m_param.size = param.size;
	}
	if (param.mask & MMASK_PACK)
	{
		m_param.ePack = param.ePack;
	}
	if (param.mask & MMASK_BUFFER)
	{
		m_param.buffer = param.buffer;
		m_pBuffer = new char[sizeof(stAParam)+m_param.size];
		m_param.totalSize = sizeof(stAParam)+m_param.size;
		memcpy(m_pBuffer,&m_param,sizeof(stAParam));
		memcpy(((char*)m_pBuffer+sizeof(stAParam)),m_param.buffer,m_param.size);
	}
	param = m_param;
}

void kMsg::GetParam(stParam& param)
{
	param = m_param;
}


MSGSIZE kMsg::Create(void* buffer,MSGSIZE totalSize)
{
	if (totalSize < sizeof (long))
		return 0;
	stParam* p = (stParam*) buffer;
	if (p->totalSize > totalSize)
		return 0;
	if (m_pBuffer)
		delete m_pBuffer;
	m_pBuffer = new char[p->totalSize];
	memcpy(m_pBuffer,buffer,p->totalSize);
	return p->totalSize;
}

_declspec(dllexport) iMsg* Message_CreateMsg(eMsgType eType,long verson1 ,long verson2 )
{
	return new kMsg;
}
