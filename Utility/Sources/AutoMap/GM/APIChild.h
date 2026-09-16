//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	APIChild.h
//	创建人			:	王西贝
//	创建时间		:	2003-7-21 15:49:41
//	文件说明		:	.................
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __APICHILD_H__
#define __APICHILD_H__

class KAPIParent;
class KAPIChild
{
public:
	KAPIChild()
	{
		m_pParentClass = NULL;
		m_pChildName = NULL;
	}

	virtual ~KAPIChild();

protected:
	KAPIParent* m_pParentClass;
	char* m_pChildName;

public:
	BOOL SetParentAPI(KAPIParent* pParent);
	virtual LPCSTR GetChildName() = 0;
	LPCSTR GetSafeChildName()
	{
		if (m_pChildName)
			return m_pChildName;
		LPCSTR lpszName = GetChildName();
		m_pChildName = new char[strlen(lpszName) + 1];
		strcpy(m_pChildName, lpszName);
		return m_pChildName;
	}
	void ClearParent() { m_pParentClass = NULL; }
};

class KAPIParent
{
public:
	virtual ~KAPIParent()
	{
		CString strTmp;
		KAPIChild* pChild = NULL;
		for (POSITION pos = mapChild.GetStartPosition(); pos;)
		{
			mapChild.GetNextAssoc(pos, strTmp, (void*&)pChild);
			if (pChild)
				pChild->ClearParent();
		}
	}
protected:
	CMapStringToPtr mapChild;
public:
	virtual BOOL LookingForChild(LPCSTR strName, void** ppv)
	{
		return mapChild.Lookup(strName, *ppv);
	}
	virtual BOOL AddChild(KAPIChild* pChild)
	{
		void* p = NULL;
		if (mapChild.Lookup(pChild->GetSafeChildName(), p))
			return FALSE;
		mapChild.SetAt(pChild->GetSafeChildName(), (void*)pChild);
		return TRUE;
	}

	virtual BOOL RemoveChild(KAPIChild* pChild)
	{
		return mapChild.RemoveKey(pChild->GetSafeChildName());
	}
};

inline BOOL KAPIChild::SetParentAPI(KAPIParent* pParent)
{
	if (pParent)
	{
		m_pParentClass = pParent;
		return pParent->AddChild(this);
	}
	return FALSE;
}

inline KAPIChild::~KAPIChild()
{
	if (m_pParentClass)
	{
		VERIFY(m_pParentClass->RemoveChild(this));
		m_pParentClass = NULL;
	}
	if (m_pChildName)
	{
		delete m_pChildName;
		m_pChildName = NULL;
	}
}

#endif //__APICHILD_H__
