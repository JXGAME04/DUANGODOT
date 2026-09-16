// cItem.h: interface for the cItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CITEM_H__0FDE2DD8_0EAA_4CFF_90A9_6DC58637BC90__INCLUDED_)
#define AFX_CITEM_H__0FDE2DD8_0EAA_4CFF_90A9_6DC58637BC90__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iItem.h"
#include <vector>
#include <fstream>

typedef std::map<std::string,std::string> stcSection;
typedef stcSection::iterator itSection;
typedef stcSection::const_iterator c_itSection;

typedef std::map<std::string,stcSection*> SectionMap;
typedef SectionMap::iterator itSectionMap;
typedef std::vector<stcSection*> SectionArray;
typedef SectionArray::iterator itSectionArray;


class cItem : public iItem  
{
public:
	cItem();
	virtual ~cItem();

	ULONG m_ulCount;
	ULONG AddRef() ;
	ULONG Release() ;
	ULONG GetCount(){return m_ulCount;}
	long m_lFlag;
	long GetFlag(){return m_lFlag;}
	void SetFlag(long lFlag);
	CString m_strFile;
	void SetFileName(LPCSTR s){m_strFile = s;}
	LPCSTR GetFileName(){return m_strFile;}
	void GetFileName(CString& strFile){strFile = m_strFile;}
	BOOL IsLoadFile(LPCSTR szFile);
	BOOL DeleteFile() ;

/////////////////////////////////////////////////////////////////////////////
//set and get
/////////////////////////////////////////////////////////////////////////////
	BOOL GetMainValue(LPCSTR szItem,CString& strValue);
	BOOL SetMainValue(LPCSTR szItem,const CString& strValue);
	BOOL GetValue(LPCSTR szItem,CString& strValue,stcSection& st);
	BOOL SetValue(LPCSTR szItem,const CString& strValue,stcSection& st);
	BOOL GetValue(LPCSTR szItem,CString& strValue,unsigned int index);
	BOOL SetValue(LPCSTR szItem,const CString& strValue,unsigned int index);
	BOOL GetValue(LPCSTR szItem,CString& strValue,LPCSTR szSection);
	BOOL SetValue(LPCSTR szItem,const CString& strValue,LPCSTR szSection);

/////////////////////////////////////////////////////////////////////////////
//clear
/////////////////////////////////////////////////////////////////////////////
	BOOL ClearKey(LPCSTR szItem);
	BOOL ClearAll();
	BOOL ClearAllButMain();
/////////////////////////////////////////////////////////////////////////////
//save
/////////////////////////////////////////////////////////////////////////////
	BOOL Save(LPCSTR szFile) ;
	BOOL SaveModify(LPCSTR szFile) ;
	BOOL SaveModify() ;
	BOOL Save() ;
	bool m_bModify;
	BOOL SaveSection(std::ofstream& of, stcSection& section);

/////////////////////////////////////////////////////////////////////////////
//load
/////////////////////////////////////////////////////////////////////////////
	BOOL Load(LPCSTR szFile) ;
	BOOL Reload(LPCSTR szFile) ;
	BOOL Refresh();
//	ItemList m_listItem;
	SectionMap m_mapSection;
	SectionArray m_aSection;
	stcSection* m_pMain;

	stSection* GetSection(LPCSTR szSection,BOOL bRead) ;
	stSection* GetMapSection(LPCSTR szSection,BOOL bRead) ;
	stSection* GetMainSection(BOOL bRead) ;
	stSection* GetIndexSection(unsigned int index,BOOL bRead) ;
	void AddIndexSection(stcSection* p, int index) ;
	void AddIndexSection(stSection* p, int index) {AddIndexSection((stcSection*)p,index);};

	BOOL		IsMain(LPCSTR szSection);

	BOOL IsEmpty(){return m_pMain == NULL && m_mapSection.size() == 0 && m_aSection.size() == 0;}
};

void GetKeyByIndex(CString& strKey, LPCSTR sKey, int index);

#endif // !defined(AFX_CITEM_H__0FDE2DD8_0EAA_4CFF_90A9_6DC58637BC90__INCLUDED_)
