//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	FireEvents.h
//	创建人			:	王西贝
//	创建时间		:	2003-6-6 11:10:48
//	文件说明		:	各种 Event 发送(invoke)的实现
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __FIREEVENTS_H__
#define __FIREEVENTS_H__

#ifdef _NOT_DEFINED_
// include the header of CComDynamicUnkArray_GIT
#include "CEventProxy.h"

template <class T>
class CProxy_IGameMasterEvents : public IConnectionPointImpl<T, &DIID__IGameMasterEvents, CComDynamicUnkArray_GIT /* !! */>
{
	//Warning this class may be recreated by the wizard.
public:
	HRESULT Fire_Alert(BSTR Message)
	{
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant* pvars = new CComVariant[1];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();

            /* > change */
			CComQIPtr< IDispatch, &IID_IDispatch > pDispatch( sp.p );
            /* < change */

            /* > change */
			if (pDispatch.p != NULL)
            /* < change */
			{
				VariantClear(&varResult);
				pvars[0] = Message;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		delete[] pvars;
		return varResult.scode;
	
	}
};
#endif

#endif //__FIREEVENTS_H__
