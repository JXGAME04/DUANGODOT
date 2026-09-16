// cStatus.cpp: implementation of the cStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "status.h"
#include "..\controls\FileFindEx.h"

#include "cStatus.h"
#include "iImage.h"

#include "iGraphics.h"
#include "iCommand.h"
#include "fileop.h"
#include "iItem.h"
#include "objUtility.h"
#include "iGraphicsUtility.h"

#include "StringDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
eStatusType			cStatus::m_eStatus = STATUS_NULL;
iGraphics*			cStatus::m_pGraphics = NULL;
CWnd*				cStatus::m_pWnd = NULL;
OutputMsgFunc		cStatus::AppOutputMsg = NULL;
iCommandMachine*	cStatus::m_pCommandMachine = NULL;
CString				cStatus::m_strDocument;
CString				cStatus::m_strSelectFile;
int					cStatus::m_nFrame = 0;
CString				cStatus::m_strNewFile;
stSelect			cStatus::m_stSelect;

cStatus::cStatus()
{
	m_ulCount = 1;
	m_bToggleFullscreen = FALSE;
	m_bPerspecitve = TRUE;
}

cStatus::~cStatus()
{
}

void cStatus::Clear()
{
	m_strSelectFile.Empty();
}

/**************************************************************************/
//com interface
/**************************************************************************/
ULONG cStatus::AddRef() 
{
	return ++m_ulCount;
}

ULONG cStatus::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

//////////////////////////////////////////////////////////////////////
//Create
//////////////////////////////////////////////////////////////////////

BOOL cStatus::Create()
{
	return TRUE;
}

BOOL cStatus::Create(iGraphics* p, CWnd* pWnd, iCommandMachine* pCommandMachine)
{
	ASSERT(m_pGraphics == NULL);
	p->AddRef();
	m_pGraphics = p;
	m_pWnd = pWnd;
	ASSERT(m_pCommandMachine == NULL);
	pCommandMachine->AddRef();
	m_pCommandMachine = pCommandMachine;

	m_stSelect.eType = SelectType_Default;
	m_stSelect.eFilter = SelectFilter_OneByDisplay;
	m_stSelect.eFilterParam = SelectFilterParam_Default;
	m_stSelect.eKind = KIND_NORMAL;
	m_stSelect.eSort = SORT_VER;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//keyboard and mouse

BOOL cStatus::GetMousePos(CPoint& pt)
{
	if (m_pWnd == NULL)
		return FALSE;
	GetCursorPos(&pt);
	CRect rc;
	m_pWnd->GetWindowRect(&rc);
	if (rc.PtInRect(pt))
	{
		pt.x -= rc.left;
		pt.y -= rc.top;
		return TRUE;
	}
	return FALSE;
}

bool cStatus::IsKeyDown(UINT vk)
{
	short state = GetKeyState(vk);
	return (HIBYTE(state)!=0 );
}

bool cStatus::IsShiftDown()
{
	return IsKeyDown(VK_LSHIFT) || IsKeyDown(VK_RSHIFT);
}

bool cStatus::IsCtrlDown()
{
	return IsKeyDown(VK_LCONTROL) || IsKeyDown(VK_RCONTROL);
}

bool cStatus::IsAltDown()
{
	return IsKeyDown(VK_LMENU) || IsKeyDown(VK_RMENU);
}


//////////////////////////////////////////////////////////////////////
//Graphics
BOOL cStatus::SetShow(long l)
{
	iGraphics::stParam param;
	param.mask = 0;
	GetGraphics()->GetParam(param);
	BOOL b = param.lShowEnum & l;

	param.mask = iGraphics::MASK_SHOW;
	param.lShowEnum = l;
	param.eOPShow = Bit_Add;
	GetGraphics()->SetParam(param);	
	return b;
}

void cStatus::ClearShow(long l)
{
	iGraphics::stParam param;
	param.mask = iGraphics::MASK_SHOW;
	param.lShowEnum = l;
	param.eOPShow = Bit_Remove;
	GetGraphics()->SetParam(param);	
}

iGraphics* cStatus::GetGraphics()
{
	return m_pGraphics; 
}

iGround* cStatus::GetGround()
{
	return GetGraphics()->GetiGround();
}

iScreen* cStatus::GetScreen()
{
	return GetGraphics()->GetiScreen();
}

cPoint cStatus::GetCameraFocus()
{
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	return param.ptTo;
}

iCamera* cStatus::GetCamera()
{
	return GetGraphics()->GetiCamera();
}

iObj* cStatus::GetSelect()
{
	return GetGround()->GetiSelect();
}

iObj* cStatus::GetMap()
{
	return GetScreen()->GetiMap();
}

iObj* cStatus::GetMapSmall()
{
	return GetScreen()->GetiMapSmall();
}

BOOL cStatus::IsGraphicsShow(long mask)
{
	iGraphics::stParam param;
	param.mask = 0;
	GetGraphics()->GetParam(param);
	return param.lShowEnum & mask;
}

BOOL cStatus::SetUpdateRegionTexture(BOOL b)
{
	iGround::stParam param;
	param.mask = iGround::MASK_UPDATEREGIONTEXTURE;
	param.bUpdateRegionTexture = b;
	GetGround()->SetParam(param);
	return TRUE;
}

BOOL cStatus::UpdateRegionTexture()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_UPDATEREGIONTEXTURE;
	GetGround()->SetParam(param);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//input

BOOL cStatus::OnLButtonDown(UINT nFlags, CPoint point)
{
	return FALSE;
}

BOOL cStatus::OnLButtonUp(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatus::OnLButtonDblClk(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatus::OnRButtonDown(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatus::OnRButtonUp(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatus::OnRButtonDblClk(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatus::OnTimer(UINT nIDEvent)
{
	if (m_bToggleFullscreen)
	{
		ToggleFullscreen();
		m_bToggleFullscreen = FALSE;
	}
	CPoint point;
	GetMousePos(point);
	if (m_pGraphics != NULL)
	{
		iGraphics::stParam param;
		param.ptMouse = cPoint(point.x,point.y);
		param.mask = iGraphics::MASK_MOUSE;
		m_pGraphics->SetParam(param,TRUE);
	}
	return FALSE;
}
BOOL cStatus::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message){return FALSE;}

void cStatus::ToggleDraw(long l)
{
	iGraphics::stParam param;
	param.mask = iGraphics::MASK_SHOW;
	param.lShowEnum = l;
	param.eOPShow = Bit_Toggle;
	GetGraphics()->SetParam(param);
}

BOOL LoadBitmapFromBMPFile( LPCSTR szFileName, CBitmap& bmp)
{   
//	BITMAP  bm;   
	HBITMAP phBitmap;
	//this is a 24 byte
	phBitmap = (HBITMAP)LoadImage( NULL, szFileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );
	if( phBitmap == NULL )
		return FALSE;   // Get the color depth of the DIBSection
	bmp.Attach(phBitmap);
	return TRUE;   
} 
/*
BOOL LoadBitmapFromBMPFile( LPCSTR szFileName, CBitmap& bmp)
{   
	BITMAP  bm;   
	HBITMAP phBitmap;
	//this is a 24 byte
	phBitmap = (HBITMAP)LoadImage( NULL, szFileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );
	if( phBitmap == NULL )
		return FALSE;   // Get the color depth of the DIBSection
	//covert 24 to 16!
	GetObject(phBitmap, sizeof(BITMAP), &bm );
	static WORD buffer[512*512];
	BYTE* pByte = (BYTE* )bm.bmBits;
	WORD* pWord = buffer;
	for (int j=0; j<bm.bmHeight*bm.bmWidth; j++)
	{
		BYTE r = *pByte++;
		BYTE g = *pByte++;
		BYTE b = *pByte++;
//		(*pWord++) = ((WORD)(r >> 3)) << 10 | ((WORD)(g >> 3)) << 5 | (b >> 3); 
		(*pWord++) = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
	}
	static WORD pExbuffer[512*512];
	unsigned int line = 512;
	for (int i=0; i<512; i++)
	{
		memcpy(pExbuffer+i*line,buffer+(512-1-i)*line, line*2);
	}

	bmp.SetBitmapBits(bm.bmHeight*bm.bmWidth*2,pExbuffer);
	DeleteObject(phBitmap);
	return TRUE;   
} 
*/

#pragma pack(1)

struct stTgaHeader
{
	byte m_id_length;
	byte m_color_map_type;
	byte m_type;
	short m_cm_index;
	short m_cm_length;
	byte m_cm_entry_size;
	short m_x_org;
	short m_y_org;
	short dwWidth;
	short dwHeight;
	byte bits_per_pixel;
	byte m_desc;
};

#define UL_TGA_BT 0x20

HRESULT SaveTGAFile(const char *filename, CBitmap& bmp, int bytecount)
{
	BITMAP bm;
	bmp.GetBitmap(&bm);

	stTgaHeader m_stHeader;
	memset(&m_stHeader,0,sizeof(m_stHeader));

	m_stHeader.m_type = 2;
	m_stHeader.bits_per_pixel = bytecount;
	m_stHeader.dwHeight = (short)bm.bmHeight;
	m_stHeader.dwWidth = (short)bm.bmWidth;
	m_stHeader.m_desc = UL_TGA_BT;

	CFile file;
	if(!file.Open(filename,CFile::modeCreate|CFile::modeWrite))
		return -1;
	
	file.Write(&m_stHeader,sizeof(stTgaHeader));
	file.Write(bm.bmBits,bm.bmHeight*bm.bmWidth*bytecount/8);
	
	file.Close();
	return 0;
}

HRESULT Save32BMPFileTo16(const char *filename,  CBitmap& bmp)
{
	BITMAP bm;
	bmp.GetBitmap(&bm);
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	bi.biSize  = sizeof(BITMAPINFOHEADER);
	bi.biWidth  = bm.bmWidth;
	bi.biHeight   = bm.bmHeight;
	bi.biPlanes   = 1;
	bi.biBitCount  = 16;
//	bi.biCompression = BI_BITFIELDS;
	bi.biCompression = 0;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed  = 0;
	bi.biClrImportant = 0;

	int size16 = bi.biWidth*bm.bmHeight*(16/8);
	int size32 = bi.biWidth*bm.bmHeight*(32/8);

	char* pExbuffer = new char[size32];
	bmp.GetBitmapBits(size32,pExbuffer);
	char* buffer = new char[size16];

	WORD* pDes16 = (WORD*)(buffer + (bi.biWidth*(bm.bmHeight-1)*(16/8)));
	char* pSrc32 = pExbuffer;
	for (int i=0; i<bm.bmHeight; i++)
	{
		WORD* pW = pDes16;
		for (int j=0; j<bm.bmWidth; j++)
		{
			char b = (*pSrc32++);
			char g = (*pSrc32++);
			char r = (*pSrc32++);
			char a = (*pSrc32++);
			*pW ++ = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
		}
		pDes16 -= bi.biWidth;
	}
	
	bfh.bfType=0x4d42;
	bfh.bfOffBits=sizeof(bfh)+sizeof(BITMAPINFOHEADER);
	bfh.bfSize=bfh.bfOffBits+size16;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;

	CFile file;
	if(!file.Open(filename,CFile::modeCreate|CFile::modeWrite))
		return -1;
	file.Write(&bfh,sizeof(bfh));
	file.Write(&bi,sizeof(bi));
	file.Write(buffer,size16);
	file.Close();

	delete buffer;
	delete pExbuffer;
	return S_OK;
}

HRESULT Save16BMPFileTo24(const char *filename,  CBitmap& bmp)
{
	BITMAP bm;
	bmp.GetBitmap(&bm);
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	bi.biSize  = sizeof(BITMAPINFOHEADER);
	bi.biWidth  = bm.bmWidth;
	bi.biHeight   = bm.bmHeight;
	bi.biPlanes   = 1;
	bi.biBitCount  = 24;
//	bi.biCompression = BI_BITFIELDS;
	bi.biCompression = 0;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed  = 0;
	bi.biClrImportant = 0;

	int size16 = bi.biWidth*bm.bmHeight*(16/8);
	int size24 = bi.biWidth*bm.bmHeight*(24/8);

	char* pExbuffer = new char[size16];
	bmp.GetBitmapBits(size16,pExbuffer);
	char* buffer = new char[size24];

	WORD* pSrc16 = (WORD*)(pExbuffer + (bi.biWidth*(bm.bmHeight-1)*(16/8)));
	char* pDes24 = buffer;
	for (int i=0; i<bm.bmHeight; i++)
	{
		for (int j=0; j<bm.bmWidth; j++)
		{
			WORD w = (*pSrc16++);
			*pDes24++ = (((WORD)(w << 11)) >> 11) << 3;
			*pDes24++ = (((WORD)(w << 5)) >> 10) << 2;
			*pDes24++ = (w >> 11) << 3;
		}
		pSrc16 -= bi.biWidth * 2;
	}
	
	bfh.bfType=0x4d42;
	bfh.bfOffBits=sizeof(bfh)+sizeof(BITMAPINFOHEADER);
	bfh.bfSize=bfh.bfOffBits+size24;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;

	CFile file;
	if(!file.Open(filename,CFile::modeCreate|CFile::modeWrite))
		return -1;
	file.Write(&bfh,sizeof(bfh));
	file.Write(&bi,sizeof(bi));
	file.Write(buffer,size24);
	file.Close();

	delete buffer;
	delete pExbuffer;
	return S_OK;
}


HRESULT SaveBMPFile(const char *filename,  CBitmap& bmp, int bytecount)
{
	BITMAP bm;
	bmp.GetBitmap(&bm);
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	bi.biSize  = sizeof(BITMAPINFOHEADER);
	bi.biWidth  = bm.bmWidth;
	bi.biHeight   = bm.bmHeight;
	bi.biPlanes   = 1;
	bi.biBitCount  = bytecount;
//	bi.biCompression = BI_BITFIELDS;
	bi.biCompression = 0;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed  = 0;
	bi.biClrImportant = 0;

	int size = bi.biWidth*bm.bmHeight*(bi.biBitCount/8);

	char* pExbuffer = new char[size];
	bmp.GetBitmapBits(size,pExbuffer);
	char* buffer = new char[size];

	unsigned int line = bi.biWidth*(bi.biBitCount/8);
	for (int i=0; i<bm.bmHeight; i++)
	{
		memcpy(buffer+i*line,pExbuffer+(bm.bmHeight-1-i)*line, line);
	}
	
	bfh.bfType=0x4d42;
	bfh.bfOffBits=sizeof(bfh)+sizeof(BITMAPINFOHEADER);
	bfh.bfSize=bfh.bfOffBits+size;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;

	CFile file;
	if(!file.Open(filename,CFile::modeCreate|CFile::modeWrite))
		return -1;
	file.Write(&bfh,sizeof(bfh));
	file.Write(&bi,sizeof(bi));
	file.Write(	buffer,size);
	file.Close();

	delete buffer;
	delete pExbuffer;
	return S_OK;
}

BOOL GetRegionSize(const CString& sName, int& sx, int& sy)
{
	int pos = sName.Find("_");
	if (pos == -1)
		return FALSE;
	CString sX = sName.Left(pos);
	CString sY = sName.Mid(pos + 1);
	sx = atoi((LPCSTR)sX);
	sy = atoi((LPCSTR)sY);
	return TRUE;
}

void cStatus::GetRect(CRect& rect) 
{
	CRect rc(LONG_MAX,LONG_MAX,0,0);

	CString m_strFile = m_strDocument;
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
			if (sName.GetLength() != 7)
				continue;
			
			int sx,sy;
			GetRegionSize(sName,sx,sy);
			rc.left = min(sx,rc.left);
			rc.top = min(sy,rc.top);
			rc.right = max(sx,rc.right);
			rc.bottom = max(sy,rc.bottom);
		}while(bFind);
	}
	rect = rc;
}

int c_nMapSize = 32;

void cStatus::CameraPerspective(BOOL b)
{
	iCamera::stParam param;
	param.mask = iCamera::MASK_FOV | iCamera::MASK_DIS | iCamera::MASK_COMMAND;;
	param.eCommand = iCamera::CCOM_UPDATE;
	if (!b)
	{
		param.fDis = 2700.f;
		param.fFOV = D3DX_PI/32;
	}
	else
	{
		param.fDis = 320.f;
		param.fFOV = D3DX_PI/4;
	}
	GetCamera()->SetParam(param);
}

void cStatus::GetWholeRegionMap()
{
	if (m_strDocument.IsEmpty())
		return;
	
	int g_nMapSize = c_nMapSize;
	if (IsShiftDown())
	{
		CString str;
		if (GetAString(str,""))
		{
			int n = atoi(str);
			if ( n > 0 )
				g_nMapSize = n;
		}
	}

	iItem* pItem = Common_CreateItem(m_strDocument,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return;
	wrap_ptr<iItem> ptr(pItem);
	CRect rect;
	GetRect(rect);
	if (rect.IsRectEmpty())
		return;
	SetItemValue(pItem,"rect",rect);
	pItem->Save();

	//size if (rect.Width()+1) * 32, (rect.Height()+1) * 32;
	CSize sz((rect.Width()+1) * g_nMapSize,(rect.Height()+1) * g_nMapSize);

	CDC* pDC = m_pWnd->GetDC();
	int bits = pDC->GetDeviceCaps(BITSPIXEL);
	CDC dcMem1,dcMem2;
	dcMem1.CreateCompatibleDC(pDC);
	dcMem2.CreateCompatibleDC(pDC);
	CBitmap bmpMap;
	bmpMap.CreateBitmap(sz.cx,sz.cy,1,bits,NULL);
//	bmp.CreateBitmap(512,512,1,c_pixByte,NULL);	
	dcMem1.SelectObject(&bmpMap);

	CameraScale(100.f);
	CameraScale(0.2f);
	
	CameraPerspective(FALSE);

	ClearShow(iGraphics::GS_OBSTACLE);
	ClearShow(iGraphics::GS_MAP);
	ClearShow(iGraphics::GS_SPECIAL);
	ClearShow(iGraphics::GS_SMALLMAP);

	GetGraphics()->GraphicsRun();
	CWaitCursor cursor;

	iGround::stParam param;
	param.mask = iGround::MASK_RENDERMAP;
	param.bRenderMap = TRUE;
	GetGround()->SetParam( param);

	//get start point and endpoint
	cRect rc(144,44,144+512,44+512);
	CString strFolder = m_strDocument.Left(m_strDocument.GetLength() - 4);

	CBitmap bmp;
	bmp.CreateBitmap(512,512,1,bits,NULL);
	char* bmpBuffer= new char [512*512*bits/8];

	CString sTrace;
	for (int i=rect.left; i<=rect.right; i++)
	{
		for (int j=rect.top; j<=rect.bottom; j++)
		{
			CString sFile;
			sFile.Format("%s\\%03d_%03d",strFolder,i,j);
			CFileFind f;
			if (f.FindFile(sFile))
			{
				sTrace.Format("rect (%d,%d,%d,%d) now (%d,%d) percent %d/100 ",
					rect.left,rect.top,rect.right,rect.bottom,
					i,j,(i - rect.left)*100 / rect.Width()
					);
				GetGraphics()->Trace(sTrace);

				//camera move to region(i,j) center
				OffsetCameraTo(cPoint(i*16+8,j*32+16));
				GetGraphics()->GraphicsRun();
				//save bmp
				GetGraphics()->SaveBmpBuffer((DWORD*)bmpBuffer,rc);
				bmp.SetBitmapBits(512*512*bits/8,bmpBuffer);

				dcMem2.SelectObject(&bmp);
				dcMem1.SetStretchBltMode(HALFTONE);
				dcMem1.StretchBlt((i-rect.left)*g_nMapSize,(j-rect.top)*g_nMapSize,g_nMapSize,g_nMapSize,&dcMem2,0,0,512,512,SRCCOPY);
			}
		}
	}
	//合并所有的bmp

	param.bRenderMap = FALSE;
	GetGround()->SetParam( param);

	if (bits == 16)
	{
		CString s = strFolder + ".bmp";
		SaveBMPFile(s,bmpMap,bits);
		s = strFolder + "24.bmp";
		Save16BMPFileTo24(s,bmpMap);
	}
	else if (bits == 32)
	{
		CString s = strFolder + "32.bmp";
		SaveBMPFile(s,bmpMap,bits);
		s = strFolder + ".bmp";
		Save32BMPFileTo16(s,bmpMap);
	}

	delete bmpBuffer;

	CameraPerspective(TRUE);
}



BOOL cStatus::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_HOME)
	{
		if (IsCtrlDown())
		{
			GetWholeRegionMap();
		}
	}

	BOOL bHandle = FALSE;
	if (IsCtrlDown())
	{
		bHandle = TRUE;
		int off = 1;
		if (IsShiftDown())
			off = -1;
		switch (nChar)
		{
		case VK_F2:
			if (IsShiftDown())
				ToggleDraw(iGraphics::GS_SMALLMAP);
			else
				ToggleDraw(iGraphics::GS_MAP);
			break;
		case VK_F3:
			ToggleDraw(iGraphics::GS_TERRAIN);
			break;
		case VK_F4:
			ToggleDraw(iGraphics::GS_SPECIAL);
			break;
		case VK_F5:
			ToggleDraw(iGraphics::GS_LIGHT);
			break;			
		case VK_F6:
			if (IsShiftDown())
				ToggleDraw(iGraphics::GS_VER);
			else
				ToggleDraw(iGraphics::GS_HOR);
			break;			
		case VK_F7:
			ToggleDraw(iGraphics::GS_OBSTACLE);
			break;
		case 'Q':
			AddWindSpeed(cPoint(0.01f*off,0,0));
			break;
		case 'W':
			AddWindSpeed(cPoint(0,0.01f*off,0));
			break;
		case 'E':
			AddWindSpeed(cPoint(0,0,0.01f*off));
			break;
		case 'F':
			m_bToggleFullscreen = TRUE;
			break;
		case 'A':
			m_bPerspecitve = !m_bPerspecitve;
			CameraPerspective(m_bPerspecitve);
			break;
		case 'T':
			ToggleGrid();
			break;
		case 0xbb: //+
			CameraScale(0.1f);
			break;
		case 0xbd: //-
			CameraScale(-0.1f);
			break;
		case VK_PRIOR:
			CameraScale(-100.f);
			break;
		case VK_NEXT:
			CameraScale(100.f);
			break;
		default:
			bHandle = FALSE;
		}
	}

	if (bHandle)
		return TRUE;

	bHandle = TRUE;

	int off = 4;
	if (IsShiftDown())
		off = 16;
	if (IsCtrlDown() | IsShiftDown())
	{
		if (IsKeyDown(VK_UP) && IsKeyDown(VK_LEFT))
		{
			CameraOffset(-off,-off*2);
		}
		else if  (IsKeyDown(VK_UP) && IsKeyDown(VK_RIGHT))
		{
			CameraOffset(off,-off*2);
		}
		if (IsKeyDown(VK_DOWN) && IsKeyDown(VK_LEFT))
		{
			CameraOffset(-off,off*2);
		}
		else if  (IsKeyDown(VK_DOWN) && IsKeyDown(VK_RIGHT))
		{
			CameraOffset(off,off*2);
		}
		else
		{
			switch (nChar)
			{
			case VK_UP:
				CameraOffset(0,-off*2);
				break;
			case VK_DOWN:
				CameraOffset(0,off*2);
				break;
			case VK_LEFT:
				CameraOffset(-off,0);
				break;
			case VK_RIGHT:
				CameraOffset(off,0);
				break;		
			default:
				bHandle = FALSE;
			}
		}
	}
	else
	{
		if (IsKeyDown(VK_UP) && IsKeyDown(VK_LEFT))
		{
			CameraOffsetScreen(-off,-off);
		}
		else if  (IsKeyDown(VK_UP) && IsKeyDown(VK_RIGHT))
		{
			CameraOffsetScreen(off,-off);
		}
		if (IsKeyDown(VK_DOWN) && IsKeyDown(VK_LEFT))
		{
			CameraOffsetScreen(-off,off);
		}
		else if  (IsKeyDown(VK_DOWN) && IsKeyDown(VK_RIGHT))
		{
			CameraOffsetScreen(off,off);
		}
		else
		{
			switch (nChar)
			{
			case VK_ESCAPE:
				SetSelectFile("",0);
				ClearSelect();
				break;
			case VK_UP:
				CameraOffsetScreen(0,-off);
				break;
			case VK_DOWN:
				CameraOffsetScreen(0,off);
				break;
			case VK_LEFT:
				CameraOffsetScreen(-off,0);
				break;
			case VK_RIGHT:
				CameraOffsetScreen(off,0);
				break;			
/*
			case VK_F1:
				ToggleTrace(iGraphics::TRACE_HELP);
				break;
			case VK_F2:
				ToggleTrace(iGraphics::TRACE_MOUSE);
				break;
*/
			default:
				bHandle = FALSE;
			}
		}
	}

	return bHandle;
}

BOOL cStatus::OnMouseMove(UINT nFlags, CPoint point)
{
	return TRUE;
}

BOOL cStatus::IsIdle(){return TRUE;}
BOOL cStatus::OnCameraMove(CPoint point){return TRUE;}
BOOL cStatus::Copy(){return TRUE;}
BOOL cStatus::Cut(){return TRUE;}
BOOL cStatus::Paste(){return TRUE;}

/////////////////////////////////////////////////////////////////////////////
//Document and commands
BOOL cStatus::Save()
{
	if (GetGround()->IsModify())
	{
		if (AfxMessageBox("当前的文件已经改变，存储么？",MB_OKCANCEL) == IDOK)
		{
			iGround::stParam param;
			param.mask = 0;
			GetGround()->GetParam(param);
			OnSaveDocument(param.szFile);
		}
	}
	return TRUE;
}

BOOL cStatus::OnClose()
{
	Save();
	return TRUE;
}

BOOL cStatus::OnNewDocument(LPCTSTR lpszPathName)
{
	Clear();

	Save();

	CWaitCursor cursor;

	m_strDocument = lpszPathName;

	stCommand_Document param;
	param.type = COM_FILE_NEWDOCUMENT;
	param.size = sizeof(param);
	strcpy(param.strFile,lpszPathName);
	
	DoCommand(&param);

	return TRUE;
}

BOOL cStatus::OnOpenDocument(LPCTSTR lpszPathName)
{
	Clear();

	Save();

	CWaitCursor cursor;

	m_strDocument = lpszPathName;

	stCommand_Document param;
	param.type = COM_FILE_OPENDOCUMENT;
	param.size = sizeof(param);
	strcpy(param.strFile,lpszPathName);
	
	DoCommand(&param);

	return TRUE;
}

BOOL cStatus::OnSaveDocument(LPCTSTR lpszPathName)
{
	Clear();

	CWaitCursor cursor;

	m_strDocument = lpszPathName;
	
	stCommand_Document param;
	param.type = COM_FILE_SAVEDOCUMENT;
	param.size = sizeof(param);
	strcpy(param.strFile,lpszPathName);
	
	DoCommand(&param);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//redo and undo

BOOL cStatus::Undo()
{
	if (!CanUndo())
		return FALSE;
	return GetCommandMachine()->Undo();
}

BOOL cStatus::Redo()
{
	if (!CanRedo())
		return FALSE;
	return GetCommandMachine()->Redo();
}

BOOL cStatus::CanRedo()
{
	return GetCommandMachine()->CanRedo();
}

BOOL cStatus::CanUndo()
{
	return GetCommandMachine()->CanUndo();
}

BOOL cStatus::RedoToEnd()
{
	CWaitCursor cursor;
	return GetCommandMachine()->RedoToEnd();
}

BOOL cStatus::UndoToBegin()
{
	CWaitCursor cursor;
	return GetCommandMachine()->UndoToBegin();
}

//////////////////////////////////////////////////////////////////////
//Output
void cStatus::OutputMsg(int nStatus, LPCTSTR strMsg)
{
	AppOutputMsg(nStatus, strMsg);
}

//////////////////////////////////////////////////////////////////////
//Output
eStatusType cStatus::GetFileType(LPCSTR szFile)
{
	BOOL b;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	b = GetItemValue(pItem,"type",sValue);
	if (!b)
		return STATUS_NULL;
	ASSERT(b);
	eStatusType nType = StatusNameToID(sValue);
	return nType;	
}

int	cStatus::GetSelectFileFrames()
{
	int nFrame;
	iItem* pItem = Common_CreateItem(GetSelectFile(),iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"frames",nFrame);
	return nFrame;
}

int	cStatus::GetSelectFileDirections()
{
	int nFrame;
	iItem* pItem = Common_CreateItem(GetSelectFile(),iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"directions",nFrame);
	return nFrame;
}

eMaskSetting cStatus::FindFileAndGetValue(LPCSTR szFile,LPCSTR szPath,stSetting& set)
{
	CString sFolder = szPath;
	AppGetFullPath(sFolder);
	CString sPathName;
	if (AppFindFile(szFile,sFolder,sPathName))
	{
		eMaskSetting e = GetFileSetting(sPathName,set);
		return e;
	}
	return MASK_EMPTY;
}

//////////////////////////////////////////////////////////////////////
//app message
//////////////////////////////////////////////////////////////////////
void cStatus::BeginMacroCommand()
{
	GetCommandMachine()->BeginMacroCommand();
}

void cStatus::EndMacroCommand()
{
	GetCommandMachine()->EndMacroCommand();
}

void cStatus::DoCommand(stCommand* pCommand)
{
	iCommand* p = GetCommandMachine()->CreateCommand(pCommand);
	GetCommandMachine()->Command(p);
//	GetCommandMessage(p);
	p->Release();
}

void cStatus::GetCommandMessage(iCommand* p)
{
	CString strMsg;
	p->GetMessage(strMsg,OUTPUT_OK);
	if (!strMsg.IsEmpty())
		OutputMsg(OUTPUT_OK,strMsg);

	CString strMsgNext;
	p->GetMessage(strMsgNext,OUTPUT_NEXT);
	if (!strMsgNext.IsEmpty())
		OutputMsg(OUTPUT_NEXT,strMsgNext);
}

void cStatus::SetSetting(const stSetting& st)
{
	if (st.mask & MASK_BOOL)
		m_stSetting.b = st.b;
	if (st.mask & MASK_INT)
		m_stSetting.n = st.n;
	if (st.mask & MASK_FLOAT)
		m_stSetting.f = st.f;
	if (st.mask & MASK_RECT)
		m_stSetting.rect = st.rect;
	if (st.mask & MASK_POINT)
		m_stSetting.pt = st.pt;
	if (st.mask & MASK_STRING)
		m_stSetting.str = st.str;
	if (st.mask & MASK_COLOR)
		m_stSetting.cr = st.cr;
	m_stSetting.mask |= st.mask;
}

//////////////////////////////////////////////////////////////////////
//on select
//////////////////////////////////////////////////////////////////////
BOOL cStatus::OnLostFocus()
{
	ClearSelect();
	return TRUE;
}

void cStatus::MoveiSelect(int e, cPoint point)
{
	GU_SetSelectType(GetSelect(),(SelectEnum)e);
	GU_OffsetSelectTo(GetSelect(),point);
}

void cStatus::MoveiSelect(int e, CPoint point)
{
	cPoint pt;
	GetSelect()->GetOffset(point,pt);
	MoveiSelect(e,pt);
}

void cStatus::ClearSelect()
{
	GU_SetSelectType(GetSelect(),SELECT_NULL);
}

BOOL cStatus::OnRightSelectedFile(LPCTSTR szFile, int nFrame)
{
	if (OnRightSelectedSetting(szFile) != MASK_EMPTY)
		return TRUE;
	if (OnRightSelectedCommand(szFile))
		return TRUE;
	if (OnSelectedFile(szFile,nFrame))
		return TRUE;
	return FALSE;
}

BOOL cStatus::OnRightSelectedCommand(LPCTSTR szFile)
{
	return FALSE;
}

eMaskSetting cStatus::OnRightSelectedSetting(LPCTSTR szFile)
{
	//clear settings
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	if (!GetItemValue(pItem,"want",sValue))
		return MASK_EMPTY;
	eMaskSetting e = WantNameToID(sValue);
	if (e == MASK_EMPTY)
		return MASK_EMPTY;
	CString strHelp;
	if (!GetItemValue(pItem,"help",strHelp))
		return MASK_EMPTY;

//default setting
	stSetting setting;
	setting.b = FALSE;
	setting.f = 0;
	setting.n = 0;
	setting.rect = CRect(0,0,0,0);
	setting.cr = cColor(0,0,0,0);
	setting.str = "null";
	setting.mask = e;

	SetSetting(setting);
	SetValueSetting(strHelp,setting);
	SetItemValue(pItem,"help",strHelp);
	pItem->Save();

	OnSelectedSetting(szFile);
	return e;
}

BOOL cStatus::OnLeftSelectedFile(LPCTSTR szFile, int nFrame)
{
	if (GetFileType(szFile) != GetStatus())
	{
		m_strSelectFile.Empty();
		return FALSE;
	}
	if (OnLeftSelectedSetting(szFile) != MASK_EMPTY)
		return TRUE;
	if (OnLeftSelectedCommand(szFile))
		return TRUE;
	if (OnSelectedFile(szFile,nFrame))
		return TRUE;
	return FALSE;
}

void cStatus::OnSelectedSetting(LPCTSTR szFile)
{
	return;
}

BOOL cStatus::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	if (GetItemValue(pItem,"file",sValue))
	{
		SetSelectFile(szFile,nFrame);
		return TRUE;
	}
	return FALSE;
}

BOOL cStatus::OnLeftSelectedCommand(LPCTSTR szFile)
{
	return FALSE;
}

eMaskSetting cStatus::GetFileSetting(LPCTSTR szFile, stSetting& st)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (pItem == NULL)
		return MASK_EMPTY;
	CString sValue;
	BOOL b;
	b = GetItemValue(pItem,"want",sValue);
	if (!b)
		return MASK_EMPTY;
	eMaskSetting e = WantNameToID(sValue);
	CString strHelp;
	if (!GetItemValue(pItem,"help",strHelp))
		return MASK_EMPTY;
	st.mask = e;
	return GetValueSetting(strHelp,st);
}

BOOL cStatus::GetAString(CString& str,LPCSTR szTile)
{
	CStringDlg dlg;
	dlg.SetString(MASK_STRING,"");
	dlg.SetTitle(szTile);
	if (dlg.DoModal() == IDOK)
	{
		dlg.GetString(MASK_STRING,str);
		return TRUE;
	}
	return FALSE;
}

eMaskSetting cStatus::OnLeftSelectedSetting(LPCTSTR szFile)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	CString sTitle;
	BOOL b;
	b = GetItemValue(pItem,"want",sValue);
	if (!b)
		return MASK_EMPTY;
	eMaskSetting e = WantNameToID(sValue);
	b = GetItemValue(pItem,"title",sTitle);

	ASSERT(e != MASK_EMPTY);

	CString strHelp;
	stSetting setting;
	setting.mask = e;

	if (e == MASK_BOOL)
	{
		GetItemValue(pItem,"help",setting.b);
		setting.b = !setting.b;		
		SetSetting(setting);
		SetItemValue(pItem,"help",setting.b);
		pItem->Save();
	}
	else
	{
		CStringDlg dlg;
		VERIFY(GetItemValue(pItem,"help",strHelp));
		dlg.SetString(e,strHelp);
		if (!sTitle.IsEmpty())
			dlg.SetTitle(sTitle);
		if (dlg.DoModal() == IDOK)
		{
			dlg.GetSetting(setting);
			SetSetting(setting);
			dlg.GetString(e,strHelp);
			SetItemValue(pItem,"help",strHelp);
			pItem->Save();
		}
		else
			return MASK_EMPTY;
	}

	OnSelectedSetting(szFile);
	return e;
}

////////////////////////////////////////////////////////////////////////
//global command
////////////////////////////////////////////////////////////////////////
void cStatus::CameraOffsetScreen(int x,int y)
{
	int sw = 800;
	int sh = 600;
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	int sx = 800 / 2 + x;
	int sy = 600 / 2 + y;
	cPoint ptTo;
	GetCamera()->ScreenToGround(CPoint(sx,sy),ptTo);
	cPoint p = ptTo - param.ptTo;
	CameraOffset(p.x,p.y);
/*
	param.mask = iCamera::MASK_TO | iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	param.ptTo = ptTo;
	GetCamera()->SetParam(param);
*/
}

void cStatus::CameraOffset(float x,float y)
{
	stCommand_OffsetCamera param;
	param.type = COM_CAMERA_OFFSET;
	param.size = sizeof(param);
	param.ptTo.x = x ;
	param.ptTo.y = y ;
	param.ptTo.z = 0;
	DoCommand(&param);
	CPoint pt;
	BOOL b = GetMousePos(pt);
	OnCameraMove(pt);
}

void cStatus::CameraScale(float f)
{
	stCommand_ScaleCamera st;
	st.size = sizeof(st);
	st.type = COM_CAMERA_SCALE;
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	st.fScale = param.fScale + f;
	DoCommand(&st);
}

//brightness
void cStatus::AddWindSpeed(cPoint ptSpeed)
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	param.mask = iGround::MASK_WINDSPEED;
	param.ptWindSpeed += ptSpeed;
	GetGround()->SetParam(param);
}

void cStatus::SetGroundColor(cColor cr)
{
	stCommand_ChangeGroundBrightness st;
	st.crColor = cr;
	st.size = sizeof(st);
	st.type = COM_GROUND_CHANGEBRIGHTNESS;
	DoCommand(&st);
}

void cStatus::SetGroundBrightness(int n)
{
	cColor cr = GetGroundColor();
	stCommand_ChangeGroundBrightness st;
//	st.crColor.r = n;
//	st.crColor.g = n;
//	st.crColor.b = n;
	st.crColor = cr;
	st.crColor.a = n;
	st.size = sizeof(st);
	st.type = COM_GROUND_CHANGEBRIGHTNESS;
	DoCommand(&st);
}

int cStatus::GetGroundBrightness()
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	return param.crColor.a;
}

cColor cStatus::GetGroundColor()
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	return param.crColor;
}

// full screen
void cStatus::ToggleFullscreen() 
{
	iGraphics::stParam param;
	param.mask = 0;
	m_pGraphics->GetParam(param);
	param.mask = iGraphics::MASK_FULLSCREEN;
	param.bFullScreen = !param.bFullScreen;
	m_pGraphics->SetParam(param);
}

void cStatus::ToggleTrace(int e)
{
	iGraphics::stParam param;
	param.mask = 0;
	m_pGraphics->GetParam(param);
	if (param.lTraceEnum & e)
		param.eOPTrace = Bit_Remove;
	else
		param.eOPTrace = Bit_Add;
	param.mask = iGraphics::MASK_TRACE;
	param.lTraceEnum = e;
	m_pGraphics->SetParam(param);
}

void cStatus::ToggleGrid()
{
	stCommand st;
	st.type = COM_GROUND_TOGGLEGRID;
	st.size = sizeof(st);
	DoCommand(&st);
}

/////////////////////////////////////////////////////////////////////////////
BOOL cStatus::OffsetCameraTo(cPoint pt)
{
	stCommand_OffsetCameraTo cmd;
	cmd.type = COM_CAMERA_OFFSETTO;
	cmd.ptTo = pt;
	DoCommand(&cmd);
	return TRUE;	
}

BOOL cStatus::ClickOnMap(CPoint ptScreen) //是否点击了小地图
{
	if (!(IsCtrlDown() && IsShiftDown()))
		return FALSE;
	cPoint ptGround;
	if (IsGraphicsShow(iGraphics::GS_MAP))
	{
		HRESULT hr = GU_GetMapClickPos(GetMap(),ptScreen,ptGround);
		if (!FAILED(hr))
		{
			OffsetCameraTo(ptGround);
			return TRUE;
		}
	}
	if (IsGraphicsShow(iGraphics::GS_SMALLMAP))
	{
		HRESULT hr = GU_GetMapSmallClickPos(GetMapSmall(),ptScreen,ptGround);
		if (!FAILED(hr))
		{
			OffsetCameraTo(ptGround);
			return TRUE;
		}
	}

#pragma message (ATTENTION("当地图跳转的时候，redo和undo会出错"))
	return FALSE;
}

////////////////////////////////////////////////////////////////////////
//String to ID
////////////////////////////////////////////////////////////////////////

eMaskSetting cStatus::WantNameToID(const CString& s)
{
	static CString s_strWantName[] = 
	{
		"rect",
		"point",
		"bool",
		"string",
		"color",
		"int",
		"float",
		"cpoint",
		"clear",
		"",
	};
	int n = NameToID(s_strWantName,s);
	if (n == -1)
		return MASK_EMPTY;
	int e = 1 << n;
	return (eMaskSetting) (e);
}


eStatusType cStatus::StatusNameToID(const CString& s)
{
	static CString s_strTypeName[] =
	{
		"general",
			
		"camera",

		"region",
		"light_static",

		"terrain_square",
		"terrain_diamond",
		"terrain_road",
		
		"doodad_simple",
		"doodad_dynamic",

		"group",
		"group_dynamic",

		"obstacle",

		"special",

		"",
	};
	return (eStatusType)NameToID(s_strTypeName,s);
}

////////////////////////////////////////////////////////////////////////
//Seting utility
////////////////////////////////////////////////////////////////////////

eMaskSetting SetValueSetting(CString& strHelp, const stSetting& st)
{
	eMaskSetting e = (eMaskSetting)st.mask ;
	switch(e)
	{
	case MASK_FLOAT:
		ValueToString(st.f,strHelp);
		break;
	case MASK_INT:
		ValueToString(st.n,strHelp);
		break;
	case MASK_POINT:
		ValueToString(st.pt,strHelp);
		break;
	case MASK_RECT:
		ValueToString(st.rect,strHelp);
		break;
	case MASK_BOOL:
		ValueToString(st.b,strHelp);
		break;
	case MASK_COLOR:
		ValueToString(st.cr,strHelp);
		break;
	case MASK_STRING:
		ValueToString(st.str.c_str(),strHelp);
		break;
	default:
		return MASK_EMPTY;
	}
	return e;
}

eMaskSetting GetValueSetting(const CString& strHelp, stSetting& st)
{
	eMaskSetting e = (eMaskSetting)st.mask ;
	switch(e)
	{
	case MASK_FLOAT:
		StringToValue(strHelp,st.f);
		break;
	case MASK_INT:
		StringToValue(strHelp,st.n);
		break;
	case MASK_POINT:
		StringToValue(strHelp,st.pt);
		break;
	case MASK_RECT:
		StringToValue(strHelp,st.rect);
		break;
	case MASK_BOOL:
		StringToValue(strHelp,st.b);
		break;
	case MASK_COLOR:
		StringToValue(strHelp,st.cr);
		break;
	case MASK_STRING:
		{
			CString str;
			StringToValue(strHelp,str);
			st.str = str;
		}
		break;
	case MASK_CPOINT:
		StringToValue(strHelp,st.point);
		break;		
	default:
		return MASK_EMPTY;
	}
	return e;
}

void MaskRect(cRect& rc, cPoint& p1,cPoint& p2)
{
	rc.l= min(p1.x,p2.x);
	rc.r= max(p1.x,p2.x);
	rc.t= min(p1.y,p2.y);
	rc.b= max(p1.y,p2.y);
}

void MaskRect(CRect& rc, CPoint& p1,CPoint& p2)
{
	rc.left = min(p1.x,p2.x);
	rc.right = max(p1.x,p2.x);
	rc.top = min(p1.y,p2.y);
	rc.bottom = max(p1.y,p2.y);
}

void MaskRect(RECT& rc, POINT& p1,POINT& p2)
{
	rc.left = min(p1.x,p2.x);
	rc.right = max(p1.x,p2.x);
	rc.top = min(p1.y,p2.y);
	rc.bottom = max(p1.y,p2.y);
}

KindEnum cStatus::GetObjKind(iObj* p)
{
	stGroundObjParam param;
	param.ClearMask();
	p->GetParam(param);
	return param.eKind;
}

void cStatus::GetGroundEvent(CString& str)
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	str = param.szDynamic;
}

BOOL cStatus::GetIniFile(const CString& strEvent,long lID,const CString& sDynamic, CString& sini)
{
	if (m_strDocument.IsEmpty())
		return FALSE;
	CString sPath,sName,sExt;
	AppParsePathNameExt(m_strDocument,sPath,sName,sExt);
	sini.Format("%s\\%s\\%s-%s\\%05d.ini",sPath,sName,sDynamic,strEvent,lID);
	AppBuildPath(sini);
	//AppGetFullPath(sini);
	return TRUE;
}

BOOL cStatus::GetObjIniFile(iObj* p,CString& sini)
{
	if (p == NULL)
		return FALSE;
	stGroundObjParam param;
	param.ClearMask();
	p->GetParam(param);
	CString sDynamic;
	switch (param.eDynamic)
	{
	case DYNAMIC_NPC:
		sDynamic = "npc";
		break;
	case DYNAMIC_BOX:
		sDynamic = "box";
		break;
	case DYNAMIC_TRAP:
		sDynamic = "trap";
		break;
	case DYNAMIC_CLIENTONLYNPC:
		sDynamic = "clientonlynpc";
		break;
	case DYNAMIC_CLIENTONLYBOX:
		sDynamic = "clientonlybox";
		break;
	case DYNAMIC_CLIENTONLYTRAP:
		sDynamic = "clientonlytrap";
		break;
	}
	CString s = param.szEvent;
	long lID = param.lID;

	return GetIniFile(s,lID,sDynamic,sini);
}

