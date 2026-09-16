// cCmdGroundChangeTile.cpp: implementation of the cCmdGroundChangeTile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdGroundChangeTile.h"

#include "iAutomap.h"
#include "iImage.h"
#include "fileop.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// struct
//////////////////////////////////////////////////////////////////////
HRESULT stChangeAutoTile::Do()
{
	p->ResetTile(x,y,stTile);
	return 0;
}

HRESULT stChangeGroundTile::Do()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_CHANGETILE;
	param.tile = tile;
	param.xPos = x;
	param.yPos = y;
	p->SetParam(param);
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdGroundChangeTile::cCmdGroundChangeTile(iCommandMachine* p) : cCommand(p)
{

}

cCmdGroundChangeTile::~cCmdGroundChangeTile()
{
	RELEASE(GetParam().pAutoGround);
}

stCommand_ChangeGround& cCmdGroundChangeTile::GetParam()
{
	return *((stCommand_ChangeGround*)GetBuffer());
}

HRESULT	cCmdGroundChangeTile::SetParam(stCommand* param)
{
	HRESULT hr = cCommand::SetParam(param);
	ASSERT(GetParam().pAutoGround);
	GetParam().pAutoGround->AddRef();
	return hr;
}


UINT StyleToFrame(eTileStyle eStyle)
{
	eTileStyle aStype[20] = 
	{
		//the first 9
		TILE_0001,TILE_0011,TILE_0010,TILE_0101,TILE_1111,TILE_1010,TILE_0100,TILE_1100,TILE_1000,
		//the next 9
		TILE_1110,TILE_1100,TILE_1101,TILE_1010,TILE_0000,TILE_0101,TILE_1011,TILE_0011,TILE_0111,
		TILE_1001,
		TILE_0110,
	};
	for (int i=0; i<20; i++)
	{
		if (aStype[i] == eStyle)
			return i;
	}
	ASSERT(0);
	return -1;
}

HRESULT cCmdGroundChangeTile::TileToResource(const stAutoTile& autoTile, stTile& tile)
{
//	ASSERT(autoTile.e0 != GROUND_NULL && autoTile.e1 != GROUND_NULL);

	CString sExt = GetAutoGround()->GetExt();
	//这里假定所有的文件都是spr的文件
	if (autoTile.e0 == autoTile.e1)
	{
		//地表图素
		CString s0 = autoTile.s0;
		CString sPath = GetAutoGround()->GetTerrainFolder();
		CString sPathName = sPath + "\\" + s0 + "." + sExt;
		
		iImage* pImage = Image_CreateImage(sPathName);
		pImage->LoadFileInfo(sPathName);
		stImageInfo info;
		pImage->GetImageInfo(info);
		tile.iDirection = 0;
		tile.idResource = 0;
		tile.iFrame = rand() % info.nFrames;
		AppGetShortPath(sPathName);
		strcpy(tile.szResource,sPathName);
		pImage->Release();
		//find spr file at direcionary 美工图素\\地表图素
	}
	else
	{
		//地表拼接图素
		CString s0 = autoTile.s0;
		CString s1 = autoTile.s1;
		CString sPath = GetAutoGround()->GetConnectionFolder();
		CString sPathName = sPath + "\\" + s0 + "_" + s1 + "." + sExt;
		
		eTileStyle eStyle = autoTile.eStyle;
		CFileFind f;
		if (!f.FindFile(sPathName))
		{
			sPathName = sPath + "\\" + s1 + "_" + s0 + "." + sExt;			
			//eStyle = (eTileStyle)(TILE_1111 - eStyle);
			if (!f.FindFile(sPathName))
			{
				CString strErr;
				strErr = "not found file " + sPathName + " !, use default spr instead";
				AfxMessageBox(strErr);
				sPathName = "system\\spr\\default_link.spr";
//				ASSERT(0);
//				return -1;
			}
		}
		else
		{
			eStyle = (eTileStyle)(TILE_1111 - eStyle);
		}
		iImage* pImage = Image_CreateImage(sPathName);
		pImage->LoadFileInfo(sPathName);
		stImageInfo info;
		pImage->GetImageInfo(info);
		tile.iFrame = StyleToFrame(eStyle);
		tile.idResource = 0;
		tile.iDirection = rand() % (info.nDirections);
		AppGetShortPath(sPathName);
		strcpy(tile.szResource,sPathName);		
		pImage->Release();
	}
	return 0;
}

eDoType cCmdGroundChangeTile::Do() 
{
	stCommand_ChangeGround& m_param = GetParam();
	CString sName;
	CString sPath,sExt;
	
	if (strlen(m_param.strFile) != 0)
	{
		AppParsePathNameExt(m_param.strFile,sPath,sName,sExt);
		ASSERT(sName.GetLength() < MAX_NAME);
	}

	iAutoGround* pGround = m_param.pAutoGround;
	int nTiles = pGround->GetTileSize() / c_nWidthTile;

	pGround->Load(m_param.strDocument);
	BOOL b;
	b = pGround->ChangeTile(m_param.xAutoGround,m_param.yAutoGround,sName,m_param.bIgnore);
	if (!b)
		return DO_NOTHING;

	for (int x = m_param.xAutoGround - MAX_AUTOTILE/2; x <= m_param.xAutoGround + MAX_AUTOTILE/2; x++)
	for (int y = m_param.yAutoGround - MAX_AUTOTILE/2; y <= m_param.yAutoGround + MAX_AUTOTILE/2; y++)
	{
		if (pGround->IsTileChanged(x,y))
		{
			//get tile
			stChangeAutoTile AutoTile;
			AutoTile.p = pGround;
			AutoTile.x = x;
			AutoTile.y = y;
			pGround->GetTile(x,y,AutoTile.stTile);
			m_listAutoTileRedo.push_back(AutoTile);
			
			stChangeAutoTile AutoTileBack;
			AutoTileBack.p = pGround;
			AutoTileBack.x = x;
			AutoTileBack.y = y;
			pGround->GetBackTile(x,y,AutoTileBack.stTile);
			m_listAutoTileUndo.push_back(AutoTileBack);

			for (int i=0; i<nTiles; i++)
			for (int j=0; j<nTiles; j++)
			{
				stChangeGroundTile tile;
				tile.p = GetGround();
				tile.x = x * nTiles + i;
				tile.y = y * nTiles + j;

				iGround::stParam param;
				param.xPos = tile.x;
				param.yPos = tile.y;
				param.mask = iGround::MASK_COMMAND;

				param.eCommand = iGround::GCOM_GETTILE;
				GetGround()->GetParam(param);
				tile.tile = param.tile;
				m_listGroundTileUndo.push_back(tile);

				if (i == 0 && j == 0)
				{
					if ( strlen(m_param.strFile) != 0 )
						TileToResource(AutoTile.stTile,tile.tile);
					else
					{
						tile.tile.iDirection = 0;
						tile.tile.iFrame = 0;
						tile.tile.idResource = 0;
						strcpy(tile.tile.szResource,"system\\spr\\RegionTileDefault.spr");
					}
				}
				else
					tile.tile.szResource[0] = 0;

				param.eCommand = iGround::GCOM_CHANGETILE;
				param.tile = tile.tile;
				GetGround()->SetParam(param);
				m_listGroundTileRedo.push_back(tile);
			}
		}
	}
//	pGround->Save(m_param.strDocument);
	
	return DO_CANUNDO;
}

eDoType cCmdGroundChangeTile::Redo() 
{
	if (m_listAutoTileRedo.size() == 0 && m_listGroundTileRedo.size() == 0)
		return DO_NOTHING;
	for_all(m_listAutoTileRedo,mem_fun_ref<HRESULT,stChangeAutoTile>(&stChangeAutoTile::Do));
	for_all(m_listGroundTileRedo,mem_fun_ref<HRESULT,stChangeGroundTile>(&stChangeGroundTile::Do));
	return DO_CANUNDO;
}

eDoType cCmdGroundChangeTile::Undo() 
{
	if (m_listAutoTileUndo.size() == 0 && m_listGroundTileUndo.size() == 0)
		return DO_NOTHING;
	for_all(m_listAutoTileUndo,mem_fun_ref<HRESULT,stChangeAutoTile>(&stChangeAutoTile::Do));
	for_all(m_listGroundTileUndo,mem_fun_ref<HRESULT,stChangeGroundTile>(&stChangeGroundTile::Do));
	return DO_CANUNDO;
}
