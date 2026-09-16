// cStatusGroupDynamic.cpp: implementation of the cStatusGroupDynamic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusGroupDynamic.h"

#include "cStatusDynamic.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cStatusGroupDynamic::cStatusGroupDynamic()
{
	m_pDynamic = new cStatusDynamic;
}

cStatusGroupDynamic::~cStatusGroupDynamic()
{
	delete m_pDynamic;
}

//////////////////////////////////////////////////////////////
//copy and create
//////////////////////////////////////////////////////////////
BOOL cStatusGroupDynamic::Create()
{
	cStatusGroup::Create();
	m_pDynamic->Create();

	m_pDynamic->CreateFromFolder("游戏资源\\动态组件");
	//load attribute and settings!

	return TRUE;
}

void cStatusGroupDynamic::AfterCreatePlane(iObj* pPlane,const CString& m_strSelectFile,const CString& m_strCopyDataFile)
{
	m_pDynamic->AfterCreatePlane(pPlane,m_strSelectFile,m_strCopyDataFile);
}

BOOL cStatusGroupDynamic::SelectDoodad(CPoint point,cDoodad* & pDoodad)
{
	m_stSelect.eType = SelectType_Dynamic;
	return cStatusGroup::SelectDoodad(point,pDoodad);
}

BOOL cStatusGroupDynamic::SelectDoodad(cPoint point1, cPoint point2, cDoodad* & pDoodad)
{
	m_stSelect.eType = SelectType_Dynamic;
	return cStatusGroup::SelectDoodad(point1,point2,pDoodad);
}

///////////////////////////////////////////////////////////////////
//virtual event
///////////////////////////////////////////////////////////////////
BOOL cStatusGroupDynamic::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case 'U':
	case 'M':
	case 'L':
		break;
	default:
		cStatusGroup::OnKeyDown(nChar,nRepCnt,nFlags);
	}
	m_pDynamic->OnKeyDown(nChar,nRepCnt,nFlags);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//utitly
/////////////////////////////////////////////////////////////////////
BOOL cStatusGroupDynamic::GetOffsetOfPlane(iObj* pPlane,cPoint& pt)
{
	return m_pDynamic->GetOffsetOfPlane(pPlane,pt);
}

BOOL cStatusGroupDynamic::GetObjIniFile(iObj* p,CString& sini)
{
	return m_pDynamic->GetObjIniFile(p,sini);
}

void cStatusGroupDynamic::OnSelectedSetting(LPCTSTR szFile)
{
	m_pDynamic->OnSelectedSetting(szFile,m_stSetting);	
	cStatusGroup::OnSelectedSetting(szFile);
}
