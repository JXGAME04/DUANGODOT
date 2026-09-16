
#ifndef IAUTOMAP_H
#define IAUTOMAP_H

#include "iPublic.h"

class cAutoTile;
class cAutoRoad;
class cAutoWall;

const int MAX_AUTOTILE = 16;
//16 是一次ChangeTile需要遍历的空间;
//如果tile的size = 64,  需要周围4个region的信息
//如果tile的size = 128, 需要周围16个region的信息

typedef int eGroundKind;
enum eTileSide{TILE_LEFT,TILE_TOP,TILE_RIGHT,TILE_BOTTOM};
enum eTileOrder{TILE_LEFTTOP,TILE_RIGHTTOP,TILE_LEFTBOTTOM,TILE_RIGHTBOTTOM};

enum eTileStyle{
	TILE_0000,TILE_0001,TILE_0010,TILE_0011,
	TILE_0100,TILE_0101,TILE_0110,TILE_0111,
	TILE_1000,TILE_1001,TILE_1010,TILE_1011,
	TILE_1100,TILE_1101,TILE_1110,TILE_1111,
};

struct stAutoTile
{
	char s0[MAX_NAME];
	char s1[MAX_NAME];
	eGroundKind aKind[4];
	eTileStyle eStyle;
	eGroundKind e0,e1;
	DWORD dwParam;
	void Fill(eGroundKind kind)
	{
		for (int i=0; i<4; i++)
			aKind[i] = kind;
		e0 = kind;e1 = kind;
		eStyle = TILE_0000;
	}
};

struct stAutoTileSide
{
	eGroundKind e0,e1;
	bool operator == (stAutoTileSide st){return e0 == st.e0 && e1 == st.e1; }
};

class iAutoGround
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual void New(LPCSTR strFile) = 0;
//	virtual BOOL Save(LPCSTR strFile) = 0;
	virtual BOOL Load(LPCSTR strFile) = 0;
	
	virtual bool IsAllNull(int x, int y) = 0;
	virtual void ResetTile(int x,int y,stAutoTile& tile) = 0;
	virtual void GetTile(int x,int y,stAutoTile& tile) = 0;
	virtual void GetBackTile(int x,int y,stAutoTile& tile) = 0;
	virtual bool IsTileChanged(int x,int y) = 0;
	
	virtual bool ChangeTile(int x,int y,LPCSTR szKind, BOOL bIgnore) = 0;
	virtual void SetAllGround(const CRect& rect, LPCSTR szKind) = 0;

	virtual LPCSTR GetExt() = 0;
	virtual LPCSTR GetConnectionFolder() = 0;
	virtual LPCSTR GetTerrainFolder() = 0;
	virtual	int	 GetTileSize() = 0;
	
	virtual HRESULT	BackFile(int xRegion, int yRegion, LPCSTR szFile) = 0;
	virtual HRESULT	RestoreFile(int xRegion, int yRegion, LPCSTR szFile) = 0;
};

_declspec(dllexport) iAutoGround* AutoMap_CreateAutoGround(int tileSize, LPCSTR szTerrainFolder,LPCSTR szConnectionFolder,LPCSTR szExt);

#endif