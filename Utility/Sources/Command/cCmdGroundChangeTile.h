// cCmdGroundChangeTile.h: interface for the cCmdGroundChangeTile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_cCmdGroundChangeTile_H__926898F2_0B4B_4884_967A_1F6F743087CA__INCLUDED_)
#define AFX_cCmdGroundChangeTile_H__926898F2_0B4B_4884_967A_1F6F743087CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

#include "iAutomap.h"

struct stChangeAutoTile
{
	iAutoGround* p;
	int x,y;
	stAutoTile stTile;
	HRESULT Do();
};
typedef std::list<stChangeAutoTile> listAutoTile;
typedef listAutoTile::iterator itAutoTile;

struct stChangeGroundTile
{
	iGround* p;
	int x,y;
	stTile tile;
	HRESULT Do();
};
typedef std::list<stChangeGroundTile> listGroundTile;
typedef listGroundTile::iterator itGroundTile;

class cCmdGroundChangeTile : public cCommand  
{
public:
	cCmdGroundChangeTile(iCommandMachine* p);
	virtual ~cCmdGroundChangeTile();

	stCommand_ChangeGround& GetParam();
	iAutoGround* GetAutoGround(){return GetParam().pAutoGround;}

	listAutoTile m_listAutoTileRedo;
	listAutoTile m_listAutoTileUndo;
	listGroundTile m_listGroundTileRedo;
	listGroundTile m_listGroundTileUndo;

protected:	
	HRESULT TileToResource(const stAutoTile& autoTile, stTile& res);

public:	
	eDoType Undo();
	eDoType Redo() ;
	eDoType Do() ;
	
	HRESULT	SetParam(stCommand* pCommand);
};

#endif // !defined(AFX_cCmdGroundChangeTile_H__926898F2_0B4B_4884_967A_1F6F743087CA__INCLUDED_)
