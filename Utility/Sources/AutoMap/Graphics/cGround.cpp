// cGround.cpp: implementation of the cGround class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cGround.h"

#include "cGraphics.h"
#include "cLight.h"
#include "cPlane.h"
#include "cPlaneVer.h"
#include "cPlaneHor.h"
#include "cCamera.h"
#include "cRegion.h"
#include "cObstacle.h"
#include "cSelect.h"

#include "cScreen.h"
#include "cMap.h"
#include "cMapSmall.h"
#include "cColorMap.h"
#include "cMapObstacle.h"

#include "iItem.h"
#include "fileop.h"

#include "cThread.h"
#include "cLock.h"

#include "Position.h"
#include "template.h"

#include "D3DUtil.h"
#include "stLine.h"
#include "coordinate.h"

#include "cTimer.h"
#include "iPack2d.h"
#include "planedoodad.h"

#include "iZip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const int c_xOff = cGround::MAX_REGION_WIDTH / 2 - 1; 
const int c_yOff = cGround::MAX_REGION_HEIGHT / 2 - 1;

//#define _DEBUG_REGION00

cGround::cGround(cGraphics* p)
{
	m_pGraphics = p;
	m_ulCount = 1;
	
	m_pThread = NULL;

	m_pSelect = NULL;
	m_pColorMap = NULL;

	for (int i=0; i<MAX_REGION_WIDTH; i++)
	for (int j=0; j<MAX_REGION_HEIGHT; j++)
		m_aRegion[i][j] = NULL;
	
	m_wRegion = 0;
	m_hRegion = 0;
	m_pTree = new cBTree;
	m_pColorMap = new cColorMap;
	m_pMapObstacle = new cMapObstacle(this);
	m_param.bBuildTree = TRUE;
	m_param.bRenderMap = FALSE;

	for (i=0; i<PACK_NUM; i++)
	{
		m_aPack[i] = NULL;
	}
	m_pZip = NULL;

	Init("");
	GetColorMap()->InitTile();

	m_param.bPlayering = FALSE;
}

cGround::~cGround()
{
	m_pTree->clear();
	delete m_pTree;
	delete m_pColorMap;
	delete m_pMapObstacle;
	for (int i=0; i<PACK_NUM; i++)
	{
		RELEASE(m_aPack[i]);
	}
	RELEASE(m_pZip);
}

/**************************************************************************/
//com
/**************************************************************************/
ULONG cGround::AddRef() 
{
	return ++m_ulCount;
}

ULONG cGround::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

//////////////////////////////////////////////////////////////////////
//Graphics
//////////////////////////////////////////////////////////////////////
cScreen* cGround::GetcScreen()
{
	return GetGraphics()->GetcScreen();
}

cMapSmall* cGround::GetcMapSmall()
{
	return GetcScreen()->GetcMapSmall();
}

cMap* cGround::GetcMap()
{
	return GetcScreen()->GetcMap();
}

iObj* cGround::GetiSelect()
{
	return m_pSelect;
}

cCamera* cGround::GetcCamera()
{
	return GetGraphics()->GetcCamera();
}

/**************************************************************************/
//trace
/**************************************************************************/
void cGround::Trace()
{
}

/**************************************************************************/
//begin and end
/**************************************************************************/
HRESULT cGround::BeginThread()
{
	ASSERT(m_pThread == NULL);
#pragma message (ATTENTION("去掉了所有多线程的地方!"))
	m_pThread = new cThread;
	m_pThread->RunThread(Thread,this,THREAD_PRIORITY_NORMAL);
	return S_OK;
}

HRESULT cGround::EndThread()
{
	if (m_pThread == NULL)
		return S_OK;

	ASSERT(m_pThread);
	m_pThread->EndThread();	
	delete m_pThread;
	m_pThread = NULL;

	return S_OK;
}

HRESULT cGround::Begin()
{
	GetGraphics()->CreateGraphicsElement(GT_SELECT,&m_pSelect);
	//create a thread to autoload the dark region!
	InitializeCriticalSection(&m_csUpdateRegionPosOrSize);

	return S_OK;
}

HRESULT cGround::End()
{
	RELEASE(m_pSelect);

	EndThread();

	DeleteCriticalSection(&m_csUpdateRegionPosOrSize);

	return S_OK;
}

HRESULT cGround::Run()
{
	HRESULT hr = S_OK;
	for (itSpecial it = m_listSpecial.begin(); it != m_listSpecial.end(); ++it)
	{
		iSpecial* p = *it;
		if (p->IsActive())
			p->Run();
		else
			p->Active();
	}
/*
	for (int x = 0; x<m_wRegion; x++)
	for (int y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p)
			p->Run(rc);
	}
*/
	return hr;
}

/**************************************************************************/
//thread
/**************************************************************************/
void cGround::SetLoadedRegion(cRegion* pRegion)
{
	#pragma message (ATTENTION("未完成的函数，在进程中可能会被new，load打断"))
	int x,y;
	pRegion->GetPos(x,y);

	RegionToCoordinate(x,y);

	if (x < 0 || y < 0 || x >= MAX_REGION_WIDTH || y >= MAX_REGION_HEIGHT)
	{
		pRegion->Release();
		return;
	}

	if (m_aRegion[x][y] != NULL)
	{
		ASSERT(0);
//		pRegion->Release();
		return;
	}
	m_aRegion[x][y] = pRegion;
	m_listRegion.push_back(pRegion);
	return;
}

BOOL cGround::GetNearestUnloadRegion(int& rx,int& ry, BOOL bTexture)
{
//	cRect r = GetRectTileFormat();
	int w,h;
	GetRegionSize(w,h);
	const int c_nWExtra = 0;
	const int c_nHExtra = 0;
	float fw = (float)w/2 + 1.5f;
	float fh = (float)h/2 + 1.5f;
//	int tx,ty;
//	ComputeRegionPosBySize(tx,ty,w,h,r);

	#pragma message (ATTENTION("sort not consider!"))

	//use tile not ground to compute this!
	cPoint pt = GetcCamera()->GetFocus();
	GroundToTile(pt);
	TileToRegion(pt);

	float dis = fw*fh;
	int nx = -1;
	int ny = -1;
	BOOL bFind = FALSE;

	for (int x = 0; x < MAX_REGION_WIDTH; x++ )
	for (int y = 0; y < MAX_REGION_HEIGHT; y++ )
	{
		BOOL b = FALSE;
		if (!bTexture)
		{
			if (m_aRegion[x][y] == NULL)
				b = TRUE;
		}
		else
		{
			if (m_param.bRenderMap)
			{
				if (x != c_xOff + 1 || y != c_yOff + 1)
					continue;
			}
			if (m_aRegion[x][y])
			{
				if (m_aRegion[x][y]->m_pTexture == NULL)
				{
					m_aRegion[x][y]->PrepareTerrainTexture();
				}
				ASSERT(m_aRegion[x][y]->m_pTexture);
				if (m_aRegion[x][y]->m_bUpdateTexture == FALSE)
					b = TRUE;
			}
		}
		if (b)
		{
			int nnx = x;
			int nny = y;
			CoordinateToRegion(nnx,nny);
			float fx = nnx + 0.5f;
			float fy = nny + 0.5f;
//			fx += 0.5f;		//中点
//			fy += 0.5f;
			float d = (fx - pt.x)*(fx - pt.x) + (fy - pt.y)*(fy - pt.y);
			if (d < dis)
			{
				dis = d;
				nx = x;
				ny = y;
				bFind = TRUE;
			}
		}
	}
	rx = nx;
	ry = ny;
	return bFind;
}

cRegion* cGround::ThreadAddcRegion(int x,int y, LPCSTR szFolder)
{
//	cTimerFunction timer("cGround::threadaddcregion");

	cRegion* p = new cRegion(GetGraphics());
	p->SetPaintPlane(TRUE);
	p->SetPaintTerrain(FALSE);

	ASSERT(x>=0 && y>=0);

	p->SetPos(x,y);
	//基本信息
	p->Load(szFolder);
	//动态信息

	iRegion::stParam param;
 	param.mask = iRegion::MASK_MAPEDIT;
	param.lMapeditEnum = m_param.lMapeditEnum;
	param.eOPMapedit = Bit_Set;
	p->SetParam(param,true);
//	m_listRegion.push_back(p);

	CString sMessage;
	sMessage.Format("region %d_%d load by another thread!\n",x,y);
	OutputDebugString(sMessage);

	return p;
}

BOOL cGround::UpdateNearestRegionTexture()
{
	int nx,ny;
	if (!GetNearestUnloadRegion(nx,ny,TRUE))
		return FALSE;
	//return coordinate 
	if (nx < 0 || ny < 0)
		return FALSE;
	
	CoordinateToRegion(nx,ny);
	if (nx < 0 || ny < 0)
		return FALSE;

	cRegion* pRegion = GetcRegionByRegion(nx,ny);
	ASSERT(pRegion);
	ASSERT(pRegion->m_pTexture);

	int x = nx;
	int y = ny;

	int i,j;
	for (i=-1; i<2; i++)
	for (j=-1; j<2; j++)
	{
		cRegion* pRegionLink = GetcRegionByRegion(x+i,y+j);
		if (pRegionLink == NULL)
			return FALSE;				
	}
	pRegion->SetPaintTerrain(FALSE);

	for (i=-1; i<2; i++)
	for (j=-1; j<2; j++)
	{
		if (i != 0 || j != 0)
		{
			cRegion* pRegionLink = GetcRegionByRegion(x+i,y+j);
			pRegion->AddLinkHorFromRegion(pRegionLink);
		}
	}

//	pRegion->PrepareTerrainTexture();
	pRegion->UpdateHorTexture();
//
//	pRegion->RemoveLinkHor();

	pRegion->SetPaintTerrain(TRUE);
	
	return TRUE;
}

BOOL cGround::LoadNearestRegion()
{
	int nx,ny;
	{
		cLock lock(&m_csUpdateRegionPosOrSize);
		if (!GetNearestUnloadRegion(nx,ny,FALSE))
			return FALSE;
		//return coordinate 
		if (nx < 0 || ny < 0)
			return FALSE;
		CoordinateToRegion(nx,ny);
		if (nx < 0 || ny < 0)
			return FALSE;
	}

	CString strFold ;
	GetGroundFolder(strFold);

	cRegion* pRegion = ThreadAddcRegion(nx,ny,strFold);

	//我们可以操作region的贴图因为确信只有这个时刻
	//主线程会访问这个texture，
	//也就是说，主线程只在updateregion时访问region的texture
	//如果想load spr的file需要更高的同步要求

	//add horlink to the near region!
	pRegion->BeginPainting();
	
	if (!m_param.bRenderMap)
	{
//		pRegion->PrepareTerrainTexture();
	}
	//现在graphics 只能通过临界区来访问，是安全的
	
	{
		cLock lock(&m_csUpdateRegionPosOrSize);
		SetLoadedRegion(pRegion);
	}
	
	return TRUE;
}

UINT cGround::ThreadRun()
{
	const int c_nSleep = 10;
	int n = 0;
	BOOL b = FALSE;

	while (m_pThread->WaitKillEvent(0) == WAIT_TIMEOUT)
	{
		if (b)
		{
			cLock lock(&m_csUpdateRegionPosOrSize);
			b = UpdateNearestRegionTexture();
		}
		if (LoadNearestRegion())
		{
			b = TRUE;
		}
		if (b)
			Sleep(0);
		else
			Sleep(10);
	}
	m_pThread->SetEndEvent();

	return 0;
}

UINT cGround::Thread(LPVOID lParam)
{
	OutputDebugString("\nGround thread start.\n\n");

	cGround* pGround = (cGround*)lParam;
	pGround->ThreadRun();

	OutputDebugString("\nGround thread exit.\n\n");
	
	return 0;
}

/**************************************************************************/
//param
/**************************************************************************/

HRESULT cGround::GetParam(stParam& param) 
{
	if (param.mask == MASK_COMMAND)
	{
		switch(param.eCommand)
		{
		case GCOM_GETTILE:
			return GetRegionTile(param.xPos,param.yPos,param.tile);
		}
	}
	else if (param.mask == 0)
		param = m_param;	
	return 0;
}

HRESULT cGround::SetParam(stParam& param, bool bRunNow)
{
	if (bRunNow)
		return setParam(param);
	AddRef();
	param.wParam = (long)this;
	param.lParam = 0; 
	GetGraphics()->PushMessage(&param,sizeof(param),GT_GROUND);
	return 0;
}

HRESULT cGround::UpdateVertex(int x,int y, stVertexGround v)
{
	cRegion* pRegion = GetcRegionByTile(cPoint(x,y));
	if (pRegion)
		pRegion->SetTileVertex(x,y,v);
	else
		return -1;
	return S_OK;
}

HRESULT cGround::setParam(const stParam& param) 
{
	if (param.mask == MASK_COMMAND)
	{
		switch(param.eCommand)
		{
		case GCOM_VERTEXUPDATE:
			UpdateVertex(param.xPos,param.yPos,param.tile.v);
			break;
		case GCOM_NEW:
			New(param.szFile);
			break;
		case GCOM_LOAD:
			{
				CString s = param.szFile;
				AppGetFullPath(s);
				Load(s);
			}
			break;
		case GCOM_SAVE:
			{
				CString s = param.szFile;
				AppGetFullPath(s);
				Save(s);
			}
			break;
		case GCOM_NEWDYNAMIC:
			m_param.szDynamic[0] = 0;
			break;
		case GCOM_OPENDYNAMIC:
//			DynamicOpen(param.szDynamic);
			strcpy(m_param.szDynamic,param.szDynamic);
			break;
		case GCOM_SAVEDYNAMIC:
//			DynamicSave(param.szDynamic);
			break;
		case GCOM_ADDOBJ:
			AddcObj(param.pObj);
			param.pObj->Release();
			break;
		case GCOM_REMOVEOBJ:
			RemovecObj(param.pObj);
			param.pObj->Release();
			break;
		case GCOM_FILLREGION:
			return FillRegion(param.rcFill,param.tile.szResource);
		case GCOM_CHANGETILE:
			return SetRegionTile(param.xPos,param.yPos,param.tile);
		case GCOM_UPDATEREGIONTEXTURE:
			return UpdateRegionTexture();
		}
	}
	if (param.mask & MASK_WINDSPEED)
		m_param.ptWindSpeed = param.ptWindSpeed;
	if (param.mask & MASK_OBSTACLE_TYPE)
		m_param.eObstalceType = param.eObstalceType;
//	if (param.mask & MASK_BRIGHTNESS)
//		m_param.crColor.a = param.crColor.a;
	if (param.mask & MASK_COLOR)
		m_param.crColor = param.crColor;
	if (param.mask & MASK_UPDATEREGIONTEXTURE)
		m_param.bUpdateRegionTexture = param.bUpdateRegionTexture;
	if (param.mask & MASK_RENDERMAP)
		m_param.bRenderMap = param.bRenderMap;
	if (param.mask & MASK_BUILDTREE)
	{
		m_param.bBuildTree = param.bBuildTree;
		if (m_param.bBuildTree == TRUE)
		{
			RebuildTree();
		}
	}
	if (param.mask & MASK_MAPEDIT)
	{
		BitOP(m_param.lMapeditEnum,param.eOPMapedit,param.lMapeditEnum);
		iRegion::stParam param;
		param.mask = iRegion::MASK_MAPEDIT;
		param.lMapeditEnum = m_param.lMapeditEnum;
		param.eOPMapedit = Bit_Set;
		for (itcRegion it = m_listRegion.begin(); it!= m_listRegion.end(); ++it)
			(*it)->SetParam(param, true);
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////
//new load and save
//////////////////////////////////////////////////////////////////////
HRESULT cGround::Init(LPCSTR szGround)
{
	//删除临时的texture

	FinalCleanup();

	m_xRegion = -1;
	m_yRegion = -1;
	int b = m_param.bPlayering;
	ZeroMemory(&m_param,sizeof(m_param));

	m_param.bPlayering = b;
	m_param.bUpdateRegionTexture = TRUE;
	m_param.bBuildTree = TRUE;
	m_param.crColor = cColor(255,255,255,255);

	CString sWor = szGround;
	sWor = sWor.Left(sWor.GetLength() - 3)+"wor";
	strcpy(m_param.szFile,sWor);

	m_param.ptWindSpeed = cPoint(0.1f,0,0);
	m_param.eObstalceType = OT_TILE;

	stResourceA resPos;
	resPos.stDevice.eUsage = USAGE_REGION;
	GetGraphics()->ClearTexture(resPos);

	m_bRebuildRegion = TRUE;

	for (int i=0; i<PACK_NUM; i++)
	{
		RELEASE(m_aPack[i]);
	}
	RELEASE(m_pZip);

	return S_OK;
}

BOOL cGround::IsModify()
{
	for (itcRegion it = m_listRegion.begin(); it != m_listRegion.end(); ++it)
	{
		cRegion* p = (*it);
		if (p->IsModify(MASK_ALL))
			return TRUE;
	}
	return FALSE;
}

HRESULT cGround::AskSave()
{
	return S_OK;
	BOOL bModify = FALSE;
	if (IsModify())
	{
		if (AfxMessageBox("当前的文件已经改变，存储么？",MB_YESNO) == IDYES)
		{
			CString strFolder;
			GetGroundFolder(strFolder);
			for (itcRegion it = m_listRegion.begin(); it != m_listRegion.end(); ++it)
			{
				cRegion* p = (*it);
				p->Save(strFolder);
			}
		}
	}
	return S_OK;
}

CString g_aPackFileString[PACK_NUM] = 
{
	PKFILE_NPC,
	PKFILE_BOX,
	PKFILE_TRAP,
	PKFILE_CLIENTONLYNPC,
	PKFILE_CLIENTONLYBOX,
	PKFILE_CLIENTONLYTRAP,
	PKFILE_PLANEVER,
	PKFILE_PLANEVERBELOWGROUND,
	PKFILE_PLANEHORBELOWVER,
	PKFILE_LIGHT,
	PKFILE_OBSTACLE,
	PKFILE_TERRAIN,
	PKFILE_PROPERTY,
};

HRESULT cGround::LoadWor()
{
	if (m_pZip != NULL)
	{
		if (m_pZip->Open(PKFILE_WOR))
		{
			stParam param;
			m_pZip->Read(&param,sizeof(param));
			m_param.crColor = param.crColor;
			m_param.ptWindSpeed = param.ptWindSpeed;
			m_param.lVerson = param.lVerson;
			m_param.rcGround = param.rcGround;
			if (m_param.crColor == cColor(0,0,0,0))
				m_param.crColor = cColor(255,255,255,255);
		}
	}
	else
	{
		iItem* pItem = Common_CreateItem(m_param.szFile,iItem::FLAG_LOAD);
		if (pItem != NULL)
		{	
			wrap_ptr<iItem> ptr(pItem);
			GetItemValue(pItem,"verson",m_param.lVerson);
			GetItemValue(pItem,"color",m_param.crColor);
			GetItemValue(pItem,"windspeed",m_param.ptWindSpeed);
		}
	}
	return TRUE;
}

HRESULT cGround::Load(LPCSTR szGround)
{
	if (IsEditing())
		AskSave();

	if (strlen(szGround) == 0)
	{
		ASSERT(0);
		return -1;
	}

	EndThread();
	
	Init(szGround);

	//load ground data
	CString strExt;
	CString strFold;
	AppParseFileName(szGround,strFold,strExt);
	CString strZip = strFold + ".zip";
	m_pZip = Zip_CreateZipRead(zip_normal);
	if (!m_pZip->load(strZip))
	{
		RELEASE(m_pZip);
	}

	LoadWor();

	//then info the map and smallmap
	GetcMap()->OnFileOpen(szGround);
	GetcMapSmall()->OnFileOpen(szGround);

	//load camera
//	if (!IsPlaying())
	{
		CString strCam = strFold + "\\" + FILE_CAMERA;
		GetcCamera()->Load(strCam);
	}

	//load special
	CString strSpecial = strFold + "\\" + FILE_SPECIAL;
	LoadSpecial(strSpecial);

	for (int i=0; i<PACK_NUM; i++)
	{
		m_aPack[i] = Common_CreatePack2D();
		BOOL bGet = FALSE;
		if (m_pZip)
		{
			if (m_aPack[i]->Open(g_aPackFileString[i],m_pZip))
			{
				bGet = TRUE;
			}
		}
		if (!bGet)
		{
			CString strPack = strFold + "\\" + g_aPackFileString[i];
			if (!m_aPack[i]->Open(strPack,NULL))
				RELEASE(m_aPack[i]);
		}
	}
	
	BeginThread();
	
	return S_OK;
}

HRESULT cGround::New(LPCSTR szGround)
{
	AskSave();

	EndThread();
	
	Init(szGround);
	
	GetcMap()->OnFileOpen(szGround);
	GetcMapSmall()->OnFileOpen(szGround);

	if (GetcCamera())
		GetcCamera()->New();
	m_param.crColor = cColor(255,255,255,255);

	BeginThread();


	return 0;
}


HRESULT cGround::Save(LPCSTR szGround)
{
#ifdef GET_FILELIST
	CStdioFile f;
	CString s = "filelist.txt";
	AppGetFullPath(s);
	if (f.Open(s,CFile::modeCreate|CFile::modeWrite))
	{
		for (itFile it = g_mapFile.begin(); it != g_mapFile.end(); ++it)
		{
			CString strTxt = (*it).first.c_str();
			f.WriteString(strTxt);
			f.WriteString("\n");
			CString strSpr = strTxt.Left(strTxt.GetLength() - 3)+"spr";
			AppGetFullPath(strSpr);
			CString strDes = strSpr;
			strDes.Replace("游戏资源","FileList");
			AppBuildPath(strDes);
			AppCopyFile(strSpr,strDes);
		}
	}
	f.Close();
#endif
	if (strlen(szGround) == 0)
	{
		ASSERT(0);
		return -1;
	}
	CString sWor = szGround;
	sWor = sWor.Left(sWor.GetLength() - 3)+"wor";
	strcpy(m_param.szFile,sWor);

	//save ground data
	iItem* pItem = Common_CreateItem(szGround,iItem::FLAG_LOAD);
	if (pItem == NULL)
		pItem = Common_CreateItem(szGround,iItem::FLAG_CREATE);
	ASSERT(pItem);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"verson",m_param.lVerson);
	SetItemValue(pItem,"color",m_param.crColor);
	SetItemValue(pItem,"windspeed",m_param.ptWindSpeed);
	SetItemValue(pItem,"event",m_param.szDynamic);
	pItem->Save();

	//save camera
	CString strExt;
	CString strFold;
	AppParseFileName(szGround,strFold,strExt);
	AppBuildPath(strFold);

	CString strCam = strFold + "\\" + FILE_CAMERA;
	GetcCamera()->Save(strCam);
	CString strSpecial = strFold + "\\" + FILE_SPECIAL;
	SaveSpecial(strSpecial);
	
	//存储静态的
	SaveStatic(strFold);

	return S_OK;
}

HRESULT cGround::SaveStatic(CString& strFold)
{
	for_all(m_listRegion,bind2nd(mem_fun1<HRESULT,cRegion,LPCSTR>(&cRegion::Save),(LPCSTR)strFold));
	return S_OK;
}

/////////////////////////////////////////////////////////////////
//dynamic
/////////////////////////////////////////////////////////////////

BOOL cGround::FindDynamicNick(LPCSTR szNick)
{
	for(itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++it)
	{
		cGroundObj* p = (*it);
		CString s = p->GetEvent();
		if (s.CompareNoCase(szNick) == 0)
			return TRUE;
	}
	return FALSE;
}

HRESULT cGround::ClearDynamic()
{
	//remove from lightlist and verlist
	for_all(m_listDynamicObj,bind2nd(mem_fun1<BOOL,cGroundObj,BOOL>(&cGroundObj::SetInGround),FALSE));
	for_all(m_listDynamicObj,mem_fun<HRESULT,cGroundObj>(&cGroundObj::EndPainting));
	for_all(m_listDynamicObj,mem_fun<ULONG,cGroundObj>(&cGroundObj::Release));
	m_listDynamicObj.clear();
	return S_OK;
}

BOOL cGround::IsDisplayEvent(LPCSTR szEvent)
{
	CString s = szEvent;
	if (s.IsEmpty())
		return TRUE;
	if (strlen(m_param.szDynamic) == 0)
		return TRUE;
	if (s.CompareNoCase(m_param.szDynamic) == 0)
		return TRUE;
	return FALSE;
}


void cGround::GetGroundFolder(CString& str)
{
	CString strPath = m_param.szFile;
	if (strPath.IsEmpty())
		return ;
	//the file ext must be ".wor"
	str = strPath.Left(strPath.GetLength() - 4);
}

/*
void cGround::GetRegionFolder(CString& strFolder, LPCSTR path, int x,int y)
{
	strFolder.Format("%s\\%04d_%04d",path,x,y);
}
*/

//////////////////////////////////////////////////////////////////////
//region function
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//根据坐标得到region的指针
//////////////////////////////////////////////////////////////////////
cRegion* cGround::GetFocusRegion()
{
	return GetcRegion(1,1);
//	cPoint pt = GetcCamera()->GetFocus();
//	return GetcRegionByGround(pt);
}

cRegion* cGround::GetcRegionByScreen(CPoint pt)
{
	cPoint ptGround;
	ScreenToGround(pt,ptGround);
	return GetcRegionByGround(ptGround);
}

iRegion* cGround::GetiRegion(cPoint ptGround)
{
	cRegion* p = GetcRegionByGround(ptGround);
	if (p != NULL)
		p->AddRef();
	return p;
}

cRegion* cGround::GetcRegionByGround(cPoint pt)
{
	GroundToTile(pt);
	return GetcRegionByTile(pt);
}

cRegion* cGround::GetcRegionByTile(cPoint pt)
{
	TileToRegion(pt);
	return GetcRegionByRegion(pt);
}

cRegion* cGround::GetcRegionByRegion(int tx,int ty)
{
	RegionToCoordinate(tx,ty);
	return GetcRegionByCoordinate(tx,ty);
}

cRegion* cGround::GetcRegionByRegion(cPoint ptRegion)
{
	return GetcRegionByRegion(ptRegion.x,ptRegion.y);
}

cRegion* cGround::GetcRegion(int tx,int ty)
{
	RegionOffset(tx,ty);
	return GetcRegionByCoordinate(tx,ty);
}

cRegion* cGround::GetcRegionByCoordinate(int tx,int ty)
{
	if (!IsRegionCoordinateValid(tx,ty))
		return NULL;
	return m_aRegion[tx][ty];
}

BOOL cGround::IsRegionCoordinateValid(int x,int y)
{
	BOOL b = (x >=0 && x < MAX_REGION_WIDTH && y >= 0 && y < MAX_REGION_HEIGHT);
	return b;
}

void cGround::RegionToCoordinate(int& tx,int& ty)
{
	tx -= m_xRegion;
	tx += c_xOff;
	ty -= m_yRegion;
	ty += c_yOff;
}

void cGround::CoordinateToRegion(int& tx,int& ty)
{
	tx += m_xRegion;
	tx -= c_xOff;
	ty += m_yRegion;
	ty -= c_yOff;
}

void cGround::RegionOffset(int& tx,int& ty)
{
	tx += c_xOff;
	ty += c_yOff;
}

void cGround::RegionUnoffset(int& tx,int& ty)
{
	tx -= c_xOff;
	ty -= c_yOff;
}

bool cGround::IsCoordinateInPaint(int tx,int ty)
{
	RegionUnoffset(tx,ty);
	if (tx >=0 && tx < m_wRegion && ty >= 0 && ty < m_hRegion)
		return true;
	return false;
}

bool cGround::IsCoordinateInPaint2(int tx,int ty)
{
	RegionUnoffset(tx,ty);
	if (tx >=-1 && tx <= m_wRegion && ty >= -1 && ty <= m_hRegion)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////
//得到region的范围
//////////////////////////////////////////////////////////////////////
cRect cGround::GetRectGround()
{
	int width,height;
	GetGraphics()->GetScreen(width,height);
	cPoint p[4];
	p[0] = cPoint(0,0,0);
	p[1] = cPoint(width,0,0);
	p[2] = cPoint(0,height,0);
	p[3] = cPoint (width,height,0);
	cRect r = c_rectMin;
	for (int i=0; i<4; i++)
	{
		GetcCamera()->ScreenToGround(p[i]);
		r.FormatMinMax(p[i]);
	}
	return r;
}

cRect cGround::GetRectGroundFormat()
{
	cRect r = GetRectGround();
	r.FormatMin(0,0);
	return r;
}

cRect cGround::GetRectTile()
{
	cRect r = GetRectGround();
	GroundToTile(r.l,r.t);
	GroundToTile(r.r,r.b);	
	return r;
}

cRect cGround::GetRectTileFormat()
{
	cRect r = GetRectTile();
	r.FormatMin(0,0);
	return r;
}

void cGround::GetRegionSize(int &w, int &h)
{
	w = m_wRegion;
	h = m_hRegion;
}

void cGround::ComputeRegionSize(int& w, int& h)
{
	cRect r = GetRectTile();
	w = r.Width();
	h = r.Height();
	TileToRegion(w,h);
	w += 2;
	h += 2;
}

void cGround::ComputeRegionPosBySize(int& x,int& y, int w, int h, const cRect& r)
{
	//Get the center of the rect
	float fx = (r.l + r.r) / 2.f;
	float fy = (r.t + r.b) / 2.f;
	TileToRegion(fx,fy);
	fx -= w/2.f;
	fy -= h/2.f;

	if (fx < 0)
		x = 0;
	else 
	{
		x = fx;
		if ( fx - x > 0.5f )
			x ++;
	}
	if (fy < 0)
		y = 0;
	else 
	{
		y = fy;
		if ( fy - y > 0.5f )
			y ++;
	}
}

void cGround::RecomputeRegionPos(int& x,int& y)
{
	cRect r = GetRectTileFormat();
	ComputeRegionPosBySize(x,y,m_wRegion,m_hRegion,r);
}

cPoint cGround::GetCenterTileByRegion() //get center according to m_xRegion,m_yRegion
{
	cRect rcRegion;
	rcRegion.l = m_xRegion ;
	rcRegion.t = m_yRegion ;
	rcRegion.r = (m_xRegion + m_wRegion) ;
	rcRegion.b = (m_yRegion + m_hRegion) ;
	RegionToTile(rcRegion.l,rcRegion.t);
	RegionToTile(rcRegion.r,rcRegion.b);
	return cPoint((rcRegion.l+rcRegion.r)/2,(rcRegion.t+rcRegion.b)/2);	
}

//重新写这个函数
bool cGround::IsRegionCoverScreen()
{
	if (m_xRegion < 0 || m_yRegion < 0)
		return false;
	cPoint pt = GetCenterTileByRegion();
	cPoint point = GetcCamera()->GetFocus();
	GroundToTile(point);
	#pragma message (ATTENTION("图素必须限制大小跟高度"))
	const float c_fExtra = 0.5f;
	if (abs (point.x-pt.x) > c_tWidthRegion * c_fExtra)
	{
		if (m_xRegion == 0 && point.x < pt.x)	
			return true;
		return false;
	}
	else if (abs (point.y-pt.y) > c_tHeightRegion * c_fExtra)
	{
		if (m_yRegion == 0 && point.y < pt.y)
			return true;
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
//region clear
//////////////////////////////////////////////////////////////////////
HRESULT cGround::ClearRegionList(cRegionList& rlist)
{
	for (itcRegion it = rlist.begin(); it != rlist.end(); ++it)
		(*it)->Release();
	rlist.clear();
	return S_OK;
}

HRESULT cGround::RemoveRegion(cRegion* pRegion)
{
	int size = m_listRegion.size();
	m_listRegion.remove(pRegion);
	ASSERT(m_listRegion.size() < size);
	pRegion->FinalCleanup();
	int x,y;
	pRegion->GetPos(x,y);
	RegionToCoordinate(x,y);
	ASSERT(IsRegionCoordinateValid(x,y));
	//release twice
	m_aRegion[x][y] = NULL;
	pRegion->Release();
//	RELEASE(m_aRegion[x][y]);
//	pRegion->Release();
	return S_OK;
}

HRESULT cGround::ClearGround()
{
	m_listPlaneVerPoint.clear();
	m_listPlaneVerSeam.clear();
	for (itALine it = m_aLine.begin(); it != m_aLine.end(); ++it)
	{
		delete (*it);
	}
	m_aLine.clear();
	m_listPlaneVer.clear();

	m_listPlaneAboveVer.clear();
	m_listLight.clear();
	m_pTree->clear();

	GetColorMap()->ClearTile();

	return S_OK;
}

HRESULT cGround::ClearAllRegion()
{
	for (int i=0; i<MAX_REGION_WIDTH; i++)
	for (int j=0; j<MAX_REGION_HEIGHT; j++)
	{
		cRegion* pRegion = m_aRegion[i][j];
		if (pRegion)
			RemoveRegion(pRegion);
	}
	return S_OK;
}

HRESULT cGround::ClearFarestRegion()
{
	int w,h;
	GetRegionSize(w,h);
	//left + 1 and right + 1
	if (m_listRegion.size() < (w+1+1)*(h+1+1))
		return S_OK;

	cPoint pt = GetcCamera()->GetFocus();
	GroundToTile(pt);
	TileToRegion(pt);
	return 	ClearFarestRegion(pt);
}	

HRESULT cGround::ClearFarestRegion(cPoint ptFocusTile)
{
	float maxdis = MIN_LONG;
	itcRegion itGet;
	bool bGet = false;

	for (itcRegion it = m_listRegion.begin(); it!= m_listRegion.end(); ++it)
	{
		cRegion* pRegion = (*it);
		if (pRegion->IsPaintTerrain() || pRegion->IsPaintPlane())
			continue;
		int x,y;
		pRegion->GetPos(x,y);
		float dis = fabs(x + 0.5f - ptFocusTile.x) + 
					fabs(y + 0.5f - ptFocusTile.y) ;	
		if (dis > maxdis)
		{
			maxdis = dis;
			itGet = it;
			bGet = true;
		}
	}
	if (bGet)
	{
		cRegion* pRegion = (*itGet);
		CString strFolder;
		GetGroundFolder(strFolder);
		ASSERT(!strFolder.IsEmpty());
		ASSERT(!pRegion->IsPaintTerrain());
		ASSERT(!pRegion->IsPaintPlane());
		pRegion->Save(strFolder);

#ifdef _DEBUG_GROUND	
		int x,y;
		pRegion->GetPos(x,y);
		CString sPath;
		sPath.Format("clear farest Region: %s\\%03d_03%d\\ start\n",strFolder,x,y);
		OutputDebugString(sPath);
#endif	

		RemoveRegion(pRegion);
	}
	else
	{
//		ASSERT(0);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//update and rebuild
//////////////////////////////////////////////////////////////////////
HRESULT cGround::UpdateRegion()
{
	//缩放变化
	int wRegion,hRegion;
	ComputeRegionSize(wRegion,hRegion);
	if (m_wRegion != wRegion || m_hRegion != hRegion)
	{
		m_wRegion = wRegion;
		m_hRegion = hRegion;

		return OffsetRegion();
	}

	//移动出了屏幕
	if (!IsRegionCoverScreen())
	{
		OffsetRegion();
		GetcMap()->OnRegionUpdate(m_xRegion+m_wRegion/2,m_yRegion+m_hRegion/2);
		//统一来更新3*3中所有的图素
	}

//先不使用多线程的部分
	return S_OK;
}

HRESULT cGround::OffsetRegion()
{
	cTimerFunction timer("OffsetRegion");

	m_bRebuildRegion = FALSE;

	ClearGround();

	int x,y;
	RecomputeRegionPos(x,y);

	cLock lock(&m_csUpdateRegionPosOrSize);

	cRegion* aRegion[MAX_REGION_WIDTH][MAX_REGION_HEIGHT];
	int i,j;
	for (i=0; i<MAX_REGION_WIDTH; i++)
	for (j=0; j<MAX_REGION_HEIGHT; j++)
	{
		aRegion[i][j] = m_aRegion[i][j];
		m_aRegion[i][j] = NULL;
	}

	for (i=0; i<MAX_REGION_WIDTH; i++)
	for (j=0; j<MAX_REGION_HEIGHT; j++)
	{
		if (aRegion[i][j] == NULL)
			continue;
		int tx,ty;
		tx = i + m_xRegion - x;	 			
		ty = j + m_yRegion - y;
		if (IsRegionCoordinateValid(tx,ty))
			m_aRegion[tx][ty] = aRegion[i][j];
		else //region 远远超出了范围
		{
			CString strFolder;
			GetGroundFolder(strFolder);
			aRegion[i][j]->Save(strFolder);
			int size = m_listRegion.size();
			m_listRegion.remove(aRegion[i][j]);
			ASSERT(m_listRegion.size() < size);
			aRegion[i][j]->FinalCleanup();
			//release twice
			RELEASE(aRegion[i][j]);
			RELEASE(aRegion[i][j]);				
		}
	}

	//在新的位置上面
	m_xRegion = x;
	m_yRegion = y;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//region add
//////////////////////////////////////////////////////////////////////

HRESULT cGround::RebuildRegion()
{
	cTimerFunction timer("RebuildRegion");

	int x,y;

	m_param.bBuildTree = FALSE;

//	cLock lock(&m_csUpdateRegionPosOrSize);

	{
		cTimerFunction timer("LoadDarkRegion : prepareterrainture");	
		for (x = 0; x < MAX_REGION_WIDTH; x++)
		for (y = 0; y < MAX_REGION_HEIGHT; y++)
		{
			if (m_aRegion[x][y])
				m_aRegion[x][y]->SetPaintTerrain(FALSE);
		}
	}

	{
		cTimerFunction timer("LoadDarkRegion : add obj");	
		for (x = 0; x < m_wRegion; x++)
		for (y = 0; y < m_hRegion; y++)
		{
			int tx = x;
			int ty = y;
			RegionOffset(tx,ty);
			ASSERT(m_aRegion[tx][ty] != NULL);
			cPlaneList& rlist = m_aRegion[tx][ty]->m_listPlane;
			for (itcPlane it = rlist.begin(); it != rlist.end(); ++ it)
			{
				AddPlane(*it, m_aRegion[tx][ty],false);
			}
			cLightList& llist = m_aRegion[tx][ty]->m_listLight;
			for (itcLight itl = llist.begin(); itl != llist.end(); ++ itl)
				AddLight(*itl);		
		}
	}

	{
		for (x = 0; x < MAX_REGION_WIDTH; x++)
		for (y = 0; y < MAX_REGION_HEIGHT; y++)
		{
			if (m_aRegion[x][y])
				m_aRegion[x][y]->SetPaintTerrain(TRUE);
		}
	}

	m_param.bBuildTree = TRUE;
	RebuildTree();
	GetMapObstacle()->Build();

//	if (!m_param.bRenderMap)
		RebuildColorMap();

	m_bRebuildRegion = TRUE;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//操作Region中的一些元素
//////////////////////////////////////////////////////////////////////
void cGround::RepaintTerrain()
{
	for (itcRegion it = m_listRegion.begin(); it != m_listRegion.end(); ++it)
		(*it)->RepaintTerrain();
}

HRESULT cGround::UpdateRegionTexture()
{
	int x,y;
	for (x = 0; x < m_wRegion; x++)
	for (y = 0; y < m_hRegion; y++)
	{
		cRegion* pRegion = GetcRegion(x,y);
		if (!pRegion)
			continue;
		if (pRegion ->IsTextureDirty() )
			pRegion->UpdateGroundTexture();
	}
	return S_OK;
}

int cGround::GetRegionNum()
{
	return m_listRegion.size();
}

int cGround::GetRegionMemory()
{
	return GetRegionNum()*sizeof(cRegion)/1024/1024;
}

cRegion* cGround::GetLinkRegion(cRegion* p, eRegionDirection e)
{
	int x,y;
	p->GetPos(x,y);
	RegionToCoordinate(x,y);
	ASSERT(m_aRegion[x][y]);
	switch (e)
	{
	case D_0:
		y--;
		break;
	case D_1:
		x++;
		y--;
		break;
	case D_3:
		x++;
		break;
	case D_5:
		x++;
		y++;
		break;
	case D_6:
		y++;
		break;
	case D_7:
		x--;
		y++;
		break;
	case D_9:
		x--;
		break;
	case D_11:
		x--;
		y--;
		break;
	case D_12:
		y--;
		break;
	}
	if (IsRegionCoordinateValid(x,y))
		return m_aRegion[x][y];
	return NULL;
}

HRESULT cGround::GetRegionTile(int x,int y,stTile& Tile)
{
	cRegion* pRegion = GetcRegionByTile(cPoint(x,y));
	if (pRegion)
		pRegion->GetTile(x,y,Tile);
	else
		return -1;
	return S_OK;
}

HRESULT cGround::SetRegionTile(int x,int y,const stTile& Tile)
{
	cRegion* pRegion = GetcRegionByTile(cPoint(x,y));
	if (pRegion)
		pRegion->SetTile(x,y,Tile);
	else
		return -1;
	return S_OK;
}

HRESULT cGround::FillRegion(CRect rcFill,LPCSTR szSprFile)
{
	for (int x = rcFill.left; x < rcFill.right; x ++)
	for (int y = rcFill.top ; y < rcFill.bottom; y++ )
	{
		HRESULT hr = FillRegion(x,y,szSprFile);
		if (FAILED(hr))
			return hr;
	}
	return S_OK;
}

HRESULT	cGround::BackFile(int tx, int ty, LPCSTR szFile)
{
	if (tx < 0 || ty < 0)
		return -1;
	cRegion* pRegion = GetcRegionByRegion(tx,ty);
	CString s;
	if (pRegion)
		pRegion->GetTileFileName(s);
	else
	{
		CString strFold ;
		GetGroundFolder(strFold);
		cRegion region(GetGraphics());
		region.SetPos(tx,ty);
		region.SetRegionFolder(strFold);
		region.GetTileFileName(s);
	}
	CFileFind f;
	if (f.FindFile(s))
	{
		AppCopyFile(s,szFile);
		return S_OK;
	}
	return -1;
}

HRESULT	cGround::RestoreFile(int tx, int ty, LPCSTR szFile)
{
	if (tx < 0 || ty < 0)
		return -1;
	cRegion* pRegion = GetcRegionByRegion(tx,ty);
	if (pRegion)
		pRegion->SetTileFileName(szFile);
	else
	{
		CString strFold ;
		GetGroundFolder(strFold);
		cRegion region(GetGraphics());
		region.SetPos(tx,ty);
		region.SetRegionFolder(strFold);
		region.SetTileFileName(szFile);
	}
	return S_OK;
}

HRESULT cGround::FillRegion(int tx,int ty,LPCSTR szSprFile)
{
	//first to get which region to !
	//should know the szSprFile's frames
	if (tx < 0 || ty < 0)
		return -1;

	cRegion* pRegion = GetcRegionByRegion(tx,ty);
	if (pRegion)
	{
		pRegion->Fill(szSprFile);
		if (pRegion->IsPaintTerrain())
			pRegion->UpdateGroundTexture();
		return S_OK;
	}

//临时生成一个来操作。
	CString strFold ;
	GetGroundFolder(strFold);
	cRegion region(GetGraphics());
	region.SetPos(tx,ty);
	region.SetRegionFolder(strFold);
	region.LoadTerrain();
	region.Fill(szSprFile);
	region.SaveTerrain();
	return S_OK;
}

int	cGround::GetPlaneNum()
{
	int num = 0;
	for (int x = 0; x < m_wRegion; x++)
	for (int y = 0; y < m_hRegion; y++)
	{
		cRegion* pRegion = GetcRegion(x,y);
		if (pRegion)
			num += pRegion->GetPlaneNum();
	}
	return num;
}

int	cGround::GetLightNum()
{
	int num = 0;
	for (int x = 0; x < m_wRegion; x++)
	for (int y = 0; y < m_hRegion; y++)
	{
		cRegion* pRegion = GetcRegion(x,y);
		if (pRegion)
			num += pRegion->GetLightNum();
	}
	return num;
}

////////////////////////////////////////////////////////////////////////////
//add and remove
////////////////////////////////////////////////////////////////////////////
HRESULT cGround::RebuildTree()
{
	cTimerFunction timer("RebuildTree");

	for_all(m_listPlaneVer,mem_fun<bool,cPlaneVer>(&cPlaneVer::BeginBuildTree));
	//sort 
	//m_aLine.sort();
	//
	SortLineArray(m_aLine);
//	sort(m_aLine.begin(),m_aLine.end(),sortfun);
	m_pTree->clear();
	
	//
	for (itALine it = m_aLine.begin(); it!= m_aLine.end(); it++)
	{
		stLine& line = *(*it);
		float f = line.GetLengthSquare();
		for (itcPlaneVer it = line.listVer.begin(); it != line.listVer.end(); ++it)
		{
			m_pTree->insert((*it));
		}
	}

	{
		for (itcPlaneVer it = m_listPlaneVerSeam.begin(); it != m_listPlaneVerSeam.end(); ++it)
			m_pTree->insert((*it));
	}
	{
		for (itcPlaneVer it = m_listPlaneVerPoint.begin(); it != m_listPlaneVerPoint.end(); ++it)
			m_pTree->insert((*it));
	}
	{
		for (itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++it)
		{
			cPlaneVer* p = dynamic_cast<cPlaneVer*>(*it);
			if (p != NULL)
			{
				p->BeginBuildTree();
				m_pTree->insert((p));
				p->EndBuildTree();
			}
		}

		for (int x = 0; x < m_wRegion; x++)
		for (int y = 0; y < m_hRegion; y++)
		{
			cRegion* pRegion = GetcRegion(x,y);
			if (!pRegion)
				continue;
			cPlaneList& llist = pRegion->m_listClientNpc;
			for (itcPlane itl = llist.begin(); itl != llist.end(); ++ itl)
			{
				cPlaneVer* p = dynamic_cast<cPlaneVer*>(*itl);
				if (p != NULL)
				{
					p->BeginBuildTree();
					m_pTree->insert((p));
					p->EndBuildTree();
				}
			}
		}
	}

	m_pTree->build();

	for_all(m_listPlaneVer,mem_fun<bool,cPlaneVer>(&cPlaneVer::EndBuildTree));

	return S_OK;
}

HRESULT cGround::AddiObj(iObj* p, bool bRunNow)
{
	if (p == NULL)
	{
		ASSERT(0);
		return -1;
	}
	p->AddRef();
	stParam param;
	param.mask = MASK_COMMAND;
	param.eCommand = GCOM_ADDOBJ;
	param.pObj = p;
	SetParam(param,bRunNow);
	return S_OK;
}

HRESULT cGround::RemoveiObj(iObj* p,bool bRunNow)
{
	if (p == NULL)
	{
		ASSERT(0);
		return -1;
	}
	p->AddRef();
	stParam param;
	param.mask = MASK_COMMAND;
	param.eCommand = GCOM_REMOVEOBJ;
	param.pObj = p;
	SetParam(param,bRunNow);
	return S_OK;
}

BOOL cGround::AddVer(cPlaneVer* p)
{
	//Ver已经在场景中了
	if (!T_AddPlaneToListByLayer<cPlaneVer>(m_listPlaneVer,p))
		return FALSE;

	//把ver的底边落在哪个格子和相应的挡光信息加入到地图
	if (m_param.bBuildTree)
		GetColorMap()->AddPlaneToTile(p);

	switch (p->GetVersortType())
	{
	case VERSORT_POINT:
		m_listPlaneVerPoint.push_back(p);
		if (m_param.bBuildTree)
			m_pTree->insert(p);
		return TRUE;
	case VERSORT_SEAM:
		//如果先sort，再插入的话更简单
		{
			bool bInsert = false;
			cPoint p1,p2;
			p->GetSortSeam(p1,p2);
			int length = DistanceSquare(p1,p2);
			for (itcPlaneVer it = m_listPlaneVerSeam.begin(); it != m_listPlaneVerSeam.end(); ++it)
			{
				(*it)->GetSortSeam(p1,p2);
				int len = DistanceSquare(p1,p2);
				if (len < length)
				{
					m_listPlaneVerSeam.insert(it,p);
					bInsert = true;
					break;
				}
			}
			if (!bInsert)
				m_listPlaneVerSeam.push_back(p);
		}
		if (m_param.bBuildTree)
			RebuildTree();
		return TRUE;
	case VERSORT_TREE:
		//add ver to megalist		
		{
			AddPlaneVerToLineArray(m_aLine,p);
			//build tree
			if (m_param.bBuildTree)
				RebuildTree();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL cGround::RemoveVer(cPlaneVer* p)
{
	if (!T_RemovePlaneFromList<cPlaneVer>(m_listPlaneVer,p))
		return FALSE;

	if (m_param.bBuildTree)
		GetColorMap()->RemovePlaneFromTile(p);

	//remove it from tree
	if (m_param.bBuildTree)
	{
		if (!m_pTree->remove(p))
			return FALSE;
	}
		//remove from point 
	switch (p->GetVersortType())
	{
	case VERSORT_POINT:
		m_listPlaneVerPoint.remove(p);
		return TRUE;
	case VERSORT_SEAM:
		m_listPlaneVerSeam.remove(p);
		if (m_param.bBuildTree)
			RebuildTree();
		return TRUE;
	case VERSORT_TREE:
		{
	//remove from line
//			while(m_pTree->remove(p));
			for (itALine it = m_aLine.begin(); it!= m_aLine.end(); it++)
			{
				stLine& line = *(*it);
				BOOL bRemove = FALSE;
				for (itcPlaneVer i = line.listVer.begin(); i != line.listVer.end(); ++i)
				{
					if (*i == p)
					{
						line.listVer.erase(i);
						bRemove = TRUE;
						break;
					}
				}
				if (bRemove)
				{
					if (line.listVer.size() == 0)
					{
						delete (&line);
						m_aLine.erase(it);
						break;
					}
				}
			}
			if (m_param.bBuildTree)
				RebuildTree();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL cGround::AddPlaneAbove(cPlane* p)
{
	return T_AddPlaneToListByLayer<cPlane>(m_listPlaneAboveVer,p);
}

BOOL cGround::RemovePlaneAbove(cPlane* p)
{
	return T_RemovePlaneFromList<cPlane>(m_listPlaneAboveVer,p);
}

const float c_fAboveHeight = 2.5f;
//const float c_fAboveHeight = 200 / c_ptScale.z;

void cGround::AddPlane(cPlane* p,cRegion* pRegion, BOOL bToRegion)
{
	cPoint pt = p->GetOffset();
	if(pt.z > c_fAboveHeight)
	{
		AddPlaneAbove(p);
		return;
	}

	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer)
	{
		AddVer(pVer);
	}
	else
	{
		cPlaneHor* pHor = dynamic_cast<cPlaneHor*>(p);
		ASSERT(pHor);
		if (pHor->GetSortType() < SORT_VER)
		{
			if (bToRegion)
				pRegion->AddHor(pHor,true);
			for (int i=0; i<D_12; i++)
			{
				cRegion* pRegion2 = GetLinkRegion(pRegion,(eRegionDirection)i);
				if (pRegion2 != NULL)
				{
					//得到相反的方向
					int ti = i + D_6;
					if (ti >= D_12)
						ti -= D_12;
					if (pRegion2->CrossPlane(pHor,(eRegionDirection)ti))
						pRegion2->AddLinkHor(pHor);
				}
			}
		}
	}
}

HRESULT cGround::AddcPlane(cPlane* p)
{
	cPoint pt = p->GetCenterPosition();
	cRegion* pRegion = GetcRegionByGround(pt);
	if (pRegion == NULL)
	{
		//选择了一个物件，然后屏幕移动很远，再次选择,pRegion = NULL
		p->ClearTexture();
		return -1;
	}
	//仅仅是放在它的list中
	pRegion->AddcPlane(p);
	if (pRegion->IsPaintPlane())
		AddPlane(p,pRegion,true);
	return S_OK;
}

HRESULT cGround::RemovecPlane(cPlane* p)
{
#pragma message (ATTENTION("无奈出此下策"))
	int x,y;
	p->GetRegion(x,y);
	if (x < 0 || y < 0)
	{
		p->ClearTexture();
		return -1;
	}
	cRegion* pRegion = GetcRegionByRegion(x,y);
	if (!pRegion)
		return -1;
	ASSERT(pRegion);
	pRegion->RemovecPlane(p);
	if (pRegion->IsPaintPlane())
		RemovePlane(p,pRegion);
	return S_OK;
}

void cGround::RemovePlane(cPlane* p,cRegion* pRegion)
{
	cPoint pt = p->GetOffset();
	if(pt.z > c_fAboveHeight)
	{
		RemovePlaneAbove(p);
		return;
	}

	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer)
	{
		RemoveVer(pVer);
	}
	else
	{
		cPlaneHor* pHor = dynamic_cast<cPlaneHor*>(p);
		ASSERT(pHor);
		if (pHor->GetSortType() < SORT_VER)
		{
			pRegion->RemoveHor(pHor,true);
			for (int i=0; i<D_12; i++)
			{
				cRegion* pRegion2 = GetLinkRegion(pRegion,(eRegionDirection)i);
				if (pRegion2 != NULL && pRegion2->IsPaintTerrain())
				{
					//得到相反的方向
					int ti = i + D_6;
					if (ti >= D_12)
						ti -= D_12;
					if (pRegion2->CrossPlane(pHor,(eRegionDirection)ti))
						pRegion2->RemoveLinkHor(pHor);
				}
			}
		}
	}
}

HRESULT cGround::AddcObstacle(cObstacle* p)
{
	cPoint pt = p->GetOffset();
	cRegion* pRegion = GetcRegionByGround(pt);
	if (pRegion == NULL)
		return -1;
	pRegion->AddcObstacle(p);

	return S_OK;
}

HRESULT cGround::RemovecObstacle(cObstacle* p)
{
	int x,y;
	p->GetRegion(x,y);
	if (x < 0 || y < 0)
	{
		return -1;
	}
	cRegion* pRegion = GetcRegionByRegion(x,y);
	if (pRegion == NULL)
		return -1;
	pRegion->RemovecObstacle(p);
	
	return S_OK;
}

HRESULT cGround::AddClientOnlyNpc(cPlane* p)
{
	cPoint pt = p->GetOffset();
	cRegion* pRegion = GetcRegionByGround(pt);
	if (pRegion == NULL)
		return -1;
	pRegion->AddClientOnlyNpc(p);

	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer!=NULL)
		m_pTree->insert(pVer);
	
	return S_OK;
}

HRESULT cGround::RemoveClientOnlyNpc(cPlane* p)
{
	int x,y;
	p->GetRegion(x,y);
	if (x < 0 || y < 0)
	{
		return -1;
	}
	cRegion* pRegion = GetcRegionByRegion(x,y);
	if (pRegion == NULL)
		return -1;
	pRegion->RemoveClientOnlyNpc(p);

	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer!=NULL)
		m_pTree->remove(pVer);
	
	return S_OK;
}

HRESULT cGround::AddVerBelowGround(cPlane* p)
{
	cPoint pt = p->GetOffset();
	cRegion* pRegion = GetcRegionByGround(pt);
	if (pRegion == NULL)
		return -1;
	pRegion->AddVerBelowGround(p);
	return S_OK;
}

HRESULT cGround::RemoveVerBelowGround(cPlane* p)
{
	int x,y;
	p->GetRegion(x,y);
	if (x < 0 || y < 0)
	{
		return -1;
	}
	cRegion* pRegion = GetcRegionByRegion(x,y);
	if (pRegion == NULL)
		return -1;
	pRegion->RemoveVerBelowGround(p);
	
	return S_OK;
}

HRESULT cGround::AddcLight(cLight* p)
{
	cPoint pt = p->GetOffset();
	cRegion* pRegion = GetcRegionByGround(pt);
	if (pRegion == NULL)
		return -1;
	pRegion->AddcLight(p);
	if (pRegion->IsPaintPlane())
		AddLight(p);
	return S_OK;
}

void cGround::AddLight(cLight* p)
{
	m_listLight.push_back(p);
}

HRESULT cGround::RemovecLight(cLight* p)
{
	int x,y;
	p->GetRegion(x,y);
	if (x < 0 || y < 0)
	{
		return -1;
	}
	cRegion* pRegion = GetcRegionByRegion(x,y);
	if (pRegion == NULL)
		return -1;
	pRegion->RemovecLight(p);
	if (pRegion->IsPaintPlane())
		RemoveLight(p);
	return S_OK;
}

void cGround::RemoveLight(cLight* p)
{
	m_listLight.remove(p);
}

HRESULT cGround::AddcObj(iObj* pObj)
{
	cGroundObj* pGroundObj = dynamic_cast<cGroundObj*>(pObj);
	if (pGroundObj == NULL)
	{
		ASSERT(0);
		return -1;
	}
	if (!pGroundObj->IsStatic())
	{
		if (pGroundObj->IsNoSave())
		{
			pGroundObj->AddRef();
			pGroundObj->SetInGround(TRUE);
			m_listDynamicObj.push_back(pGroundObj);
			cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(pObj);
			if (pVer!=NULL)
			{
				m_pTree->insert(pVer);
			}			
		}
		else
			AddClientOnlyNpc(dynamic_cast<cPlane*>(pObj));			
		//这个动态的物体是什么？
		return S_OK;
	}
	if (pGroundObj->IsMoving())
	{
		pGroundObj->AddRef();
		pGroundObj->SetInGround(TRUE);
		ASSERT(dynamic_cast<cPlaneHor*>(pGroundObj));
		m_listMovingObj.push_back(dynamic_cast<cPlaneHor*>(pGroundObj));
		return S_OK;
	}

	cPlane* p = dynamic_cast<cPlane*>(pObj);
	if(p !=NULL )
	{
		if (p->IsSortVerBelowGround())
			AddVerBelowGround(p);
		else
			AddcPlane(p);
	}
	else
	{
		cLight* p = dynamic_cast<cLight*>(pObj);
		if (p != NULL)
			AddcLight(p);
		else
		{
			cObstacle* p = dynamic_cast<cObstacle*>(pObj);
			if (p != NULL)
				AddcObstacle(p);
			else
				ASSERT(0);
		}
	}	
	return S_OK;
}

HRESULT cGround::RemovecObj(iObj* pObj)
{
	cGroundObj* pGroundObj = dynamic_cast<cGroundObj*>(pObj);
	if (pGroundObj == NULL)
	{
		ASSERT(0);
		return -1;
	}
	if (!pGroundObj->IsStatic())
	{
		if (pGroundObj->IsNoSave())
		{
			pGroundObj->SetInGround(FALSE);
			int size = m_listDynamicObj.size();
			m_listDynamicObj.remove(pGroundObj);
			if (size == m_listDynamicObj.size() + 1)
				pObj->Release();
			else
				ASSERT(0);
			cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(pObj);
			if (pVer!=NULL)
				m_pTree->remove(pVer);			
		}
		else
			RemoveClientOnlyNpc(dynamic_cast<cPlane*>(pObj));			
		return S_OK;
	}
	if (pGroundObj->IsMoving())
	{
		pGroundObj->SetInGround(FALSE);
		int size = m_listMovingObj.size();
		ASSERT(dynamic_cast<cPlaneHor*>(pGroundObj));
		m_listMovingObj.remove(dynamic_cast<cPlaneHor*>(pGroundObj));
		if (size == m_listMovingObj.size() + 1)
			pObj->Release();
		else
			ASSERT(0);
		return S_OK;
	}

	cPlane* p = dynamic_cast<cPlane*>(pObj);
	if(p !=NULL )
	{
		if (p->IsSortVerBelowGround())
			RemoveVerBelowGround(p);
		else
			RemovecPlane(p);
	}
	else
	{
		cLight* p = dynamic_cast<cLight*>(pObj);
		if (p!= NULL)
			RemovecLight(p);
		else
		{
			cObstacle* p = dynamic_cast<cObstacle*>(pObj);
			if (p != NULL)
				RemovecObstacle(p);
			else 
				ASSERT(0);
		}
	}	
	return S_OK;
}

/**************************************************************************/
//从地图上获取元素
//外部调用,不会很频繁,所以不优化了
/**************************************************************************/
//得到自动拼接的点
const float c_fMinDisForLink = 0.3f; 
BOOL cGround::GetLinkOffsetVer(cPoint& ptOffset, cPlaneVer* p, int eLink,cPoint ptPlane, float& fMinDis )
{
	BOOL b = FALSE;
	for (itcPlaneVer it = m_listPlaneVer.begin(); it != m_listPlaneVer.end(); ++it)
	{
		cPlaneVer* pPlane = (*it);
		if (pPlane == p)
			continue;

		cPoint aPoint[2];
		int nPoint = 2;
		pPlane->GetBottomSeam(aPoint[0],aPoint[1]);
		for (int i=0; i<nPoint; i++)
		{
			aPoint[i].z = 0;
			float f = DistanceAdd(ptPlane,aPoint[i]);
			if (f < fMinDis)
			{
				fMinDis = f;
				ptOffset = aPoint[i];
				b = TRUE;
			}
		}
	}
	return b;
}

BOOL cGround::GetLinkOffsetTile(cPoint& ptOffset, cPoint ptPlane, float& fMinDis )
{
	GroundToTile(ptPlane);

	int mul = 2;

	cPoint pt[4];
	pt[0].x = floor(ptPlane.x/mul)*mul;
	pt[0].y = floor(ptPlane.y/mul)*mul;
	pt[0].z = 0;
	pt[1].x = floor(ptPlane.x/mul)*mul;
	pt[1].y = ceil(ptPlane.y/mul)*mul;
	pt[1].z = 0;
	pt[2].x = ceil(ptPlane.x/mul)*mul;
	pt[2].y = floor(ptPlane.y/mul)*mul;
	pt[2].z = 0;
	pt[3].x = ceil(ptPlane.x/mul)*mul;
	pt[3].y = ceil(ptPlane.y/mul)*mul;
	pt[3].z = 0;
	BOOL b = FALSE;
	for (int i=0; i<4; i++)
	{
		float f = DistanceAdd(ptPlane,pt[i]);
		if (f < fMinDis/2)
		{
			fMinDis = f*2;
			TileToGround(pt[i],ptOffset);
			b = TRUE;
		}
	}
	return b;
}

BOOL cGround::GetLinkOffset(cPoint& ptOffset, cPlane* p, int eLink,cPoint ptPlane, float& fMinDis )
{
	if (eLink == LINK_TILE)
	{
		return GetLinkOffsetTile(ptOffset,ptPlane,fMinDis);
	}
	else if (eLink == LINK_VER)
	{
		if (dynamic_cast<cPlaneVer*>(p) == NULL)
			return FALSE;
		return GetLinkOffsetVer(ptOffset,dynamic_cast<cPlaneVer*>(p),eLink,ptPlane,fMinDis);
	}
	return FALSE;
}

BOOL cGround::GetLinkOffset(cPoint& ptOffset, iObj* p, int eLink)
{
	if (eLink == LINK_NULL)
		return FALSE;
	cPlane* pPlane = dynamic_cast<cPlane*>(p);
	if (pPlane == NULL)
	{
		ASSERT(0);
		return FALSE;
	}
	cPoint aPoint[5];
	int nPoint = 0;

	//平行的面考虑四个角
	cPlaneVer* pPlaneVer = dynamic_cast<cPlaneVer*>(p);
	if (pPlaneVer)
	{
		nPoint = 2;
		pPlaneVer->GetBottomSeam(aPoint[0],aPoint[1]);
		aPoint[0].z = 0;
		aPoint[1].z = 0;
	}
	else
	{
		cPlaneHor* pPlaneHor = dynamic_cast<cPlaneHor*>(p);
		nPoint = 4;
		pPlaneHor->GetRectPoint(aPoint);
	}
//	nPoint ++;
//	aPoint[4] = pPlane->GetOffset();

	float fMinDis = c_fMinDisForLink;
	cPoint ptMinDis;
	for (int i=0; i<nPoint; i++)
	{
		cPoint pt;
		if ( GetLinkOffset(pt,pPlane,eLink,aPoint[i],fMinDis) )
			ptMinDis = pt - aPoint[i];
	}

	if (fMinDis == c_fMinDisForLink)
		return FALSE;

	ptOffset = ptMinDis + pPlane->GetOffset();
	return TRUE;
}

BOOL cGround::BlockObstacle(cPoint pt,ObstacleEnum e)
{
	return GetMapObstacle()->IsBlock(pt,e);
	
	for (int x = 0; x < MAX_REGION_WIDTH; x++)
	for (int y = 0; y < MAX_REGION_HEIGHT; y++)
	{
		cRegion* pRegion = m_aRegion[x][y];
		if (pRegion == NULL)
			continue;
		if (pRegion->BlockObstacle(pt,e))
			return TRUE;
	}
	return FALSE;
}

ObstacleEnum cGround::CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise)
{
	for (int x = 0; x < MAX_REGION_WIDTH; x++)
	for (int y = 0; y < MAX_REGION_HEIGHT; y++)
	{
		cRegion* pRegion = m_aRegion[x][y];
		if (pRegion == NULL)
			continue;
		ObstacleEnum e = pRegion->CheckObstacle(pt,vSpeed,bClockWise);
		if (e != OBSTACLE_NULL)
			return e;
	}
	return OBSTACLE_NULL;
}

HRESULT cGround::GetTileType(cPoint ptGround,long& l)
{
	bool b = GetColorMap()->GetTileType(ptGround,l);
	if (b)
		return S_OK;
	return -1;
}

////////////////////////////////////////////////////////////////////////////
//getobj
////////////////////////////////////////////////////////////////////////////
//获取组件
HRESULT cGround::GetiObj(int nID, iObjList& iList)
{
	//get the 9 tile or get of the list?
	for (itcRegion it = m_listRegion.begin(); it != m_listRegion.end(); ++it)
	{
		cRegion* pRegion = (*it);
		pRegion->GetiObj(nID,iList);
	}
	T_GetAt(nID,m_listDynamicObj,iList);
	return S_OK;
}

HRESULT	cGround::GetiLight(stSelect& st,iObjList& tlist)
{
	cLightList rlist;
	for (int x = 0; x < m_wRegion; x++)
	for (int y = 0; y < m_hRegion; y++)
	{
		cRegion* pRegion = GetcRegion(x,y);
		if (!pRegion)
			continue;
		pRegion->GetiLight(st,rlist);
	}
	insert(rlist.begin(),rlist.end(),tlist);	
	return S_OK;
}

HRESULT	cGround::GetiObstacle(stSelect& st,iObjList& tlist)
{
	cObstacleList rlist;

	for (int x = 0; x < MAX_REGION_WIDTH; x++)
	for (int y = 0; y < MAX_REGION_HEIGHT; y++)
	{
		cRegion* pRegion = m_aRegion[x][y];
		if (pRegion == NULL)
			continue;
		pRegion->GetiObstacle(st,rlist);
	}

	switch (st.ePoint)
	{
	case SelectPoint_OneByScreen:
		if (rlist.empty())
		{	
			for (itcPlaneHor it = m_listMovingObj.begin(); it != m_listMovingObj.end(); ++it)
			{
				cPlaneHor* p = (*it);
				cObstacle* pObs = p->GetObstacle(st.ptScreen);
				if (pObs != NULL)
				{
					rlist.push_back(pObs);
					break;
				}
			}
		}
		break;
	case SelectPoint_RectByDiamond:
		{
			//选择一个区域作为组件，这样的话不要选择阻挡			
		}
		break;
	}

	insert(rlist.begin(),rlist.end(),tlist);	
	return S_OK;
}

HRESULT	cGround::GetiPlane(stSelect& st,iObjList& tlist)
{
	if (st.eType == SelectType_Dynamic)
	{
		switch (st.ePoint)
		{
		case SelectPoint_OneByScreen:
			{
				for (itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++ it)
				{
					if ((*it)->Contain(st.ptScreen))
					{
						(*it)->AddRef();
						tlist.push_back(*it);
						break;
					}
				}
				break;
			}
		case SelectPoint_RectByGround:
			{
				for (itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++ it)
				{
					if ((*it)->ContainedGround(st.rcGround))
					{
						(*it)->AddRef();	
						tlist.push_back(*it);
					}
				}
				break;				
			}
		case SelectPoint_RectByDiamond:
			{
				for (itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++ it)
				{
					if ((*it)->ContainedDiamond(st.rcGround))
					{
						(*it)->AddRef();
						tlist.push_back(*it);
					}
				}
				break;
			}
		}
//		return S_OK;	dont return
	}

	cPlaneList rlist;
//	for (int x = 0; x < MAX_REGION_WIDTH; x++)
//	for (int y = 0; y < MAX_REGION_HEIGHT; y++)
	for (int x = 0; x < m_wRegion; x++)
	for (int y = 0; y < m_hRegion; y++)
	{
		cRegion* pRegion = GetcRegion(x,y);
		if (pRegion == NULL)
			continue;
		pRegion->GetiPlane(st,rlist);
	}
	insert(rlist.begin(),rlist.end(),tlist);
	for_all(rlist,mem_fun<ULONG,iObj>(&iObj::AddRef));
	return S_OK;
}

HRESULT	cGround::GetiPlaneByPoint(POINT ptScreen,iObjList& tlist)
{
	stSelect st;
	st.ePoint = SelectPoint_OneByScreen;
	st.ptScreen = ptScreen;
	st.eFilter = SelectFilter_OneByDisplay;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiPlane(st,tlist);
}

HRESULT	cGround::GetiObstacleByPoint(POINT ptScreen,iObjList& tlist) 
{
	stSelect st;
	st.ePoint = SelectPoint_OneByScreen;
	st.ptScreen = ptScreen;
	st.eFilter = SelectFilter_First;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiObstacle(st,tlist);
}

HRESULT	cGround::GetiLightByPoint(POINT ptScreen,iObjList& tlist)
{
	stSelect st;
	st.ePoint = SelectPoint_OneByScreen;
	st.ptScreen = ptScreen;
	st.eFilter = SelectFilter_First;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiLight(st,tlist);
}

HRESULT cGround::GetiPlaneByRect(cRect rcGround,iObjList& tlist)
{
	return -1;
}

HRESULT	cGround::GetiObstacleByRect(cRect rcGround,iObjList& tlist)
{
	return -1;
}

HRESULT	cGround::GetiLightByRect(cRect rcGround,iObjList& tlist) 
{
	return -1;
}

HRESULT	cGround::GetiPlaneByDiamond(cRect rcGround,iObjList& tlist)
{
	stSelect st;
	st.ePoint = SelectPoint_RectByDiamond;
	st.rcGround = rcGround;
	st.eFilter = SelectFilter_All;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiPlane(st,tlist);
}

HRESULT	cGround::GetiObstacleByDiamond(cRect rcGround,iObjList& tlist)
{
	stSelect st;
	st.ePoint = SelectPoint_RectByDiamond;
	st.rcGround = rcGround;
	st.eFilter = SelectFilter_All;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiObstacle(st,tlist);
}

HRESULT	cGround::GetiLightByDiamond(cRect rcGround,iObjList& tlist)
{
	stSelect st;
	st.ePoint = SelectPoint_RectByDiamond;
	st.rcGround = rcGround;
	st.eFilter = SelectFilter_All;
	st.eFilterParam = SelectFilterParam_Default;
	st.eType = SelectType_Default;
	return GetiLight(st,tlist);
}


/**************************************************************************/
//paint loop
/**************************************************************************/
void cGround::CheckDisplay()
{
//	for_all(m_listRegion,mem_fun<bool,cRegion>(&cRegion::CheckDisplay));
	cRect rc = GetRectGround();
	for (int x = 0; x<m_wRegion; x++)
	for (int y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p)
			p->CheckDisplay(rc);
	}

	for_all(m_listLight,bind2nd(mem_fun1<bool,cLight,const cRect*>(&cLight::CheckDisplay),&rc));
	for_all(m_listPlaneVer,bind2nd(mem_fun1<bool,cPlane,const cRect*>(&cPlane::CheckDisplay),&rc));
	for_all(m_listPlaneAboveVer, bind2nd(mem_fun1<bool,cPlane,const cRect*>(&cPlane::CheckDisplay),&rc));
	for_all(m_listMovingObj,bind2nd(mem_fun1<bool,cPlaneHor,const cRect*>(&cPlaneHor::CheckDisplay),&rc));
	for_all(m_listDynamicObj,bind2nd(mem_fun1<bool,cGroundObj,const cRect*>(&cGroundObj::CheckDisplay),&rc));
}

void cGround::BeginLighting()
{
	if (!IsRegionValid())
		return;

	int x,y;
	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p)
			p->BeginLighting(GetColor());
	}

//	int light = m_param.nBrightness;
	cColor cr = GetColor();
//	cr.a = 255;
//	cr.r = light;
//	cr.g = light;
//	cr.b = light;

	for_all(m_listLight,bind2nd(mem_fun1<bool,cLight,cColor>(&cLight::BeginLighting),cr));
	for_all(m_listPlaneVer,bind2nd(mem_fun1<bool,cPlane,cColor>(&cPlane::BeginLighting),cr));
	for_all(m_listPlaneAboveVer, bind2nd(mem_fun1<bool,cPlane,cColor>(&cPlane::BeginLighting),cr));
	for_all(m_listMovingObj,bind2nd(mem_fun1<bool,cPlaneHor,cColor>(&cPlaneHor::BeginLighting),cr));
	for_all(m_listDynamicObj,bind2nd(mem_fun1<bool,cGroundObj,cColor>(&cGroundObj::BeginLighting),cr));

	m_pColorMap->BeginLighting(cr);
}

void cGround::GetLightMap()
{
	if (!IsRegionValid())
		return;

#pragma message (ATTENTION("get light map of the ver and add ver to lights's obstacle"))
	//先把光源加入到树中，向上遍历可以获得照亮和阻挡的边
	//向下遍历可以获得阻挡的边
	for (itcLight it = m_listLight.begin(); it != m_listLight.end(); it ++)
	{
		if ((*it)->IsDisplay() && (*it)->IsEnable())
			GetColorMap()->GetLightMap((*it));
	}
	for (itcGroundObj it2 = m_listDynamicObj.begin(); it2 != m_listDynamicObj.end(); ++it2)
	{
		cLight* pLight = dynamic_cast<cLight*>(*it2);
		if (pLight != NULL && pLight ->IsDisplay())
			GetColorMap()->GetLightMap((pLight));
	}
}

void cGround::EndLighting()
{
	if (!IsRegionValid())
		return;

	m_pColorMap->EndLighting();

	int x,y;
	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p)
			p->EndLighting();
	}

	for_all(m_listPlaneVer,mem_fun<bool,cPlane>(&cPlane::EndLighting));
//	for_all(m_listPlaneAboveVer,mem_fun<bool,cPlane>(&cPlane::EndLighting));
	for_all(m_listMovingObj,mem_fun<bool,cPlaneHor>(&cPlaneHor::EndLighting));
	for_all(m_listDynamicObj,mem_fun<bool,cGroundObj>(&cGroundObj::EndLighting));
	for_all(m_listLight,mem_fun<bool,cLight>(&cLight::EndLighting));
}

bool cGround::GetColor(const stVertexGround& v, DWORD& value)
{
	cPoint ptTemp = GetPoint(v);
	return GetColor(ptTemp,value);
}

bool cGround::GetColor(cPoint ptGround,DWORD& value)
{
	return GetColorMap()->GetColor(ptGround,value);
}

bool cGround::GetLightParam(const stVertexGround& v,stLightParam& param)
{
	cPoint ptTemp = GetPoint(v);
	return GetLightParam(ptTemp,param);
}

bool cGround::GetLightParam(cPoint ptGround, stLightParam& param)
{
	return GetColorMap()->GetLightParam(ptGround,param);
}

void cGround::RebuildColorMap()
{
	float x = m_xRegion;
	float y = m_yRegion;
	RegionToTile(x,y);
	TileToGround(x,y);
	m_pColorMap->SetOffset(x,y);
	for (itcPlaneVer it = m_listPlaneVer.begin(); it != m_listPlaneVer.end(); ++it)
		m_pColorMap->AddPlaneToTile(*it);
}

void cGround::Draw()
{
	if (!IsRegionValid())
		return;

	//绘制地表
	int x,y;

	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p->IsValid())
			p->PaintBeforeTerrain();
	}

	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		cRegion* p = GetcRegion(x,y);
		if (p->IsValid())
			p->PaintTerrain();
	}

	//绘制当前激活的物体,低于ver的
	for_all(m_listMovingObj,mem_fun<HRESULT,cPlaneHor>(&cPlaneHor::Paint));

	m_nPlaneDraw = 0;

	for (itSpecial it = m_listSpecial.begin(); it!= m_listSpecial.end(); ++it)
	{
		iSpecial* p = (*it);
		if (p->IsActive())
		{
			if (p->IsDraw(iSpecial::SD_DRAWBEFOREALL))
				p->Draw(iSpecial::SD_DRAWBEFOREALL);
		}
	}

	//绘制垂直物体
	m_pTree->draw();

	//该层统一排序，因为没有阻挡和光照，所以切分没有意义
	//绘制高的物体
	for_all(m_listPlaneAboveVer,mem_fun<HRESULT,cPlane>(&cPlane::Draw));

	//使用贴图的和画线的和写字的统统不兼容
	PaintOthers(); //will call drawspecial

	//绘制离子效果，现在离子效果与前面同样不兼容
	for (it = m_listSpecial.begin(); it!= m_listSpecial.end(); ++it)
	{
		iSpecial* p = (*it);
		if (p->IsActive())
		{
			if (p->IsDraw(iSpecial::SD_DRAWAFTERALL))
				p->Draw(iSpecial::SD_DRAWAFTERALL);
		}
	}
}

void cGround::PaintOthers()
{
	m_pSelect->Draw();

	int x,y;
	for (x = 0; x < MAX_REGION_WIDTH; x++)
	for (y = 0; y < MAX_REGION_HEIGHT; y++)
	{
		cRegion* pRegion = m_aRegion[x][y];
		if (pRegion == NULL)
			continue;
		pRegion->PaintObstacle();
	}

	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		//绘制光源和障碍
		cRegion* p = GetcRegion(x,y);
		if (p->IsValid())
			p->PaintOthers();
	}

	GetMapObstacle()->Draw();

	for(itcGroundObj it = m_listDynamicObj.begin(); it != m_listDynamicObj.end(); ++it)
	{
		cPlane* p = dynamic_cast<cPlane*>(*it);
		if (p)
		{
			p->DrawSelect();
			p->DrawTrace();
			p->DrawObstacle();
//			p->DrawOffset();
		}
		else
		{
			cObstacle* p = dynamic_cast<cObstacle*>(*it);
			if (p)
			{
				if( GetGraphics()->IsShow(iGraphics::GS_OBSTACLE))
					p->Draw();
			}
			else
			{
				cLight* p = dynamic_cast<cLight*>(*it);
				if (GetGraphics()->IsShow(iGraphics::GS_LIGHT))
					p->Draw();
			}
		}
	}
	DrawSpecial();

	//使用贴图的和画线的和写字的统统不兼容
	for (x = 0; x<m_wRegion; x++)
	for (y = 0; y<m_hRegion; y++)
	{
		//绘制光源和障碍
		cRegion* p = GetcRegion(x,y);
		if (p->IsValid())
			p->PaintText();
	}
}

void cGround::Paint()
{
	BOOL bValid = TRUE;

	if (m_param.bRenderMap == FALSE)
	{
next2:
		for (int x = 0; x<m_wRegion; x++)
		for (int y = 0; y<m_hRegion; y++)
		{
			cRegion* p = GetcRegion(x,y);
			if (p == NULL)
			{
				Sleep(0);
				goto next2;
			}
//			if (p->m_pTexture == NULL || !p->m_bUpdateTexture)
//				return ;
		}
	}
	else
	{
next:
		for (int x = 0; x<m_wRegion; x++)
		for (int y = 0; y<m_hRegion; y++)
		{
			cRegion* p = GetcRegion(x,y);
			if (p == NULL)
			{
				Sleep(0);
				goto next;
			}
			if (x == 1 && y == 1)
			{
				if (p->m_pTexture == NULL || !p->m_bUpdateTexture)
				{
					Sleep(0);
					goto next;
				}
			}
		}
	}

	if (!m_bRebuildRegion)
		RebuildRegion();

	HRESULT hr;
	hr = SetEnvironment();
	
	LARGE_INTEGER c1,c2,c3,c4,c5,c6;
	//c6,c7,freq;
	LARGE_INTEGER freq;

	BOOL b;
	b = QueryPerformanceCounter(&c1);
	CheckDisplay();
	b = QueryPerformanceCounter(&c2);
	BeginLighting();
	b = QueryPerformanceCounter(&c3);
	long value = GetColor().Value();
	if (value != 0xffffffff)
		GetLightMap();
	b = QueryPerformanceCounter(&c4);
	if (value != 0xffffffff)
		EndLighting();
	b = QueryPerformanceCounter(&c5);
	Draw();
	b = QueryPerformanceCounter(&c6);
	int t1,t2,t3,t4,t5,t6;
	//t6,t7;
    t1 = c2.LowPart - c1.LowPart;
    t2 = c3.LowPart - c2.LowPart;
    t3 = c4.LowPart - c3.LowPart;
    t4 = c5.LowPart - c4.LowPart;
    t5 = c6.LowPart - c5.LowPart;
	t6 = GetGraphics()->GetFlipTicks();

	QueryPerformanceFrequency(&freq);
	int f= freq.LowPart/1000;
//*
	if (GetGraphics()->IsTrace(iGraphics::TRACE_FPS))
	{
		CString s;
		s.Format("FPS: %02d, CKD: %02d, BL: %02d, GLM: %02d, EL: %02d, PP: %02d, Flip: %02d.\n",GetGraphics()->GetFps(),t1/f,t2/f,t3/f,t4/f,t5/f,t6/f);
		GetGraphics()->Trace(s);
	}
//*/
	hr = ClearEnvironment();
	return ;
}

///////////////////////////////////////////////////////////////////////////////////
//notify
///////////////////////////////////////////////////////////////////////////////////

HRESULT cGround::CameraNotify(cCamera* p,eNotifyType eType,long lParam)
{
	if (eType == NOTIFY_POSITION_CHANGED)
	{
		UpdateRegion();
		ClearFarestRegion();
		for (itSpecial it = m_listSpecial.begin(); it!= m_listSpecial.end(); ++it)
		{
			iSpecial* p = (*it);
			if (p->IsActive() && p->IsAcceptEvent(iSpecial::SE_CAMERANOTIFY))
				p->OnEvent(iSpecial::SE_CAMERANOTIFY,0,0);
		}
		GetcMapSmall()->OnCameraUpdate();
	}
	return S_OK;
}

HRESULT cGround::ObjNotify(cGroundObj* p,eNotifyType eType, long lParam )
{
	switch (eType)
	{
	case NOTIFY_MOVING_CHANGING:
		RemovecObj(p);
		break;
	case NOTIFY_MOVING_CHANGED:
		AddcObj(p);
		break;
	case NOTIFY_POSITION_CHANGING:
		if (!p->IsMoving())
			RemovecObj(p);
		break; 
	case NOTIFY_POSITION_CHANGED:
		if (!p->IsMoving())
			AddcObj(p);
		break;
	}
	return TRUE;
}

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/


/**************************************************************************/
//device
/**************************************************************************/
HRESULT cGround::FinalCleanup()
{
	ClearAllRegion();

	ClearGround();

	ClearDynamic();
	//Setinground false 有必要，因为编辑器可以保存一段关于obj的指针
	for_all(m_listMovingObj,bind2nd(mem_fun1<BOOL,cPlaneHor,BOOL>(&cPlaneHor::SetInGround),FALSE));
	for_all(m_listMovingObj,mem_fun<ULONG,cPlaneHor>(&cPlaneHor::Release));
	m_listMovingObj.clear();

	for_all(m_listSpecial,mem_fun<HRESULT,iSpecial>(&iSpecial::FinalCleanup));
	for_all(m_listSpecial,mem_fun<ULONG,iSpecial>(&iSpecial::Release));
	m_listSpecial.clear();

	return S_OK;
}

HRESULT cGround::InitDeviceObjects()
{
	for_all(m_listRegion,mem_fun<HRESULT,cRegion>(&cRegion::InitDeviceObjects));
	for_all(m_listMovingObj,mem_fun<HRESULT,cPlaneHor>(&cPlaneHor::InitDeviceObjects));
	for_all(m_listSpecial,mem_fun<HRESULT,iSpecial>(&iSpecial::InitDeviceObjects));
	return S_OK;
};

HRESULT cGround::RestoreDeviceObjects()
{
	for_all(m_listRegion,mem_fun<HRESULT,cRegion>(&cRegion::RestoreDeviceObjects));
	for_all(m_listMovingObj,mem_fun<HRESULT,cPlaneHor>(&cPlaneHor::RestoreDeviceObjects));
	for_all(m_listSpecial,mem_fun<HRESULT,iSpecial>(&iSpecial::RestoreDeviceObjects));
	return S_OK;
};                     

HRESULT cGround::InvalidateDeviceObjects()
{
	for_all(m_listRegion,mem_fun<HRESULT,cRegion>(&cRegion::InvalidateDeviceObjects));
	for_all(m_listMovingObj,mem_fun<HRESULT,cPlaneHor>(&cPlaneHor::InvalidateDeviceObjects));
	for_all(m_listSpecial,mem_fun<HRESULT,iSpecial>(&iSpecial::InvalidateDeviceObjects));
	return S_OK;
};

HRESULT cGround::DeleteDeviceObjects()
{
	for_all(m_listRegion,mem_fun<HRESULT,cRegion>(&cRegion::DeleteDeviceObjects));
	for_all(m_listMovingObj,mem_fun<HRESULT,cPlaneHor>(&cPlaneHor::DeleteDeviceObjects));
	for_all(m_listSpecial,mem_fun<HRESULT,iSpecial>(&iSpecial::DeleteDeviceObjects));
	return S_OK;
};

////////////////////////////////////////////////////////////////////////////
//坐标变换	
////////////////////////////////////////////////////////////////////////////
float cGround::GetGroundHeight(cPoint ptGround)
{
	return 0;
}

HRESULT cGround::ScreenToGround(CPoint ptScreen, cPoint& ptGround)
{
	cPoint pt(ptScreen.x,ptScreen.y,0);
	pt.z = GetGroundHeight(pt);
	//pt.z = 当前地形的高度
	GetcCamera()->ScreenToGround(pt,ptGround);
	return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//special
////////////////////////////////////////////////////////////////////////////
void cGround::DrawSpecial()
{
	for (itSpecial it = m_listSpecial.begin(); it!= m_listSpecial.end(); ++it)
	{
		iSpecial* p = (*it);
		if (p->IsActive())
		{
			if (GetGraphics()->IsShow(iGraphics::GS_SPECIAL))
				p->Draw();
		}
	}
}

void cGround::LoadSpecial(LPCSTR szFile)
{
	T_LoadList<SpecialList,iSpecial>(m_listSpecial,szFile,GetGraphics());
}

void cGround::SaveSpecial(LPCSTR szFile)
{
	T_SaveList(m_listSpecial,szFile);
}

HRESULT	cGround::AddiSpecial(iSpecial* p)
{
	p->AddRef();
	m_listSpecial.push_back(p);
	return S_OK;
}

HRESULT	cGround::RemoveiSpecial(iSpecial* p)
{
	m_listSpecial.remove(p);
	p->Release();
	return S_OK;
}

iSpecial* cGround::SelectiSpecial(POINT ptScreen) 
{
	cPoint ptGround;
	ScreenToGround(ptScreen,ptGround);
	for (itSpecial it = m_listSpecial.begin(); it != m_listSpecial.end(); ++it)
	{
		iSpecial* p = (*it);
		if (p->Contain(ptGround))
		{
			p->AddRef();
			return p;
		}
	}
	return NULL;
}

//////////////////////////////////////////////
//game
//////////////////////////////////////////////
BOOL cGround::Game_Load(LPCSTR szMap,cPoint ptCameraTo)
{
	GetcCamera()->New();

	m_param.bPlayering = TRUE;
	Load(szMap);
	iCamera::stParam param;
	param.mask = iCamera::MASK_TO | iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	param.ptTo = ptCameraTo;
	GetcCamera()->setParam(param);

	return TRUE;
}

BOOL cGround::FindWalkPath(std::list<cPoint>& ptlist, cPoint ptStart, cPoint ptEnd)
{
	return GetMapObstacle()->FindWalkPath(ptlist,ptStart,ptEnd);
}
