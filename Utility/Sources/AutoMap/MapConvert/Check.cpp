// MapConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"
#include "iItem.h"
#include "iGraphics.h"
#include "..\controls\FileFindEx.h"
#include "math.h"
#include <iomanip>
#include <fstream>
#include <shlwapi.h>
#include "iImage.h"
#include "foreach.h"
#include "KSpbioTemplate.h"

#include "izip.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////
//check file length
/////////////////////////////////////////////////////////////
const int c_nMaxLength = 80;
int g_max_length = 0;
int g_count = 0;

bool IsGraphics(LPCSTR szFile)
{
	CString s = szFile;
	if (s.Find("_group") != -1)
		return 0;
	if (s.Find("美术图素") == -1
		&& s.Find("地图动画") == -1)
		return 0;
	return 1;
}

int on_length(LPCSTR szFile)
{
	if (!IsGraphics(szFile))
		return 0;
	CString sFile = szFile;
	AppGetShortPath(sFile);
	if (sFile.GetLength() >= c_nMaxLength)
	{
		CString sMessage;
		sMessage.Format("file\n '%s' \n length is %d\n",sFile,sFile.GetLength());
		theApp.WriteLogString(sMessage);
		OutputDebugString(sMessage);
//		AfxMessageBox(sMessage);
		g_max_length = max(g_max_length,sFile.GetLength());
		g_count ++;
	}
	return 0;
}

void CMapConvertDlg::OnLength() 
{
	CWaitCursor cursor;
	CString strFolder = "游戏资源";
	CString szFindFile = "*.*";
	AppGetFullPath(strFolder);

	g_max_length = 0;
	g_count = 0;
	foreach_fileunderfolder(on_length,szFindFile,strFolder);
	CString sMessage;
	sMessage.Format("max file length is %d great than %d file is %d",g_max_length,c_nMaxLength,g_count);
	AfxMessageBox(sMessage);
	
	theApp.OpenLog();
}

/////////////////////////////////////////////////////////////
//check alpha value
/////////////////////////////////////////////////////////////
int on_alpha(LPCSTR szFile)
{
	CString sFile = szFile;
	AppGetShortPath(sFile);
	iImage* pImage = Image_CreateImage(szFile);
	if (!FAILED(pImage->LoadFile(szFile)))
	{
		if (pImage->IsAlphaBad())
		{
			theApp.WriteLogString(sFile);
			OutputDebugString(sFile);			
			OutputDebugString("\n");
		}
	}
	pImage->Release();
	return 0;
}

void CMapConvertDlg::OnAlpha() 
{
	CWaitCursor cursor;
	CString strFolder = "游戏资源\\美术图素";
	CString szFindFile = "*.spr";
	AppGetFullPath(strFolder);

	foreach_fileunderfolder(on_alpha,szFindFile,strFolder);

	theApp.OpenLog();
}

/////////////////////////////////////////////////////////////////////////
//validate
/////////////////////////////////////////////////////////////////////////
void CMapConvertDlg::OnValidate() 
{
	if (!VerifyFile())
		return;
	CString s = m_strFile.Left(m_strFile.GetLength()-4);
	CWaitCursor cursor;
	if (ValidateFolder(s))
	{
		AfxMessageBox("恭喜，没有无效的文件名!");
	}
}

BOOL CMapConvertDlg::ValidateIni(const CString& strFile)
{
	iItem* pItem = Common_CreateItem(strFile,iItem::FLAG_RELOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (pItem != NULL)
	{
		CString strValue;
		int i=0;
		while(1)
		{
			if (! GetItemValue(pItem,"template",strValue,i++) )
				break;
			if (strValue.IsEmpty())
				continue;
			CString sValue = strValue;
			AppGetFullPath(sValue);
			CFileFind f;
			if (!f.FindFile(sValue) || GetFileLength(sValue) == 0)
			{
				CString strMsg = "找到了无效的文件名: ";
				strMsg += strValue;
				AfxMessageBox(strMsg);
				m_strInvalid = strValue;
				m_strSrc = strValue;
//				m_strDes = strValue;
				UpdateData(FALSE);
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL CMapConvertDlg::ValidateFolder(const CString& strFile)
{
	CFileFind f;
	BOOL b;
	CString s = strFile + "\\*.ini";
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			if (f.IsDots())
				continue;
			CString strIni = f.GetFilePath();
			if (!ValidateIni(strIni))
				return FALSE;
		}
		while(b);
	}
	s = strFile + "\\*.";
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			if (f.IsDots())
				continue;
			CString strIni = f.GetFilePath();
			if (!ValidateFolder(strIni))
				return FALSE;
		}
		while(b);
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
//replace
/////////////////////////////////////////////////////////////////////////
void CMapConvertDlg::OnBrowse() 
{
	UpdateData();
	CString strFile;
	CString sWork;
	AppGetWorkingFolder(sWork);
	sWork += "\\游戏资源\\美术图素";
	if (AppGetFileDialog(strFile,sWork,"*.txt"))
	{
		AppGetShortPath(strFile);
		m_strDes = strFile;
		UpdateData(FALSE);
	}	
}

void CMapConvertDlg::OnReplace() 
{
	if (!VerifyFile())
		return;
	if (m_strDes.IsEmpty() || m_strSrc.IsEmpty())
	{
		AfxMessageBox("字符串不能为空！");
		return;		
	}
	if (m_strDes == m_strSrc)
		return;
	CString s = m_strFile.Left(m_strFile.GetLength()-4);
	CWaitCursor cursor;
	ReplaceFolder(s,m_strSrc,m_strDes);
	//可以指向上一层的目录,来替换所有的ini文件
}

void CMapConvertDlg::ReplaceIni(const CString& strFile,const CString&  strSrc,const CString& strDes)
{
	CFile f;
	if (!f.Open(strFile,CFile::modeRead ))
		return;
	int n = f.GetLength();
	if (n == 0)
		return;
	char* buffer = new char[n+1];
	f.Read(buffer,n);
	buffer[n] = 0;
	CString s = buffer;
	s.Replace(strSrc,strDes);
	f.Close();
	
	CFile file;
	if (file.Open(strFile,CFile::modeWrite |  CFile::modeCreate))
	{
		file.Write((LPCSTR)s,s.GetLength());
		file.Close();
	}
}

void CMapConvertDlg::ReplaceFolder(const CString& strFile,const CString&  strSrc,const CString& strDes)
{
	CFileFind f;
	BOOL b;
	CString s = strFile + "\\*.ini";
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			if (f.IsDots())
				continue;
			CString strIni = f.GetFilePath();
			ReplaceIni(strIni,strSrc,strDes);
		}
		while(b);
	}
	s = strFile + "\\*.";
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			if (f.IsDots())
				continue;
			CString strIni = f.GetFilePath();
			ReplaceFolder(strIni,strSrc,strDes);
		}
		while(b);
	}
}

/////////////////////////////////////////////////////////////
//检查冗余的spr文件
/////////////////////////////////////////////////////////////
#include <hash_map>
typedef std::hash_map<std::string,int,std::hash<std::string>, std::equal_to<std::string> > stringMap;
typedef stringMap::iterator itString;

stringMap g_mapUsed;
stringMap g_mapAll;
stringMap g_mapUnused;

int on_checkspr(LPCSTR szIni)
{
	CString strFile = szIni;
	if (strFile.Find(FILE_PLANEVERBELOWGROUND) == -1
		&& strFile.Find(FILE_PLANEHORBELOWVER) == -1
		&& strFile.Find(FILE_PLANEVER) == -1)
	{
		return 0;
	}
	iItem* pItem = Common_CreateItem(strFile,iItem::FLAG_RELOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (pItem != NULL)
	{
		CString strValue;
		int i=0;
		while(1)
		{
			if (! GetItemValue(pItem,"template",strValue,i++) )
				break;
			if (strValue.IsEmpty())
				continue;
			g_mapUsed[(LPCSTR)strValue]++;
		}
	}
	return 0;
}

int on_getallfiles(LPCSTR szFile)
{
	if (!IsGraphics(szFile))
		return 0;
	CString s = szFile;
	AppGetShortPath(s);
	g_mapAll[(LPCSTR)s] = 0;
	return 0;
}

void CMapConvertDlg::GetAllFiles()
{
	g_mapAll.clear();
	CString strFolder = "游戏资源";
	AppGetFullPath(strFolder);
	foreach_fileunderfolder(on_getallfiles,"*.txt",strFolder);

	CStdioFile f;
	CString sFile = "templateall.txt";
	AppGetFullPath(sFile);
	if (!f.Open(sFile,CFile::modeCreate | CFile::modeWrite))
		return;
	for (itString it = g_mapAll.begin(); it != g_mapAll.end(); ++it)
	{
		f.WriteString((*it).first.c_str());
		f.WriteString("\n");
	}
}

void CMapConvertDlg::OnCheckspr() 
{
	CString strFolder = "maps";
	AppGetFullPath(strFolder);

	int n = AfxMessageBox("是否确认maps目录下放了全部的地图文件?",MB_YESNOCANCEL);
	if (n == IDCANCEL)
		return;
	if (n == IDNO)
	{
		if (!AppGetPathDialog(strFolder))
			return;
	}

	CWaitCursor cursor;
	GetAllFiles();

	foreach_fileunderfolder(on_checkspr,"*.ini",strFolder);

	CStdioFile f;
	CString sFile = "templateused.txt";
	AppGetFullPath(sFile);
	if (!f.Open(sFile,CFile::modeCreate | CFile::modeWrite))
		return;
	
	for (itString it = g_mapUsed.begin(); it != g_mapUsed.end(); ++it)
	{
		g_mapAll[(*it).first] = (*it).second;
		f.WriteString((*it).first.c_str());
		f.WriteString("\n");
	}
	
	{
		CStdioFile f;
		CString sFile = "templateallused.txt";
		AppGetFullPath(sFile);
		if (!f.Open(sFile,CFile::modeCreate | CFile::modeWrite))
			return;
		for (itString it = g_mapAll.begin(); it != g_mapAll.end(); ++it)
		{
			int n = (*it).second;
			if (n == 0)
				continue;
			CString s;
			s.Format("%s used %d times\n",(*it).first.c_str(),n);
			f.WriteString(s);
		}
	}

	{
		CStdioFile f;
		CString sFile = "templatenoused.txt";
		AppGetFullPath(sFile);
		if (!f.Open(sFile,CFile::modeCreate | CFile::modeWrite))
			return;
		for (itString it = g_mapAll.begin(); it != g_mapAll.end(); ++it)
		{
			int n = (*it).second;
			if (n != 0)
				continue;
			f.WriteString((*it).first.c_str());
			f.WriteString("\n");
		}
	}
}
