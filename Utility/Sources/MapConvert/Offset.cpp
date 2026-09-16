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

/////////////////////////////////////////////////////////
//平移整个地图
/////////////////////////////////////////////////////////
cPoint CMapConvertDlg::GetOffsetGround()
{
	return cPoint(m_nRight*16,m_nBottom*32);
}

void CMapConvertDlg::OffsetFile(LPCSTR szFile)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	int count;
	if (!GetItemValue(pItem,"elementnum",count))
		return;
	cPoint ptOffsetGround = GetOffsetGround();
	for (int i=0; i<count; i++)
	{
		cPoint ptOffset;
		if (GetItemValue(pItem,"groundoffset",ptOffset,i))
		{
			ptOffset += ptOffsetGround;
			SetItemValue(pItem,"groundoffset",ptOffset,i);
		}
	}
	pItem->Save();
}

void CMapConvertDlg::OffsetFolder(LPCSTR szFolder)
{
	CFileFind f;
	CString strFind = szFolder;
	strFind += "\\*.ini";
	if (!f.FindFile(strFind))
		return ;
	BOOL b;
	do 
	{
		b = f.FindNextFile();
		OffsetFile(f.GetFilePath());
	}while(b);
}

BOOL CMapConvertDlg::OffsetCamera()
{
	CFileFind find;
	CString sCamera = m_strFile.Left(m_strFile.GetLength()-4) + "\\camera.txt";
	if (!find.FindFile(sCamera))
	{
		AfxMessageBox("not found the camera.txt file");
		return FALSE;
	}
	iItem* pItem = Common_CreateItem(sCamera,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	cPoint ptOffset;
	if (!GetItemValue(pItem,"to",ptOffset))
	{
		AfxMessageBox("camera.txt not valid");
		return FALSE;
	}
	ptOffset += GetOffsetGround();
	SetItemValue(pItem,"to",ptOffset);
	pItem->Save();
	return TRUE;
}

void CMapConvertDlg::DeleteUnvalidRegionFolder()
{
	CFileFindEx f;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);

	CWaitCursor cursor;

	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4) + "\\*_*";
	if (!f.FindFile(sFolder))
		return;
	BOOL b;
	do
	{
		b = f.FindNextFile();
		CString sName = f.GetFileName();
		if (sName.GetLength() == 7)
			continue;
		int sx,sy;
		if (!GetRegionSize(sName,sx,sy))
			continue;
		CString sNew;
		sNew.Format("\\%03d_%03d",sx, sy);		
		CString sFolder;
		AppParsePath(f.GetFilePath(),sFolder);
		sFolder += sNew;
		BOOL b = AppRenameFolder(f.GetFilePath(),sFolder);
		if (!b)
		{
			CString sCopy = f.GetFilePath() + "\\*.*";
			AppCopyFolder(sCopy,sFolder);
			AppDeleteFolder(f.GetFilePath());
		}
	}while(b);
}

void CMapConvertDlg::OnOffset() 
{
	if (!VerifyFile())
		return ;
	//移动camera!
	if (!OffsetCamera())
		return ;

	if (m_nRight == 0 && m_nBottom == 0)
	{
		DeleteUnvalidRegionFolder();
		return;
	}

	CWaitCursor cursor;

	//还有动态物体需要移动
	OffsetFolder(m_strFile.Left(m_strFile.GetLength()-4));	//本目录

	CFileFindEx f;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);

	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4) + "\\???_???";
	if (!f.FindFile(sFolder))
		return;
	BOOL b;
	do
	{
		b = f.FindNextFile();
		CString sName = f.GetFileName();
		if (sName.GetLength() != 7)
			continue;
		OutputDebugString("offset the folder ");
		OutputDebugString(sName);
		OutputDebugString("\n");
		int sx,sy;
		if (!GetRegionSize(sName,sx,sy))
			continue;
		if (sx + m_nRight >= 0 && sy + m_nBottom >= 0) //可以移动
		{
			if (m_nRight != 0 || m_nBottom != 0)
				OffsetFolder(f.GetFilePath());
			CString sNew;
			sNew.Format("\\%03d_%03d_temp",sx+m_nRight, sy+m_nBottom);
			CString sFolder;
			AppParsePath(f.GetFilePath(),sFolder);
			sFolder += sNew;

			if (sFolder != f.GetFilePath())
			{
				BOOL b = AppRenameFolder(f.GetFilePath(),sFolder);
				if (! b)
				{
					CString sMsg;
					sMsg = "error when rename file " + f.GetFilePath() + " to " + sFolder;
					AfxMessageBox(sMsg);
					return;
				}
			}
		}
		else
		{
			AfxMessageBox("some folder can not move, and there will lost data!");
			return;
		}
	}while(b);

	if (m_nRight != 0 || m_nBottom != 0)
	{
		sFolder = m_strFile.Left(m_strFile.GetLength()-4) + "\\???_???_temp";
		if (!f.FindFile(sFolder))
			return;
		BOOL b;
		do
		{
			b = f.FindNextFile();
			CString sName = f.GetFileName();
			int sx,sy;
			if (!GetRegionSize2(sName,sx,sy))
				continue;

			CString sNew;
			sNew.Format("\\%03d_%03d",sx, sy);
			CString sFolder;
			AppParsePath(f.GetFilePath(),sFolder);
			sFolder += sNew;
			BOOL b = AppRenameFolder(f.GetFilePath(),sFolder);
			if (! b)
			{
				CString sMsg;
				sMsg = "error when rename file " + f.GetFilePath() + " to " + sFolder;
				AfxMessageBox(sMsg);
				return;
			}
		}while(b);
	}
}

/////////////////////////////////////////////////////////
//将地图分开
/////////////////////////////////////////////////////////
CString g_strDesFolder;
CRect g_rcDiv;

int on_div(LPCSTR szFolder)
{
	CString sPath,sName;
	AppParsePathName(szFolder,sPath,sName);

	int sx,sy;
	bool bCopy = FALSE;
	if (GetRegionSize(sName,sx,sy))
	{
		if (sx >= g_rcDiv.left && sx <= g_rcDiv.right && sy >= g_rcDiv.top && sy <= g_rcDiv.bottom)
		{
			bCopy = TRUE;
		}
	}
	else
	{
		bCopy = TRUE;
	}

	if (bCopy)
	{
		CString strDes = g_strDesFolder + "\\" + sName;
		AppCopyFolder(szFolder,strDes);
	}

	return 0;
}

void CMapConvertDlg::OnDiv() 
{
	if (!VerifyFile())
		return;
	CRect rc(m_nDivLeft,m_nDivTop,m_nDivRight,m_nDivBottom);
	if (rc.Width() < 0 || rc.Height() < 0)
	{
		AfxMessageBox("input rect wrong!");
		return ;
	}
	g_rcDiv = rc;

	if (m_strNewMap.IsEmpty())
	{
		AfxMessageBox("input new map wrong!");
		return ;
	}
	else
	{
		if (m_strNewMap.Find(".") != -1)
		{
			AfxMessageBox("input '.' is invalid!");
			return ;
		}
	}
	
	CString strPath,strName,strExt;
	AppParsePathNameExt(m_strFile,strPath,strName,strExt);
	CString strDesFile = strPath + "\\" + m_strNewMap + ".wor";
	AppCopyFile(m_strFile,strDesFile);

	g_strDesFolder = strPath + "\\" + m_strNewMap;
	AppBuildPath(g_strDesFolder);
	CWaitCursor cursor;
	FindFolderUnderMap(on_div);
}
