// cStatusMachine.cpp: implementation of the cStatusMachine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "status.h"

#include "cStatusMachine.h"
#include "cStatusGeneral.h"
#include "cStatusCamera.h"
#include "cStatusRegion.h"
#include "cStatusLightStatic.h"
#include "cStatusTerrainSquare.h"
#include "cStatusTerrainDiamond.h"
#include "cStatusTerrainRoad.h"
#include "cStatusDoodadSimple.h"
#include "cStatusGroup.h"
#include "cStatusObstacle.h"
#include "cStatusTest.h"
//#include "cStatusGame.h"
#include "cStatusSpecial.h"
#include "cStatusDoodadDynamic.h"
#include "cStatusGroupDynamic.h"

#include "iGraphics.h"
#include "iCommand.h"
#include "fileop.h"

#include "Mmsystem.h"

#pragma comment (lib,"winmm.lib")

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cStatusMachine::cStatusMachine() 
{
	m_pStatus = NULL;
}

cStatusMachine::~cStatusMachine()
{
	RELEASE(m_pStatus);
	RELEASE(m_pCommandMachine);
	RELEASE(m_pGraphics);
}

iCommandMachine* cStatusMachine::CreateCommandMachine(iGraphics* p)
{
	iCommandMachine* pCommandMachine;
	CString strAppName;
	AppGetExeName(strAppName);
	pCommandMachine = Command_CreateCommandMachine(strAppName,p);
	ASSERT(pCommandMachine);
	CString strWorkingFolder;
	AppGetWorkingFolder(strWorkingFolder);
	pCommandMachine->Start(strWorkingFolder);
	return pCommandMachine;
}

cStatus* cStatusMachine::CreateStatus(eStatusType e)
{
	cStatus* p = NULL;
	switch (e)
	{
	case STATUS_REGION:
		p = new cStatusRegion;
		break;
	case STATUS_LIGHT_STATIC:
		p = new cStatusLightStatic;
		break;
	case STATUS_GENERAL:
		p = new cStatusGeneral;
		break;
	case STATUS_DOODAD_SIMPLE:
		p = new cStatusDoodadSimple;
		break;
	case STATUS_GROUP:
		p = new cStatusGroup;
		break;
	case STATUS_TERRAIN_SQUARE:
		p = new cStatusTerrainSquare;
		break;
	case STATUS_TERRAIN_DIAMOND:
		p = new cStatusTerrainDiamond;
		break;
	case STATUS_TERRAIN_ROAD:
		p = new cStatusTerrainRoad;
		break;
	case STATUS_CAMERA:
		p = new cStatusCamera;
		break;
	case STATUS_OBSTACLE:
		p = new cStatusObstacle;
		break;
	case STATUS_GAME:
//		p = new cStatusGame;
		break;
	case STATUS_TEST:
		p = new cStatusTest;
		break;
	case STATUS_DOODAD_DYNAMIC:
		p = new cStatusDoodadDynamic;
		break;
	case STATUS_GROUP_DYNAMIC:
		p = new cStatusGroupDynamic;
		break;
	case STATUS_SPECIAL:
		p = new cStatusSpecial;
		break;
	}
	p->Create();
	return p;
}

BOOL cStatusMachine::Create(iGraphics* p,CWnd* pWnd,OutputMsgFunc func)
{
	AppOutputMsg = func;
	iCommandMachine* pCommandMachine = CreateCommandMachine(p);
	BOOL b = cStatus::Create(p,pWnd,pCommandMachine);
	pCommandMachine->Release();
	
	ChangeToStatus(STATUS_GENERAL);

	return b;
}

/////////////////////////////////////////////////////////////////////////////
//input
BOOL cStatusMachine::OnNewDocument(LPCTSTR lpszPathName)
{
	if (GetcStatus() == NULL)
		return cStatus::OnNewDocument(lpszPathName);
	return GetcStatus()->OnNewDocument(lpszPathName);
}

BOOL cStatusMachine::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (GetcStatus() == NULL)
		return cStatus::OnOpenDocument(lpszPathName);
	return GetcStatus()->OnOpenDocument(lpszPathName);
}

BOOL cStatusMachine::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (GetcStatus() == NULL)
		return cStatus::OnSaveDocument(lpszPathName);
	return GetcStatus()->OnSaveDocument(lpszPathName);
}

BOOL cStatusMachine::OnClose()
{
	if (GetcStatus() == NULL)
		return cStatus::OnClose();
	return GetcStatus()->OnClose();
}

BOOL cStatusMachine::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (ClickOnMap(point))
		return TRUE;
	if (GetcStatus() == NULL)
		return cStatus::OnLButtonDown(nFlags,point);
	return GetcStatus()->OnLButtonDown(nFlags,point);
}

BOOL cStatusMachine::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetcStatus() == NULL)
		return cStatus::OnLButtonUp(nFlags,point);
	return GetcStatus()->OnLButtonUp(nFlags,point);
}

BOOL cStatusMachine::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (GetcStatus() == NULL)
		return cStatus::OnLButtonDblClk(nFlags,point);
	return GetcStatus()->OnLButtonDblClk(nFlags,point);
}

BOOL cStatusMachine::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (GetcStatus() == NULL)
		return cStatus::OnRButtonDown(nFlags,point);
	return GetcStatus()->OnRButtonDown(nFlags,point);
}

BOOL cStatusMachine::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (GetcStatus() == NULL)
		return cStatus::OnRButtonUp(nFlags,point);
	return GetcStatus()->OnRButtonUp(nFlags,point);
}

BOOL cStatusMachine::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (GetcStatus() == NULL)
		return cStatus::OnRButtonDblClk(nFlags,point);
	return GetcStatus()->OnRButtonDblClk(nFlags,point);
}

BOOL cStatusMachine::OnTimer(UINT nIDEvent)
{
	static CPoint s_ptMouse;
	if (GetcStatus() == NULL)
	{
		return cStatus::OnTimer(nIDEvent);
	}
	CPoint ptMouse;
	if (GetMousePos(ptMouse))
	{
		if (s_ptMouse != ptMouse)
		{
			s_ptMouse = ptMouse;
			GetcStatus()->OnMouseMove(0,ptMouse);
		}
	}
	return GetcStatus()->OnTimer(nIDEvent);
}

BOOL cStatusMachine::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (GetcStatus() == NULL)
		return cStatus::OnSetCursor(pWnd,nHitTest,message);
	return GetcStatus()->OnSetCursor(pWnd,nHitTest,message);
}

BOOL cStatusMachine::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	eStatusType e = STATUS_NULL;
	if (!IsCtrlDown() && !IsAltDown())
	{
		switch (nChar)
		{
		case VK_F2:
//			e = STATUS_GENERAL;
			break;
		case VK_F3:
			e = STATUS_REGION;
			break;
		case VK_F4:
			e = STATUS_SPECIAL;
			break;
		case VK_F5:
			e = STATUS_LIGHT_STATIC;
			break;
		case VK_F6:
			e = STATUS_DOODAD_SIMPLE;
			break;
		case VK_F7:
			e = STATUS_OBSTACLE;
			break;
		case VK_F8:
			e = STATUS_GROUP;
			break;
		case VK_F9:
			e = STATUS_TEST;
			break;
		case VK_F11:
			e = STATUS_DOODAD_DYNAMIC;
			break;
		case VK_F12:
			e = STATUS_GROUP_DYNAMIC;
			break;
		}
	}
	else
	{
		switch (nChar)
		{
		case VK_F11:
			if (IsCtrlDown())
				e = STATUS_GROUP_DYNAMIC;
			break;
		case VK_F9:
			if (IsCtrlDown())
				e = STATUS_GAME;
		}
	}

	if (e != STATUS_NULL)
	{
		BOOL b = ChangeToStatus(e);
		if (b == TRUE)
			return TRUE;
	}

	if (GetcStatus() == NULL)
	{
		return cStatus::OnKeyDown(nChar, nRepCnt, nFlags);
	}
	return GetcStatus()->OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL cStatusMachine::OnMouseMove(UINT nFlags, CPoint point)
{
	return FALSE;

	static DWORD time = 0;
	DWORD t = timeGetTime();
	if (t - time < 40)
		return FALSE;
	time = t;

	if (GetcStatus() == NULL)
		return cStatus::OnMouseMove(nFlags,point);
	return GetcStatus()->OnMouseMove(nFlags,point);
}

BOOL cStatusMachine::Copy()
{
	if (GetcStatus() == NULL)
		return cStatus::Copy();
	return GetcStatus()->Copy();
}

BOOL cStatusMachine::Cut()
{
	if (GetcStatus() == NULL)
		return cStatus::Cut();
	return GetcStatus()->Cut();
}

BOOL cStatusMachine::Paste()
{
	if (GetcStatus() == NULL)
		return cStatus::Paste();
	return GetcStatus()->Paste();
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//window operator
BOOL cStatusMachine::OnRightSelectedFile(LPCTSTR szFile, int nFrame)
{
	if (!OnSelectedType(szFile))
	{
		//无法确定该文件
		return cStatus::OnRightSelectedFile(szFile,nFrame);
	}
	if (!GetcStatus())
		return FALSE;
	return GetcStatus()->OnRightSelectedFile(szFile,nFrame);
}

BOOL cStatusMachine::OnLeftSelectedFile(LPCTSTR szFile, int nFrame)
{
	if (!OnSelectedType(szFile))
	{
		//无法确定该文件
		return cStatus::OnLeftSelectedFile(szFile,nFrame);
	}
	if (!GetcStatus())
		return FALSE;
	return GetcStatus()->OnLeftSelectedFile(szFile,nFrame);
}

BOOL cStatusMachine::ChangeToStatus(eStatusType nType)
{
	if (nType == STATUS_NULL)
		return FALSE;
	if (GetStatus() == nType)
	{
		//继续以前的状态
		return FALSE;
	}
	else
	{
		//设置新的状态
		if (m_pStatus)
			m_pStatus->OnLostFocus();
		RELEASE(m_pStatus);
		m_strSelectFile.Empty();
		m_pStatus = CreateStatus(nType);
		m_pStatus->OnSetFocus();
		SetStatus(nType);
		return TRUE;
	}

}

BOOL cStatusMachine::OnSelectedType(LPCTSTR szFile)
{
	eStatusType nType = GetFileType(szFile);
	if (nType == STATUS_NULL)
		return FALSE;
	ChangeToStatus(nType);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

_declspec(dllexport) iStatusMachine* Status_CreateStatusMachine()
{
	return new cStatusMachine;
}
