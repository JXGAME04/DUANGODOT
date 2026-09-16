// Autoupdate.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Autoupdate.h"
#include "AutoupdateDlg.h"
#include "iitem.h"

#include "clog.h"
#include "fileop.h"

#include "izip.h"
#include "ctimer.h"

#include "StartDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateApp

BEGIN_MESSAGE_MAP(CAutoupdateApp, CWinApp)
	//{{AFX_MSG_MAP(CAutoupdateApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateApp construction
class wrap_connect
{
public:
	CFtpConnection* m_pConnect;
	wrap_connect(CFtpConnection* p) : m_pConnect(p){};
	~wrap_connect(){CAutoupdateApp::CloseConnect(m_pConnect);}
};

#define bindconnect(p) 	CFtpConnection* p = GetConnect();\
						if (p == NULL)\
							return false;\
						wrap_connect ptr_##p(p);


const int c_nExtraSpeed = 2500 * 1024;

CAutoupdateApp::CAutoupdateApp()
{
	m_pSession = NULL;
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_pCurrentFile = NULL;
	m_nExtraSpeed = c_nExtraSpeed;
	m_pLog = new cAppLog;
	m_hMutex = NULL;
	m_pDlg = new CStartDlg;
}

CAutoupdateApp::~CAutoupdateApp()
{
//	DeleteAllFiles();
	delete m_pSession;
	for (itFile it = m_listUpdate.begin(); it != m_listUpdate.end(); ++it)
		delete (*it);
	delete m_pLog;
	if (m_hMutex)
		CloseHandle(m_hMutex);
	if (m_pDlg)
	{
		m_pDlg->DestroyWindow();
		delete m_pDlg;
	}
}

void CAutoupdateApp::WriteLogString(LPCSTR szString)
{
	m_pLog->write_log(szString);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAutoupdateApp object

CAutoupdateApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateApp initialization
void CAutoupdateApp::CloseConnect(CFtpConnection* p)
{
	if (p != NULL)
		p->Close();
	delete p;
}

CFtpConnection* CAutoupdateApp::GetConnect()
{
	CFtpConnection* p = NULL;
	try
	{
		p = m_pSession->GetFtpConnection(_T(m_strFtpServer));
		CString s;
		p->GetCurrentDirectory(s);
		if (!p->SetCurrentDirectory(m_strFtpDirectory))
			return NULL;
	}
	catch(CInternetException* pEx)
	{
		CatchError(pEx);
		delete p;
		p = NULL;
	}
	return p;
}

BOOL CAutoupdateApp::DownloadFileToBuffer(stFile& st)
{
	bindconnect(pConnect);
	//Download a.zip file and put it as download\a.zip 
	//but how to deal with disk not enough ?
	
	try
	{
		CInternetFile* pFile = pConnect->OpenFile((LPCTSTR)st.strFileName);

		st.buffer = new char[st.length+1];
		st.buffer[st.length] = 0;
		pFile->Read(st.buffer,st.length);

		pFile->Close();
		delete pFile;
	}
	catch(CInternetException* pEx)
	{
		CatchError(pEx);
		return FALSE;
	}

	return TRUE;
}

#define IDS_DOWNLOAD "Downloaded"
BOOL CAutoupdateApp::DownloadFile(stFile& st)
{
	st.strFileNameTemp = (CString)IDS_DOWNLOAD+"\\"+st.strFileName;
	AppGetFullPath(st.strFileNameTemp);
	CFileFind f;
	if (f.FindFile(st.strFileNameTemp))
	{
		f.FindNextFile();
		DWORD len = f.GetLength();
		if (len == st.length)
		{
			st.length_downloaded = len;
			return TRUE;
		}
		else if (m_bResume)
		{
			st.length_downloaded = len;
		}
		else
		{
			st.length_downloaded = 0;
		}
	}
	
	bindconnect(pConnect);
	//Download a.zip file and put it as download\a.zip 
	//but how to deal with disk not enough ?
	try
	{
		CFile file;
		
		st.strFileNameTemp = "Downloaded\\"+st.strFileName;
		AppGetFullPath(st.strFileNameTemp);
		AppBuildPath(st.strFileNameTemp);

		if (st.length_downloaded != 0) //resume
		{
			//Seek to the file first
			CString strLen;
			HINTERNET hResponse;

			strLen.Format("%d",st.length_downloaded);
			CString strSeekOffsetCMD = "REST " + strLen;

			BOOL bIE5 = FALSE;
			BOOL bRet = FtpCommand(*pConnect, // WinInet Connection handle
							   FALSE,        // No, I don't expect a response
							   FTP_TRANSFER_TYPE_BINARY, // I'm receiving ASCII
							   strSeekOffsetCMD,      // This is the FTP command I am passing
							   0,           // No context needed
							   &hResponse,
							   &bIE5	); // The handle to read the response 
			ASSERT(bIE5);
			if (!bRet)
			{
				CString strMsg;
				strMsg.Format("Error:%d", GetLastError());
				AfxMessageBox(strMsg, MB_OK);
				return FALSE;
			}
			if ( !file.Open(st.strFileNameTemp,CFile::modeWrite) )
			{
				return FALSE;
			}
			file.SeekToEnd();
		}
		else
		{
			if (! file.Open(st.strFileNameTemp,CFile::modeCreate | CFile::modeWrite))
				return FALSE;
		}

		CInternetFile* pFile = pConnect->OpenFile((LPCTSTR)st.strFileName);


		//速率40k/s
		const int BUF_SIZE = 40960;
		char buffer[BUF_SIZE];
	
		int length_downloading = st.length - st.length_downloaded;
		while (length_downloading > 0)
		{
			if (m_bEndThread == TRUE)
			{
				pFile->Close();
				delete pFile;
				return FALSE;
			}
			// read remote data into buffer			
			cTimerLimit timer(100);

			int sz = BUF_SIZE;
			if (length_downloading < BUF_SIZE)
				sz = length_downloading;
			int size = pFile->Read(buffer, sz);

			file.Write(buffer,size);
			length_downloading -= size;
			st.length_downloaded += size;
		}

		pFile->Close();
		delete pFile;
	}
	catch(CInternetException* pEx)
	{
		CatchError(pEx);
		return FALSE;
	}
	
	return TRUE;
}

BOOL CAutoupdateApp::ExtractFile(const stFile* st)
{
	if (st == NULL)
		return FALSE;

	if (st->strFileNameTemp.IsEmpty())
		return FALSE;

	iZipExtract* pZip = Zip_CreateZipExtract(zip_normal);
	wrap_ptr<iZipExtract> ptr(pZip);
	if (pZip->load(st->strFileNameTemp))
	{
		CString s = m_strExtractTo;
		if (s.IsEmpty())
			AppGetWorkingFolder(s);
		else
			AppGetFullPath(s);

		const DWORD size_extra = 1024*1024;

		DWORD size = pZip->get_uncompressedsize();
		m_nExtraSpeed = 0;
		while(1)
		{
			if (DiskHaveSpace(size + size_extra))
				break;
			else
			{
				CString sMessage;
				sMessage.Format("disk space is not enough! need %dK(%dM) size\n please clearn disk then click yes",size / 1024,size / (1024*1024));
				if ( AfxMessageBox(sMessage,MB_YESNO) == IDNO)
					return FALSE;
			}		
		}
		m_nExtraSpeed = c_nExtraSpeed;
		if (pZip->extract(s))
		{
			//extract may be failed but there is no error handle
			return TRUE;
		}
	}
	CString s;
	s.Format("Failed when extract file %s, Please remove the file's readonly attribute, or the file may be corrupt, Autoupdate Abort!",st->strFileName);
	AfxMessageBox(s);
/*
	cZip zip;
	if (zip->ReadBuffer(st->buffer))
	{
		if (zip->Unzip(AppGetWorkingFolder()))
			return FALSE;
	}
*/
	return FALSE;
}

BOOL CAutoupdateApp::UpdateClientVerson()
{
	iItem* pItem = Common_CreateItem("autoupdate.ini",iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	m_nLocalVerson = m_nServerVerson;
	SetItemValue(pItem,"verson",m_nLocalVerson);
	pItem->Save();
	return TRUE;	
}

BOOL CAutoupdateApp::GetLocalVerson()
{
	m_strMutex = "Worldeditor";
	m_nLocalVerson = 0;
	m_strFtpServer = "192.168.26.136";
	m_strFtpDirectory = "test";
	m_strGame = "s3client.exe";
	iItem* pItem = Common_CreateItem("autoupdate.ini",iItem::FLAG_LOAD);
	if (pItem == NULL)
		return TRUE;
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"mutex",m_strMutex);
	GetItemValue(pItem,"verson",m_nLocalVerson);
	GetItemValue(pItem,"ftpserver",m_strFtpServer);
	GetItemValue(pItem,"ftpdirectory",m_strFtpDirectory);
	GetItemValue(pItem,"extractto",m_strExtractTo);
	GetItemValue(pItem,"game",m_strGame);
	if (m_nLocalVerson < 0)
		m_nLocalVerson = 0;
	return TRUE;
}

void CAutoupdateApp::CatchError(CInternetException* pEx)
{
	TCHAR sz[1024];
	pEx->GetErrorMessage(sz, 1024);
	CString s = "ERROR!  ";
	s += sz;
	s += "\n";
	OutputDebugString(s);
	AfxMessageBox(s);
	pEx->Delete();
}

BOOL CAutoupdateApp::GetServerVerson()
{
	bindconnect(pConnect);

	CFtpFileFind f(pConnect);
	CString s;
	pConnect->GetCurrentDirectory(s);
	try
	{
		BOOL b = f.FindFile(_T("version.ini"));
		if (!b)
		{
			s = "can not find file verson.ini under " + s;
			AfxMessageBox(s);
			return FALSE;
		}
		f.FindNextFile();
		m_nServerVerson = -1;
	}
	catch (CInternetException* pEx)
	{
		CatchError(pEx);
		return FALSE;
	}

	try
	{
		stFile st;
		st.length = f.GetLength();
		st.strFileName = f.GetFileName();
		if (!DownloadFileToBuffer(st))
			return FALSE;
		m_nServerVerson = atoi(st.buffer);
	}
	catch (CInternetException* pEx)
	{
		CatchError(pEx);
		return FALSE;
	}
	if (m_nServerVerson < 0)
	{
		AfxMessageBox("the server verson is < 0!");
		return FALSE;
	}
	return TRUE;
}

void OnExecute(LPCSTR szFile)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	char buffer[256];

	CString s = szFile;
	AppGetFullPath(s);
	strcpy(buffer,LPCSTR(s));

    // Start the child process. 
    if( !CreateProcess( NULL, // No module name (use command line). 
		buffer,				// Command line. 
		NULL,				// Process handle not inheritable. 
		NULL,				// Thread handle not inheritable. 
		FALSE,				// Set handle inheritance to FALSE. 
		0,					// No creation flags. 
		NULL,				// Use parent's environment block. 
		NULL,				// Use parent's starting directory. 
		&si,				// Pointer to STARTUPINFO structure.
		&pi )				// Pointer to PROCESS_INFORMATION structure.
    ) 
    {
        //ErrorExit( "CreateProcess failed." );
		//ASSERT(0);
		s = "update ok! but can not execute " + s + " !";
		AfxMessageBox(s);
		return;
    }


	// Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

void CAutoupdateApp::RunGame()
{
	OnExecute(m_strGame);
}

BOOL CAutoupdateApp::IsTheSelfRun()
{
	m_hMutex = CreateMutex(NULL,FALSE,"Autoupdate");
	if(m_hMutex)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CAutoupdateApp::IsTheGameRun()
{
	HANDLE m_hGMutex = CreateMutex(NULL,FALSE,m_strMutex);
	if(m_hGMutex)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(m_hGMutex);
			return TRUE;
		}
	}
	CloseHandle(m_hGMutex);
	return FALSE;
}

BOOL CAutoupdateApp::InitSession()
{
	m_pSession = new CInternetSession(_T("MyProgram/1.0"));

	bindconnect(pConnect);

	return TRUE;
}

BOOL CAutoupdateApp::GetFileList()
{
	int nCurrentVerson = m_nLocalVerson;
	m_nTotalSize = 0;

	//if find serverno.zip, auto update to it!
	int nServerVerson = m_nServerVerson;
	
	while(nServerVerson > nCurrentVerson)
	{
		bindconnect(pConnect);
		CFtpFileFind f(pConnect);
		CString sFind;
		sFind.Format("%06d.zip",nServerVerson);
		BOOL b = f.FindFile(sFind);
		if (b)
		{
			f.FindNextFile();
			stFile* pFile = new stFile;
			pFile->length = f.GetLength();
			pFile->strFileName = f.GetFileName();
			m_nTotalSize += pFile->length;
			m_listUpdate.push_back(pFile);
			nCurrentVerson = nServerVerson;
			break;
		}
		nServerVerson--;
	}

	while(nCurrentVerson != m_nServerVerson)
	{
		int nMaxVerson = nCurrentVerson;
		CString strMaxName;
		int nMaxLength;

		bindconnect(pConnect);
		CFtpFileFind f(pConnect);
		CString sFind;
		sFind.Format("%06d_??????.zip",nCurrentVerson);
		BOOL b = f.FindFile(sFind);
		if (!b)
			return FALSE;
		do 
		{
			b = f.FindNextFile();
			CString sName = f.GetFileName();
			int pos = sName.Find('_');
			ASSERT(pos != -1);
			CString sVer = sName.Mid(pos + 1);
			int nVer = atoi(sVer);
			if (nVer > nMaxVerson)
			{
				nMaxVerson = nVer;
				strMaxName = sName;
				nMaxLength = f.GetLength();
				ASSERT(nMaxLength >= 0);
			}
		}
		while(b);
		if (nCurrentVerson < nMaxVerson)
		{
			stFile* pFile = new stFile;
			pFile->length = nMaxLength;
			pFile->strFileName = strMaxName;
			m_nTotalSize += pFile->length;
			m_listUpdate.push_back(pFile);
			nCurrentVerson = nMaxVerson;
		}
		else
			return FALSE;
	}

	return TRUE;
}

BOOL CAutoupdateApp::DeleteAllFiles()
{
	BOOL bRet = TRUE;
	CString strDownload = IDS_DOWNLOAD;
	AppGetFullPath(strDownload);
	bRet = AppDeleteFolder(strDownload);
/*
	FileList& flist = m_listUpdate;
	for (itFile it = flist.begin(); it != flist.end(); ++it)
	{
		stFile* pFile = (*it);
		if (!pFile->strFileNameTemp.IsEmpty())
		{
			BOOL b = DeleteFile(pFile->strFileNameTemp);
			pFile->strFileNameTemp.Empty();
			if (!b)
			{
				bRet = FALSE;
			}
		}
	}
*/
	return bRet;
}

BOOL CAutoupdateApp::ExtractAllFiles()
{
	m_nExtractedNum = 0;
	m_nExtractedSize = 0;

	FileList& flist = m_listUpdate;
	for (itFile it = flist.begin(); it != flist.end(); ++it)
	{
		stFile* pFile = (*it);
		m_pCurrentFile = pFile;
		m_strExtractingFile = pFile->strFileName;
		if (!ExtractFile(pFile))
			return FALSE;
		if (m_bEndThread)
			return FALSE;
		m_nExtractedNum ++;
		m_nExtractedSize += pFile->length;
	}
	return TRUE;
}

BOOL CAutoupdateApp::DownloadAllFiles()
{
	m_nDownloadedNum = 0;
	m_nDownloadedSize = 0;

	FileList& flist = m_listUpdate;
	for (itFile it = flist.begin(); it != flist.end(); ++it)
	{
		stFile* pFile = (*it);
		m_pCurrentFile = pFile;
		m_strDownloadingFile = pFile->strFileName;
		if (!DownloadFile(*pFile))
			return FALSE;
		if (m_bEndThread)
			return FALSE;
		m_nDownloadedNum ++;
		m_nDownloadedSize += pFile->length;
	}
	return TRUE;
}

UINT UpdateFile(LPVOID p)
{
	CAutoupdateApp* pApp = (CAutoupdateApp*) p;
	if (pApp->DownloadAllFiles())
	{
		if (pApp->ExtractAllFiles())
		{
			pApp->m_pCurrentFile = NULL;
			if (pApp->DeleteAllFiles())
			{
			}
			pApp->m_bFinished = TRUE;
			return TRUE;
		}
		else
		{
			//当extract出错的时候,删除临时文件 
			pApp->DeleteAllFiles();
		}
	}
	//当download出错的时候,不删除临时文件,下次可以续传
	//pApp->DeleteAllFiles();
	pApp->m_bFinished = -1;
	return 0;
}

BOOL CAutoupdateApp::UpdateFileThread()
{
	m_bEndThread = FALSE;
	m_bFinished = FALSE;
	m_pThread = AfxBeginThread(UpdateFile,this);
	return TRUE;
}

unsigned int CAutoupdateApp::GetDownloadedSize()
{
	return m_nDownloadedSize + GetDownloadingDownloadedSize();
}

unsigned int CAutoupdateApp::GetDownloadingSize()
{
	if (m_pCurrentFile != NULL)
		return m_pCurrentFile->length;
	return 0;
}

unsigned int CAutoupdateApp::GetDownloadingDownloadedSize()
{
	if (m_pCurrentFile != NULL)
		return m_pCurrentFile->length_downloaded;
	return 0;
}

unsigned int CAutoupdateApp::GetExtractedSize()
{
	return m_nExtractedSize + GetExtractingExtractedSize();
}

unsigned int CAutoupdateApp::GetExtractingSize()
{
	if (m_pCurrentFile != NULL)
		return m_pCurrentFile->length;
	return 0;
}

unsigned int CAutoupdateApp::GetExtractingExtractedSize()
{
	m_nExtracting += m_nExtraSpeed;
	if (m_nExtracting > GetExtractingSize())
		m_nExtracting = GetExtractingSize();
	return m_nExtracting;
}

//for can not get so much large file,size always < max_ulong, so use int not unsigned int!
BOOL CAutoupdateApp::DiskHaveSpace(int size)
{
	CString szDisk;
	AppGetWorkingFolder(szDisk);
	if (szDisk.Right(szDisk.GetLength() - 1) != "\\")
		szDisk += "\\";
	ULARGE_INTEGER i64FreeBytesToCaller,i64TotalBytes,i64FreeBytes;
	BOOL b = GetDiskFreeSpaceEx (szDisk,
				&i64FreeBytesToCaller,
				&i64TotalBytes,
				&i64FreeBytes);
	if (b)
	{
		if (i64FreeBytes.HighPart > 0 || size < i64FreeBytes.LowPart)
			return TRUE;
	}
	return FALSE;
}

typedef BOOL (*FtpCommandFunc) (          
	HINTERNET hConnect,
    BOOL fExpectResponse,
    DWORD dwFlags,
    LPCTSTR lpszCommand,
    DWORD_PTR dwContext,
    HINTERNET* phFtpCommand
);

BOOL CAutoupdateApp::FtpCommand(	
	HINTERNET hConnect,
    BOOL fExpectResponse,
    DWORD dwFlags,
    LPCTSTR lpszCommand,
    DWORD_PTR dwContext,
    HINTERNET* phFtpCommand,
	BOOL* bIE5
)
{
	*bIE5 = TRUE;
	return ::FtpCommand(hConnect,fExpectResponse,dwFlags,lpszCommand,dwContext,phFtpCommand);
/*
    HINSTANCE hinstLib; 
    FtpCommandFunc ProcAdd ; 
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE; 
	BOOL bRet;
 
    // Get a handle to the DLL module.
	*bIE5 = FALSE;
 
    hinstLib = LoadLibrary("wininet"); 
 
    // If the handle is valid, try to get the function address.
    if (hinstLib != NULL) 
    { 
        ProcAdd = (FtpCommandFunc) GetProcAddress(hinstLib, "FtpCommandA"); 
 
        // If the function address is valid, call the function.
 
        if (fRunTimeLinkSuccess = (ProcAdd != NULL)) 
		{
			bRet = (ProcAdd)(hConnect,fExpectResponse,dwFlags,lpszCommand,dwContext,phFtpCommand);
			*bIE5 = TRUE;
		}
 
        // Free the DLL module.
 
        fFreeResult = FreeLibrary(hinstLib); 
    } 

	return bRet;
*/
}

BOOL CAutoupdateApp::PauseResumeSupported()
{
	bindconnect(pConnect);
	HINTERNET hConnect = *pConnect; 

//	BOOL bRet = pConnect->Command("REST 0",CmdRespNone,FTP_TRANSFER_TYPE_ASCII);

	BOOL bIE5 = FALSE;
	HINTERNET hResponse;
	BOOL bRet = FtpCommand(hConnect,FALSE,FTP_TRANSFER_TYPE_ASCII, 
							"REST 0",
							0,           // No context needed
							&hResponse,
							&bIE5); // The handle to read the response 
	if (!bIE5)
		return FALSE;

    if (!bRet)
    {
		CString strMsg;
		strMsg.Format("Error:%d", GetLastError());
		AfxMessageBox(strMsg, MB_OK);
		return FALSE;
    }

	DWORD dwError;
	TCHAR tpszBuff[2*1024];
	DWORD dwBuffLen = 2*1024;
	bRet = ::InternetGetLastResponseInfo(&dwError, tpszBuff, &dwBuffLen); 
	
	if(strstr(tpszBuff, "350")) //a successfull 
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CAutoupdateApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	m_pLog->create();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	if (!m_pDlg->Create(CStartDlg::IDD))
	{
		AfxMessageBox("Autoupdate is corrupt!");
		return FALSE;
	}
	m_pDlg->ShowWindow(SW_SHOW);

	m_pDlg->SetPrompt("Check if there is another autoupdate running");
	if (IsTheSelfRun())
	{
		AfxMessageBox("Autoupdate is running, please wait!");
		return FALSE;
	}
	m_pDlg->SetPrompt("Get the local verson...");
	if (!GetLocalVerson())
	{
		m_nLocalVerson = 0;
		AfxMessageBox("can not get local verson!");
		return FALSE;
	}
	m_pDlg->SetPrompt("Check if the game running...");
	if (IsTheGameRun())
	{
		AfxMessageBox("the game is running! please close it then retry!");
		return FALSE;
	}

	m_pDlg->SetPrompt("Connecting to server...");
	if (!InitSession())
	{
		AfxMessageBox("can not connect to server!");
		return FALSE;
	}
	m_pDlg->SetPrompt("Getting the server verson...");
	if (!GetServerVerson())
	{
		AfxMessageBox("can not get server verson!");
		return FALSE;
	}
	
	m_bResume = PauseResumeSupported();

	if (m_nLocalVerson == m_nServerVerson)
	{
		m_pDlg->SetPrompt("Running the game...");
		RunGame();
		return FALSE;
	}
	else
	{
		m_pDlg->SetPrompt("Getting the files list...");
		if (!GetFileList())
		{
			AfxMessageBox("Can not get update file list!");
			return FALSE;
		}
	}

	m_pDlg->SetPrompt("Getting the files size...");
	DWORD size = GetDownloadSize();
	while(1)
	{
		m_pDlg->SetPrompt("Check if there is enough space...");
		if (DiskHaveSpace(size))
		{
			break;
		}
		else
		{
			CString sMessage;
			sMessage.Format("disk space is not enough! need %dK(%dM) size\n please clearn disk then click yes",size / 1024,size / (1024*1024));
			if ( AfxMessageBox(sMessage,MB_YESNO) == IDNO)
				return FALSE;
		}		
	}

	m_pDlg->SetPrompt("Start the downloading thread...");

	UpdateFileThread();

	SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL );

	m_pDlg->ShowWindow(SW_HIDE);
	
	CAutoupdateDlg dlg;
//	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		RunGame();
/*
		if (UpdateClientVerson())
		{
			m_pDlg->ShowWindow(SW_SHOW);
			m_pDlg->SetPrompt("Running the game...");
		}
*/
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	m_bEndThread = TRUE;
	if (m_pThread)
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);

	
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
