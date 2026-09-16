// ResourceManagerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ResourceManager.h"
#include "ResourceManagerDlg.h"

#include "iImage.h"
#include "..\controls\FileFindEx.h"
#include "iitem.h"

#include "fileop.h"
#include "planedoodad.h"

#include "dividedlg.h"
#include <fstream>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
const int TIMER_AUTOPLAY = 10002;
const int TP_MODIFY_NUM = TP_ANILOOP + 1;

bool g_bCopyToNet = TRUE;

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
// CResourceManagerDlg dialog

CResourceManagerDlg::CResourceManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CResourceManagerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CResourceManagerDlg)
	m_bVersort = FALSE;
	m_bKind = FALSE;
	m_bTextureUse = FALSE;
	m_bSave = FALSE;
	m_bCenterY = FALSE;
	m_bCenterX = FALSE;
	m_bPlaneType = FALSE;
	m_bRadius = FALSE;
	m_bObstacleType = FALSE;
	m_bLighted = FALSE;
	m_bLayer = FALSE;
	m_bHeight = FALSE;
	m_bAngleZ = FALSE;
	m_bAngleXY = FALSE;
	m_bLighting = FALSE;
	m_nFrame = 0;
	m_strAnimation = _T("");
	m_bAutoPlay = FALSE;
	m_nDirection = 0;
	m_nSpeed = 10;
	m_bShowCenter = FALSE;
	m_bAll = FALSE;
	m_bAlphaFormat = FALSE;
	m_bAlphaSet = TRUE;
	m_nCenterSetting = -1;
	m_bAniSpeed = TRUE;
	m_bRemoveShadow = FALSE;
	m_bAniLoop = TRUE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bCreate = false;
	m_nOption = 0;

	for (int i=0; i<MAX_COMBOX; i++)
	{
		m_aCB[i] = NULL;
		m_aAll[i] = NULL;
	}
	m_aCB[TP_TEXTUREUSE]	=	&m_cbTextureUse;
	m_aCB[TP_PLANETYPE]		=	&m_cbPlaneType;
	m_aCB[TP_LIGHTING]		=	&m_cbLighting;
	m_aCB[TP_LIGHTED]		=	&m_cbLighted;
	m_aCB[TP_RADIUS]		=	&m_cbRadius;
	m_aCB[TP_SORT]			=	&m_cbLayer;
	m_aCB[TP_ANGLEXY]		=	&m_cbAngleXY;
	m_aCB[TP_ANGLEZ]		=	&m_cbAngleZ;
	m_aCB[TP_CENTERX]		=	&m_cbCenterX;
	m_aCB[TP_CENTERY]		=	&m_cbCenterY;
	m_aCB[TP_HEIGHT]		=	&m_cbHeight;
	m_aCB[TP_OBSTACLETYPE]	=	&m_cbObstacleType;
	m_aCB[TP_DYNAMICTYPE]	=	&m_cbSave;
	m_aCB[TP_KIND]			=	&m_cbKind;
	m_aCB[TP_VERSORT]		=   &m_cbVersort;
	m_aCB[TP_ANISPEED]		=   &m_cbAniSpeed;
	m_aCB[TP_ANILOOP]		=   &m_cbAniLoop;

	m_aAll[TP_TEXTUREUSE]	=	&m_bTextureUse;
	m_aAll[TP_PLANETYPE]	=	&m_bPlaneType;
	m_aAll[TP_LIGHTING]		=	&m_bLighting;
	m_aAll[TP_LIGHTED]		=	&m_bLighted;
	m_aAll[TP_RADIUS]		=	&m_bRadius;
	m_aAll[TP_SORT]			=	&m_bLayer;
	m_aAll[TP_ANGLEXY]		=	&m_bAngleXY;
	m_aAll[TP_ANGLEZ]		=	&m_bAngleZ;
	m_aAll[TP_CENTERX]		=	&m_bCenterX;
	m_aAll[TP_CENTERY]		=	&m_bCenterY;
	m_aAll[TP_HEIGHT]		=	&m_bHeight;
	m_aAll[TP_OBSTACLETYPE]	=	&m_bObstacleType;
	m_aAll[TP_DYNAMICTYPE]	=	&m_bSave;
	m_aAll[TP_KIND]			=	&m_bKind;
	m_aAll[TP_VERSORT]		=   &m_bVersort;
	m_aAll[TP_ANISPEED]		=   &m_bAniSpeed;
	m_aAll[TP_ANILOOP]		=   &m_bAniLoop;
}

void CResourceManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResourceManagerDlg)
	DDX_Control(pDX, IDC_ANISPEED, m_cbAniSpeed);
	DDX_Control(pDX, IDC_ANILOOP, m_cbAniLoop);
	DDX_Control(pDX, IDC_CENTERSETTING, m_cbCenterSetting);
	DDX_Control(pDX, IDC_SPINY, m_spinY);
	DDX_Control(pDX, IDC_SPINX, m_spinX);
	DDX_Control(pDX, IDC_VERSORT, m_cbVersort);
	DDX_Control(pDX, IDC_KIND, m_cbKind);
	DDX_Control(pDX, IDC_TEXTUREUSE, m_cbTextureUse);
	DDX_Control(pDX, IDC_SAVETYPE, m_cbSave);
	DDX_Control(pDX, IDC_CENTERY, m_cbCenterY);
	DDX_Control(pDX, IDC_CENTERX, m_cbCenterX);
	DDX_Control(pDX, IDC_PLANETYPE, m_cbPlaneType);
	DDX_Control(pDX, IDC_RADIUS, m_cbRadius);
	DDX_Control(pDX, IDC_OBSTACLETYPE, m_cbObstacleType);
	DDX_Control(pDX, IDC_LIGHTED, m_cbLighted);
	DDX_Control(pDX, IDC_LAYER, m_cbLayer);
	DDX_Control(pDX, IDC_HEIGHT, m_cbHeight);
	DDX_Control(pDX, IDC_ANGLEZ, m_cbAngleZ);
	DDX_Control(pDX, IDC_ANGLEXY, m_cbAngleXY);
	DDX_Control(pDX, IDC_LIGHTING, m_cbLighting);
	DDX_Check(pDX, IDC_ALL_VERSORT, m_bVersort);
	DDX_Check(pDX, IDC_ALL_KIND, m_bKind);
	DDX_Check(pDX, IDC_ALL_TEXTUREUSE, m_bTextureUse);
	DDX_Check(pDX, IDC_ALL_SAVETYPE, m_bSave);
	DDX_Check(pDX, IDC_ALL_CENTERY, m_bCenterY);
	DDX_Check(pDX, IDC_ALL_CENTERX, m_bCenterX);
	DDX_Check(pDX, IDC_ALL_PLANETYPE, m_bPlaneType);
	DDX_Check(pDX, IDC_ALL_RADIUS, m_bRadius);
	DDX_Check(pDX, IDC_ALL_OBSTACLETYPE, m_bObstacleType);
	DDX_Check(pDX, IDC_ALL_LIGHTED, m_bLighted);
	DDX_Check(pDX, IDC_ALL_LAYER, m_bLayer);
	DDX_Check(pDX, IDC_ALL_HEIGHT, m_bHeight);
	DDX_Check(pDX, IDC_ALL_ANGLEZ, m_bAngleZ);
	DDX_Check(pDX, IDC_ALL_ANGLEXY, m_bAngleXY);
	DDX_Check(pDX, IDC_ALL_LIGHTING, m_bLighting);
	DDX_Control(pDX, IDC_TREE_DES, m_treeDes);
	DDX_Control(pDX, IDC_TREE_SOURCE, m_treeSrc);
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinSpeed);
	DDX_Control(pDX, IDC_SPIN_FRAME, m_spinFrame);
	DDX_Control(pDX, IDC_SPIN_DIRECTION, m_spinDirection);
	DDX_Control(pDX, IDC_SPIN_ANIMATION, m_spinAnimation);
	DDX_Control(pDX, IDC_STATIC_SPR, m_staticSpr);
	DDX_Text(pDX, IDC_FRAME, m_nFrame);
	DDX_Text(pDX, IDC_ANIMATION, m_strAnimation);
	DDX_Check(pDX, IDC_AUTOPLAY, m_bAutoPlay);
	DDX_Text(pDX, IDC_DIRECTION, m_nDirection);
	DDX_Text(pDX, IDC_SPEED, m_nSpeed);
	DDX_Check(pDX, IDC_SHOWCENTER, m_bShowCenter);
	DDX_Check(pDX, IDC_ALL, m_bAll);
	DDX_Check(pDX, IDC_ALPHAFORMAT, m_bAlphaFormat);
	DDX_Check(pDX, IDC_ALPHASET, m_bAlphaSet);
	DDX_CBIndex(pDX, IDC_CENTERSETTING, m_nCenterSetting);
	DDX_Check(pDX, IDC_ALL_ANISPEED, m_bAniSpeed);
	DDX_Check(pDX, IDC_SHADOW, m_bRemoveShadow);
	DDX_Check(pDX, IDC_ALL_ANILOOP, m_bAniLoop);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CResourceManagerDlg, CDialog)
	//{{AFX_MSG_MAP(CResourceManagerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_FRAME, OnChangeFrame)
	ON_EN_CHANGE(IDC_DIRECTION, OnChangeDirection)
	ON_BN_CLICKED(IDC_AUTOPLAY, OnAutoplay)
	ON_BN_CLICKED(IDC_STATIC_SPR, OnStaticSpr)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_SHOWCENTER, OnShowcenter)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_CBN_EDITCHANGE(IDC_CENTERX, OnEditchangeCenterx)
	ON_CBN_EDITCHANGE(IDC_CENTERY, OnEditchangeCentery)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SPR2TXT, OnSpr2txt)
	ON_CBN_SELCHANGE(IDC_PLANETYPE, OnSelchangePlanetype)
	ON_BN_CLICKED(IDC_GETCENTER, OnGetcenter)
	ON_BN_CLICKED(IDC_LOG, OnLog)
	ON_BN_CLICKED(IDC_INI2TXT, OnIni2txt)
	ON_BN_CLICKED(IDC_LOCATE, OnLocate)
	ON_BN_CLICKED(IDC_ADDSPRINFO, OnAddsprinfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceManagerDlg message handlers

/////////////////////////////////////////////////////////////////////////////
//vitrual window function

BOOL CResourceManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_spinX.SetRange(0,1024);
	m_spinY.SetRange(0,1024);
	
	int i;
	for (i=0; i<TP_MODIFY_NUM; i++)
	{
		int j = 0;
		CString* aStr = GetTablePlaneValue(i);
		while(!aStr[j].IsEmpty())
			m_aCB[i]->AddString(aStr[j++]);
	}

	LoadOption();

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
	
	// TODO: Add extra initialization here
	ConstructTrees();

	m_spinSpeed.SetRange(0,20);
	SetTimer(TIMER_AUTOPLAY,100/4,NULL);

	m_bCreate = true;

	CString setting[] = {"根据alpha的最低点","底边的最低点","底边的最高点","底边的中点","图片的中心",""};
	i = 0;
	while(!setting[i].IsEmpty())
		m_cbCenterSetting.AddString(setting[i++]);
	return TRUE;
}

void CResourceManagerDlg::ConstructTrees()
{
	CImageList m_listNormal;

	BOOL b = m_listNormal.Create(16,16,ILC_COLOR8,3,3);
	ASSERT(b);

	UINT aICON[] = 
	{
		IDI_ITEM_FOLDER,
		IDI_ITEM_FOLDEROPEN,
		IDI_ITEM_FILE,
		IDI_ITEM_NPC,
		IDI_ITEM_OBJECT,
		IDI_ITEM_TRAP,
		IDI_ITEM_CAMERA,
		IDI_ITEM_LIGHT,
		IDI_ITEM_TERRA,
		IDI_ITEM_ROOM,
		IDI_ITEM_ROAD,
		IDI_ITEM_DOODAD,
		IDI_ITEM_DOODADCOMPLEX,
		IDI_ITEM_GROUP,
		IDI_ITEM_OBSTACLE,
		IDI_ITEM_SPECIAL,
		IDI_ITEM_SETTING,
		IDI_ITEM_COMMAND,
		
		IDI_ITEM_FOLDERHAVEFILE,
		IDI_ITEM_FOLDERHAVEFILEOPEN,		
		-1,
	};

	int i=0;
	while(aICON[i] != -1)
	{
		HICON hicon ;
		hicon = AfxGetApp()->LoadIcon(aICON[i]);
		VERIFY(m_listNormal.Add(hicon) != -1);
		i++;
	}

	m_treeDes.ConstructIcon(&m_listNormal);
	m_treeSrc.ConstructIcon(&m_listNormal);
	m_treeDes.SetGetIconByFile(true);
	m_treeSrc.SetGetIconByContent(true);
	
	{
		CString sIni = "users\\rm_folder.ini";
		AppGetFullPath(sIni);
		iItem* pItem = Common_CreateItem(sIni,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		int index;
		CString strValue;
		if (GetItemValue(pItem,"select",index) && GetItemValue(pItem,"folders",strValue,index))
		{
			int pos;
			strValue.TrimRight();
			strValue.TrimLeft();
			do
			{
				pos = strValue.Find(",");
				if (pos == -1)
					break;
				CString strFolder = strValue.Left(pos);
				m_treeDes.AddLoadFolder(strFolder);
				m_treeSrc.AddLoadFolder(strFolder);
				strValue= strValue.Mid(pos + 1);
			}while(!strValue.IsEmpty());
		}
		else
		{
			AfxMessageBox("file: rm_folder.ini error!");
		}
	}

	CString sFile;
	sFile = "users\\rm_path.ini";
	AppGetFullPath(sFile);
	iItem* pItem = Common_CreateItem(sFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"美术资源",m_strSrcRoot))
	{
		m_strSrcRoot = "\\美术资源";
		AppGetFullPath(m_strSrcRoot);
	}
	m_treeSrc.ConstructTree(m_strSrcRoot,"");
	if (!GetItemValue(pItem,"网络资源",m_strNetRoot))
	{
		m_strNetRoot = "\\网络资源";
		AppGetFullPath(m_strNetRoot);//使用本地的相对目录
	}

	AppGetWorkingFolder(m_strDesRoot);
	m_strDesRoot += "\\游戏资源";
	m_treeDes.ConstructTree(m_strDesRoot,"txt");

	CString sini = "users\\rm_treesrc_quickkey.txt";
	AppGetFullPath(sini);
	m_treeSrc.LoadQuickKey(sini);
	sini = "users\\rm_treedes_quickkey.txt";
	AppGetFullPath(sini);
	m_treeDes.LoadQuickKey(sini);
}

void CResourceManagerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CResourceManagerDlg::OnPaint() 
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
HCURSOR CResourceManagerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

LRESULT CResourceManagerDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	switch (message)
	{
	case WM_DIRTREE_LEFTSELECTED:
		OnSelectFile((LPCSTR)wParam,(CDirTree*)lParam);
		return 0;
	case WM_DIRTREE_RIGHTSELECTED:
		OnDbSelectFile((LPCSTR)wParam,(CDirTree*)lParam);
		return 0;
	}
	
	return CDialog::WindowProc(message, wParam, lParam);
}

void CResourceManagerDlg::SetStaticFrame(int nDirection,int nFrame)
{
	if (m_strSelectFile.IsEmpty())
		return;
	UINT frames = m_staticSpr.GetFrames();
	UINT directions = m_staticSpr.GetDirections();
	int index = frames / directions * nDirection + nFrame;
	ASSERT(index < frames);
	UpdatePlaneInfo(index);
	CString s;
	int xCenter,yCenter;
	m_aCB[TP_CENTERX]->GetWindowText(s);
	StringToValue(s,xCenter);
	m_aCB[TP_CENTERY]->GetWindowText(s);
	StringToValue(s,yCenter);
	m_staticSpr.SetFrame(nDirection,nFrame);	
	m_staticSpr.SetCenter(CPoint(xCenter,yCenter));
	//set info!
}

void CResourceManagerDlg::OnChangeFrame() 
{
	if (m_bCreate)
	{
		UpdateData();
		SetStaticFrame(m_nDirection,m_nFrame);	
	}
}

void CResourceManagerDlg::OnChangeDirection() 
{
	if (m_bCreate)
	{
		UpdateData();
		SetStaticFrame(m_nDirection,m_nFrame);	
	}	
}

void CResourceManagerDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == TIMER_AUTOPLAY)
	{
		static UINT nFrame = 0;
		UpdateData();
		if (m_bAutoPlay)
		{
			nFrame++;
			if (nFrame < m_nSpeed)
				return;
			nFrame = 0;
			UINT frames = m_staticSpr.GetFrames();
			if (frames == 1)
				return ;
			UINT directions = m_staticSpr.GetDirections();
			m_nFrame++;
			if (m_nFrame >= frames / directions)
			{
				m_nFrame = 0;
				m_nDirection ++;
				if (m_nDirection >= directions)
					m_nDirection = 0;
			}
			UpdateData(FALSE);
			SetStaticFrame(m_nDirection,m_nFrame);	
		}
		OnEditchangeCenter();		
	}
	CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
//option
int FindString(const CString& sSrc, const CString& sDes)
{
	CString s1,s2;
	s1 = sSrc;
	s1.MakeLower();
	s2 = sDes;
	s2.MakeLower();
	return s1.Find(s2);
}

void GetString(const CString& sSrc, const CString& key, CString& value)
{
	int pos;
	pos = FindString(sSrc,key);
	ASSERT(pos != -1);
	value = sSrc.Mid(pos+key.GetLength());
	value.TrimLeft();
	value.TrimRight();
}

void GetString(const CString& sSrc, const CString& key, int& value)
{
	CString s;
	GetString(sSrc,key,s);
	value = atoi((LPCSTR)s);
}

void CResourceManagerDlg::LoadOption()
{
//	AppGetWorkingFolder(sFile);
	CString sFile;
	sFile = "system\\ini\\rm_key.ini";
	AppGetFullPath(sFile);

	m_nOption = 0;
	CStdioFile file;
	if (!file.Open(sFile,CFile::modeRead))
		return;
	CString s;
	while(1)
	{
		do
		{
			file.ReadString(s);
		}while(s.IsEmpty());
		
		if (s.Find("[END]")!=-1)
			break;
		//read key
		GetString(s,"[KEY]",m_aOption[m_nOption].strKeyword);
		//read ext
		VERIFY(file.ReadString(s));
		GetString(s,"[EXT]",m_aOption[m_nOption].strExt);
		//read size
		VERIFY(file.ReadString(s));
		GetString(s,"[WIDTH]",m_aOption[m_nOption].width);
		VERIFY(file.ReadString(s));
		GetString(s,"[HEIGHT]",m_aOption[m_nOption].height);
		//read direction
		VERIFY(file.ReadString(s));
		GetString(s,"[DIRECTIONS]",m_aOption[m_nOption].nDirection);
		VERIFY(file.ReadString(s));
		GetString(s,"[SORT]",m_aOption[m_nOption].nSort);
		VERIFY(file.ReadString(s));
		GetString(s,"[ICON]",m_aOption[m_nOption].sIcon);
		VERIFY(file.ReadString(s));
		GetString(s,"[IMAGEFOLDER]",m_aOption[m_nOption].sImageFolder);
		VERIFY(file.ReadString(s));
		GetString(s,"[TYPE]",m_aOption[m_nOption].sType);
		VERIFY(file.ReadString(s));
		GetString(s,"[GAMETYPE]",m_aOption[m_nOption].sGameType);
		VERIFY(file.ReadString(s));
		GetString(s,"[FULLIMAGE]",m_aOption[m_nOption].nFullImage);
		m_nOption++;
	}
}

BOOL CResourceManagerDlg::GetOptionByKeyword(LPCSTR szFolder,stOption& item)
{
	CString sFolder = szFolder;
	sFolder.MakeLower();
	for (int i=0; i<m_nOption; i++)
	{
		if (sFolder.Find(m_aOption[i].strKeyword)!=-1)
		{
			item = m_aOption[i];
			return TRUE;
		}
	}
	item = m_aOption[i-1];//默认都是简单建筑
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//Message handle

void CResourceManagerDlg::OnSelectFile(LPCSTR szFile,CDirTree* pTree)
{
	if (pTree == &m_treeSrc)
		return;
	ASSERT(pTree == &m_treeDes);
	CString s = szFile;
	if (s.Right(4) == ".txt")
	{
		m_strSelectFile = szFile;
		m_strSelectFolder.Empty();
	}
	else if (s.Find('.') == -1)
	{
		m_strSelectFile.Empty();
		m_strSelectFolder = szFile;
	}
	else
	{
		m_strSelectFile.Empty();
		m_strSelectFolder.Empty();
	}
	if (!m_strSelectFile.IsEmpty())
		ShowImage(szFile);
}

void CResourceManagerDlg::OnDbSelectFile(LPCSTR sz,CDirTree* pTree)
{
	if (pTree == &m_treeDes)
	{
		OnProperty(sz);
		return;
	};

	ASSERT(pTree == &m_treeSrc);

	UpdateData(TRUE);

	BOOL b;
	stOption item;
	b = GetOptionByKeyword(sz,item);
	int n;
	if (item.strKeyword == "(fz)")
	{
		if (ConvertPsd(sz))
			return;
		n  = ConvertTgb(sz);
		if (n == -1)	//dlg choose cancle
			return;
		if (n == 0)		//if not fount tgb
		{
			if (ConvertTga(sz))
				return;
		}
	}

	if (b == TRUE)
	{
		ConvertFolderByOption(sz,item,(n!=1));
	}
	else
	{
		ConvertFolder(sz);
	}
}

/////////////////////////////////////////////////////////////////////////////
//show property

void CResourceManagerDlg::OnProperty(LPCSTR szFile)
{
	CString s = szFile;
	if (s.Right(4) != ".txt")
		return;
	CString sValue;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"planetype",sValue);
}

/////////////////////////////////////////////////////////////////////////////
//show image

void CResourceManagerDlg::ShowImage(LPCSTR szFile)
{
	CString sValue;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	BOOL b = GetItemValue(pItem,"file",sValue);
	if (!b)
		return;
	sValue = szFile;
	sValue = sValue.Left(sValue.GetLength() - 4) + ".spr";

	m_staticSpr.SetSprFile(sValue);
	SetStaticFrame(0,0);
	int frames = m_staticSpr.GetFrames();
	int directions = m_staticSpr.GetDirections();
	m_spinFrame.SetRange(0,frames / directions - 1);
	m_spinDirection.SetRange(0,directions-1);

	CString sFrame;
	CWnd* p;
	sFrame.Format("(%d)",frames / directions - 1);
	p = GetDlgItem(IDC_STATIC_FRAME);
	p->SetWindowText(sFrame);
	sFrame.Format("(%d)",directions);
	p = GetDlgItem(IDC_STATIC_DIRECTION);
	p->SetWindowText(sFrame);

	m_nFrame = 0;
	m_nDirection = 0;
	UpdateData(FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//convert
int CResourceManagerDlg::ConvertTgb(LPCSTR szPath)
{
	CString strFolder = szPath;
	CString strFind;
	strFind = strFolder + "\\0.tgb";
	CFileFind f;
	BOOL b = f.FindFile(strFind);
	if (!b)
		return FALSE;
	f.FindNextFile();
	CString strPath = f.GetFilePath();

	CString strSpr;
	GetSprFileNameByFolder(szPath,strSpr);
	CString sTxt;
	GetTxtFileNameByImage(strSpr,sTxt);

	CDivideDlg dlg;
	dlg.SetFile(strPath);
	dlg.SetTxtFile(sTxt);
	if ( dlg.DoModal() == IDOK)
	{
		CString sGroup;
		dlg.GetGroupFile(sGroup);
		if (!sGroup.IsEmpty())
		{
			CopyFileToNet(sGroup);
			CopyFileToNet(sTxt);
		}
		return 1;
	}
	return -1;
}

void CResourceManagerDlg::AlphaFormatTgaFile(LPCSTR szFile)
{
	iImage* p = Image_CreateImage(szFile);
	p->LoadFile(szFile);
	p->AlphaFormat();
	p->SaveFile(szFile);
	p->Release();
}

void CResourceManagerDlg::AlphaFormatTga(LPCSTR szPath)
{
	CWaitCursor cursor;
	
	CString strFolder = szPath;
	CString strFind;
	strFind = strFolder + "\\*.tga";
	CFileFind f;
	BOOL b = f.FindFile(strFind);
	if (!b)
		return;
	while(b)
	{
		b = f.FindNextFile();
		CString strPath = f.GetFilePath();
		AlphaFormatTgaFile(strPath);
	}
}

BOOL CResourceManagerDlg::ConvertTga(LPCSTR szPath)
{
	return ConvertExt(szPath,"*.tga");
}

BOOL CResourceManagerDlg::ConvertPsd(LPCSTR szPath)
{
	return ConvertExt(szPath,"*.psd");
}

void CResourceManagerDlg::ConvertExtFile(LPCSTR szFile, LPCSTR szPath, LPCSTR szExt)
{
	CString sExt = szExt;
	if (sExt == "*.tga")
	{
		CString s = szPath;
		s += "\\0.tgb";
		iImage* p = Image_CreateImage(szFile);
		p->LoadFile(szFile);
		p->MakeSmallestImage(m_bRemoveShadow);
		if (m_bRemoveShadow)
			p->RemoveShadow();
		p->SaveFile(s);
//		AppCopyFile(szFile,s);
	}
	else if (sExt == "*.psd")
	{
		iImage* p = Image_CreateImage(szFile);
		p->LoadFile(szFile);
		p->SaveFile(szPath);
		p->Release();
	}
}

BOOL CResourceManagerDlg::ConvertExt(LPCSTR szPath,LPCSTR szExt)
{
	CWaitCursor cursor;
	
	CString strFolder = szPath;
	int pos = strFolder.ReverseFind('\\');
	ASSERT(pos != -1);
	CString strName = strFolder.Mid(pos+1);
	CString strFind;
	strFind = strFolder + "\\" + szExt;
	CFileFind f;
	BOOL b = f.FindFile(strFind);
	if (!b)
		return FALSE;
	while(b)
	{
		b = f.FindNextFile();
		CString strPath = f.GetFilePath();
		CString sPath,sName,sExt;
		AppParsePathNameExt(strPath,sPath,sName,sExt);
		if (sName.Find(".") != -1)
		{
			CString strErr;
			strErr = "文件名称非法: " + strPath + "使用了两个'.' ";
			AfxMessageBox(strErr);
			return FALSE;
		}
		int pos = sPath.ReverseFind('\\');
		ASSERT(pos != -1);
		CString sDir = sPath.Mid(pos+1);
		sPath += "\\";
		sPath += sName;
//		sPath += sDir+sName;
		CFileFind f;
		if (f.FindFile(sPath) == FALSE)
		{
			AppBuildPath(sPath);
			ConvertExtFile(strPath,sPath,szExt);
			m_treeSrc.AddNode(sPath);
		}
		else
		{
			ConvertExtFile(strPath,sPath,szExt);
		}
	}
	return TRUE;
}

void CResourceManagerDlg::ConvertFolderByOption(LPCSTR sz,stOption& item,bool bDelete)
{
	CWaitCursor cursor;

	CString strSpr;
	GetSprFileNameByFolder(sz,strSpr);
	if (bDelete)
	{
		CString sTxt;
		GetTxtFileNameByImage(strSpr,sTxt);
		iItem* pItem = Common_CreateItem(sTxt,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItem);
//		pItem->ClearAll();
	}

	iImage* pImage = Image_CreateImage(strSpr); 

	CString strFolder = sz;
	int dir = 0;
	if (item.nSort == 1)
	{
		while(1)
		{
			CString strFind;
			strFind.Format("%s\\*_%d.%s",(LPCSTR)strFolder,dir++,(LPCSTR)item.strExt);
	
			CFileFindEx f;
			BOOL b = f.FindFile(strFind);
			if (!b)
				break;

			CWaitCursor cursor;
			while(b)
			{
				b = f.FindNextFile();
				CString sPath = f.GetFilePath();
				
				OutputDebugString(sPath);
				OutputDebugString("\n");

				iImage* p = Image_CreateImage(sPath);
				if (p == NULL)
				{
					ASSERT(0);
					continue;
				}
				p->LoadFile(sPath);
				if (item.width == 0 && item.height == 0)
				{
					pImage->AddImage(p);
				}
				else
				{
					p->DivideTo(item.width,item.height);
					pImage->AddImage(p);
				}
				p->Release();
			}
		}

	}
	else
	{
		CString strFind;
		strFind = strFolder + "\\*." + item.strExt;
		CFileFindEx f;
		BOOL b = f.FindFile(strFind);
		if (!b)
		{
			pImage->Release();
			return;
		}

		CWaitCursor cursor;
		while(b)
		{
			b = f.FindNextFile();
			CString sPath = f.GetFilePath();

			if (item.strExt == "spr")
			{
				pImage->LoadFile(sPath);
				break;
			}

			OutputDebugString(sPath);
			OutputDebugString("\n");

			iImage* p = Image_CreateImage(sPath);
			p->LoadFile(sPath);
			
			if (m_bAlphaFormat)
				p->AlphaFormat();
			if (!m_bAlphaSet)
				p->SetAlpha1();

			#pragma message (ATTENTION("HERE A ERROR"))
//			if (item.nFullImage == 1)
//			if (sPath.Find("墙壁45") != -1 || sPath.Find("墙壁135") != -1 )
			//任何图片不做切分.
			
			if (item.width == 0 && item.height == 0)
			{
				pImage->AddImage(p);
			}
			else
			{
				if (p->DivideTo(item.width,item.height))
					pImage->AddImage(p);
				else
				{
					CString strErr;
					strErr.Format("文件'%s'的长宽无法被(%d,%d),切割",sPath,item.width,item.height);
					AfxMessageBox(strErr);
				}
			}
			p->Release();
		}
	}


	stImageInfo info;
	if (item.nSort == 0)
		info.nDirections = 1;
	else
		info.nDirections = dir - 1;

	HRESULT hr = SaveImageFile(pImage,strSpr,&item);
	pImage->Release();
//	if (!FAILED(hr))
//		m_treeDes.AddNode(strSpr);
}

void CResourceManagerDlg::OnLog() 
{
	// TODO: Add your control notification handler code here
	if (m_strSelectFile.IsEmpty())
		return;

	CWaitCursor cursor;
	CString strSpr = m_strSelectFile;
	int pos = FindString(strSpr,m_strDesRoot);
	ASSERT(pos != -1);
	CString sRight = strSpr.Mid(pos+m_strDesRoot.GetLength()+1);
	CString s = m_strNetRoot + "\\" + sRight;
	CString sLog = s.Left(s.GetLength() - 4)+".log";
	CFileFind f;
	if (f.FindFile(sLog))
		ShellExecute(NULL,"open",sLog,NULL,NULL,SW_SHOW);
}

BOOL CResourceManagerDlg::CopyFileToNet(LPCSTR szFile)
{
	if (!g_bCopyToNet)
		return FALSE;
	
	CString strSpr = szFile;
	if (strSpr.IsEmpty())
		return FALSE;

	if (strSpr.Find("test") != -1)
		return FALSE;

	int pos = FindString(strSpr,m_strDesRoot);
	ASSERT(pos != -1);
	CString sRight = strSpr.Mid(pos+m_strDesRoot.GetLength()+1);
	CString s = m_strNetRoot + "\\" + sRight;
	BOOL b = AppCopyFile(strSpr,s);
	if (b)
	{
		CString sLog = s.Left(s.GetLength() - 4)+".log";
		b = AppWriteSimpleLogFile(sLog);
	}
	return b;
}

void CResourceManagerDlg::ConvertFolder(LPCSTR sz)
{
	CWaitCursor cursor;

	CString strFolder = sz;
	int pos = strFolder.ReverseFind('\\');
	ASSERT(pos != -1);
	CString strName = strFolder.Mid(pos+1);
	CString strSpr = strFolder + '\\' + strName + ".spr";
	CString strFind;
	strFind = strFolder + "\\*.tga";
	CFileFind f;
	BOOL b = f.FindFile(strFind);
	if (!b)
	{
		strFind = strFolder + "\\*.bmp";
		b = f.FindFile(strFind);
		if (!b)
			return;
	}
	iImage* pImage = Image_CreateImage(strSpr); 
	while(b)
	{
		b = f.FindNextFile();
		CString sPath = f.GetFilePath();
		iImage* p = Image_CreateImage(sPath);
		p->LoadFile(sPath);
		pImage->AddImage(p);
		p->Release();
	}

//	int n = strSpr.Replace(m_strSrcRoot,m_strDesRoot);
//	ASSERT(n == 1);
	HRESULT hr = SaveImageFile(pImage,strSpr,NULL);
	pImage->Release();
//	if (!FAILED(hr))
//		m_treeDes.AddNode(strSpr);
}


HRESULT CResourceManagerDlg::SaveImageFile(iImage* p,LPCSTR szImage,stOption* pOption)
{
	CString strImage;
	if (pOption != NULL && pOption->sImageFolder != "null")
		GetDesImageFile(szImage,pOption->sImageFolder,strImage);
	else
		strImage = szImage;
	AppBuildPath(strImage);

	stImageInfo Info;
	ZeroMemory(&Info,sizeof(Info));
	if (pOption != NULL)
		Info.nDirections = pOption->nDirection;
	else 
		Info.nDirections = 1;

	HRESULT hr = p->SaveFile(strImage,&Info);
	CopyFileToNet(strImage);

	if (FAILED(hr))
		return hr;
	p->GetImageInfo(Info);

	if (pOption == NULL)
		return hr;

	CString sTxt;
	GetTxtFileNameByImage(szImage,sTxt);

	CString sRelative ;
	GetRelativeFile(strImage,sRelative);
	
	CreateTxtFile(sTxt,pOption->sIcon,pOption->sType,pOption->sGameType,sRelative,
		Info.nFrames,Info.nDirections);

	m_treeDes.AddNode(sTxt);
	return hr;
}

CString CResourceManagerDlg::GetTableDefaultValue(int i, LPCSTR szType)
{
	CString strDefault = "system\\ini\\planedefaultvalue.ini";
	AppGetFullPath(strDefault);
	iItem* pItem = Common_CreateItem(strDefault,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	CString s;
	BOOL b = GetItemValue(pItem,GetTablePlaneName(i),s,szType);
	if (b == false)
		GetItemValue(pItem,GetTablePlaneName(i),s);
	return s;
}

void CResourceManagerDlg::CreateTxtFile(const CString& strFile,const CString& strIcon,const CString& strType, const CString& strGameType, const CString& strSpr,
										int frames, int directions)
{
	CString sValue;
	AppBuildPath(strFile);
	iItem* pItem = Common_CreateItem(strFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"icon",strIcon);
	SetItemValue(pItem,"planedefaulttype",strType);
	SetItemValue(pItem,"type",strGameType);
	{
		CString sPath,sName;
		AppParsePathName(strSpr,sPath,sName);
		SetItemValue(pItem,"file",sName);
	}
//	SetItemValue(pItem,"file",strSpr);
	SetItemValue(pItem,"frames",frames);
	SetItemValue(pItem,"directions",directions);
	long lResourceNum;
	AppGetDynamicNum("resourcenum",lResourceNum);
	SetItemValue(pItem,"resourceid",lResourceNum);

////////////--modify--start--///////////////////////////////
	//yujiang modify 2003 2 15
	//add spr info to template!
	iImage* pImage = Image_CreateImage(strSpr);
	pImage->LoadFile(strSpr);
	for (int i=0; i<frames; i++)
	{
		stFrameInfo info;
		pImage->GetFrameInfo(info,i);
		int n = info.width;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRWIDTH),n,i);
		n = info.height;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRHEIGHT),n,i);
		n = info.xCenterToUpleft;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERX),n,i);
		n = info.yCenterToUpleft;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERY),n,i);
	}
////////////--modify--end--///////////////////////////////

	for ( i=TP_PLANETYPE; i<TP_MODIFY_NUM; i++)
	{
		if (!GetItemValue(pItem,GetTablePlaneName(i),sValue))
		{
			CString s = GetTableDefaultValue(i,strType);
			if (s.IsEmpty())
				break;
			SetItemValue(pItem,GetTablePlaneName(i),s);
		}
	}
	pItem->Save();
	CopyFileToNet(strFile);
}

void CResourceManagerDlg::OnAutoplay() 
{
	UpdateData();	
}

void CResourceManagerDlg::OnStaticSpr() 
{
	CPoint pt;
	BOOL b = m_staticSpr.GetCursorOnImage(pt);
	if (!b)
		return;
	CString s;
	s.Format("%d",pt.x);
	m_aCB[TP_CENTERX]->SetWindowText(s);
	s.Format("%d",pt.y);
	m_aCB[TP_CENTERY]->SetWindowText(s);
}

void CResourceManagerDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect r;
	m_staticSpr.GetWindowRect(r);
	CPoint pt;
	GetCursorPos(&pt);
	if (r.PtInRect(pt))
	{
		OnStaticSpr();
	}
	CDialog::OnLButtonDown(nFlags, point);
}

void CResourceManagerDlg::OnShowcenter() 
{
	UpdateData();
	m_staticSpr.SetShowCenter(m_bShowCenter);
}

void CResourceManagerDlg::SaveAllFolder(LPCSTR szFile) 
{
	CFileFind f;
	CString strFind = szFile;
	strFind += "\\*.txt";
	if (f.FindFile(strFind))
	{
		BOOL b;
		do 
		{
			b = f.FindNextFile();
			CString s = f.GetFilePath();
			SaveAllFile(s);
		}
		while(b);
	}
	strFind = szFile;
	strFind += "\\*.";
	if (f.FindFile(strFind))
	{
		BOOL b;
		do 
		{
			b = f.FindNextFile();
			if (f.IsDots())
				continue;
			CString s = f.GetFilePath();
			SaveAllFolder(s);
		}
		while(b);
	}
}

void CResourceManagerDlg::SaveAllFile(LPCSTR szFile) 
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);

	for (int i=0; i<TP_MODIFY_NUM; i++)
	{
		if ((*m_aAll[i]) == FALSE)
			continue;
		CString sValue;
		pItem->ClearKey(GetTablePlaneName(i));
		DWORD style = m_aCB[i]->GetStyle();
		if (style & WS_DISABLED)
			continue;
		m_aCB[i]->GetWindowText(sValue);
		if (sValue.IsEmpty())
			continue;
		SetItemValue(pItem,GetTablePlaneName(i),sValue);
	}

	pItem->Save();
	CopyFileToNet(szFile);
}

void CResourceManagerDlg::OnSave() 
{
	// TODO: Add your control notification handler code here
	UpdateData();
	CWaitCursor cursor;

	if (m_bAll)
	{
		if (!m_strSelectFolder.IsEmpty())		
		{
			CString sMsg = "确定要改变目录\"" + m_strSelectFolder + "\"下的所有文件么?";
			if (AfxMessageBox(sMsg,MB_OKCANCEL) == IDOK)
				SaveAllFolder(m_strSelectFolder);
		}
		else if (!m_strSelectFile.IsEmpty())
			SaveAllFile(m_strSelectFile);
		return;
	}
	
	if (m_strSelectFile.IsEmpty())
		return;

	UINT frames = m_staticSpr.GetFrames();
	UINT directions = m_staticSpr.GetDirections();
	int index = frames / directions * m_nDirection + m_nFrame;
	ASSERT(index < frames);
	CString sValue;
	iItem* pItem = Common_CreateItem(m_strSelectFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"planedefaulttype",sValue))
		return;

	//clear all 但是重心不改变
	for (int i=0; i<TP_MODIFY_NUM; i++)
	{
		CString sValue;
		DWORD style = m_aCB[i]->GetStyle();
		if (style & WS_DISABLED)
			continue;
		m_aCB[i]->GetWindowText(sValue);
		if (sValue.IsEmpty())
			continue;
		CString sName = GetTablePlaneName(i);
		SetItemValue(pItem,sName,sValue,index);
	}
	pItem->Save();
	CopyFileToNet(m_strSelectFile);

	//为了对付拼命点鼠标的人.
	Sleep(300);
}

void CResourceManagerDlg::UpdatePlaneInfo(int index)
{
	CString sType;
	CString sIcon;
	iItem* pItem = Common_CreateItem(m_strSelectFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"planedefaulttype",sType))
		return;
	if (!GetItemValue(pItem,"icon",sIcon))
		return;
//	if (sValue != "doodad_simple")
//		return;
	CString sValue;
	for (int i=0; i<TP_MODIFY_NUM; i++)
	{
		if (!GetItemValue(pItem,GetTablePlaneName(i),sValue,index))
			sValue = GetTableDefaultValue(i,sType);
		int n = StringToPlaneTableValue(i,sValue);
		if (n != -1)
			m_aCB[i]->SetCurSel(n);
		else
			m_aCB[i]->SetWindowText(sValue);
	}
	ComboxValidate();
}

void CResourceManagerDlg::OnEditchangeCenter() 
{
	CString s;
	CPoint pt;
	m_aCB[TP_CENTERX]->GetWindowText(s);
	StringToValue(s,pt.x);
	m_aCB[TP_CENTERY]->GetWindowText(s);
	StringToValue(s,pt.y);
	m_staticSpr.SetCenter(pt);
}

void CResourceManagerDlg::OnEditchangeCenterx() 
{
	// TODO: Add your control notification handler code here
	CString s;
	m_aCB[TP_CENTERX]->GetWindowText(s);
	CPoint pt;
	m_staticSpr.GetCenter(pt);
	StringToValue(s,pt.x);
	m_staticSpr.SetCenter(pt);
}

void CResourceManagerDlg::OnEditchangeCentery() 
{
	CString s;
	m_aCB[TP_CENTERY]->GetWindowText(s);
	CPoint pt;
	m_staticSpr.GetCenter(pt);
	StringToValue(s,pt.y);
	m_staticSpr.SetCenter(pt);
}

void CResourceManagerDlg::GetSprFileNameByFolder(LPCSTR szFolder, CString& strSpr)
{
	CString strFolder = szFolder;
	int pos = strFolder.ReverseFind('\\');
	ASSERT(pos != -1 && pos != strFolder.GetLength()-1);
	strSpr = strFolder + ".spr";
	int n = strSpr.Replace(m_strSrcRoot,m_strDesRoot);
	ASSERT(n == 1);
}

void GetTxtFileNameByImage(LPCSTR szImage, CString& sTxt)
{
	CString s = szImage;
	int pos = s.ReverseFind('.');
	ASSERT(pos != -1);
	sTxt = s.Left(pos)+".txt";
}

void GetRelativeFile(LPCSTR szImage,CString& sRelative)
{
	CString sWorkingFolder;
	AppGetWorkingFolder(sWorkingFolder);
	CString strImage = szImage;
	int pos = FindString(strImage,sWorkingFolder);
	if (pos != -1)
		sRelative = strImage.Mid(pos + sWorkingFolder.GetLength() + 1);
	else
		sRelative = strImage;
}

void GetDesImageFile(LPCSTR szImage,CString& sRelative,CString& sPathName)
{
	CString strImage = szImage;
	int pos = strImage.ReverseFind('\\');
	ASSERT(pos != -1);
	CString sName = strImage.Mid(pos + 1);
	CString sWork;
	AppGetWorkingFolder(sWork);
	sPathName = sWork + "\\" + sRelative + "\\" + sName;
}

void CResourceManagerDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	KillTimer(TIMER_AUTOPLAY);
}

/*
void CResourceManagerDlg::OnAlphaformat() 
{
	CString strPath;
	if (AppGetPathDialog(strPath))
	{
		AlphaFormatTga(strPath);
	}
}
*/

void CResourceManagerDlg::ComboxValidate()
{
	int index = m_cbPlaneType.GetCurSel();
	PlaneEnum e = (PlaneEnum)index;
	switch (index)
	{
	case PLANE_HORNORMAL:
	case PLANE_HORMESH:
		m_cbLighting.EnableWindow(FALSE);
		m_cbAngleXY.EnableWindow(FALSE);
		m_cbLighted.EnableWindow(FALSE);
		m_cbRadius.EnableWindow(FALSE);
		m_cbVersort.EnableWindow(FALSE);
		m_cbLayer.EnableWindow(TRUE);
		m_cbAniSpeed.EnableWindow(FALSE);
		break;
	case PLANE_VERNORMAL:
		m_cbLighting.EnableWindow(FALSE);
		m_cbAngleXY.EnableWindow(TRUE);
		m_cbLighted.EnableWindow(TRUE);
		m_cbRadius.EnableWindow(TRUE);
		m_cbVersort.EnableWindow(FALSE);
		m_cbLayer.EnableWindow(TRUE);
		m_cbAniSpeed.EnableWindow(TRUE);
		break;
	case PLANE_VERMESH:
	case PLANE_VERDYNAMIC:
		m_cbLighting.EnableWindow(TRUE);
		m_cbAngleXY.EnableWindow(TRUE);
		m_cbLighted.EnableWindow(TRUE);
		m_cbRadius.EnableWindow(TRUE);
		m_cbVersort.EnableWindow(TRUE);
		m_cbLayer.EnableWindow(FALSE);
		m_cbAniSpeed.EnableWindow(FALSE);
		break;
	}
}

void CResourceManagerDlg::OnSelchangePlanetype() 
{
	ComboxValidate();
}

void CResourceManagerDlg::OnGetcenter() 
{
	// TODO: Add your control notification handler code here
	UpdateData();
	if (m_nCenterSetting == -1)
		return;
	CPoint point;
	switch (m_nCenterSetting)
	{
	case CENTER_ALPHA:
		point = GetCenterByAlpha();
		break;
	case CENTER_TOPEST:
		break;
	case CENTER_BOTTOMEST:
		break;
	case CENTER_BOTTOMMIDDLE:
		break;
	case CENTER_IMAGECENTER:
		break;
	}
	CString s;
	s.Format("%d",point.x);
	m_aCB[TP_CENTERX]->SetWindowText(s);
	s.Format("%d",point.y);
	m_aCB[TP_CENTERY]->SetWindowText(s);
}

CPoint CResourceManagerDlg::GetCenterByAlpha() 
{
	int x=0;
	int y=0;
	m_staticSpr.GetAlphaBottom(x,y);
	return CPoint(x,y);
}

////////////////////////////////////////////////////////////////////
//utility
////////////////////////////////////////////////////////////////////
void CResourceManagerDlg::OnSpr2txt() 
{
	CString strPath,sPath;
	CString sWork;
	AppGetWorkingFolder(sWork);
	sWork += "\\游戏资源\\地图动画";
	if (!AppGetFileDialog(sPath,sWork,"*.spr"))
		return;
	CString sName;
	AppParsePathName(sPath, strPath, sName);
	

	CFileFind f;
	CString strFind = strPath + "\\*.spr";
	if (!f.FindFile(strFind))
		return;
	BOOL b;
	do
	{
		b = f.FindNextFile();
		CString sPath = f.GetFilePath();
		CString strSpr = f.GetFilePath();
		CString strTxt = sPath.Left(sPath.GetLength() - 4) + ".txt";
		iItem* pItemTxt = Common_CreateItem(strTxt,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItemTxt);

		iImage* pImage = Image_CreateImage(strSpr);
		if (pImage->LoadFileInfo(strSpr))
		{
			stImageInfo info;
			pImage->GetImageInfo(info);
			if (strTxt.Find("地图动画") != -1)
			{
				CreateTxtFile(strTxt,"doodad","plane_ani","doodad_simple",strSpr,info.nFrames,info.nDirections);
			}
			else
			{
				g_bCopyToNet = FALSE;
				CreateTxtFile(strTxt,"doodad","plane_ani","doodad_simple",strSpr,info.nFrames,info.nDirections);
				g_bCopyToNet = TRUE;
			}
		}
		pImage->Release();
	}while(b);
}

/*
BOOL GetSprFile(CString& strSpr,LPCSTR szIni,LPCSTR szSection)
{
	return false;
}
*/

void CResourceManagerDlg::OnIni2txt(LPCSTR szIni)
{
	//for each section!
	ifstream f(szIni);
	if (!f.is_open())
		return;
	vector<string> m_aString;

	string svalue;
	getline(f,svalue);
	while(!f.eof())
	{
		getline(f,svalue,'\t');
		m_aString.push_back(svalue);
		getline(f,svalue);
	}

	//for each column of the ini!
	CString strPath = szIni;
	CString strName = strPath.Left(strPath.GetLength() - 4);

	CString strTemplate = szIni;
	AppGetShortPath(strTemplate);
	CString strSection;
	for (int i=0; i<m_aString.size(); i++)
	{
		strSection = m_aString.at(i).c_str();
		if (strSection.IsEmpty())
			continue;
		CString strSpr;
		if (!GetSprFile(strSpr,szIni,strSection))
		{
			CString sMessage;
			sMessage.Format("error not found spr file ini '%s' section '%s'\n",szIni,strSection);
			CString strLog = "rm_log.log";
			AppGetFullPath(strLog);
			ofstream f(strLog);
			f << (LPCSTR) sMessage;
			//AfxMessageBox(strSpr);
			strName.MakeLower();
			if (strName.Find("clientonly") != -1)
				strSpr = "system\\spr\\npc.spr";
			else if (strName.Find("npc") != -1)
				strSpr = "system\\spr\\npc.spr";
			else if (strName.Find("trap") != -1)
				strSpr = "system\\spr\\trap.spr";
			else if (strName.Find("box") != -1)
				strSpr = "system\\spr\\box.spr";
			else
				continue;
		}

		CString strTxt = strName + ".txt";
		strTxt.Format("%s_%s.txt",strName,strSection);
		g_DebugLog((LPSTR)(LPCSTR)strTxt);
		iItem* pItemTxt = Common_CreateItem(strTxt,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItemTxt);

		//find spr file of the section!
		iImage* pImage = Image_CreateImage(strSpr);
		if (pImage->LoadFileInfo(strSpr))
		{
			CString strSprNew = strTxt.Left(strTxt.GetLength() - 3) + "spr";
			AppGetFullPath(strSpr);
			AppCopyFile(strSpr,strSprNew);
			g_DebugLog("Copy %s to %s", strSpr , strSprNew);
			CopyFileToNet(strSprNew);
			g_DebugLog("GopyFileToNet1");

			stImageInfo info;
			pImage->GetImageInfo(info);
			g_DebugLog("GetImageInfo");
			strTxt.MakeLower();
   			if (strTxt.Find("clientonlynpc") != -1)
			{
				//use same icon!
				CreateTxtFile(strTxt,"npc","plane_clientonlynpc","doodad_dynamic",strSpr,info.nFrames,info.nDirections);
			}
			if (strTxt.Find("npc") != -1)
			{
				CreateTxtFile(strTxt,"npc","plane_npc","doodad_dynamic",strSpr,info.nFrames,info.nDirections);
			}
			else if (strTxt.Find("trap") != -1)
			{
				CreateTxtFile(strTxt,"trap","plane_trap","doodad_dynamic",strSpr,info.nFrames,info.nDirections);
			}
			else if (strTxt.Find("box") != -1)
			{
				CreateTxtFile(strTxt,"box","plane_box","doodad_dynamic",strSpr,info.nFrames,info.nDirections);
			}
			g_DebugLog("GetTxtFileis %s", strTxt);
			
			iItem* pItemTxt = Common_CreateItem(strTxt,iItem::FLAG_LOAD);
			wrap_ptr<iItem> ptr(pItemTxt);
/*
			int num = 1;
			SetItemValue(pItemTxt,"anispeed",num);
			num = -1;
			SetItemValue(pItemTxt,"centerx",num);
			SetItemValue(pItemTxt,"centery",num);
*/
			SetItemValue(pItemTxt,"template",strTemplate);
			SetItemValue(pItemTxt,"section",strSection);
			pItemTxt->Save();
			CopyFileToNet(strTxt);
		}
		pImage->Release();
	}
	
}

void CResourceManagerDlg::OnIni2txt() 
{
	CWaitCursor cursor;
	
	CString strPath,sPath;
	CString sWork;
	AppGetWorkingFolder(sWork);
	sWork += "\\游戏资源\\动态物体";
	if (!AppGetFileDialog(sPath,sWork,"*.tab"))
		return;
	CString sName;
	AppParsePathName(sPath, strPath, sName);
	
	CFileFind f;
	CString strFind = strPath + "\\*.tab";
	if (!f.FindFile(strFind))
		return;
	BOOL b;
	do
	{
		b = f.FindNextFile();
		CString sPath = f.GetFilePath();
		OnIni2txt(sPath);
	}while(b);	
}
/*
void RemoveShadowFile(LPCSTR szFile)
{
	CString s = szFile;
	OutputDebugString(s);
	OutputDebugString("\n");
	iImage* pImage = Image_CreateImage(szFile);
	pImage->LoadFile(szFile);
	pImage->RemoveShadow();
	pImage->SaveFile(szFile);
	pImage->Release();
}

void RemoveShadow(LPCSTR szFolder)
{
	//RemoveShadowFile(szFolder);
	{
		CString sFile = szFolder;
		sFile += "\\*.spr";
		CFileFindEx f;
		f.SetOnlyDirectory(FALSE);
		f.SetOnlyFile(TRUE);
		if (f.FindFile(sFile))
		{
			BOOL b;
			do
			{
				b = f.FindNextFile();
				RemoveShadowFile(f.GetFilePath());
			}while(b);
		}			
	}

	CString sFile = szFolder;
	sFile += "\\*.";
	CFileFindEx f;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);
	if (f.FindFile(sFile))
	{
		BOOL b;
		do
		{
			b = f.FindNextFile();
			RemoveShadow(f.GetFilePath());
		}while(b);
	}
}

void CResourceManagerDlg::OnShadow() 
{
	if (m_strSelectFolder.IsEmpty())
		return ;
	RemoveShadow(m_strSelectFolder);
}
*/

void CResourceManagerDlg::OnLocate() 
{
	CString s = m_strSelectFolder;
	if (s.IsEmpty())
	{
		s = m_strSelectFile;
		if (s.IsEmpty())
			return;
		CString sPath,sName;
		AppParsePathName(s,sPath,sName); 
		s = sPath;
	}
//	int pos = m_strSelectFolder.Find(m_strDesRoot);
//	ASSERT(pos == 0);
	s.Replace(m_strDesRoot,m_strSrcRoot);
	m_treeSrc.AddNode(s);
}

void CResourceManagerDlg::OnAddsprinfo() 
{
}
////////////--add--end--///////////////////////////////
