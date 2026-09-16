// cZipRead.h: interface for the cZipRead class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CZIPREAD_H__8BEB3149_F41F_4919_BEB1_03C3138F04CD__INCLUDED_)
#define AFX_CZIPREAD_H__8BEB3149_F41F_4919_BEB1_03C3138F04CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iZip.h"
#include "unzipper.h"

class cZipRead : public iZipRead  
{
public:
	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; };
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;};
	cZipRead();
	virtual ~cZipRead();
	
	CUnzipper m_unzipper;

	bool load(LPCSTR szZipFile) ;		//load a .zip file from disk

	bool locate(LPCSTR szFile) ;		//locate a file then can read
	int	length() ;						//length of the file located
	int	read(void* buf, int* bufsize) ;

	BOOL Open(LPCSTR szFile) ;
	DWORD GetLength();
	UINT Read(void* lpBuf, UINT nCount) ;

	char* m_pBuffer;
	char* m_pPos;
	int m_nLength;
	int m_nBufferLength;
};

#endif // !defined(AFX_CZIPREAD_H__8BEB3149_F41F_4919_BEB1_03C3138F04CD__INCLUDED_)
