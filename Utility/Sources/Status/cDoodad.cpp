// cDoodad.cpp: implementation of the cDoodad class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cDoodad.h"

#include "iGraphics.h"
#include "iItem.h"
#include "fileop.h"
#include "planedoodad.h"
#include "objutility.h"
#include "iGraphicsUtility.h"

#include "cStatusGroup.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int cDoodad::s_nDoodad = -1;

cDoodad::cDoodad(cStatusGroup* pGroupStatus)
{
	m_pGroupStatus = pGroupStatus;
	m_ulCount = 1;
}

cDoodad::~cDoodad()
{
	Clear();
}

/**************************************************************************/
//com interface
/**************************************************************************/
ULONG cDoodad::AddRef() 
{
	return ++m_ulCount;
}

ULONG cDoodad::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

int cDoodad::GetID() 
{
	long npcnum = 0;
	AppGetDynamicNum("doodadnum",npcnum);
	return npcnum;
}

/**************************************************************************/
//graphics
/**************************************************************************/
iGraphics* cDoodad::GetGraphics()
{
	return cStatus::GetGraphics();
}

iGround* cDoodad::GetGround()
{
	return cStatus::GetGround();
}

iCamera* cDoodad::GetCamera()
{
	return cStatus::GetCamera();
}

void cDoodad::BeginMacroCommand()
{
	cStatus::BeginMacroCommand();
}

void cDoodad::EndMacroCommand()
{
	cStatus::EndMacroCommand();
}

CString cDoodad::GetGroupFile()
{
	CString s;
	BOOL bFirst = true;
	iObj* p = m_listUnit.front().pObj;
	stGroundObjParam param;
	param.ClearMask();
	p->GetParam(param);
	if (param.lGroupID == 0 || strlen(param.szGroup) == 0)
	{
		p = m_listUnit.back().pObj;
		param.ClearMask();
		p->GetParam(param);
		if (param.lGroupID == 0 || strlen(param.szGroup) == 0)
		{
			AfxMessageBox("请最先或最后选择组件图素!");
			return s;
		}
	}
	s = param.szGroup;
	return s;
}

/**************************************************************************/
//plane
/**************************************************************************/
void cDoodad::Clear()
{
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjTrace(p,false);
		SetObjSelect(p,false);
		p->Release();
	}
	m_listUnit.clear();
}

void cDoodad::Delete()
{
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjTrace(p,false);
		SetObjSelect(p,false);
		RemoveObj(p);	
		RELEASE(p);
	}
	m_listUnit.clear();
	EndMacroCommand();
}

void cDoodad::Offset(cPoint pt,bool bRunnow)
{
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		OffsetObj(p,pt,bRunnow);	
	}
	EndMacroCommand();
}

void cDoodad::AddLayer(int layer)
{
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		stUnit st = (*it);
		switch (st.eElement)
		{
		case GT_LIGHT:
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			break;
		default:			//plane
			{
				stPlaneParam param;
				param.ClearMask();
				(*it).pObj->GetParam(param);
				param.nLayer += layer;
				param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_LAYER);
				(*it).pObj->setParam(param);
			}
			break;
		}
	}
}

void cDoodad::OffsetTo(CPoint ptScreen,bool bRunnow)
{
/*
	if (m_listUnit.size() == 0)
		return;
	iObj* p = m_listUnit.front().pObj;
	cPoint ptGround;
	p->GetOffset(ptScreen,ptGround);

	OffsetTo(ptGround,bRunnow);
*/
	if (m_listUnit.size() == 0)
		return;

	BeginMacroCommand();

	iGround::stParam param;
	param.mask = iGround::MASK_BUILDTREE;
	param.bBuildTree = FALSE;
	GetGround()->SetParam(param);

	cPoint ptGround;
	ptGround.x = ptScreen.x;
	ptGround.y = ptScreen.y;
	ptGround.z = 0;
//	p->GetOffset(ptScreen,ptGround);
//	GetGround()->ScreenToGround(ptScreen,ptGround);
	GetCamera()->ScreenToGround(ptGround);

	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		stUnit st = (*it);
		OffsetObjTo((*it).pObj,st.ptOffset+ptGround,bRunnow);
	}
	
	EndMacroCommand();
	
	param.bBuildTree = TRUE;
	GetGround()->SetParam(param);
}

void cDoodad::SetGroupFile(LPCSTR szFile,bool bRunNow)
{
	CString s = szFile;
	AppGetShortPath(s);
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjGroupFile(p,s,bRunNow);
	}
	EndMacroCommand();
}

void cDoodad::SetID(int idDoodad,bool bRunNow)
{
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjID(p,idDoodad,bRunNow);
	}
	EndMacroCommand();
}

void cDoodad::Add()
{
	BeginMacroCommand();
	for (itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		AddObj((*it).pObj);
	}
	EndMacroCommand();
}

void cDoodad::OffsetTo(cPoint pt,bool bRunnow)
{
	BeginMacroCommand();
	iGround::stParam param;
	param.mask = iGround::MASK_BUILDTREE;
	param.bBuildTree = FALSE;
	GetGround()->SetParam(param);
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
//must do twice!
		OffsetObjTo(p,pt);	
		OffsetObj(p,(*it).ptOffset,bRunnow);
	}
	param.bBuildTree = TRUE;
	GetGround()->SetParam(param);
	EndMacroCommand();
}

void cDoodad::SetMoving(bool bMoving)
{
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjMoving(p,bMoving);	
	}
	EndMacroCommand();
}

void cDoodad::SetSelect(bool bSelect)
{
	return ;
	BeginMacroCommand();
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjSelect(p,bSelect);	
	}
	EndMacroCommand();
}

void cDoodad::SetTrace(bool bTrace)
{
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		SetObjTrace(p,bTrace);	
	}
}

void cDoodad::AddList(iObjList& ilist)
{
	//for (itUnit it = m_listUnit.begin(); it != m_listUnit.end(); ++it)
	for (itiObj it = ilist.begin(); it!= ilist.end(); ++it)
	{
		bool bFind = false;
		for (itUnit it2 = m_listUnit.begin(); it2!= m_listUnit.end(); ++it2)
		{
			if ((*it2).pObj == *it)
			{
				bFind = true;
				break;
			}
		}
		if (bFind)
		{
			m_listUnit.erase(it2);
			SetObjSelect((*it),false);
			SetObjTrace((*it),false);	
			(*it)->Release();
		}
		else
		{
			(*it)->AddRef();
			SetObjSelect((*it),true);
			stUnit st;
			st.pObj = (*it);
			m_listUnit.push_back(st);
		}
	}
	for_all(ilist,mem_fun<ULONG,iObj>(&iObj::Release));
	
	UpdateOffset();
}

void cDoodad::UpdateOffset()
{
	if (m_listUnit.size() == 0)
		return;
	
	cPoint ptOffset;
	stGroundObjParam param;
	param.ClearMask();
	stUnit st = m_listUnit.front();
	st.pObj->GetParam(param);
	ptOffset = param.ptOffset;
	ptOffset.z = 0;

	listUnit ulist;

	for (itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		param.ClearMask();
		(*it).pObj->GetParam(param);
		stUnit st;
		st.pObj = (*it).pObj;
		st.ptOffset = param.ptOffset - ptOffset;
		ulist.push_back(st);
	}
	m_listUnit.clear();
	m_listUnit.insert(m_listUnit.end(),ulist.begin(),ulist.end());
}

void cDoodad::Select(cRect rcDiamond,iObjList& ilist, stSelect& select)
{
	select.rcGround = rcDiamond;
	select.ePoint = SelectPoint_RectByDiamond;
	SelectFilterEnum e = select.eFilter;
	select.eFilter = SelectFilter_All;
	iObjList listPlane;
	GetGround()->GetiPlane(select,listPlane);
	ilist.insert(ilist.end(),listPlane.begin(),listPlane.end());
	iObjList listLight;
	GetGround()->GetiLight(select,listLight);
	ilist.insert(ilist.end(),listLight.begin(),listLight.end());
	iObjList listObstacle;
	GetGround()->GetiObstacle(select,listObstacle);
	ilist.insert(ilist.end(),listObstacle.begin(),listObstacle.end());
}

void cDoodad::Select(POINT pt,iObjList& ilist, stSelect& select)
{
	iObj* pObj = NULL;

	select.ptScreen = pt;
	select.ePoint = SelectPoint_OneByScreen;
	
	iObjList llist;
	GetGround()->GetiLight(select,llist);
	if (llist.size() > 0)
	{
		pObj = llist.front();
		for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
	}
	else
	{
		GetGround()->GetiObstacle(select,llist);
		if (llist.size() > 0)
		{
			pObj = llist.front();
			for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
		}
		else 
		{
			GetGround()->GetiPlane(select,llist);
			ASSERT(llist.size() <= 1);
			if (llist.size() > 0)
			{
				pObj = llist.front();
				for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
			}
		}
	}

	if(pObj == NULL)
		return;

	stGroundObjParam param;
	param.ClearMask();
	pObj->GetParam(param);
	if (param.lGroupID != 0)
		GetGround()->GetiObj(param.lGroupID,ilist);
	else
	{
		pObj->AddRef();
		ilist.push_back(pObj);
	}
}

void cDoodad::Select(cRect rcDiamond, stSelect& select)
{
	Clear();
	AddSelect(rcDiamond,select);
}

void cDoodad::Select(POINT pt,stSelect& select)
{
	Clear();
	AddSelect(pt, select);
}

void cDoodad::AddSelect(POINT pt, stSelect& select)
{
	iObjList ilist;
	Select(pt, ilist, select);	
	AddList(ilist);
}

void cDoodad::AddSelect(cRect rcDiamond, stSelect& select)
{
	iObjList ilist;
	Select(rcDiamond,ilist,select);	
	AddList(ilist);
}

BOOL cDoodad::Create(CPoint point,LPCSTR szFile, bool bRefresh)
{
	if (strlen(szFile) == 0)
		return FALSE;
	ASSERT(m_listUnit.size() == 0);

	Load(szFile);
	CStringList listRefresh;
	
	listUnit ulist;
	for (itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		stUnit st = (*it);
		iObj* pPlane = NULL;
		switch (st.eElement)
		{
		case GT_LIGHT:
			{
				stLightParam param = st.paramLight;
				CreateLight(GetGraphics(),pPlane,param);
			}
			break;
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			{
				stObstacleParam param;
				param.eObstacle = st.eObstacle;
				param.nPoint = st.nPoint;
				for (int i=0; i<param.nPoint; i++)
					param.aPoint[i] = st.aPoint[i];
				CreateObstacle(GetGraphics(),pPlane,param);
			}
			break;
		default:
#pragma message (ATTENTION("refreshed the spr file but not "))
			if (bRefresh)
			{
				if (listRefresh.Find(st.sName) == NULL)
				{
					CString s =st.sName;
					iItem* pItem = Common_CreateItem(s,iItem::FLAG_RELOAD);
					if (pItem == NULL)
						break;
					wrap_ptr<iItem> ptr(pItem);
					CString strFile;
					if (GetItemValue(pItem,"file",strFile))
					{
						listRefresh.AddTail(s);
						int pos = s.ReverseFind('\\');
						if (pos != -1)
						{
							strFile = s.Left(pos) + "\\" + strFile;
							AppGetShortPath(strFile);
							GetGraphics()->RefreshSpr(strFile);
						}
					}
				}
			}
			CreatePlane(GetGraphics(),pPlane,st.sName,st.nFrame);
			GU_SetPlaneLayer(pPlane,st.nLayer);
			if (!st.sDataName.IsEmpty())
			{
				m_pGroupStatus->AfterCreatePlane(pPlane,st.sName,st.sDataName);
			}
			break;
		}
		st.pObj = pPlane;
		SetObjSelect(pPlane,true);
		ulist.push_back(st);
	}

	m_listUnit.clear();
	m_listUnit.insert(m_listUnit.end(),ulist.begin(),ulist.end());
	//create each plane
	int idDoodad = GetID();
	SetID(idDoodad,true);
	SetGroupFile(szFile,true);
	OffsetTo(point,true);
	Add();

	return TRUE;
}

void cDoodad::Save()
{
	SaveAs(m_strName);
}

void cDoodad::Load(LPCSTR szName)
{
	m_strName = szName;

	long e = iItem::FLAG_LOAD;
	iItem* pItem = Common_CreateItem(szName,e);
	if (pItem == NULL)
		return;
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	VERIFY(GetItemValue(pItem,"objnum",sValue));
	int n;
	StringToValue(sValue,n);
	if (n == 0)
		return;

	for (int i=0; i<n; i++)
	{
		stSection* pSection = pItem->GetIndexSection(i);
		if (pSection == NULL)
			continue;

		stUnit st;
		st.pObj = NULL;
		int n;
		VERIFY(GetSectionValue(pSection,"elementtype",n));
		st.eElement = (ElementEnum) n;
		VERIFY(GetSectionValue(pSection,"objoffset",st.ptOffset));
		switch (st.eElement)
		{
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			{
				int n;
				VERIFY(GetSectionValue(pSection,"obstacletype",n));
				st.eObstacle = (ObstacleEnum)n;
				VERIFY(GetSectionValue(pSection,"obstaclepointnum",st.nPoint));
				ASSERT(st.nPoint < MAX_POINT);
				CString sValue;
				VERIFY(GetSectionValue(pSection,"obstaclepointarray",sValue));
				StringToPointArray(sValue,st.aPoint);
				st.eKind = KIND_NORMAL;
				if (GetSectionValue(pSection,"obstaclekind",sValue))
					st.eKind = (KindEnum)StringToPlaneTableValue(TP_KIND,sValue);
			}
			break;
		case GT_LIGHT:
			{
				stLightParam& param = st.paramLight;
				GetSectionValue(pSection,"lightcolor",param.crColor);
				GetSectionValue(pSection,"lightradius",param.fRange);

				GetSectionValue(pSection,"varrangemax",param.fRangeVarMax);
				GetSectionValue(pSection,"varrangespeed",param.fRangeVarSpeed);
				GetSectionValue(pSection,"varrangeframe",param.fRangeVarFrame);

				GetSectionValue(pSection,"posjumpmax",param.fPosJumpMax);
				GetSectionValue(pSection,"posjumpspeed",param.ptPosJumpSpeed);
				GetSectionValue(pSection,"posjumpframe",param.fPosJumpFrame);

				GetSectionValue(pSection,"frameflash",param.fFlashFrame);
				GetSectionValue(pSection,"framedark",param.nFlashDarkFrame);
			}
			break;
		default:	//plane
			{
				VERIFY(GetSectionValue(pSection,"planefile",st.sName));
				VERIFY(GetSectionValue(pSection,"planeframe",st.nFrame));
				GetSectionValue(pSection,"planedatafile",st.sDataName);
				GetSectionValue(pSection,"planelayer",st.nLayer);
			}
			break;
		}

		m_listUnit.push_back(st);
	}
}

//存储一个plane的阻挡信息,存储到plane的模板文件
void cDoodad::SaveFirstPlane()
{
	CWaitCursor cursor;
	//listunit 必须调整，是第一个物件是Plane而不是Obstacle
	CString strPlaneTxt;
	int iPlaneTxt;
	float fHeight;
	BOOL bFirst = true;
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		stObjParam param;
		param.ClearMask();
		p->GetParam(param);
		switch (param.eElement)
		{
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			if (bFirst)
			{
				AfxMessageBox("请先选中plane");
				break;
			}
			break;
		case GT_LIGHT:
			if (bFirst)
			{
				AfxMessageBox("请先选中plane");
				break;
			}
			break;
		default://plane
			if (strPlaneTxt.IsEmpty())
			{
				stPlaneParam param;
				param.ClearMask();
				p->GetParam(param);
				iPlaneTxt = param.stResA.stRes.iFrame;
				strPlaneTxt = param.szTemplate;
				fHeight = param.ptOffset.z;
				break;
			}
			else
			{
				AfxMessageBox("多于一个plane文件");
				return;
			}
		}
		bFirst = false;
	}
	if (strPlaneTxt.IsEmpty())
	{
		AfxMessageBox("清选择一个plane文件");
		return;
	}
	CString strSrc = strPlaneTxt;
	AppGetFullPath(strSrc);
	CString strBack = strSrc.Left(strSrc.GetLength() - 3)+"bak";
	AppCopyFile(strSrc,strBack);	
	iItem* pItem = Common_CreateItem(strPlaneTxt,iItem::FLAG_LOAD);
	ASSERT(pItem);
	wrap_ptr<iItem> ptr(pItem);
	int index = iPlaneTxt;
	stSection* pSection = pItem->GetIndexSection(index,FALSE);
	int nObstacle = 0;
	for(it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		iObj* p = (*it).pObj;
		cPoint ptOffset = (*it).ptOffset;
		stObjParam param;
		param.ClearMask();
		p->GetParam(param);
		switch (param.eElement)
		{
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			{
				stObstacleParam param;
				param.ClearMask();
				p->GetParam(param);
				for (int i=0; i<param.nPoint; i++)
				{
					param.aPoint[i] += ptOffset;
				}
				int nTemp = param.eObstacle;
				CString sKey;
				sKey.Format("obstacletype(%d)",nObstacle);
				SetSectionValue(pSection,sKey,nTemp);
				sKey.Format("obstaclepointnum(%d)",nObstacle);
				SetSectionValue(pSection,sKey,param.nPoint);
				CString sValue;
				PointArrayToString(param.aPoint,param.nPoint,sValue);
				sKey.Format("obstaclepointarray(%d)",nObstacle);
				SetSectionValue(pSection,sKey,sValue);
				sKey.Format("obstaclekind(%d)",nObstacle);
				//GetSectionValue(pSection,sKey,cast_int(param.eKind));
				sValue =  GetTablePlaneValue(TP_KIND,param.eKind);
				SetSectionValue(pSection,sKey,sValue);
				nObstacle++;
				break;
			}
		case GT_LIGHT:
			break;
		default:
			break;
		}
	}
	SetSectionValue(pSection,"obstaclenum",nObstacle);
	SetSectionValue(pSection,"height",fHeight);
	pItem->Save();

	//copy file to net?
	CopyFileToNet(strPlaneTxt);
}

BOOL cDoodad::CopyFileToNet(LPCSTR szFile) 	//szFile already short name
{
	CString sFile;
	sFile = "users\\rm_path.ini";
	AppGetFullPath(sFile);
	iItem* pItem = Common_CreateItem(sFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);

	CString m_strNetRoot;
	VERIFY(GetItemValue(pItem,"网络资源",m_strNetRoot));
	int pos = m_strNetRoot.Find("\\游戏资源");
	ASSERT(pos != -1);
	m_strNetRoot = m_strNetRoot.Left(pos);
	
	CString strTxt = szFile;
	CString s = m_strNetRoot + szFile;
	AppGetFullPath(strTxt);
	BOOL b = AppCopyFile(strTxt,s);
	if (b)
	{
		CString sLog = s.Left(s.GetLength() - 4)+".log";
		b = AppWriteSimpleLogFile(sLog);
	}
	return b;
}

BOOL cDoodad::GetFirstGroupFile(CString& s)
{
	BOOL bFirst = true;
	iObj* p = m_listUnit.front().pObj;
	stGroundObjParam param;
	param.ClearMask();
	p->GetParam(param);
	if (param.lGroupID == 0 || strlen(param.szGroup) == 0)
	{
		return FALSE;

	}
	s = param.szGroup;
	return TRUE;
}

//修改组件的信息
void cDoodad::SaveFirstDoodad()
{
	CString szShort = GetGroupFile();
	CString strSrc = szShort;
	AppGetFullPath(strSrc);
	CString strBack = strSrc.Left(strSrc.GetLength() - 3)+"bak";
	AppCopyFile(strSrc,strBack);	

	SaveAs(szShort);
}

void cDoodad::SaveAs(LPCSTR szName)
{
	if (strlen(szName) == 0)
		return;

	m_strName = szName;
	
	iItem* pItem = Common_CreateItem(szName,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);

	CString sValue;
	int n = m_listUnit.size();
	SetItemValue(pItem,"icon","group");
	LPCSTR szType = m_pGroupStatus->GetTypeString();
	SetItemValue(pItem,"type",szType);
	SetItemValue(pItem,"grouptype","normal");
	SetItemValue(pItem,"objnum",n);
	int index = 0;
	CString sWork;
	AppGetWorkingFolder(sWork);
	for(itUnit it = m_listUnit.begin(); it!= m_listUnit.end(); ++it)
	{
		stSection* pSection = pItem->GetIndexSection(index, FALSE);
		iObj* p = (*it).pObj;
		stObjParam param;
		param.ClearMask();
		p->GetParam(param);

		SetSectionValue(pSection,"elementtype",param.eElement);
		SetSectionValue(pSection,"objoffset",(*it).ptOffset);
		switch (param.eElement)
		{
		case 1006:
		case GT_OBSTACLE:
		case GT_OBSTACLE_OLD:
			{
				stObstacleParam param;
				param.ClearMask();
				p->GetParam(param);
/*
				cPoint pt = param.aPoint[0];
				for(int i=0; i<param.nPoint; i++)
				{
					param.aPoint[i] -= pt;
				}
*/
				int nTemp = param.eObstacle;
				SetSectionValue(pSection,"obstacletype",nTemp);
				SetSectionValue(pSection,"obstaclepointnum",param.nPoint);
				CString sValue;
				PointArrayToString(param.aPoint,param.nPoint,sValue);
				SetSectionValue(pSection,"obstaclepointarray",sValue);
//				SetSectionValue(pSection,"obstaclekind",cast_int(param.eKind));
				sValue = GetTablePlaneValue(TP_KIND,param.eKind);
				SetSectionValue(pSection,"obstaclekind",sValue);
			}
			break;
		case GT_LIGHT:
			{
				stLightParam param;
				param.ClearMask();
				p->GetParam(param);

				SetSectionValue(pSection,"lightcolor",param.crColor);
				SetSectionValue(pSection,"lightradius",param.fRange);

			//	SetSectionValue(pSection,"range",m_param.fRange);
			//	SetSectionValue(pSection,"mainlight",m_param.bMainlight);

				SetSectionValue(pSection,"varrangemax",param.fRangeVarMax);
				SetSectionValue(pSection,"varrangespeed",param.fRangeVarSpeed);
				SetSectionValue(pSection,"varrangeframe",param.fRangeVarFrame);

				SetSectionValue(pSection,"posjumpmax",param.fPosJumpMax);
				SetSectionValue(pSection,"posjumpspeed",param.ptPosJumpSpeed);
				SetSectionValue(pSection,"posjumpframe",param.fPosJumpFrame);

				SetSectionValue(pSection,"frameflash",param.fFlashFrame);
				SetSectionValue(pSection,"framedark",param.nFlashDarkFrame);
			}
			break;
		default:
			{
				stPlaneParam param;
				CString sDataName;
				param.ClearMask();
				p->GetParam(param);
				(*it).nFrame = param.stResA.stRes.iFrame;
				SetSectionValue(pSection,"planefile",param.szTemplate);
				if (cStatus::GetObjIniFile(p,sDataName))
				{
					CString sDes; 
					//动态组件//defaultgroup//
					CString sFolder = m_strName.Left(m_strName.GetLength() - 4);
					sDes.Format("%s\\%d.ini",sFolder,index);
					AppBuildPath(sDes);
					AppCopyFile(sDataName,sDes);
					SetSectionValue(pSection,"planedatafile",sDes);
				}
				SetSectionValue(pSection,"planeframe",(*it).nFrame);
				(*it).nLayer = param.nLayer;
				SetSectionValue(pSection,"planelayer",(*it).nLayer);
			}
			break;
		}
		index++;
	}
	pItem->Save();
	CopyFileToNet(szName);
}
