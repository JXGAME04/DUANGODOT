#pragma once

#pragma warning (disable: 4786)
#pragma warning (disable: 4503)

#include "iColor.h"
#include "iPublic.h"
#include "stringvalue.h"

class iItem
{
public:
	//可能的搭配： load&save create&save reload&save
	enum FlagEnum{
		FLAG_LOAD	= 0x01, 
		FLAG_RELOAD = 0x02, 
		FLAG_CREATE = 0x04,
		FLAG_SAVE	= 0x08, 
	};

	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual ULONG GetCount() = 0;
	virtual long GetFlag() = 0;

	//bRead = true 只读，可能返回空
	//bRead = false 可写，返回一定不为空
	
	virtual stSection* GetSection(LPCSTR szSection,BOOL bRead = TRUE) = 0;
	virtual stSection* GetMapSection(LPCSTR szSection,BOOL bRead = TRUE) = 0;
	virtual stSection* GetMainSection(BOOL bRead = TRUE) = 0;
	//如果index存在返回index,否则返回main,注意一定不返回null
	virtual stSection* GetIndexSection(unsigned int index,BOOL bRead = TRUE) = 0;
	virtual void AddIndexSection(stSection* p,int index) = 0;
	
	virtual BOOL Load(LPCSTR szFile) = 0;
	virtual BOOL GetValue(LPCSTR szItem,CString& strValue,LPCSTR szSection = "MAIN") = 0;
	virtual BOOL SetValue(LPCSTR szItem,const CString& strValue,LPCSTR szSection = "MAIN") = 0;
	virtual BOOL GetValue(LPCSTR szItem,CString& strValue,unsigned int index) = 0;
	virtual BOOL SetValue(LPCSTR szItem,const CString& strValue,unsigned int index) = 0;
//	virtual BOOL ClearSection(LPCSTR szSection = "MAIN") = 0;;
	virtual BOOL ClearKey(LPCSTR szItem) = 0;
	virtual BOOL ClearAll() = 0;
	virtual BOOL ClearAllButMain() = 0;
//	virtual BOOL Save(LPCSTR szFile) = 0;
	virtual BOOL Save() = 0;
	virtual BOOL SaveModify(LPCSTR szFile) = 0;
	virtual BOOL IsEmpty() = 0;
	virtual BOOL Refresh() = 0;
	virtual LPCSTR GetFileName() = 0;
	virtual BOOL DeleteFile() = 0;
};


_declspec(dllexport) iItem*	Common_CreateItem(LPCSTR szFile, long flag);
_declspec(dllexport) BOOL SetSectionString(stSection* p,LPCSTR sKey, const CString& sValue);
_declspec(dllexport) BOOL GetSectionString(const stSection* p,LPCSTR sKey, CString& sValue);

//在map中使用了静态变量标志结束，所以无法在dll之间使用
template <class T>
BOOL GetSectionValue(const stSection* p,LPCSTR sKey, T& value)
{
	CString strValue;
	if (GetSectionString(p,sKey,strValue))
	{
		StringToValue(strValue,value);
		return TRUE;
	}
	return FALSE;
}

template <class T>
BOOL SetSectionValue(stSection* p,LPCSTR sKey, T& value)
{
	CString strValue;
	ValueToString(value,strValue);
	if (SetSectionString(p,sKey,strValue))
		return TRUE;
	return FALSE;
}

template <class T>
BOOL GetItemValue(iItem* pItem,LPCSTR sKey, T& value)
{
	stSection* p = pItem->GetMainSection();
	if (p == NULL)
		return FALSE;
	return GetSectionValue(p,sKey,value);
}

template <class T>
BOOL SetItemValue(iItem* pItem,LPCSTR sKey, T& value)
{
	stSection* p = pItem->GetMainSection(FALSE);
	if (p == NULL)
		return FALSE;
	return SetSectionValue(p,sKey,value);
}

template <class T>
BOOL GetItemValue(iItem* pItem,LPCSTR sKey, T& value,LPCSTR szSection)
{
	stSection* p = pItem->GetMapSection(szSection);
	BOOL bGet = FALSE;
	if (p != NULL)
		bGet = GetSectionValue(p,sKey,value);
	if (!bGet)
	{
		p = pItem->GetMainSection();
		bGet = GetSectionValue(p,sKey,value);
	}
	return bGet;
}

template <class T>
BOOL SetItemValue(iItem* pItem,LPCSTR sKey, T& value, LPCSTR szSection)
{
	stSection* p = pItem->GetMapSection(szSection,FALSE);
	if (p == NULL)
		return FALSE;
	return SetSectionValue(p,sKey,value);
}

template <class T>
BOOL GetItemValue(iItem* pItem,LPCSTR sKey, T& value,int index)
{
	stSection* p = pItem->GetIndexSection(index);
	BOOL bGet = FALSE;
	if (p != NULL)
		bGet = GetSectionValue(p,sKey,value);
	if (!bGet)
	{
		p = pItem->GetMainSection();
		if (p)
			bGet = GetSectionValue(p,sKey,value);
	}
	return bGet;
}

template <class T>
BOOL SetItemValue(iItem* pItem,LPCSTR sKey, T& value, int index)
{
	stSection* p = pItem->GetIndexSection(index,FALSE);
	if (p == NULL)
		return FALSE;
	return SetSectionValue(p,sKey,value);
}

