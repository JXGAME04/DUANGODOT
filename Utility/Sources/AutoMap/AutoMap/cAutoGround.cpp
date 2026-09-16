// cAutoGround.cpp: implementation of the cAutoGround class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AutoMap.h"
#include "cAutoGround.h"
#include "fileop.h"
#include "iImage.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
cAutoGround::cAutoGround()
{
	m_ulCount = 1;
	ClearConnection();

	int x,y;
	for (x=0; x<MAX_AUTOTILE; x++)
	for (y=0; y<MAX_AUTOTILE; y++)
	{
		if (x-1 >= 0)
			m_tile.aTile[x][y].SetTile(TILE_LEFT,&m_tile.aTile[x-1][y]);
		if (x+1 < MAX_AUTOTILE)
			m_tile.aTile[x][y].SetTile(TILE_RIGHT,&m_tile.aTile[x+1][y]);
		if (y-1 >= 0)
			m_tile.aTile[x][y].SetTile(TILE_TOP,&m_tile.aTile[x][y-1]);
		if (y+1 < MAX_AUTOTILE)
			m_tile.aTile[x][y].SetTile(TILE_BOTTOM,&m_tile.aTile[x][y+1]);
		m_tile.aTile[x][y].SetGround(this);
		m_tile.aTile[x][y].SetXY(x,y);
	}
	m_tile.x = -1;
	m_tile.y = -1;
}

cAutoGround::~cAutoGround()
{
	Save(m_strName);
	m_aKind.clear();
}

//////////////////////////////////////////////////////////////////
// com 

ULONG cAutoGround::AddRef() 
{
	return ++m_ulCount;
}

ULONG cAutoGround::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

//////////////////////////////////////////////////////////////////
//load and save
HRESULT	cAutoGround::BackFile(int xRegion, int yRegion, LPCSTR szFile)
{
	CString s;
	GetFilePath(s,xRegion,yRegion);
	CFileFind f;
	if (!f.FindFile(s))
		return -1;
	AppCopyFile(s,szFile);
	return S_OK;
}

HRESULT	cAutoGround::RestoreFile(int xRegion, int yRegion, LPCSTR szFile)
{
	CString s;
	GetFilePath(s,xRegion,yRegion);
	CFileFind f;
	if (!f.FindFile(szFile))
		return -1;
	AppCopyFile(szFile,s);
	return S_OK;
}

void cAutoGround::GetFilePath(CString& s, int tx, int ty)
{
	CString sName = m_strName;
	int pos = sName.ReverseFind('.');
	sName = sName.Left(pos);
	s.Format("%s\\%03d_%03d\\autoground_%d.dat",(LPCSTR)sName,tx,ty,m_nSize);
	AppBuildPath(s);
}

void cAutoGround::New(LPCSTR szFile)
{
	m_strName = szFile;
	m_tile.x = -1;
	m_tile.y = -1;
}

BOOL cAutoGround::Load(LPCSTR strFile)
{
	New(strFile);
	return TRUE;
}

BOOL cAutoGround::Save(LPCSTR strFile) 
{
	m_strName = strFile;

	if (m_tile.x < 0 || m_tile.y < 0)
		return FALSE;

	int num = GetRegionNum();
	for (int i=0; i<num; i++)
	for (int j=0; j<num; j++)
	{
		int tx = m_tile.x + i;
		int ty = m_tile.y + j;

		CString s;
		GetFilePath(s,tx,ty);
		CFile file;
		VERIFY(AppOpenFile(file,s,CFile::modeCreate| CFile::modeWrite,"autoground"));
//		if (!AppOpenFile(file,FileName,CFile::modeRead | CFile::shareDenyWrite,"image"))
//		eGroundKind eKind = m_tile.eKind[i][j];
//		file.Write(&eKind,sizeof(eGroundKind));
		file.Write(&tx,sizeof(tx));
		file.Write(&ty,sizeof(ty));
		int tiles = MAX_AUTOTILE/num*MAX_AUTOTILE/num;
		file.Write(&tiles,sizeof(tiles));
		for (int x=i*MAX_AUTOTILE/num; x<(i+1)*MAX_AUTOTILE/num; x++)
		for (int y=j*MAX_AUTOTILE/num; y<(j+1)*MAX_AUTOTILE/num; y++)
		{
			Format(m_tile.aTile[x][y].m_stTile);
			file.Write(&m_tile.aTile[x][y].m_stTile,sizeof(stAutoTile));
		}
		file.Close();
	}

	return	0;	
}

BOOL cAutoGround::Load(int tx, int ty)
{
	SetAllGround(GROUND_NULL);

	m_tile.x = tx;
	m_tile.y = ty;

	int num = GetRegionNum();
	for (int i=0; i<num; i++)
	for (int j=0; j<num; j++)
	{
		CString s;
		GetFilePath(s,tx+i,ty+j);
		CFile file;
		if (!(file.Open(s,CFile::modeRead)))
			continue;
		int nx,ny;
//		file.Read(&(m_tile.eKind[i][j]),sizeof(eGroundKind));
		file.Read(&nx,sizeof(nx));
		file.Read(&ny,sizeof(ny));
#pragma message (ATTENTION("因为地图作了移动，所以这里错了"))
//		ASSERT(nx == tx+i && ny == ty+j);
		int numTile;
		file.Read(&numTile,sizeof(numTile));
		ASSERT( numTile == MAX_AUTOTILE/num * MAX_AUTOTILE/num );
		for (int x=i*MAX_AUTOTILE/num; x<(i+1)*MAX_AUTOTILE/num; x++)
		for (int y=j*MAX_AUTOTILE/num; y<(j+1)*MAX_AUTOTILE/num; y++)
		{
			//m_tile.aTile[x][y].Load(file);
			file.Read(&m_tile.aTile[x][y].m_stTile,sizeof(stAutoTile));
			Unformat(m_tile.aTile[x][y].m_stTile);
		}
		file.Close();
		
	}
	return TRUE;
}

void cAutoGround::SetAllGround(eGroundKind eKind)
{
	stAutoTile st;
	st.Fill(eKind);
	int i,j;
	for (i=0; i<MAX_AUTOTILE; i++)
	for (j=0; j<MAX_AUTOTILE; j++)
		m_tile.aTile[i][j].SetTile(st);
}

//////////////////////////////////////////////////////////////////

//ground info
HRESULT cAutoGround::SetTerrainFolder(LPCSTR szTerrainFolder,LPCSTR szExt) 
{
	m_strTerrainFolder = szTerrainFolder;
	m_strExt = szExt;
	CFileFind f;
	CString sFind = m_strTerrainFolder + "\\*." + szExt;
	BOOL b = f.FindFile(sFind);
	if (!b)
	{
		return 0;
	}
	while (b)
	{
		b = f.FindNextFile();
		CString sName = f.GetFileName();
		if (sName.Find("大") == 0)
		{
			if (GetTileSize() != 128)
				continue;
		}
		else
		{
			if (GetTileSize() != 64)
				continue;
		}
		int pos = sName.Find('.');
		sName = sName.Left(pos);
		AddGroundKind(sName);
	}
	return 0;
}

HRESULT cAutoGround::SetConnectionFolder(LPCSTR szConnectionFolder,LPCSTR szExt)
{
	m_strConnectionFolder = szConnectionFolder;
	m_strExt = szExt;
	CFileFind f;
	CString sFind = m_strConnectionFolder + "\\*." + szExt;
	BOOL b = f.FindFile(sFind);
	if (!b)
	{
//		ASSERT(0);
		return 0;
	}
	while (b)
	{
		b = f.FindNextFile();
		CString sName = f.GetFileName();
		if (sName.Find("大") == 0)
		{
			if (GetTileSize() != 128)
				continue;
		}
		else
		{
			if (GetTileSize() != 64)
				continue;
		}
		int pos = sName.Find('.');
		sName = sName.Left(pos);
		SetConnection(sName);
	}	
	return 0;
}

//kind
void cAutoGround::AddGroundKind(CString sKind)
{
	m_aKind.push_back(sKind);
}

CString cAutoGround::KindToName(eGroundKind kind)
{
	ASSERT(kind < GetGroundKindNum());
	return m_aKind[kind];
}

eGroundKind cAutoGround::NameToKind(LPCSTR szName)
{
	vector<CString>::iterator it = find(m_aKind.begin(),m_aKind.end(),szName);
	if (it == m_aKind.end())
		return GROUND_NULL;
	return it - m_aKind.begin();
}

int cAutoGround::GetGroundKindNum()
{
	return m_aKind.size();
}

void cAutoGround::Format(stAutoTile& m_stTile)
{
	if (m_stTile.e0 != GROUND_NULL || m_stTile.e1 != GROUND_NULL)
	{
		ASSERT(m_stTile.e0 != GROUND_NULL && m_stTile.e1 != GROUND_NULL);
		CString s0 = KindToName(m_stTile.e0);
		CString s1 = KindToName(m_stTile.e1);
		strcpy(m_stTile.s0,s0);
		strcpy(m_stTile.s1,s1);
	}
}

void cAutoGround::Unformat(stAutoTile& m_stTile)
{
	if (m_stTile.e0 != GROUND_NULL || m_stTile.e1 != GROUND_NULL)
	{
		ASSERT(m_stTile.e0 != GROUND_NULL && m_stTile.e1 != GROUND_NULL);
		eGroundKind k0 = NameToKind(m_stTile.s0);
		eGroundKind k1 = NameToKind(m_stTile.s1);
		for (int i=0; i<4; i++)
		{
			if (m_stTile.aKind[i] == m_stTile.e0)
				m_stTile.aKind[i] = k0;
			else if (m_stTile.aKind[i] == m_stTile.e1)
				m_stTile.aKind[i] = k1;
			else
				ASSERT(0);
		}
		m_stTile.e0 = k0;
		m_stTile.e1 = k1;
	}
}

//connect
bool cAutoGround::IsConnect(eGroundKind x,eGroundKind y)
{
	return m_aaConnection[x][y];
}

void cAutoGround::SetConnection(CString sConnect)
{
	int pos = sConnect.Find('_');
	if (pos == -1)
		return;
	CString sX = sConnect.Left(pos);
	CString sY = sConnect.Mid(pos+1);
	eGroundKind e0 = NameToKind(sX);
	eGroundKind e1 =  NameToKind(sY);
	if (e0 == GROUND_NULL || e1 == GROUND_NULL)
		return;
	SetConnection(e0,e1);
}

void cAutoGround::SetConnection(eGroundKind x,eGroundKind y)
{
	m_aaConnection[x][y] = true;
	m_aaConnection[y][x] = true;
}

void cAutoGround::ClearConnection()	
{
	for (int i=0; i<MAX_TERRAKIND; i++)
	for (int j=0; j<MAX_TERRAKIND; j++)
		m_aaConnection[i][j] = false;
}

//////////////////////////////////////////////////////////////////
//interface
//call it just after changetile so they are true
void cAutoGround::GetBackTile(int x,int y,stAutoTile& tile)
{
	FormatPoint3(x,y);
	ASSERT(!(x < 0 || x >= MAX_AUTOTILE || y < 0 || y >= MAX_AUTOTILE));
	m_tile.aTile[x][y].GetBackTile(tile);		
	Format(tile);
}

void cAutoGround::GetTile(int x,int y,stAutoTile& tile)
{
	FormatPoint3(x,y);
	ASSERT(!(x < 0 || x >= MAX_AUTOTILE || y < 0 || y >= MAX_AUTOTILE));
	m_tile.aTile[x][y].GetTile(tile);		
	Format(tile);
}

void cAutoGround::ResetTile(int x,int y,eGroundKind eKind)
{
	stAutoTile st;
	st.Fill(eKind);
	ResetTile(x,y,st);
}

void cAutoGround::ResetTile(int x,int y,stAutoTile& tile)
{
	int tx = x / (GetRegionSize());
	int ty = y / (GetRegionSize());
	int nx = x % (GetRegionSize());
	int ny = y % (GetRegionSize());
	//indicate which region 
	CString s;
	GetFilePath(s,tx,ty);
	CFile file;
//	VERIFY(file.Open(s,CFile::modeWrite));
	VERIFY(AppOpenFile(file,s,CFile::modeWrite,"autoground"));
	//just write the region
	struct stHeader
	{
		//eGroundKind eKind;
		int tx,ty;
		int tiles;
	};
	file.Seek( sizeof(stHeader) + sizeof(tile) * (nx * GetRegionSize() + ny ), CFile::begin);
	file.Write(&tile,sizeof(tile));
	file.Close();
}

void cAutoGround::SetTile(int x,int y,eGroundKind eKind)
{
	stAutoTile st;
	st.Fill(eKind);
	SetTile(x,y,st);
}

void cAutoGround::SetTile(int x,int y,stAutoTile& tile)
{
	//#pragma message (ATTENTION("should check the x,y"))
	ASSERT(!(x < 0 || x >= MAX_AUTOTILE || y < 0 || y >= MAX_AUTOTILE));
	m_tile.aTile[x][y].SetTile(tile);		
}

bool cAutoGround::IsTileChanged(int x,int y)
{
	FormatPoint3(x,y);
	if (x < 0 || x >= MAX_AUTOTILE || y < 0 || y >= MAX_AUTOTILE)
		return false;
	return m_tile.aTile[x][y].IsChanged();
};

//////////////////////////////////////////////////////////////////
//operator

bool cAutoGround::IsAllNull(int x,int y)
{
	if (m_strName.IsEmpty())
		return false;
	FormatPoint(x,y);
	int i = x / GetRegionSize();
	int j = y / GetRegionSize();

	for (int tx = 0; tx < GetRegionSize(); tx++)
	for (int ty = 0; ty < GetRegionSize(); ty++)
	{
		if (!m_tile.aTile[tx+i*GetRegionSize()][ty+j*GetRegionSize()].IsNull())
			return false;
	}
	return true;
}

void cAutoGround::SetAllGround(const CRect& rect, LPCSTR szKind)
{
	SetAllGround(rect,NameToKind(szKind));
	Save(m_strName);
}

void cAutoGround::SetAllGround(const CRect& rect, eGroundKind eKind)
{
	for (int x=rect.left; x<rect.right;	x++)
	for (int y=rect.top; y<rect.bottom; y++)
	{
		SetAllGround(x,y,eKind);
	}
}

void cAutoGround::SetAllGround(int x,int y, eGroundKind eKind)
{
	stAutoTile tile;
	tile.Fill(eKind);
	Format(tile);

	CString s;
	GetFilePath(s,x,y);
	CFile file;
//	VERIFY(file.Open(s,CFile::modeWrite|CFile::modeCreate));
	VERIFY(AppOpenFile(file,s,CFile::modeCreate| CFile::modeWrite,"autoground"));
//	file.Write(&eKind,sizeof(eKind));
	file.Write(&x,sizeof(x));
	file.Write(&y,sizeof(y));
	int num;
	num = GetRegionSize() * GetRegionSize() ;
	file.Write(&num,sizeof(num));
	for (int i=0; i<num; i++)
		file.Write(&tile,sizeof(tile));
	file.Close();
}

bool cAutoGround::ChangeTile(int x,int y,LPCSTR szKind, BOOL bIgnore)
{
	if (x < 0 || y < 0)
		return false;
	return ChangeTile(x,y,NameToKind(szKind),bIgnore);
}

bool cAutoGround::ChangeTile(int tx,int ty,eGroundKind eKind, BOOL bIgnore)
{
	if (m_strName.IsEmpty())
		return false;
	//do some things!

	FormatPoint(tx,ty);
//************************************************************
//0. 记录所有tile的初始状态
//************************************************************
	for (int x=0; x<MAX_AUTOTILE; x++)
	for (int y=0; y<MAX_AUTOTILE; y++)
	{
		m_tile.aTile[x][y].Back();		
	}

	if (bIgnore)
	{
		stAutoTile st;
		st.Fill(eKind);
		m_tile.aTile[tx][ty].Back();		
		m_tile.aTile[tx][ty].m_nChoose = 0;
		m_tile.aTile[tx][ty].SetTile(st);
		Save(m_strName);
		return TRUE;
	}

	bool b = false;
	b = ChangeTile(tx,ty,1,1,eKind);
	
	if (!b)
	{
		for (int x=0; x<MAX_AUTOTILE; x++)
		for (int y=0; y<MAX_AUTOTILE; y++)
			m_tile.aTile[x][y].Restore();		
	}
	else
	{
		Save(m_strName);
	}

	return b;
}

//////////////////////////////////////////////////////////////////
//utility

bool cAutoGround::FormatPoint3(int& x, int& y)
{
	x -= m_tile.x * (GetRegionSize());
	y -= m_tile.y * (GetRegionSize());
//	ASSERT( x >= 0 && x <MAX_AUTOTILE );
//	ASSERT( y >= 0 && y <MAX_AUTOTILE );
	return TRUE;
}

bool cAutoGround::FormatPoint2(int& x, int& y)
{
	int nx = x ;
	int ny = y ;
	nx -= m_tile.x * (GetRegionSize());
	ny -= m_tile.y * (GetRegionSize());
	if ( nx >= 0 && nx <MAX_AUTOTILE && ny >= 0 && ny <MAX_AUTOTILE )
	{
		x = nx;
		y = ny;
		return TRUE;
	}
	return FormatPoint(x,y);
}

bool cAutoGround::FormatPoint(int& x, int& y)
{
	int tx = x / (GetRegionSize());
	int ty = y / (GetRegionSize());
	if (x % (GetRegionSize()) < GetRegionSize()/2)
		tx -- ;
	if (y % (GetRegionSize()) < GetRegionSize()/2)
		ty -- ;
	if (tx < 0)
		tx = 0;
	if (ty < 0)
		ty = 0;

	Load(tx,ty);

	x -= m_tile.x * (GetRegionSize());
	y -= m_tile.y * (GetRegionSize());
	ASSERT(x >= 0 && y >= 0);
	return TRUE;
}

#ifdef _DEBUG
//#define _TRACE1
#endif
void DumpTile(cAutoTile* pLeave)
{
	CString strKind[] = 
	{
		"GROUND_CAOYUAN", "GROUND_HUANGTU",	"GROUND_HEITU","GROUND_KUCAO","GROUND_SATU",
		"GROUND_SAXI","GROUND_SAMO","GROUND_BIBO","GROUND_BOXUE","GROUND_BINGMIAN",
		"GROUND_HONGTU","GROUND_HUANGSHUI","GROUND_LUOYE","GROUND_HONGXI","GROUND_HUANGXI",
		"GROUND_CHANGCAO","GROUND_QINGSHUI","GROUND_DACAOYUAN","GROUND_DABIBO",
	};
	CString strStyle[] = 
	{
		"TILE_0000","TILE_0001","TILE_0010","TILE_0011",
		"TILE_0100","TILE_0101","TILE_0110","TILE_0111",
		"TILE_1000","TILE_1001","TILE_1010","TILE_1011",
		"TILE_1100","TILE_1101","TILE_1110","TILE_1111",
	};
#ifdef _TRACE		
	afxDump << "pos(" << pLeave->m_tile.x << "," << pLeave->m_tile.y << ")"
		<< " e0: " << strKind[pLeave->m_stTile.e0] << " e1: " << strKind[pLeave->m_stTile.e1] << " style: " << strStyle[pLeave->m_stTile.eStyle]
		<< " chooses: " << pLeave->m_tileList.GetCount()  << " choose: " << pLeave->m_nChoose << "\n"; 
#endif
}

void DumpID(LPCSTR strMessage,int id)
{
#ifdef _TRACE		
	afxDump << strMessage << id << "\n";
#endif
}

void DumpPos(LPCSTR strMessage,int x, int y)
{
#ifdef _TRACE		
	afxDump << strMessage << " pos(" << x << "," << y << ")\n" ;
#endif
}

bool cAutoGround::ChangeTile(int xCenter,int yCenter,float xWidth,float yWidth,eGroundKind eKind)
{
	int i,j;

	int x0 = xCenter;
	int x1 = xCenter+xWidth;
	int y0 = yCenter;
	int y1 = yCenter+yWidth;

	//trim the x,y no need it
//************************************************************
//1．构造一棵树，把将要改变的tile，作为树根节点，改变后放入。
//************************************************************

	cAutoTile* pTile = NULL;
	for (i=x0; i<x1; i++)
	for (j=y0; j<y1; j++)
	{
		SetTile(i,j,eKind);
		if (pTile != NULL)
			pTile->AddNext(&m_tile.aTile[i][j]);
		pTile = &m_tile.aTile[i][j];
		pTile->SetRoot(true);
	}

	cAutoTile* pRoot = &m_tile.aTile[x0][y0];
	cAutoTile* pEnd = pRoot;
	while(pEnd->m_pNext != NULL)
		pEnd = pEnd->m_pNext;

	bool b = ProcessRoot(pRoot,pEnd);
	if (!b)
	{
		
	}
	return b;
}

bool cAutoGround::ProcessRoot(cAutoTile* pRoot,cAutoTile* pEnd)
{
//************************************************************
//2．按广度遍历，如果树上的所有节点均不引起冲突，返回true。
//************************************************************
#ifdef _TRACE1
	afxDump << "\n\n process root start \n\n";
#endif
	int nLen = pRoot->GetLength();
	do
	{
		cAutoTile* pFather;
		cAutoTile* pLeave = pRoot->GetFirstConflictLeave(&pFather);
		if (pLeave == NULL)
			break;

		int nDepth = pFather->GetDepth();	
		if (nDepth < MAX_DEPTH && pLeave->ChangeTile(nDepth ))
		{
//************************************************************
//3．否则对于该节点，如果改变成功改变后添加到树中，转2。
//************************************************************

			DumpID("add a leave. id: ",nLen);
			DumpTile (pLeave);

			pEnd->AddNext(pLeave);
			pLeave->AddFather(pFather);
			nLen ++;
			pEnd = pLeave;
			continue;
		}
		else
		{

///*
			DumpPos("Conflict father",pFather->m_x,pFather->m_y);
			DumpPos("Conflict leave",pLeave->m_x,pLeave->m_y);
//*/
//************************************************************
//4．如果3不成功，将其前一个节点（广度遍历的逆过程），取其下一个方案，
//如果成功，将所有后继叶子的方案值清空，并且恢复到初始状态，转3。
//向前做这个过程，直到树根.
//************************************************************

			tileList list;
			list.AddTail(pLeave);
			
			cAutoTile* pPrev = NULL;
			bool bChooseNext = false;

			while(!pEnd->IsRoot())
			{			
				cAutoTile* pConnect = pEnd;
				while(!pConnect->HaveConnect(list))
				{
					pConnect = pConnect->m_pPrev;
					if (pConnect->IsRoot())
						return false;
				}
				while(pConnect != pEnd)
				{	
					DumpID("remove a leave: ", nLen - 1);
					
					pPrev = pEnd->m_pPrev;
					pPrev->m_pNext = NULL;
					pEnd->Restore();
					nLen --;
					pEnd = pPrev;
				}

				if (pEnd->ChooseNext())
				{
					DumpID("change true: ", nLen - 1);
					DumpTile(pEnd);
					
					bChooseNext = true;
					break;
				}
				else
				{
					list.AddTail(pEnd);
					pPrev = pEnd->m_pPrev;
					pPrev->m_pNext = NULL;
					pEnd->Restore();
					nLen --;
					pEnd = pPrev;					
				}
			}
			if (bChooseNext)
				continue;
			else
				return false;
		}
	}
	while(1);
#ifdef _TRACE1
	afxDump << "process root success! \n\n";
#endif
	return true;
}

