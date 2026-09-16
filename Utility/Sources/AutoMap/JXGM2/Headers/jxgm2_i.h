// -------------------------------------------------------------------------
//	文件名		：	jxgm2_i.h
//	创建者		：	万里
//	创建时间	：	2003-9-17 16:27:41
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __JXGM2_I_H__
#define __JXGM2_I_H__

#include "jxgm_i.h"

#define UNFILTER	0x0001		//不要过滤特殊脚本指令
#define GMLOGINOUT	0x0002		//要通知其他GM客户端登陆和登出
#define GMAUTORETURN	0x0004	//要自动回复客户端的脚本执行


struct ISystemConnect2 : public ISystemConnect
{
	virtual DWORD STDMETHODCALLTYPE GetIP() = 0;
	virtual DWORD STDMETHODCALLTYPE GetRelayID() = 0;
	virtual void STDMETHODCALLTYPE SetFlag(int nFlag) = 0;
};

struct ISystemOnline2: public ISystemOnline
{
	virtual HRESULT STDMETHODCALLTYPE OpenSystemConnect2(DWORD nIP, short nPort, ISystemConnect2** ppConnect) = 0;
};

STDAPI CreateSystemOnline2(ISystemOnline2** ppSystem);

#endif // __JXGM2_I_H__