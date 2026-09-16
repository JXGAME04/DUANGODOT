// cZipWrite.h: interface for the cZipWrite class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CZIPWRITE_H__F3BF2EB7_74DB_42B5_891D_2F5C84AF6B1F__INCLUDED_)
#define AFX_CZIPWRITE_H__F3BF2EB7_74DB_42B5_891D_2F5C84AF6B1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iZip.h"

class cZipWrite : public iZipWrite  
{
public:
	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; };
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;};
	cZipWrite();
	virtual ~cZipWrite();

	void* m_zip;
	bool create(LPCSTR szFile) ;				//create a .zip file
	bool add(LPCSTR szFile,LPCSTR szZipName) ;	//add a file to zip file
	bool close() ;								//save	
};

#endif // !defined(AFX_CZIPWRITE_H__F3BF2EB7_74DB_42B5_891D_2F5C84AF6B1F__INCLUDED_)
