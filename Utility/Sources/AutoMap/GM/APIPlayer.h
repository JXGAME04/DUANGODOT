//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIPlayer.h
//	创建人			:	王西贝
//	创建时间		:	2003-7-7 19:15:43
//	文件说明		:	Player API 的实现工具类头文件
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __APIPLAYER_H__
#define __APIPLAYER_H__

#include "GameMaster_i.h"
#include "APIChild.h"

class KGameMasterCore;
class KPlayerApiTool :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IKPlayer, &IID_IKPlayer, &LIBID_GameMasterLib>,
	public KAPIChild, public KAPIParent
{
public:
	KPlayerApiTool();
	void Init(KGameMasterCore*, LPCSTR strName);
	virtual ~KPlayerApiTool();
	LPCSTR GetChildName() { return m_strAccountName; }

protected:
	CString m_strAccountName;
	KGameMasterCore* m_pGMCoreRef;

public:
	STDMETHOD(KickOffActiveRole)(long ForbidMinutes);
	STDMETHOD(UnlockAccount)(/*[in]*/ BSTR strPassword);
	STDMETHOD(ItemRoleByIndexInGW)(/*[in]*/ BSTR strGameWorldName, /*[in]*/long Index, /*[out, retval]*/ IKRole** prop);
	STDMETHOD(ItemRoleByNameInGW)(/*[in]*/ BSTR strGameWorldName, /*[in]*/BSTR strRoleName, /*[out, retval]*/ IKRole** prop);
	STDMETHOD(get_CountOfRolesInGW)(/*[in]*/ BSTR strGameWorldName, /*[out, retval]*/ long *pVal);
	STDMETHOD(get_ActiveRole)(/*[out, retval]*/ IKRole** pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);

	typedef CComCreator< CComObjectNoLock < KPlayerApiTool > > _CreatorClass;

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(KPlayerApiTool)
	COM_INTERFACE_ENTRY(IKPlayer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

};

#endif //__APIPLAYER_H__
