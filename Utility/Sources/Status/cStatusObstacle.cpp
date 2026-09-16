// cStatusObstacle.cpp: implementation of the cStatusObstacle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cStatusObstacle.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iItem.h"
#include "fileop.h"
#include "objUtility.h"

#include "Status.h"
#include "Obstacledlg.h"
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strObstacleType = "障碍类型.txt";
const CString c_strObstacleSeam = "使用线段障碍.txt";
const CString c_strObstacleHeight = "障碍高度.txt";
//const CString c_strPointModify = "修改点.txt";
const CString c_strObstacleCapture= "截获障碍的名称.txt";

class cAssertValue
{
public:
	int* _p;
	int _max;
	cAssertValue(int& n, int MAX){_p = &n;_max = MAX;ASSERT(n <= MAX);}
	~cAssertValue(){ASSERT(*_p < _max);}
};

cStatusObstacle::cStatusObstacle()
{
	m_pObstacleLeft = NULL;
	m_pObstacleRight = NULL;

	m_bTrace = false;
	m_nPoint = 0;

	m_fHeight = 0;
	m_iPointModify = -1;
	m_bSeam= FALSE;
}

cStatusObstacle::~cStatusObstacle()
{
	Clear();
}

void cStatusObstacle::Clear()
{
	cStatus::Clear();

	SetObjSelect(m_pObstacleLeft,false);
	SetObjTrace(m_pObstacleLeft,false);
	SetObjSelect(m_pObstacleRight,false);
	SetObjTrace(m_pObstacleRight,false);
	RELEASE(m_pObstacleLeft);
	RELEASE(m_pObstacleRight);
}

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
int cStatusObstacle::GetPointNum()
{
	return m_nPoint;
}

void cStatusObstacle::SetObstacleSeam(bool b)
{
	m_bSeam = b;
	iGround::stParam param;
	param.mask = iGround::MASK_OBSTACLE_TYPE;
	if (b)
		param.eObstalceType = OT_SEAM;
	else
		param.eObstalceType = OT_TILE;
	GetGround()->SetParam(param);
}

BOOL cStatusObstacle::Create()
{
	cStatus::Create();

	CString sFolder = "游戏资源\\障碍\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strObstacleType,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_eType = (ObstacleEnum)set.n ;	
	}
	if (AppFindFile(c_strObstacleSeam,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_BOOL);
		SetObstacleSeam(set.b);
	}
/*	if (AppFindFile(c_strPointModify,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_iPointModify = set.n ;	
	}
*/	if (AppFindFile(c_strObstacleCapture,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		m_strObstacleCapture = set.str.c_str();
	}
	return TRUE;
}

BOOL cStatusObstacle::OnSetFocus()
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);

	GU_SetSelectSize(GetSelect(),0.1f,0.2f);
	GU_ChangeSelectColor(GetSelect(),cColor(255,0,0));

	m_bShowObstacle = SetShow(iGraphics::GS_OBSTACLE);
	return TRUE;
}

BOOL cStatusObstacle::OnLostFocus()
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);

	if (m_bShowObstacle == FALSE)
		ClearShow(iGraphics::GS_OBSTACLE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////

void cStatusObstacle::OnSelectedSetting(LPCTSTR szFile)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strObstacleType)
		m_eType = (ObstacleEnum) m_stSetting.n ;
	if (sName == c_strObstacleSeam)
		SetObstacleSeam(m_stSetting.b);
//	if (sName == c_strPointModify)
//		m_iPointModify = m_stSetting.n ;
	if (sName == c_strObstacleCapture)
		m_strObstacleCapture = m_stSetting.str.c_str();
}

BOOL cStatusObstacle::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	int eGeometry;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	if (GetItemValue(pItem,"geometrytype",eGeometry))
	{
		m_eGeometry  = (ObstacleGeometryEnum) eGeometry;
		SetSelectFile(szFile,nFrame);
		m_nPoint = 0;
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////

BOOL cStatusObstacle::MoveSelect(CPoint point)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (m_pObstacleLeft == NULL)
	{
		if (!IsSelected())
		{
			ClearSelect();
			return FALSE;
		}
	
		MoveiSelect(SELECT_SQUARE,point);
	}
	else 
	{
		ASSERT(m_pObstacleRight == NULL);
		if (m_eGeometry == OBSTACLE_GEOMETRY_USERDEFINE)
		{
			OffsetObjTo(m_pObstacleLeft,point);
		}
		else
		{
			if (m_nPoint < 1)
				return FALSE;

			GetGround()->ScreenToGround(point,m_aPoint[m_nPoint-1]);
			SetObstacleGeometry(m_pObstacleLeft);
		}
	}
	return TRUE;
}

BOOL cStatusObstacle::OnCameraMove(CPoint point)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	return MoveSelect(point);
}

BOOL cStatusObstacle::OnMouseMove(UINT nFlags, CPoint point)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	cStatus::OnMouseMove(nFlags,point);
	MoveSelect(point);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
void cStatusObstacle::ToggleObstacleTrace()
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	m_bTrace = !m_bTrace;
	SetObjTrace(m_pObstacleLeft,m_bTrace);
	SetObjTrace(m_pObstacleRight,m_bTrace);
}

/////////////////////////////////////////////////////////////////////
//Obstacle
/////////////////////////////////////////////////////////////////////
void cStatusObstacle::CaptureObstacle(iObj* pObstacle)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (pObstacle == NULL)
		return;

	stObstacleParam param;
	param.ClearMask();
	pObstacle->GetParam(param);

	cPoint pt = param.aPoint[0];
	for(int i=0; i<param.nPoint; i++)
	{
		param.aPoint[i] -= pt;
	}

	CString strFile;
	GetObstacleFileString(strFile);

	CFileFind f;
	bool bNew = true;
	if (f.FindFile(strFile))
		bNew = false;

	iItem* pItem = Common_CreateItem(strFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"icon","obstacle");
	SetItemValue(pItem,"type","obstacle");
	SetItemValue(pItem,"obstacletype",cast_int(param.eObstacle));
	int n = 4;
	SetItemValue(pItem,"geometrytype",n);
	SetItemValue(pItem,"pointnum",param.nPoint);
	CString sValue;
	PointArrayToString(param.aPoint,param.nPoint,sValue);
	SetItemValue(pItem,"pointarray",sValue);
	pItem->Save();
	if (bNew)
		SetNewFile(strFile);	
}

void cStatusObstacle::AddSelectPoint(iObj* p)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (p == NULL)
		return;
	stObstacleParam param;
	p->GetParam(param);
	if (param.iPoint == -1)
		return;
	if (param.iPoint <0 || param.iPoint >= param.nPoint)
		return;
	if (param.nPoint == stObstacleParam::MAX_POINT)
		return;
	for (int i=param.nPoint+1; i>param.iPoint; i--)
	{
		param.aPoint[i] = param.aPoint[i-1];
	}
	if (param.iPoint > 0)
	{
		param.aPoint[param.iPoint] = param.aPoint[param.iPoint-1]+param.aPoint[param.iPoint+1];
		param.aPoint[param.iPoint].x /= 2.f;
		param.aPoint[param.iPoint].y /= 2.f;
		param.aPoint[param.iPoint].z /= 2.f;
	}
	param.nPoint ++;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);
	SetObjParam(p,param);
}

void cStatusObstacle::DeleteSelectPoint(iObj* p)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (p == NULL)
		return;
	stObstacleParam param;
	p->GetParam(param);
	if (param.iPoint == -1)
		return;
	if (param.iPoint <0 || param.iPoint >= param.nPoint)
		return;
	if (param.nPoint <= 1)
		return;
	for (int i=param.iPoint; i<param.nPoint-1; i++)
	{
		param.aPoint[i] = param.aPoint[i+1];
	}
	param.nPoint --;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO );
	SetObjParam(p,param);
	GU_SetObstacleSelectPoint(p,-1);
}

void cStatusObstacle::SelectObstacle(CPoint point, iObj*& pObstacle)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	ASSERT(pObstacle == NULL);
	iObjList llist;
	POINT ptScreen = {point.x,point.y};
	GetGround()->GetiObstacleByPoint(ptScreen,llist);
	if (llist.size() > 0)
	{
		pObstacle = llist.front();
		pObstacle->AddRef();
		stObstacleParam param;
		param.ClearMask();
		pObstacle->GetParam(param);
		cPoint pt;
		GetCamera()->ScreenToGround(point,pt);
		param.iPoint = -1;
		if (param.nPoint > 1)
		{
			for (int i=0; i<param.nPoint; i++)
			{
				cPoint p = param.aPoint[i] - (pt-param.ptOffset);
				if( fabs(p.x) + fabs(p.y) < .4f)
				{
					m_iPointModify = i;
					param.iPoint = i;
				}
			}
		}
		GU_SetObstacleSelectPoint(pObstacle,param.iPoint);
//这样的写法导致无法还原
	}
	for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
}

void cStatusObstacle::OffsetObstalcePoint(iObj* pObstacle,int iPoint,cPoint ptOffset)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (pObstacle == NULL || iPoint < 0)
		return;
	stObstacleParam param;
	param.ClearMask();
	pObstacle->GetParam(param);
	if (iPoint >= param.nPoint)
		return;

	if (iPoint == 0 || iPoint == param.nPoint-1 )
	{
		if (param.nPoint > 2 && param.aPoint[0] == param.aPoint[param.nPoint-1])
		{
			param.aPoint[param.nPoint-1-iPoint] += ptOffset;
		}
	}
	param.aPoint[iPoint] += ptOffset;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);
	SetObjParam(pObstacle,param);
}

void cStatusObstacle::SetObstacleGeometry(iObj* pObstacle)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	stObstacleParam param;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);
	param.nPoint = m_nPoint;
	for (int i=0; i<m_nPoint; i++)
	{
		param.aPoint[i] = m_aPoint[i] - m_aPoint[0];
	}
	SetObjParam(pObstacle,param);
	OffsetObjTo(pObstacle,m_aPoint[0]);
}

void cStatusObstacle::CreateObstacle(CPoint point,iObj*& pObstacle)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	stObstacleParam param;
	//user define
	iItem* pItem = Common_CreateItem(GetSelectFile(),iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	param.eObstacle = m_eType;
	GetItemValue(pItem,"obstacletype",cast_int(param.eObstacle));

	GetGraphics()->CreateGraphicsElement(GT_OBSTACLE,&pObstacle);
	GU_SetObstacleType(pObstacle,param.eObstacle);

	if (GetItemValue(pItem,"pointnum",param.nPoint))
	{
		CString sValue;
		VERIFY(GetItemValue(pItem,"pointarray",sValue));
		StringToPointArray(sValue,param.aPoint);
		GU_SetObstaclePointinfo(pObstacle,param);
		pObstacle->GetOffset(point,param.ptOffset);
		GU_OffsetObjTo(pObstacle,param.ptOffset);
	}
	else
	{
		SetObstacleGeometry(pObstacle);
	}

	AddObj(pObstacle);

	ClearSelect();
}

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////

BOOL cStatusObstacle::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);
	const float off = 0.1f;

	int nColor = 5;
	float fHeight = 0.1f;
	if (IsShiftDown())
	{
		nColor *= -1;
		fHeight *= -1;
	}

	float fDis = 0.2f;
	
	switch (nChar)
	{
	case 0xbb: //+
		AddSelectPoint(m_pObstacleRight);
		break;
	case 0xbd: //-
	case VK_BACK:
		DeleteSelectPoint(m_pObstacleRight);
		break;
	case VK_DELETE:
	case VK_ESCAPE:
		RemoveAndReleaseObj(m_pObstacleLeft);
		RemoveAndReleaseObj(m_pObstacleRight);
		break;
	case VK_SPACE:
		Clear();
		break;
	case 'W':
		OffsetObj(m_pObstacleRight,cPoint(0,-off));
		break;
	case 'S':
		OffsetObj(m_pObstacleRight,cPoint(0,off));
		break;
	case 'A':
		OffsetObj(m_pObstacleRight,cPoint(-off,0));
		break;
	case 'D':
		OffsetObj(m_pObstacleRight,cPoint(off,0));
		break;
	case 'Q':
		OffsetObj(m_pObstacleRight,cPoint(-off,-off));
		break;
	case 'E':
		OffsetObj(m_pObstacleRight,cPoint(off,-off));
		break;
	case 'Z':
		OffsetObj(m_pObstacleRight,cPoint(-off,off));
		break;
	case 'C':
		OffsetObj(m_pObstacleRight,cPoint(off,off));
		break;

	case 'P':
		OnObstacleProperty(m_pObstacleRight);
		break;
/*
	case 'H':
		AddObjHeight(m_pObstacleLeft,fHeight);
		break;
*/
	case VK_NUMPAD8:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(0,-off));
		break;
	case VK_NUMPAD2:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(0,off));
		break;
	case VK_NUMPAD4:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(-off,0));
		break;
	case VK_NUMPAD6:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(off,0));
		break;
	case VK_NUMPAD7:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(-off,-off));
		break;
	case VK_NUMPAD9:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(off,-off));
		break;
	case VK_NUMPAD1:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(-off,off));
		break;
	case VK_NUMPAD3:
		OffsetObstalcePoint(m_pObstacleRight,m_iPointModify,cPoint(off,off));
		break;

	case VK_F7:
		ToggleObstacleTrace();
		break;
	case 'J':
		if (IsCtrlDown())
			CaptureObstacle(m_pObstacleRight);
		break;
	}
	return TRUE;
}

BOOL cStatusObstacle::OnLButtonDown(UINT nFlags, CPoint point)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (m_pObstacleLeft)
	{
		ASSERT(m_pObstacleRight == NULL);
		switch (m_eGeometry)
		{
		case OBSTACLE_GEOMETRY_SEAM:
			GetGround()->ScreenToGround(point,m_aPoint[1]);
			SetObstacleGeometry(m_pObstacleLeft);
			SetObjSelect(m_pObstacleLeft,false);
			SetObjTrace(m_pObstacleLeft,false);
			RELEASE(m_pObstacleLeft);
			return TRUE;
		case OBSTACLE_GEOMETRY_POINT:
		case OBSTACLE_GEOMETRY_USERDEFINE:
			SetObjSelect(m_pObstacleLeft,false);
			SetObjTrace(m_pObstacleLeft,false);
			RELEASE(m_pObstacleLeft);
			return TRUE;
		case OBSTACLE_GEOMETRY_POLYGON:
			{
				if (m_nPoint >= MAX_POINT - 1)
					return FALSE;
				if (m_nPoint < 1)
					return FALSE;
				cPoint pt;
				GetGround()->ScreenToGround(point,pt);
				if (fabs(pt.x - m_aPoint[0].x) + fabs(pt.y - m_aPoint[0].y) < 0.5f)
				{
					m_aPoint[m_nPoint-1] = m_aPoint[0];
					SetObstacleGeometry(m_pObstacleLeft);
					SetObjSelect(m_pObstacleLeft,false);
					SetObjTrace(m_pObstacleLeft,false);
					RELEASE(m_pObstacleLeft);
				}
				else
				{
					GetGround()->ScreenToGround(point,m_aPoint[m_nPoint-1]);
					GetGround()->ScreenToGround(point,m_aPoint[m_nPoint]);
					m_nPoint++;
					SetObstacleGeometry(m_pObstacleLeft);
				}
				return TRUE;
			}
		}
		return TRUE;
	}

	SetObjSelect(m_pObstacleRight,false);
	SetObjTrace(m_pObstacleRight,false);
	RELEASE(m_pObstacleRight);

	if (!IsSelected())
	{
		SelectObstacle(point,m_pObstacleLeft);
		SetObjSelect(m_pObstacleLeft,true);
		SetObjTrace(m_pObstacleLeft,m_bTrace);
		if (m_pObstacleLeft)
		{
			m_eGeometry = OBSTACLE_GEOMETRY_USERDEFINE;
			ClearSelect();
		}
	}
	else
	{
		if ( m_eGeometry == OBSTACLE_GEOMETRY_USERDEFINE)
		{
			CreateObstacle(point,m_pObstacleLeft);
			SetObjSelect(m_pObstacleLeft,true);
			SetObjTrace(m_pObstacleLeft,m_bTrace);
		}
		else
		{
			if (m_eGeometry == OBSTACLE_GEOMETRY_POINT)
			{
				m_nPoint = 1;
				GetGround()->ScreenToGround(point,m_aPoint[0]);
			}
			else
			{
				m_nPoint = 2;
				GetGround()->ScreenToGround(point,m_aPoint[0]);
				m_aPoint[1] = m_aPoint[0];
			}
			CreateObstacle(point,m_pObstacleLeft);
			SetObjSelect(m_pObstacleLeft,true);
			SetObjTrace(m_pObstacleLeft,m_bTrace);
		}
	}
	return TRUE;
}

BOOL cStatusObstacle::OnRButtonDown(UINT nFlags, CPoint point)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (m_pObstacleLeft != NULL)	
	{
		if (m_nPoint > 2)
		{
			m_nPoint --;
			SetObstacleGeometry(m_pObstacleLeft);
		}
		SetObjSelect(m_pObstacleLeft,false);
		SetObjTrace(m_pObstacleLeft,false);
		RELEASE(m_pObstacleLeft);
		m_nPoint = 0;
		return TRUE;
	}
	if (m_pObstacleRight)
	{
		SetObjSelect(m_pObstacleRight,false);
		SetObjTrace(m_pObstacleRight,false);
		RELEASE(m_pObstacleRight);
	}

	SelectObstacle(point,m_pObstacleRight);
	SetObjSelect(m_pObstacleRight,true);
	SetObjTrace(m_pObstacleRight,m_bTrace);
	if (m_pObstacleRight)
		ClearSelect();

	return TRUE;
}

void cStatusObstacle::OnObstacleProperty(iObj* p)
{
	cAssertValue assertPointNum(m_nPoint,MAX_POINT);
	if (p == NULL)
		return ;
	stObstacleParam param;
	param.ClearMask();
	p->GetParam(param);
	CObstacleDlg dlg;
	dlg.m_nKind = param.eKind;
	dlg.m_nType = param.eObstacle;
	dlg.m_fRadius = param.fRadius;
	if (dlg.DoModal() == IDOK)
	{
		GU_SetObstacleType(p,(ObstacleEnum)dlg.m_nType);
		GU_SetObstacleRadius(p,dlg.m_fRadius);
		GU_SetObjKind(p,(KindEnum)dlg.m_nKind);
	}
}

void cStatusObstacle::GetObstacleFileString(CString& sFile)
{
	CString sWork;
	sWork = "\\游戏资源\\障碍\\";
	if (m_strObstacleCapture.IsEmpty())
		sFile = sWork + "ObstacleQuick.txt";
	if (sFile.Right(4) == ".txt")
		sFile = sWork + m_strObstacleCapture;
	else
		sFile = sWork + m_strObstacleCapture+ ".txt";
	AppGetFullPath(sFile);
}

BOOL cStatusObstacle::Copy()
{
	if (m_pObstacleRight)
	{
		CString strFile = "defaultobstalce";
		CString s = m_strObstacleCapture;
		m_strObstacleCapture = strFile;
		CaptureObstacle(m_pObstacleRight);
		GetObstacleFileString(strFile);
		m_strSelectFile = strFile;
		m_strObstacleCapture = s;
		return TRUE;
	}
	return FALSE;
}
