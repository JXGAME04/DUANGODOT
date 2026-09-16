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
//将spr的信息添加到模板上！
/////////////////////////////////////////////////////////////
int on_fillsprinfo(LPCSTR szFile)
{
	CString str = szFile;
	CString strSpr = str.Left(str.GetLength() - 3) + "spr";
	CFileFind f;
	if (!f.FindFile(strSpr))
	{
		return 0;
	}
	iItem* pItem = Common_CreateItem(str,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	iImage* pImage = Image_CreateImage(strSpr);
	wrap_ptr<iImage> ptrImage(pImage);
	pImage->LoadFile(strSpr);
	stImageInfo info;
	pImage->GetImageInfo(info);
	int n = info.nFrames;
	SetItemValue(pItem,"frames",n);
	for (int i=0; i<info.nFrames; i++)
	{
		stFrameInfo info;
		pImage->GetFrameInfo(info,i);
		int n = info.width;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRWIDTH),n,i);
		n = info.height;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRHEIGHT),n,i);
		n = info.xCenterToUpleft;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERX),n,i);	
		n = info.yCenterToUpleft;
		SetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERY),n,i);
	}
	pItem->Save();
	return 0;
}

void CMapConvertDlg::OnFillsprinfo() 
{
	CString strWork,strPath;
	AppGetWorkingFolder(strWork);
//	if (!AppGetPathDialog(strPath,strWork))
//		return ;
	strPath = strWork + "\\游戏资源";
	CWaitCursor cursor;

	foreach_fileunderfolder(on_fillsprinfo,"*.txt",strPath);
}

/////////////////////////////////////////////////////////////
//将制定目录下所有的模板文件打包！
/////////////////////////////////////////////////////////////
int g_nTemplate = 0;
CStdioFile g_fileIndex;
Pack1DEnum g_ePack1D;

int on_counttemplate(LPCSTR szFile)
{
	CString str = szFile;
	CString strSpr = str.Left(str.GetLength() - 3) + "spr";
	CFileFind f;
	if (!f.FindFile(strSpr))
	{
		return 0;
	}
	g_nTemplate++;
	return 0;
}

int g_iTemplate = 0;
int on_coverttemplate(LPCSTR szFile)
{
	CString str = szFile; 
	CString strSpr = str.Left(str.GetLength() - 3) + "spr";
	CFileFind f;
	if (!f.FindFile(strSpr))
	{
		return 0;
	}

	AppGetShortPath(str);
	g_fileIndex.WriteString(str);
	g_fileIndex.WriteString("\n");
	Common_GetPack1D(g_ePack1D)->Convert(szFile);
	g_iTemplate ++;
	return 0;
}

void CMapConvertDlg::OnZipnpc() 
{
	g_ePack1D = Pack1D_Animation;

	CString strWork,strPath;
	AppGetWorkingFolder(strWork);
//	if (!AppGetPathDialog(strPath,strWork))
//		return ;
	CWaitCursor cursor;

	g_nTemplate = 0;
	strPath = strWork + "\\游戏资源\\动态物体";
	foreach_fileunderfolder(on_counttemplate,"*.txt",strPath);

	Common_GetPack1D(Pack1D_Animation)->SetConvertNum(g_nTemplate);

	CString sFile = strWork + "\\template";
	AppBuildPath(sFile);
	sFile = strWork + FILE_TEMPLATE_INDEX_NPC;
	g_iTemplate = 0;
	if (g_fileIndex.Open(sFile,CFile::modeCreate|CFile::modeWrite))
	{
		strPath = strWork + "\\游戏资源\\动态物体";
		foreach_fileunderfolder(on_coverttemplate,"*.txt",strPath);	

		sFile = strWork + FILE_TEMPLATE_ZIP_NPC;
		Common_GetPack1D(Pack1D_Animation)->Save(sFile);
		g_fileIndex.Close();
	}	
}

void CMapConvertDlg::OnZiptemplate() 
{
	g_ePack1D = Pack1D_Plane;

	CString strWork,strPath;
	AppGetWorkingFolder(strWork);
//	if (!AppGetPathDialog(strPath,strWork))
//		return ;
	CWaitCursor cursor;

	g_nTemplate = 0;
	strPath = strWork + "\\游戏资源\\美术图素";
	foreach_fileunderfolder(on_counttemplate,"*.txt",strPath);
	strPath = strWork + "\\游戏资源\\地图动画";
	foreach_fileunderfolder(on_counttemplate,"*.txt",strPath);

	Common_GetPack1D(Pack1D_Plane)->SetConvertNum(g_nTemplate);

	CString sFile = strWork + "\\template";
	AppBuildPath(sFile);
	sFile = strWork + FILE_TEMPLATE_INDEX;
	g_iTemplate = 0;
	if (g_fileIndex.Open(sFile,CFile::modeCreate|CFile::modeWrite))
	{
		strPath = strWork + "\\游戏资源\\美术图素";
		foreach_fileunderfolder(on_coverttemplate,"*.txt",strPath);	
		strPath = strWork + "\\游戏资源\\地图动画";
		foreach_fileunderfolder(on_coverttemplate,"*.txt",strPath);	

		sFile = strWork + FILE_TEMPLATE_ZIP;
		Common_GetPack1D(Pack1D_Plane)->Save(sFile);
		g_fileIndex.Close();
	}
}

///////////////////////////////////////////////////
//pack ground file only!
///////////////////////////////////////////////////
int g_iGround = 0;
CStdioFile g_fileGround;

int on_covertground(LPCSTR szFile)
{
	CString str = szFile;
	CString strSpr = str.Left(str.GetLength() - 3) + "txt";
	CFileFind f;
	if (!f.FindFile(strSpr))
	{
		return 0;
	}

	CString strWork;
	AppGetWorkingFolder(strWork);
	CString strDes;
	strDes.Format("%s%s\\%04d.spr",strWork,FOLDER_SPR_TERRAIN,g_iGround);
	AppBuildPath(strDes);
	AppCopyFile(szFile,strDes);

	AppGetShortPath(str);
	g_fileGround.WriteString(str);
	g_fileGround.WriteString("\n");
	g_iGround ++;
	return 0;
}

void CMapConvertDlg::OnZipground() 
{
	CString strWork,strPath;
	AppGetWorkingFolder(strWork);

	CWaitCursor cursor;
	
	CString sFile = strWork + "\\template";
	AppBuildPath(sFile);
	sFile = strWork + FILE_TEMPLATE_TERRAININDEX;
	g_iGround = 0;

	if (g_fileGround.Open(sFile,CFile::modeCreate|CFile::modeWrite))
	{
		strPath = strWork + "\\游戏资源\\室外地表";
		foreach_fileunderfolder(on_covertground,"*.spr",strPath);	
		g_fileGround.Close();
	}
}
