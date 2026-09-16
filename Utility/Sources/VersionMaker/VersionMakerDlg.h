// VersionMakerDlg.h : header file
//

#if !defined(AFX_VERSIONMAKERDLG_H__885D3D82_372B_45A5_8D2A_8A9FC6384725__INCLUDED_)
#define AFX_VERSIONMAKERDLG_H__885D3D82_372B_45A5_8D2A_8A9FC6384725__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include"KFileInfo.h"
#include<strstream>
#include"FilePath.h"

#define PackFileName "Update"
#define CompressMethod 1
#define FileCommentFileName "FilesComment.ini"
#define RemoteRelativeFolder ""
#define ResourcePackName "update.pak"
#define VersionConfigFileName "Version.cfg"
#define ServerUpdateFolder    "."
#define LocalDataFolder       "%PROGRAMDIR\\DATA\\"
#include<fstream>
#include"FileUpdateAttribute.h"
#include "SourceDest.h"
#include"RecordProcess.h"
#include"UpdateRelativeFileProcess1.h"
#include<vector>
#include<memory>

using namespace std;




typedef  bool (*MYPROC)(vector<string>& ,const string& ,string&);
void DisplayErrorInfo(string& ErrorInfo = string(""));

class CVersionMakerDlg : public CDialog
{
public:
	virtual  ~CVersionMakerDlg();
	void BackupUpDateFile(const char * SrcFolder,const char * BackUpFolder);
	void DeleteADirectoryAllFile(const char * DirectoryName);
	string GetversionHalfPath(int nOldVersion);
	CVersionMakerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVersionMakerDlg)
	enum { IDD = IDD_VERSIONMAKER_DIALOG };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVersionMakerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void InNotRootFileProcess(vector<FilePath>& FileName,int nOldVersion,string RootName);
	virtual string CreateVersionFolder(int nOldVersion);
	virtual string GetUpdataDataFolderName();
	virtual string GetVersionDataFolderName();
	virtual void InRootFileProcess(vector<FilePath>& FileName,string& ThisVersionFileFolderName) const ;
	HICON			m_hIcon;
	CEdit			m_DesPath;
	CEdit			m_SrcPath;
	int				m_nCurVersion;
	KFileInfoVector	m_NewFileListVector;
	int             m_BackUpStatus;
	MYPROC          ProcAdd; 

	HMODULE PackageLib;
	bool	VerifyInfo();
	bool	CopySrcToDes();
	bool	MakeFileList();
	void	SortFileListInfo();
	void	OutputToFileLsit(const char* szFileListName);
	void	MakeVersionInfo();
	void	MakeVersionInfo(int VersionNumber);
	bool	GetUpdateFileNameOnThisVersion(const char* szOldFileList,vector<string >& UpdateFileName);
	bool	ReadFileList(KFileInfoVector& FileInfoVector, const char* szFileListName);
	void	RefreshVersionInfo();
	
	// Generated message map functions
	//{{AFX_MSG(CVersionMakerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSrcbrowse();
	afx_msg void OnDesbrowse();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnChangeDesPath();
	afx_msg void OnResume();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


	friend void CopyFileCB(const char* szFileName, void* pParam);
	friend void MakeFileListCB(const char* szFileName, void* pParam);
private:
	bool CheckFileNameInFileS(const string& FileSample, vector<string>& FileGroup) const ;
	virtual void GetUpdateRelativeFile(vector<string>& FileNameS) const;
	void SetFileAttributeOfAFolder(const char * FolderName,DWORD  dwFileAttributes );
	void SetBackUpStatus(int Status);
	void SetVersionNumber(int Number);
	string GetUpdateDataBackUpFolderName();
	string GetIndexFolder();
	void RunAutoBat(const string& DataPath,const string&  ToolsPath,const string& InfoPath,const string& IndexPath);
	string GetInfoFolderName() const;
	string GetToolsFolderName();
	void CreateVersionConfigFile();
	fstream FileComment;
	void PickOutFileNameBetweenRootAndNotRoot(vector<string>& FileNameArray ,vector<FilePath>& FileNameInRoot,vector<FilePath>& FileNameNotInRoot,const string& RootPath);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERSIONMAKERDLG_H__885D3D82_372B_45A5_8D2A_8A9FC6384725__INCLUDED_)
