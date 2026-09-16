//******************************************************************************
/*! \file       VersionMakerDlg.cpp
*   \brief      自动更新版本制作工具
*
*				详细描述
*   \author		zhq
*   \version	版本号
*   \date		2003-5-9 9:07:40
*   \sa			参考内容
*   \todo		将要做的工作
*******************************************************************************/

#include "stdafx.h"
#include "VersionMaker.h"
#include "VersionMakerDlg.h"
#include "KLEFile.h"
#include "KLEFilePathMgr.h"
#include "KLEFileEnumerate.h"
#include "KFileInfo.h"
#include"ompStringNotMatchCase.h"
#include<string>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


using namespace std;

CVersionMakerDlg::CVersionMakerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVersionMakerDlg::IDD, pParent)
	,m_BackUpStatus(-1)
{
	//{{AFX_DATA_INIT(CVersionMakerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	PackageLib = LoadLibrary("PackageFiles.dll");
		
	if(NULL == PackageLib)
	{
		DisplayErrorInfo();
		throw exception("Error when LoadLibrary"); 
		
		
		
	}
	
	ProcAdd  = (MYPROC)GetProcAddress(PackageLib,"pack");
	
	if(!ProcAdd)
	{
		DisplayErrorInfo();
		throw exception("Error when GetProcAddress"); 
	}
	
	m_nCurVersion = -1;
}

void CVersionMakerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVersionMakerDlg)
	DDX_Control(pDX, IDC_DESEDIT, m_DesPath);
	DDX_Control(pDX, IDC_SRCEDIT, m_SrcPath);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVersionMakerDlg, CDialog)
	//{{AFX_MSG_MAP(CVersionMakerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SRCBROWSE, OnSrcbrowse)
	ON_BN_CLICKED(IDC_DESBROWSE, OnDesbrowse)
	ON_EN_CHANGE(IDC_DESEDIT, OnChangeDesPath)
	ON_BN_CLICKED(IDC_BUTTON1, OnResume)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CVersionMakerDlg::OnInitDialog()
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
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void DisplayErrorInfo(string& ErrorInfo)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	MessageBox( NULL, ErrorInfo.c_str(),(LPCTSTR)lpMsgBuf , MB_OK |MB_ICONWARNING );
	// Free the buffer.
	LocalFree( lpMsgBuf );
	
	
	
}


void RunProcess(char * CommandLine)
{
	STARTUPINFO si;
    PROCESS_INFORMATION pi;
	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	
    // Start the child process. 
    if( !CreateProcess( NULL, // No module name (use command line). 
        CommandLine, // Command line. 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
        0,                // No creation flags. 
        NULL,             // Use parent's environment block. 
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi )             // Pointer to PROCESS_INFORMATION structure.
		) 
    {
		string ErrorInfo = string("CreateProcess(" )+ string(CommandLine);
        DisplayErrorInfo(ErrorInfo);
		throw exception("ERROR when CreateProcess ");
    }
	WaitForSingleObject( pi.hProcess, INFINITE );


	
}
void CVersionMakerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVersionMakerDlg::OnPaint() 
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
HCURSOR CVersionMakerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// 让用户选择源路径，使用Shell功能
void CVersionMakerDlg::OnSrcbrowse() 
{
	// 让用户选择路径，使用Shell功能
	BROWSEINFO		bInfo;
	LPITEMIDLIST	pItemList = NULL;
	char			szSrcPath[256];
	
	memset(&bInfo, 0, sizeof(BROWSEINFO));
	bInfo.hwndOwner	= m_hWnd;
	bInfo.lpszTitle = "请选择本次需要更新的文件的根目录";
	bInfo.ulFlags	= BIF_DONTGOBELOWDOMAIN;

	pItemList = SHBrowseForFolder(&bInfo);
	if(pItemList == NULL)
	{
		return;
	}
	else
	{
		if(SHGetPathFromIDList(pItemList, szSrcPath))
		{
			m_SrcPath.SetWindowText(szSrcPath);
		}
		else
			return;
	}
}
	
// 让用户选择目标路径，使用Shell功能
void CVersionMakerDlg::OnDesbrowse() 
{
	
	BROWSEINFO		bInfo;
	LPITEMIDLIST	pItemList = NULL;
	char			szDesPath[256];
	
	memset(&bInfo, 0, sizeof(BROWSEINFO));
	bInfo.hwndOwner	= m_hWnd;
	bInfo.lpszTitle = "请选择版本制作的根目录";
	bInfo.ulFlags	= BIF_DONTGOBELOWDOMAIN;

	pItemList = SHBrowseForFolder(&bInfo);
	if(pItemList == NULL)
	{
		return;
	}
	else
	{
		if(SHGetPathFromIDList(pItemList, szDesPath))
		{
			m_DesPath.SetWindowText(szDesPath);
		}
		else
			return;
	}
	
}

void CVersionMakerDlg::OnOK() 
{
	if (false == VerifyInfo())
	{
		::MessageBox(m_hWnd, "参数校验出错", "信息", MB_OK);
		return;
	}
	BackupUpDateFile(GetUpdataDataFolderName().c_str(),GetUpdateDataBackUpFolderName().c_str());
	// 拷贝本次更新内容
	SetBackUpStatus(FALSE);
	SetFileAttributeOfAFolder(GetUpdataDataFolderName().c_str(),FILE_ATTRIBUTE_NORMAL);
	
	CopySrcToDes();

	// 生成本次文件列表
	MakeFileList();

	// 生成各个版本的信息
	MakeVersionInfo();
	
	RefreshVersionInfo();

	::MessageBox(m_hWnd, "版本创建成功", "提示", MB_OK);

	CDialog::OnOK();
}

void CVersionMakerDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

//******************************************************************************
/*! \fn     bool CVersionMakerDlg::VerifyInfo()
*   \brief  开始处理前检验合法性,还可以做更多的校验
*   \return bool 返回值的描述
*******************************************************************************/
bool CVersionMakerDlg::VerifyInfo()
{
	CString			strBuf;

	m_SrcPath.GetWindowText(strBuf);
	if ( false == KLEFilePathMgr::IsPathExist(strBuf) || strBuf.GetLength() == 0)
		return false;

	m_DesPath.GetWindowText(strBuf);
	if ( false == KLEFilePathMgr::IsPathExist(strBuf) || strBuf.GetLength() == 0)
		return false;

	if( m_nCurVersion < 0)
		return false;
	return true;
}

//******************************************************************************
/*! \fn     bool CVersionMakerDlg::CopySrcToDes()
*   \brief  简要描述
*   \return bool 返回值的描述
*******************************************************************************/
bool CVersionMakerDlg::CopySrcToDes()
{
	KLEFileEnumerate	FileEnumerate;
	CString				strBuf;

	m_SrcPath.GetWindowText(strBuf);
	FileEnumerate.SetCallBack(CopyFileCB);
	FileEnumerate.SetParam(this);
	FileEnumerate.EnumFileInFolder(strBuf);

	return true;
}

//******************************************************************************
/*! \fn     bool CVersionMakerDlg::MakeFileList()
*   \brief  生成文件列表
*   \return bool 返回值的描述
*******************************************************************************/
bool CVersionMakerDlg::MakeFileList()
{
	KLEFileEnumerate	FileEnumerate;
	char				szDataPath[KLELNAMELEN];
	char				szFileListName[KLELNAMELEN];
	CString				strBuf;

	m_DesPath.GetWindowText(strBuf);
	sprintf(szDataPath, "%s%s", strBuf, KLEP_DATA);
	sprintf(szFileListName, "%s%s\\FileList%04d", strBuf, KLEP_FILELIST, m_nCurVersion+1);

	
	FileEnumerate.SetCallBack(MakeFileListCB);
	FileEnumerate.SetParam(this);
	FileEnumerate.EnumFileInFolder(szDataPath);

	SortFileListInfo();
	OutputToFileLsit(szFileListName);

	return true;
}

//******************************************************************************
/*! \fn     void CVersionMakerDlg::OnChangeDesPath()
*   \brief  目标路径改变，在此处添加版本号提取
*******************************************************************************/
void CVersionMakerDlg::OnChangeDesPath() 
{
	KLEFile File;
	CString	strBuf;
	char	szVersionFile[KLELNAMELEN];
	char	szBuf[KLESNAMELEN];

	m_DesPath.GetWindowText(strBuf);
	sprintf(szVersionFile, "%s%s", strBuf, KLEP_VERSIONFILE);
	
	fstream VersionInfoFile;
	VersionInfoFile.open(szVersionFile,ios::in);
	if (VersionInfoFile.is_open())
	{
		VersionInfoFile >>szBuf;

		m_nCurVersion = atoi(szBuf);

		VersionInfoFile >>szBuf;


		m_BackUpStatus = atoi(szBuf);



		



	}
	else
		m_nCurVersion = -1;
	VersionInfoFile.close();
}
void RecordFileToVector(const char * szFileName,void * pParam)
{
	vector<string> * FileRecordSet = (vector<string> *) pParam;

    FileRecordSet->push_back(string(szFileName));



}
void CopyFile(const char * szFileName,void * pParam)
{
	char				szHalfPath[KLELNAMELEN];
	char				szDesFullPath[KLELNAMELEN];
	SourceDest *  Address = (SourceDest*)pParam;
	CString				strBuf(Address->GetSource().c_str());
	//替换根路径--原路径的根路径替换为目标路径的跟路径{
	
	strcpy(szHalfPath, szFileName +  strBuf.GetLength() + 1);
	
	strBuf = CString((Address->GetDest()).c_str());
	sprintf(szDesFullPath, "%s%s%s", strBuf, "\\", szHalfPath);
	//}
	MakeSureDirectoryPathExists(szDesFullPath);
	CopyFile(szFileName, szDesFullPath, FALSE);
	
}

//******************************************************************************
/*! \fn     void CopyFile(const char* szFileName, void* pParam)
*   \brief  拷贝数据的回调函数
*   \param  const char* szFileName 参数的描述
*   \param  void* pParam 参数的描述
*******************************************************************************/
void CopyFileCB(const char* szFileName, void* pParam)
{
	char				szHalfPath[KLELNAMELEN];
	char				szDesFullPath[KLELNAMELEN];
	CVersionMakerDlg*	pDlg = (CVersionMakerDlg*)pParam;
	CString				strBuf;
//替换根路径--原路径的根路径替换为目标路径的跟路径{
	pDlg->m_SrcPath.GetWindowText(strBuf);
	strcpy(szHalfPath, szFileName +  strBuf.GetLength() + 1);

	pDlg->m_DesPath.GetWindowText(strBuf);
	sprintf(szDesFullPath, "%s%s%s%s", strBuf, KLEP_DATA, "\\", szHalfPath);
//}
	if(!MakeSureDirectoryPathExists(szDesFullPath))
	{
		string ErrorInfo = string("Error when MakeSureDirectoryPathExists(") +string(szDesFullPath) + string(" )");
		DisplayErrorInfo(ErrorInfo);
	}
	
	
	if(!CopyFile(szFileName, szDesFullPath, FALSE))
	{
		string ErrorInfo = string("Error when CopyFile(") +string(szFileName) + string(",") + string(szDesFullPath) + string(")");
		DisplayErrorInfo(ErrorInfo);

		throw exception (ErrorInfo.c_str());

	}
}




//******************************************************************************
/*! \fn     oid MakeFileListCB(const char* szFileName, void* pParam)
*   \brief  生成文件列表内容的回调函数，文件列表内容包含，文件名和文件创建时间
*   \param  const char* szFileName 参数的描述
*   \param  void* pParam 参数的描述
*******************************************************************************/
void MakeFileListCB(const char* szFileName, void* pParam)
{
	char				szContent[KLELNAMELEN];
	KFileInfo			FileInfo;
	WIN32_FIND_DATA		ffd ;
	SYSTEMTIME			SysTime;
	FILETIME			LocalFileTime;
	CVersionMakerDlg*	pDlg = (CVersionMakerDlg*)pParam;

	
	//sprintf(szContent, "%s%s", szFileName, "\n");
	FileInfo.m_szFileName = szFileName;

	// 输出文件的创建时间
	HANDLE hFindFile = FindFirstFile(szFileName, &ffd);
	if (INVALID_HANDLE_VALUE == hFindFile)
		return;
	FindClose(hFindFile);

	FileTimeToLocalFileTime(&ffd.ftLastWriteTime, &LocalFileTime);
	FileTimeToSystemTime(&LocalFileTime, &SysTime);
	sprintf(szContent, "%d年%d月%d日%d时%d分%d秒%d毫秒",
		SysTime.wYear, SysTime.wMonth, SysTime.wDay,
		SysTime.wHour, SysTime.wMinute, SysTime.wSecond, 
		SysTime.wMilliseconds);

	FileInfo.m_szFileCreateTime = szContent;

	pDlg->m_NewFileListVector.push_back(FileInfo);	
}

//******************************************************************************
/*! \fn     void CVersionMakerDlg::SortFileListInfo()
*   \brief  将生成的文件列表内容排序排序
*******************************************************************************/
void CVersionMakerDlg::SortFileListInfo()
{
	sort(m_NewFileListVector.begin(), &m_NewFileListVector[m_NewFileListVector.size()]);
}

//******************************************************************************
/*! \fn     void CVersionMakerDlg::OutputToFileLsit(szFileListName)
*   \brief  将文件列表内容输出到文件中
*   \param  szFileListName 文件列表的名字
*******************************************************************************/
void CVersionMakerDlg::OutputToFileLsit(const char* szFileListName)
{
	KLEFile		File;
	char		szContent[KLELNAMELEN];

	File.Open(szFileListName, KLEFSB_OPENMODE_CREATE);
	
	// Iterator is used to loop through the vector.
    KFileInfoVector::iterator theIterator;

	for (theIterator = m_NewFileListVector.begin(); theIterator != m_NewFileListVector.end();
         theIterator++)
	{
		sprintf(szContent, "%s%s", theIterator->m_szFileName.c_str(), "\n");
		File.WriteString(szContent);
		sprintf(szContent, "%s%s", theIterator->m_szFileCreateTime.c_str(), "\n");
		File.WriteString(szContent);
	}
		 
	File.Close();
}

void CVersionMakerDlg::MakeVersionInfo()
{
	// 生成其他版本更新内容

	

	DeleteADirectoryAllFile(GetVersionDataFolderName().c_str());
	CreateVersionConfigFile();
	string FileCommentFilePath = GetInfoFolderName()+ "\\" + FileCommentFileName;

	FileComment.open(FileCommentFilePath.c_str(),ios::out); 

  
	
    	
	for (int i = 0; i <= m_nCurVersion; i++)
	{
		MakeVersionInfo(i);
	}
    
	

   

	FileUpdateAttribute UpdateAttribute(string(VersionConfigFileName));
	UpdateAttribute.SetBelone(string("-1")) ;

	UpdateAttribute.SetRemotePath(string(RemoteRelativeFolder));

    UpdateAttribute.SetUpdateFileMethod( string("Last;Copy")) ;
	
	UpdateAttribute.DumpTostream((iostream&)FileComment);

	FileComment.close(); 

	RunAutoBat(GetVersionDataFolderName(),GetToolsFolderName(),GetInfoFolderName(),GetIndexFolder());
}

void CVersionMakerDlg::MakeVersionInfo(int VersionNumber)
{

	//string ThisVersionFileFolderName = CreateVersionFolder(nOldVersion);
	
   // string VersionFileHalfPath       = GetversionHalfPath(nOldVersion);
	
	char	szOldFileList[KLELNAMELEN];
	CString	strBuf;

	m_DesPath.GetWindowText(strBuf);
	sprintf(szOldFileList, "%s%s\\%s%04d", strBuf, KLEP_FILELIST, "FileList", VersionNumber);


	vector<string> UpdateFileName;

	
	
	if ( false == GetUpdateFileNameOnThisVersion(szOldFileList,UpdateFileName))
	{
		::MessageBox(m_hWnd, "Error in GetUpdateFileName", "警告", MB_OK);
	}
	
	vector<FilePath> UpdateFileNameInRoot,UpdateFileNameNotInRoot;

	string RootPath = GetUpdataDataFolderName();


	int Count = UpdateFileName.size();
		
	PickOutFileNameBetweenRootAndNotRoot(UpdateFileName,UpdateFileNameInRoot,UpdateFileNameNotInRoot,RootPath);


	

	if(VersionNumber == 0)
	{
		
		InRootFileProcess(UpdateFileNameInRoot,GetVersionDataFolderName());
	}
	InNotRootFileProcess(UpdateFileNameNotInRoot,VersionNumber,GetUpdataDataFolderName());



	

	

    



}

bool CVersionMakerDlg::GetUpdateFileNameOnThisVersion(const char* szOldFileList,vector<string>& UpdateFileName)
{
	KFileInfoVector				OldFileVector;

	KFileInfoVector::iterator	NewIterator, FindIterator;

	int Count = m_NewFileListVector.size ();
	if ( false == ReadFileList(OldFileVector, szOldFileList))
	{
		for( NewIterator = m_NewFileListVector.begin(); NewIterator != m_NewFileListVector.end(); NewIterator++)
		{
			UpdateFileName.push_back(NewIterator->m_szFileName);
		}
		goto SAMEPROCESS;
	}
		

	for( NewIterator = m_NewFileListVector.begin(); NewIterator != m_NewFileListVector.end(); NewIterator++)
	{
		FindIterator = find(OldFileVector.begin(), OldFileVector.end(), *NewIterator);
		if (FindIterator != OldFileVector.end())
		{
			if ( FindIterator->m_szFileCreateTime == NewIterator->m_szFileCreateTime)
				continue;
		}

		UpdateFileName.push_back(NewIterator->m_szFileName);
	}

SAMEPROCESS:
	// 分两种情况生成列表
	/*
	CString		strBuf;
	char		szBuf[256];
	m_DesPath.GetWindowText(strBuf);
	
	sprintf(szBuf, "%s%s", strBuf, KLEP_DATA);
	*/
	
    
	



	return true;
}

bool CVersionMakerDlg::ReadFileList(KFileInfoVector& FileInfoVector, const char* szFileListName)
{
	KLEFile			File;
	KFileInfo		FileInfo;
	char			szFileNameBuf[KLELNAMELEN];
	char			szFileTimeBuf[KLELNAMELEN];
	int 			nRet1, nRet2;

	if ( false == File.Open(szFileListName, KLEFST_OPENMODE_EXIST))
	{
		//cout << "Failed to open Old version filelist" << endl;
		return false;
	}
	
	for(;;)
	{
		nRet1	= File.ReadLine(szFileNameBuf, 256);
		nRet2	= File.ReadLine(szFileTimeBuf, 256);
		
		if (nRet1 == KLEFS_ERROR || nRet1 == KLEFS_ERROR)
			return false;
		else if (nRet1 == KLEFS_EOF || nRet1 == KLEFS_EOF)
		{
			return true;
		}
		else
		{
			int i = 0;
			while(szFileNameBuf[i] != '\n' && szFileNameBuf[i] != '\0')
				i++;
			szFileNameBuf[i] = '\0';

			i = 0;
			while(szFileTimeBuf[i] != '\n' && szFileTimeBuf[i] != '\0')
				i++;
			szFileTimeBuf[i] = '\0';

			FileInfo.m_szFileName		= szFileNameBuf;
			FileInfo.m_szFileCreateTime = szFileTimeBuf;
			FileInfoVector.push_back(FileInfo);
		}		
	}	

	return true;
}

//******************************************************************************
/*! \fn     void CVersionMakerDlg::RefreshVersionInfo()
*   \brief  版本生成成功后，修改版本号
*******************************************************************************/
void CVersionMakerDlg::RefreshVersionInfo()
{
	SetVersionNumber(m_nCurVersion +1);



}

void CVersionMakerDlg::PickOutFileNameBetweenRootAndNotRoot(vector<string>& FileNameArray ,vector<FilePath>& FileNameInRoot,vector<FilePath>& FileNameNotInRoot,const string& RootPath)
{
/*	char szDataPath[MAX_PATH];
	CString			strBuf;
	string RootPath;
	m_DesPath.GetWindowText(strBuf);

	sprintf(szDataPath, "%s%s\\", strBuf, KLEP_DATA);
	
	RootPath = szDataPath; 
*/

	//应该先去出所有空字符,在进行处理,跟路径和文件路径都要去出空字符
	//当前假设已去过
	vector<string>::iterator FileNamePoint;
	
	string HalfPath;
	string FileName;
	for(FileNamePoint = FileNameArray.begin(); FileNamePoint!=FileNameArray.end();FileNamePoint++)
    {
		FileName = * FileNamePoint;
		
		int Position = FileName.find(RootPath);
		if(Position == string::npos)
		{
			throw exception("FileName is Not in root path!");
		}

		Position += RootPath.length() + 1 ;  
        
		HalfPath = FileName.substr(Position);

		FilePath Path(FileName,HalfPath);
		if(HalfPath.find('\\') == string::npos )
		{
			
			FileNameInRoot.push_back(Path);


		}
		else
		{
			FileNameNotInRoot.push_back(Path);  

		}
         
    }




	 




}

void CVersionMakerDlg::InRootFileProcess(vector<FilePath>& FileName,string& ThisVersionFileFolderName)const 
{
	vector<string> UpdateRelativeFiles;
	GetUpdateRelativeFile(UpdateRelativeFiles);

	
	vector<FilePath>::iterator FilePoint; 
	

	for(FilePoint = FileName.begin();FilePoint!=FileName.end();FilePoint++)
	{
		string VersionFileName = ThisVersionFileFolderName +"\\"+   (*FilePoint).HalfPath;
		

		if(!MakeSureDirectoryPathExists(VersionFileName.c_str()))
		{
			string ErrorInfo = string("Error when MakeSureDirectoryPathExists(") +VersionFileName + string(" )");
			DisplayErrorInfo(ErrorInfo);
		}
		
		
		if(!CopyFile((* FilePoint).FullPath.c_str(), VersionFileName.c_str(), FALSE))
		{
			string ErrorInfo = string("Error when CopyFile(") +(* FilePoint).FullPath + string(",") +  VersionFileName + string(",FALSE)");
			DisplayErrorInfo(ErrorInfo);
			throw exception(ErrorInfo.c_str());
			
		}
	


		FileUpdateAttribute UpdateAttribute((*FilePoint).HalfPath);
		UpdateAttribute.SetBelone(string("-1")) ;
		string RemotePath = RemoteRelativeFolder;
        		
		UpdateAttribute.SetRemotePath(RemotePath);

		if(CheckFileNameInFileS((* FilePoint).HalfPath,UpdateRelativeFiles))
		{
			UpdateAttribute.SetUpdateRelative("1"); 
		}

		UpdateAttribute.DumpTostream((iostream&)FileComment);
		
		
    
	}





}

string CVersionMakerDlg::GetVersionDataFolderName()
{
	char szVersionDataPath[MAX_PATH];
	CString				strBuf;

	m_DesPath.GetWindowText(strBuf);
	sprintf(szVersionDataPath, "%s%s", strBuf, KLEP_VERSIONDATA);

    string VersionDataPath = szVersionDataPath;

	return VersionDataPath;

}

string CVersionMakerDlg::GetUpdataDataFolderName()
{
	char szUpdataDataPath[MAX_PATH];
	CString				strBuf;
	
	m_DesPath.GetWindowText(strBuf);
	sprintf(szUpdataDataPath, "%s%s", strBuf, KLEP_DATA);
	
    string UpdataDataPath = szUpdataDataPath;
	
	return UpdataDataPath;
	
	

}

string CVersionMakerDlg::CreateVersionFolder(int nOldVersion)
{
	strstream VersionFileFolder;
	
	VersionFileFolder<<GetVersionDataFolderName() << "\\"<< nOldVersion<<'\0';
	
	string ThisVersionFileFolderName =  VersionFileFolder.str();    
	CreateDirectory(ThisVersionFileFolderName.c_str(),NULL);
	
	
	return ThisVersionFileFolderName;
	
	
}


void CVersionMakerDlg::InNotRootFileProcess(vector<FilePath>& FileName,int nOldVersion,string RootName)
{
	
	if(FileName.size()>0 )
	{
		vector<string> FileHalfPathArray;
		vector<FilePath>::iterator Pointer;
		int Count = FileName.size(); 
		for(Pointer = FileName.begin();Pointer!=FileName.end();Pointer++)
		{
		//	string HalfName = (* Pointer).HalfPath;
			
			FileHalfPathArray.push_back((* Pointer).HalfPath);
			
			
		}
		
		
	
		
		strstream PackFullFileNameStream;
		PackFullFileNameStream<< PackFileName << nOldVersion << "-" << m_nCurVersion + 1<<".pak"<<ends;
		
		
		string PackFullFileName = PackFullFileNameStream.str();
		
		string PackName =  GetVersionDataFolderName() + "\\"+ PackFullFileName;
		
	
		int Result = (* ProcAdd)(FileHalfPathArray,RootName,PackName);
		if(Result =false)
		{
			MessageBox("PackageFile Error");
			throw exception("Error when PackageFiles");

		}

	    
		
		
		
		strstream Belongstream ;
		Belongstream<<nOldVersion<<ends;
		
		string Belong = Belongstream.str();
		
		
		
		
		FileUpdateAttribute UpdateAttribute(PackFullFileName);
		UpdateAttribute.SetBelone(Belong) ;
		string RemotePath = RemoteRelativeFolder ;
		
		UpdateAttribute.SetRemotePath(RemotePath);
		UpdateAttribute.SetLocalPath(string(LocalDataFolder )); 

		UpdateAttribute.SetUpdateFileMethod( string("Package(") +  ResourcePackName + ")") ;
		
		UpdateAttribute.DumpTostream((iostream&)FileComment);
		
	}
	
	
	



}

string CVersionMakerDlg::GetversionHalfPath(int nOldVersion )
{
	strstream VersionFileHalfFolder;
	
	VersionFileHalfFolder << nOldVersion<<'\0';
	
	
	
	return VersionFileHalfFolder.str();


}

void CVersionMakerDlg::CreateVersionConfigFile()
{
	string VersionConfigPath = GetVersionDataFolderName() + "\\"+ VersionConfigFileName;
	//  确保目录创建成功
	MakeSureDirectoryPathExists(VersionConfigPath.c_str());
	fstream VersionConfigFile(VersionConfigPath.c_str(),ios::out );
	
    VersionConfigFile<<"[Version]"<<endl;
    VersionConfigFile<<"Version="<<m_nCurVersion + 1<<ends;

	VersionConfigFile.clear();
	VersionConfigFile.close();

}

string CVersionMakerDlg::GetToolsFolderName()
{
	char szUpdataDataPath[MAX_PATH];
	CString				strBuf;
	
	m_DesPath.GetWindowText(strBuf);
	sprintf(szUpdataDataPath, "%s%s", strBuf, KLEP_TOOLS);
	
    string UpdataDataPath = szUpdataDataPath;
	
	return UpdataDataPath;


}

string CVersionMakerDlg::GetInfoFolderName() const
{
	char szUpdataDataPath[MAX_PATH];
	CString				strBuf;
	
	m_DesPath.GetWindowText(strBuf);
	sprintf(szUpdataDataPath, "%s%s", strBuf, KLEP_INFO);
	
    string UpdataDataPath = szUpdataDataPath;
	
	return UpdataDataPath;



}

void CVersionMakerDlg::RunAutoBat(const string& DataPath,const string&  ToolsPath,const string& InfoPath,const string& IndexPath)
{
	string  FilesPath = DataPath + " ";
	string  OutPath   = InfoPath + "\\index.txt ";
	string  CommonIndexPath = InfoPath + "\\CommonIndex.ini ";
	string  FilesCommentPath = InfoPath + "\\" + FileCommentFileName + " " ;
	string  infoPath         = InfoPath + "\\info.txt ";
	string  IndexGenExeFile  = ToolsPath + "\\IndexGen ";
	string  Command = IndexGenExeFile +   FilesPath + OutPath + " 1  "+ ServerUpdateFolder + " " +  CommonIndexPath +  FilesCommentPath + infoPath;
	
	RunProcess((char *)Command.c_str());
	
	string  CompressDllExeFile = ToolsPath + "\\CompressDll ";
	
	Command  = CompressDllExeFile + "-l " + OutPath + IndexPath+"\\index.dat";
	
	RunProcess((char *)Command.c_str());
	

}

string CVersionMakerDlg::GetIndexFolder()
{
	char szUpdataDataPath[MAX_PATH];
	CString				strBuf;
	
	m_DesPath.GetWindowText(strBuf);
	sprintf(szUpdataDataPath, "%s%s", strBuf, KLEP_INDEX);
	
    string UpdataDataPath = szUpdataDataPath;
	
	return UpdataDataPath;


}

void CVersionMakerDlg::DeleteADirectoryAllFile(const char * DirectoryName)
{
	KLEFileEnumerate	FileEnumerate;
	CString				strBuf(DirectoryName);
    
	vector<string> FilenameSet;
	FileEnumerate.SetCallBack(RecordFileToVector);
	FileEnumerate.SetParam(&FilenameSet);
	FileEnumerate.EnumFileInFolder(strBuf);
    vector<string>::iterator Pointer;
	for(Pointer = FilenameSet.begin();Pointer != FilenameSet.end();Pointer++ )
	{
		if(!SetFileAttributes((*Pointer).c_str(),FILE_ATTRIBUTE_NORMAL))
		{
			string ErrorInfo = string("SetFileAttributes('") + *Pointer +  string("',FILE_ATTRIBUTE_NORMAL)");
			DisplayErrorInfo(ErrorInfo);

			
		}
		if(!DeleteFile((*Pointer).c_str()))
		{
			string ErrorInfo = string("DeleteFile(") + *Pointer +  string(")");
			DisplayErrorInfo(ErrorInfo);
			
		}
	}


}

void CVersionMakerDlg::BackupUpDateFile(const char * SrcFolder,const char * BackUpFolder)
{
	DeleteADirectoryAllFile(BackUpFolder);
	SourceDest Adress(SrcFolder,BackUpFolder);
	KLEFileEnumerate	FileEnumerate;
	


	FileEnumerate.SetCallBack(CopyFile);
	FileEnumerate.SetParam(&Adress);
	FileEnumerate.EnumFileInFolder(SrcFolder);
    


}

string CVersionMakerDlg::GetUpdateDataBackUpFolderName()
{
	char szUpdataDataPath[MAX_PATH];
	CString				strBuf;
	
	m_DesPath.GetWindowText(strBuf);
	sprintf(szUpdataDataPath, "%s%s", strBuf, KLEP_DATABK);
	
    string UpdataDataPath = szUpdataDataPath;
	
	return UpdataDataPath;


}

void CVersionMakerDlg::SetVersionNumber(int Number)
{
		CString	strBuf;
	char	szVersionFile[KLELNAMELEN];
	

	m_DesPath.GetWindowText(strBuf);
	sprintf(szVersionFile, "%s%s", strBuf, KLEP_VERSIONFILE);
	fstream VersionInfoFile;
	VersionInfoFile.open(szVersionFile,ios::out);
	if (VersionInfoFile.is_open())
	{
		m_nCurVersion = Number;
		VersionInfoFile <<Number<<endl;

		
		VersionInfoFile <<m_BackUpStatus;


	

	}

		
}

void CVersionMakerDlg::OnResume() 
{
	// TODO: Add your control notification handler code here

	CString			strBuf;

	m_DesPath.GetWindowText(strBuf);
	if ( false == KLEFilePathMgr::IsPathExist(strBuf) || strBuf.GetLength() == 0)
	{
		MessageBox("恢复失败");
		
		return ;
	}
    
	if(m_nCurVersion >0 && m_BackUpStatus ==0 )
	{
		
		DeleteADirectoryAllFile(GetUpdataDataFolderName().c_str());
		BackupUpDateFile(GetUpdateDataBackUpFolderName().c_str(),GetUpdataDataFolderName().c_str());
		
		SetVersionNumber(m_nCurVersion -1);
		
		MessageBox("成功恢复");
		SetBackUpStatus(TRUE);
	}
	else
	{
		MessageBox("恢复失败");
	}



}


void CVersionMakerDlg::SetBackUpStatus(int Status)
{

	CString	strBuf;
	
	char	szVersionFile[KLELNAMELEN];

	m_DesPath.GetWindowText(strBuf);
	sprintf(szVersionFile, "%s%s", strBuf, KLEP_VERSIONFILE);
	fstream VersionInfoFile;
	VersionInfoFile.open(szVersionFile,ios::out);
	if (VersionInfoFile.is_open())
	{
		VersionInfoFile <<m_nCurVersion<<endl;

		m_BackUpStatus = Status;
		VersionInfoFile <<Status;


	

	}

		






}

void CVersionMakerDlg::SetFileAttributeOfAFolder(const char *FolderName, DWORD dwFileAttributes)
{
	KLEFileEnumerate	FileEnumerate;
	CString				strBuf(FolderName);
    
	vector<string> FilenameSet;
	FileEnumerate.SetCallBack(RecordFileToVector);
	FileEnumerate.SetParam(&FilenameSet);
	FileEnumerate.EnumFileInFolder(strBuf);
    vector<string>::iterator Pointer;
	for(Pointer = FilenameSet.begin();Pointer != FilenameSet.end();Pointer++ )
	{
		if(!SetFileAttributes((*Pointer).c_str(),dwFileAttributes))
		{
			string FileAttributes;
			if(dwFileAttributes& FILE_ATTRIBUTE_ARCHIVE )
			{
				FileAttributes += "FILE_ATTRIBUTE_ARCHIVE|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_HIDDEN )
			{
				FileAttributes += "FILE_ATTRIBUTE_HIDDEN|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_NORMAL )
			{
				FileAttributes += "FILE_ATTRIBUTE_NORMAL|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )
			{
				FileAttributes += "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_OFFLINE )
			{
				FileAttributes += "FILE_ATTRIBUTE_OFFLINE|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_READONLY )
			{
				FileAttributes += "FILE_ATTRIBUTE_READONLY|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_SYSTEM )
			{
				FileAttributes += "FILE_ATTRIBUTE_SYSTEM|";

			}
			if(dwFileAttributes& FILE_ATTRIBUTE_TEMPORARY )
			{
				FileAttributes += "FILE_ATTRIBUTE_TEMPORARY|";

			}


			string ErrorInfo = "SetFileAttributes('" + (*Pointer) +"'," + FileAttributes +")";
			DisplayErrorInfo(ErrorInfo);
		}
	}



}

void CVersionMakerDlg::GetUpdateRelativeFile(vector<string>& FileNameS) const
{
	try
	{
		string UpdateRelativeFilePath = GetInfoFolderName() + string(KLEP_UpdateRelativeFile);
		ifstream in(UpdateRelativeFilePath.c_str());
		auto_ptr<UpdateRelativeFileProcess> UpdateRelativeProcess (new UpdateRelativeFileProcess());
		RecordProcess InfoProcess;
		
		InfoProcess.AddProcessFunction(UpdateRelativeProcess.get());
		
		InfoProcess.AnalyseRecordStream(in);
		
		FileNameS = UpdateRelativeProcess->GetResult();
		
		
		
		
		
	}
	catch(const exception& Error)
	{
		::MessageBox(NULL,Error.what(),"GetUpdateRelativeFile",MB_ICONERROR);
		throw Error;
	}

}

bool CVersionMakerDlg::CheckFileNameInFileS(const string& FileSample, vector<string>& FileGroup) const
{
	vector<string>::iterator Position;
	Position = find_if(FileGroup.begin(),FileGroup.end(),CompStringNotMatchCase(FileSample));
	if(Position == FileGroup.end())
	{
		return false;
	}
	else
	{
		return true;
	}


}

CVersionMakerDlg::~CVersionMakerDlg()
{
	if(FreeLibrary(PackageLib))
	{
		DisplayErrorInfo(string("FreeLibrary"));
		throw exception("Error when FreeLibrary"); 


	}
		

}
