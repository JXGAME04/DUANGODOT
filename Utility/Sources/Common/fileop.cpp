#include "stdafx.h"
#include "fileop.h"

#include "shellapi.h"
#include "ATLCONV.H"
#include "afxsock.h"
#include "iItem.h"
#include "stringvalue.h"

const int c_nMaxPath = 256;
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) void AppParsePath(LPCSTR sFullPath, CString& sPath)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
}

_declspec(dllexport) void AppParseDisk(LPCSTR sFullPath, CString& sDisk)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.Find(':');
	ASSERT(pos != -1);
	sDisk = szFullPath.Left(pos);
}

_declspec(dllexport) void AppParsePathName(LPCSTR sFullPath, CString& sPath, CString& sName)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
	sName = szFullPath.Mid(pos + 1);
}

_declspec(dllexport) void AppParseFileName(LPCSTR sFile, CString& sName, CString& sExt)
{
	sName = sFile;
	int pos = sName.ReverseFind('.');
	if (pos != -1)
	{
		sExt = sName.Mid(pos+1);
		sName = sName.Left(pos);
	}
}

_declspec(dllexport) void AppParsePathNameExt(LPCSTR sFullPath, CString& sPath, CString& sName, CString& sExt)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
	sName = szFullPath.Mid(pos + 1);
	pos = sName.ReverseFind('.');
	if (pos != -1)
	{
		sExt = sName.Mid(pos+1);
		sName = sName.Left(pos);
	}
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) void AppGetModuleFile(CString& strFile)
{
	char str[c_nMaxPath];
	GetModuleFileName(GetModuleHandle(NULL),str,c_nMaxPath);
	strFile = str;
}

_declspec(dllexport) void AppGetWorkingDisk(CString& sDisk)
{
	CString s;
	AppGetWorkingFolder(s);
	AppParseDisk(s,sDisk);
}

_declspec(dllexport) void AppGetWorkingFolder(CString& strWorkingFolder)
{
	char str[c_nMaxPath];
	GetModuleFileName(GetModuleHandle(NULL),str,c_nMaxPath);
	AppParsePath(str,strWorkingFolder);
}

_declspec(dllexport) void AppGetExeName(CString& strAppName)
{
	char str[c_nMaxPath];
	GetModuleFileName(GetModuleHandle(NULL),str,c_nMaxPath);
	CString sPath,sExt;
	AppParsePathNameExt(str,sPath,strAppName,sExt);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppCopyFile(LPCSTR szSrc,LPCSTR szDes,BOOL bFailIfExists)
{
	AppBuildPath(szDes);
	BOOL b = CopyFile(szSrc,szDes,bFailIfExists);
	return b;
}

BOOL AppFileOperator(LPCSTR lpszSrc, LPCSTR lpszDest, int op)
{
	SHFILEOPSTRUCT stFileOP;
	ZeroMemory(&stFileOP,sizeof(stFileOP));

	ASSERT(strlen(lpszSrc) < c_nMaxPath-1);
	char bufferSrc[c_nMaxPath];
//	bufferSrc[strlen(lpszSrc)] = 0;
	ZeroMemory(&bufferSrc,c_nMaxPath);
	strcpy(bufferSrc,lpszSrc);
	stFileOP.pFrom = bufferSrc;
//	stFileOP.pFrom = lpszSrc;

	ASSERT(strlen(lpszDest) < c_nMaxPath-1);
	char bufferDest[c_nMaxPath];
//	bufferSrc[strlen(lpszDes)] = 0;
	ZeroMemory(&bufferDest,c_nMaxPath);
	strcpy(bufferDest,lpszDest);
	stFileOP.pTo = bufferDest;
//	stFileOP.pTo = lpszDest;

	stFileOP.wFunc = op;
	stFileOP.fFlags = FOF_SILENT | FOF_NOCONFIRMATION; 

	return SHFileOperation(&stFileOP) == 0;
}

_declspec(dllexport) BOOL AppDeleteFolder(LPCSTR lpszFolderPathName)
{
	return AppFileOperator(lpszFolderPathName,"",FO_DELETE);
}

_declspec(dllexport) BOOL AppMoveFolder(LPCSTR lpszSrc, LPCSTR lpszDest)
{
	return AppFileOperator(lpszSrc,lpszDest,FO_MOVE);
}

_declspec(dllexport) BOOL AppCopyFolder(LPCSTR lpszSrc, LPCSTR lpszDest)
{
	return AppFileOperator(lpszSrc,lpszDest,FO_COPY);
}

_declspec(dllexport) BOOL AppRenameFolder(LPCSTR lpszSrc, LPCSTR lpszDest)
{
	return AppFileOperator(lpszSrc,lpszDest,FO_RENAME);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppBuildPath(LPCSTR szDes)
{
	CString sDes = szDes;
	if (sDes.Right(1) != '\\' && sDes.GetAt(sDes.GetLength()-4) != '.')
		sDes += '\\';
	int nStart = 0;
	if (sDes.GetAt(0) == '\\' && sDes.GetAt(1) == '\\')
	{
		nStart = sDes.Find('\\',2);
		if (nStart == -1)
			return FALSE;
		nStart++;
	}
	do
	{
		nStart = sDes.Find('\\',nStart);
		if (nStart == -1)
			break;
		CFileFind f;
		CString strSub = sDes.Left(nStart);
		BOOL b = f.FindFile(strSub);
		if (!strSub.IsEmpty() && !b)
		{
			BOOL b = CreateDirectory(strSub,NULL);
			//ASSERT(b);
			if (!b)
			{
/*
				CString strMsg = "copy file failure for can not create dir: ";
				strMsg += strSub;
				AfxMessageBox(strMsg);
				return false;
*/
			}
		}
		nStart++;
	}while(1);
	return true;
}

_declspec(dllexport) BOOL AppGetShortPath(CString& sPath)
{
	sPath.MakeLower();
	CString sWork;
	AppGetWorkingFolder(sWork);
	sWork.MakeLower();
	if (sPath.Find(sWork) == 0)
		sPath = sPath.Mid(sWork.GetLength());
	return TRUE;
}

_declspec(dllexport) BOOL AppGetFullPath(CString& sPath)
{
	ASSERT(!sPath.IsEmpty());
	CString sWork;
	AppGetWorkingFolder(sWork);
	if (sPath.Find(':') != -1 || sPath.Find("\\\\") != -1)
		return TRUE;
	if (sPath.Find(sWork) == -1)
	{
		if (sPath.GetAt(0) == '\\')
			sPath = sWork + sPath;
		else
			sPath = sWork + "\\" + sPath;
	}
	return TRUE;
}

bool IsLine(char c)
{
	return c == '\\' || c == '/';
}

bool IsLineLeft(LPCSTR sz)
{
	CString s = sz;
	char c = s.GetAt(0);
	return IsLine(c);
}

bool IsLineRight(LPCSTR sz)
{
	CString s = sz;
	char c = s.GetAt(s.GetLength() - 1);
	return IsLine(c);	
}

_declspec(dllexport) BOOL AppAddPath(CString& sPath,LPCSTR szFolder)
{
	if (IsLineRight(sPath))
	{
		if (IsLineLeft(szFolder))
		{
			char* sz = (char*)szFolder;
			sz++;
			sPath += sz;
		}
		else
			sPath += szFolder;
	}
	else
	{
		if (IsLineLeft(szFolder))
			sPath += szFolder;
		else
		{
			sPath += "\\";
			sPath += szFolder;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
BOOL AppFindFileA(LPCSTR sFileName,LPCSTR lpszFolderPathName, CString& sPathName)
{
	CFileFind finder;
	CString strPattern;
	
	BOOL bFound;

	strPattern = lpszFolderPathName;
	if(strPattern[strPattern.GetLength() - 1] != _T('\\'))
		strPattern += "\\";
	strPattern += sFileName;
	if (finder.FindFile(strPattern))
	{
		finder.FindNextFile();
		sPathName = finder.GetFilePath();
		return TRUE;
	}

	strPattern = lpszFolderPathName;
	if(strPattern[strPattern.GetLength() - 1] != _T('\\'))
		strPattern += "\\";
	strPattern += _T("*.");
	
	bFound = finder.FindFile(strPattern);
	while(bFound)
	{
		bFound = finder.FindNextFile();
		if(finder.IsDots()) continue;
		if(finder.IsDirectory())
		{
			if(AppFindFileA(sFileName,finder.GetFilePath(),sPathName))
			{
				finder.Close();
				return TRUE;
			}
		}
	}
	return FALSE;
}

_declspec(dllexport) BOOL AppFindFile(LPCSTR sFileName,LPCSTR lpszFolderPathName, CString& sPathName)
{	
	if (!AppFindFileA(sFileName,lpszFolderPathName,sPathName))
	{
		CString strErr;
		strErr.Format("Can not find system file: '%s', 请更新这个文件",sFileName);
//		AfxMessageBox(strErr);
		return FALSE;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppGetPathDialog(CString& strPath, LPCSTR szRoot)
{
	BROWSEINFO bi;
	char diskname[MAX_PATH];
	char path[MAX_PATH];

	ITEMIDLIST *pidl;
	bi.hwndOwner=AfxGetMainWnd()->GetSafeHwnd();

	bi.pidlRoot=0;
	bi.pszDisplayName=diskname;
	bi.lpszTitle="select a path";
	bi.ulFlags=BIF_RETURNONLYFSDIRS;
	bi.lpfn=0;
	bi.lParam=0;
	bi.iImage=0;

	if( pidl = SHBrowseForFolder(&bi) )
	{
		SHGetPathFromIDList(pidl, path);
		strPath = path;
		return TRUE;
	}
	return FALSE;
}

_declspec(dllexport) BOOL AppGetFileDialog(CString& strFile,LPCSTR szPath,LPCSTR szFilter, BOOL Load)
{
	char tmp[256];
	strcpy(tmp,szFilter);
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
	ofn.hInstance = AfxGetInstanceHandle ();
	ofn.lpstrFilter = szFilter;		// file types
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 0;		// index of first one
	ofn.nMaxCustFilter = 0;	
	ofn.lpstrFile = tmp;		// initial file suggested
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = NULL;		// dlg title
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = szPath;   // initial path
	ofn.Flags = OFN_SHOWHELP | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_SHAREAWARE;
	ofn.lpstrDefExt = szFilter ;		// default extension
	ofn.lpfnHook = NULL;

	CString s;
	if( Load )	//??
	{
		if( GetOpenFileName((LPOPENFILENAME)&ofn)) 
			strFile = ofn.lpstrFile;
	}
	else	//??
	{
		if( GetSaveFileName((LPOPENFILENAME)&ofn) ) 
			strFile = ofn.lpstrFile;
	}
	if (strFile.IsEmpty())
		return FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppOpenFile(CFile& file, LPCSTR FileName, long flag, LPCSTR szLocate)
{
	if (strlen(FileName) == 0)
	{
		CString strErr;
		strErr.Format("file error:open a null file at %s",szLocate);
		AfxMessageBox(strErr);
		return FALSE;
	}
	CString strFile = FileName;
	AppGetFullPath(strFile);
	CFileException e;
	if (!file.Open(strFile,flag ,&e))
	{
		char s[c_nMaxPath];
		e.GetErrorMessage(s,c_nMaxPath);
		CString strErr;
		strErr.Format("file:%s open error:%s at %s",FileName, s, szLocate);
//		AfxMessageBox(strErr);
		return FALSE;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppGetHostName(CString& sName)
{
	int num = 0;
	char buffer[256];
	VERIFY(gethostname(buffer,256) != SOCKET_ERROR);
	sName = buffer;
	return TRUE;
}

_declspec(dllexport) BOOL AppWriteSimpleLogFile(LPCSTR szLogFile)
{
	CStdioFile f;
	if (f.Open(szLogFile,CFile::modeWrite|CFile::modeCreate|CFile::modeNoTruncate))
	{
		f.SeekToEnd();
		CString sHost;
		AppGetHostName(sHost);
		CTime time = CTime::GetCurrentTime();
		CString sTime = time.Format( "%H:%M, %B %d, %Y" );
		CString sMsg = sHost + " modify the file at " + sTime + "\n";
		f.WriteString(sMsg);
		return TRUE;
	}	
	return FALSE;
}

_declspec(dllexport) BOOL AppGetDynamicNum(LPCSTR szKey, long& num)
{
	num = 0;
	char buffer[256];
	VERIFY(gethostname(buffer,256) != SOCKET_ERROR);
	CString strHost = buffer;
	ASSERT(!strHost.IsEmpty());
	CString strDynamic = "dynamic\\dynamic_"+strHost+".ini";
	iItem* pItem = Common_CreateItem(strDynamic,iItem::FLAG_LOAD);
	if (pItem == NULL)
		pItem = Common_CreateItem(strDynamic,iItem::FLAG_CREATE);
	else
		GetItemValue(pItem,szKey,num);		
	wrap_ptr<iItem> ptr(pItem);
	num++;
	SetItemValue(pItem,szKey,num);
	pItem->Save();	
	return num;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL AppIsFileOld(LPCSTR szFile)
{
	CFileFind f; 
	if (f.FindFile(szFile))
	{
		CTime time;
		f.GetLastWriteTime(time);
		CString strFileExe;
		AppGetModuleFile(strFileExe);
		if (f.FindFile(strFileExe))
		{
			CTime timeExe;
			f.GetLastWriteTime(timeExe);
			return (time < timeExe);
		}
	}
	return TRUE;
}
