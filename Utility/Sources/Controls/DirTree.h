#if !defined(AFX_DirTree_H__E9AE50E5_32E3_45CB_9A23_02F8F4BF5D2F__INCLUDED_)
#define AFX_DirTree_H__E9AE50E5_32E3_45CB_9A23_02F8F4BF5D2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DirTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDirTree window

//find a folder start at a give directionary
#define WM_DIRTREE_LEFTSELECTED WM_USER+1000
#define WM_DIRTREE_RIGHTSELECTED WM_DIRTREE_LEFTSELECTED+1

enum eIconType
{
	ICON_NULL = -1,
	
	ICON_FOLDER,
	ICON_FOLDEROPEN,
	ICON_NORMAL, 

	ICON_NPC, 
	ICON_OBJECT,
	ICON_TRAP,
	ICON_CAMERA, 
	ICON_LIGHT,
	ICON_TERRA,
	ICON_ROOM,
	ICON_ROAD,
	ICON_DOODAD,
	ICON_DOODADCOMPLEX,
	ICON_GROUP,
	ICON_OBSTACLE,
	ICON_SPECIAL,

	ICON_SETTING,
	ICON_COMMAND,
	ICON_TEMPLATE,
	
	ICON_FOLDERHAVEFILE,
	ICON_FOLDERHAVEFILEOPEN,

	ICON_NUM,
};

class CDirTree : public CTreeCtrl
{
// Construction
public:
	CDirTree();
	CString m_strRoot;
	CString m_strExt;
	CString m_strFile;
	bool m_bFolderIsFile;
	bool m_bGetIconByFile;
	bool m_bGetIconByName;
	bool m_bGetIconByContent;
	void SetGetIconByFile(bool b){m_bGetIconByFile = b;}
	void SetGetIconByName(bool b){m_bGetIconByName = b;}
	void SetGetIconByContent(bool b){m_bGetIconByContent= b;};
	void SetFolderIsFile(bool b){m_bFolderIsFile = b;};

	void LoadQuickKey(LPCSTR szFile);
	void SaveQuickKey();
	CString m_strQuickKeyFile;
	CString m_aQuickSave[10];
	void Select(int i);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDirTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	CStringList m_listLoadFolder;
	void AddLoadFolder(const CString& str){m_listLoadFolder.AddTail(str);};
	void ClearLoadFolder(){m_listLoadFolder.RemoveAll();};

	CString GetSelectedFile();
	CImageList m_listNormal;
//	BOOL ConstructIcon(HICON hiconFolder, HICON hiconFolderOpen, HICON hFile);
	BOOL ConstructIcon(CImageList* rlist);
	BOOL ConstructTree(const CString& root, const CString& ext);
	BOOL Reload();
	BOOL AddNode(const CString& file);
	BOOL SelectNode(const CString& file);
	void CollapseAll();
protected:
	bool IsItemFolder(HTREEITEM h);
	void InsertFolder(const HTREEITEM father, const CString& strPath, int layer);
	void InsertFile(const HTREEITEM father, const CString& strPath);
	HTREEITEM FindChildFolder(HTREEITEM hitem,const CString& strName);
	HTREEITEM FindChildFile(HTREEITEM hitem,const CString& strName);
	HTREEITEM AddChildFolder(HTREEITEM hitem,const CString& sFolder, LPCSTR szPath);
	HTREEITEM AddChildFile(HTREEITEM hitem,const CString& sPathFile);
	void FillItem(TVITEM& item,const CString& sName, int iIcon);
	void FillItem(TVITEM& item,const CString& sPathFile);
	int GetImageInfo(const CString& strFile);
	void GetHelpInfo(const CString& szFile, CString& strHelp);
// Keyboard
	static bool			IsKeyDown(UINT vk);
	static bool			IsCtrlDown();
	static bool			IsShiftDown();
	static bool			IsAltDown();

public:	
//call back
	void UpdateSelectedNode();
	void ClearChildItem(HTREEITEM hItem);
	void DeleteSelect();

	virtual ~CDirTree();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDirTree)
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

eIconType IconNameToID(const CString& s);
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FOLDERTREE_H__E9AE50E5_32E3_45CB_9A23_02F8F4BF5D2F__INCLUDED_)
