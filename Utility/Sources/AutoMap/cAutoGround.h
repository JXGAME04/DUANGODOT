// cAutoGround.h: interface for the cAutoGround class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAUTOGROUND_H__DA6952CA_C154_40B6_8F98_1F4FA6DD6C0D__INCLUDED_)
#define AFX_CAUTOGROUND_H__DA6952CA_C154_40B6_8F98_1F4FA6DD6C0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iAutomap.h"
#include "cAutoTile.h"

#pragma warning (disable: 4786)
#pragma warning (disable: 4503)

#include <queue>
#include <list>
#include <vector>
using namespace std;

const eGroundKind GROUND_NULL = -1;
const int MAX_TERRAKIND = 40;
const int MAX_DEPTH = 5;

struct stTileArray
{
//	eGroundKind eKind[2][2];
	int x,y;
	cAutoTile aTile[MAX_AUTOTILE][MAX_AUTOTILE];
};

struct stConnect
{
	eGroundKind a;
	eGroundKind b;
};

class cAutoGround  : public iAutoGround
{
public:
	cAutoGround();
	virtual ~cAutoGround();

	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;

//////////////////////////////////////////////////////////////////
//load and save

	CString m_strName;
	void New(LPCSTR strFile);
	virtual BOOL Save(LPCSTR strFile);
	virtual BOOL Load(LPCSTR strFile);
	virtual BOOL Load(int x, int y);

	virtual HRESULT	BackFile(int xRegion, int yRegion, LPCSTR szFile);
	virtual HRESULT	RestoreFile(int xRegion, int yRegion, LPCSTR szFile);

	//x 0..1 y 0..1
	void GetFilePath(CString& s, int x, int y);
	void SetAllGround(eGroundKind eKind);
	int m_nSize;
	void SetTileSize(int size){m_nSize = size;}
	int GetTileSize(){return m_nSize;}
	int GetRegionNum(){return m_nSize / 32; }
	int GetRegionSize(){return 512 / m_nSize; }
		
//////////////////////////////////////////////////////////////////
//tile
	stTileArray m_tile;

//////////////////////////////////////////////////////////////////

//ground info
	HRESULT SetTerrainFolder(LPCSTR szTerrainFolder,LPCSTR szExt) ;
	CString m_strTerrainFolder;
	HRESULT SetConnectionFolder(LPCSTR szConnectionFolder,LPCSTR szExt) ;
	CString m_strConnectionFolder;
	CString m_strExt;
	LPCSTR GetExt(){return m_strExt;}
	LPCSTR GetConnectionFolder(){return m_strConnectionFolder;}
	LPCSTR GetTerrainFolder(){return m_strTerrainFolder;}

//kind
	vector<CString> m_aKind;
	eGroundKind NameToKind(LPCSTR szName);
	CString KindToName(eGroundKind kind);
	void AddGroundKind(CString sKind);
	int GetGroundKindNum();
	void Format(stAutoTile& tile);
	void Unformat(stAutoTile& tile);

//connect
	bool m_aaConnection[MAX_TERRAKIND][MAX_TERRAKIND];
	bool IsConnect(eGroundKind x,eGroundKind y);
	void SetConnection(CString sConnect);
	void SetConnection(eGroundKind x,eGroundKind y);
	void ClearConnection()	;

//////////////////////////////////////////////////////////////////
//utility
protected:
	bool FormatPoint(int& x, int& y);
	bool FormatPoint2(int& x, int& y);
	bool FormatPoint3(int& x, int& y);
	bool ProcessRoot(cAutoTile* pRoot,cAutoTile* pEnd);
	bool ChangeTile(int x,int y,float w, float h,eGroundKind eKind);
//	eGroundKind GetCurKind(){return m_tile.eKind[0][0];}
	eTileSide GetOppositeSide(int i);

//////////////////////////////////////////////////////////////////
//operator
public:
	bool ChangeTile(int x,int y,LPCSTR sKind, BOOL bIgnore);
	bool ChangeTile(int x,int y,eGroundKind eKind, BOOL bIgnore);
//	void SetAllGround(int x, int y, LPCSTR sKind);
//	void SetAllGround(int x, int y, eGroundKind eKind);
	void SetAllGround(const CRect& rect, LPCSTR szKind);
	void SetAllGround(const CRect& rect, eGroundKind eKind);
	void SetAllGround(int x,int y, eGroundKind eKind);

	void SetTile(int x,int y,eGroundKind eKind);
	void SetTile(int x,int y,stAutoTile& tile);

//////////////////////////////////////////////////////////////////
//interface
public:
	void ResetTile(int x,int y,eGroundKind eKind);
	void ResetTile(int x,int y,stAutoTile& tile);
	bool IsAllNull(int x,int y);
	bool IsTileChanged(int x,int y);
	void GetTile(int x,int y,stAutoTile& tile);
	void GetBackTile(int x,int y,stAutoTile& tile);	
		
};


#endif // !defined(AFX_CAUTOGROUND_H__DA6952CA_C154_40B6_8F98_1F4FA6DD6C0D__INCLUDED_)
