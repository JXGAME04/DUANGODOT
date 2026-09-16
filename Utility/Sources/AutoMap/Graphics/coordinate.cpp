#include "stdafx.h"
#include "iGraphics.h"
#include "coordinate.h"

_declspec(dllexport) void TileToGround(float x,float y,float &tx,float &ty)
{
	tx = x * c_ptMul.x;
	ty = y * c_ptMul.y;
}

_declspec(dllexport) void GroundToTile(float x,float y,float &tx,float &ty)
{
	tx = x / c_ptMul.x ;
	ty = y / c_ptMul.y ;	
}

_declspec(dllexport) void TileToDiamond(float x,float y,float &tx,float &ty)
{
	float w = 2.f;
	float h = 1.f;
//	tx = (float)(y / h - x / w + 1.f) / 2.f;
//	ty = (float)(y / h + x / w + 1.f) / 2.f;	
	tx = (y / h - x / w ) / 2.f;
	ty = (y / h + x / w ) / 2.f;
}

_declspec(dllexport) void DiamondToTile(float x,float y,float &tx,float &ty)
{
	float w = 2.f;
	float h = 1.f;
//	tx = w * (floor(y) - floor(x));
//	ty = h * (floor(y) + floor(x));
	tx = w * (y - x);
	ty = h * (y + x);
}

_declspec(dllexport) void TileToDiamondSmall(float x,float y,float &tx,float &ty)
{
	float w = 1.f;
	float h = 0.5f;
	tx = (y / h - x / w ) / 2.f;
	ty = (y / h + x / w ) / 2.f;
}

_declspec(dllexport) void DiamondSmallToTile(float x,float y,float &tx,float &ty)
{
	float w = 1.f;
	float h = 0.5f;
	tx = w * (y - x);
	ty = h * (y + x);
}

_declspec(dllexport) void TileToDiamondTile(float x,float y,float &tx,float &ty) 
{
	float fx,fy;
	TileToDiamond(x,y,fx,fy);
	DiamondToTile(fx,fy,tx,ty);
}

_declspec(dllexport) void TileToDiamondSmallTile(float x,float y,float &tx,float &ty)
{
	float fx,fy;
	TileToDiamondSmall(x,y,fx,fy);
	DiamondSmallToTile(fx,fy,tx,ty);
}

_declspec(dllexport) void TileToRegion(float x,float y,float &tx,float &ty)
{
	tx = x / c_tWidthRegion;
	ty = y / c_tHeightRegion;
}

_declspec(dllexport) void TileToRegion(int x, int y, int& tx, int& ty)
{
	tx = x / c_tWidthRegion;
	ty = y / c_tHeightRegion;
}

_declspec(dllexport) void RegionToTile(float x,float y,float &tx,float &ty)
{
	tx = x * c_tWidthRegion;
	ty = y * c_tHeightRegion;
}

_declspec(dllexport) void TileToRegion(float &tx,float &ty){TileToRegion(tx,ty,tx,ty);};
_declspec(dllexport) void TileToRegion(cPoint p,cPoint& pt){TileToRegion(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void TileToRegion(cPoint& pt){TileToRegion(pt,pt);};
_declspec(dllexport) void TileToRegion(int &tx,int &ty){TileToRegion(tx,ty,tx,ty);};

_declspec(dllexport) void RegionToTile(float &tx,float &ty){RegionToTile(tx,ty,tx,ty);};
_declspec(dllexport) void RegionToTile(cPoint p,cPoint& pt){RegionToTile(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void RegionToTile(cPoint& pt){RegionToTile(pt,pt);};

_declspec(dllexport) void TileToGround(float &tx,float &ty){TileToGround(tx,ty,tx,ty);};
_declspec(dllexport) void TileToGround(cPoint p,cPoint& pt){TileToGround(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void TileToGround(cPoint& pt){TileToGround(pt,pt);};

_declspec(dllexport) void GroundToTile(float &tx,float &ty){GroundToTile(tx,ty,tx,ty);};
_declspec(dllexport) void GroundToTile(cPoint p,cPoint& pt){GroundToTile(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void GroundToTile(cPoint& pt){GroundToTile(pt,pt);};

_declspec(dllexport) void TileToDiamondTile(float &tx,float &ty){TileToDiamondTile(tx,ty,tx,ty);};
_declspec(dllexport) void TileToDiamondTile(cPoint p,cPoint& pt){TileToDiamondTile(p.x,p.y,pt.x,pt.y);};	
_declspec(dllexport) void TileToDiamondTile(cPoint& pt){TileToDiamondTile(pt,pt);};

_declspec(dllexport) void TileToDiamondSmallTile(float &tx,float &ty){TileToDiamondSmallTile(tx,ty,tx,ty);};
_declspec(dllexport) void TileToDiamondSmallTile(cPoint p,cPoint& pt){TileToDiamondSmallTile(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void TileToDiamondSmallTile(cPoint& pt){TileToDiamondSmallTile(pt,pt);};

_declspec(dllexport) void DiamondSmallToTile(float &tx,float &ty){DiamondSmallToTile(tx,ty,tx,ty);};
_declspec(dllexport) void DiamondSmallToTile(cPoint p,cPoint& pt){DiamondSmallToTile(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void DiamondSmallToTile(cPoint& pt){DiamondSmallToTile(pt,pt);};

_declspec(dllexport) void TileToDiamondSmall(float &tx,float &ty){TileToDiamondSmall(tx,ty,tx,ty);};
_declspec(dllexport) void TileToDiamondSmall(cPoint p,cPoint& pt){TileToDiamondSmall(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void TileToDiamondSmall(cPoint& pt){TileToDiamondSmall(pt,pt);};

_declspec(dllexport) void TileToDiamond(float &tx,float &ty){TileToDiamond(tx,ty,tx,ty);};
_declspec(dllexport) void TileToDiamond(cPoint p,cPoint& pt){TileToDiamond(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void TileToDiamond(cPoint& pt){TileToDiamond(pt,pt);};

_declspec(dllexport) void DiamondToTile(float &tx,float &ty){DiamondToTile(tx,ty,tx,ty);};
_declspec(dllexport) void DiamondToTile(cPoint p,cPoint& pt){DiamondToTile(p.x,p.y,pt.x,pt.y);};
_declspec(dllexport) void DiamondToTile(cPoint& pt){DiamondToTile(pt,pt);};
