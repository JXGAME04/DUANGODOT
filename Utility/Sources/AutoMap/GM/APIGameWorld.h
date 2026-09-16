//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIGameWorld.h
//	创建人			:	王西贝
//	创建时间		:	2003-7-7 15:10:01
//	文件说明		:	GameWorld API 的实现工具类头文件
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __APIGAMEWORLD_H__
#define __APIGAMEWORLD_H__

#include "GameMaster_i.h"

class KGameMasterCore;
class KGameWorldApiTool :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IKGameWorld, &IID_IKGameWorld, &LIBID_GameMasterLib>,
	public KAPIChild
{
public:
	KGameWorldApiTool();
	void Init(KGameMasterCore*, LPCSTR strGWName, DWORD dwIP);
	virtual ~KGameWorldApiTool();

protected:
	CString m_strGWName;
	DWORD m_dwIP;
	KGameMasterCore* m_pGMCoreRef;

public:
	LPCSTR GetChildName() { return m_strGWName; }

public:
	STDMETHOD(get_IsRunning)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_OnlineCount)(/*[out, retval]*/long *pVal);
	STDMETHOD(BroadCast)(/*[in]*/BSTR strMessage);

	typedef CComCreator< CComObjectNoLock < KGameWorldApiTool > > _CreatorClass;

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(KGameWorldApiTool)
	COM_INTERFACE_ENTRY(IKGameWorld)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


};

#endif //__APIGAMEWORLD_H__
