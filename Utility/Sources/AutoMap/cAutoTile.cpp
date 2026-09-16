// cAutoTile.cpp: implementation of the cAutoTile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AutoMap.h"
#include "cAutoTile.h"
#include "cAutoGround.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cAutoTile::cAutoTile()
{
	for (int i=0; i<4; i++)
	{
		m_aTile[i] = NULL;
		m_stTile.aKind[i] = GROUND_NULL;
		m_stTile.e0 = GROUND_NULL;
		m_stTile.e1 = GROUND_NULL;
		m_stTile.eStyle = TILE_0000;
		m_stTile.dwParam = 0;
	}
	m_nChoose = -1;
	m_bRoot = false;
	m_pPrev = NULL;
	m_pNext = NULL;
	m_pFather = NULL;
	m_x = -1;
	m_y = -1;
}

cAutoTile::~cAutoTile()
{

}

bool cAutoTile::IsNull()
{
	if (m_stTile.e0 == GROUND_NULL && m_stTile.e1 == GROUND_NULL)
		return true;
	return false;
}

void cAutoTile::Save(CFile& file)
{
/*
	if (m_stTile.e0 != GROUND_NULL || m_stTile.e1 != GROUND_NULL)
	{
		ASSERT(m_stTile.e0 != GROUND_NULL && m_stTile.e1 != GROUND_NULL);
		CString s0 = GetGround()->KindToName(m_stTile.e0);
		CString s1 = GetGround()->KindToName(m_stTile.e1);
		strcpy(m_stTile.s0,s0);
		strcpy(m_stTile.s1,s1);
	}
*/
	GetGround()->Format(m_stTile);
	file.Write(&m_stTile,sizeof(m_stTile));
}

void cAutoTile::Load(CFile& file)
{
	file.Read(&m_stTile,sizeof(m_stTile));
	GetGround()->Unformat(m_stTile);
/*
	if (m_stTile.e0 != GROUND_NULL || m_stTile.e1 != GROUND_NULL)
	{
		ASSERT(m_stTile.e0 != GROUND_NULL && m_stTile.e1 != GROUND_NULL);
		eGroundKind k0 = GetGround()->NameToKind(m_stTile.s0);
		eGroundKind k1 = GetGround()->NameToKind(m_stTile.s1);
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
*/
}

void cAutoTile::AddFather(cAutoTile* p)
{
	m_pFather = p;
}

void cAutoTile::AddNext(cAutoTile* p)
{
	m_pNext = p;
	p->m_pPrev = this;
}

int cAutoTile::GetLength()
{
	cAutoTile* pTile = this;
	int nDepth = 0;
	do
	{
		pTile = pTile->m_pNext;
		nDepth ++;
	}while(pTile);
	return nDepth;
}

cAutoTile* cAutoTile::GetFirstConflictLeave(cAutoTile** pFather)
{
	cAutoTile* pTile = this;
	do
	{
		for (int i=0; i<4; i++)
		{
			if (!pTile->Fit(i))
			{
				*pFather = pTile;
				return pTile->m_aTile[i];
			}
		}
		pTile = pTile->m_pNext;
	}while(pTile);
	return NULL;
}

cAutoTile* cAutoTile::GetPrevLeave()
{
	return m_pPrev;
}

/*
//style like
00 00 00 01 10 00 01 01 10 10 11 01 10 11 11 11
00 01 10 00 00 11 01 10 01 10 00 11 11 01 10 11
*/

const int g_nStyle = 16;
int g_aStyle[16][2][2] = 
{
	0,0,0,0,
	0,0,0,1,
	0,0,1,0,
	0,0,1,1,
	0,1,0,0,
	0,1,0,1,
	0,1,1,0,
	0,1,1,1,
	1,0,0,0,
	1,0,0,1,
	1,0,1,0,
	1,0,1,1,
	1,1,0,0,
	1,1,0,1,
	1,1,1,0,
	1,1,1,1,
};

bool cAutoTile::ConstructTileList(int bPerfect)
{
	//寻找四周已经上好的e0,e1
	if (bPerfect == MAX_DEPTH - 2)
	{
		int n;
		n = 0;
	}
	
	int nKind = 0;
	eGroundKind e[2][2];
	for (int i=0; i<2; i++)
	for (int j=0; j<2; j++)
		e[i][j] = GROUND_NULL;

	for (i=0; i<4; i++)
	{
		if (m_aTile[i]!=NULL && m_aTile[i]->IsChanged())
		{
			int x1,y1,x2,y2;
			stAutoTileSide eSide = m_aTile[i]->GetSide(GetOppositeSide(i));
			switch (i)
			{
			case TILE_LEFT:
				x1 = 0;
				y1 = 0;
				x2 = 0;
				y2 = 1;
				break;
			case TILE_RIGHT:
				x1 = 1;
				y1 = 0;
				x2 = 1;
				y2 = 1;
				break;
			case TILE_TOP:
				x1 = 0;
				y1 = 0;
				x2 = 1;
				y2 = 0;
				break;
			case TILE_BOTTOM:
				x1 = 0;
				y1 = 1;
				x2 = 1;
				y2 = 1;
				break;
			}
			//产生冲突
			if (e[x1][y1] == GROUND_NULL)
				e[x1][y1] = eSide.e0;
			else if (e[x1][y1] != eSide.e0)
				return false;
			if (e[x2][y2] == GROUND_NULL)
				e[x2][y2] = eSide.e1;
			else if (e[x2][y2] != eSide.e1)
				return false;
		}
	}

	eGroundKind e0= GROUND_NULL,e1= GROUND_NULL;
	for (i=0; i<2; i++)
	for (int j=0; j<2; j++)
	{
		eGroundKind kind = e[i][j];
		if ( kind != GROUND_NULL)
		{
			if (e0 == GROUND_NULL)
				e0 = kind;
			else if (e0 != kind)
			{
				if (e1 == GROUND_NULL)
					e1 = kind;
				else if (e1 != kind)
					return false;
				//一个tile中最多有两种图素
			}
		}
	}
	ASSERT(e0 != GROUND_NULL);
	if (e1 == GROUND_NULL)
	{
		//test all connect of the e0
		for (int i=0; i<GetGround()->GetGroundKindNum(); i++)
		{
			eGroundKind e1 = i;
			if (GroundKindNearby(e1) && e1 != e0)
				GetPossibleTile(e0,e1,e);
		}

		stAutoTile st;
		st.e0 = e0;
		st.e1 = e0;
		st.eStyle = TILE_0000;
		for (i=0; i<4; i++)
			st.aKind[i] = e0;
		m_tileList.AddTail(st);

	}
	else 
	{
		if (!GetPossibleTile(e0,e1,e))
			return false;
	}
//排序是为了改动尽可能的小
	SortList(bPerfect);
	return true;
}

bool cAutoTile::GroundKindNearby(eGroundKind e)
{
	for (int i=0; i<4; i++)
	{
		if (m_aTile[i] != NULL && m_aTile[i]->AGroundKindNearby(e))
			return true;
	}
	return false;
}

bool cAutoTile::HaveConnect(tileList& rlist)
{
	for (POSITION pos = rlist.GetHeadPosition(); pos != NULL;)
	{
		cAutoTile* p = rlist.GetNext(pos);
		if (HaveConnect(p))
			return true;
	}
	return false;
}

bool cAutoTile::HaveConnect(cAutoTile* p)
{
	for (int i=0; i<4; i++)
	{
		if (m_aTile[i] == p)
			return true;
	}
	return false;
}

bool cAutoTile::HaveKind(eGroundKind e)
{
	if (m_stTile.e0 == e && m_stTile.eStyle != TILE_1111)
		return true;
	if (m_stTile.e1 == e && m_stTile.eStyle != TILE_0000)
		return true;	
	return false;
}

bool cAutoTile::AGroundKindNearby(eGroundKind e)
{
	if (HaveKind(e))
		return true;
	for (int i=0; i<4; i++)
	{
		if (m_aTile[i] != NULL &&  m_aTile[i]->HaveKind(e) )
			return true;
	}
	return false;
}

bool cAutoTile::GetPossibleTile(eGroundKind e0,eGroundKind e1, eGroundKind e[2][2])
{
	if (!GetGround()->IsConnect(e0, e1))
		return false;
	
	stAutoTile st;
	for (int i=0; i<g_nStyle; i++)
	{
		if (i == TILE_0000)
			continue;
		for (int x=0; x<2; x++)
		for (int y=0; y<2; y++)
		{
			if (g_aStyle[i][y][x] == 0 && e[x][y] == e1)
			{
				goto next;
			}
			if (g_aStyle[i][y][x] == 1 && e[x][y] == e0)
			{
				goto next;
			}
		}
		GetTileByStyle((eTileStyle) i,e0,e1,st);
		m_tileList.AddTail(st);
next:
		;
	}
	return true;
}

void cAutoTile::GetTileByStyle(eTileStyle eStyle,eGroundKind e0,eGroundKind e1,stAutoTile& st)
{
	int nStyle = eStyle;
	if (e0 > e1)
	{	
		nStyle = TILE_1111  - nStyle;
		eGroundKind e = e0;
		e0 = e1;
		e1 = e;
	}
	
	st.e0 = e0;
	st.e1 = e1;
	st.eStyle = (eTileStyle )nStyle;
	
	int j = 0x8;
	for (int i=0; i<4; i++)
	{ 
		if (nStyle & j)
			st.aKind[i] = e1;
		else
			st.aKind[i] = e0;
		j >>= 1;
	}
}

bool cAutoTile::FitSide(int side)
{
	stAutoTile st;
	st.e0 = GROUND_NULL;
	st.e1 = GROUND_NULL;
	for (int i=0; i<4; i++)
		st.aKind[i] = GROUND_NULL;
	st.eStyle = TILE_0000;
	switch (side)
	{
	case 0:
	case 2:
		if (m_aTile[0] != NULL)
		{
			st.aKind[TILE_LEFTTOP] = m_aTile[0]->m_stTile.aKind[TILE_RIGHTTOP];
			st.aKind[TILE_LEFTBOTTOM] = m_aTile[0]->m_stTile.aKind[TILE_RIGHTBOTTOM];
		}
		if (m_aTile[2] != NULL)
		{
			st.aKind[TILE_RIGHTTOP] = m_aTile[2]->m_stTile.aKind[TILE_LEFTTOP];
			st.aKind[TILE_RIGHTBOTTOM] = m_aTile[2]->m_stTile.aKind[TILE_LEFTBOTTOM];
		}
		break;
	case 1:
	case 3:
		if (m_aTile[1] != NULL)
		{
			st.aKind[TILE_LEFTTOP] = m_aTile[1]->m_stTile.aKind[TILE_LEFTBOTTOM];
			st.aKind[TILE_RIGHTTOP] = m_aTile[1]->m_stTile.aKind[TILE_RIGHTBOTTOM];
		}
		if (m_aTile[3] != NULL)
		{
			st.aKind[TILE_LEFTBOTTOM] = m_aTile[3]->m_stTile.aKind[TILE_LEFTTOP];
			st.aKind[TILE_RIGHTBOTTOM] = m_aTile[3]->m_stTile.aKind[TILE_RIGHTTOP];
		}
		break;
	}
	for (i = 0; i < 4; i++ )
	{
		if (st.aKind[i] != GROUND_NULL)
		{
			if (st.e0 == GROUND_NULL || st.e0 == st.aKind[i])
				st.e0 = st.aKind[i];
			else if (st.e1 == GROUND_NULL || st.e1 == st.aKind[i])
				st.e1 = st.aKind[i];
			else
				return false;
		}
	}
	
	if (st.e1 != GROUND_NULL)
	if (!GetGround()->IsConnect(st.e0,st.e1))
		return false;

	stAutoTile stTemp = m_stTile;
	m_stTile = st;
	for (i=0; i<4; i++)
	{
		if (m_aTile[i] != NULL)
		{
			if (m_aTile[i]->IsChanged() || i == side)
			if (!FitNull(i))
			{
				break;
			}
		}
	}
	m_stTile = stTemp;
	if (i == 4)
		return true;
	return false;
}

void cAutoTile::SortList(int bPerfect)
{
	stAutoTile stTemp = m_stTile;
	int nTemp = m_nChoose;
	m_nChoose = 0;

	if (bPerfect == MAX_DEPTH - 2)
	{
		int n;
		n = 0;
	}

	CList<stAutoTile,stAutoTile&> listTemp;
	int nMaxFit = 0;
	{
		for (POSITION pos = m_tileList.GetHeadPosition(); pos!=NULL;)
		{
			stAutoTile stTile = m_tileList.GetNext(pos);	
			m_stTile = stTile;	
			int nFit = 0;
			bool bContinue = false;
			for (int i=0; i<4; i++)
			{
				if (Fit(i))
					nFit++;
				else if (bPerfect == MAX_DEPTH - 1)
				{
					bContinue = true;
				}

				if (bPerfect == MAX_DEPTH - 2)
				{
					if (m_aTile[i]!=NULL && !m_aTile[i]->IsChanged())
					{
						bool b;
						int side = i;
						b = m_aTile[i]->FitSide(side);
						if (!b)
							bContinue = true;
					}
				}
				if (bContinue == true)
					break;
			}
			if (bContinue)
				continue;
			if (nFit >= nMaxFit)
			{
				nMaxFit = nFit;
				stTile.dwParam = nFit;
				listTemp.AddTail(stTile);
			}
		}
	}

	m_tileList.RemoveAll();	
	for (POSITION pos = listTemp.GetHeadPosition(); pos!=NULL;)
	{
		stAutoTile stTile = listTemp.GetNext(pos);	
		if (stTile.dwParam == nMaxFit)
		{
			m_tileList.AddTail(stTile);
		}
	}

	m_stTile = stTemp;
	m_nChoose = nTemp;
}

int cAutoTile::GetDepth()
{
	int n = 0;
	cAutoTile* pFather = this;
	while (pFather)
	{
		pFather = pFather->m_pFather;
		n ++;
	}
	return n;
}

bool cAutoTile::ChangeTile(int bPerfect)
{
	bool b = ConstructTileList(bPerfect);
	if (!b)
		return false;
	m_nChoose = 0;
	POSITION pos = m_tileList.FindIndex(m_nChoose);
	if (pos == NULL)
	{
		m_nChoose = -1;
		return false;
	}
	m_stTile = m_tileList.GetAt(pos);
	return true;
}

bool cAutoTile::ChooseNext()
{
	ASSERT(m_nChoose != -1);
	POSITION pos = m_tileList.FindIndex(++m_nChoose);
	if (pos == NULL)
		return false;
	m_stTile = m_tileList.GetAt(pos);
	return true;
}

void cAutoTile::Back()
{
	m_stTileBack = m_stTile;

	m_nChoose = -1;
	m_pPrev = NULL;
	m_pNext = NULL;
	m_pFather = NULL;
	m_bRoot = false;
	m_tileList.RemoveAll();
}

void cAutoTile::Restore()
{
	m_nChoose = -1;
	m_pPrev = NULL;
	m_pNext = NULL;
	m_pFather = NULL;
	m_bRoot = false;
	m_tileList.RemoveAll();

	m_stTile = m_stTileBack;	
}

void cAutoTile::SetKind(int dir, eGroundKind eKind)
{
	m_stTile.aKind[dir] = eKind;
}

void cAutoTile::SetKind(eGroundKind eKind)
{
	for (int i=0; i<4; i++)
		m_stTile.aKind[i] = eKind;
}

stAutoTileSide cAutoTile::GetSide(int nSide)
{
	stAutoTileSide side;
	switch (nSide)
	{
	case TILE_LEFT:
		side.e0 = m_stTile.aKind[TILE_LEFTTOP];
		side.e1 = m_stTile.aKind[TILE_LEFTBOTTOM];
		break;
	case TILE_RIGHT:
		side.e0 = m_stTile.aKind[TILE_RIGHTTOP];
		side.e1 = m_stTile.aKind[TILE_RIGHTBOTTOM];
		break;
	case TILE_TOP:
		side.e0 = m_stTile.aKind[TILE_LEFTTOP];
		side.e1 = m_stTile.aKind[TILE_RIGHTTOP];
		break;
	case TILE_BOTTOM:
		side.e0 = m_stTile.aKind[TILE_LEFTBOTTOM];
		side.e1 = m_stTile.aKind[TILE_RIGHTBOTTOM];
		break;
	}
	return side;
}

bool cAutoTile::FitNull(int side)
{
	return Fit(side);
}

bool cAutoTile::Fit(int side)
{
	eTileSide eSide0 = (eTileSide)side;
	eTileSide eSide1 = GetOppositeSide(side);
	if (m_aTile[eSide0] == NULL)
		return true;
	stAutoTileSide s0 = GetSide(side);
	stAutoTileSide s1 = m_aTile[eSide0]->GetSide(eSide1);
	if (s0.e0 == GROUND_NULL || s0.e1 == GROUND_NULL)
		return true;
	if (s1.e0 == GROUND_NULL || s1.e1 == GROUND_NULL)
		return true;
	if ( s0 == s1 )
		return true;
	return false;
}

eTileSide cAutoTile::GetOppositeSide(int i)
{
	i+=2;
	if (i >= 4)
		i-=4;
	return (eTileSide)i;
}
