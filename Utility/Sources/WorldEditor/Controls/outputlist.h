#if !defined(AFX_OUTPUTLIST_H__0CF860E4_A2E6_4918_8617_C1AD6B8D1DCB__INCLUDED_)
#define AFX_OUTPUTLIST_H__0CF860E4_A2E6_4918_8617_C1AD6B8D1DCB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// outputlist.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COutputList window
#include "iitem.h"

struct stOutputItem
{
	eOutputType e;
	char	msg[128];
};

typedef std::list<stOutputItem> OutputItemList;
typedef OutputItemList::iterator itOutputItem;

class COutputList : public CListCtrl
{
// Construction
public:
	COutputList();

// Attributes
public:
	OutputItemList m_listOutputItem;
	enum {MAX_ITEM = 32,};

	bool m_bCreate;
	BOOL Create();
	void Message(int nStatus, LPCSTR szMsg);
protected:
	void AddItem(stOutputItem& item);
	
	void MessageItem(const stOutputItem& item);
	BOOL FindNearItem(const stOutputItem& item);

	void RemoveItem();
public:
	CImageList m_listNormal;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COutputList)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COutputList();

	// Generated message map functions
protected:
	//{{AFX_MSG(COutputList)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTPUTLIST_H__0CF860E4_A2E6_4918_8617_C1AD6B8D1DCB__INCLUDED_)
