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


/////////////////////////////////////////////////////////////////////////
//clear
/////////////////////////////////////////////////////////////////////////
int on_clear(LPCSTR szFile)
{
	OutputDebugString(szFile);
	OutputDebugString("  delete!\n");
	AppDeleteFolder(szFile);	
	return 0;
}

int on_clear_invalidregionfolder(LPCSTR szFile)
{
	CString s = szFile;
	int pos = s.ReverseFind('\\');
	if (pos == -1)
		return 0;
	s = s.Mid(pos + 1);
	pos = s.Find('_');
	if (pos == -1)
		return 0;
	CString sLeft = s.Left(pos);
	CString sRight = s.Mid(pos + 1);
	int x,y;
	x = y = 0;
	x = atoi(sLeft);
	y = atoi(sRight);
	if (x>0 && y>0)
	{
		if (s.GetLength() != 7)
		{
			AppDeleteFolder(szFile);
		}
	}
	return 0;
}

void CMapConvertDlg::OnClear() 
{
	CWaitCursor cursor;
	FindFileUnderMap(on_clear,"*.bin");
	FindFileUnderMap(on_clear,"Obstacle.dat");
	FindFileUnderMap(on_clear,"Obstacle.txt");
	FindFolderUnderMap(on_clear_invalidregionfolder);	
	OnClearnull();
	OnIni();
}
//*/
/////////////////////////////////////////////////////////////////////////
//clear null
/////////////////////////////////////////////////////////////////////////
int on_clear_null(LPCSTR szFile)
{
	if (GetFileLength(szFile) == 0)
	{
		AppDeleteFolder(szFile);
		OutputDebugString(szFile);
		OutputDebugString("  delete!\n");
	}
	return 0;
}

bool is_folderunderfolder(LPCSTR szFolder)
{
	CFileFindEx f;
	f.SetOnlyFile(FALSE);
	f.SetOnlyDirectory(TRUE);
	CString sFind = szFolder;
	sFind += "\\*.";
	return f.FindFile(sFind);
}

int on_clear_nullfolder(LPCSTR szFolder)
{
	if (is_folderunderfolder(szFolder))
		return 0;
	CFileFindEx f;
	f.SetOnlyFile(TRUE);
	f.SetOnlyDirectory(FALSE);
	BOOL bnull = true;
	CString sFind = szFolder;
//	sFind += "\\*.*";
	sFind += "\\*.ini";
	if (f.FindFile(sFind))
	{
		BOOL bFind;
		do 
		{
			bFind = f.FindNextFile();
			CString strName = f.GetFilePath();
			if (GetFileLength(f.GetFilePath()) > 0)
			{
				bnull = false;
				break;
			}
		}while(bFind);
	}
	if (bnull)
	{
		sFind = szFolder;
		sFind += "\\Region.dat";
		if (f.FindFile(sFind))
			bnull = false;
	}

	if (bnull)
	{
		AppDeleteFolder(szFolder);
		OutputDebugString(szFolder);
		OutputDebugString("  delete!\n");
	}
	return 0;
}

void CMapConvertDlg::OnClearnull() 
{
	CWaitCursor cursor;
	FindFileUnderMap(on_clear_null,"*.ini");
	FindFolderUnderMap(on_clear_nullfolder);	
}
