#pragma once

_declspec(dllexport) void TileToRegion(float x,float y,float &tx,float &ty);
_declspec(dllexport) void TileToRegion(float &tx,float &ty);
_declspec(dllexport) void TileToRegion(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToRegion(cPoint& pt);
_declspec(dllexport) void TileToRegion(int x,int y,int &tx,int &ty);
_declspec(dllexport) void TileToRegion(int &tx,int &ty);

_declspec(dllexport) void RegionToTile(float x,float y,float &tx,float &ty);
_declspec(dllexport) void RegionToTile(float &tx,float &ty);
_declspec(dllexport) void RegionToTile(cPoint p,cPoint& pt);
_declspec(dllexport) void RegionToTile(cPoint& pt);

_declspec(dllexport) void TileToGround(float x,float y,float &tx,float &ty);
_declspec(dllexport) void TileToGround(float &tx,float &ty);
_declspec(dllexport) void TileToGround(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToGround(cPoint& pt);

_declspec(dllexport) void GroundToTile(float x,float y,float &tx,float &ty);
_declspec(dllexport) void GroundToTile(float &tx,float &ty);
_declspec(dllexport) void GroundToTile(cPoint p,cPoint& pt);
_declspec(dllexport) void GroundToTile(cPoint& pt);

_declspec(dllexport) void TileToDiamondTile(float x,float y,float &tx,float &ty) ;	
_declspec(dllexport) void TileToDiamondTile(float &tx,float &ty);
_declspec(dllexport) void TileToDiamondTile(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToDiamondTile(cPoint& pt);

_declspec(dllexport) void TileToDiamondSmallTile(float x,float y,float &tx,float &ty) ;
_declspec(dllexport) void TileToDiamondSmallTile(float &tx,float &ty);
_declspec(dllexport) void TileToDiamondSmallTile(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToDiamondSmallTile(cPoint& pt);

_declspec(dllexport) void DiamondSmallToTile(float x,float y,float &tx,float &ty);
_declspec(dllexport) void DiamondSmallToTile(float &tx,float &ty);
_declspec(dllexport) void DiamondSmallToTile(cPoint p,cPoint& pt);
_declspec(dllexport) void DiamondSmallToTile(cPoint& pt);

_declspec(dllexport) void TileToDiamondSmall(float x,float y,float &tx,float &ty);
_declspec(dllexport) void TileToDiamondSmall(float &tx,float &ty);
_declspec(dllexport) void TileToDiamondSmall(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToDiamondSmall(cPoint& pt);

_declspec(dllexport) void TileToDiamond(float x,float y,float &tx,float &ty);
_declspec(dllexport) void TileToDiamond(float &tx,float &ty);
_declspec(dllexport) void TileToDiamond(cPoint p,cPoint& pt);
_declspec(dllexport) void TileToDiamond(cPoint& pt);

_declspec(dllexport) void DiamondToTile(float x,float y,float &tx,float &ty);
_declspec(dllexport) void DiamondToTile(float &tx,float &ty);
_declspec(dllexport) void DiamondToTile(cPoint p,cPoint& pt);
_declspec(dllexport) void DiamondToTile(cPoint& pt);
