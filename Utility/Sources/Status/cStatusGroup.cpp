// cStatusGroup.cpp: implementation of the cStatusGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusGroup.h"
#include "cDoodad.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iItem.h"
#include "iImage.h"
#include "fileop.h"
#include "objutility.h"

#include "cclipboard.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//const CString m_strShadow = "带阴影.txt";
const CString c_strDoodadCapture = "组件文件名称.txt";

cStatusGroup::cStatusGroup()
{
	m_pDoodadLeft = NULL;
	m_pDoodadRight = NULL;
	m_bTrace = false;
	m_bMouseStart = false;

//	m_stSelect.eType = SelectType_Default;
//	m_stSelect.eFilter = SelectFilter_OneByDisplay;
//	m_stSelect.eFilterParam = SelectFilterParam_Default;
//	m_stSelect.eKind = KIND_NORMAL;
//	m_stSelect.eSort = SORT_VER;
}

void cStatusGroup::Clear()
{
//	SetDoodadSelect(m_pDoodadLeft,false);
//	SetDoodadSelect(m_pDoodadRight,false);
	cStatus::Clear();
	RELEASE(m_pDoodadLeft);
	RELEASE(m_pDoodadRight);
}

cStatusGroup::~cStatusGroup()
{
	Clear();
}

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////

BOOL cStatusGroup::Create()
{
	cStatus::Create();


	CString sFolder = GetGroupFolder();
	sFolder += "\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strDoodadCapture,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		m_strDoodadCapture = set.str.c_str() ;	
	}

	return TRUE;
}

BOOL cStatusGroup::OnSetFocus()
{
	GU_SetSelectSize(GetSelect(),0.1f,0.2f);
	GU_ChangeSelectColor(GetSelect(),cColor(0,255,0));
	return TRUE;
}

void cStatusGroup::GetDoodadFileString(CString& sFile)
{
	CString sWork;
	sWork.Format("\\%s\\",GetGroupFolder());
	if (m_strDoodadCapture.IsEmpty())
		m_strDoodadCapture = "GroupQuick.txt";
	m_strDoodadCapture.MakeLower();
//	sFile = sWork + m_strDoodadCapture;
	if (m_strDoodadCapture.Right(4) == ".txt")
		sFile = sWork + m_strDoodadCapture;
	else
		sFile = sWork + m_strDoodadCapture+ ".txt";
	AppGetFullPath(sFile);
}

void cStatusGroup::SaveDoodad(cDoodad* p,LPCSTR szFile)
{
	if (!p)
		return;
	CString strFile = szFile;
	AppBuildPath(strFile);	
	p->SaveAs(strFile);
}

BOOL cStatusGroup::SaveDoodadOfFirstPlane()
{
	//存储一个plane的阻挡信息,存储到plane的模板文件
	//一次存储对所有的plane产生作用.
	if (m_pDoodadRight == NULL)
		return FALSE;
	m_pDoodadRight->SaveFirstPlane();
	return TRUE;
}

BOOL cStatusGroup::SaveDoodadOfFirstDoodad()
{
	//修改组件的信息
	if (m_pDoodadRight == NULL)
		return FALSE;
	m_pDoodadRight->SaveFirstDoodad();
	return TRUE;
}

void cStatusGroup::CaptureDoodad()
{
	if (m_pDoodadRight == NULL)
		return;

	CString strFile;
	GetDoodadFileString(strFile);

	bool bNew = true;
	CFileFind f;
	if (f.FindFile(strFile))
		bNew = false;

	SaveDoodad(m_pDoodadRight,strFile);
	if (bNew)
		SetNewFile(strFile);
}

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////

BOOL cStatusGroup::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	if (GetItemValue(pItem,"grouptype",sValue))
	{
		SetSelectFile(szFile,nFrame);
		return TRUE;
	}
	return FALSE;
}


void cStatusGroup::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strDoodadCapture)
	{
		m_strDoodadCapture = m_stSetting.str.c_str();
	}
}

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////

BOOL cStatusGroup::MoveSelect(CPoint point)
{
	if (m_pDoodadLeft == NULL)
	{
		if (m_bMouseStart)
		{
			//screen select!
			cPoint ptStart;
			cPoint ptEnd;
			ptStart = m_ptMouseStart;
//			GetGround()->ScreenToGround(m_ptMouseStart,ptStart);
			GetGround()->ScreenToGround(point,ptEnd);
			GU_SetSelectType(GetSelect(),SELECT_PARALLELOGRAM);
			GU_SetSelectOffsetEnd(GetSelect(),ptEnd);
			GU_OffsetSelectTo(GetSelect(),ptStart);
		}
		else if (!IsSelected())
		{
			ClearSelect();
			return FALSE;
		}
		else
		{
			MoveiSelect(SELECT_SQUARE,point);
		}
	}
	else 
	{
		ASSERT (m_pDoodadRight == NULL);
		m_pDoodadLeft->OffsetTo(point);
	}
	return TRUE;
}

BOOL cStatusGroup::OnCameraMove(CPoint point)
{
	return MoveSelect(point);
}

BOOL cStatusGroup::OnMouseMove(UINT nFlags, CPoint point)
{
	cStatus::OnMouseMove(nFlags,point);

	MoveSelect(point);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////

void cStatusGroup::SetDoodadMoving(cDoodad* pDoodad, bool bMoving)
{
	if (pDoodad == NULL)
		return;
	pDoodad->SetMoving(bMoving);
}

void cStatusGroup::SetDoodadSelect(cDoodad* pDoodad, bool bSelect)
{
	if (pDoodad == NULL)
		return;
	pDoodad->SetSelect(bSelect);
	SetDoodadMoving(pDoodad,bSelect);
}

void cStatusGroup::SetDoodadTrace(cDoodad* pDoodad, bool bTrace)
{
	if (pDoodad == NULL)
		return;
//	pDoodad->SetTrace(bTrace);
}

void cStatusGroup::ToggleDoodadTrace()
{
	m_bTrace = !m_bTrace;
	SetDoodadTrace(m_pDoodadLeft,m_bTrace);
	SetDoodadTrace(m_pDoodadRight,m_bTrace);
}

/////////////////////////////////////////////////////////////////////
//Doodad
/////////////////////////////////////////////////////////////////////
BOOL cStatusGroup::SelectDoodad(cPoint point1, cPoint point2, cDoodad* & pDoodad)
{
	if (pDoodad == NULL)
		pDoodad = new cDoodad(this);
	m_stSelect.ePoint = SelectPoint_RectByDiamond;
	GroundToTile(point1);
	TileToDiamond(point1);
	GroundToTile(point2);
	TileToDiamond(point2);
	SelectFilterEnum e = m_stSelect.eFilter;
	m_stSelect.eFilter = SelectFilter_All;
	cRect rc;
	MaskRect(rc,point1,point2);
	pDoodad->AddSelect(rc,m_stSelect);
	m_stSelect.eFilter = e;
	if (pDoodad->m_listUnit.size() == 0)
	{
		RELEASE(pDoodad);
	}
	return TRUE;
}

BOOL cStatusGroup::SelectDoodad(CPoint point,cDoodad* & pDoodad)
{
	if (pDoodad == NULL)
		pDoodad = new cDoodad(this);
	m_stSelect.ePoint = SelectPoint_OneByScreen;
	POINT pt = {point.x,point.y};
	pDoodad->AddSelect(pt,m_stSelect);
	if (pDoodad->m_listUnit.size() == 0)
	{
		RELEASE(pDoodad);
	}
	return TRUE;
}

void cStatusGroup::CreateDoodad(CPoint point,cDoodad* & pDoodad)
{
	ASSERT(IsSelected());
	if (m_strSelectFile.IsEmpty())
		return;
	pDoodad = new cDoodad(this);
	if (IsShiftDown())
	{
		iItem* pItem = Common_CreateItem(m_strSelectFile,iItem::FLAG_RELOAD);
		if (pItem == NULL)
			return;
		wrap_ptr<iItem> ptr(pItem);
	}
	pDoodad->Create(point,GetSelectFile(),IsShiftDown());
	ClearSelect();
}

void cStatusGroup::RemoveDoodad(cDoodad* pDoodad)
{
	if (pDoodad == NULL)
		return;
	pDoodad->Delete();
}

void cStatusGroup::OffsetDoodad(cDoodad* pDoodad,cPoint ptOffset)
{
	if (pDoodad == NULL)
		return;
	pDoodad->Offset(ptOffset);
}

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
BOOL cStatusGroup::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);
	
	float off = 0.1f;
	if (IsShiftDown())
	{
		off *= 0.1f;
	}
	else if (IsCtrlDown())
	{
		off *= 10.f;
	}

	float angle = 1.f;
	if (IsShiftDown())
		angle = -angle;

	cDoodad* pDoodad = m_pDoodadLeft;
	if (pDoodad == NULL)
		pDoodad = m_pDoodadRight;

	switch (nChar)
	{
	case VK_DELETE:
	case VK_ESCAPE:
//		SetDoodadSelect(m_pDoodadLeft,false);
//		SetDoodadSelect(m_pDoodadRight,false);
		RemoveDoodad(m_pDoodadLeft);
		RemoveDoodad(m_pDoodadRight);
		RELEASE(m_pDoodadLeft);
		RELEASE(m_pDoodadRight);
		break;
	case VK_SPACE:
		Clear();
		break;
	case 'W':
		OffsetDoodad(m_pDoodadRight,cPoint(0,-off));
		break;
	case 'X':
	case 'S':
		OffsetDoodad(m_pDoodadRight,cPoint(0,off));
		break;
	case 'A':
		OffsetDoodad(m_pDoodadRight,cPoint(-off,0));
		break;
	case 'D':
		OffsetDoodad(m_pDoodadRight,cPoint(off,0));
		break;
	case 'Q':
		OffsetDoodad(m_pDoodadRight,cPoint(-off,-off));
		break;
	case 'E':
		OffsetDoodad(m_pDoodadRight,cPoint(off,-off));
		break;
	case 'Z':
		OffsetDoodad(m_pDoodadRight,cPoint(-off,off));
		break;
	case 'C':
		OffsetDoodad(m_pDoodadRight,cPoint(off,off));
		break;
	case 'G':
		OffsetDoodad(m_pDoodadRight,cPoint(0,0,-off));
		break;
	case 'H':
		OffsetDoodad(m_pDoodadRight,cPoint(0,0,off));
		break;	
	case 'U':
		if (IsCtrlDown())
			//把组件储存到第一个txt文件中去！
			SaveDoodadOfFirstPlane();
		break;
	case 'J':
		if (IsCtrlDown())
			CaptureDoodad();
		break;
	case 'M':
		if (IsCtrlDown())
			//把组件储存到第一个group txt文件中去！
			SaveDoodadOfFirstDoodad();
		break;
/*
	case 'O':
		if (pDoodad == NULL)
			break;
		pDoodad->Save();
		break;
*/
	case 'I':
		OnSelectSetting();
		break;		
	case 'L':
		if (pDoodad)
			pDoodad->AddLayer(IsShiftDown() ? -1 : 1);
		break;
//change center of the Doodad
//change anglexy and anglez of the Doodad
//
	case VK_F8:
		ToggleDoodadTrace();
		break;
	}
	return TRUE;
}

BOOL cStatusGroup::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_pDoodadLeft != NULL)	
		return TRUE;
	if (IsSelected())
		return TRUE;

	if (m_pDoodadRight)
	{
		if (IsCtrlDown())
		{
			SelectDoodad(point,m_pDoodadRight);
//			SetDoodadSelect(m_pDoodadRight,true);
			return TRUE;
		}
		else
		{
//			SetDoodadSelect(m_pDoodadRight,false);
			RELEASE(m_pDoodadRight);
		}
	}

	SelectDoodad(point,m_pDoodadRight);
//	SetDoodadSelect(m_pDoodadRight,true);
	if (m_pDoodadRight)
		ClearSelect();
	return TRUE;
}

BOOL cStatusGroup::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (IsAltDown())
	{
		m_bMouseStart = true;
//		SetDoodadSelect(m_pDoodadLeft,false);
		RELEASE(m_pDoodadLeft);
//		SetDoodadSelect(m_pDoodadRight,false);
		RELEASE(m_pDoodadRight);
		GetGround()->ScreenToGround(point,m_ptMouseStart);
		return TRUE;
	}
	if (m_bMouseStart)
	{
		cPoint ptEnd;
		GetGround()->ScreenToGround(point,ptEnd);
		SelectDoodad(m_ptMouseStart,ptEnd,m_pDoodadRight);
//		SetDoodadSelect(m_pDoodadRight,true);
		m_bMouseStart = false;
		ClearSelect();
		return TRUE;
	}

	if (m_pDoodadLeft)
	{
//		SetDoodadSelect(m_pDoodadLeft,false);
		RELEASE(m_pDoodadLeft);
		ASSERT(m_pDoodadRight == NULL);
		return TRUE;
	}
	if (m_pDoodadRight)
	{
		if (0 && IsCtrlDown())
		{
			//左选的切换到右选
			m_pDoodadLeft = m_pDoodadRight;
			m_pDoodadRight = NULL;
			return TRUE;
		}
		else
		{
//			SetDoodadSelect(m_pDoodadRight,false);
			RELEASE(m_pDoodadRight);
		}
	}

	if (!IsSelected())
	{
		//单选
		if (IsCtrlDown())
		{
			SelectDoodad(point,m_pDoodadLeft);
//			SetDoodadSelect(m_pDoodadLeft,true);
		}
		else
		{
			SelectDoodad(point,m_pDoodadRight);
//			SetDoodadSelect(m_pDoodadRight,true);
		}
	}
	else
	{
		CreateDoodad(point,m_pDoodadLeft);
//		SetDoodadSelect(m_pDoodadLeft,true);
	}

	return TRUE;
}

BOOL cStatusGroup::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pDoodadRight)
	{
		CString s = m_pDoodadRight->GetGroupFile();
		if (s.IsEmpty())
			return FALSE;
		AppGetFullPath(s);
		ShellExecute(NULL,"open",(LPCSTR)s,NULL,NULL,SW_SHOW);
		
		return TRUE;
	}
	return FALSE;
}

#include "selectdlg.h"

void cStatusGroup::OnSelectSetting()
{
	CSelectDlg dlg;
	dlg.SetSelect(m_stSelect);
	if (dlg.DoModal() == IDOK)
		dlg.GetSelect(m_stSelect);
}

BOOL cStatusGroup::Copy()
{
	if (m_pDoodadRight)
	{
		//copy first groupfile into clipboard
		CString sGroup;
		if (m_pDoodadRight->GetFirstGroupFile(sGroup))
		{
			Clipboard_Copy(sGroup,m_pWnd->GetSafeHwnd());		
		}
		
		CString strFile = "defaultgroup";
		CString s = m_strDoodadCapture;
		m_strDoodadCapture = strFile;
		CaptureDoodad();
		GetDoodadFileString(strFile);
		m_strSelectFile = strFile;
		m_strDoodadCapture = s;
		return TRUE;
	}
	return FALSE;
}

