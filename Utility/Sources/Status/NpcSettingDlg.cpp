// NpcSettingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "NpcSettingDlg.h"
#include "Kengine.h"
#include "KTabFile.h"
#include "KINiFile.h"
#include "Shlwapi.h"
#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNpcSettingDlg dialog


CNpcSettingDlg::CNpcSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNpcSettingDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNpcSettingDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_TemplateFile = "";
}


void CNpcSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNpcSettingDlg)
	DDX_Control(pDX, IDC_ACTIVEINIFORMCTRL1, m_ActiveIniForm);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNpcSettingDlg, CDialog)
	//{{AFX_MSG_MAP(CNpcSettingDlg)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNpcSettingDlg message handlers

void CNpcSettingDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	int minpos;
	int maxpos;
	
	GetScrollRange(SB_VERT,&minpos, &maxpos); 
	
	maxpos = GetScrollLimit(SB_VERT);
	
	// Get the current position of scroll box.
	int curpos = GetScrollPos(SB_VERT);
	
	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		curpos = minpos;
		break;
		
	case SB_RIGHT:      // Scroll to far right.
		curpos = maxpos;
		break;
		
	case SB_ENDSCROLL:   // End scroll.
		break;
		
	case SB_LINELEFT:      // Scroll left.
		if (curpos == 0) return;
		if (curpos > minpos)
			curpos--;
		break;
		
	case SB_LINERIGHT:   // Scroll right.
		if (curpos == maxpos) return;
		if (curpos < maxpos)
			curpos++;
		break;
		
	case SB_PAGELEFT:    // Scroll one page left.
		{
			// Get the page size. 
			SCROLLINFO   info;
			GetScrollInfo(SB_VERT,&info, SIF_ALL);
			
			if (curpos > minpos)
				curpos = max(minpos, curpos - (int) info.nPage);
		}
		break;
		
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			// Get the page size. 
			SCROLLINFO   info;
			GetScrollInfo(SB_VERT,&info, SIF_ALL);
			
			if (curpos < maxpos)
				curpos = min(maxpos, curpos + (int) info.nPage);
		}
		break;
		
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		curpos = nPos;      // of the scroll box at the end of the drag operation.
		break;
		
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		curpos = nPos;     // position that the scroll box has been dragged to.
		break;
	}
	
	// Set the new position of the thumb (scroll box).
	SetScrollPos(SB_VERT,curpos);	
	OnSize(0,0,0);
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CNpcSettingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNpcSettingDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	int minpos;
	int maxpos;
	
	
	GetScrollRange(SB_HORZ,&minpos, &maxpos); 
	
	maxpos = GetScrollLimit(SB_HORZ);
	
	// Get the current position of scroll box.
	int curpos = GetScrollPos(SB_HORZ);
	
	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		curpos = minpos;
		break;
		
	case SB_RIGHT:      // Scroll to far right.
		curpos = maxpos;
		break;
		
	case SB_ENDSCROLL:   // End scroll.
		break;
		
	case SB_LINELEFT:      // Scroll left.
		if (curpos > minpos)
			curpos--;
		break;
		
	case SB_LINERIGHT:   // Scroll right.
		if (curpos < maxpos)
			curpos++;
		break;
		
	case SB_PAGELEFT:    // Scroll one page left.
		{
			// Get the page size. 
			SCROLLINFO   info;
			GetScrollInfo(SB_HORZ,&info, SIF_ALL);
			
			if (curpos > minpos)
				curpos = max(minpos, curpos - (int) info.nPage);
		}
		break;
		
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			// Get the page size. 
			SCROLLINFO   info;
			GetScrollInfo(SB_HORZ,&info, SIF_ALL);
			
			if (curpos < maxpos)
				curpos = min(maxpos, curpos + (int) info.nPage);
		}
		break;
		
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		curpos = nPos;      // of the scroll box at the end of the drag operation.
		break;
		
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		curpos = nPos;     // position that the scroll box has been dragged to.
		break;
	}
	
	// Set the new position of the thumb (scroll box).
	SetScrollPos(SB_HORZ,curpos);
	OnSize(0,0,0);
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CNpcSettingDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	CRect rectClient;
	GetClientRect (rectClient);
	
	
	if (!(m_ActiveIniForm.GetSafeHwnd())) return ;
	
	int nVPos = GetScrollPos(SB_VERT);	
	int nHPos = GetScrollPos(SB_HORZ);
	
	m_ActiveIniForm.SetWindowPos(NULL, -1 * nHPos * 10, -1 * nVPos * 50 -5 , nHPos * 10 + rectClient.Width() , nVPos * 50 + rectClient.Height() , 0);
	
}

void CNpcSettingDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	OnSize(0,0,0);
	// TODO: Add your message handler code here
	
}

BEGIN_EVENTSINK_MAP(CNpcSettingDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CNpcSettingDlg)
	ON_EVENT(CNpcSettingDlg, IDC_ACTIVEINIFORMCTRL1, 1 /* DataChanged */, OnDataChangedActiveiniformctrl1, VTS_BSTR)
	ON_EVENT(CNpcSettingDlg, IDC_ACTIVEINIFORMCTRL1, 3 /* CtrlDoubleClick */, OnCtrlDoubleClickActiveiniformctrl1, VTS_BSTR)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CNpcSettingDlg::OnDataChangedActiveiniformctrl1(LPCTSTR DataName) 
{
	// TODO: Add your control notification handler code here
	CString strDataName = DataName;
	
	if (strDataName == "Level")  
	{
		CString Value = m_ActiveIniForm.GetData(strDataName);
		KIniFile ExtraIniFile;
		if (ExtraIniFile.Load((LPCSTR)m_ExtraFile))
		{
			ExtraIniFile.WriteString("MAIN", (LPCSTR)strDataName, (LPCSTR)Value);
		}
		else
			//无法读取，新建
		{
			ExtraIniFile.WriteString("MAIN", (LPCSTR)strDataName, (LPCSTR)Value);
		}
		
		ExtraIniFile.Save((LPCSTR)m_ExtraFile);
		return ;
	};

	
	if (strDataName == "NpcResType")//如果是人物类型的话,需要更换其相关的各个部件的可选择的装备列表
	{
		
		KTabFile SysTabFile;
		char PathName[300];
		g_SetFilePath("\\Settings\\NpcRes");
		g_GetFullPath(PathName, "人物类型.txt");
		if (SysTabFile.Load(PathName))
		{
			char UnitFile[100];
			
			SysTabFile.GetString(atoi(m_ActiveIniForm.GetData(DataName)) + 2, "部件说明文件名","",UnitFile, 100);
			//更换该部件
			if (UnitFile[0])
			{
				g_GetFullPath(PathName, UnitFile);
				KTabFile TabFile;
				if (TabFile.Load(PathName))
				{
					for (int i  = 0; i < TabFile.GetHeight() -1; i ++)
					{
						char UnitName[200];//部件组成名称
						char FirstUnitUnit[200];//该部件下的第一个部件
						TabFile.GetString(i + 2, 2, "" , UnitName, 200 );
						TabFile.GetString(i + 2, 3, "" , FirstUnitUnit, 200 );
						char FirstUnitUnitFile[300];//第一个部件所对应的信息文件；需要从该信息文件中获得所包括的装备名称
						SysTabFile.GetString(atoi(m_ActiveIniForm.GetData(DataName)) + 2, FirstUnitUnit, "", FirstUnitUnitFile, 300);
						g_GetFullPath(PathName, FirstUnitUnitFile);
						KTabFile TabFile1;
						CString EquitNameList;//装备名称列表
						if (TabFile1.Load(PathName))
						{
							
							for (int j = 0 ; j < TabFile1.GetHeight() - 1; j ++)
							{
								char EquitName[200];
								TabFile1.GetString(j + 2, 1, "", EquitName, 200);
								if (j  > 0 ) EquitNameList  += "|";
								EquitNameList += EquitName;
							}
							
						}
						char TypeName[200];
						strcpy(TypeName,"");
						if (!strcmp(UnitName,"头盔"))
						{
							strcpy(TypeName,"HelmType");
						}
						
						if (!strcmp(UnitName,"装备"))
						{
							strcpy(TypeName,"ArmorType");
						}
						
						if (!strcmp(UnitName,"武器"))
						{
							strcpy(TypeName,"WeaponType");
						}
						
						
						if (!strcmp(UnitName,"马"))
						{
							strcpy(TypeName,"HorseType");
						}
						
						m_ActiveIniForm.AddCtrl(TypeName, "IndexList", EquitNameList,"", "");
						
						
					}
				}
			}
		}
	}
	
	if (strDataName == "Series")
	{
		KIniFile IniFile;
		char FilePath[500];
		g_SetFilePath("");
		g_GetFullPath(FilePath, "CharacterEdit.ini");
		char szValue[2000];
		CString strSeries = m_ActiveIniForm.GetData(strDataName);
		
		if (IniFile.Load(FilePath))
		{
			IniFile.GetString("SKILLLIST", strSeries.GetBuffer(200), "", szValue, 2000);
			m_ActiveIniForm.AddCtrl("Skill1", "StringList",  szValue,"","");
			m_ActiveIniForm.AddCtrl("Skill2", "StringList",  szValue,"","");
			m_ActiveIniForm.AddCtrl("Skill3", "StringList",  szValue,"","");
			m_ActiveIniForm.AddCtrl("Skill4", "StringList",  szValue,"","");
		}
	}
	

	if (!m_bLoadingTemplate)
	{
	
		CString Value = m_ActiveIniForm.GetData(strDataName);

	char FilePath[200];
	
	CString szTemplateFile = m_TemplateFile;
	szTemplateFile.MakeUpper();

	if (strstr(m_TemplateFile, "NPC"))
	{
		strcpy(FilePath, "\\Settings\\Npcs.txt");
	}
	else if  (strstr(m_TemplateFile,"BOX"))
	{
		strcpy(FilePath, "\\Settings\\Obj\\ObjData.txt");
	}
	else
		strcpy(FilePath , (LPCSTR)m_TemplateFile);

	KTabFile TempIniFile;
	
	if (!TempIniFile.Load(FilePath)) return ;
	char szValue[200];
	
	if (TempIniFile.GetString((LPSTR)((LPCSTR)(m_Section)),(LPSTR)((LPCSTR)strDataName), "", szValue,200))
	{
		//模板值与当前值不同,则修改Extra文件
		if (Value != szValue) 
		{
			KIniFile ExtraIniFile;
			if (ExtraIniFile.Load(m_ExtraFile.GetBuffer(200)))
			{
				ExtraIniFile.WriteString("1", (LPCSTR)strDataName, (LPCSTR)Value);
			}
			else
				//无法读取，新建
			{
				ExtraIniFile.WriteString("1", (LPCSTR)strDataName, (LPCSTR)Value);
			}
			
			ExtraIniFile.Save((LPCSTR)m_ExtraFile);
		}
		else //如果相同则删除Extra文件中的该KeyName
		{
			KIniFile ExtraIniFile;
			if (ExtraIniFile.Load((LPCSTR)m_ExtraFile))
			{
				ExtraIniFile.EraseKey("1", (LPCSTR)strDataName);
			}
			ExtraIniFile.Save((LPCSTR)m_ExtraFile);					
		}
			
	}
	
	}
	
	Invalidate(FALSE);
}

void CNpcSettingDlg::OnCtrlDoubleClickActiveiniformctrl1(LPCTSTR DataName) 
{
	// TODO: Add your control notification handler code here
	CString strDataName = DataName;
	
	if (strDataName == "Script")
	{
		MessageBox("Please Using Alt + DoubleClick To Set TrapScript!");
	}
	if (strDataName == "ActionScript" || strDataName == "AIScript" 		)
	{
		CFileDialog filedlg(TRUE);
		if (filedlg.DoModal() == IDOK)
		{
			char  PathName[400];
			char FullPath[300];
			CString sWork;
			AppGetWorkingFolder(sWork);
			PathRelativePathTo(PathName,sWork, FILE_ATTRIBUTE_DIRECTORY, filedlg.GetPathName(), FILE_ATTRIBUTE_NORMAL );
			m_ActiveIniForm.ModifyData(DataName,PathName, PathName);
		}
		
	}
	
	if (strDataName == "ImageName" || strDataName == "SoundName" || strDataName == "ScriptName")
	{
		CFileDialog filedlg(TRUE);
		char  PathName[400];
		char FullPath[300];
		if (filedlg.DoModal() == IDOK)
		{
			
			CString sWork;
			AppGetWorkingFolder(sWork);

			PathRelativePathTo(PathName,sWork, FILE_ATTRIBUTE_DIRECTORY, filedlg.GetPathName(), FILE_ATTRIBUTE_NORMAL );
			m_ActiveIniForm.ModifyData(DataName,PathName, PathName);
		}
	}
	

}