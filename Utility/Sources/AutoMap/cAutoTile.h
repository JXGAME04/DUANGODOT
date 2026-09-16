// cAutoTile.h: interface for the cAutoTile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAUTOTILE_H__DC6F22A6_0492_4B4A_AD75_54DBD85F3333__INCLUDED_)
#define AFX_CAUTOTILE_H__DC6F22A6_0492_4B4A_AD75_54DBD85F3333__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iAutomap.h"
class cAutoGround;

typedef CTypedPtrList <CPtrList, cAutoTile*>  tileList;

class cAutoTile  
{
public:
	cAutoTile();
	virtual ~cAutoTile();
	
	cAutoGround* m_pGround;
	cAutoGround* GetGround(){return m_pGround;}
	void SetGround(cAutoGround* p){m_pGround = p;}

	void Save(CFile& file) ;
	void Load(CFile& file) ;
	
	int m_x,m_y;
	void SetXY(int x,int y){m_x = x;m_y = y;}
	stAutoTile m_stTile;
	stAutoTile m_stTileBack;
	int m_nChoose;
	cAutoTile* m_pNext,*m_pPrev;
	cAutoTile*  m_pFather;
	CList<stAutoTile,stAutoTile&> m_tileList;
	void AddNext(cAutoTile* p);
	void AddFather(cAutoTile* p);
	cAutoTile* GetFirstConflictLeave(cAutoTile** p);
	int GetLength();
	int GetDepth();
	bool ConstructTileList(int bPerfect);
	bool IsChanged(){return (m_nChoose != -1 || m_bRoot );}
	bool ChangeTile(int bPerfect);
	cAutoTile* GetPrevLeave();
	bool ChooseNext();
	bool m_bRoot;
	void SetRoot(bool bRoot){m_bRoot = bRoot;};
	bool IsRoot(){return m_bRoot;}
	bool GetPossibleTile(eGroundKind e0,eGroundKind e1, eGroundKind e[2][2]);
	void SortList(int bPerfect);
	void GetTileByStyle(eTileStyle eStyle,eGroundKind e0,eGroundKind e1,stAutoTile& st);
	bool AGroundKindNearby(eGroundKind e);
	bool GroundKindNearby(eGroundKind e);
	bool HaveKind(eGroundKind e);
	bool HaveConnect(cAutoTile* p);
	bool HaveConnect(tileList& list);
	bool FitSide(int side);
	bool FitNull(int side);
		
	void Back();
	void Restore();
	cAutoTile* m_aTile[4];
	void SetTile(eTileSide e, cAutoTile* pTile){m_aTile[e] = pTile;}
	bool IsNull();
	
	void GetTile(stAutoTile& st) {st = m_stTile;};
	void GetBackTile(stAutoTile& st) {st = m_stTileBack;};
	
	void SetTile(stAutoTile& st) {m_stTile = st;};
	
	eGroundKind GetKind(int order){return m_stTile.aKind[order];}
	void SetKind(int order, eGroundKind eKind);
	void SetKind(eGroundKind eKind);
	stAutoTileSide GetSide(int nSide);

	bool Fit(int side);
	
	eTileSide GetOppositeSide(int i);
};


#endif // !defined(AFX_CAUTOTILE_H__DC6F22A6_0492_4B4A_AD75_54DBD85F3333__INCLUDED_)
