/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue May 09 15:08:46 2006
 */
/* Compiler settings for F:\DownLoads\Utility\Sources\AutoMap\GM\GameMaster.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __GameMaster_i_h__
#define __GameMaster_i_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IKRole_FWD_DEFINED__
#define __IKRole_FWD_DEFINED__
typedef interface IKRole IKRole;
#endif 	/* __IKRole_FWD_DEFINED__ */


#ifndef __IKPlayer_FWD_DEFINED__
#define __IKPlayer_FWD_DEFINED__
typedef interface IKPlayer IKPlayer;
#endif 	/* __IKPlayer_FWD_DEFINED__ */


#ifndef __IKGameWorld_FWD_DEFINED__
#define __IKGameWorld_FWD_DEFINED__
typedef interface IKGameWorld IKGameWorld;
#endif 	/* __IKGameWorld_FWD_DEFINED__ */


#ifndef __IKGameMaster_FWD_DEFINED__
#define __IKGameMaster_FWD_DEFINED__
typedef interface IKGameMaster IKGameMaster;
#endif 	/* __IKGameMaster_FWD_DEFINED__ */


#ifndef __GameMaster_FWD_DEFINED__
#define __GameMaster_FWD_DEFINED__

#ifdef __cplusplus
typedef class GameMaster GameMaster;
#else
typedef struct GameMaster GameMaster;
#endif /* __cplusplus */

#endif 	/* __GameMaster_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IKRole_INTERFACE_DEFINED__
#define __IKRole_INTERFACE_DEFINED__

/* interface IKRole */
/* [oleautomation][nonextensible][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKRole;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AC961E16-2C9F-4dcf-9BCE-228B1218D2B6")
    IKRole : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PlayerAccountName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendMessageToThis( 
            /* [in] */ BSTR strMessage) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PutInJail( 
            /* [in] */ long Minutes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FreeFromJail( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShutUp( 
            /* [in] */ long Minutes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FreeChat( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RoleInGameWorldName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKRoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IKRole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IKRole __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IKRole __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IKRole __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IKRole __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IKRole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IKRole __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IKRole __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PlayerAccountName )( 
            IKRole __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendMessageToThis )( 
            IKRole __RPC_FAR * This,
            /* [in] */ BSTR strMessage);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutInJail )( 
            IKRole __RPC_FAR * This,
            /* [in] */ long Minutes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeFromJail )( 
            IKRole __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShutUp )( 
            IKRole __RPC_FAR * This,
            /* [in] */ long Minutes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeChat )( 
            IKRole __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RoleInGameWorldName )( 
            IKRole __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IKRoleVtbl;

    interface IKRole
    {
        CONST_VTBL struct IKRoleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKRole_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKRole_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKRole_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKRole_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKRole_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKRole_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKRole_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKRole_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IKRole_get_PlayerAccountName(This,pVal)	\
    (This)->lpVtbl -> get_PlayerAccountName(This,pVal)

#define IKRole_SendMessageToThis(This,strMessage)	\
    (This)->lpVtbl -> SendMessageToThis(This,strMessage)

#define IKRole_PutInJail(This,Minutes)	\
    (This)->lpVtbl -> PutInJail(This,Minutes)

#define IKRole_FreeFromJail(This)	\
    (This)->lpVtbl -> FreeFromJail(This)

#define IKRole_ShutUp(This,Minutes)	\
    (This)->lpVtbl -> ShutUp(This,Minutes)

#define IKRole_FreeChat(This)	\
    (This)->lpVtbl -> FreeChat(This)

#define IKRole_get_RoleInGameWorldName(This,pVal)	\
    (This)->lpVtbl -> get_RoleInGameWorldName(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKRole_get_Name_Proxy( 
    IKRole __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IKRole_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKRole_get_PlayerAccountName_Proxy( 
    IKRole __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IKRole_get_PlayerAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKRole_SendMessageToThis_Proxy( 
    IKRole __RPC_FAR * This,
    /* [in] */ BSTR strMessage);


void __RPC_STUB IKRole_SendMessageToThis_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKRole_PutInJail_Proxy( 
    IKRole __RPC_FAR * This,
    /* [in] */ long Minutes);


void __RPC_STUB IKRole_PutInJail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKRole_FreeFromJail_Proxy( 
    IKRole __RPC_FAR * This);


void __RPC_STUB IKRole_FreeFromJail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKRole_ShutUp_Proxy( 
    IKRole __RPC_FAR * This,
    /* [in] */ long Minutes);


void __RPC_STUB IKRole_ShutUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKRole_FreeChat_Proxy( 
    IKRole __RPC_FAR * This);


void __RPC_STUB IKRole_FreeChat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKRole_get_RoleInGameWorldName_Proxy( 
    IKRole __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IKRole_get_RoleInGameWorldName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKRole_INTERFACE_DEFINED__ */


#ifndef __IKPlayer_INTERFACE_DEFINED__
#define __IKPlayer_INTERFACE_DEFINED__

/* interface IKPlayer */
/* [oleautomation][nonextensible][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKPlayer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98C49B84-9506-4c52-95E9-5208E502B389")
    IKPlayer : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveRole( 
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountOfRolesInGW( 
            /* [in] */ BSTR strGameWorldName,
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemRoleByNameInGW( 
            /* [in] */ BSTR strGameWorldName,
            /* [in] */ BSTR strRoleName,
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemRoleByIndexInGW( 
            /* [in] */ BSTR strGameWorldName,
            /* [in] */ long Index,
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnlockAccount( 
            /* [in] */ BSTR strPassword) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE KickOffActiveRole( 
            /* [in] */ long ForbidMinutes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKPlayerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IKPlayer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IKPlayer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IKPlayer __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IKPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveRole )( 
            IKPlayer __RPC_FAR * This,
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountOfRolesInGW )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ BSTR strGameWorldName,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemRoleByNameInGW )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ BSTR strGameWorldName,
            /* [in] */ BSTR strRoleName,
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemRoleByIndexInGW )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ BSTR strGameWorldName,
            /* [in] */ long Index,
            /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockAccount )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ BSTR strPassword);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KickOffActiveRole )( 
            IKPlayer __RPC_FAR * This,
            /* [in] */ long ForbidMinutes);
        
        END_INTERFACE
    } IKPlayerVtbl;

    interface IKPlayer
    {
        CONST_VTBL struct IKPlayerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKPlayer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKPlayer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKPlayer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKPlayer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKPlayer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKPlayer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKPlayer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKPlayer_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IKPlayer_get_ActiveRole(This,prop)	\
    (This)->lpVtbl -> get_ActiveRole(This,prop)

#define IKPlayer_get_CountOfRolesInGW(This,strGameWorldName,pVal)	\
    (This)->lpVtbl -> get_CountOfRolesInGW(This,strGameWorldName,pVal)

#define IKPlayer_ItemRoleByNameInGW(This,strGameWorldName,strRoleName,prop)	\
    (This)->lpVtbl -> ItemRoleByNameInGW(This,strGameWorldName,strRoleName,prop)

#define IKPlayer_ItemRoleByIndexInGW(This,strGameWorldName,Index,prop)	\
    (This)->lpVtbl -> ItemRoleByIndexInGW(This,strGameWorldName,Index,prop)

#define IKPlayer_UnlockAccount(This,strPassword)	\
    (This)->lpVtbl -> UnlockAccount(This,strPassword)

#define IKPlayer_KickOffActiveRole(This,ForbidMinutes)	\
    (This)->lpVtbl -> KickOffActiveRole(This,ForbidMinutes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKPlayer_get_Name_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IKPlayer_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKPlayer_get_ActiveRole_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKPlayer_get_ActiveRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKPlayer_get_CountOfRolesInGW_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [in] */ BSTR strGameWorldName,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IKPlayer_get_CountOfRolesInGW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKPlayer_ItemRoleByNameInGW_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [in] */ BSTR strGameWorldName,
    /* [in] */ BSTR strRoleName,
    /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKPlayer_ItemRoleByNameInGW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKPlayer_ItemRoleByIndexInGW_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [in] */ BSTR strGameWorldName,
    /* [in] */ long Index,
    /* [retval][out] */ IKRole __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKPlayer_ItemRoleByIndexInGW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKPlayer_UnlockAccount_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [in] */ BSTR strPassword);


void __RPC_STUB IKPlayer_UnlockAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKPlayer_KickOffActiveRole_Proxy( 
    IKPlayer __RPC_FAR * This,
    /* [in] */ long ForbidMinutes);


void __RPC_STUB IKPlayer_KickOffActiveRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKPlayer_INTERFACE_DEFINED__ */


#ifndef __IKGameWorld_INTERFACE_DEFINED__
#define __IKGameWorld_INTERFACE_DEFINED__

/* interface IKGameWorld */
/* [oleautomation][nonextensible][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKGameWorld;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4D98C1D8-9AEE-427b-A9D3-E27BD47B7049")
    IKGameWorld : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OnlineCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsRunning( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BroadCast( 
            /* [in] */ BSTR strCharMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKGameWorldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IKGameWorld __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IKGameWorld __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IKGameWorld __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IKGameWorld __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IKGameWorld __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IKGameWorld __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IKGameWorld __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OnlineCount )( 
            IKGameWorld __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IKGameWorld __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsRunning )( 
            IKGameWorld __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BroadCast )( 
            IKGameWorld __RPC_FAR * This,
            /* [in] */ BSTR strCharMessage);
        
        END_INTERFACE
    } IKGameWorldVtbl;

    interface IKGameWorld
    {
        CONST_VTBL struct IKGameWorldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKGameWorld_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKGameWorld_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKGameWorld_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKGameWorld_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKGameWorld_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKGameWorld_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKGameWorld_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKGameWorld_get_OnlineCount(This,pVal)	\
    (This)->lpVtbl -> get_OnlineCount(This,pVal)

#define IKGameWorld_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IKGameWorld_get_IsRunning(This,pVal)	\
    (This)->lpVtbl -> get_IsRunning(This,pVal)

#define IKGameWorld_BroadCast(This,strCharMessage)	\
    (This)->lpVtbl -> BroadCast(This,strCharMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameWorld_get_OnlineCount_Proxy( 
    IKGameWorld __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IKGameWorld_get_OnlineCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameWorld_get_Name_Proxy( 
    IKGameWorld __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IKGameWorld_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameWorld_get_IsRunning_Proxy( 
    IKGameWorld __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IKGameWorld_get_IsRunning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameWorld_BroadCast_Proxy( 
    IKGameWorld __RPC_FAR * This,
    /* [in] */ BSTR strCharMessage);


void __RPC_STUB IKGameWorld_BroadCast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKGameWorld_INTERFACE_DEFINED__ */


#ifndef __IKGameMaster_INTERFACE_DEFINED__
#define __IKGameMaster_INTERFACE_DEFINED__

/* interface IKGameMaster */
/* [oleautomation][nonextensible][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKGameMaster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C75CC7B-3379-4a82-9FAB-9454C6D22434")
    IKGameMaster : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalOnlineCount( 
            /* [retval][out] */ long __RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalRegisteredCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsGMSystemRunning( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FindPlayer( 
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FindPlayerInRealTime( 
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountOfGameWorlds( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemGameWorldByName( 
            /* [in] */ BSTR strGWName,
            /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemGameWorldByIndex( 
            /* [in] */ long Index,
            /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemPlayerByName( 
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ IKPlayer __RPC_FAR *__RPC_FAR *prop) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SpeakToTheWorld( 
            /* [in] */ BSTR strCharMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKGameMasterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IKGameMaster __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IKGameMaster __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IKGameMaster __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalOnlineCount )( 
            IKGameMaster __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *prop);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalRegisteredCount )( 
            IKGameMaster __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsGMSystemRunning )( 
            IKGameMaster __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPlayer )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPlayerInRealTime )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountOfGameWorlds )( 
            IKGameMaster __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemGameWorldByName )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ BSTR strGWName,
            /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemGameWorldByIndex )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ long Index,
            /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemPlayerByName )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ BSTR strAccount,
            /* [retval][out] */ IKPlayer __RPC_FAR *__RPC_FAR *prop);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpeakToTheWorld )( 
            IKGameMaster __RPC_FAR * This,
            /* [in] */ BSTR strCharMessage);
        
        END_INTERFACE
    } IKGameMasterVtbl;

    interface IKGameMaster
    {
        CONST_VTBL struct IKGameMasterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKGameMaster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKGameMaster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKGameMaster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKGameMaster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKGameMaster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKGameMaster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKGameMaster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKGameMaster_get_TotalOnlineCount(This,prop)	\
    (This)->lpVtbl -> get_TotalOnlineCount(This,prop)

#define IKGameMaster_get_TotalRegisteredCount(This,pVal)	\
    (This)->lpVtbl -> get_TotalRegisteredCount(This,pVal)

#define IKGameMaster_get_IsGMSystemRunning(This,pVal)	\
    (This)->lpVtbl -> get_IsGMSystemRunning(This,pVal)

#define IKGameMaster_FindPlayer(This,strAccount,pstrInGameWorld)	\
    (This)->lpVtbl -> FindPlayer(This,strAccount,pstrInGameWorld)

#define IKGameMaster_FindPlayerInRealTime(This,strAccount,pstrInGameWorld)	\
    (This)->lpVtbl -> FindPlayerInRealTime(This,strAccount,pstrInGameWorld)

#define IKGameMaster_get_CountOfGameWorlds(This,pVal)	\
    (This)->lpVtbl -> get_CountOfGameWorlds(This,pVal)

#define IKGameMaster_ItemGameWorldByName(This,strGWName,prop)	\
    (This)->lpVtbl -> ItemGameWorldByName(This,strGWName,prop)

#define IKGameMaster_ItemGameWorldByIndex(This,Index,prop)	\
    (This)->lpVtbl -> ItemGameWorldByIndex(This,Index,prop)

#define IKGameMaster_ItemPlayerByName(This,strAccount,prop)	\
    (This)->lpVtbl -> ItemPlayerByName(This,strAccount,prop)

#define IKGameMaster_SpeakToTheWorld(This,strCharMessage)	\
    (This)->lpVtbl -> SpeakToTheWorld(This,strCharMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameMaster_get_TotalOnlineCount_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *prop);


void __RPC_STUB IKGameMaster_get_TotalOnlineCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameMaster_get_TotalRegisteredCount_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IKGameMaster_get_TotalRegisteredCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameMaster_get_IsGMSystemRunning_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IKGameMaster_get_IsGMSystemRunning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_FindPlayer_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ BSTR strAccount,
    /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld);


void __RPC_STUB IKGameMaster_FindPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_FindPlayerInRealTime_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ BSTR strAccount,
    /* [retval][out] */ BSTR __RPC_FAR *pstrInGameWorld);


void __RPC_STUB IKGameMaster_FindPlayerInRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKGameMaster_get_CountOfGameWorlds_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IKGameMaster_get_CountOfGameWorlds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_ItemGameWorldByName_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ BSTR strGWName,
    /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKGameMaster_ItemGameWorldByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_ItemGameWorldByIndex_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ long Index,
    /* [retval][out] */ IKGameWorld __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKGameMaster_ItemGameWorldByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_ItemPlayerByName_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ BSTR strAccount,
    /* [retval][out] */ IKPlayer __RPC_FAR *__RPC_FAR *prop);


void __RPC_STUB IKGameMaster_ItemPlayerByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKGameMaster_SpeakToTheWorld_Proxy( 
    IKGameMaster __RPC_FAR * This,
    /* [in] */ BSTR strCharMessage);


void __RPC_STUB IKGameMaster_SpeakToTheWorld_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKGameMaster_INTERFACE_DEFINED__ */



#ifndef __GameMasterLib_LIBRARY_DEFINED__
#define __GameMasterLib_LIBRARY_DEFINED__

/* library GameMasterLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_GameMasterLib;

EXTERN_C const CLSID CLSID_GameMaster;

#ifdef __cplusplus

class DECLSPEC_UUID("D67E5BA4-2C4F-4ec4-AC21-504703F32121")
GameMaster;
#endif
#endif /* __GameMasterLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
