// MapConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"
#include "iItem.h"
#include "iGraphics.h"
#include "..\controls\FileFindEx.h"
#include "math.h"
#include <iomanip>
#include <fstream>
#include <shlwapi.h>
#include "iImage.h"
#include "foreach.h"
#include "KSpbioTemplate.h"

#include "izip.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapConvertDlg dialog

CMapConvertDlg::CMapConvertDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapConvertDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapConvertDlg)
	m_strFile = _T("");
	m_strDes = _T("");
	m_strSrc = _T("");
	m_nRight = 0;
	m_nBottom = 0;
	m_nEnlarge = 4;
	m_nDivBottom = 100;
	m_nDivTop = 90;
	m_nDivRight = 100;
	m_nDivLeft = 90;
	m_strNewMap = _T("test");
	m_bGround = FALSE;
	m_bPlane = FALSE;
	m_bNpc = FALSE;
	m_bObj = FALSE;
	m_bTrap = FALSE;
	m_bObstacle = FALSE;
	m_bLight = FALSE;
	m_strArtmap = _T("");
	m_strDesignMap = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	for (int x=0; x<MAX_REGION; x++)
	for (int y=0; y<MAX_REGION; y++)
	{
		m_aRegion[x][y] = NULL;
		m_aTrap[x][y] = NULL;
	}

	m_bBatMap = false;
	m_bStep9 = false;
	Common_GetPack1D(Pack1D_Plane)->Open(FILE_TEMPLATE_ZIP,FILE_TEMPLATE_INDEX);
}

void CMapConvertDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapConvertDlg)
	DDX_Text(pDX, IDC_MAPNAME, m_strFile);
	DDX_Text(pDX, IDC_DES, m_strDes);
	DDX_Text(pDX, IDC_SRC, m_strSrc);
	DDX_Text(pDX, IDC_RIGHT, m_nRight);
	DDX_Text(pDX, IDC_RIGHT2, m_nBottom);
	DDX_Text(pDX, IDC_DIV_BOTTOM, m_nDivBottom);
	DDX_Text(pDX, IDC_DIV_TOP, m_nDivTop);
	DDX_Text(pDX, IDC_DIV_RIGHT, m_nDivRight);
	DDX_Text(pDX, IDC_DIV_LEFT, m_nDivLeft);
	DDX_Text(pDX, IDC_NEWMAP, m_strNewMap);
	DDX_Check(pDX, IDC_BIN_GROUND_LAYER, m_bGround);
	DDX_Check(pDX, IDC_BIN_BIO, m_bPlane);
	DDX_Check(pDX, IDC_BIN_NPC, m_bNpc);
	DDX_Check(pDX, IDC_BIN_OBJ, m_bObj);
	DDX_Check(pDX, IDC_BIN_TRAP, m_bTrap);
	DDX_Check(pDX, IDC_BIN_OBSTACLE, m_bObstacle);
	DDX_Check(pDX, IDC_BIN_LIGHT, m_bLight);
	DDX_Text(pDX, IDC_ART_MAP, m_strArtmap);
	DDX_Text(pDX, IDC_DESIGN_MAP, m_strDesignMap);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMapConvertDlg, CDialog)
	//{{AFX_MSG_MAP(CMapConvertDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_REPLACE, OnReplace)
	ON_BN_CLICKED(IDC_MAP, OnMap)
	ON_BN_CLICKED(IDC_VALIDATE, OnValidate)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_CLEARNULL, OnClearnull)
	ON_BN_CLICKED(IDC_INI, OnIni)
	ON_BN_CLICKED(IDC_OFFSET, OnOffset)
	ON_BN_CLICKED(IDC_OBSTACLE, OnObstacle)
	ON_BN_CLICKED(IDC_IMAGE, OnImage)
	ON_BN_CLICKED(IDC_LENGTH, OnLength)
	ON_BN_CLICKED(IDC_ALPHA, OnAlpha)
	ON_BN_CLICKED(IDC_BINARY, OnBinary)
	ON_BN_CLICKED(IDC_START_BAT, OnStartBat)
	ON_BN_CLICKED(IDC_MAP_ADD, OnAddMap)
	ON_BN_CLICKED(IDC_MAP_REMOVE, OnRemoveMap)
	ON_BN_CLICKED(IDC_ZIPTEMPLATE, OnZiptemplate)
	ON_BN_CLICKED(IDC_ZIPMAP, OnZipmap)
	ON_BN_CLICKED(IDC_FILLSPRINFO, OnFillsprinfo)
	ON_BN_CLICKED(IDC_CONVERCOORD, OnConvercoord)
	ON_BN_CLICKED(IDC_CHECKSPR, OnCheckspr)
	ON_BN_CLICKED(IDC_ZIPGROUND, OnZipground)
	ON_BN_CLICKED(IDC_DIV, OnDiv)
	ON_BN_CLICKED(IDC_ZIPNPC, OnZipnpc)
	ON_BN_CLICKED(IDC_TRAP, OnTrap)
	ON_BN_CLICKED(IDC_CONVERNPC, OnConvernpc)
	ON_BN_CLICKED(IDC_DESIGNMAP, OnDesignmap)
	ON_BN_CLICKED(IDC_ARTMAP, OnArtmap)
	ON_BN_CLICKED(IDC_JOIN_ART, OnJoinArt)
	ON_BN_CLICKED(IDC_JOIN_DESIGN, OnJoinDesign)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//utilty
/////////////////////////////////////////////////////////////////////////////

int GetFileLength(LPCSTR szFile)
{
	CStdioFile f;
	if(f.Open(szFile,CFile::modeRead))
	{
		return f.GetLength();
	}
	return -1;
}

BOOL GetRegionSize2(LPCSTR szName, int& sx, int& sy)
{
	CString sName = szName;
	int pos = sName.Find("_");
	if (pos == -1)
		return FALSE;
	CString sX = sName.Left(pos);
	CString sY = sName.Mid(pos + 1);
	sx = atoi((LPCSTR)sX);
	sy = atoi((LPCSTR)sY);
	return TRUE;
}

BOOL GetRegionSize(LPCSTR szName, int& sx, int& sy)
{
	CString sName = szName;
	int pos = sName.Find("_");
	if (pos == -1)
		return FALSE;
	CString sX = sName.Left(pos);
	CString sY = sName.Mid(pos + 1);
	if (sX.GetLength() != 3 || sY.GetLength() != 3)
		return FALSE;
	sx = atoi((LPCSTR)sX);
	sy = atoi((LPCSTR)sY);
	return TRUE;
}

BOOL GetFullFolderRegionSize(LPCSTR sName, int& sx, int& sy)
{
	CString str = sName;
	int pos = str.ReverseFind('\\');
	if (pos == -1)
		return FALSE;
	str = str.Mid(pos + 1);	
	return GetRegionSize(str,sx,sy);
}

BOOL GetFullFileRegionSize(LPCSTR  sName, int& sx, int& sy)
{
	CString str = sName;
	int pos = str.ReverseFind('\\');
	if (pos == -1)
		return FALSE;
	str = str.Left(pos);
	return GetFullFolderRegionSize(str,sx,sy);
}

BOOL IsAnyInDir(const CString& sName)
{
	CString sFind = sName + "\\" + "*.*";
	CFileFind find;
	BOOL b = find.FindFile(sFind);
	BOOL bValidFile = false;
	if (b)
	{
		do 
		{
			b = find.FindNextFile();
			if (find.IsDots())
				continue;
			if (find.IsDirectory())
				return TRUE;
			CFile f;
			f.Open(find.GetFilePath(),CFile::modeRead);
			if (f.GetLength() > 0)
			{
				bValidFile = true;
				break;
			}
		}
		while(b);
	}
	return bValidFile;
}

BOOL IsAnyIniInDir(const CString& sName,LPCSTR strExt)
{
	CString sFind = sName + "\\" + strExt;
	CFileFind find;
	BOOL b = find.FindFile(sFind);
	BOOL bValidFile = false;
	if (b)
	{
		do 
		{
			b = find.FindNextFile();
			CFile f;
			f.Open(find.GetFilePath(),CFile::modeRead);
			if (f.GetLength() > 0)
			{
				bValidFile = true;
				break;
			}
		}
		while(b);
	}
	return bValidFile;
}

/////////////////////////////////////////////////////////////////
// CMapConvertDlg message handlers
/////////////////////////////////////////////////////////////////

BOOL CMapConvertDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CheckDlgButton(IDC_BIN_GROUND_LAYER, BST_CHECKED);
	CheckDlgButton(IDC_BIN_BIO, BST_CHECKED);
	CheckDlgButton(IDC_BIN_OBSTACLE, BST_CHECKED);

	HWND hListWnd = NULL;
	GetDlgItem(IDC_MAP_LIST, &hListWnd);
	
	//设置文件列表框的属性与分栏信息
	ListView_SetExtendedListViewStyle(hListWnd,
		ListView_GetExtendedListViewStyle(hListWnd) |	LVS_EX_FULLROWSELECT);

	LVCOLUMN	lvc;
	char		szHeaderText[40];
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.cx = 120;
	lvc.pszText = szHeaderText;
	lvc.cchTextMax = 40;
	strcpy(szHeaderText, "地图文件");
	ListView_InsertColumn(hListWnd, 0, &lvc);

	lvc.mask |= LVCF_SUBITEM;
	lvc.cx = 260;
	lvc.iSubItem = 1;
	strcpy(szHeaderText, "来源位置");
	ListView_InsertColumn(hListWnd, 1, &lvc);

	LoadIni();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMapConvertDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMapConvertDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMapConvertDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
/////////////////////////////////////////////////////////////////////////
//common
/////////////////////////////////////////////////////////////////////////
BOOL CMapConvertDlg::VerifyFile()
{
	UpdateData();
	if (m_strFile.IsEmpty())
	{
		AfxMessageBox("请先选择地图！");
		return FALSE;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
//browse
/////////////////////////////////////////////////////////////////////////
void CMapConvertDlg::GetFile(CString& m_strFile)
{
	UpdateData();
	if (m_strFile.IsEmpty())
	{
		CString strFile;
		CString sWork;
		AppGetWorkingFolder(sWork);
		sWork += "\\maps";
		if (AppGetFileDialog(strFile,sWork,"*.wor"))
		{
			m_strFile = strFile;
			UpdateData(FALSE);
		}
	}
	else
	{
		CString strFolder;
		AppParsePath(m_strFile,strFolder);
		CString strFile;
		if (AppGetFileDialog(strFile,strFolder,"*.wor"))
		{
			m_strFile = strFile;
			UpdateData(FALSE);
		}
	}
}

void CMapConvertDlg::OnMap() 
{
	GetFile(m_strFile);
}

/////////////////////////////////////////////////////////
//get rect
/////////////////////////////////////////////////////////
void CMapConvertDlg::GetRect(CRect& rect) 
{
	if (!VerifyFile())
		return;
	
	CRect rc(LONG_MAX,LONG_MAX,0,0);

	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4) + "\\???_???";
	CFileFindEx f;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);

	if (f.FindFile(sFolder))
	{
		BOOL bFind;
		do
		{
			bFind = f.FindNextFile();
			CString sName = f.GetFileName();
			CString sPath = f.GetFilePath();
			if (sName.GetLength() != 7)
				continue;
			
			if (!IsAnyInDir(sPath))
				continue;

			int sx,sy;
			if (!GetRegionSize(sName,sx,sy))
				continue;
			rc.left = min(sx,rc.left);
			rc.top = min(sy,rc.top);
			rc.right = max(sx,rc.right);
			rc.bottom = max(sy,rc.bottom);
		}while(bFind);
	}
	rect = rc;
}


/////////////////////////////////////////////////////////////////////
//utiltiy
/////////////////////////////////////////////////////////////////////
void CMapConvertDlg::FindFileUnderMap(mc_doonce fun,LPCSTR szFindFile)
{
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	int pos = m_strFile.ReverseFind('.');
	CString strFolder = m_strFile.Left(pos);
	foreach_fileunderfolder(fun,szFindFile,strFolder);
}

void CMapConvertDlg::FindFolderUnderMap(mc_doonce fun)
{
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	int pos = m_strFile.ReverseFind('.');
	CString strFolder = m_strFile.Left(pos);
	foreach_folderunderfolder(fun,strFolder);
}

void CMapConvertDlg::FindEachRegionFolder(mc_doonce fun)
{
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	foreachregion f(fun);
	f.SetFindFile("");
	f.SetFindFolder("");
	f.foreach_file(m_strFile);
}

void CMapConvertDlg::FindEachRegionUpFolder(mc_doonce fun)
{
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	int pos = m_strFile.ReverseFind('\\');
	CString strFolder = m_strFile.Left(pos);
	foreachregion f(fun);
	f.SetFindFile("");
	f.SetFindFolder("");
	f.foreach_file(strFolder);
}

void CMapConvertDlg::SetMapVerson(long l)
{
	if (!VerifyFile())
		return;
	iItem* pItem = Common_CreateItem(m_strFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"verson",l);
	pItem->Save();
}

void CMapConvertDlg::SaveIni()
{
	CString str = "MapConvert.ini";
	AppGetFullPath(str);
	iItem* pItem = Common_CreateItem(str,iItem::FLAG_CREATE);
	if (pItem != NULL)
	{
		wrap_ptr<iItem> ptr(pItem);
		SetItemValue(pItem,"map",m_strFile);
		SetItemValue(pItem,"artmap",m_strArtmap);
		SetItemValue(pItem,"designmap",m_strDesignMap);

		int bChecked;
		bChecked = (IsDlgButtonChecked(IDC_STEP_4) == BST_CHECKED);
		SetItemValue(pItem, "Step4", bChecked);
		bChecked = (IsDlgButtonChecked(IDC_STEP_6) == BST_CHECKED);
		SetItemValue(pItem, "Step6", bChecked);
		bChecked = (IsDlgButtonChecked(IDC_STEP_7) == BST_CHECKED);
		SetItemValue(pItem, "Step7", bChecked);
		bChecked = (IsDlgButtonChecked(IDC_STEP_9) == BST_CHECKED);
		SetItemValue(pItem, "Step9", bChecked);

		HWND hListWnd = NULL;
		const char* pListSection = "MapList";
		GetDlgItem(IDC_MAP_LIST, &hListWnd);
		int nCount = ListView_GetItemCount(hListWnd);
		SetItemValue(pItem, "Count", nCount, pListSection);
		char	szBuffer[MAX_PATH], szKey[8], szFile[MAX_PATH];
		for (int i = 0; i < nCount; i++)
		{
			itoa(i, szKey, 10);
			ListView_GetItemText(hListWnd, i, 1, szBuffer, MAX_PATH);
			ListView_GetItemText(hListWnd, i, 0, szFile, MAX_PATH);
			strcat(szBuffer, "\\");
			strcat(szBuffer, szFile);
			SetItemValue(pItem, szKey, szBuffer, pListSection);
		}

		pItem->Save();
	}
}

BOOL CMapConvertDlg::CheckFileIncluded(const char* pszPath, const char* pszFile)
{
	char	szBuffer[MAX_PATH];
	HWND hListWnd = NULL;
	GetDlgItem(IDC_MAP_LIST, &hListWnd);
	if (pszPath == NULL || pszFile == NULL || hListWnd == NULL)
		return FALSE;
	int	nCount = ListView_GetItemCount(hListWnd);
	for (int i = 0; i < nCount; i++)
	{
		ListView_GetItemText(hListWnd, i, 0, szBuffer, MAX_PATH);
		if (!strcmp(szBuffer, pszFile))
		{
			ListView_GetItemText(hListWnd, i, 1, szBuffer, MAX_PATH);
			if (!strcmp(szBuffer, pszPath))
				return TRUE;
		}
	}
	return FALSE;
}

void CMapConvertDlg::LoadIni()
{
	CString str = "MapConvert.ini";
	AppGetFullPath(str);
	iItem* pItem = Common_CreateItem(str,iItem::FLAG_LOAD);
	if (pItem != NULL)
	{
		wrap_ptr<iItem> ptr(pItem);
		GetItemValue(pItem,"map",m_strFile);
		GetItemValue(pItem,"artmap",m_strArtmap);
		GetItemValue(pItem,"designmap",m_strDesignMap);
		UpdateData(FALSE);

		int bChecked;
		if (!GetItemValue(pItem, "Step4", bChecked))
			bChecked = false;
		CheckDlgButton(IDC_STEP_4, bChecked ? BST_CHECKED : BST_UNCHECKED);
		
		if (!GetItemValue(pItem, "Step6", bChecked))
			bChecked = true;
		CheckDlgButton(IDC_STEP_6, bChecked ? BST_CHECKED : BST_UNCHECKED);
			
		if (!GetItemValue(pItem, "Step7", bChecked))
			bChecked = true;
		CheckDlgButton(IDC_STEP_7, bChecked ? BST_CHECKED : BST_UNCHECKED);
		
		if (!GetItemValue(pItem, "Step9", bChecked))
			bChecked = false;
		CheckDlgButton(IDC_STEP_9, bChecked ? BST_CHECKED : BST_UNCHECKED);

		HWND hListWnd = NULL;
		int nCount = 0;
		const char* pListSection = "MapList";
		GetDlgItem(IDC_MAP_LIST, &hListWnd);
		ListView_DeleteAllItems(hListWnd);
		GetItemValue(pItem, "Count", nCount, pListSection);
		char	szBuffer[MAX_PATH] = "", szKey[8];
		LV_ITEM			lvi;
		memset(&lvi, 0, sizeof(LV_ITEM));
		lvi.mask = LVIF_TEXT;
		lvi.iItem       = 0x7fffffff;
		lvi.pszText		= szBuffer;
		lvi.cchTextMax  = sizeof(szBuffer);
		CString	str;
		for (int i = 0; i < nCount; i++)
		{
			itoa(i, szKey, 10);
			GetItemValue(pItem, szKey, szBuffer, pListSection);
			str = szBuffer;
			int nPos = str.ReverseFind('\\');
			if (nPos > 1)
			{
				lvi.pszText = &szBuffer[nPos + 1];
				lvi.cchTextMax = strlen(lvi.pszText);
				szBuffer[nPos] = 0;
				int nItem = ListView_InsertItem(hListWnd, &lvi);
				ListView_SetItemText(hListWnd, nItem, 1, szBuffer);
			}
		}
	}
}

void CMapConvertDlg::OnCancel() 
{
	SaveIni();	
	CDialog::OnCancel();
}


void CMapConvertDlg::OnAddMap()
{
	OPENFILENAME	ofn;
	char*			pszFile = NULL;
	char			szFile[MAX_PATH];
	char			szPath[MAX_PATH];
	int				iItem, nLen;
	LV_ITEM			lvi;

	HWND hListWnd = NULL;
	GetDlgItem(IDC_MAP_LIST, &hListWnd);

	szFile[0] = '\0';
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
		OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	ofn.lpstrTitle="选择要转换的地图";
	ofn.nMaxFile=0x8000;
	ofn.lpstrFile=new char[ofn.nMaxFile];
	_ASSERT(ofn.lpstrFile);
	ofn.lpstrFile[0]=0;
	ofn.lpstrFilter = "剑网地图\0*.wor\0";
	if (!GetOpenFileName(&ofn))
	{
		delete[] ofn.lpstrFile;
		return;
	}
	strcpy(szPath, ofn.lpstrFile);
	szPath[ofn.nFileOffset - 1] = 0;
	pszFile = ofn.lpstrFile + ofn.nFileOffset;

	memset(&lvi, 0, sizeof(LV_ITEM));
	lvi.mask = LVIF_TEXT;
	iItem = ListView_GetItemCount(hListWnd);
	while(pszFile[0])
	{
		nLen = (int)strlen(pszFile);
		if (!CheckFileIncluded(szPath, pszFile))
		{
			lvi.iItem       = iItem;
			lvi.pszText		= pszFile;
			lvi.cchTextMax  = nLen + 1;
			if ((iItem = ListView_InsertItem(hListWnd, &lvi)) == -1)
				break;
			ListView_SetItemText(hListWnd, iItem, 1, szPath);
		}
		else
			MessageBox("不要折腾我，看清楚先。\n你头先已经加入这个文件了。", pszFile, MB_OK | MB_ICONINFORMATION);
		pszFile += nLen + 1;
	}
	delete[] ofn.lpstrFile;

}

void CMapConvertDlg::OnRemoveMap()
{
	int nItem = 0;
	HWND hListWnd = NULL;
	GetDlgItem(IDC_MAP_LIST, &hListWnd);
	while((nItem = ListView_GetNextItem(hListWnd, nItem - 1, LVNI_SELECTED)) >= 0)
		ListView_DeleteItem(hListWnd, nItem);
}


void CMapConvertDlg::OnStartBat()
{
	HWND hListWnd = NULL;
	GetDlgItem(IDC_MAP_LIST, &hListWnd);
	int nCount = ListView_GetItemCount(hListWnd);

	bool bStep4 = (IsDlgButtonChecked(IDC_STEP_4) == BST_CHECKED);
	bool bStep6 = (IsDlgButtonChecked(IDC_STEP_6) == BST_CHECKED);
	bool bStep7 = (IsDlgButtonChecked(IDC_STEP_7) == BST_CHECKED);
	m_bStep9 = (IsDlgButtonChecked(IDC_STEP_9) == BST_CHECKED);
	if (nCount <= 0 || (!bStep4 && !bStep6 && !bStep7 && !m_bStep9))
		return;

	m_bBatMap = true;
	char	szBuffer[MAX_PATH], szKey[8], szFile[MAX_PATH];
	for (int i = 0; m_bBatMap && i < nCount; i++)
	{
		itoa(i, szKey, 10);
		ListView_GetItemText(hListWnd, i, 1, szBuffer, MAX_PATH);
		ListView_GetItemText(hListWnd, i, 0, szFile, MAX_PATH);
		strcat(szBuffer, "\\");
		strcat(szBuffer, szFile);
		
		unsigned dword = GetFileAttributes(szBuffer);
		if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
		{
			CString error;
			error.Format("不存在文件:%s", szBuffer);
			SetDlgItemText(IDC_NOW_FILE, error);
			continue;
		}
		
		//选中地图
		UpdateData();
		m_strFile = szBuffer;
		SetDlgItemText(IDC_NOW_FILE, szBuffer);
		UpdateData(FALSE);

		if (bStep4)
			OnImage();
		if (bStep6)
			OnClear();
		if (bStep7)
			OnBinary();
	}

	if (m_bBatMap == true)
	{
		MessageBox("地图转换二进制版操作结束。", "操作结束", MB_OK);
		m_bBatMap = false;
	}

	m_bStep9 = false;
}

