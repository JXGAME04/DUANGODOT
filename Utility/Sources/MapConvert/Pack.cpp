// MapConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"
#include "iPack2D.h"
#include "iItem.h"
#include "iGraphics.h"
#include "..\controls\FileFindEx.h"
#include "math.h"
#include <iomanip>
#include <fstream>
#include <shlwapi.h>
#include "iImage.h"
#include "foreach.h"
#include "KSpbioTemplate.h"

#include "izip.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////
//将地图打包
/////////////////////////////////////////////////////////////
iPack2D* g_pPack = NULL;

int on_zip_property(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	
	iRegion::stParam m_param;
	memset(&m_param,0,sizeof(m_param));
	GetItemValue(pItem,"draw",m_param.bDraw);
	GetItemValue(pItem,"color",m_param.crColor);
	GetItemValue(pItem,"transparent",m_param.bTransparent);
	GetItemValue(pItem,"animation",cast_int(m_param.eAnimation));
	GetItemValue(pItem,"animationspeed",m_param.nSpeed);
	GetItemValue(pItem,"animationdegree",m_param.fDegree);

	g_pPack->Convert(sx,sy,&m_param,sizeof(m_param));

	return 0;
}

int on_zip_ground(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	stTile m_aTile[c_tWidthRegion+1][c_tHeightRegion+1];
	
	CFile f;
	if (!f.Open(szFile,CFile::modeRead))
		return 0;

	stTerrainTile m_aGroundTile[c_tWidthRegion+1][c_tHeightRegion+1];

	cColor m_crRegion;
	f.Read(&m_crRegion,sizeof(m_crRegion));
	f.Read(m_aTile,sizeof(stTile)*(c_tWidthRegion+1)*(c_tHeightRegion+1));
	
	for (int i=0; i<c_tWidthRegion+1; i++)
	for (int j=0; j<c_tHeightRegion+1; j++)
	{
//		m_aGroundTile[i][j].v = m_aTile[i][j].v;
//		m_aGroundTile[i][j].ptPos.x = m_aTile[i][j].v.sx;
//		m_aGroundTile[i][j].ptPos.y = m_aTile[i][j].v.sy;
//		m_aGroundTile[i][j].ptPos.z = m_aTile[i][j].v.sz;
		m_aGroundTile[i][j].idResource = -1;
		m_aGroundTile[i][j].iFrame = m_aTile[i][j].iFrame;
		m_aGroundTile[i][j].iDirection = m_aTile[i][j].iDirection;
		if (m_aTile[i][j].szResource[0] != 0)
		{
			m_aGroundTile[i][j].idResource = Template_GetTerrainIndex(m_aTile[i][j].szResource);
		//	ASSERT(m_aGroundTile[i][j].idResource != -1);
		}
	}

	g_pPack->Convert(sx,sy,&m_aGroundTile,sizeof(stTerrainTile)*(c_tWidthRegion+1)*(c_tHeightRegion+1));

	return 0;
}

int on_zip_plane(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	struct stPlane
	{
		int nPlane;
		stPlaneVerParam aParam[512];
	};
	
	static stPlane s_Plane;
	memset(&s_Plane,0,sizeof(s_Plane));
	s_Plane.nPlane = 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"elementnum",s_Plane.nPlane))
		return 0;

	for (int i=0; i<s_Plane.nPlane; i++)
	{
		stPlaneVerParam& param = s_Plane.aParam[i];
		stSection* p = pItem->GetIndexSection(i);
		GetSectionValue(p,"elementtype",cast_int(param.eElement));
		GetSectionValue(p,"color",param.crColor);
		GetSectionValue(p,"mapedit",param.lMapeditEnum);		

		if (!GetSectionValue(p,"regionoffset",param.ptOffset))
		{
			GetSectionValue(p,"groundoffset",param.ptOffset);
			if (fabs(param.ptOffset.x) < c_nWidthRegion*2 || fabs(param.ptOffset.y) < c_nHeightRegion*2)
			{
				int n;
				n = 0;
			}
			else
			{
				param.ptOffset.x -= sx * c_nWidthRegion;
				param.ptOffset.y -= sy * c_nHeightRegion;
			}
		}

		GetSectionValue(p,"groupid",param.lGroupID);
		GetSectionValue(p,"event",param.szEvent);
		GetSectionValue(p,"id",param.lID);
		GetSectionValue(p,"groupfile",param.szGroup);
		GetSectionValue(p,"template",param.szTemplate);
		GetSectionValue(p,"templateindex",param.iTemplate);

		CString strValue;
		if (GetSectionValue(p,"kind",strValue))
			param.eKind = (KindEnum)StringToPlaneTableValue(TP_KIND,strValue);

		GetSectionValue(p,"layer",param.nLayer);
		GetSectionValue(p,"iframe",param.stResA.stRes.iFrame);
		GetSectionValue(p,"texWidth",param.wTex);
		GetSectionValue(p,"texHeight",param.hTex);
		GetSectionValue(p,"texX",param.xTex);
		GetSectionValue(p,"texY",param.yTex);

		GetSectionValue(p,"sortoffy",param.fSortOffY);		
	}

	g_pPack->Convert(sx,sy,&s_Plane,sizeof(stPlaneVerParam)*s_Plane.nPlane+sizeof(s_Plane.nPlane));

	return 0;
}

int on_zip_planever(LPCSTR szFile)
{
	on_zip_plane(szFile);
	return 0;
}

int on_zip_planehor(LPCSTR szFile)
{
	on_zip_plane(szFile);
	return 0;
}

int on_zip_npc(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	struct stObj
	{
		int nObj;
		stObjPack aParam[512];
	};
	static stObj s_Obj;
	memset(&s_Obj,0,sizeof(s_Obj));
	s_Obj.nObj = 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"elementnum",s_Obj.nObj))
		return 0;

	for (int i=0; i<s_Obj.nObj; i++)
	{
		stObjPack& param = s_Obj.aParam[i];
		stSection* p = pItem->GetIndexSection(i);
		VERIFY(GetSectionValue(p,"regionoffset",param.ptOffset));
		VERIFY(GetSectionValue(p,"id",param.lID));
		VERIFY(GetSectionValue(p,"event",param.szEvent));

		CString strTemplate;
		VERIFY(GetSectionValue(p,"template",strTemplate));

		CString sPath,sName,sExt;
		AppParsePathNameExt(strTemplate,sPath,sName,sExt);
		int pos = sName.ReverseFind('_');
		
		if (pos != -1)
		{
			ASSERT(pos != -1);
			sName = sName.Mid(pos+1);
		}
		ASSERT(!sName.IsEmpty());
		strcpy(param.szKind,sName);
		//szKind 是一级模板。
		//$event_$id 是二级模板。
	}

	g_pPack->Convert(sx,sy,&s_Obj,sizeof(stObjPack)*s_Obj.nObj+sizeof(s_Obj.nObj));
	return 0;
}

int on_zip_light(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	struct stLight
	{
		int nLight;
		stLightParam aParam[512];
	};
	
	static stLight s_Light;
	memset(&s_Light,0,sizeof(s_Light));
	s_Light.nLight = 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"elementnum",s_Light.nLight))
		return 0;

	for (int i=0; i<s_Light.nLight; i++)
	{
		stLightParam& param = s_Light.aParam[i];
		stSection* p = pItem->GetIndexSection(i);
		GetSectionValue(p,"elementtype",cast_int(param.eElement));
		GetSectionValue(p,"color",param.crColor);
		GetSectionValue(p,"mapedit",param.lMapeditEnum);		

		if (!GetSectionValue(p,"regionoffset",param.ptOffset))
		{
			GetSectionValue(p,"groundoffset",param.ptOffset);
			if (fabs(param.ptOffset.x) < c_nWidthRegion*2 || fabs(param.ptOffset.y) < c_nHeightRegion*2)
			{
				int n;
				n = 0;
			}
			else
			{
				param.ptOffset.x -= sx * c_nWidthRegion;
				param.ptOffset.y -= sy * c_nHeightRegion;
			}
		}
//		GetSectionValue(p,"regionoffset",param.ptOffset);
//		GetSectionValue(p,"groundoffset",param.ptOffset);
		GetSectionValue(p,"groupid",param.lGroupID);
		GetSectionValue(p,"event",param.szEvent);
		GetSectionValue(p,"id",param.lID);
		GetSectionValue(p,"groupfile",param.szGroup);
		GetSectionValue(p,"template",param.szTemplate);
		GetSectionValue(p,"templateindex",param.iTemplate);

		CString strValue;
		if (GetSectionValue(p,"kind",strValue))
			param.eKind = (KindEnum)StringToPlaneTableValue(TP_KIND,strValue);

		GetSectionValue(p,"range",param.fRange);
		GetSectionValue(p,"mainlight",param.bMainlight);

		GetSectionValue(p,"varrangemax",param.fRangeVarMax);
		GetSectionValue(p,"varrangespeed",param.fRangeVarSpeed);
		GetSectionValue(p,"varrangeframe",param.fRangeVarFrame);

		GetSectionValue(p,"posjumpmax",param.fPosJumpMax);
		GetSectionValue(p,"posjumpspeed",param.ptPosJumpSpeed);
		GetSectionValue(p,"posjumpframe",param.fPosJumpFrame);

		GetSectionValue(p,"frameflash",param.fFlashFrame);
		GetSectionValue(p,"framedark",param.nFlashDarkFrame);
	}

	g_pPack->Convert(sx,sy,&s_Light,sizeof(stLightParam)*s_Light.nLight+sizeof(s_Light.nLight));

	return 0;
}

int on_zip_obstacle(LPCSTR szFile)
{
	int sx,sy;
	if (!GetFullFileRegionSize(szFile,sx,sy))
		return 0;

	struct stObstacle
	{
		int nObstacle;
		stObstacleParam aParam[512];
	};
	
	static stObstacle s_Obstacle;
	memset(&s_Obstacle,0,sizeof(s_Obstacle));
	s_Obstacle.nObstacle = 0;

	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (!GetItemValue(pItem,"elementnum",s_Obstacle.nObstacle))
		return 0;

	for (int i=0; i<s_Obstacle.nObstacle; i++)
	{
		stObstacleParam& param = s_Obstacle.aParam[i];
		stSection* p = pItem->GetIndexSection(i);
		GetSectionValue(p,"elementtype",cast_int(param.eElement));
		GetSectionValue(p,"color",param.crColor);
		GetSectionValue(p,"mapedit",param.lMapeditEnum);		

		if (!GetSectionValue(p,"regionoffset",param.ptOffset))
		{
			GetSectionValue(p,"groundoffset",param.ptOffset);
			if (fabs(param.ptOffset.x) < c_nWidthRegion*2 || fabs(param.ptOffset.y) < c_nHeightRegion*2)
			{
				int n;
				n = 0;
			}
			else
			{
				param.ptOffset.x -= sx * c_nWidthRegion;
				param.ptOffset.y -= sy * c_nHeightRegion;
			}
		}
//		GetSectionValue(p,"regionoffset",param.ptOffset);
//		GetSectionValue(p,"groundoffset",param.ptOffset);
		GetSectionValue(p,"groupid",param.lGroupID);
		GetSectionValue(p,"event",param.szEvent);
		GetSectionValue(p,"id",param.lID);
		GetSectionValue(p,"groupfile",param.szGroup);
		GetSectionValue(p,"template",param.szTemplate);
		GetSectionValue(p,"templateindex",param.iTemplate);

		CString strValue;
		if (GetSectionValue(p,"kind",strValue))
			param.eKind = (KindEnum)StringToPlaneTableValue(TP_KIND,strValue);

		GetSectionValue(p,"obstacletype",cast_int(param.eObstacle));
		GetSectionValue(p,"pointnum",param.nPoint);
		GetSectionValue(p,"radius",param.fRadius);
		GetSectionValue(p,"pointarray",strValue);
		if (param.nPoint == 1)
			param.aPoint[0] = cPoint(0,0,0);
		else
			StringToPointArray(strValue,param.aPoint);
	}

	g_pPack->Convert(sx,sy,&s_Obstacle,sizeof(stObstacleParam)*s_Obstacle.nObstacle+sizeof(s_Obstacle.nObstacle));

	return 0;
}

void CMapConvertDlg::PackFile(mc_doonce fun, LPCSTR szRaw, LPCSTR szPack, const CString& strFolder, const CRect& rect)
{
	g_pPack = Common_CreatePack2D();
	g_pPack->SetConvertRect(rect);
	FindFileUnderMap(fun,szRaw);	
	CString sFile;
	sFile = strFolder + "\\" + szPack;
	g_pPack->Save(sFile);
	g_pPack->Release();
}


void CMapConvertDlg::BinWorFile(CString& strFile,CRect rect)
{
	CString strPath,strName,strExt;
	AppParsePathNameExt(m_strFile,strPath,strName,strExt);
	CString strFolder = strPath + "\\" + strName;

	iItem* pItem = Common_CreateItem(m_strFile,iItem::FLAG_LOAD);
	if (pItem != NULL)
	{	
		iGround::stParam param;
		ZeroMemory(&param,sizeof(param));
		wrap_ptr<iItem> ptr(pItem);
		param.lVerson = 0;
		GetItemValue(pItem,"verson",param.lVerson);
		param.crColor = cColor(255,255,255,255);
		GetItemValue(pItem,"color",param.crColor);
		GetItemValue(pItem,"windspeed",param.ptWindSpeed);
		param.rcGround = rect;

		strFile = strFolder + "\\" + PKFILE_WOR;
		CFile f;
		if (f.Open(strFile,CFile::modeCreate | CFile::modeWrite))
		{
			f.Write(&param,sizeof(param));
			f.Close();
		}
	}
}

void CMapConvertDlg::BinCameraFile(CString& strFile)
{
	CString strPath,strName,strExt;
	AppParsePathNameExt(m_strFile,strPath,strName,strExt);
	CString strFolder = strPath + "\\" + strName;

	iItem* pItem = Common_CreateItem(strFile,iItem::FLAG_LOAD);
	if (pItem != NULL)
	{	
		iCamera::stParam param;
		wrap_ptr<iItem> ptr(pItem);
		ZeroMemory(&param,sizeof(param));
		GetItemValue(pItem,"eye",param.ptEye);
		GetItemValue(pItem,"to",param.ptTo);
		GetItemValue(pItem,"scale",param.fScale);

		strFile = strFolder + "\\" + PKFILE_CAMERA;
		CFile f;
		if (f.Open(strFile,CFile::modeCreate | CFile::modeWrite))
		{
			f.Write(&param,sizeof(param));
			f.Close();
		}
	}
}

void CMapConvertDlg::OnZipmap() 
{
	//FILE_PROPERTY,FILE_GROUND,FILE_PLANEVER,FILE_PLANEHORBELOWVER,FILE_LIGHT,FILE_NPC,FILE_OBSTACLE, need pack
	if(!VerifyFile())
		return;
	
	CString strPath,strName,strExt;
	AppParsePathNameExt(m_strFile,strPath,strName,strExt);
//	CString strFolder = m_strFile.Left(m_strFile.GetLength() - 4);
	CString strFolder = strPath + "\\" + strName;

	CWaitCursor cursor;

	CRect rect;
	GetRect(rect);

	iZipWrite* pZip = Zip_CreateZipWrite(zip_normal);
	wrap_ptr<iZipWrite> ptr(pZip);

	CString strFile;
	strFile = strFolder + ".zip";
	pZip->create(strFile);

	if (m_bGround)
	{
		PackFile(on_zip_property,FILE_PROPERTY,PKFILE_PROPERTY,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_PROPERTY;
		pZip->add(strFile,PKFILE_PROPERTY);

		PackFile(on_zip_ground,FILE_TERRAIN,PKFILE_TERRAIN,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_TERRAIN;
		pZip->add(strFile,PKFILE_TERRAIN);

		strFile = strFolder + "\\" + FILE_CAMERA;
		BinCameraFile(strFile);
		pZip->add(strFile,PKFILE_CAMERA);

		strFile = m_strFile;
		BinWorFile(strFile,rect);
		pZip->add(strFile,PKFILE_WOR);
	}
	if (m_bPlane)
	{
		PackFile(on_zip_planever,FILE_PLANEVER,PKFILE_PLANEVER,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_PLANEVER;
		pZip->add(strFile,PKFILE_PLANEVER);

		PackFile(on_zip_planehor,FILE_PLANEHORBELOWVER,PKFILE_PLANEHORBELOWVER,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_PLANEHORBELOWVER;
		pZip->add(strFile,PKFILE_PLANEHORBELOWVER);
	}
	if (m_bLight)
	{
		PackFile(on_zip_light,FILE_LIGHT,PKFILE_LIGHT,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_LIGHT;
		pZip->add(strFile,PKFILE_LIGHT);
	}
	if (m_bObstacle)
	{
		PackFile(on_zip_obstacle,FILE_OBSTACLE,PKFILE_OBSTACLE,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_OBSTACLE;
		pZip->add(strFile,PKFILE_OBSTACLE);	
//		strFile = strFolder + "\\" + BINFILE_OBSTACLE;
//		pZip->add(strFile,BINFILE_OBSTACLE);
	}

	if (m_bTrap)
	{
		strFile = strFolder + "\\" + BINFILE_TRAP;
		pZip->add(strFile,BINFILE_TRAP);
	}

	//二级模板和script文件不打包，为了修改的方便，
	//而且服务器一次读取代价不大
	if (m_bNpc)
	{
		PackFile(on_zip_npc,FILE_NPC,PKFILE_NPC,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_NPC;
		pZip->add(strFile,PKFILE_NPC);

		PackFile(on_zip_npc,FILE_CLIENTONLYNPC,PKFILE_CLIENTONLYNPC,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_CLIENTONLYNPC;
		pZip->add(strFile,PKFILE_CLIENTONLYNPC);
	}
	if (m_bObj)
	{
		PackFile(on_zip_npc,FILE_BOX,PKFILE_BOX,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_BOX;
		pZip->add(strFile,PKFILE_BOX);

		PackFile(on_zip_npc,FILE_CLIENTONLYBOX,PKFILE_CLIENTONLYBOX,strFolder,rect);
		strFile = strFolder + "\\" + PKFILE_CLIENTONLYBOX;
		pZip->add(strFile,PKFILE_CLIENTONLYBOX);
	}

	pZip->close();
}
