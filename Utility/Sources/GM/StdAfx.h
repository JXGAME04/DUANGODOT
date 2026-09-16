// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__47ABE936_B3EF_4151_84F7_1CDAEC083EEF__INCLUDED_)
#define AFX_STDAFX_H__47ABE936_B3EF_4151_84F7_1CDAEC083EEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "KWin32.h"

// =========================================================================
// 当条件为假时跳转：
#define KS_CHECK_BOOLEX(result, expr)										\
	if (!(result))															\
	{																		\
		expr;																\
		ASSERT(0);															\
		goto KS_EXIT;														\
	}
#define KS_CHECK_BOOL(result)			KS_CHECK_BOOLEX(result, NULL)
#define KS_CHECK(hr)					KS_CHECK_BOOL(SUCCEEDED(hr))

//ATL Defines Below
#define _ATL_APARTMENT_THREADED

#define METHOD_PROLOGUE_ATL AFX_MANAGE_STATE(m_pModuleState)

#define m_dwRef						m_dwRefAtl
#define m_pOuterUnknown				m_pOuterUnknownAtl
#define InternalQueryInterface		InternalQueryInterfaceAtl
#define InternalAddRef				InternalAddRefAtl
#define InternalRelease				InternalReleaseAtl
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CGMModule : public CComModule
{
public:
	bool m_bATLInited;
	CGMModule();
	~CGMModule();

	LONG Unlock();
	LONG Lock();
	LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2);
	DWORD dwThreadID;
};
extern CGMModule _Module;
#include <atlcom.h>

#ifndef _AFX_OLD_EXCEPTIONS

// MFC does not rely on auto-delete semantics of the TRY..CATCH macros,
//  therefore those macros are mapped to something closer to the native
//  C++ exception handling mechanism when building MFC itself.

#undef TRY
#define TRY { try {

#undef CATCH
#define CATCH(class, e) } catch (class* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); UNUSED(e);

#undef AND_CATCH
#define AND_CATCH(class, e) } catch (class* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); UNUSED(e);

#undef CATCH_ALL
#define CATCH_ALL(e) } catch (CException* e) \
	{ { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); UNUSED(e);

#undef AND_CATCH_ALL
#define AND_CATCH_ALL(e) } catch (CException* e) \
	{ { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); UNUSED(e);

#undef END_TRY
#define END_TRY } catch (CException* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); e->Delete(); } }

#undef THROW_LAST
#define THROW_LAST() throw

// Because of the above definitions of TRY...CATCH it is necessary to
//  explicitly delete exception objects at the catch site.

#define DELETE_EXCEPTION(e) do { e->Delete(); } while (0)
#define NO_CPP_EXCEPTION(expr)

#else   //!_AFX_OLD_EXCEPTIONS

// In this case, the TRY..CATCH macros provide auto-delete semantics, so
//  it is not necessary to explicitly delete exception objects at the catch site.

#define DELETE_EXCEPTION(e)
#define NO_CPP_EXCEPTION(expr) expr

#endif  //_AFX_OLD_EXCEPTIONS


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__47ABE936_B3EF_4151_84F7_1CDAEC083EEF__INCLUDED_)
