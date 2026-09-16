// ProcessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "gmdebug.h"
#include "ProcessDlg.h"
#include "WinSock2.h"

#include "GMDebugDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProcessDlg dialog


CProcessDlg::CProcessDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProcessDlg)
	m_strAccount = _T("");
	m_strAction = _T("");
	m_strRole = _T("");
	m_strIP = _T("");
	//}}AFX_DATA_INIT

	m_nSendIP = INADDR_BROADCAST;
}


void CProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcessDlg)
	DDX_Control(pDX, IDC_LIST_RELAY, m_RelayList);
	DDX_Control(pDX, IDC_LIST_ERROR, m_ErrorList);
	DDX_Text(pDX, IDC_EDIT_ACCOUNT, m_strAccount);
	DDX_Text(pDX, IDC_EDIT_ACTION, m_strAction);
	DDX_Text(pDX, IDC_EDIT_ROLE, m_strRole);
	DDX_Text(pDX, IDC_EDIT_IP, m_strIP);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProcessDlg, CDialog)
	//{{AFX_MSG_MAP(CProcessDlg)
	ON_BN_CLICKED(IDC_BUTTON_FREEZE, OnButtonFreeze)
	ON_BN_CLICKED(IDC_BUTTON_COUNT, OnButtonCount)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_UNLOCK, OnButtonUnlock)
	ON_BN_CLICKED(IDC_BUTTON_RETURN, OnButtonReturn)
	ON_BN_CLICKED(IDC_RADIO_BROAD, OnRadioBroad)
	ON_BN_CLICKED(IDC_RADIO_IP, OnRadioIp)
	ON_BN_CLICKED(IDC_RADIO_RELAY, OnRadioRelay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcessDlg message handlers

void CProcessDlg::OnButtonFreeze() 
{
	CGMDebugApp* pApp = (CGMDebugApp*)AfxGetApp();
	if (pApp == NULL)
		return;
	UpdateData(TRUE);
	if (m_strAccount.IsEmpty())
		return;
	pApp->FreezeAccount(m_strAccount);
}

void CProcessDlg::OnButtonCount() 
{
	CGMDebugApp* pApp = (CGMDebugApp*)AfxGetApp();
	if (pApp == NULL)
		return;

	CPtrList docs;
	pApp->GetDocs(docs);
	POSITION pos = docs.GetHeadPosition();
	while (pos)
	{
		CGMDebugDoc* pDoc = (CGMDebugDoc*)docs.GetNext(pos);
		if (pDoc)
		{
			pApp->SendOnlinePlayerCount(pDoc->m_strDocName);
		}
	}

	pApp->SendRegisterUserCount();
}

void CProcessDlg::OnButtonRefresh() 
{
	CGMDebugApp* pApp = (CGMDebugApp*)AfxGetApp();
	if (pApp == NULL)
		return;

	m_RelayList.ResetContent();
	CPtrList docs;
	pApp->GetDocs(docs);
	POSITION pos = docs.GetHeadPosition();
	while (pos)
	{
		CGMDebugDoc* pDoc = (CGMDebugDoc*)docs.GetNext(pos);
		if (pDoc)
		{
			m_RelayList.InsertString(-1, pDoc->GetTitle());
		}
	}
}

void CProcessDlg::OnButtonSend() 
{
	UpdateData(TRUE);
	if (m_strAction.IsEmpty())
	{
		AfxMessageBox("脚本指令不能为空!", MB_OK);
		return;
	}

	CString strOld = m_strAction;
	int nReturn = strOld.Find("\r\n");
	while (nReturn >= 0)
	{
		m_strAction = strOld.Left(nReturn);
		Send(0);
		strOld = strOld.Right(strOld.GetLength() - m_strAction.GetLength() - 2);
		nReturn = strOld.Find("\r\n");
	}
	m_strAction = strOld;
	Send(0);
}

void CProcessDlg::OnButtonReturn() 
{
	UpdateData(TRUE);
	if (m_strAction.IsEmpty())
	{
		AfxMessageBox("脚本指令不能为空!", MB_OK);
		return;
	}
	if (m_strAction.GetLength() > 128)
	{
		AfxMessageBox("脚本指令长度超过128!", MB_OK);
		return;
	}
	static DWORD xReturn = 0;
	CString strOld = m_strAction;
	int nReturn = strOld.Find("\r\n");
	while (nReturn >= 0)
	{
		m_strAction = strOld.Left(nReturn);
		xReturn++;
		Send(xReturn);
		strOld = strOld.Right(strOld.GetLength() - m_strAction.GetLength() - 2);
		nReturn = strOld.Find("\r\n");
	}
	m_strAction = strOld;
	xReturn++;
	Send(xReturn);
}

void CProcessDlg::Send(DWORD nReturn)
{
	CGMDebugApp* pApp = (CGMDebugApp*)AfxGetApp();
	if (pApp == NULL)
		return;
	if (m_strAction.IsEmpty())
	{
		return;
	}
	if (m_strAction.GetLength() > 128)
	{
		return;
	}

	if (m_strAccount.IsEmpty())
	{
		int nSel = m_RelayList.GetCurSel();
		DWORD nRelayID = -1;

		if (nSel >= 0)
		{
			CString strDoc;
			m_RelayList.GetText(nSel, strDoc);
			CPtrList docs;
			pApp->GetDocs(docs);
			POSITION pos = docs.GetHeadPosition();
			while (pos)
			{
				CGMDebugDoc* pDoc = (CGMDebugDoc*)docs.GetNext(pos);
				if (pDoc && pDoc->GetTitle() == strDoc)
				{
					nRelayID = pDoc->m_docRelayID;
					break;
				}
			}
		}

		if (m_strRole.IsEmpty())
		{
			if (nRelayID == -1)	//帐号角色都为空,且没有选择服务器,不能执行
				return;

			int nCheck = GetCheckedRadioButton(IDC_RADIO_RELAY, IDC_RADIO_IP);
			if (nCheck == IDC_RADIO_RELAY)
				m_nSendIP = 0;
			else if (nCheck == IDC_RADIO_BROAD)
				m_nSendIP = INADDR_BROADCAST;
			else
				m_nSendIP = inet_addr(m_strIP);
			//向指定服务器组执行脚本
			pApp->ExecuteAction(nRelayID, m_strAction, nReturn, m_nSendIP);
		}
		else
		{
			if (nRelayID == -1)
				return;	//指定了角色名,没有指定服务器,不能执行
			
			//按照ID和角色名执行脚本
			pApp->ExecuteAction(m_strRole, nRelayID, m_strAction, nReturn);
		}
	}
	else	//按照帐号名执行脚本
	{
		pApp->ExecuteAction(m_strAccount, m_strAction, nReturn);
	}
}

void CProcessDlg::OnButtonUnlock() 
{
	CGMDebugApp* pApp = (CGMDebugApp*)AfxGetApp();
	if (pApp == NULL)
		return;
	UpdateData(TRUE);
	if (m_strAccount.IsEmpty())
		return;
	pApp->UnlockPlayerAccount(m_strAccount);
}

BOOL CProcessDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CheckRadioButton(IDC_RADIO_RELAY, IDC_RADIO_IP, IDC_RADIO_BROAD);
	
	OnButtonRefresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProcessDlg::OnRadioBroad() 
{
	CheckRadioButton(IDC_RADIO_RELAY, IDC_RADIO_IP, IDC_RADIO_BROAD);
}

void CProcessDlg::OnRadioIp()
{
	CheckRadioButton(IDC_RADIO_RELAY, IDC_RADIO_IP, IDC_RADIO_IP);
}

void CProcessDlg::OnRadioRelay() 
{
	CheckRadioButton(IDC_RADIO_RELAY, IDC_RADIO_IP, IDC_RADIO_RELAY);
}
