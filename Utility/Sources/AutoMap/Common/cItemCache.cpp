// cItemCache.cpp: implementation of the cItemCache class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cItemCache.h"

#include "cItem.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cItemCache::cItemCache()
{
	InitializeCriticalSection(&m_cs);
}

cItemCache::~cItemCache()
{
	for (itcItem it = m_list.begin(); it != m_list.end(); ++it)
	{
		(*it)->SaveModify();
		(*it)->Release();
	}
	DeleteCriticalSection(&m_cs);
}

#include "clock.h"

cItem* cItemCache::CreateItem(LPCSTR strFile, long flag)
{
	ASSERT(strlen(strFile) != 0);

	CString szFile = strFile;
	AppGetFullPath(szFile);
	cLock lock(&m_cs);
	cItem* p = FindItem(szFile);
	lock.Unlock();
	if (p != NULL)
	{
		if (flag & iItem::FLAG_CREATE)
			p->ClearAll();
		else if (flag & iItem::FLAG_RELOAD)
			p->Refresh();
		p->SetFlag(flag);
		return p;
	}

	//如果文件存在
	
	p = new cItem;
	p->SetFlag(flag);
	if (flag & iItem::FLAG_LOAD || flag & iItem::FLAG_RELOAD)
	{
		if (!p->Load(szFile))
		{
			delete p;
			return NULL;
		}
	}
	else
	{
		p->SetFileName(szFile);
	}
	//else
	cLock lock2(&m_cs);
	p->AddRef();
	m_list.push_front(p);
	FreeItem();
	lock2.Unlock();

	return p;
}

cItem* cItemCache::FindItem(LPCSTR szFile)
{
	for (itcItem it = m_list.begin(); it != m_list.end(); ++it)
	{
		cItem* p = (*it);
		if (p->IsLoadFile(szFile))
		{
			m_list.erase(it);
			m_list.push_front(p);
			p->AddRef();
			return p;
		}
	}
	return NULL;
}

BOOL cItemCache::FreeItem()
{
	if (m_list.size() < MAX_ITEM)
		return FALSE;
	BOOL bErase = false;
	for (cItemList::reverse_iterator it = m_list.rbegin(); it!= m_list.rend(); ++it)
	{
		cItem* p = (*it);
		if (p->GetCount() == 1)
		{
			bErase = true;
			m_list.remove(p);
			p->SaveModify();
			p->Release();
		}
	}
	return bErase;
}

void cItemCache::RemoveItem(cItem* p)
{
	ASSERT(p);
	m_list.remove(p);
	p->Release();
}

#include "common.h"
_declspec(dllexport) iItem*	Common_CreateItem(LPCSTR szFile,long l)
{
	static cItemCache cache;
//	cItemCache& cache = *(theApp.m_pCache);
	if (strlen(szFile) == 0)
	{
		AfxMessageBox("itemerror: 无法创建一个空的文件");
		return NULL;
	}
	return cache.CreateItem(szFile,l);
}

