// cStatusDoodadDynamic.cpp: implementation of the cStatusDoodadDynamic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusDoodadDynamic.h"

#include "iItem.h"
#include "objutility.h"
#include "iGraphicsUtility.h"
#include "fileop.h"
#include "iCommand.h"

#include "cStatusDynamic.h"
/*
#include "kengine.h"
#include "NpcSettingDlg.h"
#include "KTabFile.h"
#include "objDlg.h"
*/

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//注意：现在npc的nick不能改变，因为nick是一个指向_data.ini的指针

cStatusDoodadDynamic::cStatusDoodadDynamic()
{
	m_bTrace = TRUE;

	m_pDynamic = new cStatusDynamic;
}

cStatusDoodadDynamic::~cStatusDoodadDynamic()
{
	delete m_pDynamic;
}


BOOL cStatusDoodadDynamic::Create()
{
	cStatus::Create();
	m_pDynamic->Create();
	m_pDynamic->CreateFromFolder("游戏资源\\动态物体");
	
	m_bLinkHor = false;
	m_bLinkVer = false;
	m_bLinkTile = false;

	return TRUE;
}


void cStatusDoodadDynamic::CreatePlane(CPoint point,iObj* &pPlane, int nFrame)
{
	cStatusDoodadSimple::CreatePlane(point,pPlane,nFrame);
	m_pDynamic->AfterCreatePlane(pPlane,m_strSelectFile,m_strCopyDataFile);
}

void cStatusDoodadDynamic::SelectPlane(CPoint point,iObj* & pPlane)
{
	m_stSelect.eType = SelectType_Dynamic;
	cStatusDoodadSimple::SelectPlane(point,pPlane);
}

///////////////////////////////////////////////////////////////////
//virtual event
///////////////////////////////////////////////////////////////////
BOOL cStatusDoodadDynamic::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatusDoodadSimple::OnKeyDown(nChar,nRepCnt,nFlags);
	m_pDynamic->OnKeyDown(nChar,nRepCnt,nFlags);
	switch (nChar)
	{
	case 'P':
		m_pDynamic->ObjPropertyExtra(m_pPlaneRight);
		break;
	}
	return TRUE;
}

BOOL cStatusDoodadDynamic::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pPlaneRight == NULL)
		return FALSE;
	CString strFile;
	if (IsShiftDown())
	{
		if (!m_pDynamic->GetObjIniFile(m_pPlaneRight,strFile))
			return FALSE;
	}
	else
	{
		if (IsAltDown())
		{
			DynamicEnum eDynamic;
			GU_GetObjDynamic(m_pPlaneRight,eDynamic);
			if (eDynamic == DYNAMIC_TRAP)
			{
				m_pDynamic->ChangeTrapScript(m_pPlaneRight);
			}
			return TRUE;
		}
		else
		{
			if (!m_pDynamic->GetObjScriptFile(m_pPlaneRight,strFile))
			return FALSE;
		}
	}
		
	if (!strFile.IsEmpty())
	{
		CFileFind f;
		if (!f.FindFile(strFile))
		{
			CFile file;
			file.Open(strFile,CFile::modeCreate);
		}
		ShellExecute(NULL,"open",(LPCSTR)strFile,NULL,NULL,SW_SHOW);
	}
	return TRUE;
}

BOOL cStatusDoodadDynamic::OnRButtonDown(UINT nFlags, CPoint point)
{
	cStatusDoodadSimple::OnRButtonDown(nFlags, point);
	if (m_pPlaneRight != NULL)
	{
		m_pDynamic->ObjPropertyExtra(m_pPlaneRight);
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//utitly
/////////////////////////////////////////////////////////////////////
BOOL cStatusDoodadDynamic::GetOffsetOfPlane(iObj* pPlane,cPoint& pt)
{
	return m_pDynamic->GetOffsetOfPlane(pPlane,pt);
}

BOOL cStatusDoodadDynamic::GetObjIniFile(iObj* p,CString& sini)
{
	return m_pDynamic->GetObjIniFile(p,sini);
}

void cStatusDoodadDynamic::OnSelectedSetting(LPCTSTR szFile)
{
	m_pDynamic->OnSelectedSetting(szFile,m_stSetting);
}
