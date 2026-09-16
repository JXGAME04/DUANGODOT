// Autoupdate.h : main header file for the AUTOUPDATE application
//

#if !defined(AFX_AUTOUPDATE_H__D4E4CC3B_07EF_4279_A014_4B15DA3760FA__INCLUDED_)
#define AFX_AUTOUPDATE_H__D4E4CC3B_07EF_4279_A014_4B15DA3760FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateApp:
// See Autoupdate.cpp for the implementation of this class
//
#include <list>
//using namespace std;

class CInternetSession;
class CFtpConnection;
class CInternetException;

struct stFile
{
	char* buffer;
	unsigned int length;
	unsigned int length_downloaded;
	CString strFileName;
	CString strFileNameTemp;		//如果文件很大，必须存储成临时文件
	stFile(){buffer = NULL;length = 0;length_downloaded = 0;}
	~stFile(){delete buffer;}
};

typedef std::list<stFile*> FileList;
typedef FileList::iterator itFile;


class cAppLog;

class CStartDlg;

class CAutoupdateApp : public CWinApp
{
public:
	CAutoupdateApp();
	~CAutoupdateApp();

	CStartDlg* m_pDlg;

	CString m_strFtpServer;
	CString m_strFtpDirectory;
	CString m_strExtractTo;
	CString m_strGame;

	CString m_strMutex;
	HANDLE m_hMutex;

	cAppLog* m_pLog;
	void WriteLogString(LPCSTR szString);

	BOOL IsTheGameRun();
	BOOL IsTheSelfRun();
	void RunGame();

	int	 m_nExtraSpeed;
	void CatchError(CInternetException* pEx);
	
	BOOL UpdateClientVerson();

	BOOL DiskHaveSpace(int size);

	FileList m_listUpdate;
//download
	stFile* m_pCurrentFile;

	unsigned int GetDownloadNum(){return m_listUpdate.size();}
	unsigned int m_nDownloadedNum;
	unsigned int GetDownloadedNum(){return m_nDownloadedNum;}
	unsigned int m_nDownloadedSize;
	unsigned int GetDownloadedSize();
	unsigned int GetDownloadingSize();
	unsigned int GetDownloadingDownloadedSize();

	CString m_strDownloadingFile;
	LPCSTR GetDownloadingFile(){return m_strDownloadingFile;}
	unsigned int m_nTotalSize;
	unsigned int GetDownloadSize(){return m_nTotalSize;}

	
	unsigned int GetExtractNum(){return GetDownloadNum();}
	unsigned int m_nExtractedNum;
	unsigned int GetExtractedNum(){return m_nExtractedNum;}
	unsigned int m_nExtractedSize;
	unsigned int GetExtractedSize();
	unsigned int GetExtractingSize();
	unsigned int m_nExtracting;
	unsigned int GetExtractingExtractedSize();

	CString m_strExtractingFile;
	LPCSTR GetExtractingFile(){return m_strExtractingFile;}
	unsigned int GetExtractSize(){return GetDownloadSize();}

	BOOL GetFileList();
	BOOL InitSession();
	int	 m_nLocalVerson;
	int  m_nServerVerson;
	BOOL GetLocalVerson();
	BOOL GetServerVerson();
	CInternetSession* m_pSession;
	CInternetSession* GetSession(){return m_pSession;}
	BOOL m_bResume;
	BOOL PauseResumeSupported();

	CFtpConnection* GetConnect();
	static void CloseConnect(CFtpConnection* p);

	BOOL m_bFinished;
	BOOL m_bEndThread;
	CWinThread* m_pThread;
	BOOL DownloadAllFiles();
	BOOL DownloadFileToBuffer(stFile& st);
	BOOL DownloadFile(stFile& st);
	BOOL ExtractAllFiles();
	BOOL DeleteAllFiles();
	BOOL ExtractFile(const stFile* st);

	BOOL UpdateFileThread();

	BOOL FtpCommand(	
		HINTERNET hConnect,
		BOOL fExpectResponse,
		DWORD dwFlags,
		LPCTSTR lpszCommand,
		DWORD_PTR dwContext,
		HINTERNET* phFtpCommand,
		BOOL* bIE5
	);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoupdateApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL
	
// Implementation

	//{{AFX_MSG(CAutoupdateApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CAutoupdateApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOUPDATE_H__D4E4CC3B_07EF_4279_A014_4B15DA3760FA__INCLUDED_)
