// kMsg.h: interface for the kMsg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KMSG_H__0168140B_046F_457C_9689_9C8E3D777AE5__INCLUDED_)
#define AFX_KMSG_H__0168140B_046F_457C_9689_9C8E3D777AE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iMsg.h"

class kMsg : public iMsg  
{
public:
	kMsg();
	virtual ~kMsg();

	ULONG m_ulCount;
	virtual ULONG AddRef() {return ++m_ulCount; };
	virtual ULONG Release() {ULONG count = --m_ulCount; if(m_ulCount == 0)delete this; return count;};
	
	struct stAParam
	{
		MSGSIZE totalSize;
		MSGTYPE type;
		eMsgPack ePack;
		eMsgCode eCode;
		MSGSIZE size;
	};
	//set and get
	stParam m_param;
	virtual void SetParam(stParam& param) ;
	virtual void GetParam(stParam& param) ;
	virtual void SetData(void* buffer,MSGSIZE size,MSGTYPE type,eMsgPack pack = PACK_NULL,eMsgCode code = CODE_NULL) ;
	//整个消息的大小和位置
	void* m_pBuffer;

	virtual stParam* GetParam(){return (stParam*) m_pBuffer;}
	virtual MSGSIZE GetSize(){return GetParam()->totalSize;};
	virtual MSGSIZE GetDataSize(){return GetParam()->size;};
	virtual MSGTYPE GetType(){return GetParam()->type;};
	virtual void* GetData(){return &GetParam()->buffer;};
	virtual void* GetBuffer(){return m_pBuffer;};

	virtual MSGSIZE Create(void* buffer,MSGSIZE totalSize) ;	
	
	void* m_pSender;
	eSenderType m_eSenderType;
	virtual void SetSender(void* p, eSenderType eType){m_pSender = p;m_eSenderType = eType;};
	virtual void GetSender(void*& p, eSenderType& eType){p = m_pSender ; eType = m_eSenderType;};
	
};

#endif // !defined(AFX_KMSG_H__0168140B_046F_457C_9689_9C8E3D777AE5__INCLUDED_)
