// DirTree.cpp : implementation file
//

#include "stdafx.h"
#include "DirTree.h"
#include "iItem.h"
#include "fileop.h"
#include "..\controls\FileFindEx.h"
#include "cClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CString g_strContent[] =
{
	"tga","bmp","psd","tgb",""
};
/////////////////////////////////////////////////////////////////////////////
// CDirTree

CDirTree::CDirTree()
{
	m_bFolderIsFile = false;
	m_bGetIconByFile = false;
	m_bGetIconByName = false;
	m_bGetIconByContent = false;
	for (int i =0; i<10; i++)
	{
		m_aQuickSave[i] = "null";
	}
}

void CDirTree::CollapseAll()
{
	
}

CDirTree::~CDirTree()
{
	SaveQuickKey();
}


BEGIN_MESSAGE_MAP(CDirTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CDirTree)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDirTree message handlers
void CDirTree::LoadQuickKey(LPCSTR szFile)
{
	m_strQuickKeyFile = szFile;
	if (m_strQuickKeyFile.IsEmpty())
		return;
	CStdioFile file;
	if(!AppOpenFile(file,m_strQuickKeyFile, CFile::modeRead, "dirtree"))
	{
		return;
	}
	for (int i =0; i<10; i++)
	{
		file.ReadString(m_aQuickSave[i]);
	}
}

void CDirTree::SaveQuickKey()
{
	if (m_strQuickKeyFile.IsEmpty())
		return;
	CStdioFile file;
	if(!AppOpenFile(file,m_strQuickKeyFile,CFile::modeCreate | CFile::modeWrite, "dirtree"))
		return;
	for (int i =0; i<10; i++)
	{
		AppGetShortPath(m_aQuickSave[i]);
		file.WriteString(m_aQuickSave[i]);
		file.WriteString("\n");
	}
}

BOOL CDirTree::ConstructIcon(CImageList* rlist)
{
	BOOL b = m_listNormal.Create(rlist);
	ASSERT(b);
	SetImageList(&m_listNormal,TVSIL_NORMAL);
	return b;
}

BOOL CDirTree::Reload()
{
	DeleteAllItems();
	return ConstructTree(m_strRoot,m_strExt);
}

HTREEITEM CDirTree::FindChildFile(HTREEITEM hitem,const CString& strName)
{
	char ch[256];
	TVITEM v;
	v.cchTextMax = 256;
	v.pszText = ch;
	v.mask = TVIF_TEXT;

	HTREEITEM hNextItem;
	HTREEITEM hChildItem = GetChildItem(hitem);

	while (hChildItem != NULL)
	{
		v.hItem = hChildItem;
		GetItem(&v);
		CString s = ch;
		s += "." + m_strExt;
		if (s.CompareNoCase(strName) == 0)
			return hChildItem;
		hNextItem = GetNextItem(hChildItem, TVGN_NEXT);
		hChildItem = hNextItem;
	}
	return NULL;
}

HTREEITEM CDirTree::FindChildFolder(HTREEITEM hitem,const CString& strName)
{
	char ch[256];
	TVITEM v;
	v.cchTextMax = 256;
	v.pszText = ch;
	v.mask = TVIF_TEXT;

	HTREEITEM hNextItem;
	HTREEITEM hChildItem = GetChildItem(hitem);

	while (hChildItem != NULL)
	{
		v.hItem = hChildItem;
		GetItem(&v);
		CString s = ch;
		if (s.CompareNoCase(strName) == 0)
			return hChildItem;
		hNextItem = GetNextItem(hChildItem, TVGN_NEXT);
		hChildItem = hNextItem;
	}
	return NULL;
}

HTREEITEM CDirTree::AddChildFolder(HTREEITEM hitem,const CString& sFolder, LPCSTR szPath)
{
	TVINSERTSTRUCT t;
	t.hInsertAfter = TVI_LAST;
	t.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
	t.item.iImage = 0;
	t.item.iSelectedImage = 1;
	t.item.stateMask = TVIS_STATEIMAGEMASK ;
	t.hParent = hitem;
	static char buffer[256];
	strcpy(buffer,sFolder);
	t.item.pszText = buffer;

	if (szPath != NULL && m_bGetIconByContent == true)
	{
		CString sPath = szPath;
		int n = 0;
		bool bFind = false;
		while(1)
		{
			if (g_strContent[n].IsEmpty())
				break;
			CString sFind = sPath + "\\*." + g_strContent[n++];
			CFileFind f;
			if (f.FindFile(sFind))
			{
				bFind = true;
				break;
			}
		}
		if (bFind)
		{
			t.item.iImage = ICON_FOLDERHAVEFILE;
			t.item.iSelectedImage = ICON_FOLDERHAVEFILEOPEN;
		}
	}
	return InsertItem(&t);
}

void AddHelp(CString& str, const CString& strHelp)
{
	if (!strHelp.IsEmpty())
		str = str + "{" + strHelp + "}"; 
}

void RemoveHelp(CString& str)
{
	int pos = str.Find("{");
	if (pos != -1)
		str = str.Left(pos);
}

void AddExt(CString& str, const CString& strExt)
{
	if (!strExt.IsEmpty())
		str = str + "." + strExt;
}

void RemoveExt(CString& str, const CString& strExt)
{
	int pos = str.Find(strExt);
	if (pos != -1)
	{
		//remove '.' also
		str = str.Left(pos - 1);
	}
}

HTREEITEM CDirTree::AddChildFile(HTREEITEM hitem,const CString& sPathFile)
{
	TVINSERTSTRUCT t;
	t.hInsertAfter = TVI_LAST;
	FillItem(t.item,sPathFile);
	t.hParent = hitem;
	return InsertItem(&t);
}

void CDirTree::FillItem(TVITEM& t,const CString& sPathFile)
{
	CString strHelp;
	GetHelpInfo(sPathFile,strHelp);
	int n = GetImageInfo(sPathFile);
	if (n == -1)
		n = 2;
	CString sPath,sName;
	AppParsePathName(sPathFile,sPath,sName);
	RemoveExt(sName,m_strExt);
	AddHelp(sName,strHelp);
	FillItem(t,sName,n);
}

void CDirTree::FillItem(TVITEM& item,const CString& sName, int iIcon)
{
	static char buffer[256];
	item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
	item.iImage = iIcon;
	item.iSelectedImage = iIcon;
	item.stateMask = TVIS_STATEIMAGEMASK ;
	strcpy(buffer,sName);
	item.pszText = buffer;
}

BOOL CDirTree::SelectNode(const CString& file)
{
	int pos = file.Find(m_strRoot);
	if (pos == -1)
		return FALSE;
	pos += m_strRoot.GetLength()+1;
	HTREEITEM hitem = GetRootItem();
	while(1)
	{
		int pos2 = file.Find('\\',pos);
		if (pos2 == -1)
			break;
		CString sFolder = file.Mid(pos,pos2-pos);

		HTREEITEM hChild = FindChildFolder(hitem,sFolder);
		if (!hChild)
		{
			//create it
			return FALSE;
		}
		Expand(hitem,TVE_EXPAND);
		hitem = hChild;
		pos = pos2+1;
	}

	CString sName = file.Mid(pos);
	HTREEITEM hChild = FindChildFile(hitem,sName);
	if (!hChild)
	{
		return FALSE;
	}
	
	Expand(hitem,TVE_EXPAND);
	hitem = hChild;

//	SetItemState(hitem,TVIS_SELECTED, TVIS_SELECTED);

	if (file.Find('.') != -1)
	{
		m_strFile = file;
		GetParent()->PostMessage(WM_DIRTREE_LEFTSELECTED,(long)(LPCSTR)m_strFile,(long) this);
	}

	return TRUE;
}

BOOL CDirTree::AddNode(const CString& strFile)
{
	CString file = strFile;
	file.MakeLower();
	CString strRoot = m_strRoot;
	strRoot.MakeLower();
	int pos = file.Find(strRoot);
	if (pos == -1)
		return FALSE;
	ASSERT(pos != -1);
	pos += m_strRoot.GetLength()+1;
	HTREEITEM hitem = GetRootItem();
	while(1)
	{
		int pos2 = file.Find('\\',pos);
		if (pos2 == -1)
			break;
		CString sFolder = file.Mid(pos,pos2-pos);

		HTREEITEM hChild = FindChildFolder(hitem,sFolder);
		if (!hChild)
		{
			hChild = AddChildFolder(hitem,sFolder,NULL);
			//SetItemState(hitem,TVIS_EXPANDED, TVIS_EXPANDED);
		}
		Expand(hitem,TVE_EXPAND);
		hitem = hChild;
		pos = pos2+1;
	}

	CString sName = file.Mid(pos);
	HTREEITEM hChild = FindChildFile(hitem,sName);
	if (!hChild)
	{
		if (sName.Find('.') == -1)
		{
			HTREEITEM hChild = FindChildFolder(hitem,sName);
			if (!hChild)
				hChild = AddChildFolder(hitem,sName,file);
		}
		else
			hChild = AddChildFile(hitem,file);
	}
	
	Expand(hitem,TVE_EXPAND);
	hitem = hChild;

//	EnsureVisible(hitem);
	SelectItem(hitem);
	SelectSetFirstVisible(hitem);

//	SetItemState(hitem,TVIS_SELECTED, TVIS_SELECTED);
//	SetItemState(hitem,TVIS_SELECTED, 0);

	if (file.Find('.') != -1)
	{
		m_strFile = file;
		GetParent()->PostMessage(WM_DIRTREE_LEFTSELECTED,(long)(LPCSTR)m_strFile,(long) this);
	}

	return TRUE;
}

BOOL CDirTree::ConstructTree(const CString& sRoot, const CString& sExt)
{
	m_strRoot = sRoot;
	m_strExt = sExt;
	if (m_strRoot.IsEmpty())
		return FALSE;

	CFileFind f;
	if (!f.FindFile(m_strRoot))	
		return FALSE;
	int pos = m_strRoot.ReverseFind('\\');
	if (pos == -1)
		return FALSE;
	CString strName = m_strRoot.Right(m_strRoot.GetLength() - pos - 1);
	if (strName.IsEmpty())
		return FALSE;

	TVINSERTSTRUCT t;
	t.hInsertAfter = TVI_LAST;
	t.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	t.item.iImage = 0;
	t.item.iSelectedImage = 1;
	t.hParent = TVI_ROOT;
	static char buffer[256];
	strcpy(buffer,strName);
	t.item.pszText = buffer;

	HTREEITEM root = InsertItem(&t);
	InsertFolder(root,m_strRoot,0);

	Expand(root,TVE_EXPAND);
	return TRUE;
}

void CDirTree::GetHelpInfo(const CString& strName, CString& strHelp)
{
	if (m_bGetIconByFile)
	{
		iItem* pItem = Common_CreateItem(strName,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		BOOL b = GetItemValue(pItem,"help",strHelp);
	}
}

int CDirTree::GetImageInfo(const CString& strName)
{
	if (m_bGetIconByFile)
	{
		iItem* pItem = Common_CreateItem(strName,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		CString sValue;
		BOOL b = GetItemValue(pItem,"icon",sValue);
		if (!b)
		{
			CString strErr;
			strErr = "file " + strName + " format is not valid, not found item 'icon=' ";
			AfxMessageBox(strErr);
			return ICON_NORMAL;
		}
 		ASSERT(b);
		GetItemValue(pItem,"icon",sValue);
		eIconType e = IconNameToID(sValue);
		return e;
	}
	else if (m_bGetIconByName)
	{
		ASSERT(0);
	}

	return -1;
}

void CDirTree::InsertFile(const HTREEITEM father, const CString& strPath)
{
	if (m_strExt.IsEmpty())
		return;

	CString strFind = strPath + "\\*." + m_strExt;

	CFileFindEx f;
	if (!f.FindFile(strFind))
	{
		return;
	}
	int bFind;
	do
	{
		bFind = f.FindNextFile();
		CString strPathName = f.GetFilePath();
		AddChildFile(father,strPathName);
	}
	while(bFind);
}

void CDirTree::InsertFolder(const HTREEITEM father, const CString& strPath, int layer)
{
	InsertFile(father,strPath);

	CString strFind = strPath + "\\*.";

	CFileFindEx f;
	f.SetOnlyFile(false);
	f.SetOnlyDirectory(true);

	if (!f.FindFile(strFind))
	{
		return;
	}
	BOOL bFind;
	bool bAddSubFolder = false;
	do
	{
		bFind = f.FindNextFile();
		CString strName = f.GetFileName();
		if (strName == "." || strName == "..")
			continue;

		if (layer == 0)
		{
			if (!m_listLoadFolder.IsEmpty() && m_listLoadFolder.Find(strName) == NULL )
				continue;
		}

		HTREEITEM root = AddChildFolder(father,strName,f.GetFilePath());
//		InsertFolder(root,f.GetFilePath(),layer+1);
		bAddSubFolder = true;
	}
	while(bFind);
	if (!bAddSubFolder && m_bFolderIsFile)
	{
		SetItemImage(father,2,2);
	}
}

void CDirTree::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

bool CDirTree::IsItemFolder(HTREEITEM h)
{
	int nImage,nSelectImage;
	GetItemImage(h,nImage,nSelectImage);
	bool bFolder = false;
	if (nImage == ICON_FOLDER || nImage == ICON_FOLDERHAVEFILE)
	{
//		ASSERT(nSelectImage == ICON_FOLDEROPEN);
		bFolder = true;
	}
	return bFolder;
}

CString CDirTree::GetSelectedFile()
{
	HTREEITEM h = GetSelectedItem();

	bool bFolder = IsItemFolder(h);

	HTREEITEM root = GetRootItem();
	if (h == root)
		return m_strRoot;

	char ch[256];
	TVITEM v;
	v.cchTextMax = 256;
	v.pszText = ch;
	v.mask = TVIF_TEXT;

	v.hItem = h;
	GetItem(&v);
	CString s = ch;
	
	do 
	{
		h = GetParentItem(h);
		if (h == root)
			break;
		v.hItem = h;
		GetItem(&v);
		CString sfather = ch;
		s = sfather + "\\" + s;
	}
	while( h!=root);

	if (!bFolder)
	{
		RemoveHelp(s);
		AddExt(s,m_strExt);
	}

	return m_strRoot + "\\" + s;
}

void CDirTree::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CDirTree::ClearChildItem(HTREEITEM hmyItem)
{
	if (ItemHasChildren(hmyItem))
	{
	   HTREEITEM hNextItem;
	   HTREEITEM hChildItem = GetChildItem(hmyItem);

	   while (hChildItem != NULL)
	   {
		  hNextItem = GetNextItem(hChildItem, TVGN_NEXT);
		  DeleteItem(hChildItem);
		  hChildItem = hNextItem;
	   }
	}	
}

void CDirTree::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	//展开该节点
	HTREEITEM hItem = HitTest(point, &nFlags);
	if (hItem != NULL && hItem != GetRootItem() && hItem == GetSelectedItem())
	{
		m_strFile = GetSelectedFile();
		if (m_strFile.Find(".") == -1) 
		{
			ClearChildItem(hItem);
			InsertFolder(hItem,m_strFile,1);
		}
	}
	CTreeCtrl::OnLButtonDblClk(nFlags, point);
}

void CDirTree::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTreeCtrl::OnLButtonDown(nFlags, point);

	HTREEITEM hItem = HitTest(point, &nFlags);
	if (hItem == NULL)
		return;
	HTREEITEM h = GetSelectedItem();
	if (h == hItem)
	{
		m_strFile = GetSelectedFile();
		GetParent()->PostMessage(WM_DIRTREE_LEFTSELECTED,(long)(LPCSTR)m_strFile,(long) this);
	}
}

void CDirTree::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CTreeCtrl::OnRButtonDown(nFlags, point);
	
	HTREEITEM hItem = HitTest(point, &nFlags);
	if (hItem == NULL)
		return;
	HTREEITEM h = GetSelectedItem();
	if (h == hItem)
	{
		m_strFile = GetSelectedFile();
		GetParent()->PostMessage(WM_DIRTREE_RIGHTSELECTED,(long)(LPCSTR)m_strFile,(long) this);
	}
}

void CDirTree::DeleteSelect()
{
	//delete the file, 
	HTREEITEM h = GetSelectedItem();
	if (h == NULL)
		return;
	if (h == GetRootItem() || GetParentItem(h) == GetRootItem() || GetParentItem(GetParentItem(h)) == GetRootItem())
	{
		AfxMessageBox("不能删除这一级目录");
		return;
	}

	m_strFile = GetSelectedFile();
	if (m_strFile.Find(".") == -1)
	{
//		if (GetChildItem(h) != NULL)
		{
			AfxMessageBox("不能删除目录");
			return;
		}
	}

	CString strExt = m_strFile.Right(4);
	strExt.MakeLower();
	if (strExt == ".txt")//if it is a txt file and asscial with a spr, delete the spr also!
	{
		iItem* pItem = Common_CreateItem(m_strFile,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		CString strFile;
		if (GetItemValue(pItem,"file",strFile))
		{
//			CString strMsg;
//			strMsg = "有一个文件'" + strFile +"'与之关联，一起删除么？";
//			if (AfxMessageBox(strMsg) == IDOK)
			{
				strFile = m_strFile;
				strFile = strFile.Left(strFile.GetLength() - 4) + ".spr";
				AppGetFullPath(strFile);
				AppDeleteFolder(strFile);		
			}
		}
		pItem->ClearAll();
		DeleteItem(h);		
		AppDeleteFolder(m_strFile);	
	}
	else
	{
		DeleteItem(h);
		AppDeleteFolder(m_strFile);
	}
}

bool CDirTree::IsKeyDown(UINT vk)
{
	short state = GetKeyState(vk);
	return (HIBYTE(state)!=0 );
}

bool CDirTree::IsShiftDown()
{
	return IsKeyDown(VK_LSHIFT) || IsKeyDown(VK_RSHIFT);
}

bool CDirTree::IsCtrlDown()
{
	return IsKeyDown(VK_LCONTROL) || IsKeyDown(VK_RCONTROL);
}

bool CDirTree::IsAltDown()
{
	return IsKeyDown(VK_LMENU) || IsKeyDown(VK_RMENU);
}

void CDirTree::Select(int i)
{
	if (i < 0 || i > 9)
		return ;
	CString strFile = m_aQuickSave[i];
	if (strFile.IsEmpty() || strFile == "null")
		return;
	AppGetFullPath(strFile);
	AddNode(strFile);
}

void CDirTree::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	case VK_F5:
		Reload();
		break;
	case VK_DELETE:
		DeleteSelect();
		break;
	case 'L':
		if (IsCtrlDown())
		{	
			CString str;
			Clipboard_Paste(str,this->GetSafeHwnd());
			str.TrimLeft();
			str.TrimRight();
			if (!str.IsEmpty())
			{
				AppGetFullPath(str);
				AddNode(str);
			}
		}
		break;
	case 'K':
		if (IsCtrlDown())
		{
			CString strFile = GetSelectedFile();
			if (!strFile.IsEmpty())
			{
				Clipboard_Copy(strFile,GetSafeHwnd());
			}
		}
	}

	if (nChar >= '0' && nChar <= '9')
	{
		int index = nChar - '0';
		if (IsCtrlDown())	
		{
			//save
			CString strFile = GetSelectedFile();
			if (!strFile.IsEmpty())
			{
				m_aQuickSave[index] = strFile;
				//及时的存盘
				SaveQuickKey();
			}
		}
		else
		{
			Select(index);
		}
	}
	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

/////////////////////////////////////////////////////////////////////////////
//call back
/////////////////////////////////////////////////////////////////////////////
void CDirTree::UpdateSelectedNode()
{
	HTREEITEM h = GetSelectedItem();
	if (h == NULL)
		return;
	int nImage,nSelectImage;
	GetItemImage(h,nImage,nSelectImage);
	if (IsItemFolder(h))
		return;

	CString s = GetSelectedFile();
	TVITEM item;
	item.hItem = h;
	FillItem(item,s);
	SetItem(&item);
}

/////////////////////////////////////////////////////////////////////////////
//utility

eIconType IconNameToID(const CString& s)
{
	static CString s_strIconName[] =
	{
		"folder",
		"folderopen",
		"normal",
		"npc",
		"box",
		"trap",
		"camera",
		"light",
		"terra",
		"diamond",
		"road",
		"doodad",
		"doodad_complex",
		"group",
		"obstacle",
		"special",
		"setting",
		"command",
		"template",

		"",
		"",
	};
	return (eIconType)NameToID(s_strIconName,s);
}


