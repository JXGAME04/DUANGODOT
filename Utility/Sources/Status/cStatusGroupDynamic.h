// cStatusGroupDynamic.h: interface for the cStatusGroupDynamic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSGROUPDYNAMIC_H__8E949DB1_39DB_4239_AD1D_178BDA73F2D5__INCLUDED_)
#define AFX_CSTATUSGROUPDYNAMIC_H__8E949DB1_39DB_4239_AD1D_178BDA73F2D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatusGroup.h"

class cStatusDynamic;

class cStatusGroupDynamic  : public cStatusGroup
{
public:
	cStatusGroupDynamic();
	virtual ~cStatusGroupDynamic();

	virtual LPCSTR GetTypeString(){return "group_dynamic";};
	virtual LPCSTR GetGroupFolder(){return "游戏资源\\动态组件";};

	cStatusDynamic* m_pDynamic;

	virtual BOOL Create();
	virtual BOOL SelectDoodad(CPoint point,cDoodad* & pDoodad);
	virtual BOOL SelectDoodad(cPoint point1, cPoint point2, cDoodad* & pDoodad);

//event
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
//utitly
	virtual BOOL GetOffsetOfPlane(iObj* pPlane,cPoint& pt);
	virtual BOOL GetObjIniFile(iObj* p,CString& sini);

	//virtual void AfterCreatePlane(iObj* pPlane,const CString& m_strCopyDataFile);
	virtual void AfterCreatePlane(iObj* pPlane,const CString& m_strSelectFile,const CString& m_strCopyDataFile);

	void OnSelectedSetting(LPCTSTR szFile);
};

#endif // !defined(AFX_CSTATUSGROUPDYNAMIC_H__8E949DB1_39DB_4239_AD1D_178BDA73F2D5__INCLUDED_)
