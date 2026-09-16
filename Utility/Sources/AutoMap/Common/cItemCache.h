// cItemCache.h: interface for the cItemCache class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CITEMCACHE_H__7389DB9C_027F_4022_AD31_CA052851B343__INCLUDED_)
#define AFX_CITEMCACHE_H__7389DB9C_027F_4022_AD31_CA052851B343__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>

class cItem;
typedef std::list<cItem*> cItemList;
typedef cItemList::iterator itcItem;

#include "iItem.h"

class cItemCache  
{
public:
	CRITICAL_SECTION m_cs;
	cItemCache();
	virtual ~cItemCache();
	cItemList m_list;
	cItem*	CreateItem(LPCSTR szFileb, long flag);
	void RemoveItem(cItem* p);
	enum {MAX_ITEM = 32};
protected:
	cItem*	FindItem(LPCSTR szFile);
	BOOL FreeItem();
};

#endif // !defined(AFX_CITEMCACHE_H__7389DB9C_027F_4022_AD31_CA052851B343__INCLUDED_)
