// cRegion.cpp: implementation of the cRegion class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cRegion.h"

#include "ctimer.h"

#include "cGraphics.h"
#include "cGround.h"
#include "cPlane.h"
#include "cLight.h"
#include "cPlaneHor.h"
#include "cPlaneVer.h"
#include "cMesh.h"
#include "cObstacle.h"
#include "template.h"
#include "Position.h"

#include "iitem.h"
#include "iImage.h"
#include "iCommand.h"
#include "fileop.h"
#include "GraphicsPublic.h"

#include "planedoodad.h"
#include "iPack2d.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const char g_szDefaultTileName[] = "system\\spr\\RegionTileDefault.spr";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
cRegion::cRegion(cGraphics* p)
{
	m_ulCount = 1;

	m_pGraphics = p;

	m_pTexture = NULL;
	m_pMesh = NULL;

	m_lModify = 0;
	m_bTextureDirty = FALSE;
	
	m_param.crColor = cColor(255,255,255,255);
	m_param.bTransparent = FALSE;
	m_param.bDraw = TRUE;
	m_param.eAnimation = ANI_NULL;
	m_param.nSpeed = 5;
	m_param.fDegree = 16.f;

	m_bUpdateTexture = FALSE;

	int n = sizeof(m_aTile);
	n = n;
}

cRegion::~cRegion()
{
	FinalCleanup();
}

////////////////////////////////////////////////////////////////////
//com
////////////////////////////////////////////////////////////////////
ULONG cRegion::AddRef() 
{
	return ++m_ulCount;
}

ULONG cRegion::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

////////////////////////////////////////////////////////////////////
//Graphics
////////////////////////////////////////////////////////////////////

cCamera* cRegion::GetCamera()
{
	return GetGraphics()->GetcCamera();
}

cGround* cRegion::GetGround()
{
	return GetGraphics()->GetcGround();
}

cScreen* cRegion::GetScreen()
{
	return GetGraphics()->GetcScreen();
}

LPDIRECT3DDEVICE8 cRegion::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

///////////////////////////////////////////////////////////////////////////
//get and set
///////////////////////////////////////////////////////////////////////////
HRESULT	cRegion::GetParam(stParam& param)
{
	if (param.mask == 0)
		param = m_param;
	return S_OK;
}

void cRegion::GetTerrainName(CString& strTerrain)
{
	strTerrain = m_strName + "\\Region.dat";
}

HRESULT	cRegion::OnCommand(stParam& param)
{
	switch (param.eCommand)
	{
	case RCOM_DELETE:
		{
			if (m_strName.IsEmpty())
				return -1;
			CString strTerrain;
			GetTerrainName(strTerrain);
			AppDeleteFolder(strTerrain);
			ClearModify(MODIFY_TERRAIN);
			InitTerrain();
			UpdateGroundTexture();
		}
		break;
	}
	return 0;
}

HRESULT	cRegion::SetParam(stParam& param, bool bRunNow )
{
	if (param.mask & MASK_COMMAND)
		OnCommand(param);
	if (param.mask & MASK_DRAW)
		m_param.bDraw = param.bDraw;
	if (param.mask & MASK_ANIMATION)
	{
		m_param.eAnimation = param.eAnimation;
		SetMeshAnimation();
	}
	if (param.mask & MASK_ANIMATIONSPEED)
	{
		if (param.nSpeed >= 1)
		{
			m_param.nSpeed = param.nSpeed;
			SetMeshAnimation();
		}
	}
	if (param.mask & MASK_ANIMATIONDEGREE)
	{
		if (param.fDegree > 1.f)
		{
			m_param.fDegree = param.fDegree;
			SetMeshAnimation();
		}
	}

	if (param.mask & MASK_TRANSPARENT)
	{
		if (m_param.bTransparent != param.bTransparent)
		{
			m_param.bTransparent = param.bTransparent;
			if (m_pTexture)
			{
				m_pTexture->ChangeFormat(GetFormat());
				RepaintTerrain();
			}
		}
		#pragma message (ATTENTION("recreate the texture"))	
	}
	if (param.mask & MASK_MAPEDIT)
	{
		BitOP(m_param.lMapeditEnum,param.eOPMapedit,param.lMapeditEnum);
		m_pMesh->SetMapeditParam(m_param.lMapeditEnum);
	}
	if (param.mask & MASK_COLOR)
		m_param.crColor = param.crColor;

	if (param.mask != MASK_MAPEDIT)
		AddModify(MODIFY_PROPERTY);


	return S_OK;
}

D3DFORMAT cRegion::GetFormat()
{
	D3DFORMAT e;
	if (m_param.bTransparent == FALSE)
		e = D3DFMT_A4R4G4B4;
//		e = D3DFMT_R5G6B5;
	else
		e = D3DFMT_A4R4G4B4;
	return e;
}

void cRegion::ClearModify()
{
	m_lModify = 0;
}

BOOL cRegion::IsModify(long mask)
{
	return m_lModify & mask;
};

void cRegion::ClearModify(long mask)
{
	m_lModify &= (~mask);
}

void cRegion::AddModify(long mask)
{
	m_lModify |= mask;
}

/////////////////////////////////////////////////////////////////////////
//tile
/////////////////////////////////////////////////////////////////////////
stTile* cRegion::GetTile(int x, int y)
{
	if (x<0 || x>c_tWidthRegion || y<0 || y>c_tHeightRegion)
	{
		ASSERT(0);
		return NULL;
	}
	return &m_aTile[x][y];
}

HRESULT cRegion::SetTileFileName(LPCSTR szFile)
{
	LoadTerrain(szFile);
	if (m_pTexture)
	{
		RepaintTerrain();
	}
	AddModify(MODIFY_TERRAIN);
	SaveTerrain();
	return S_OK;
}

HRESULT cRegion::GetTileFileName(CString&  strFile)
{
	SaveTerrain();
	strFile = m_strName + "\\Region.dat";
	return S_OK;
}

HRESULT cRegion::Fill(LPCSTR szSprFile)
{
	int nFrames ;
	const char* szFile = NULL;

	if (strlen(szSprFile) != 0)
	{
		szFile = szSprFile;
		//just to get the frames
		iTexture* pTexture = GetGraphics()->FindSprResource(szSprFile);
		nFrames = pTexture->GetFrames();
		pTexture->Release();
	}
	else
	{
		szFile = g_szDefaultTileName;
		nFrames = 1;
	}

	CString strFile = szSprFile;
	int large = 1;
	if (!strFile.IsEmpty())
	{
		iImage* pImage = Image_CreateImage(strFile);
		pImage->LoadFile(strFile);
		stFrameInfo info;
		pImage->GetFrameInfo(info);
		if (info.width == 64 && info.height == 64)
		{
			large = 1;
		}
		else if (info.width == 128 && info.height == 128)
		{
			large = 2;
		}
		else
		{
			CString strMsg;
			strMsg.Format("file %s 不是有效的地表文件,尺寸不正确！",strFile);
			AfxMessageBox(strMsg);
		}
		pImage->Release();
	}
	
	for (int i=0; i<c_tWidthRegion; i++)
	for (int j=0; j<c_tHeightRegion; j++)	
	{
		stTile& tile = *GetTile(i,j);
		if ((i % (2*large) == 0 ) && (j % (2*large) == 0))
		{
			tile.iFrame = rand() % nFrames;
			tile.iDirection = 0;
			strcpy(tile.szResource,szFile);
			tile.idResource = 0;
			//width and height get by the res!
		}
		else
		{
			tile.szResource[0] = 0;			
			tile.iFrame = 0;
			tile.iDirection = 0;
			tile.idResource = 0;
		}
	}

//所有的东西都及时存储
	AddModify(MODIFY_TERRAIN);

	return S_OK;
}

HRESULT cRegion::GetTile(int x, int y, stTile& Tile)
{
	int tx = x - c_tWidthRegion * m_x;
	int ty = y - c_tWidthRegion * m_y;

	ASSERT(tx >=0 && tx < c_tWidthRegion && ty >=0 && ty < c_tHeightRegion);
	Tile = *GetTile(tx,ty);

	return 0;
}

HRESULT cRegion::SetTileVertex(int x, int y, stVertexGround v)
{
	int tx = x - c_tWidthRegion * m_x;
	int ty = y - c_tWidthRegion * m_y;

	ASSERT(tx >=0 && tx < c_tWidthRegion && ty >=0 && ty < c_tHeightRegion);
	GetTile(tx,ty)->v = v;

	AddModify(MODIFY_TERRAIN);

	ReCreateMesh();

	return 0;
}

HRESULT cRegion::SetTile(int x, int y, const stTile& Tile)
{
	int tx = x - c_tWidthRegion * m_x;
	int ty = y - c_tWidthRegion * m_y;

	ASSERT(tx >=0 && tx < c_tWidthRegion && ty >=0 && ty < c_tHeightRegion);
	*GetTile(tx,ty) = Tile;
	AddModify(MODIFY_TERRAIN);

	if (m_pTexture == NULL)
		return 0;
	
	if (!GetGround()->IsUpdateRegionTexture())
	{
		m_bTextureDirty = TRUE;
		return 0;
	}
	RepaintRectTile(CRect(tx,ty,tx+1,ty+1));

	return 0;
}

/////////////////////////////////////////////////////////////////////////
//load and save
/////////////////////////////////////////////////////////////////////////
void cRegion::SetRegionFolder(LPCSTR szFolder)
{
	m_strName.Format("%s\\%03d_%03d",szFolder,m_x,m_y);
}

void cRegion::GetRegionFolder(CString & strFolder)
{
	strFolder = m_strName;
}

HRESULT cRegion::LoadForMap(LPCSTR szFolder)
{
	LoadObstacle();	
	LoadClientOnlyNpc();
	LoadPlaneHor();
	LoadPlaneVer();

	CPoint ptRegion(m_x,m_y);
	for_all(m_listPlane,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
//	for_all(m_listLight,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listObstacle,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listClientNpc,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));

	for_all(m_listPlane,mem_fun<HRESULT ,cPlane>(cPlane::FillVertexInfoNotCallGraphics) );

	ClearModify();

	return S_OK;
}

HRESULT cRegion::Load(LPCSTR szFolder)
{
	cTimerFunction timer("cRegion::load");
	
	SetRegionFolder(szFolder);
	
	BOOL bBadFile = FALSE;

	LoadProperty();
	LoadTerrain();

	//为了格式的兼容
	if (1)
	{
		//新的格式
		if (LoadPlaneVer() == 1)
			bBadFile = TRUE;
		if (LoadPlaneHor() == 1)
			bBadFile = TRUE;
	}

	LoadLight();	
	LoadObstacle();	
	LoadClientOnlyNpc();
	LoadPlaneVerBelowGround();

	CPoint ptRegion(m_x,m_y);
	for_all(m_listPlane,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listPlaneVerBelowGround,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listLight,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listObstacle,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));
	for_all(m_listClientNpc,bind2nd(mem_fun1<bool,cGroundObj,CPoint>(&cGroundObj::SetRegion),ptRegion));

	for_all(m_listPlane,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(cPlane::SetInGround),true));
	for_all(m_listPlaneVerBelowGround,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(cPlane::SetInGround),true));
	for_all(m_listLight,bind2nd(mem_fun1<BOOL,cLight,BOOL>(cLight::SetInGround),true));
	for_all(m_listObstacle,bind2nd(mem_fun1<BOOL,cObstacle,BOOL>(cObstacle::SetInGround),true));
	for_all(m_listClientNpc,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(cPlane::SetInGround),true));

#pragma message (ATTENTION("去掉了npc等的部分"))
//	LoadDynamic("MAIN");
	for(itcPlane it = m_listPlane.begin(); it != m_listPlane.end(); ++it)
	{
		cPlaneHor* p = dynamic_cast<cPlaneHor*>(*it);
		if (p != NULL)
			AddHor(p,false);
	}

	ClearModify();
	if (bBadFile)
	{
		AddModify(MODIFY_VER|MODIFY_HORBELOWVER);
	}

	return S_OK;
}

void cRegion::CreateFolder(CString& s)
{
	CFileFind f;
	if (!f.FindFile(s))
		VERIFY(CreateDirectory(s,NULL));
}

HRESULT cRegion::Save(LPCSTR szFolder)
{
	SetRegionFolder(szFolder);

	SavePlaneVer();
	SavePlaneHor();
//	SavePlaneHorAboveVer();
	SaveLight();	
	SaveObstacle();	
	SaveTerrain();
	SaveProperty();
//	SaveLinkHor();
	SaveClientOnlyNpc();
	SavePlaneVerBelowGround();

	ClearModify();

	return S_OK;
}

/*
HRESULT cRegion::LoadLinkHor(cPlaneHorList& plist)
{
#pragma message (ATTENTION("这个比较复杂，需要其他9个region的信息才行"))
	CString strFile = m_strName + "\\PlaneHorLink.ini";
	T_LoadPlaneList<cPlaneHorList,cPlaneHor>(plist,strFile,GetGraphics());
	for (itcPlaneHor it = plist.begin(); it	!= plist.end(); ++it)
	{
		(*it)->SetGroupID(0);
	}
	return S_OK;
}

HRESULT cRegion::SaveLinkHor()
{
	CString strFile = m_strName + "\\PlaneHorLink.ini";
	return T_SaveList(m_listLinkPlaneHor,strFile);
}
*/

void cRegion::InitTile()
{
	for (int x = 0; x < c_tWidthRegion+1; x++)
	for (int y = 0; y < c_tHeightRegion+1; y++)
	{
		stTile& tile = *(GetTile(x,y));
		tile.v.sx = x;
		tile.v.sy = y;
		tile.v.sz = 0;
		TileToGround(tile.v.sx,tile.v.sy);
		tile.v.color = -1;
	}
}

HRESULT cRegion::InitTerrain()
{
	Fill("");
	InitTile();
	CreateMesh();
	return S_OK;
}

HRESULT cRegion::LoadTerrain()
{
	cTimerFunction timer("cRegion::loadterrain");
	iPack2D* pPack = GetGround()->GetPack(PACK_GROUND);
	if (pPack != NULL)
	{
		void* pOff = pPack->Locate(m_x,m_y);
		if (pOff != NULL)
		{
			stTerrainTile m_aGroundTile[c_tWidthRegion+1][c_tHeightRegion+1];
			
			memcpy(m_aGroundTile,pOff,sizeof(stTerrainTile) * (c_tWidthRegion+1)*(c_tHeightRegion+1));
			for (int i=0; i<c_tWidthRegion+1; i++)
			for (int j=0; j<c_tHeightRegion+1; j++)
			{
//				m_aTile[i][j].v.sx = m_aGroundTile[i][j].ptPos.x;
//				m_aTile[i][j].v.sy = m_aGroundTile[i][j].ptPos.y;
//				m_aTile[i][j].v.sz = m_aGroundTile[i][j].ptPos.z;
				m_aTile[i][j].iFrame = m_aGroundTile[i][j].iFrame;
				m_aTile[i][j].iDirection = m_aGroundTile[i][j].iDirection;
				m_aTile[i][j].idResource = m_aGroundTile[i][j].idResource;
				if (m_aTile[i][j].idResource != -1)
				{
					sprintf(m_aTile[i][j].szResource,"%s\\%04d.spr",FOLDER_SPR_TERRAIN,m_aTile[i][j].idResource);
				}
				else
					m_aTile[i][j].szResource[0] = 0;
			}

//			stTile& tile = *(GetTile(0,0));
//			if (tile.v.sx > 10.f || tile.v.sy > 10.f)
			{
				InitTile();
			}
			CreateMesh();
		}
		else
			InitTerrain();
		return S_OK;
	}
	CString file;
	file = m_strName + "\\" + FILE_TERRAIN;
	return LoadTerrain(file);
}

HRESULT cRegion::LoadTerrain(LPCSTR file)
{
	CFile f;
	if (f.Open(file,CFile::modeRead))
	{
		cColor m_crRegion;
		f.Read(&m_crRegion,sizeof(m_crRegion));
		f.Read(m_aTile,sizeof(m_aTile));
/*		stTile* pTile = GetTile(0,0);
		for (int i=0; i<(c_tWidthRegion+1)*(c_tHeightRegion+1); i++)
		{
			f.Read(pTile,sizeof(stTile));
			pTile++;
		}
*/		stTile& tile = *(GetTile(0,0));
		if (tile.v.sx > 10.f || tile.v.sy > 10.f)
		{
			InitTile();
			AddModify(MODIFY_TERRAIN);
		}
/*
		for (int x = 0; x < c_tWidthRegion+1; x++)
		for (int y = 0; y < c_tHeightRegion+1; y++)
		{
			stTile& tile = *(GetTile(x,y));
			tile.v.sx = m_x * c_tWidthRegion + x;
			tile.v.sy = m_y * c_tHeightRegion + y;
			tile.v.sz = 0;
			TileToGround(tile.v.sx,tile.v.sy);
		}
//*/
		CreateMesh();
	}
	else 
	{
		InitTerrain();
	}
	return S_OK;
}

HRESULT cRegion::LoadProperty()
{
	cTimerFunction timer("cRegion::loadproperty");

	iPack2D* pPack = GetGround()->GetPack(PACK_REGION);
	if (pPack != NULL)
	{
		stParam* pParam = (stParam* )(pPack->Locate(m_x,m_y));
		if (pParam != NULL)
			memcpy(&m_param,pParam,sizeof(stParam));
		return S_OK;
	}

	CString file = m_strName + "\\" + FILE_PROPERTY;
	iItem* pItem = Common_CreateItem(file,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return S_OK;
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"draw",m_param.bDraw);
	GetItemValue(pItem,"color",m_param.crColor);
	GetItemValue(pItem,"transparent",m_param.bTransparent);
	GetItemValue(pItem,"animation",cast_int(m_param.eAnimation));
	GetItemValue(pItem,"animationspeed",m_param.nSpeed);
	GetItemValue(pItem,"animationdegree",m_param.fDegree);
	return S_OK;
}

HRESULT cRegion::SaveTerrain()
{
	CString file;
	file = m_strName + "\\" + FILE_TERRAIN;
	return SaveTerrain(file);
}

HRESULT cRegion::SaveTerrain(LPCSTR file)
{
	if (!IsModify(MODIFY_TERRAIN))
		return FALSE;
	ClearModify(MODIFY_TERRAIN);

	CreateFolder(m_strName);

	CFile f;
	VERIFY(AppOpenFile(f,file,CFile::modeCreate | CFile::modeWrite,"region"));
	cColor m_crRegion;
	f.Write(&m_crRegion,sizeof(m_crRegion));

	stTile* pTile = GetTile(0,0);
	for (int i=0; i<(c_tWidthRegion+1)*(c_tHeightRegion+1); i++)
	{
		f.Write(pTile,sizeof(stTile));
		pTile++;
	}
	return S_OK;
}

HRESULT cRegion::SaveProperty()
{
	if (!IsModify(MODIFY_PROPERTY))
		return FALSE;
	ClearModify(MODIFY_PROPERTY);

	CreateFolder(m_strName);

	CString file = m_strName + "\\" + FILE_PROPERTY;
	iItem* pItem = Common_CreateItem(file,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	if (pItem != NULL)
	{
		//save it!
		SetItemValue(pItem,"draw",m_param.bDraw);
		SetItemValue(pItem,"color",m_param.crColor);
		SetItemValue(pItem,"transparent",m_param.bTransparent);
		SetItemValue(pItem,"animation",cast_int(m_param.eAnimation));
		SetItemValue(pItem,"animationspeed",m_param.nSpeed);
		SetItemValue(pItem,"animationdegree",m_param.fDegree);
		pItem->Save();
	}
	return S_OK;
}

HRESULT cRegion::LoadPlaneVer()
{
	cTimerFunction timer("cRegion::LoadPlaneVer");
	CString strFile = m_strName + "\\" + FILE_PLANEVER;
	iPack2D* pPack = GetGround()->GetPack(PACK_PLANEVER);
	HRESULT hr = LoadPlane(strFile,pPack);
	return hr;
}

HRESULT cRegion::LoadPlaneVerBelowGround()
{
	cTimerFunction timer("cRegion::LoadPlaneVer");
	CString strFile = m_strName + "\\" + FILE_PLANEVERBELOWGROUND;
	iPack2D* pPack = GetGround()->GetPack(PACK_PLANEVERBELOWGROUND);
	HRESULT hr = LoadPlane(m_listPlaneVerBelowGround,strFile,pPack);
	return hr;
}

HRESULT cRegion::LoadPlaneHor()
{
	cTimerFunction timer("cRegion::LoadPlaneHor");
	CString strFile = m_strName + "\\" + FILE_PLANEHORBELOWVER;
	iPack2D* pPack = GetGround()->GetPack(PACK_PLANEHOR);
	HRESULT hr = LoadPlane(strFile,pPack);
	return hr;
}

/*
HRESULT cRegion::LoadPlaneHorAboveVer()
{
	CString strFile = m_strName + "\\PlaneHorAboveVer.ini";
	return LoadPlane(strFile);
	return -1;
}
*/

HRESULT cRegion::LoadPlane(cPlaneList& plist,LPCSTR file,iPack2D* pPack)
{
	if (pPack != NULL)
	{
		void* pOffset = pPack->Locate(m_x,m_y);
		if (pOffset == NULL)
			return S_OK;
		return T_LoadPackList<cPlaneList,cPlane>(plist,GetGraphics(),pOffset);
	}
	return T_LoadPlaneList<cPlaneList,cPlane>(plist,file,GetGraphics());
}

HRESULT cRegion::LoadPlane(LPCSTR file,iPack2D* pPack)
{
	return LoadPlane(m_listPlane,file,pPack);
}

HRESULT cRegion::LoadLight()
{
	CString file = m_strName + "\\" + FILE_LIGHT;
	iPack2D* pPack = GetGround()->GetPack(PACK_LIGHT);
	if (pPack != NULL)
	{
		void* pOffset = pPack->Locate(m_x,m_y);
		if (pOffset == NULL)
			return S_OK;
		return T_LoadPackList<cLightList,cLight>(m_listLight,GetGraphics(),pOffset);
	}
	T_LoadList<cLightList,cLight>(m_listLight,file,GetGraphics());
	return S_OK;
}

HRESULT cRegion::LoadObstacle()
{
//	if (GetGround()->IsPlaying())
//		return S_OK;
	
	CString file = m_strName + "\\" + FILE_OBSTACLE;
	
	iPack2D* pPack = GetGround()->GetPack(PACK_OBSTACLE);
	if (pPack != NULL)
	{
		void* pOffset = pPack->Locate(m_x,m_y);
		if (pOffset == NULL)
			return S_OK;
		return T_LoadPackList<cObstacleList,cObstacle>(m_listObstacle,GetGraphics(),pOffset);
	}
	T_LoadList<cObstacleList,cObstacle>(m_listObstacle,file,GetGraphics());
	return S_OK;
/*
	for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		p->ClearModify();
	}
*/
	return S_OK;
}

HRESULT cRegion::LoadClientOnlyNpc()
{
	if (GetGround()->IsPlaying())
		return S_OK;
	CString file;
	iPack2D* pPack;
	file = m_strName + "\\" + FILE_NPC;
	pPack = GetGround()->GetPack(PACK_NPC);
	LoadPlane(m_listClientNpc,file,pPack);
	file = m_strName + "\\" + FILE_BOX;
	pPack = GetGround()->GetPack(PACK_BOX);
	LoadPlane(m_listClientNpc,file,pPack);
	file = m_strName + "\\" + FILE_TRAP;
	pPack = GetGround()->GetPack(PACK_TRAP);
	LoadPlane(m_listClientNpc,file,pPack);
/*
	file = m_strName + "\\" + FILE_CLIENTONLYBOX;
	pPack = GetGround()->GetPack(PACK_CLIENTONLYBOX);
	LoadPlane(m_listClientNpc,file,pPack);
	file = m_strName + "\\" + FILE_CLIENTONLYTRAP;
	pPack = GetGround()->GetPack(PACK_CLIENTONLYTRAP);
	LoadPlane(m_listClientNpc,file,pPack);
	file = m_strName + "\\" + FILE_CLIENTONLYNPC;
	pPack = GetGround()->GetPack(PACK_CLIENTONLYNPC);
	LoadPlane(m_listClientNpc,file,pPack);
*/
	return S_OK;
}

HRESULT cRegion::SavePlaneObstacle(LPCSTR szFile, cPlaneList& rlist)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_SAVE);
	wrap_ptr<iItem> ptr(pItem);
	int num = 0;
	for (itcPlane it = rlist.begin(); it != rlist.end(); ++it)
	{
		cPlane* p = (*it);
		p->SaveObstalce(pItem,num);
	}
	if (num == 0)
		pItem->DeleteFile();
	else
	{
		SetItemValue(pItem,"elementnum",num);
		pItem->Save();
	}
	return S_OK;
}

HRESULT cRegion::SavePlaneVer()
{	
//	if (!IsModify(MODIFY_VER))
//		return FALSE;
	if (!IsModify(MODIFY_VER) && !IsModify(MODIFY_HORBELOWVER) )
		return FALSE;
	ClearModify(MODIFY_VER);
	CreateFolder(m_strName);

	CString strFile = m_strName + "\\" + FILE_PLANEVER;
	cPlaneList rlist;
	insert_if(m_listPlane.begin(),m_listPlane.end(),
		rlist,bind2nd(mem_fun1<BOOL,cPlane,SortEnum>(&cPlane::IsSortType),SORT_VER));
	HRESULT hr = SavePlane(strFile,rlist);
	
	CString strObs = m_strName + "\\" + FILE_PLANEVEROBSTACLE;
	hr = SavePlaneObstacle(strObs,rlist);
	return hr;
}

HRESULT cRegion::SavePlaneVerBelowGround()
{
	if (!IsModify(MODFIY_VERBELOWGROUND))
		return FALSE;
	ClearModify(MODFIY_VERBELOWGROUND);
	CreateFolder(m_strName);

	CString strFile = m_strName + "\\" + FILE_PLANEVERBELOWGROUND;
	cPlaneList rlist;
	HRESULT hr = SavePlane(strFile,m_listPlaneVerBelowGround);
	
	return hr;
}

HRESULT cRegion::SavePlaneHor()
{
//	if (!IsModify(MODIFY_HORBELOWVER))
//		return FALSE;
	if (!IsModify(MODIFY_VER) && !IsModify(MODIFY_HORBELOWVER) )
		return FALSE;
	ClearModify(MODIFY_HORBELOWVER);
	CreateFolder(m_strName);

	CString strFile = m_strName + "\\" + FILE_PLANEHORBELOWVER;
	cPlaneList rlist;
	insert_if(m_listPlane.begin(),m_listPlane.end(),
		rlist,bind2nd(mem_fun1<BOOL,cPlane,SortEnum>(&cPlane::IsSortType),SORT_HORGROUND));
	insert_if(m_listPlane.begin(),m_listPlane.end(),
		rlist,bind2nd(mem_fun1<BOOL,cPlane,SortEnum>(&cPlane::IsSortType),SORT_HORBELOWVER));
	HRESULT hr = SavePlane(strFile,rlist);

	CString strObs = m_strName + "\\" + FILE_PLANEHORBELOWVEROBSTACLE;
	hr = SavePlaneObstacle(strObs,rlist);
	return hr;
}

/*
HRESULT cRegion::SavePlaneHorAboveVer()
{
	if (!IsModify(MODIFY_HORABOVEVER))
		return FALSE;
	ClearModify(MODIFY_HORABOVEVER);
	CreateFolder(m_strName);

	cPlaneList rlist;
	insert_if(m_listPlane.begin(),m_listPlane.end(),
		rlist,bind2nd(mem_fun1<BOOL,cPlane,SortEnum>(&cPlane::IsSortType),SORT_HORABOVEVER));

	CString strFile = m_strName + "\\PlaneHorAboveVer.ini";
	return SavePlane(strFile,rlist);
}
*/

HRESULT cRegion::SavePlane(LPCSTR file, cPlaneList& rlist)
{
	CreateFolder(m_strName);
	return T_SaveList(rlist,file);
}

HRESULT cRegion::SaveLight()
{
	if (!IsModify(MODIFY_LIGHT))
	{
		for (itcLight it = m_listLight.begin(); it != m_listLight.end(); ++it)
		{
			cLight* p = (*it);
			if (p->IsModify())
				break;
		}
		if (it == m_listLight.end())
			return FALSE;
	}
	ClearModify(MODIFY_LIGHT);
	CreateFolder(m_strName);
	{
		for (itcLight it = m_listLight.begin(); it != m_listLight.end(); ++it)
		{
			cLight* p = (*it);
			p->ClearModify();
		}
	}

	CString file = m_strName + "\\" + FILE_LIGHT;
	return T_SaveList(m_listLight,file);
}

HRESULT cRegion::SaveObstacle()
{
	if (!IsModify(MODIFY_OBSTACLE))
	{
		for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
		{
			cObstacle* p = (*it);
			if (p->IsModify())
				break;
		}
		if (it == m_listObstacle.end())
			return FALSE;
	}
	ClearModify(MODIFY_OBSTACLE);
	{
		for (itcObstacle it = m_listObstacle.begin(); it != m_listObstacle.end(); ++it)
		{
			cObstacle* p = (*it);
			p->ClearModify();
		}
	}
	CreateFolder(m_strName);
	CString file = m_strName + "\\" + FILE_OBSTACLE;
	return T_SaveList(m_listObstacle,file);
}

HRESULT cRegion::SaveClientOnlyNpc()
{
	if (!IsModify(MODIFY_CLIENTNPC))
		return FALSE;
	ClearModify(MODIFY_CLIENTNPC);
	CreateFolder(m_strName);

	cPlaneList listnpc;
	cPlaneList listbox;
	cPlaneList listtrap;
	cPlaneList listclientonlynpc,listclientonlybox,listclientonlytrap;
	for (itcPlane it = m_listClientNpc.begin(); it != m_listClientNpc.end(); ++it)
	{
		cPlane* p = (*it);
		switch(p->GetDynamicType())
		{
		case DYNAMIC_NPC:
			listnpc.push_back(p);
			break;
		case DYNAMIC_BOX:
			listbox.push_back(p);
			break;
		case DYNAMIC_TRAP:
			listtrap.push_back(p);
			break;
		case DYNAMIC_CLIENTONLYNPC:
			listclientonlynpc.push_back(p);
			break;
		case DYNAMIC_CLIENTONLYBOX:
			listclientonlybox.push_back(p);
			break;
		case DYNAMIC_CLIENTONLYTRAP:
			listclientonlytrap.push_back(p);
			break;
		}
	}
	CString file;
	file = m_strName + "\\" + FILE_NPC;
	T_SaveList(listnpc,file);
	file = m_strName + "\\" + FILE_BOX;
	T_SaveList(listbox,file);
	file = m_strName + "\\" + FILE_TRAP;
	T_SaveList(listtrap,file);
/*
	file = m_strName + "\\" + FILE_CLIENTONLYNPC;
	T_SaveList(listclientonlynpc,file);
	file = m_strName + "\\" + FILE_CLIENTONLYBOX;
	T_SaveList(listclientonlybox,file);
	file = m_strName + "\\" + FILE_CLIENTONLYTRAP;
	T_SaveList(listclientonlytrap,file);
*/
	return S_OK;
}
/////////////////////////////////////////////////////////////////////////
//mesh
/////////////////////////////////////////////////////////////////////////

const int c_wRegionGroundTile = c_nWidthRegion * GROUND_DIVIDE;
const int c_hRegionGroundTile = c_nHeightRegion * GROUND_DIVIDE;

void cRegion::SetMeshAnimation()
{
	if (m_pMesh == NULL)
		return;

	switch (m_param.eAnimation)
	{
	case ANI_NULL:
		m_pMesh->SetAnimation(cMesh::ANI_BLEND_NULL);	
		break;
	case ANI_BLEND:
		m_pMesh->SetAnimation(cMesh::ANI_BLEND_SLIGHT);	
		m_pMesh->SetAnimationSpeed(m_param.nSpeed);
		m_pMesh->SetAnimationDegree(m_param.fDegree);
		break;
	}
}

void cRegion::ReCreateMesh()
{
	RELEASE(m_pMesh);
	CreateMesh();
}

void cRegion::CreateMesh()
{
	if (m_pMesh != NULL)
		return;
	GetGraphics()->CreateGraphicsElement(GT_MESH,&m_pMesh);
	stVertexGround aV[(c_wRegionGroundTile+1)*(c_hRegionGroundTile+1)];

	int offx = c_nWidthRegion * m_x;
	int offy = c_nHeightRegion * m_y;

	for (int x=0; x<c_wRegionGroundTile+1; x++)
	for (int y=0; y<c_hRegionGroundTile+1; y++)
	{
		int i = x*(c_hRegionGroundTile+1)+y;
//		aV[i] = GetTile(x,y)->v;
		if (y & 1)
		{
			stTile& tile1 = *GetTile(x,(y-1)/2);
			stTile& tile2 = *GetTile(x,(y+1)/2);
			aV[i].sx = (tile1.v.sx + tile2.v.sx)/2 + offx;			
			aV[i].sy = (tile1.v.sy + tile2.v.sy)/2 + offy;			
			aV[i].sz = (tile1.v.sz + tile2.v.sz)/2;			
		}
		else
		{
			stTile& tile = *GetTile(x,y/2);
			aV[i] = tile.v;
			aV[i].sx += offx;
			aV[i].sy += offy;
		}
		if (aV[i].sz > 0)
		{
			aV[i].sz = aV[i].sz;
		}
		aV[i].color = -1;

//		aV[i].sx = offx + (float)x / GROUND_DIVIDE;
//		aV[i].sy = offy + (float)y / GROUND_DIVIDE;
//		aV[i].sz = 0;

		aV[i].tu = (float)x / (float)c_wRegionGroundTile;
		aV[i].tv = (float)y / (float)c_hRegionGroundTile;
	}
	BOOL b = m_pMesh->CreateHor(aV,c_wRegionGroundTile,c_hRegionGroundTile);
	m_pMesh->SetMapeditParam(m_param.lMapeditEnum);
	SetMeshAnimation();
}

/////////////////////////////////////////////////////////////////////////
//texture
/////////////////////////////////////////////////////////////////////////

void cRegion::SetResource(stResourceA& pos)
{
	pos.stDevice.format = GetFormat();
	pos.stDevice.eUsage = USAGE_REGION;
	pos.stDevice.eMedia = MEDIA_MEMORY;
	pos.stDevice.pool = D3DPOOL_SYSTEMMEM;
//	pos.stRes.idResource = m_x * 1000 + m_y + 1000000;
	sprintf(pos.stRes.szResource,"%03d_%03d",m_x,m_y);
	pos.stRes.idResource = 0;
	pos.stRes.iFrame = 0;
}

void cRegion::ChangeSubTexture(int x, int y, bool bUpdate)
{
	ASSERT(m_pTexture != NULL);
	stTile& tile = *GetTile(x,y);
	if (strlen(tile.szResource) == 0)
		return;

	if (!GetGraphics()->IsShow(iGraphics::GS_TERRAIN) && 
		strcmp(g_szDefaultTileName,tile.szResource) == 0)
		return;

	stResourceA pos;
	pos.stDevice.eUsage = USAGE_STATIC;
	pos.stDevice.eMedia = MEDIA_SPR;
	pos.stRes.idResource = tile.idResource;
	strcpy(pos.stRes.szResource, tile.szResource);

#ifdef GET_FILELIST
	g_mapFile[tile.szResource] = 0;
#endif

	iTexture* pTex = GetGraphics()->FindTexture(pos,true);
	if (pTex == NULL)
		return;
	ASSERT(pTex);
	int nFrames = pTex->GetFrames();
	int nDirections = pTex->GetDirections();
	int iFrame = tile.iDirection * nFrames / nDirections + tile.iFrame;
	ULONG width,height;
	pTex->GetImageSize(iFrame,width,height);
	ASSERT(iFrame < nFrames);
	cRect rc(0,0,width,height);

	stRender render;
	render.pTexDes = m_pTexture;
	render.ptDes = CPoint(x*c_nWidthTile, y*c_nHeightTile);
	render.nFrameDes = 0;

	render.pTexSrc = pTex;
	render.rcSrc = CRect(rc.l,rc.t,rc.r,rc.b);
	render.nFrameSrc = iFrame;
	render.format = GetFormat();
	render.eOp = Render_Copy;

	render.pTexSrc->Render(render);

	if (bUpdate)
	{
		CRect rcDirty = CRect(x*c_nWidthTile,y*c_nHeightTile,
			x*c_nWidthTile+width,y*c_nHeightTile+height);
		m_pTexture->Update(&rcDirty);
	}

	pTex->Release();
}

HRESULT cRegion::BeginPainting()
{
	cTimerFunction timer("cRegion::beginpainting");
	FindTerrainTexture();
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::BeginPainting));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::BeginPainting));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::BeginPainting));
	return S_OK;
}

HRESULT cRegion::EndPainting()
{	
	ClearTerrainTexture();
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::EndPainting));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::EndPainting));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::EndPainting));
	return S_OK;
}

void cRegion::FindTerrainTexture()
{
	if (m_x == 50 &&  m_y == 55)
	{
		int n;
		n = 1;
	}
	if (m_pTexture == NULL)
	{
		stResourceA pos;
		SetResource(pos);
		pos.stDevice.eMedia = MEDIA_MEMORY;
		//可能找不到
		m_pTexture = GetGraphics()->FindTexture(pos,false);
	}
	if (m_pTexture != NULL && !m_pTexture->IsValid())
	{
		RELEASE(m_pTexture);
		CString str;
		str.Format("region %d,%d release texture \n",m_x, m_y);
		OutputDebugString(str);
	}
}

#include "cTimer.h"
void cRegion::PrepareTerrainTexture()
{
//	cTimerFunction timer("PrepareTerrainTexture");
	if (m_x == 50 &&  m_y == 55)
	{
		int n;
		n = 1;
	}
	if (m_pTexture == NULL)
	{
		CreateTerrainTexture();
		UpdateTerrainTexture();
	}
	else
	{
		//UpdateGroundTexture();
	}
}

void cRegion::CreateTerrainTexture()
{
	if (m_pTexture == NULL)
	{
		m_bUpdateTexture = FALSE;

		stResourceA pos;
		SetResource(pos);
		pos.stDevice.eMedia = MEDIA_MEMORY;
		iTexture* pTexture = GetGraphics()->CreateiTexture(pos, c_nWidthRegionTexture, c_nHeightRegionTexture);
		GetGraphics()->AddTexture(pos,pTexture);
		m_pTexture = pTexture;
/*
		for (int i=0; i<3; i++)
		for (int j=0; j<3; j++)
			m_bUpdateHorLink[i][j] = FALSE;
*/
		m_bUpdateTexture = FALSE;
	}
}

void cRegion::RepaintTerrain()
{
	if (m_pTexture != NULL)
	{
		UpdateGroundTexture();
	}
}

void cRegion::UpdateTerrainTexture()
{
	cTimerFunction timer("UpdateTerrainTexture");
	m_pTexture->Clear();
	for (int x=0; x<c_tWidthRegion; x++)
	for (int y=0; y<c_tHeightRegion; y++)
	{
		ChangeSubTexture(x,y,false);
	}
}

void cRegion::UpdateGroundTexture()
{
	//cTimerFunction timer("UpdateGroundTexture");
	UpdateTerrainTexture();
	UpdateHorTexture();
}

void cRegion::UpdateHorTexture()
{
	cTimerFunction timer("UpdateHorTexture");
	cRect rc(0,0,0,0);

	cPlaneHorList* aList[2];
	aList[0] = &m_listPlaneHorGround;
	aList[1] = &m_listPlaneHorBelowVer;
	for (int i=0; i<2; i++)
	{
		for (itcPlaneHor it = aList[i]->begin(); it != aList[i]->end();  ++it)
		{
			cPlaneHor* p = (*it);
			DrawHor(p,rc);
		}
	}
	m_pTexture->Update(NULL);
	m_bUpdateTexture = TRUE;
}

void cRegion::RepaintRectTile(CRect& rc)
{
	for (int x = rc.left; x < rc.right; x ++)
	for (int y = rc.top; y < rc.bottom; y ++)
	{
		ChangeSubTexture(x,y,false);
	}

	cRect rcGround = cRect(rc.left,rc.top,rc.right,rc.bottom);
	TileToGround(rcGround.l,rcGround.t);
	TileToGround(rcGround.r,rcGround.b);

	cRect rcRegion;
	GetRegionRectGround(rcRegion);

	cPlaneHorList* aList[2];
	aList[0] = &m_listPlaneHorGround;
	aList[1] = &m_listPlaneHorBelowVer;
	for (int i = 0; i <= 1; i++)
	{
		for (itcPlaneHor it = aList[i]->begin(); it != aList[i]->end();  ++it)
		{
			cPlaneHor* p = (*it);
			if (p->IsMoving())
				continue;

			cRect rcHor;
			p->GetGroundRect(rcHor);
			rcHor.l -= rcRegion.l;
			rcHor.r -= rcRegion.l;
			rcHor.t -= rcRegion.t;
			rcHor.b -= rcRegion.t;
			cRect r = rcHor & rcGround;
			if (!r.IsEmpty())
			{
				DrawHor(p,r);
//				OutputDebugString("draw a hor!\n");
			}
		}
	}

	CRect rect; 
	rect.left = rc.left * c_ptScale.x;
	rect.right = rc.right * c_ptScale.x;
	rect.top = rc.top * c_ptScale.x;
	rect.bottom = rc.bottom * c_ptScale.x;
	m_pTexture->Update(&rect);
}

void cRegion::RepaintHor(cPlaneHor* pPlane)
{
	//因为只在地图编辑器使用，所以不优化了
	if (m_pTexture == NULL)
		return ;
	m_bTextureDirty = FALSE;
	if (!GetGround()->IsUpdateRegionTexture())
	{
		m_bTextureDirty = TRUE;
		return;
	}

	cRect rcPlane;
	pPlane->GetGroundRect(rcPlane);
	
	//扩大这个rc
/*
	rcPlane.l --;
	rcPlane.r ++;
	rcPlane.t --;
	rcPlane.b ++;
*/
	cRect rcRegion;
	GetRegionRectGround(rcRegion);
	cRect rc = rcPlane & rcRegion;
	if (rc.IsEmpty())
		return ;

	rc.l -= rcRegion.l;
	rc.r -= rcRegion.l;
	rc.t -= rcRegion.t;
	rc.b -= rcRegion.t;
	GroundToTile(rc.l,rc.t);
	GroundToTile(rc.r,rc.b);

	//多过2个格子
#pragma message (ATTENTION("when use 2 在yes那里掉图素"))
	int enlarge = 4;

	int l = floor(rc.l);
	l = l - l % enlarge;
	rc.l = l;
	int r = ceil(rc.r);
	r = (r + enlarge - 1) / enlarge * enlarge;
	rc.r = r;
	int t = floor(rc.t);
	t = t - t % enlarge;
	rc.t = t;
	int b = ceil(rc.b);
	b = (b + enlarge - 1) / enlarge * enlarge;
	rc.b = b;

	CRect rect(rc.l,rc.t,rc.r,rc.b);

	CRect rcTexture(rect.left * c_nWidthTile, rect.top * c_nHeightTile, rect.right * c_nWidthTile, rect.bottom * c_nHeightTile);
	m_pTexture->ClearRect(rcTexture);
	RepaintRectTile(rect);
}

int GetInt(float f)
{
	int t = floor(f);
	if (f - t > 0.5f)
		return t+1;
	return t;
}
//更新在rc中的部分
void cRegion::DrawHor(cPlaneHor* pPlane, const cRect& rect)
{
	if (! (pPlane->GetSortType() <= SORT_HORBELOWVER && !pPlane->IsMoving()) )
		return;

	//get position 
	cRect rc;
	pPlane->BeginPainting();
	pPlane->GetGroundRect(rc);
	int offx = m_x * c_tWidthRegion;
	int offy = m_y * c_tHeightRegion * 2;
	rc.l -= offx;
	rc.r -= offx;
	rc.t -= offy;
	rc.b -= offy;
	//得到regiond的内部坐标

	cRect rcGround(0,0,c_tWidthRegion,c_tHeightRegion * 2);
	cRect rcDes = rect;
	if (rcDes.IsEmpty())
		rcDes = rc & rcGround;

	cPoint ptDes = cPoint(rcDes.l,rcDes.t);
	if (rcDes.IsEmpty())
		return;

	cPoint ptSrc;
	ptSrc.x = (rcDes.l - rc.l);
	ptSrc.y = (rcDes.t - rc.t);
	//得到src的坐标

	cRect rcSrc;
	rcSrc.l = ptSrc.x;
	rcSrc.t = ptSrc.y;
	rcSrc.r = rcSrc.l + rcDes.Width();
	rcSrc.b = rcSrc.t + rcDes.Height();
	
	//render plane from ptFrom to rcDes;
	ASSERT(pPlane->GetResNum() == 1);

	stResourceA pos;
	pos.stDevice.eUsage = USAGE_STATIC;
	pos.stDevice.eMedia = MEDIA_SPR;
	pos.stRes.idResource = 0;
	CString strSpr;
	pPlane->GetSprFile(strSpr);
	strcpy(pos.stRes.szResource,strSpr);

	iTexture* pTex = GetGraphics()->FindTexture(pos,true);
	ASSERT(pTex);
	int nFrames = pTex->GetFrames();
	int iFrame = pPlane->GetiFrame(0);
	if (iFrame >= nFrames)
	{
		iFrame = 0;
		OutputDebugString("frame index error here!");
	}
	ASSERT(iFrame < nFrames);
	ULONG width,height;
	pTex->GetImageSize(iFrame,width,height);

	stRender render;
	render.pTexDes = m_pTexture;
	//直接转换成为int会有一个像素的偏差.
	render.ptDes = CPoint(GetInt(ptDes.x*c_ptScale.x), GetInt(ptDes.y*c_ptScale.x/2));
	render.nFrameDes = 0;
	render.pTexSrc = pTex;
	render.rcSrc = CRect(	GetInt(rcSrc.l*c_ptScale.x),GetInt(rcSrc.t*c_ptScale.x/2),
							GetInt(rcSrc.r*c_ptScale.x),GetInt(rcSrc.b*c_ptScale.x/2));
	render.nFrameSrc = iFrame;
	render.format = GetFormat();
	render.eOp = Render_Alpha;

	cTimer time;
	if (render.rcSrc.right - render.rcSrc.left > 0 && 
		render.rcSrc.bottom - render.rcSrc.top > 0)
		render.pTexSrc->Render(render);
	int t = time.GetTimeMS();
	pTex->Release();
}

/////////////////////////////////////////////////////////////////////////
//paint loop
/////////////////////////////////////////////////////////////////////////

bool cRegion::CheckDisplay(const cRect& rc)
{
	if (!IsValid())
		return false;
	#pragma message (ATTENTION("just check the tile the give it to mesh"))
	m_bInScreen = m_pMesh->CheckDisplay(rc);
	//只需要check tile就足够了
	cRect rcTile;
	GetRegionRectTile(rcTile);
	cRect rcCheck = rc;
	//下降几个格子，保证一个格子中图素的高度小于4
//	rcCheck.b += c_nMaxPlaneHeight;
	GroundToTile(rcCheck.l,rcCheck.t);
	GroundToTile(rcCheck.r,rcCheck.b);
	m_rcTile = rcTile & rcCheck;
	m_rcTile.Offset(-m_x * c_tWidthRegion,-m_y * c_tHeightRegion);
	m_bInScreen = !m_rcTile.IsEmpty();
	return m_bInScreen;
}

bool cRegion::BeginLighting(cColor color)
{
	if (!IsValid())
		return false;
	cColor cr = GetVertexColor(color);
	m_pMesh->BeginLighting(cr);

	for_all(m_listPlaneVerBelowGround,bind2nd(mem_fun1<bool,cPlane,cColor>(&cPlane::BeginLighting),color));
	return true;
}

bool cRegion::EndLighting()
{
	if (!IsValid())
		return false;
	if (!m_param.bTransparent)
		m_pMesh->EndLighting();	
	return true;
}

cColor cRegion::GetVertexColor(cColor color)
{
	cColor cr ;
	cr.a = color.a;
	cr.r = m_param.crColor.r * color.r / 255;
	cr.g = m_param.crColor.g * color.g / 255;
	cr.b = m_param.crColor.b * color.b / 255;
	return cr;
}
/////////////////////////////////////////////////////////////////////////
//device
/////////////////////////////////////////////////////////////////////////
HRESULT cRegion::InitDeviceObjects()
{
	if (!IsValid())
		return false;

	for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::InitDeviceObjects));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::InitDeviceObjects));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::InitDeviceObjects));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::EndPainting));
	if (m_pMesh)
		m_pMesh->InitDeviceObjects();
	
	return S_OK;
};

HRESULT cRegion::RestoreDeviceObjects()
{
	if (!IsValid())
		return false;

	for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::RestoreDeviceObjects));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::RestoreDeviceObjects));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::RestoreDeviceObjects));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::RestoreDeviceObjects));
	if (m_pMesh)
		m_pMesh->RestoreDeviceObjects();
	
	return S_OK;
};                     

HRESULT cRegion::InvalidateDeviceObjects()
{
	if (!IsValid())
		return false;

	for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::InvalidateDeviceObjects));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::InvalidateDeviceObjects));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::InvalidateDeviceObjects));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::InvalidateDeviceObjects));
	if (m_pMesh)
		m_pMesh->InvalidateDeviceObjects();
	
	return S_OK;
};

HRESULT cRegion::DeleteDeviceObjects()
{
	if (!IsValid())
		return false;

	for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::DeleteDeviceObjects));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::DeleteDeviceObjects));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::DeleteDeviceObjects));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::DeleteDeviceObjects));
	if (m_pMesh)
		m_pMesh->DeleteDeviceObjects();
	
	return S_OK;
};

HRESULT cRegion::FinalCleanup()
{
	if (!IsValid())
		return false;

	for_all(m_listLight,bind2nd(mem_fun1<BOOL,cLight,BOOL>(&cLight::SetInGround),FALSE));
	for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::FinalCleanup));
	for_all(m_listLight,mem_fun<ULONG,cLight>(&cLight::Release));
	m_listLight.clear();

	for_all(m_listObstacle,bind2nd(mem_fun1<BOOL,cObstacle,BOOL>(&cObstacle::SetInGround),FALSE));
	for_all(m_listObstacle,mem_fun<ULONG,cObstacle>(&cObstacle::Release));
	m_listObstacle.clear();

	for_all(m_listPlane,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(&cPlane::SetInGround),FALSE));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::FinalCleanup));
	for_all(m_listPlane,mem_fun<ULONG,cPlane>(&cPlane::Release));
	m_listPlane.clear();

	for_all(m_listPlaneVerBelowGround,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(&cPlane::SetInGround),FALSE));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::FinalCleanup));
	for_all(m_listPlaneVerBelowGround,mem_fun<ULONG,cPlane>(&cPlane::Release));
	m_listPlaneVerBelowGround.clear();

	for_all(m_listClientNpc,bind2nd(mem_fun1<BOOL,cPlane,BOOL>(&cPlane::SetInGround),FALSE));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::FinalCleanup));
	for_all(m_listClientNpc,mem_fun<ULONG,cPlane>(&cPlane::Release));
	m_listClientNpc.clear();

	for_all(m_listLinkPlaneHor,mem_fun<ULONG,cPlane>(&cPlane::Release));
	m_listLinkPlaneHor.clear();

	m_listPlaneHorGround.clear();
	m_listPlaneHorBelowVer.clear();

	if (m_pMesh)
	{
		m_pMesh->FinalCleanup();
		RELEASE(m_pMesh);
	}
	RELEASE(m_pTexture);
	
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//size
///////////////////////////////////////////////////////////////////////////
void cRegion::GetRegionRectTile(cRect& rcGround)
{
	rcGround.l = m_x*c_tWidthRegion;
	rcGround.r = rcGround.l + c_tWidthRegion;
	rcGround.t = m_y*c_tHeightRegion;
	rcGround.b = rcGround.t + c_tHeightRegion;
}

void cRegion::GetRegionRectGround(cRect& rcGround)
{
	GetRegionRectTile(rcGround);
	TileToGround(rcGround.l,rcGround.t);
	TileToGround(rcGround.r,rcGround.b);
/*	rcGround.l = GetTile(0,0)->v.sx;
	rcGround.r = GetTile(c_tWidthRegion,c_tHeightRegion)->v.sx;
	rcGround.t = GetTile(0,0)->v.sy;
	rcGround.b = GetTile(c_tWidthRegion,c_tHeightRegion)->v.sy;
*/
}

BOOL cRegion::IsPointInRegion(cPoint p)
{
	cRect rcGround;
	GetRegionRectGround(rcGround);
	return IsPointInRect(p,rcGround);
}

BOOL cRegion::IsRectCrossRegion(cRect rc)
{
	cRect rcGround;
	GetRegionRectGround(rcGround);
	cRect r = rcGround & rc;
	return (!r.IsAbsEmpty());
}

BOOL cRegion::CrossPlane(cPlane* p,eRegionDirection e)
{
#pragma message (ATTENTION("function not finish"))	
	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer != NULL)
	{
		cPoint p1,p2;
		pVer->GetBottomSeam(p1,p2);
		cRect rc;
		rc.l = _cpp_min(p1.x,p2.x);
		rc.r = _cpp_max(p1.x,p2.x);
		rc.t = _cpp_min(p1.y,p2.y);
		rc.b = _cpp_max(p1.y,p2.y);
		if (IsRectCrossRegion(rc))
			return TRUE;
		return FALSE;
	}
	cPlaneHor* pHor = dynamic_cast<cPlaneHor*>(p);
	if (pHor != NULL)
	{
		cRect rc;
		pHor->GetGroundRect(rc);
		if (IsRectCrossRegion(rc))
			return TRUE;
		return FALSE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//elmenent of the region
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::AddLinkHorFromRegion(cRegion* pRegion)
{
	//for all hor of the pRegion;
/*
	cPlaneHorList plist = pRegion->m_listPlaneHorBelowVer;
	plist.insert(plist.end(),pRegion->m_listPlaneHorGround.begin(),
		pRegion->m_listPlaneHorGround.end());
*/
	cRect rcRegion;
	GetRegionRectGround(rcRegion);
	cRect rc;
//	for (itcPlaneHor it = plist.begin(); it != plist.end(); ++it)
	for (itcPlane it = pRegion->m_listPlane.begin(); it != pRegion->m_listPlane.end(); ++it)
	{
		cPlaneHor* p = dynamic_cast<cPlaneHor*>(*it);
		if (p == NULL)
			continue;
		p->GetGroundRect(rc);
		cRect r = rc& rcRegion;
		if (!r.IsEmpty())
		{
			AddLinkHor(p);
		}
	}
	return TRUE;
}

BOOL cRegion::RemoveLinkHor()
{
	cPlaneHorList plist = m_listLinkPlaneHor;
	for (itcPlaneHor it = plist.begin(); it != plist.end(); ++it)
	{
		RemoveLinkHor(*it);
	}
//	for_all(m_listLinkPlaneHor,mem_fun<ULONG,cPlane>(&cPlane::Release));
//	m_listLinkPlaneHor.clear();
	return TRUE;
}


BOOL cRegion::AddLinkHor(cPlaneHor* p)
{
	for (itcPlaneHor it = m_listLinkPlaneHor.begin(); it != m_listLinkPlaneHor.end(); ++it)
	{
		if ((*it) == (p))					//p 不应该在列表中
			return FALSE;
	}
	p->AddRef();
	m_listLinkPlaneHor.push_back(p);
	AddHor(p,true);
	return TRUE;
}

BOOL cRegion::RemoveLinkHor(cPlaneHor* p)
{
	int size = m_listLinkPlaneHor.size();
	m_listLinkPlaneHor.remove(p);
	if (size != m_listLinkPlaneHor.size()+1) 	//p 不在列表中
		return FALSE;
	RemoveHor(p, true);
	p->Release();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//plane
/////////////////////////////////////////////////////////////////////////
void cRegion::UpdateModify(iObj* pObj)
{
	if (dynamic_cast<cObstacle*>(pObj))
	{
		AddModify(MODIFY_OBSTACLE);
	}
	else if (dynamic_cast<cLight*>(pObj))
	{
		AddModify(MODIFY_LIGHT);
	}
	else if (dynamic_cast<cPlane*>(pObj))
	{
		cPlane* p = dynamic_cast<cPlane*>(pObj);
		if (p->GetDynamicType() != DYNAMIC_STATIC)
		{
			AddModify(MODIFY_CLIENTNPC);
			return;
		}

		switch (p->GetSortType())
		{
		case SORT_VER:
			AddModify(MODIFY_VER);
			break;
		case SORT_HORBELOWVER:
		case SORT_HORGROUND:
			AddModify(MODIFY_HORBELOWVER);
			break;
		case SORT_HORABOVEVER:
			AddModify(MODIFY_HORABOVEVER);
			break;
		}
	}
}

cPlaneHorList* cRegion::GetPlaneHorList(cPlaneHor* p)
{
	cPlaneHorList* plist = NULL;
	if (p->GetSortType() == SORT_HORGROUND)
		plist = &m_listPlaneHorGround;
	else if (p->GetSortType() == SORT_HORBELOWVER)
		plist = &m_listPlaneHorBelowVer;
	else 
	{
		stPlaneHorParam& param = *(p->GetPlaneHorParam());
		param.eSort = SORT_HORBELOWVER;
		plist = &m_listPlaneHorBelowVer;
	}
	return plist;
}

BOOL cRegion::AddHor(cPlaneHor* p, bool bRepaint)
{
	//the list should sort
	cPlaneHorList* pplist = GetPlaneHorList(p);
	if (pplist == NULL)
	{
		ASSERT(0);
		return FALSE;
	}

	if (!T_AddPlaneToListByLayer<cPlaneHor>(*pplist,p))
		return FALSE;

	if ( bRepaint && IsPaintTerrain())//只有需要绘制terrain的时候
		RepaintHor(p);
	return TRUE;
}

BOOL cRegion::RemoveHor(cPlaneHor* p, bool bRepaint)
{
	//the list should sort
	cPlaneHorList* pplist = GetPlaneHorList(p);
	if (pplist == NULL)
	{
		ASSERT(0);
		return FALSE;
	}
	
	if (!T_RemovePlaneFromList<cPlaneHor>(*pplist,p))
		return FALSE;

	if (bRepaint && IsPaintTerrain())
		RepaintHor(p);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//plane
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::FindcPlane(cPlane* p)
{
	return ( find(m_listPlane.begin(),m_listPlane.end(),p) != m_listPlane.end() );
}

BOOL cRegion::AddcPlane(cPlane* p)
{
	if (T_AddToList<cPlane>(m_listPlane,p))
	{
		p->SetRegion(m_x,m_y);
		UpdateModify(p);

		if (IsPaintPlane())
			p->FindTexture();
		else
			p->ClearTexture();

		return TRUE;
	}
	return FALSE;
}

BOOL cRegion::RemovecPlane(cPlane* p)
{
	//remove Plane from tree
	if (T_RemoveFromList<cPlane>(m_listPlane,p))
	{
		p->SetRegion(-1,-1);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//light
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::AddcLight(cLight* p)
{
	if (T_AddToList<cLight>(m_listLight,p))
	{
		p->SetRegion(m_x,m_y);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

BOOL cRegion::RemovecLight(cLight* p)
{
	if (T_RemoveFromList<cLight>(m_listLight,p))
	{
		p->SetRegion(-1,-1);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//obstacle
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::AddcObstacle(cObstacle* p)
{
	if (T_AddToList<cObstacle>(m_listObstacle,p))
	{
		p->SetRegion(m_x,m_y);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

BOOL cRegion::RemovecObstacle(cObstacle* p)
{
	if (T_RemoveFromList<cObstacle>(m_listObstacle,p))
	{
		p->SetRegion(-1,-1);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//verbelowground 
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::AddVerBelowGround(cPlane* p)
{
	if (T_AddToList<cPlane>(m_listPlaneVerBelowGround,p))
	{
		p->SetRegion(m_x,m_y);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

BOOL cRegion::RemoveVerBelowGround(cPlane* p)
{
	if (T_RemoveFromList<cPlane>(m_listPlaneVerBelowGround,p))
	{
		p->SetRegion(-1,-1);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////
//client only
/////////////////////////////////////////////////////////////////////////
BOOL cRegion::AddClientOnlyNpc(cPlane* p)
{
	if (T_AddToList<cPlane>(m_listClientNpc,p))
	{
		p->SetRegion(m_x,m_y);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

BOOL cRegion::RemoveClientOnlyNpc(cPlane* p)
{
	if (T_RemoveFromList<cPlane>(m_listClientNpc,p))
	{
		p->SetRegion(-1,-1);
		UpdateModify(p);
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//paint loop
//////////////////////////////////////////////////////////////////////////
HRESULT cRegion::PaintBeforeTerrain()
{
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(cPlane::Draw));
	return TRUE;
}

HRESULT cRegion::PaintTerrain()
{
	if (m_param.bDraw == FALSE)
		return S_OK;

//	if (GetGround()->m_param.bRenderMap)
	{
		if (m_pTexture == NULL || m_bUpdateTexture == FALSE) 
			return S_OK;
	}
	ASSERT(m_pTexture != NULL);
	ASSERT(m_pTexture->IsValid());

	HRESULT hr = GetGround()->SetTexture(m_pTexture,0);
	if (FAILED(hr))
		return hr;
	hr = m_pMesh->Draw();
	return hr;
}

HRESULT cRegion::PaintHorBelow()
{
	return S_OK;
}

HRESULT cRegion::PaintVer(int yTile)
{
	return S_OK;
}

HRESULT cRegion::PaintVer()
{
	return S_OK;
}

HRESULT cRegion::PaintHorAbove()
{
	return S_OK;
}

HRESULT cRegion::DrawTrace()
{
	if (m_param.lMapeditEnum & Mapedit_Select)
	{
		cPoint pt = GetGround()->GetWindSpeed();
		char s[256];
		sprintf(s,"(F3)Region: wind %.2f,%.2f,%.2f CR %d,%d Color %d,%d,%d ani %d speed %d degree %.1f",	
			pt.x,pt.y,pt.z,
			m_x,m_y,
			m_param.crColor.r,m_param.crColor.g,m_param.crColor.b,
			m_param.eAnimation, m_param.nSpeed, m_param.fDegree
			);
		GetGraphics()->Trace(s);
	}
	return S_OK;
}


HRESULT cRegion::PaintObstacle()
{
	if (GetGraphics()->IsShow(iGraphics::GS_OBSTACLE))
	{
		for_all(m_listObstacle,mem_fun<HRESULT,cObstacle>(&cObstacle::Draw));
		for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::DrawObstacle));
	}
	return S_OK;
}

HRESULT cRegion::PaintOthers()
{
	//SORT_HOR_BELOW_GROUND,SORT_HOR_GROUND,SORT_HOR_BELOW_VER 统一画在地表上面了
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::DrawSelect));
	for_all(m_listPlane,mem_fun<HRESULT,cPlane>(&cPlane::DrawTrace));
	
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::DrawSelect));
	for_all(m_listPlaneVerBelowGround,mem_fun<HRESULT,cPlane>(&cPlane::DrawTrace));

	//绘制障碍(为了编辑)
	if (GetGraphics()->IsShow(iGraphics::GS_LIGHT))
		for_all(m_listLight,mem_fun<HRESULT,cLight>(&cLight::Draw));

//	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::Draw));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::DrawSelect));
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::DrawTrace));

	DrawTrace();
	return S_OK;
}

HRESULT cRegion::PaintText()
{
	for_all(m_listClientNpc,mem_fun<HRESULT,cPlane>(&cPlane::DrawNick));
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//filter
////////////////////////////////////////////////////////////////////////////////

HRESULT cRegion::FilterByY(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut)
{
	int y = INT_MIN;
	cPlane* pGet = NULL;
	for (itcPlane it = listIn.begin(); it != listIn.end(); ++it)
	{
		cPlane* pPlane = (*it);
		if (pPlane->GetOffset().y > y)
		{
			pGet = pPlane;
			y = pPlane->GetOffset().y;
		}
	}
//	pGet->AddRef();
	listOut.push_back(pGet);
	return S_OK;
}

HRESULT cRegion::FilterByZ(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut)
{
	int z = INT_MIN;
	cPlane* pGet = NULL;
	for (itcPlane it = listIn.begin(); it != listIn.end(); ++it)
	{
		cPlane* pPlane = (*it);
		if (pPlane->GetOffset().z > z)
		{
			pGet = pPlane;
			z = pPlane->GetOffset().z;
		}
	}
//	pGet->AddRef();
	listOut.push_back(pGet);
	return S_OK;
}

HRESULT cRegion::FilterByDisplay(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut)
{
	ePlane2Plane e = PP_FRONT;
	if (param == SelectFilterParam_Far)
		e = PP_BACK;

	cPlane* pGet = NULL;
	for (itcPlane it = listIn.begin(); it!= listIn.end(); ++it)
	{
		cPlane* pPlane = (*it);
		if (pGet == NULL)
			pGet = pPlane;
		else if (pPlane->PositionPlane(pGet) == e)
			pGet = pPlane;
	}

//	pGet->AddRef();
	listOut.push_back(pGet);
	return S_OK;
}

HRESULT cRegion::FilterByOffset(SelectFilterParamEnum param, cPlaneList& listIn, cPlaneList& listOut,POINT point)
{
	cPoint ptGround;
	GetGround()->ScreenToGround(point,ptGround);
	for (itcPlane it = listIn.begin(); it!= listIn.end(); ++it)
	{
		cPlane* pPlane = (*it);
		if (DistanceAdd(pPlane->GetOffset(),ptGround) < 0.1f)
		{
//			(*it)->AddRef();
			listOut.push_back(*it);
			return S_OK;
		}
	}
	return S_OK;
}

HRESULT cRegion::FilterByArea(SelectFilterParamEnum param,  cPlaneList& listIn, cPlaneList& listOut)
{
	if (param == SelectFilterParam_Default)
		param = SelectFilterParam_Min;

	cPlane* pGet = NULL;
	float minArea = 0;
	for (itcPlane it = listIn.begin(); it != listIn.end(); ++it)
	{
		cPlane* pPlane = (*it);
		float area = pPlane->GetArea();
		if (pGet == NULL)
		{
			pGet = pPlane;
			minArea = area;
		}
		else if (area < minArea && param == SelectFilterParam_Min || 
			area > minArea && param == SelectFilterParam_Max )
		{
			pGet = pPlane;
			minArea = area;
		}
	}

//	pGet->AddRef();
	listOut.push_back(pGet);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//getobj
////////////////////////////////////////////////////////////////////////////

HRESULT cRegion::GetiObj(int nID, iObjList& iList)
{
	//get the 9 tile or get of the list?
	T_GetAt(nID,m_listPlane,iList);
	T_GetAt(nID,m_listObstacle,iList);
	T_GetAt(nID,m_listLight,iList);
	T_GetAt(nID,m_listClientNpc,iList);
	T_GetAt(nID,m_listPlaneVerBelowGround,iList);
	return S_OK;
}

HRESULT	cRegion::GetiLight(stSelect& st,cLightList& tlist)
{
	cLightList ilist = m_listLight;
	cLightList jlist;
	switch (st.ePoint)
	{
	case SelectPoint_OneByScreen:
		{
			itcLight it = find_if(ilist.begin(),ilist.end(),
				bind2nd(mem_fun1<BOOL,cLight,POINT>(&cLight::Contain),st.ptScreen));
			if (it != ilist.end())
				jlist.push_back(*it);
		}
		break;
	case SelectPoint_RectByDiamond:
		{
			insert_if(ilist.begin(),ilist.end(),jlist,
				bind2nd(mem_fun1<BOOL,cLight,cRect>(&cLight::ContainedDiamond),st.rcGround));
		}
		break;
	}
	for_all(jlist,mem_fun<ULONG,iObj>(&iObj::AddRef));
	insert(jlist.begin(),jlist.end(),tlist);	
	return S_OK;
}

HRESULT	cRegion::GetiObstacle(stSelect& st,cObstacleList& tlist)
{
	cObstacleList ilist = m_listObstacle;
	cObstacleList jlist;
	switch (st.ePoint)
	{
	case SelectPoint_OneByScreen:
		{
			itcObstacle it = find_if(ilist.begin(),ilist.end(),
				bind2nd(mem_fun1<BOOL,cObstacle,POINT>(&cObstacle::Contain),st.ptScreen));
			if (it != ilist.end())
			{
				jlist.push_back(*it);
			}
			else
			{
				for (itcPlane it = m_listPlane.begin(); it != m_listPlane.end(); ++it)
				{
					cPlane* p = (*it);
					cObstacle* pObs = p->GetObstacle(st.ptScreen);
					if (pObs != NULL)
					{
						jlist.push_back(pObs);
						break;
					}
				}
			}
		}
		break;
	case SelectPoint_RectByDiamond:
		{
			insert_if(ilist.begin(),ilist.end(),jlist,
				bind2nd(mem_fun1<BOOL,cObstacle,cRect>(&cObstacle::ContainedDiamond),st.rcGround));
			//insert
			//选择一个区域作为组件，这样的话不要选择阻挡
		}
		break;
	}
	for_all(jlist,mem_fun<ULONG,iObj>(&iObj::AddRef));
	insert(jlist.begin(),jlist.end(),tlist);	
	return S_OK;
}

HRESULT	cRegion::GetiPlane(stSelect& st,cPlaneList& tlist)
{
	cPlaneList ilist;
	switch (st.eType)
	{
	case SelectType_Dynamic:
		ilist = m_listClientNpc;
		break;
	case SelectType_Default:
		ilist = m_listPlane;				
		break;
	case SelectType_Sort:
		{
			int param = st.eSort;
			if (param < 0 || param >= SORT_NUM)
				return 0;

			if (param == SORT_VERBELOWGROUND)
			{
				ilist = m_listPlaneVerBelowGround;
			}
			else
			{
				insert_if(m_listPlane.begin(),m_listPlane.end(),ilist,
					bind2nd(mem_fun1<BOOL,cPlane,SortEnum>(&cPlane::IsSortType),(SortEnum)param));				
			}
		}
		break;
	case SelectType_Kind:
		{
			int param = st.eKind;
			if (param < 0 || param >= KIND_NUM)
				return 0;
			insert_if(m_listPlane.begin(),m_listPlane.end(),ilist,
				bind2nd(mem_fun1<BOOL,cPlane,long>(&cPlane::IsKind),param));
		}
		break;
	}

	if (ilist.empty())
		return 0;

	cPlaneList jlist;
	switch(st.ePoint)
	{
	case SelectPoint_OneByScreen:
		insert_if(ilist.begin(),ilist.end(),jlist,
			bind2nd(mem_fun1<BOOL,cPlane,POINT>(&cPlane::Contain),st.ptScreen));
		break;
	case SelectPoint_RectByGround:
		insert_if(ilist.begin(),ilist.end(),jlist,
			bind2nd(mem_fun1<BOOL,cPlane,cRect>(&cPlane::ContainedGround),st.rcGround));
		break;
	case SelectPoint_RectByDiamond:
		insert_if(ilist.begin(),ilist.end(),jlist,
			bind2nd(mem_fun1<BOOL,cPlane,cRect>(&cPlane::ContainedDiamond),st.rcGround));
		break;
	}

	if (jlist.empty())
	{
//		CString s;
//		s.Format("there are %d plane in the list, but not found",ilist.size());
//		AfxMessageBox(s);
		return 0;
	}

	insert(tlist.begin(), tlist.end(), jlist);
	tlist.clear();

	switch (st.eFilter)
	{
	case SelectFilter_All:
		for_all(jlist,mem_fun<ULONG,iObj>(&iObj::AddRef));
		insert(jlist.begin(),jlist.end(),tlist);
		break;
	case SelectFilter_OneByY:
		FilterByY(st.eFilterParam,jlist,tlist);		
		break;
	case SelectFilter_OneByZ:
		FilterByZ(st.eFilterParam,jlist,tlist);		
		break;
	case SelectFilter_OneByArea:
		FilterByArea(st.eFilterParam,jlist,tlist);
		break;
	case SelectFilter_OneByOffset:
		FilterByOffset(st.eFilterParam,jlist,tlist,st.ptScreen);
		break;
	case SelectFilter_OneByDisplay:
		FilterByDisplay(st.eFilterParam,jlist,tlist);
		break;
	}

	return 0;
}

BOOL cRegion::BlockObstacle(cPoint pt,ObstacleEnum e)
{
	for (itcObstacle it = m_listObstacle.begin(); it!= m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		if(p->BlockObstacle(pt,e))
			return TRUE;
	}
	for (itcPlane it2 = m_listPlane.begin(); it2!= m_listPlane.end(); ++it2)
	{
		cPlane* pPlane = (*it2);
		if (pPlane ->BlockObstacle(pt,e))
			return TRUE;
	}
	return FALSE;
}

ObstacleEnum cRegion::CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise)
{
	for (itcObstacle it = m_listObstacle.begin(); it!= m_listObstacle.end(); ++it)
	{
		cObstacle* p = (*it);
		ObstacleEnum e = p->CheckObstacle(pt,vSpeed,bClockWise);
		if (e != OBSTACLE_NULL)
			return e;
//		if (p->Contain(pt))
//		{
//			return (*it)->GetType();
//		}
	}
	for (itcPlane it2 = m_listPlane.begin(); it2!= m_listPlane.end(); ++it2)
	{
		cPlane* pPlane = (*it2);
		ObstacleEnum e = pPlane ->CheckObstacle(pt,vSpeed,bClockWise);
		if (e != OBSTACLE_NULL)
			return e;
	}
	return OBSTACLE_NULL;
}

/////////////////////////////////////////////////////////////////
//for mapconvert
/////////////////////////////////////////////////////////////////
BOOL GetRegionSize(const CString& sName, int& sx, int& sy)
{
	int pos = sName.Find("_");
	if (pos == -1)
		return FALSE;
	CString sX = sName.Left(pos);
	CString sY = sName.Mid(pos + 1);
	sx = atoi((LPCSTR)sX);
	sy = atoi((LPCSTR)sY);
	return TRUE;
}

int cRegion::MC_Load(LPCSTR szFolder)
{
	SetRegionFolder(szFolder);
	CFileFind f;
	if (!f.FindFile(m_strName))
		return 0;
	LoadPlaneVer();
	LoadPlaneHor();
//	LoadPlaneHorAboveVer();
	return 0;
}

int cRegion::MC_GetLinkInfo(LPCSTR szFolder)
{
	m_strName = szFolder;
	cPlaneList plist;
	int pos = m_strName.ReverseFind('\\');
	CString strRegion = m_strName.Mid(pos+1);
	GetRegionSize(strRegion,m_x,m_y);

	ASSERT(pos != -1);
	CString strFolder = m_strName.Left(pos);
	for (int x=-1; x<=1; x++)
	for (int y=-1; y<=1; y++)
	{
		if (x == 0 && y == 0)
			continue;
		if (m_x+x < 0 || m_y+y < 0)
			continue;
		cRegion region(GetGraphics());
		region.SetPos(m_x+x,m_y+y);
		region.SetPaintPlane(FALSE);
		region.SetPaintTerrain(FALSE);
		region.MC_Load(strFolder);
		for(itcPlane it = region.m_listPlane.begin(); it != region.m_listPlane.end(); it++)
		{
			cPlaneHor* p = dynamic_cast<cPlaneHor*> (*it);
			if (p != NULL)
			{
				p->UpdateVertexInfo();
				if (CrossPlane(p,(eRegionDirection)0))
				{
					p->AddRef();
					plist.push_back(p);
				}
			}
		}
	}

	CString strFile = m_strName + "\\planehorlink.ini";
	T_SaveList(plist,strFile);	
	if (!plist.empty())
		for_all(plist,mem_fun<ULONG,cPlane>(&cPlane::Release));
	return S_OK;
}
