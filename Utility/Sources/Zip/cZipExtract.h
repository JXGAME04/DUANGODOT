// cZipExtract.h: interface for the cZipExtract class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CZIPEXTRACT_H__8B662929_B93A_4E91_8D82_B84690DFB029__INCLUDED_)
#define AFX_CZIPEXTRACT_H__8B662929_B93A_4E91_8D82_B84690DFB029__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iZip.h"

#include "unzipper.h"

class cZipExtract : public iZipExtract  
{
public:
	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; };
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;};
	cZipExtract():m_ulCount(1){};
	virtual ~cZipExtract(){};
	
	CUnzipper m_unzipper;
	
	bool load(LPCSTR szZipFile) ;		//load a .zip file from disk
	bool load_buffer(char* buffer);		//load a .zip file from buffer
	bool extract(LPCSTR szFolder) ;		//extrace to a folder like d:\test
	DWORD get_uncompressedsize();
};

#endif // !defined(AFX_CZIPEXTRACT_H__8B662929_B93A_4E91_8D82_B84690DFB029__INCLUDED_)
