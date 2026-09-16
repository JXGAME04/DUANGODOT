// cStatusDynamic.cpp: implementation of the cStatusDynamic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusDynamic.h"
#include "..\controls\FileFindEx.h"
#include "iItem.h"
#include "objutility.h"
#include "iGraphicsUtility.h"
#include "fileop.h"
#include "iCommand.h"
#include "Shlwapi.h"
#include "kengine.h"
#include "NpcSettingDlg.h"
#include "KTabFile.h"
#include "KTabFileCtrl.h"
#include "objDlg.h"

#define PATH_NPCSETTING		"\\Settings"
#define PATH_NPCRESSETTING	"\\Settings\\NPCRES"
#define PATH_OBJSETTING		"\\Settings"
#define PATH_TRAPSETTING	"\\Settings"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//注意：现在npc的nick不能改变，因为nick是一个指向_data.ini的指针
//除非，改变nick的时候同时改变_data.ini!
/*
BOOL cStatusDynamic::RenameNick(iObj* pPlane,LPCSTR szNewNick)
{
	if (pPlane == NULL || strlen(szNewNick) == 0)
		return FALSE;

	DynamicEnum e = GetObjDynamic(pPlane);
	if (e == DYNAMIC_STATIC)
	{
		
	}
	else
	{
		CString strIni;
		GetObjIniFile(pPlane,strIni);
		CString strNick = szNewNick;
		CString strIniNew;
		GetIniFile(m_strEvent,szNewNick,strIniNew);
		CFileFind f;
		if (f.FindFile(strIniNew))
		{
			AfxMessageBox("已经有同样的名称了!");
			return FALSE;
		}
		AppRenameFolder(strIni,strIniNew);
	}	

	stPlaneParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_EVENT);
	strcpy(param.szEvent,szNewNick);
	pPlane->setParam(param);
	return TRUE;
}
*/
cStatusDynamic::cStatusDynamic()
{
	m_pSettingDlg = NULL;

	m_stSelect.eType = SelectType_Dynamic;
	m_stSelect.eFilter = SelectFilter_OneByDisplay;
	m_stSelect.eFilterParam = SelectFilterParam_Default;
	m_stSelect.eKind = KIND_NORMAL;
	m_stSelect.eSort = SORT_VER;
//	m_bAutoPlay = FALSE;
	m_nNpc = 0;
	m_nBox = 0;
	
	m_bModify = FALSE;
}

cStatusDynamic::~cStatusDynamic()
{
	if (m_pSettingDlg)
		delete m_pSettingDlg;
	SaveID();
}

void cStatusDynamic::SaveID()
{
	CString sIni = "游戏资源\\动态物体\\dynamic.ini";
	AppGetFullPath(sIni);
	iItem* pItem = Common_CreateItem(sIni,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"npcnum",m_nNpc);
	SetItemValue(pItem,"boxnum",m_nBox);
	pItem->Save();
}

void cStatusDynamic::ObjPropertyExtra(iObj* pPlane)
{
	if (pPlane == NULL)
		return;

	CString sIniFile;
	if (!GetObjIniFile(pPlane,sIniFile))
	{
		AfxMessageBox("请检查事件文件或者obj的名称是否为空");
		return;
	}
	CString strTemplateFile,strSection;
	if (!GetObjTemplate(pPlane,strTemplateFile,strSection))
	{
		AfxMessageBox("尚未指定obj的模板");
		return;
	}
	if (strTemplateFile.IsEmpty())
	{
		AfxMessageBox("尚未指定obj的模板");
		return;
	}
	AppGetFullPath(strTemplateFile);
	AppGetFullPath(sIniFile);
//	CString sScriptFile;
//	GetObjScriptFile(pPlane,sScriptFile);

	switch(GetObjDynamic(pPlane))
	{
	case DYNAMIC_NPC:
	case DYNAMIC_CLIENTONLYNPC:
		{
			//						模板文件		种类		数据保存在
			ShowNpcSetting(pPlane,strTemplateFile,strSection,sIniFile);
		}
		break;
	case DYNAMIC_BOX:
	case DYNAMIC_CLIENTONLYBOX:
		{
			ShowBoxSetting(pPlane,strTemplateFile,strSection,sIniFile);
		}
		break;
	case DYNAMIC_TRAP:
	case DYNAMIC_CLIENTONLYTRAP:
		{
			ShowTrapSetting(pPlane,strTemplateFile,strSection,sIniFile);
		}
		break;
	}
}

BOOL cStatusDynamic::ShowNpcSetting(iObj* pPlane,const CString& strTemplateFile, const CString& strSection,const CString& strObjFile)
{
	char FilePath[200];
	CRect  rc(0,0,300,400);
	
	if (m_pSettingDlg)		m_pSettingDlg->GetWindowRect(&rc);

	if (m_pSettingDlg && (m_pSettingDlg->m_CurSaveType != DYNAMIC_NPC))
	{
		if (m_pSettingDlg)
			m_pSettingDlg->ShowWindow(SW_HIDE);
		
		delete m_pSettingDlg;
		m_pSettingDlg = NULL;
	}

	int nOldVersionLevel = 0;
	KIniFile ObjIniFile;
	if (ObjIniFile.Load(strObjFile))
	{
		ObjIniFile.GetInteger("1", "Level", 0, &nOldVersionLevel);
		ObjIniFile.EraseKey("1", "Level");
		ObjIniFile.Save(strObjFile);
	}

	if (!m_pSettingDlg)
	{
		m_pSettingDlg = new CNpcSettingDlg;
		m_pSettingDlg->Create(IDD_NPCSETTING);
		m_pSettingDlg->ShowWindow(SW_HIDE);
		g_SetFilePath("\\");
		g_SetFilePath(PATH_NPCSETTING);
		g_GetFullPath(FilePath,"CharSetting.ini");
		m_pSettingDlg->m_ActiveIniForm.LoadTemplate(FilePath);
	
	m_pSettingDlg->m_bLoadingTemplate = TRUE;
	g_SetFilePath("\\");
	g_SetFilePath(PATH_NPCRESSETTING);
	KTabFile SysTabFile;
	KTabFile TabFile;
	
	
	
	g_GetFullPath(FilePath,"人物类型.txt");
	if (!SysTabFile.Load(FilePath)) 
	{
		ASSERT("无法打开人物类型.txt 文件，请确认是否已将角色有关的文件考至正确的目录下!");
		m_pSettingDlg = FALSE;
		return FALSE;
	}
	
	if (SysTabFile.Load(FilePath))
	{
		CString TypeList;
		for (int i  = 0 ;i < SysTabFile.GetHeight() - 1; i ++)
		{
			char Type[100];
			
			if (SysTabFile.GetString(i + 2,1,"",Type, 100 ))
			{
				if (i >0) TypeList += "|";
				TypeList += Type;
			}
		}
		m_pSettingDlg->m_ActiveIniForm.AddCtrl("NpcResType", "StringList", TypeList.GetBuffer(TypeList.GetLength()), "", "");
	}
	
	
	g_GetFullPath(FilePath, "Npc动作表.txt");
	//加载Npc动作表
	if (TabFile.Load(FilePath))
	{
		CString ActionList;
		for (int i  = 0 ;i < TabFile.GetHeight() - 1; i ++)
		{
			char Action[100];
			if (TabFile.GetString(i + 2,1,"",Action, 100))
			{
				if (i >0) ActionList += "|";
				ActionList += Action;
			}
		}
		m_pSettingDlg->m_ActiveIniForm.AddCtrl("动作", "IndexList", ActionList.GetBuffer(ActionList.GetLength()), "", "");
		m_pSettingDlg->m_ActiveIniForm.SetCtrlRecord("动作", FALSE);
		m_pSettingDlg->m_ActiveIniForm.SetCtrlPos("动作",7);
		TabFile.Clear();
	}
	
	m_pSettingDlg->m_ActiveIniForm.AddCtrl("RideHorse", "IndexList", "不骑马|骑马","","");
	m_pSettingDlg->m_ActiveIniForm.SetCtrlPos("RideHorse", 8);
	m_pSettingDlg->OnDataChangedActiveiniformctrl1("NpcResType");
		
	}
	
	m_pSettingDlg->m_bLoadingTemplate = TRUE;
	g_SetFilePath("");
	
///	m_pSettingDlg->m_ActiveIniForm.ModifyData("Level", "10", "10");
	
	//m_pSettingDlg->m_ActiveIniForm.Load((LPCSTR)strTemplateFile, 1, (LPCSTR)strSection, TRUE);
	m_pSettingDlg->m_ActiveIniForm.Load("\\Settings\\Npcs.txt", 1, (LPCSTR)strSection, TRUE);
	
	CString OldSeries = m_pSettingDlg->m_ActiveIniForm.GetData("Series");
		
	m_pSettingDlg->m_TemplateFile = strTemplateFile;
	
	ASSERT(!strObjFile.IsEmpty());
	g_SetFilePath("");
	//g_GetFullPath(FilePath, strObjFile.GetBuffer(200));
	m_pSettingDlg->m_ExtraFile = strObjFile;
	m_pSettingDlg->m_Section = strSection;
	
	m_pSettingDlg->m_ActiveIniForm.Load(strObjFile, 0, "1",TRUE);
	
	CString ActSeries = m_pSettingDlg->m_ActiveIniForm.GetData("Series");
	m_pSettingDlg->OnDataChangedActiveiniformctrl1("Series");
	
	KTabFile TempTabFile;

	if (OldSeries != ActSeries)//不一样，则
		m_pSettingDlg->m_ActiveIniForm.Load(strObjFile, 0, "1",TRUE);
	else
	{
		m_pSettingDlg->m_ActiveIniForm.Load("\\Settings\\Npcs.txt", 1, (LPCSTR)strSection, TRUE);
		
		m_pSettingDlg->m_ActiveIniForm.Load(strObjFile, 0, "1",TRUE);
	}
	
	KIniFile TempIniFile;
	if(TempIniFile.Load((LPCSTR)strObjFile))
	{
		char szLevel[100];
		TempIniFile.GetString("MAIN", "Level", "", szLevel, 100);
		if (nOldVersionLevel > 0 && (!szLevel[0]) )
		{
			itoa(nOldVersionLevel, szLevel, 10);
			TempIniFile.WriteInteger("MAIN", "Level", nOldVersionLevel);
		}

		m_pSettingDlg->m_ActiveIniForm.ModifyData("Level", szLevel, "");
		TempIniFile.WriteInteger("MAIN", "Type", 0);
		TempIniFile.Save((LPCSTR)strObjFile);
	}
	
	m_pSettingDlg->m_bLoadingTemplate = FALSE;	

	m_pSettingDlg->m_CurSaveType = DYNAMIC_NPC;
	m_pSettingDlg->SetWindowPos(&CWnd::wndTopMost ,rc.left,rc.top,rc.Width(),rc.Height(),0);
	m_pSettingDlg->ShowWindow(SW_SHOW);
	return	TRUE;
}

BOOL cStatusDynamic::ShowBoxSetting(iObj* pPlane,const CString& strTemplateFile, const CString& strSection,const CString& strObjFile)
{

	char FilePath[200];
	CRect  rc(0,0,300,400);
	
	if (m_pSettingDlg)		m_pSettingDlg->GetWindowRect(&rc);

	if (m_pSettingDlg && (m_pSettingDlg->m_CurSaveType != DYNAMIC_BOX))
	{
		if (m_pSettingDlg)
			m_pSettingDlg->ShowWindow(SW_HIDE);
		
		delete m_pSettingDlg;
		m_pSettingDlg = NULL;
	}
	
	if (!m_pSettingDlg)
	{
		m_pSettingDlg = new CNpcSettingDlg;
		m_pSettingDlg->Create(IDD_NPCSETTING);
		m_pSettingDlg->ShowWindow(SW_HIDE);
		
		g_SetFilePath(PATH_OBJSETTING);
		g_GetFullPath(FilePath,"ObjSetting.ini");
		m_pSettingDlg->m_ActiveIniForm.LoadTemplate(FilePath);
		m_pSettingDlg->m_bLoadingTemplate = TRUE;

	}
	
	m_pSettingDlg->m_bLoadingTemplate = TRUE;
	g_SetFilePath("");
	
	m_pSettingDlg->m_ActiveIniForm.Load("\\Settings\\Obj\\ObjData.txt",1, (LPCSTR) strSection, TRUE);
	m_pSettingDlg->m_TemplateFile = strTemplateFile;
	
	ASSERT(!strObjFile.IsEmpty());
	
	m_pSettingDlg->m_ExtraFile = strObjFile;

	m_pSettingDlg->m_ActiveIniForm.Load((LPCSTR)strObjFile,0,"1",TRUE);
	m_pSettingDlg->m_bLoadingTemplate = FALSE;	
	
	KIniFile TempIniFile;
	if(TempIniFile.Load((LPCSTR)strObjFile))
	{
		TempIniFile.WriteInteger("MAIN", "Type", 1);
		TempIniFile.Save((LPCSTR)strObjFile);
	}
	
	m_pSettingDlg->m_CurSaveType = DYNAMIC_BOX;
	m_pSettingDlg->SetWindowPos(&CWnd::wndTopMost ,rc.left,rc.top,rc.Width(),rc.Height(),0);
	m_pSettingDlg->ShowWindow(SW_SHOW);
	return TRUE;
}

BOOL cStatusDynamic::ShowTrapSetting(iObj* pPlane,const CString& strTemplateFile, const CString& strSection,const CString& strObjFile)
{
	char FilePath[200];
	long id;
	GU_GetObjID(pPlane,id);
	CString strFolder = m_strDocument.Left(m_strDocument.GetLength() - 4);
	KTabFileCtrl TabFile;
	CString strScriptTab = strFolder;
	strScriptTab += "\\";
	strScriptTab += "ScriptList.txt";
	m_strTrapScript = strScriptTab;
	TabFile.Load((LPSTR)(LPCSTR)strScriptTab);

	char szScriptFile[200];
	TabFile.GetString(id + 1, 2, "", szScriptFile, 200);

	CRect  rc(0,0,300,400);
	
	if (m_pSettingDlg)		m_pSettingDlg->GetWindowRect(&rc);
	
	if (m_pSettingDlg && (m_pSettingDlg->m_CurSaveType != DYNAMIC_TRAP))
	{
		if (m_pSettingDlg)
			m_pSettingDlg->ShowWindow(SW_HIDE);
		
		delete m_pSettingDlg;
		m_pSettingDlg = NULL;
	}
	
	if (!m_pSettingDlg)
	{
		m_pSettingDlg = new CNpcSettingDlg;
		m_pSettingDlg->Create(IDD_NPCSETTING);
		m_pSettingDlg->ShowWindow(SW_HIDE);
		
		g_SetFilePath(PATH_TRAPSETTING);
		g_GetFullPath(FilePath,"TrapSetting.ini");
		m_pSettingDlg->m_ActiveIniForm.LoadTemplate(FilePath);
		m_pSettingDlg->m_bLoadingTemplate = TRUE;
		
	}
	
	m_pSettingDlg->m_bLoadingTemplate = TRUE;
	g_SetFilePath("");
	m_pSettingDlg->m_ActiveIniForm.Load((LPCSTR)strTemplateFile,1, (LPCSTR)strSection, TRUE);
	m_pSettingDlg->m_TemplateFile = strTemplateFile;
	
	ASSERT(!strObjFile.IsEmpty());
	g_SetFilePath("");
	m_pSettingDlg->m_ExtraFile = strObjFile;
	m_pSettingDlg->m_ActiveIniForm.SetCtrlRecord("Script", FALSE);
	m_pSettingDlg->m_ActiveIniForm.Load(strObjFile,0,"1",TRUE);
	m_pSettingDlg->m_bLoadingTemplate = FALSE;	

	KIniFile TempIniFile;
	if(TempIniFile.Load((LPCSTR)strObjFile))
	{
		TempIniFile.WriteInteger("MAIN", "Type", 2);
		TempIniFile.Save((LPCSTR)strObjFile);
	}
	m_pSettingDlg->m_ActiveIniForm.ModifyData("Script", szScriptFile, szScriptFile);
	char szTrapId[20];
	sprintf(szTrapId, "%d", id);
	m_pSettingDlg->m_ActiveIniForm.ModifyData("TrapId", szTrapId, szTrapId);
	m_pSettingDlg->m_CurSaveType = DYNAMIC_TRAP;
	m_pSettingDlg->SetWindowPos(&CWnd::wndTopMost ,rc.left,rc.top,rc.Width(),rc.Height(),0);
	m_pSettingDlg->ShowWindow(SW_SHOW);
	return TRUE;
}

BOOL cStatusDynamic::ChangeTrapScript(iObj* pPlane)
{
	long id;
	GU_GetObjID(pPlane,id);
	CString strFolder = m_strDocument.Left(m_strDocument.GetLength() - 4);
	KTabFile TabFile;
	CString strScriptTab = strFolder;
	strScriptTab += "\\";
	strScriptTab += "ScriptList.txt";
	TabFile.Load((LPSTR)(LPCSTR)strScriptTab);
	
	CFileDialog filedlg(TRUE);
	char  PathName[400];
	char FullPath[300];
	if (filedlg.DoModal() == IDOK)
	{
		g_SetFilePath("");
		g_GetFullPath(FullPath, "");
		
		CString sWork;
		AppGetWorkingFolder(sWork);

		PathRelativePathTo(PathName,(LPCSTR)sWork, FILE_ATTRIBUTE_DIRECTORY, filedlg.GetPathName(), FILE_ATTRIBUTE_NORMAL );
		KTabFileCtrl TabFile;
		TabFile.Load((LPSTR)(LPCSTR)strScriptTab);
		TabFile.WriteString(1,1, "TrapId");
		TabFile.WriteString(1,2, "Script");
		TabFile.WriteInteger(id + 1,1,id);
		TabFile.WriteString(id +1,  2,PathName);
		TabFile.Save((LPSTR)(LPCSTR)strScriptTab);
	}
	return TRUE;
}

BOOL cStatusDynamic::GetTrapScript(iObj* pPlane, CString &strScriptFile)
{
	long id;
	GU_GetObjID(pPlane,id);
	CString strFolder = m_strDocument.Left(m_strDocument.GetLength() - 4);
	KTabFile TabFile;
	CString strScriptTab = strFolder;
	strScriptTab += "\\";
	strScriptTab += "ScriptList.txt";
	TabFile.Load((LPSTR)(LPCSTR)strScriptTab);
	
	char szScriptFile[200];
	TabFile.GetString(id + 1, 2, "", szScriptFile, 200);
	strScriptFile = szScriptFile;
	return TRUE;
}

const CString c_strEvent= "event名称.txt";
const CString c_strTrapID = "trap点ID.txt";

BOOL cStatusDynamic::CreateFromFolder(LPCSTR szFolder)
{
	//load attribute and settings!
	CString sFolder = szFolder;
	sFolder += "\\setting";
	AppGetFullPath(sFolder);

	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strEvent,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		SetEvent(set.str.c_str());	
	}
	if (AppFindFile(c_strTrapID,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		SetTrapID(set.n);	
	}
	return TRUE;
}

BOOL cStatusDynamic::Create()
{
	cStatus::Create();
	
	CString sIni = "游戏资源\\动态物体\\dynamic.ini";
	AppGetFullPath(sIni);
	iItem* pItem = Common_CreateItem(sIni,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);

	GetItemValue(pItem,"npcnum",m_nNpc);
	GetItemValue(pItem,"boxnum",m_nBox);

	return TRUE;
}
///////////////////////////////////////////////////////////////////
//set and get
///////////////////////////////////////////////////////////////////
DynamicEnum cStatusDynamic::GetObjDynamic(iObj* pPlane)
{
	stGroundObjParam param;
	param.ClearMask();
	pPlane->GetParam(param);
	return param.eDynamic;
}

BOOL cStatusDynamic::SetObjNick(iObj* pPlane,const CString& strNick)
{
	HRESULT hr = GU_SetObjNick(pPlane,strNick);
	return TRUE;
}

BOOL cStatusDynamic::SetObjTemplate(iObj* pPlane,const CString& strTemplate, const CString& strSection)
{
	CString strIni;
	if (GetObjIniFile(pPlane,strIni))
	{
		AppBuildPath(strIni);
		iItem* pItem = Common_CreateItem(strIni,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItem);
		if (!strTemplate.IsEmpty())
		{
			SetItemValue(pItem,"mapedit_template",strTemplate);
			if (!strSection.IsEmpty())
				SetItemValue(pItem,"mapedit_templatesection",strSection);
		}
		pItem->Save();
		return TRUE;
	}
	return FALSE;
}

BOOL cStatusDynamic::GetObjNick(iObj* pPlane,CString& strNick)
{
	stGroundObjParam param;
	param.ClearMask();
	pPlane->GetParam(param);
	strNick = param.szEvent;
	return TRUE;
}

BOOL cStatusDynamic::GetObjTemplate(iObj* pPlane,CString& strTemplate,CString& strSection)
{
	CString strIni;
	if (GetObjIniFile(pPlane,strIni))
	{
		iItem* pItem = Common_CreateItem(strIni,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		if (GetItemValue(pItem,"mapedit_template",strTemplate))
		{
			GetItemValue(pItem,"mapedit_templatesection",strSection);
			return TRUE;
		}
	}
	return FALSE;
}

/*
BOOL cStatusDynamic::GetObjAnimation(iObj* pPlane,CString& strAnimation, BOOL& bAutoplay)
{
	CString strIni;
	bAutoplay = FALSE;
	if (GetObjIniFile(pPlane,strIni))
	{
		iItem* pItem = Common_CreateItem(strIni,iItem::FLAG_LOAD);
		wrap_ptr<iItem> ptr(pItem);
		if (GetItemValue(pItem,"mapedit_animation",strAnimation))
		{
			GetItemValue(pItem,"mapedit_autoplay",bAutoplay);
			return TRUE;
		}
	}
	return FALSE;
}
*/
///////////////////////////////////////////////////////////////////
//property
///////////////////////////////////////////////////////////////////

void cStatusDynamic::ChangeObjProperty(iObj* pPlane)
{
	if (pPlane == NULL)
		return;
	
	CString strNick,strTemplate,strSection;
	BOOL bAutoPlay = FALSE;
	GetObjNick(pPlane,strNick);
	GetObjTemplate(pPlane,strTemplate,strSection);
//	GetObjAnimation(pPlane,strAnimation,bAutoPlay);

	CObjDlg dlg;
	dlg.m_strNick = strNick;
	dlg.m_strTemplate = strTemplate;
	dlg.m_strSection = strSection;
	dlg.m_bAutoPlay = bAutoPlay;
	if (dlg.DoModal() != IDOK)
		return;

	BOOL bSameNick = FALSE;
	if (dlg.m_strNick.CompareNoCase(strNick) == 0)
		bSameNick = TRUE;
	else
	{
		bSameNick = FALSE;
		strNick = dlg.m_strNick;
	}
	if (strNick.IsEmpty())
		return;
	bAutoPlay = dlg.m_bAutoPlay;
	strTemplate = dlg.m_strTemplate;
	if (!strTemplate.IsEmpty())
		AppGetShortPath(strTemplate);
	strSection= dlg.m_strSection;

	if (!bSameNick)
	{
		if (GetGround()->FindDynamicNick(strNick))
		{
			AfxMessageBox("这个名字已经被使用了");
			return;
		}
		CString sOld;
		GetObjNick(pPlane,sOld);
		CString sOldIni;
		CString sOldScript;
		if (!sOld.IsEmpty())
		{
			GetObjIniFile(pPlane,sOldIni);
			GetObjScriptFile(pPlane,sOldScript);
		}

		CString sIni;
		CString sScript;
		GetObjIniFile(pPlane,sIni);
		GetObjScriptFile(pPlane,sScript);
		CFileFind f;
		if (!sOldIni.IsEmpty() && f.FindFile(sOldIni))
		{
			AppRenameFolder(sOldIni,sIni);
		}
		if (!sOldScript.IsEmpty() && f.FindFile(sOldScript))
		{
			AppRenameFolder(sOldScript,sScript);
		}

		SetObjNick(pPlane,strNick);
	}
	SetObjTemplate(pPlane,strTemplate,strSection);
}

///////////////////////////////////////////////////////////////////
//virtual event
///////////////////////////////////////////////////////////////////

BOOL cStatusDynamic::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////
//dynamic
//////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//file
/////////////////////////////////////////////////////////////////////

BOOL cStatusDynamic::GetObjScriptFile(iObj* p,CString& sScript)
{
	CString sIni;
	CString strTemplateFile,strSection;

	if (!GetObjTemplate(p,strTemplateFile,strSection))
	{
		AfxMessageBox("尚未指定obj的模板");
		return FALSE;
	}
	if (!GetObjIniFile(p,sIni))		
		return FALSE;

	DynamicEnum eType = GetObjDynamic(p);
	


	CString strScript;
	KIniFile IniFile;
	sScript = "";
	BOOL bFindScript = FALSE;
	if (IniFile.Load(sIni))
	{
		char ActionScript[100];
		switch(eType)
		{
		case 	DYNAMIC_NPC:
			{
				if (IniFile.GetString("1", "ActionScript", "", ActionScript, 100))
				{
					sScript = ActionScript;
					bFindScript = TRUE;
				}
			}break;
			
		case	DYNAMIC_BOX:
			{
				if (IniFile.GetString("1", "ScriptName", "", ActionScript, 100))
				{
					sScript = ActionScript;
					bFindScript = TRUE;
				}
			}break;
			
		case	DYNAMIC_TRAP:
			{
				long id;
				GU_GetObjID(p,id);
				CString strFolder = m_strDocument.Left(m_strDocument.GetLength() - 4);
				KTabFileCtrl TabFile;
				CString strScriptTab = strFolder;
				strScriptTab += "\\";
				strScriptTab += "ScriptList.txt";
				m_strTrapScript = strScriptTab;
				TabFile.Load((LPSTR)(LPCSTR)strScriptTab);
				
				char szScriptFile[200];
				TabFile.GetString(id + 1, 2, "", szScriptFile, 200);
				sScript = szScriptFile;
				if (!sScript.IsEmpty())
				{
					AppGetFullPath(sScript);
					return TRUE;
				}
				
			}break;
		}

	}
	
	if (!bFindScript)
	{
		KTabFile TabFile;
		if (TabFile.Load((LPSTR)(LPCSTR)strTemplateFile))
		{
			char ActionScript[100];
			switch(eType)
			{
			case DYNAMIC_NPC:
				{
					
					if (TabFile.GetString((LPSTR)(LPCSTR)strSection, "ActionScript", "", ActionScript, 100))
					{
						sScript = ActionScript;
						bFindScript = TRUE;
					}
				}break;
			case DYNAMIC_BOX:
				{
					if (TabFile.GetString((LPSTR)(LPCSTR)strSection, "ScriptName", "", ActionScript, 100))
					{
						sScript = ActionScript;
						bFindScript = TRUE;
					}
					
				}break;
			case DYNAMIC_TRAP:
				{
					if (TabFile.GetString((LPSTR)(LPCSTR)strSection, "Script", "", ActionScript, 100))
					{
						sScript = ActionScript;
						bFindScript = TRUE;
					}
					
				}
			}

		}
	}

	if (sScript == "" )
	{
		if (eType == DYNAMIC_TRAP)
		{
			AfxMessageBox("Please Using Alt + DoubleClick To Set TrapScript!");
			return FALSE;
		}
		else
		if (AfxMessageBox("当前未设置脚本文件，是否需要自动为其生成脚本？", MB_OKCANCEL) == IDOK)
		{
			CString strScript  = sIni.Left(sIni.GetLength() - 4) + "_script.ini";
			sScript = strScript;
			AppGetShortPath(strScript);
			sScript = strScript;
			switch(eType)
			{
			case DYNAMIC_NPC:
				IniFile.WriteString("1", "ActionScript", (LPSTR)(LPCSTR)strScript);
				break;
			case DYNAMIC_BOX:
				IniFile.WriteString("1", "ScriptName", (LPSTR)(LPCSTR)strScript);
				break;
			case DYNAMIC_TRAP:
				IniFile.WriteString("1", "Script", (LPSTR)(LPCSTR)strScript);
				break;
			}
			
			IniFile.Save(sIni);
		}
		else
			return FALSE;
	}

	AppGetFullPath(sScript);
	return TRUE;
}

BOOL cStatusDynamic::GetOffsetOfPlane(iObj* pPlane,cPoint& pt)
{
	DynamicEnum e = GetObjDynamic(pPlane);
	if (e == DYNAMIC_TRAP)
	{
		pt.x = (int)pt.x;
		pt.y = (int)pt.y+1;
		return TRUE;
	}
	return FALSE;
}

BOOL cStatusDynamic::AfterCreatePlane(iObj* pPlane,const CString& m_strSelectFile,const CString& m_strCopyDataFile)
{
	if (pPlane == NULL)
		return FALSE;

	CString sPath,sName,sExt;
	AppParsePathNameExt(m_strSelectFile,sPath,sName,sExt);
		
	CString strEvent;
	DynamicEnum e = GetObjDynamic(pPlane);
	long lID = 0;
	switch (e)
	{
	case DYNAMIC_NPC:
	case DYNAMIC_CLIENTONLYNPC:
		lID = m_nNpc++;
		break;
	case DYNAMIC_BOX:
	case DYNAMIC_CLIENTONLYBOX:
		lID = m_nBox++;
		break;
	case DYNAMIC_TRAP:
	case DYNAMIC_CLIENTONLYTRAP:
		lID = GetTrapID();
		break;
	default:
		ASSERT(0);
	}
	SaveID();

	//give it a temp name!
	GU_SetObjEvent(pPlane,m_strEvent);
	GU_SetObjID(pPlane,lID);

	CString strSection;
	CString strTem;
	iItem* pItem = Common_CreateItem(m_strSelectFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (GetItemValue(pItem,"template",strTem) && GetItemValue(pItem,"section",strSection))
	{
		SetObjTemplate(pPlane,strTem,strSection);
	}
	m_bModify = TRUE;
	if (!m_strCopyDataFile.IsEmpty())
	{
		CString newDataFile;
		GetObjIniFile(pPlane,newDataFile);
		AppCopyFile(m_strCopyDataFile,newDataFile);
	}
	return TRUE;
}

void cStatusDynamic::SetEvent(LPCSTR sz)
{
	m_strEvent = sz;
	stCommand_Dynamic st;
	st.type = COM_FILE_DYNAMIC;
	st.nOperator = stCommand_Dynamic::OP_OPEN;
	strcpy(st.strFile,m_strEvent);
	DoCommand(&st);
}

void cStatusDynamic::OnSelectedSetting(LPCTSTR szFile,stSetting& st)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strEvent)
		SetEvent(st.str.c_str());
	if (sName == c_strTrapID)
		SetTrapID(st.n);
}

