// cPack1D.h: interface for the cPack1D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPACK1D_H__8625AE45_8561_41C7_ADF0_FDBCF1AB82BC__INCLUDED_)
#define AFX_CPACK1D_H__8625AE45_8561_41C7_ADF0_FDBCF1AB82BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <hash_map>
#include <string>

typedef std::string fileString;

typedef std::hash_map<fileString,char*,std::hash<fileString>, std::equal_to<fileString> > TemplateMap;
typedef TemplateMap::iterator itTemplateMap;

typedef std::hash_map<fileString, int, std::hash<fileString>, std::equal_to<fileString> > StringMap;
typedef StringMap::iterator itStringMap;

#include "iPack1D.h"

class cPack1D  : public iPack1D
{
public:
	cPack1D();
	virtual ~cPack1D();

	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; };
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;};

	int GetIndexLen(){return m_nNum;}

	BOOL Open(LPCSTR szZip,LPCSTR szIndex,iZipRead* pZip);
	void* Locate(int index) ;
	void* Locate(LPCSTR szFile);

	int m_nNum;
	long* m_pIndex;
	
	char* m_pBuffer;
	
	int m_nOffset;
	long* m_pOffset;

	void SetConvertNum(int n);
	BOOL Save(LPCSTR szTemplate);
	BOOL Convert(void* buffer, ULONG len);

//index of the template
	int GetIndex(LPCSTR szTxt);
	StringMap m_mapString;

	void Refresh(LPCSTR szTemplate);
	
	TemplateMap m_mapTemplate;
	void* OpenTemplate(LPCSTR szFile);
	BOOL Convert(LPCSTR szTxt) ;
	
	virtual BOOL LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size) = 0;
	void* LocateToFrame(LPCSTR szFile,int frame) ;
	void* LocateTo(LPCSTR szFile);
	virtual void* Locate(void* pStart,int frame) {return NULL;};
};

#endif // !defined(AFX_CPACK1D_H__8625AE45_8561_41C7_ADF0_FDBCF1AB82BC__INCLUDED_)
