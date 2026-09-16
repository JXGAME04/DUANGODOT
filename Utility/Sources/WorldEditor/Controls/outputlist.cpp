// outputlist.cpp : implementation file
//

#include "stdafx.h"
#include "..\worldeditor.h"
#include "outputlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputList

COutputList::COutputList()
{
	m_bCreate = false;
}

COutputList::~COutputList()
{
}


BEGIN_MESSAGE_MAP(COutputList, CListCtrl)
	//{{AFX_MSG_MAP(COutputList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers
CString g_aStatusString[] = 
{
	"OK",
	"Succeed",
	"Failure",
	"Warnning",
	"Error",
	"Hint",
	"Help",
	"Chat",
	"Next",
};

void COutputList::AddItem(stOutputItem& item)
{
	m_listOutputItem.push_back(item);
}

BOOL COutputList::FindNearItem(const stOutputItem& output)
{
	int n = GetItemCount();
	LVITEM item;
	for (int i=n-1; i>=max(0,n-10); i--)
	{
		char buffer[128];
		item.iItem = i;
		item.iSubItem = 0;
		item.mask = LVIF_TEXT ;
		item.cchTextMax = 128;
		item.pszText = buffer;
		BOOL b;
		b = GetItem(&item);
		ASSERT(b);
		if (g_aStatusString[output.e] != item.pszText)
			continue;
		item.iSubItem = 1;
		b = GetItem(&item);
		ASSERT(b);
		CString s = output.msg;
		if (s == item.pszText)
			return TRUE;
	}
	return FALSE;
}

void COutputList::MessageItem(const stOutputItem& output)
{
//	if (FindNearItem(output))
//		return;

	int n = GetItemCount();
	
	LVITEM item;
	item.iItem = 0;
	item.iSubItem = 0;
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	char buffer[128];
	strcpy(buffer,g_aStatusString[output.e]);
	item.pszText = buffer;
	item.iImage = output.e;
	item.state = LVIS_SELECTED | LVIS_FOCUSED;
	int iA;
	iA = InsertItem(&item);	
	//SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ASSERT(iA != - 1);
	strcpy(buffer,output.msg);
	item.iItem = iA;
	item.iSubItem = 1;
	item.mask = LVIF_TEXT;
	BOOL b = SetItem(&item);	
	ASSERT(b != 0);
		
	if (iA >= MAX_ITEM)
		RemoveItem();
}

void COutputList::Message(int nStatus, LPCSTR szMsg)
{
	stOutputItem item;
	item.e = (eOutputType)nStatus;
	strcpy(item.msg,szMsg);
	if (!m_bCreate)
	{
		AddItem(item);
		return;
	}
	else
		MessageItem(item);
}

void COutputList::RemoveItem()
{
	int n = GetItemCount();
	VERIFY(DeleteItem(n-1));
}

BOOL COutputList::Create() 
{
	BOOL b = m_listNormal.Create(16,16,ILC_COLOR8,OUTPUT_NUM,OUTPUT_NUM);
	HICON icon;
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_OK);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_SUCCEED);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_FORBID);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_WARNNING);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_ERROR);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_HINT);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_HELP);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_CHAT);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_NEXT);
	m_listNormal.Add(icon);
	icon = AfxGetApp()->LoadIcon(IDI_OUTPUT_NEW);
	m_listNormal.Add(icon);
	SetImageList(&m_listNormal,LVSIL_SMALL);
/*
	m_listNormal.Add(hiconFolder);
	m_listNormal.Add(hiconFolderOpen);
	m_listNormal.Add(hFile);
*/
	
	CString strItem1= _T("Status");
	CString strItem2= _T("Message");
	
	int statusWidth = 80;
	CRect rect;
	GetWindowRect(&rect);
	int rt = InsertColumn(0, strItem1, LVCFMT_LEFT,
		statusWidth,0);
	ASSERT(rt!=-1);
	rt = InsertColumn(1, strItem2, LVCFMT_LEFT,
		rect.Width() - statusWidth - 10,1);
	ASSERT(rt!=-1);

	for (itOutputItem it = m_listOutputItem.begin(); it!= m_listOutputItem.end(); ++it)
		MessageItem(*it);
	m_listOutputItem.clear();
	
	m_bCreate = true;

	return TRUE;  
}
