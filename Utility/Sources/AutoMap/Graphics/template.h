#pragma once

#pragma warning (disable: 4786)
#pragma warning (disable: 4503)

#include <list>
#include <vector>
#include <iostream>
#include <functional>
#include <iterator>
#include <algorithm>

#include "iItem.h"
#include "fileop.h"

using namespace std;

template <class Container,class T>
HRESULT T_LoadPackList(Container& plist, cGraphics* pGraphics, void* pOffset)
{
	Container ilist;
	T* p;
	int n;
	HRESULT hr;

	int* pLong = (int*) pOffset;
	n = *pLong++;
	stObjParam* pParam = (stObjParam*)pLong;
	for (int i=0; i<n; i++)
	{
		pGraphics->CreateGraphicsElement(pParam->eElement,&p);
		hr = p->LoadPack((void**)&pParam);
		if (FAILED(hr))
			p->Release();
		else 
			ilist.push_back(p);
	}
	plist.splice(plist.end(),ilist);
	return hr;	
}

template <class Container,class T>
HRESULT T_LoadList(Container& ilist, LPCSTR file,cGraphics* pGraphics)
{
//	HRESULT hr = T_LoadBinList<Container,T>(ilist,file,pGraphics);
//	if (!FAILED(hr))
//		return hr;
	
	CFileFind find;
	if (!find.FindFile(file))
		return S_OK;

	iItem* pItem = Common_CreateItem(file,iItem::FLAG_LOAD);	
	if (pItem == NULL)
		return S_OK;

	wrap_ptr<iItem> ptr(pItem);

	int n;
	if (!GetItemValue(pItem,"elementnum",n))
		return S_OK;

	for (int i=0; i<n; i++)
	{
		T* p;
		int type;
		stSection* pSection = pItem->GetIndexSection(i);
		ASSERT(pSection);
		VERIFY(GetSectionValue(pSection,"elementtype",type));
		pGraphics->CreateGraphicsElement((ElementEnum)type,&p);
		HRESULT hr = p->Load(pSection);
		if (FAILED(hr))
		{
			delete p;
		}
		else
		{
			ilist.push_back(p);
		}		
	}
	return S_OK;	
}

template <class Container,class T>
HRESULT T_LoadPlaneList(Container& ilist, LPCSTR file,cGraphics* pGraphics)
{
//	HRESULT hr = T_LoadBinList<Container,T>(ilist,file,pGraphics);
//	if (!FAILED(hr))
//		return hr;

	CFileFind find;
	if (!find.FindFile(file))
		return S_OK;

	iItem* pItem = Common_CreateItem(file,iItem::FLAG_LOAD);	
	if (pItem == NULL)
		return S_OK;

	wrap_ptr<iItem> ptr(pItem);

	int n;
	if (!GetItemValue(pItem,"elementnum",n))
		return S_OK;

	BOOL bNeedUpdate = FALSE;
	for (int i=0; i<n; i++)
	{
		T* p;
		int type;
		stSection* pSection = pItem->GetIndexSection(i);
		if (pSection == NULL)
			continue; //存档文件损坏
		ASSERT(pSection);
		VERIFY(GetSectionValue(pSection,"elementtype",type));
		pGraphics->CreateGraphicsElement((ElementEnum)type,&p);
		HRESULT hr = p->Load(pSection);
		if (p->GetElementByType() != p->GetElementType() )//修改了元素的类型
		{
			ElementEnum e = p->GetElementByType();
			T* pNew = NULL;
			pGraphics->CreateGraphicsElement(e,&pNew);
			ASSERT(pNew);
			hr = pNew->Load(pSection);
			p->Release();
			p = pNew;
			bNeedUpdate = TRUE;
		}
		if (FAILED(hr))
		{
			delete p;
		}
		else
		{
			ilist.push_back(p);
		}		
	}
	if (bNeedUpdate)
		return 1;
	return S_OK;	
}

template <class Container>
HRESULT T_SaveList(Container& ilist, LPCSTR file)
{
	int i = 0;
	int n = ilist.size();

	iItem* pItem = Common_CreateItem(file,iItem::FLAG_CREATE);	
	wrap_ptr<iItem> ptr(pItem);

	if (n == 0)
		pItem->DeleteFile();
	else
	{
		SetItemValue(pItem,"elementnum",n);
		for (Container::iterator it = ilist.begin(); it!= ilist.end(); ++it)
		{
			stSection* pSection = pItem->GetIndexSection(i,FALSE);
			(*it)->Save(pSection);
			i++;
		}
		pItem->Save();
	}
/*
	CString strBinaryFile = file;
	strBinaryFile = strBinaryFile.Left(strBinaryFile.GetLength() - 3) + "bin";
	CFile f;
	if (n == 0)
		AppDeleteFolder(strBinaryFile);
	else if (f.Open(strBinaryFile,CFile::modeCreate | CFile::modeWrite))
	{
		f.Write(&n,sizeof(n));
		for (Container::iterator it = ilist.begin(); it!= ilist.end(); ++it)
		{
			int type = (*it)->GetElementType();
			f.Write(&type,sizeof(type));
			(*it)->SaveBinary(f);
		}
	}
*/
	return S_OK;
}


template <class cContainer,class iContainer>
HRESULT T_GetAtMin(CPoint pt, cContainer& listSrc,  iContainer& listDes)
{
	int y = INT_MIN;
	cContainer::iterator itGet = listSrc.end();
	for (cContainer::iterator it = listSrc.begin(); it!= listSrc.end(); ++it)
	{
		if ((*it)->Contain(pt))
		{
			if ((*it)->GetOffset().y > y)
			{
				itGet = it;
				y = (*it)->GetOffset().y;
			}
		}
	}
	if (itGet != listSrc.end())
	{
		(*itGet)->AddRef();
		listDes.push_back(*itGet);
	}
	return S_OK;
}

template <class cContainer,class iContainer>
HRESULT T_GetAt(CPoint pt, cContainer& listSrc,  iContainer& listDes)
{
	for (cContainer::iterator it = listSrc.begin(); it!= listSrc.end(); ++it)
	{
		if ((*it)->Contain(pt))
		{
			(*it)->AddRef();
			listDes.push_back(*it);
		}
	}
	return S_OK;
}

template <class cContainer,class iContainer>
HRESULT T_GetAt(int nID, cContainer& listSrc,  iContainer& listDes)
{
	ASSERT(nID);
	for (cContainer::iterator it = listSrc.begin(); it!= listSrc.end(); ++it)
	{
		if ((*it)->GetGroupID() == nID)
		{
			(*it)->AddRef();
			listDes.push_back(*it);
		}
	}
	return S_OK;
}

template <class cContainer>
BOOL T_Distribute(cContainer& ilist,cGround* pGround)
{
	BOOL bDistribute = TRUE;
	for (cContainer::iterator it = ilist.begin(); it!= ilist.end(); ++it)
	{
		cPoint pt = (*it)->GetOffset();
		cRegion* pRegion = pGround->GetcRegion(pt);
		if (pRegion != NULL)
			pRegion->AddObj(*it);
		else
			bDistribute = FALSE;
	}
	return bDistribute;
}

template<class _II, class _OI> inline
void insert(_II _F, _II _L, _OI& _X)
{
	for (; _F != _L; ++_F)
		_X.push_back(*_F);
}

template<class _II, class _OI, class _Pr> inline
void insert_if(_II _F, _II _L, _OI& _X, _Pr _P)
{
	for (; _F != _L; ++_F)
		if (_P(*_F))
	_X.push_back(*_F);
}

template <class T>
BOOL T_AddToList(list<T*>& ilist, T* p)
{
	if (p->IsInGround() || p->IsChild())
		return FALSE;
	if (find(ilist.begin(),ilist.end(),p) != ilist.end())
		return FALSE;
	p->AddRef();
	ilist.push_back(p);
	p->SetInGround(true);	
	return TRUE;
}

template <class T>
BOOL T_RemoveFromList(list<T*>& ilist, T* p)
{
	if (!p->IsInGround() || p->IsChild())
		return FALSE;
	int size = ilist.size();
	ilist.remove(p);
	if (size != ilist.size()+1)
	{
		ASSERT(0);
//		return FALSE;
	}
	p->Release();
	p->SetInGround(false);
	return TRUE;	
}

template <class T>
BOOL T_RemovePlaneFromList(list<T*>& ilist, T* p)
{
	int size = ilist.size();
	ilist.remove(p);
	if (size == ilist.size() + 1)
		return TRUE;
	return FALSE;
}

template <class T>
BOOL T_AddPlaneToList(list<T*>& ilist, T* p)
{
	//hor已经在场景中了
	if (find(ilist.begin(),ilist.end(),p) != ilist.end())
		return FALSE;
	ilist.push_back(p);	
	return TRUE;
}

template <class T>
BOOL T_AddPlaneToListByLayer(list<T*>& ilist, T* p)
{
	if (find(ilist.begin(),ilist.end(),p) != ilist.end())
		return FALSE;

	bool bInsert = false;
	for (list<T*>::iterator it = ilist.begin(); it != ilist.end(); ++it)
	{
		if (p->GetLayer() < (*it)->GetLayer())
		{
			bInsert = true;
			ilist.insert(it,p);
			break;
		}
		if (p->GetLayer() == (*it)->GetLayer())
		{
			if (p->GetBottomPoint().y < (*it)->GetBottomPoint().y)
			{
				bInsert = true;
				ilist.insert(it,p);
				break;
			}
		}
	}
	if (!bInsert)
		ilist.push_back(p);	

	return TRUE;
}
