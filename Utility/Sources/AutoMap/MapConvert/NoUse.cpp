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
//文件名转为id
/////////////////////////////////////////////////////////////
int on_filetoindex(LPCSTR szFile)
{
	//planever.ini, planehorbelowver.ini, planehorground.ini
	CString aString[] = {FILE_PLANEVER,FILE_PLANEHORBELOWVER,};
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName.CollateNoCase(aString[0]) == 0 ||
		sName.CollateNoCase(aString[1]) == 0 
		)
	{
		iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
		ASSERT(pItem);
		wrap_ptr<iItem> ptr(pItem);
		int count;
		if (!GetItemValue(pItem,"elementnum",count))
		{
			AfxMessageBox("invalid file");
			return -1;
		}
/*
		iPack1D* p = Common_GetPack1D(Pack1D_Plane);
		for (int i=0; i<count; i++)
		{
			CString strTemplate;
			GetItemValue(pItem,"template",strTemplate,i);
			int n = p->GetIndex(strTemplate);
			if (n == -1)
			{
				AfxMessageBox("invalid template");
				return -1;
			}
			SetItemValue(pItem,"templateindex",n,i);
		}
*/
		pItem->Save();
	}
	return 0;
}

int on_filetoindex_dat(LPCSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName.CompareNoCase(FILE_TERRAIN) == 0)
	{
		CFile f;
		VERIFY(f.Open(szFile,CFile::modeRead ));
		cColor m_crRegion;
		f.Read(&m_crRegion,sizeof(m_crRegion));
		const int size = (c_tWidthRegion+1) * (c_tHeightRegion+1);
		stTile aTile[size];
		f.Read(aTile,sizeof(stTile) *  size);
		stTile* pTile = aTile;
		for (int i=0; i< size; i++)
		{
			if (strlen(pTile->szResource) > 0)
			{
				CString str = pTile->szResource;
				str = str.Left(str.GetLength() - 3) + "txt";
//				pTile->idResource = Common_GetTemplate()->GetIndex(str);
				if (pTile->idResource == -1)
				{
//					AfxMessageBox("invalid template");
				}
			}
			else
			{
				pTile->idResource = -1;
			}
			pTile++;
		}
		f.Close();
		VERIFY(f.Open(szFile,CFile::modeWrite));
		f.Write(&m_crRegion,sizeof(m_crRegion));
		f.Write(aTile,sizeof(stTile) *  size);		
		f.Close();
	}
	return 0;
}

void CMapConvertDlg::OnFiletoindex() 
{
	// TODO: Add your control notification handler code here
//	FindFileUnderMap(on_filetoindex_dat,"*.dat");			
	FindFileUnderMap(on_filetoindex,"*.ini");			
}

/////////////////////////////////////////////////////////////
//将地图的绝对坐标转为相对坐标
/////////////////////////////////////////////////////////////
int on_convertcoord(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	
	//get region num!
	int num = 0;
	if (GetItemValue(pItem,"elementnum",num))
	{
		if (num == 0)
			return 0;
		for (int i=0; i<num; i++)
		{
			cPoint p;
			if (GetItemValue(pItem,"groundoffset",p,i))
			{
				p.x -= sx * c_nWidthRegion;
				p.y -= sy * c_nHeightRegion;
				SetItemValue(pItem,"regionoffset",p,i);
			}
		}
	}
	pItem->Save();
	return 0;
}

void CMapConvertDlg::OnConvercoord() 
{
	FindFileUnderMap(on_convertcoord,"*.ini");		
}

/////////////////////////////////////////////////////////////
//autoground
/////////////////////////////////////////////////////////////
int on_autoground(LPCSTR szFile)
{
	CString str = szFile;
	int pos = str.ReverseFind('\\');
	ASSERT(pos != -1);
	CString strName = str.Mid(pos + 1);
	strName.MakeLower();
	if (strName.Find("autoground") == -1)
		return 0;

	str = str.Left(pos);
	pos = str.ReverseFind('\\');
	if (pos == -1)
		return 0;
	CString strFolder = str.Mid(pos+1);
	int x,y;
	if (!GetRegionSize(strFolder,x,y))
		return 0;

	CString sNew;
	sNew.Format("%03d_%03d",x,y);
	if (sNew == strFolder)
		return 0;
	CString strNew = szFile;
	strNew.Replace(strFolder,sNew);
	AppBuildPath(strNew);
	AppMoveFolder(szFile,strNew);
	CString sMessage = szFile;
	sMessage += "move to " + strNew + ".\n";
	OutputDebugString(sMessage);
	return 0;
}

void CMapConvertDlg::OnAutoground() 
{
	CWaitCursor cursor;
	FindFileUnderMap(on_autoground,"*.dat");	
}

/////////////////////////////////////////////////////////////
//link of region
/////////////////////////////////////////////////////////////
int on_linkhor_of_region(LPCSTR szFile)
{
	CString strFile = szFile;
	CString sPath,sName;
	AppParsePathName(strFile,sPath,sName);
	if (sName.GetLength() != 7)//"???_???"
		return 0;
	CString str = strFile + "\\PlaneHorLink.ini";
	CFileFind f;
	if (f.FindFile(str))
		return 0;
	CString sMessage = "create link info " + strFile + ".\n";
	OutputDebugString(sMessage);
//	createlinkhor(szFile);
	return 0;
}

void CMapConvertDlg::OnLinkhor() 
{
	CWaitCursor cursor;
	FindEachRegionFolder(on_linkhor_of_region);
//	SetMapVerson(1);
}

///////////////////////////////////////////////////////////////////////
//image info
///////////////////////////////////////////////////////////////////////
int on_imageinfo(LPCSTR szFile)
{
	CString strFile = szFile;
//	strFile.MakeLower();
	CString aString[] = {FILE_PLANEHORBELOWVER,FILE_PLANEVER};;
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName.CollateNoCase(aString[0]) == 0 ||
		sName.CollateNoCase(aString[1]) == 0 
		)
	{
		CString sMessage = "add image info " + strFile + ".\n";
		OutputDebugString(sMessage);
		updateimageinfo(szFile);
	}
	return 0;
}

void CMapConvertDlg::OnImage() 
{
	CWaitCursor cursor;
	FindFileUnderMap(on_imageinfo,"*.ini");
}

////////////////////////////////////////////////////////////////
//create a bmp from region
////////////////////////////////////////////////////////////////
void CMapConvertDlg::OnBmp() 
{
	// TODO: Add your control notification handler code here
	CWaitCursor cursor;
	
	CRect rc;
	GetRect(rc);
	//create a bitmap;
	//set each point of it!
	CString sImage = m_strFile.Left(m_strFile.GetLength()-4) + ".bmp";
	iImage* pImage = Image_CreateImage(sImage);
	stImageInfo info;
	info.width = (rc.Width()+1) * m_nEnlarge;
	info.height = (rc.Height()+1)  * m_nEnlarge;
	info.bytes_per_pixel = 3;
	int size = info.width * info.height * info.bytes_per_pixel;
	unsigned char* buffer;
	buffer = new unsigned char[size];
	info.buffer = new unsigned char[size];
	ZeroMemory(buffer,size);
	
	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4) + "\\???_???";
	CFileFindEx f;
	f.SetOnlyDirectory(TRUE);
	f.SetOnlyFile(FALSE);

	if (f.FindFile(sFolder))
	{
		BOOL bFind;
		do
		{
			bFind = f.FindNextFile();
			CString sName = f.GetFileName();
			CString sPath = f.GetFilePath();
			
			if (!IsAnyInDir(sPath))
				continue;

			int sx,sy;
			if (!GetRegionSize(sName,sx,sy))
				continue;
			int offset = ( ((sy - rc.top) * info.width + sx - rc.left) * 3 ) * m_nEnlarge;
			for (int x = 0; x < m_nEnlarge; x++)
			for (int y = 0; y < m_nEnlarge; y++)
			{
				int off = offset + ( y * info.width + x ) * 3;
				buffer[off] = 0xff;
				buffer[off+1] = 0xff;
				buffer[off+2] = 0xff;
			}
		}while(bFind);
	}

	unsigned char* des = info.buffer;
	unsigned char* src = buffer + (info.height-1) * info.width * 3;
	for (int y = 0; y < info.height; y ++)
	{
		memcpy(des,src,info.width * 3);
		des += info.width*3;
		src -= info.width*3;
	}

	pImage->CreateFile(sImage,info);
	pImage->Release();
	delete info.buffer;
	delete buffer;
}

////////////////////////////////////////////////////////////////
//get ini
////////////////////////////////////////////////////////////////
void CMapConvertDlg::OnIni() 
{
	//wor to a ini!
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	CString strIni = m_strFile;
	iItem* pItem = Common_CreateItem(strIni,iItem::FLAG_LOAD);
	ASSERT(pItem);
	CRect rc;
	GetRect(rc);
	SetItemValue(pItem,"rect",rc);
	pItem->Save();
	pItem->Release();
}
