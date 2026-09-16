// WorldEditorView.cpp : implementation of the CWorldEditorView class
//

#include "stdafx.h"
#include "WorldEditor.h"

#include "WorldEditorDoc.h"
#include "WorldEditorView.h"
#include "iitem.h"
#include "Mainfrm.h"
#include "fileop.h"

#include "iStatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView
const int TIMER_AUTOPLAY = 1002;

IMPLEMENT_DYNCREATE(CWorldEditorView, CFormView)

BEGIN_MESSAGE_MAP(CWorldEditorView, CFormView)
	//{{AFX_MSG_MAP(CWorldEditorView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_FRAME, OnChangeFrame)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_DIRECTION, OnChangeDirection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView construction/destruction

CWorldEditorView::CWorldEditorView()
	: CFormView(CWorldEditorView::IDD)
{
	//{{AFX_DATA_INIT(CWorldEditorView)
	m_nFrame = 0;
	m_strAnimation = _T("");
	m_bAutoPlay = FALSE;
	m_nDirection = 0;
	m_nSpeed = 10;
	//}}AFX_DATA_INIT
	// TODO: add construction code here
	m_bCreate = false;
}

CWorldEditorView::~CWorldEditorView()
{
}

void CWorldEditorView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWorldEditorView)
	DDX_Control(pDX, IDC_SPIN_SPEED, m_spinSpeed);
	DDX_Control(pDX, IDC_LIST, m_listOutput);
	DDX_Control(pDX, IDC_SPIN_FRAME, m_spinFrame);
	DDX_Control(pDX, IDC_SPIN_DIRECTION, m_spinDirection);
	DDX_Control(pDX, IDC_SPIN_ANIMATION, m_spinAnimation);
	DDX_Control(pDX, IDC_DIR, m_treeDir);
	DDX_Control(pDX, IDC_STATIC_SPR, m_staticSpr);
	DDX_Text(pDX, IDC_FRAME, m_nFrame);
	DDX_Text(pDX, IDC_ANIMATION, m_strAnimation);
	DDX_Check(pDX, IDC_AUTOPLAY, m_bAutoPlay);
	DDX_Text(pDX, IDC_DIRECTION, m_nDirection);
	DDX_Text(pDX, IDC_SPEED, m_nSpeed);
	//}}AFX_DATA_MAP
}

BOOL CWorldEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.cx = c_nScreenWidth - cs.x;
	cs.cy = c_nScreenHeight - cs.y;

	return CFormView::PreCreateWindow(cs);
}

void CWorldEditorView::CreateTree()
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

	m_treeDir.SetGetIconByFile(true);
	m_treeDir.ConstructIcon(&m_listNormal);

	CString sIni = "users\\we_folder.ini";
	AppGetFullPath(sIni);
	iItem* pItem = Common_CreateItem(sIni,iItem::FLAG_LOAD);
	if (pItem != NULL)
	{
		wrap_ptr<iItem> ptr(pItem);
		int index;
		CString strValue;
		if (GetItemValue(pItem,"select",index) 
			&& GetItemValue(pItem,"folders",strValue,index))
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
				m_treeDir.AddLoadFolder(strFolder);
				strValue = strValue.Mid(pos + 1);
			}while(!strValue.IsEmpty());
		}
		else
		{
			AfxMessageBox("file: we_folder.ini error!");
		}
	}

	CString s = "游戏资源";
	AppGetFullPath(s);
	m_treeDir.ConstructTree(s,"txt");

	CString sini = "users\\we_tree_quickkey.txt";
	AppGetFullPath(sini);
	m_treeDir.LoadQuickKey(sini);
}

void CWorldEditorView::AfterCreate()
{
	if (m_bCreate)
		return;

	CreateTree();

	CRect rect;
	CRect r;
	GetWindowRect(r);
	m_listOutput.GetWindowRect(rect);
	rect.right = c_nScreenWidth - 20;
	rect.top -= r.top;
	rect.bottom -= r.top;
	rect.left -= r.left;
	rect.bottom -= r.left;
	m_listOutput.MoveWindow(rect);

	m_listOutput.Create();
	m_spinSpeed.SetRange(0,20);

	SetTimer(TIMER_AUTOPLAY,100/4,NULL);

	m_bCreate = true;
}

void CWorldEditorView::OnInitialUpdate()
{
	if (m_bCreate)
		return;
	
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	AfterCreate();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView diagnostics

#ifdef _DEBUG
void CWorldEditorView::AssertValid() const
{
	CFormView::AssertValid();
}

void CWorldEditorView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CWorldEditorDoc* CWorldEditorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWorldEditorDoc)));
	return (CWorldEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView message handlers
BOOL CWorldEditorView::OnRightSelectedFile(LPCSTR szFile)
{
	return TRUE;
}

BOOL CWorldEditorView::OnSelectedFile(LPCSTR szFile)
{
	CString s = szFile;
	if (s.Right(4) != ".txt")
		return FALSE;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (!pItem)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	pItem->Refresh();
	CString sValue;
	BOOL b = GetItemValue(pItem,"file",sValue);
	if (!b)
		return TRUE;
	sValue = szFile;
	sValue = sValue.Left(sValue.GetLength() - 4) + ".spr";

//	for the file give a relative path
	if (!m_staticSpr.SetSprFile(sValue))
	{
		CString strMsg;
		strMsg.Format("file %s 和 file %s 的关联出现错误,无法使用该文件!", szFile, (LPCSTR)sValue);
		AfxMessageBox(strMsg);
		return FALSE;
	}
	m_staticSpr.SetFrame(0,0);
	int frames = m_staticSpr.GetFrames();
	int directions = m_staticSpr.GetDirections();
	m_spinFrame.SetRange(0,frames / directions - 1);
	m_spinDirection.SetRange(0,directions-1);
//	SetObjParam(GetSelect(),param);
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
	return TRUE;
}

LRESULT CWorldEditorView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	CDirTree* pTree = NULL;
	switch (message)
	{
	case WM_DIRTREE_LEFTSELECTED:
		ASSERT(lParam != 0);
		if (OnSelectedFile((LPCSTR)wParam))
		{
			theApp.GetMainFrame()->OnLeftSelectedFile((LPCSTR)wParam,0);
			pTree = (CDirTree*) lParam;
			pTree->UpdateSelectedNode();
		}
		break;
	case WM_DIRTREE_RIGHTSELECTED:
		ASSERT(lParam != 0);
		OnRightSelectedFile((LPCSTR)wParam);
		theApp.GetMainFrame()->OnRightSelectedFile((LPCSTR)wParam,0);
		pTree = (CDirTree*) lParam;
		pTree->UpdateSelectedNode();
		break;	
	}	
	return CFormView::WindowProc(message, wParam, lParam);
}

int CWorldEditorView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFormView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
//	AfterCreate();
	
	return 0;
}

void CWorldEditorView::OnDestroy() 
{
	KillTimer(TIMER_AUTOPLAY);
	CFormView::OnDestroy();
}

HRESULT CWorldEditorView::OutputMessage(LPCSTR szMessage)
{
	return 0;	
}

void CWorldEditorView::SetFrame()
{
	m_staticSpr.SetFrame(m_nDirection,m_nFrame);
	theApp.GetStatusMachine()->OnFrameChange(m_staticSpr.GetiFrame());
}

void CWorldEditorView::OnChangeFrame() 
{
	if (m_bCreate)
	{
		UpdateData();
		SetFrame();
	}
}
void CWorldEditorView::OnChangeDirection() 
{
	if (m_bCreate)
	{
		UpdateData();
		SetFrame();
	}	
}

void CWorldEditorView::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == TIMER_AUTOPLAY)
	{
		static int nFrame = 0;
		UpdateData();
		if (m_bAutoPlay)
		{
			nFrame++;
			if (nFrame < m_nSpeed)
				return;
			nFrame = 0;
			int frames = m_staticSpr.GetFrames();
			int directions = m_staticSpr.GetDirections();
			m_nFrame++;
			if (m_nFrame >= frames / directions)
			{
				m_nFrame = 0;
				m_nDirection ++;
				if (m_nDirection >= directions)
					m_nDirection = 0;
			}
			UpdateData(FALSE);
			SetFrame();
		}

		if (theApp.GetStatusMachine())
		{
			CString s = theApp.GetStatusMachine()->GetNewFile();
			if (!s.IsEmpty())
			{
				m_treeDir.AddNode(s);
				theApp.GetStatusMachine()->ClearNewFile();
			}
		}
	}
	CFormView::OnTimer(nIDEvent);
}

