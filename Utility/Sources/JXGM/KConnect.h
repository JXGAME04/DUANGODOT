// -------------------------------------------------------------------------
//	文件名		：	KConnect.h
//	创建者		：	万里
//	创建时间	：	2003-6-3 16:43:50
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __KCONNECT_H__
#define __KCONNECT_H__

#include "KThread.h"

#include <map>

#include "KRelayProtocol.h"

struct IClient;

typedef void (* FUN_PROCESS_PROTOCOL)(void* pThis, const void *pData, size_t dataLength);
typedef std::map<WORD, FUN_PROCESS_PROTOCOL> PROTOCOL_HASH;

class KConnect : public KThread
{
public:
	KConnect();
	virtual ~KConnect();
	BOOL InitClient(const size_t &maxFreeBuffers, const size_t &bufferSize);
	BOOL ConnectTo(DWORD nIP, short nPort);
	DWORD GetClientID() {return m_nClientID;}

	DWORD m_nIP;
	short m_nPort;
	void SendData(const void * const pData1, const size_t& datalength1, const void * const pData2 = NULL, const size_t& datalength2 = 0);
	void SendProtocol(BYTE nProtocol, const void * const pSrcData1, const size_t& datalength1, const void * const pSrcData2 = NULL, const size_t& datalength2 = 0);
	void SendRelayData(char* szAccount, const void * const pSrcData1, const size_t& datalength1, const void * const pSrcData2 = NULL, const size_t& datalength2 = 0);

protected:
	virtual DWORD Main(LPVOID lpParam);
	void UnInitClient();

	PROTOCOL_HASH m_ProtocolTable;
	
	IClient* m_pClient;
	DWORD m_nClientID;	//from 1

	static void __stdcall ClientEventNotify(LPVOID lpParam, const unsigned long &ulnEventType);

	static DWORD CreateClientID();
	static DWORD m_s_ClientIndex;

	virtual void ConnectCreate();
	virtual void ConnectClose();
	virtual void ConnectIdle();

	virtual void ConnectData(const void *pData, size_t dataLength);
};

#define FUNC_TIMEOUT		10000	//有返回值的函数调用允许的时间延迟

#endif // __KCONNECT_H__