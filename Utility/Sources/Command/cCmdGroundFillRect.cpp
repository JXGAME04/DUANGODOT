// cCmdGroundFillRect.cpp: implementation of the cCmdGroundFillRect class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdGroundFillRect.h"

#include "iAutomap.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdGroundFillRect::cCmdGroundFillRect(iCommandMachine* p) : cCommand(p)
{
}

cCmdGroundFillRect::~cCmdGroundFillRect()
{
	if (!m_strBackRegion.IsEmpty())
		AppDeleteFolder(m_strBackRegion);
	if (!m_strBackGround.IsEmpty())
		AppDeleteFolder(m_strBackGround);

	RELEASE(GetParam().pAutoGround);
}

stCommand_FillGround& cCmdGroundFillRect::GetParam()
{
	return *((stCommand_FillGround*)GetBuffer());
}

HRESULT	cCmdGroundFillRect::SetParam(stCommand* param)
{
	HRESULT hr = cCommand::SetParam(param);
	ASSERT(GetParam().pAutoGround);
	GetParam().pAutoGround->AddRef();
	return hr;
}

void cCmdGroundFillRect::ClearRegion(LPCSTR szFile)
{
	stCommand_FillGround& m_param = GetParam();
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_FILLREGION;
	param.rcFill = m_param.rect;
	if (szFile == NULL)
		param.tile.szResource[0] = 0;
	else
		strcpy(param.tile.szResource,m_param.strFile);
	GetGround()->SetParam(param);
}

void cCmdGroundFillRect::ClearGround(LPCSTR szFile)
{
	stCommand_FillGround& m_param = GetParam();
	CString sName;
	if (szFile != NULL)
	{
		CString sPath,sExt;
		AppParsePathNameExt(szFile,sPath,sName,sExt);
	}
	iAutoGround* pGround = GetParam().pAutoGround;
	pGround->Load(m_param.strDocument);
	pGround->SetAllGround(m_param.rect,sName);
//	pGround->Save(m_param.strDocument);
}

void cCmdGroundFillRect::GetBackFile(CString& str)
{
	CString sFile = "back\\undo_ground\\bk%05d.dat";
	AppBuildPath(sFile);
	static int i = 0;
	do
	{
		str.Format(sFile,i++);
		CFileFind f;
		if (!f.FindFile(str))
			break;
	}
	while(i);
}

eDoType cCmdGroundFillRect::Undo() 
{
	stCommand_FillGround& m_param = GetParam();
	ASSERT(m_param.rect.Width() == 1 && m_param.rect.Height() == 1);

	if (!m_strBackGround.IsEmpty())
		SetGroundBackFile();
	else
		ClearGround(NULL);
	if (!m_strBackRegion.IsEmpty())
		SetRegionBackFile();
	else
		ClearRegion(NULL);
	return DO_CANUNDO;
}

eDoType cCmdGroundFillRect::Redo() 
{
	stCommand_FillGround& m_param = GetParam();
	ASSERT(m_param.rect.Width() == 1 && m_param.rect.Height() == 1);
	ClearGround(m_param.strFile);
	ClearRegion(m_param.strFile);
	return DO_CANUNDO;
}

eDoType cCmdGroundFillRect::Do() 
{
	stCommand_FillGround& m_param = GetParam();
	//get undo file of the ground and the region!
	if (m_param.rect.Width() == 1 && m_param.rect.Height() == 1)
	{
		GetBackFile(m_strBackRegion);
		GetBackFile(m_strBackGround);
		GetGroundBackFile();
		GetRegionBackFile();
		return Redo();
	}
	
	ClearGround(m_param.strFile);
	ClearRegion(m_param.strFile);
	return DO_BREAKUNDO;
}

void cCmdGroundFillRect::GetGroundBackFile()
{
	stCommand_FillGround& m_param = GetParam();
	iAutoGround* pGround = GetParam().pAutoGround;
	pGround->Load(m_param.strDocument);
	HRESULT hr = pGround->BackFile(m_param.rect.left, m_param.rect.top, m_strBackGround);
	if (FAILED(hr))
		m_strBackGround.Empty();
}

void cCmdGroundFillRect::GetRegionBackFile()
{
	stCommand_FillGround& m_param = GetParam();
	HRESULT hr = GetGround()->BackFile(m_param.rect.left,m_param.rect.top, m_strBackRegion);
	if (FAILED(hr))
		m_strBackRegion.Empty();
}

void cCmdGroundFillRect::SetGroundBackFile()
{
	stCommand_FillGround& m_param = GetParam();
	iAutoGround* pGround = GetParam().pAutoGround;
	pGround->Load(m_param.strDocument);
	HRESULT hr = pGround->RestoreFile(m_param.rect.left, m_param.rect.top, m_strBackGround);
}

void cCmdGroundFillRect::SetRegionBackFile()
{
	stCommand_FillGround& m_param = GetParam();
	HRESULT hr = GetGround()->RestoreFile(m_param.rect.left,m_param.rect.top, m_strBackRegion);
}
