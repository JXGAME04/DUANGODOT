// cStatusCamera.cpp: implementation of the cStatusCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cStatusCamera.h"
#include "iCommon.h"
#include "iItem.h"
#include "iGraphics.h"
#include "iCommand.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strCameraGet = "camera名称.txt";
const CString c_strJumpRegion = "跳转region.txt";
const CString c_strJumpPos = "跳转坐标.txt";

cStatusCamera::cStatusCamera()
{
}

cStatusCamera::~cStatusCamera()
{

}

BOOL cStatusCamera::Create()
{
	cStatus::Create();

	CString sFolder = "游戏资源\\摄像机\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strCameraGet,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		m_strCameraGet = set.str.c_str() ;	
	}

	return TRUE;
}

void cStatusCamera::CaptureCamera()
{
	CString sWork;
	sWork = "\\游戏资源\\摄像机\\";
	if (m_strCameraGet.IsEmpty())
		sWork += "CameraQuick.txt";
	else
		sWork += m_strCameraGet + ".txt";
	AppGetFullPath(sWork);
	AppBuildPath(sWork);

	iItem* pItem = Common_CreateItem(sWork,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	CFileFind f;
	bool bNew = true;
	if (f.FindFile(sWork))
		bNew = false;

	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	SetItemValue(pItem,"icon","camera");
	SetItemValue(pItem,"type","camera");
	SetItemValue(pItem,"cameratype","normal");
	SetItemValue(pItem,"eye",param.ptEye);
	SetItemValue(pItem,"to",param.ptTo);
	SetItemValue(pItem,"scale",param.fScale);
	pItem->Save();

	if (bNew)
		SetNewFile(sWork);
}

BOOL cStatusCamera::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar, nRepCnt, nFlags);
	if (IsCtrlDown())
	{
		switch (nChar)
		{
		case 'J':
			CaptureCamera();
			break;
/*
		case 'M':
			{
				iCamera::stParam param;
				param.mask = iCamera::MASK_DIS | iCamera::MASK_SCALE;
				param.fDis = 30.0f;
				param.fScale = 1.f;
				GetCamera()->SetParam(param);
				param.mask = iCamera::MASK_COMMAND;
				param.eCommand = iCamera::CCOM_UPDATE;
				GetCamera()->SetParam(param);
				break;
			}
*/
		case 'D':
			{
				iCamera::stParam param;
				param.mask = 0;
				GetCamera()->GetParam(param);
				if (IsShiftDown())
					param.fDis -= 10.f;
				else
					param.fDis += 10.f;
				param.mask = iCamera::MASK_DIS;
				GetCamera()->SetParam(param);
				param.mask = iCamera::MASK_COMMAND;
				param.eCommand = iCamera::CCOM_UPDATE;
				GetCamera()->SetParam(param);
				break;
			}
		}
	}
	return S_OK;
}


BOOL cStatusCamera::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	if (GetItemValue(pItem,"cameratype",sValue))
	{
		SetSelectFile(szFile,nFrame);
		OnCameraSet();
		return TRUE;
	}
	return FALSE;
}

void cStatusCamera::OnCameraSet()
{
	iItem* pItem = Common_CreateItem(GetSelectFile(),iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	stCommand_SetCamera cmd;
	cmd.type = COM_CAMERA_SET;
	VERIFY(GetItemValue(pItem,"eye",cmd.ptEye));
	VERIFY(GetItemValue(pItem,"to",cmd.ptTo));
	VERIFY(GetItemValue(pItem,"scale",cmd.fScale));
//	DoCommand(&cmd);
	SetCamera(cmd.ptEye,cmd.ptTo,cmd.fScale);
}

BOOL cStatusCamera::SetCamera(cPoint ptEye,cPoint ptTo,float fScale)
{
	stCommand_SetCamera cmd;
	cmd.type = COM_CAMERA_SET;
	cmd.ptEye = ptEye;
	cmd.ptTo = ptTo;
	cmd.fScale = fScale;
	DoCommand(&cmd);
	return TRUE;
}

BOOL cStatusCamera::OnLButtonDown(UINT nFlags, CPoint point)
{
	return S_OK;
}

void cStatusCamera::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strCameraGet)
	{
		m_strCameraGet = m_stSetting.str.c_str() ;
	}
	else if (sName == c_strJumpRegion)
	{
		cPoint point = cPoint(m_stSetting.pt.x,m_stSetting.pt.y);
		if (point.x >= 0 && point.y >= 0)
		{
			point.x *= c_nWidthRegion;
			point.y *= c_nHeightRegion;
			OffsetCameraTo(point);
		}
	}
	else if (sName == c_strJumpPos)
	{
		cPoint point = cPoint(m_stSetting.pt.x,m_stSetting.pt.y);
		if (point.x >= 0 && point.y >= 0)
		{
			OffsetCameraTo(point);
		}
	}
}

