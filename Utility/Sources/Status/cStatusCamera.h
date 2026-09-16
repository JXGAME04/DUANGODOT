// cStatusCamera.h: interface for the cStatusCamera class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSCAMERA_H__B8C1A8EF_7C17_466B_821A_AA57335863EF__INCLUDED_)
#define AFX_CSTATUSCAMERA_H__B8C1A8EF_7C17_466B_821A_AA57335863EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusCamera  : public cStatus
{
public:
	cStatusCamera();
	virtual ~cStatusCamera();

//////////////////////////////////////////////////////////////////////
//create
	BOOL Create();
//////////////////////////////////////////////////////////////////////
//command
	CString m_strCameraGet;
	
	void CaptureCamera();
	void OnCameraSet();
	BOOL SetCameraTrace(bool bTrace);
	BOOL SetCamera(cPoint ptTo,cPoint ptEye,float fScale);
//////////////////////////////////////////////////////////////////////
//input
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
//////////////////////////////////////////////////////////////////////
//virtual call back
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
	void OnSelectedSetting(LPCTSTR szFile);
};

#endif // !defined(AFX_CSTATUSCAMERA_H__B8C1A8EF_7C17_466B_821A_AA57335863EF__INCLUDED_)
