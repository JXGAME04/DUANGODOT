// cZipReadDefault.h: interface for the cZipReadDefault class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CZIPREADDEFAULT_H__5B401CBA_CBDE_4ADA_801C_021C3F22F9CC__INCLUDED_)
#define AFX_CZIPREADDEFAULT_H__5B401CBA_CBDE_4ADA_801C_021C3F22F9CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iZip.h"

class cZipReadDefault : public iZipRead  
{
public:
	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; }
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;}
	cZipReadDefault():m_ulCount(1){}
	virtual ~cZipReadDefault(){}

	bool load(LPCSTR szZipFile){return false;} 		//load a .zip file from disk

	bool locate(LPCSTR szFile){return false;} 		//locate a file then can read
	int	length() {return -1;}						//length of the file located
	int	read(void* buf, int* bufsize) {return -1;}

	CFile f;
	BOOL Open(LPCSTR szFile) ;
	DWORD GetLength(){return f.GetLength();}
	UINT Read(void* lpBuf, UINT nCount){return f.Read(lpBuf,nCount);} 
};

#endif // !defined(AFX_CZIPREADDEFAULT_H__5B401CBA_CBDE_4ADA_801C_021C3F22F9CC__INCLUDED_)
