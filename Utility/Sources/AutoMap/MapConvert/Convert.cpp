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

#include "fileop.h"

#include "izip.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////
//将障碍转化成格子
/////////////////////////////////////////////////////////
struct stObstacle
{
	ObstacleEnum eObstacle;
	int nPoint;
	float fRadius;
	cPoint aPoint[MAX_POINT];
};

void CMapConvertDlg::GetRegionObstacleFile(int x, int y, CString& strRegion)
{
	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4);
	strRegion.Format("%s\\%03d_%03d\\obstacle.dat",sFolder,x,y);
}

typedef std::list<cPoint> cPointList;
typedef cPointList::iterator itcPoint;


BOOL CMapConvertDlg::ConvertAObstacle(stObstacle& obs,int xRegion,int yRegion)
{
	cPointList ptlist;
	if (obs.nPoint == 1) //点
	{
		cPoint pt = obs.aPoint[0];
		float r = obs.fRadius;
//		ptlist.push_back(cPoint((int)pt.x,(int)pt.y));
		cRect rc(pt.x-r,pt.y-r,pt.x+r,pt.y+r);
		for (int x = floor(rc.l); x <= ceil(rc.r); x++)
		for (int y = floor(rc.t); y <= ceil(rc.b); y++)
		{
			ptlist.push_back(cPoint(x,y));
		}
	}
	else if (obs.nPoint >= 2)
	{
		for (int i=0; i<obs.nPoint-1; i++) //attention -1 here
		{
			cPoint p0 = obs.aPoint[i];
			cPoint p1 = obs.aPoint[i+1];
			if (p0.x < p1.x)
				GetSeamCrossTile(p0,p1,ptlist);			
			else
				GetSeamCrossTile(p1,p0,ptlist);			
		}
	}
	//将所有的格子点放入region
	for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		int x = pt.x / c_nWidthRegion;
		int y = pt.y / c_nHeightRegion;
		int offx = (int)pt.x % c_nWidthRegion;
		int offy = (int)pt.y % c_nHeightRegion;

		if (abs(x - xRegion) >= 10 || abs(y - yRegion) >= 10)
		{
//			ASSERT(0);
			return FALSE;
		}
		ModifyObstacle(x,y,offx,offy,obs.eObstacle);
	}
	return TRUE;
}

void CMapConvertDlg::ClearObstacle()
{
	for (int x=0; x<MAX_REGION; x++)
	for (int y=0; y<MAX_REGION; y++)
	{
		if (m_aRegion[x][y])
		{
			delete m_aRegion[x][y];
			m_aRegion[x][y] = NULL;
		}
	}
	//also clear all file obstacle.dat!
}

void CMapConvertDlg::ModifyObstacle(int xRegion,int yRegion,int offx,int offy,ObstacleEnum e)
{
	ASSERT(e > 0 && e < OBSTACLE_NUM);
	if (xRegion < 0 || xRegion >= MAX_REGION || yRegion < 0 || yRegion >= MAX_REGION)
		return;
	if (m_aRegion[xRegion][yRegion] == NULL)
	{
		stObstacleTile* p = new stObstacleTile;
		ZeroMemory(p,sizeof(stObstacleTile));
		m_aRegion[xRegion][yRegion] = p;
	}
	m_aRegion[xRegion][yRegion]->aTile[offx][offy] = e;
}

enum Obj_Bar_Type			// 物件地面格子障碍类型
{
	Obj_Bar_Empty = 0,			// 空，无障碍
	Obj_Bar_Full,				// 完整障碍
	Obj_Bar_LT,					// 左上区域为障碍
	Obj_Bar_RT,					// 右上区域为障碍
	Obj_Bar_LB,					// 左下区域为障碍
	Obj_Bar_RB,					// 右下区域为障碍
	Obj_Bar_Type_Num,			// 障碍类型数
};

bool IsGenerateObstacle(long l) //该障碍是否是generate出来的
{
	short n = l;
	n >>= 4;
	return (n > Obj_Bar_Full);	
}

long GetObstacleNotGenerate(long l)
{
	if (IsGenerateObstacle(l))
		return 0;
	return l & 0x0f;
}

HRESULT Save16BMPFile(const char *filename,  int width, int height, char* buf)
{
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	bi.biSize  = sizeof(BITMAPINFOHEADER);
	bi.biWidth  = width;
	bi.biHeight   = height;
	bi.biPlanes   = 1;
	bi.biBitCount  = 16;
//	bi.biCompression = BI_BITFIELDS;
	bi.biCompression = 0;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed  = 0;
	bi.biClrImportant = 0;

	int size = bi.biWidth*bi.biHeight*(bi.biBitCount/8);

	char* pExbuffer = buf;
	char* buffer = new char[size];

	unsigned int line = bi.biWidth*(bi.biBitCount/8);
	for (int i=0; i<bi.biHeight; i++)
	{
		memcpy(buffer+i*line,pExbuffer+(bi.biHeight-1-i)*line, line);
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
	return S_OK;
}

inline void SetColorR5G6B5(WORD& cr, BYTE r, BYTE g, BYTE b)
{
	cr = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
}

void CMapConvertDlg::SaveObstacleBmp(LPCSTR szTxt)
{
	CRect rc;
	ifstream of(szTxt);
	of >> rc.left >> rc.top >> rc.right >> rc.bottom;

	int Width = (rc.Width()+1) * 16;
	int Height = (rc.Height()+1) * 32;
	int bmpWidth = Width * 2;
	int bmpHeight = Height;
	
	WORD* pBufferMin = new WORD[bmpWidth * bmpHeight];
	WORD* pBufferMax = new WORD[bmpWidth * bmpHeight];
	WORD* pWordMin = pBufferMin;
	WORD* pWordMax = pBufferMax;

	WORD aWord[10] = {0};
	SetColorR5G6B5(aWord[0],0,0,0);
	SetColorR5G6B5(aWord[1],255,255,255);
	SetColorR5G6B5(aWord[2],0,255,0);
	SetColorR5G6B5(aWord[3],0,0,255);
	SetColorR5G6B5(aWord[4],0,255,255);

	of >> hex;
	int i=0;
	while(i++ < Width * Height)
//	while(!of.eof())
	{
		WORD w;
		of >> w;
		if (w == 0xff)
			w = 0;

		if ((w & 0xf0) == (Obj_Bar_Full << 4))
		{
			*pWordMin++ = aWord[w & 0x0f];
			*pWordMin++ = aWord[w & 0x0f];
		}
		else
		{
			*pWordMin++ = aWord[0];
			*pWordMin++ = aWord[0];
		}
		w &= 0x0f;
		ASSERT(w < OBSTACLE_NUM);
		*pWordMax++ = aWord[w];
		*pWordMax++ = aWord[w];
	}

	CString strBmp;
	strBmp = szTxt;
	strBmp = strBmp.Left(strBmp.GetLength() - 4)+"_max.bmp";
	Save16BMPFile(strBmp,bmpWidth,bmpHeight,(char*)pBufferMax);
	strBmp = szTxt;
	strBmp = strBmp.Left(strBmp.GetLength() - 4)+"_min.bmp";
	Save16BMPFile(strBmp,bmpWidth,bmpHeight,(char*)pBufferMin);
	delete pBufferMax;
	delete pBufferMin;
}

void CMapConvertDlg::SaveObstacle()
{
	CString strRegion;
	CString strTxt;
	for (int x=0; x<MAX_REGION; x++)
	for (int y=0; y<MAX_REGION; y++)
	{
		if (!m_aRegion[x][y])
			continue;

		stObstacleTile &region = *m_aRegion[x][y];
//转化信息
		int i,j;

		for (i=0; i<c_nWidthRegion; i++)
		for (j=0; j<c_nHeightRegion; j++)
		{
			if (region.aTile[i][j] != 0) //本身就是障碍 
			{
				ASSERT(region.aTile[i][j] < 5 && region.aTile[i][j] > 0);
				region.aTile[i][j] |= Obj_Bar_Full << 4;
				ASSERT( (region.aTile[i][j] & 0xffffff00 )== 0);
				continue;
			}

			long left,right,top,bottom;
			left = 0;
			right = 0;
			top = 0;
			bottom = 0;
			//test link left right up down!
			if (i > 0)
				left = GetObstacleNotGenerate(region.aTile[i-1][j]);
			else if (x > 0 && m_aRegion[x-1][y] != NULL)
				left = GetObstacleNotGenerate(m_aRegion[x-1][y]->aTile[c_nWidthRegion-1][j]);
			if (i < c_nWidthRegion - 1)
				right = GetObstacleNotGenerate(region.aTile[i+1][j] );
			else if (m_aRegion[x+1][y] != NULL)
				right = GetObstacleNotGenerate(m_aRegion[x+1][y]->aTile[0][j] );

			if (j > 0)
				top = GetObstacleNotGenerate(region.aTile[i][j-1] );
			else if (y > 0 && m_aRegion[x][y-1] != NULL)
				top = GetObstacleNotGenerate(m_aRegion[x][y-1]->aTile[i][c_nHeightRegion-1] );
			if (j < c_nHeightRegion - 1)
				bottom = GetObstacleNotGenerate(region.aTile[i][j+1]);
			else if (m_aRegion[x][y+1] != NULL)
				bottom = GetObstacleNotGenerate(m_aRegion[x][y+1]->aTile[i][0] );

			int n = 0;
			int type = 0;
			if (left > 0)
			{
				n++;
				type = left;
			}
			if (right > 0)
			{
				n++;
				type = right;
			}
			if (top > 0)
			{
				n++;
				type = top;
			}
			if (bottom > 0)
			{
				n++;
				type = bottom;
			}

			long l = type;
			if (n>=3)
			{
				l = (Obj_Bar_Full << 4)  | type;
				region.aTile[i][j] = l;
			}
			else if (n == 2)
			{
				BOOL b = TRUE;
				if (left > 0 && top > 0)
					l = (Obj_Bar_LT << 4) | type;
				else if (right > 0 && top > 0)
					l = (Obj_Bar_RT << 4) | type;
				else if (left > 0 && bottom > 0)
					l = (Obj_Bar_LB << 4) | type;
				else if (right > 0 && bottom > 0)
					l = (Obj_Bar_RB << 4) | type;
				else
					b = FALSE;
				if (b)
					region.aTile[i][j] = l;
			}
			if (region.aTile[i][j] != 0)
			{
				ASSERT( (region.aTile[i][j] & 0xfffffff0 ) != 0);
			}
			ASSERT( (region.aTile[i][j] & 0xffffff00 )== 0);
			
		}

		GetRegionObstacleFile(x,y,strRegion);
		AppBuildPath(strRegion);
		CFile f;
		if (f.Open(strRegion,CFile::modeCreate|CFile::modeWrite))
			f.Write(&region,sizeof(stObstacleTile));
		strTxt = strRegion.Left(strRegion.GetLength()-4) + ".txt";
		ofstream of(strTxt);
		//of << setiosflags(ios::hex);
		for (j=0; j<c_nHeightRegion; j++)
		{
			for (i=0; i<c_nWidthRegion; i++)
			{
				of.width(2);
				of << hex << region.aTile[i][j] << ' ';
			}
			of << "\n";
		}
	}

	CRect rc;
	GetRect(rc);

	strTxt = m_strFile.Left(m_strFile.GetLength()-4) + "\\obstacle.txt";

	CString strBin;
	strBin = m_strFile.Left(m_strFile.GetLength()-4) + "\\" + BINFILE_OBSTACLE;

	CFile f;
	VERIFY(f.Open(strBin,CFile::modeCreate | CFile::modeWrite));

	ofstream of(strTxt);
	of << rc.left << ' ' << rc.top << ' ' << rc.right << ' '<< rc.bottom << "\n";
	f.Write(&rc,sizeof(rc));

	for (int y=rc.top; y<=rc.bottom; y++)
	{
		for (int j=0; j<c_nHeightRegion; j++)
		{
			for (int x=rc.left; x<=rc.right; x++)
			{
				for (int i=0; i<c_nWidthRegion; i++)
				{
					of.width(2);
					if (!m_aRegion[x][y])
					{
						of << hex << 0x00 << ' ';
						char t = 0;
						f.Write(&t,sizeof(t));
					}
					else
					{
						of << hex << m_aRegion[x][y]->aTile[i][j] << ' ';
						char t = m_aRegion[x][y]->aTile[i][j];
						f.Write(&t,sizeof(t));
					}
				}
			}
			of << "\n";
		}
	}
	of.flush();
	of.close();

	f.Close();
	
	//根据txt文件生成一张bmp
	SaveObstacleBmp(strTxt);
}

void CMapConvertDlg::ConvertObstacleFile(int xRegion,int yRegion,const CString& szFile)
{
	CFileFind f;
	if (!f.FindFile(szFile))
		return;
	
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return;
	wrap_ptr<iItem> ptr(pItem);

	int n;
	if (!GetItemValue(pItem,"elementnum",n))
		return;

	stObstacle obs;
	//0,1,2,3,4,
	cPoint ptOffset;
	for (int index=0; index<n; index++)
	{
		obs.eObstacle = OBSTACLE_NORMAL;
		int n;
		if (GetItemValue(pItem,"obstacletype",n,index))
		{
			if (n > 0 && n < OBSTACLE_NUM)
				obs.eObstacle = (ObstacleEnum)n;
		}
		else
		{
			ASSERT(0);
		}
		
		GetItemValue(pItem,"pointnum",obs.nPoint,index);
		GetItemValue(pItem,"radius",obs.fRadius,index);
		CString strValue;
		GetItemValue(pItem,"pointarray",strValue,index);
		if (obs.nPoint == 1)
			obs.aPoint[0] = cPoint(0,0,0);
		else
			StringToPointArray(strValue,obs.aPoint);
		if (GetItemValue(pItem,"regionoffset",ptOffset,index))
		{
			if (ptOffset.x < c_nWidthRegion*2 && ptOffset.y < c_nHeightRegion*2)
			{	
				ptOffset.x += xRegion * c_nWidthRegion;
				ptOffset.y += yRegion * c_nHeightRegion;
			}
		}
		else
		{
			GetItemValue(pItem,"groundoffset",ptOffset,index);
			if (ptOffset.x < c_nWidthRegion*2 && ptOffset.y < c_nHeightRegion*2)
			{	
				ptOffset.x += xRegion * c_nWidthRegion;
				ptOffset.y += yRegion * c_nHeightRegion;
			}
		}
		for (int i=0; i<obs.nPoint; i++)
			obs.aPoint[i] += ptOffset;
		BOOL b = ConvertAObstacle(obs,xRegion,yRegion);
	}
}

void CMapConvertDlg::ConvertObstacleFolder(const CString& szFolder) 
{
	int pos = szFolder.ReverseFind('\\');
	if (pos == -1)
		return;
	CString sName = szFolder.Mid(pos+1);
	int sx,sy;
	if (!GetRegionSize(sName,sx,sy))
		return;

	CString strFile;
	//clear old binary file
	strFile = szFolder + "\\obstacle.dat";
	AppDeleteFolder(strFile);
	strFile = szFolder + "\\" + FILE_OBSTACLE;
	ConvertObstacleFile(sx,sy,strFile);
	strFile = szFolder + "\\" + FILE_PLANEVEROBSTACLE;
	ConvertObstacleFile(sx,sy,strFile);
	strFile = szFolder + "\\" + FILE_PLANEHORBELOWVEROBSTACLE;
	ConvertObstacleFile(sx,sy,strFile);	
}

void CMapConvertDlg::OnObstacle() 
{
	if (!VerifyFile())
		return ;

	CWaitCursor cursor;

	ClearObstacle();

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
		ConvertObstacleFolder(f.GetFilePath());
	}while(b);

	SaveObstacle();
}

////////////////////////////////////////////////////////////////
//trap to tile
////////////////////////////////////////////////////////////////
void CMapConvertDlg::GetRegionTrapFile(int x, int y, CString& strRegion)
{
	CString sFolder = m_strFile.Left(m_strFile.GetLength()-4);
	strRegion.Format("%s\\%03d_%03d\\trap.txt",sFolder,x,y);
}

void CMapConvertDlg::SaveTrapBmp(LPCSTR szTxt)
{
	CRect rc;
	ifstream of(szTxt);
	of >> rc.left >> rc.top >> rc.right >> rc.bottom;

	int Width = (rc.Width()+1) * 16;
	int Height = (rc.Height()+1) * 32;
	int bmpWidth = Width * 2;
	int bmpHeight = Height;
	
	WORD* pBuffer = new WORD[bmpWidth * bmpHeight];
	WORD* pWord = pBuffer;

	int i=0;

	WORD color;
	SetColorR5G6B5(color,0xff,0xff,0xff);

	of >> hex;

	while(i++ < Width * Height)
//	while(!of.eof())
	{
		WORD w;
		of >> w;
		if (w == 0xff)
			w = 0;
		if (w != 0)
		{
			*pWord++ = color;
			*pWord++ = color;
		}
		else
		{
			*pWord++ = 0;
			*pWord++ = 0;
		}
	}

	CString strBmp = szTxt;
	strBmp = strBmp.Left(strBmp.GetLength() - 3)+"bmp";
	Save16BMPFile(strBmp,bmpWidth,bmpHeight,(char*)pBuffer);
	delete pBuffer;
}

void CMapConvertDlg::SaveTrap()
{
	CString strRegion;
	CString strTxt;
	for (int x=0; x<MAX_REGION; x++)
	for (int y=0; y<MAX_REGION; y++)
	{
		if (!m_aTrap[x][y])
			continue;

		stTrapTile &region = *m_aTrap[x][y];
//转化信息
		int i,j;
		GetRegionTrapFile(x,y,strRegion);
		AppBuildPath(strRegion);
		strTxt = strRegion.Left(strRegion.GetLength()-4) + ".txt";
		ofstream of(strTxt);
		//of << setiosflags(ios::hex);
		for (j=0; j<c_nHeightRegion; j++)
		{
			for (i=0; i<c_nWidthRegion; i++)
			{
				of.width(2);
				of << hex << region.aTile[i][j] << ' ';
			}
			of << "\n";
		}
	}

	CRect rc;
	GetRect(rc);

	strTxt = m_strFile.Left(m_strFile.GetLength()-4) + "\\Trap.txt";

	CString strBin;
	strBin = m_strFile.Left(m_strFile.GetLength()-4) + "\\" + BINFILE_TRAP;

	CFile f;
	VERIFY(f.Open(strBin,CFile::modeCreate | CFile::modeWrite));

	ofstream of(strTxt);
	of << rc.left << ' ' << rc.top << ' ' << rc.right << ' '<< rc.bottom << "\n";
	f.Write(&rc,sizeof(rc));

	for (int y=rc.top; y<=rc.bottom; y++)
	{
		for (int j=0; j<c_nHeightRegion; j++)
		{
			for (int x=rc.left; x<=rc.right; x++)
			{
				for (int i=0; i<c_nWidthRegion; i++)
				{
					of.width(2);
					if (!m_aTrap[x][y])
					{
						of << hex << 0x00 << ' ';
						char t = 0;
						f.Write(&t,sizeof(t));
					}
					else
					{
						of << hex << m_aTrap[x][y]->aTile[i][j] << ' ';
						char t = m_aTrap[x][y]->aTile[i][j];
						f.Write(&t,sizeof(t));
					}
				}
			}
			of << "\n";
		}
	}
	of.flush();
	of.close();

	f.Close();
	
	{
		CString strScriptFile;
		strScriptFile= m_strFile.Left(m_strFile.GetLength()-4) + "\\" + BINFILE_TRAP_SCRIPT;
		CStdioFile f;
		f.Open(strScriptFile,CFile::modeCreate | CFile::modeWrite);
		for (int i=0; i<m_aTrapScript.size(); i++)
		{
			f.WriteString(m_aTrapScript[i]);
			f.WriteString("\n");
		}
	}

	//根据txt文件生成一张bmp
	SaveTrapBmp(strTxt);
}

BOOL CMapConvertDlg::ConvertATrap(stPlaneParam& obs,int xRegion,int yRegion)
{
	ModifyTrap(xRegion,yRegion,obs.ptOffset.x,obs.ptOffset.y,obs.lID);
	return TRUE;
}

void CMapConvertDlg::ClearTrap()
{
	for (int x=0; x<MAX_REGION; x++)
	for (int y=0; y<MAX_REGION; y++)
	{
		if (m_aTrap[x][y])
		{
			delete m_aTrap[x][y];
			m_aTrap[x][y] = NULL;
		}
	}
	m_aTrapScript.clear();
	//also clear all file Trap.dat!
}

void CMapConvertDlg::ModifyTrap(int xRegion,int yRegion,int offx,int offy,long e)
{
	if (xRegion < 0 || xRegion >= MAX_REGION || yRegion < 0 || yRegion >= MAX_REGION)
		return;
	if (m_aTrap[xRegion][yRegion] == NULL)
	{
		stTrapTile* p = new stTrapTile;
		ZeroMemory(p,sizeof(stTrapTile));
		m_aTrap[xRegion][yRegion] = p;
	}
	m_aTrap[xRegion][yRegion]->aTile[offx][offy] = e;
}

BOOL GetIniFile(const CString& m_strDocument,const CString& strEvent,long lID,const CString& sDynamic, CString& sini)
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

void CMapConvertDlg::ConvertTrapFile(int sx,int sy,const CString& szFile)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	int num;
	if (!GetItemValue(pItem,"elementnum",num))
		return;

	stPlaneParam param;
	for (int i=0; i<num; i++)
	{
		GetItemValue(pItem,"regionoffset",param.ptOffset,i);
		//if it is valid!
		if (param.ptOffset.x < c_nWidthRegion && param.ptOffset.y < c_nHeightRegion)
		{
			GetItemValue(pItem,"id",param.lID,i);
			GetItemValue(pItem,"kind",cast_int(param.eKind),i);
			GetItemValue(pItem,"event",param.szEvent);

			ConvertATrap(param,sx,sy);

			CString sIni;
			GetIniFile(m_strFile,param.szEvent,param.lID,"trap",sIni);
			if (!sIni.IsEmpty())
			{
				iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
				wrap_ptr<iItem> ptr(pItem);				
				
				CString strScript;
				GetItemValue(pItem,"Script",strScript,1);
				if (!strScript.IsEmpty())
				{
					if (m_aTrapScript.size() < param.lID)
						m_aTrapScript.resize(param.lID);
					m_aTrapScript[param.lID] = strScript;
				}
			}
			//get script of the trap
		}
	}
}

void CMapConvertDlg::ConvertTrapFolder(const CString& szFolder) 
{
	int pos = szFolder.ReverseFind('\\');
	if (pos == -1)
		return;
	CString sName = szFolder.Mid(pos+1);
	int sx,sy;
	if (!GetRegionSize(sName,sx,sy))
		return;

	CString strFile;
	//clear old binary file
	strFile = szFolder + "\\" + FILE_TRAP;
	CFileFind f;
	if (f.FindFile(strFile))
		ConvertTrapFile(sx,sy,strFile);
}

void CMapConvertDlg::OnTrap() 
{
	if (!VerifyFile())
		return ;

	CWaitCursor cursor;

	ClearTrap();

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
		ConvertTrapFolder(f.GetFilePath());
	}while(b);

	SaveTrap();
}

///////////////////////////////////////////////////////
//Npc = npc + clientonlynpc + trap + box
///////////////////////////////////////////////////////
int on_convertnpc(LPCSTR szNpc)
{
	CString strNpc = szNpc;
	//"NPC.INI"
	CString strName = FILE_NPC;
	CString strBox = strNpc.Left(strNpc.GetLength() - strName.GetLength()) + FILE_BOX;
	CString strTrap = strNpc.Left(strNpc.GetLength() - strName.GetLength()) + FILE_TRAP;
	CString strNpcTemp = strNpc.Left(strNpc.GetLength() - strName.GetLength()) + "temp.ini";

	iItem* pItem = Common_CreateItem(szNpc,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	int num = 0;
	if (!GetItemValue(pItem,"elementnum",num))
		return 0;
	if (num == 0)
		return 0;

	iItem* pItemBox = NULL;
	iItem* pItemNpc = NULL;
	iItem* pItemTrap = NULL;

	int nBox = 0;
	int nTrap = 0;
	int nNpc = 0;

	for (int i = 0; i < num; i++)
	{
		CString strTemplate;
//		GetItemValue(pItem,"template",strTemplate,i);
		stSection* p = pItem->GetIndexSection(i);
		GetSectionValue(p,"template",strTemplate);
		if (strTemplate.Find("box") != -1)
		{
			if (pItemBox == NULL)
				pItemBox = Common_CreateItem(strBox,iItem::FLAG_CREATE);
			pItemBox->AddIndexSection(p,nBox++);
		}
		else if (strTemplate.Find("trap") != -1)
		{
			if (pItemTrap == NULL)
				pItemTrap = Common_CreateItem(strTrap,iItem::FLAG_CREATE);
			pItemTrap->AddIndexSection(p,nTrap++);
		}
		else if (strTemplate.Find("npc") != -1)
		{
			if (pItemNpc == NULL)
				pItemNpc = Common_CreateItem(strNpcTemp,iItem::FLAG_CREATE);
			pItemNpc->AddIndexSection(p,nNpc++);
		}
	}
	if (pItemBox)
	{
		SetItemValue(pItemBox,"elementnum",nBox);
		pItemBox->Save();
	}
	if (pItemTrap)
	{
		SetItemValue(pItemTrap,"elementnum",nTrap);
		pItemTrap->Save();
	}
	if (pItemNpc)
	{
		SetItemValue(pItemNpc,"elementnum",nNpc);
		pItemNpc->Save();
		AppCopyFile(strNpcTemp,szNpc);
		AppDeleteFolder(strNpcTemp);
	}
	else
	{
		AppDeleteFolder(szNpc);
	}

	RELEASE(pItemBox);
	RELEASE(pItemNpc);
	RELEASE(pItemTrap);
	return 0;
}

void CMapConvertDlg::OnConvernpc() 
{
	if (!VerifyFile())
		return;
	CWaitCursor cursor;
	FindFileUnderMap(on_convertnpc,"npc.ini");
}
