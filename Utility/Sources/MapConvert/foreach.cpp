#include "stdafx.h"
#include "foreach.h"
#include "..\controls\FileFindEx.h"
/////////////////////////////////////////////////////////////////////////////
//for each region
/////////////////////////////////////////////////////////////////////////////
int foreach::foreach_file(LPCSTR szFolder)
{
	if (m_strFindFile.IsEmpty())
		return doonce_folder(szFolder);
	CFileFindEx f;
	BOOL b;
	CString s;
	CString strFile = szFolder;
	s = strFile + "\\" + m_strFindFile;
	f.SetOnlyDirectory(FALSE);
	f.SetOnlyFile(TRUE);
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			CString strFile = f.GetFilePath();
			doonce_file(strFile);
		}
		while(b);
	}	
	return 0;
}

int foreach::foreach_folder(LPCSTR szFolder)
{
	CFileFindEx f;
	BOOL b;
	CString s;
	CString strFile = szFolder;
	s = strFile + "\\" + m_strFindFile;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			CString strFile = f.GetFilePath();
			doonce_file(strFile);
		}
		while(b);
	}	
	return 0;
}

int foreach::foreach_folderandfile(LPCSTR szFolder, int ndepth)
{
	if (ndepth != 0 && m_strFindFolder == "*.")
	{
		foreach_file(szFolder);
	}
	ndepth ++;
	CFileFindEx f;
	BOOL b;
	CString s;
	CString strFile = szFolder;
	s = strFile + "\\" + m_strFindFolder;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);
	if (f.FindFile(s))
	{
		do
		{
			b = f.FindNextFile();
			CString szSubFolder = f.GetFilePath();
			if (is_foldervalid(szSubFolder))
				foreach_folderandfile(szSubFolder,ndepth);
		}
		while(b);
	}
	return 0;
}

int foreachregion::doonce_file(LPCSTR szFile)
{
	foreach f(m_func);
	f.SetFindFolder("???_???");
	f.SetFindFile(m_strFileInRegion);
	f.SetFindFile("???_???");
	CString strRegion = szFile;
	int pos = strRegion.ReverseFind('.');
	strRegion = strRegion.Left(pos);
	return f.foreach_folder(strRegion);
}

bool foreachregion::is_foldervalid(LPCSTR szFolder)
{
	CString strFolder = szFolder;
	CString strWor = strFolder + ".wor";
	CFileFind f;
	if (f.FindFile(strWor))
		return false;
	return true;
}

int foreach_fileunderfolder(doonce fun, LPCSTR szFindFile, LPCSTR szSrcFolder)
{
	foreach f(fun);
	f.SetFindFile(szFindFile);
	return f.foreach_folderandfile(szSrcFolder);	
}

int foreach_folderunderfolder(doonce fun, LPCSTR szSrcFolder)
{
	foreach f(fun);
	f.SetFindFile("");
	return f.foreach_folderandfile(szSrcFolder);	
}
