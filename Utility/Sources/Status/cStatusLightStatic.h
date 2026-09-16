// cStatusLightStatic.h: interface for the cStatusLightStatic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSLIGHTSTATIC_H__E6D8F10A_FCD8_4121_9555_B3EC12497052__INCLUDED_)
#define AFX_CSTATUSLIGHTSTATIC_H__E6D8F10A_FCD8_4121_9555_B3EC12497052__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusLightStatic  : public cStatus  
{
public:
	cStatusLightStatic();
	virtual ~cStatusLightStatic();
	void Clear();
/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
	stLightParam m_param;
	float   m_fHeight;
	CString m_strLightCapture;
	BOOL Create();
	BOOL m_bShowLight;
	BOOL OnSetFocus();
	BOOL OnLostFocus();
/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////
	void OnSelectedSetting(LPCTSTR szFile);
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
	BOOL MoveSelect(CPoint pt);
	BOOL OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnCameraMove(CPoint point);
	
/////////////////////////////////////////////////////////////////////
//light
/////////////////////////////////////////////////////////////////////
	iObj* m_pLightLeft;
	iObj* m_pLightRight;
	void CreateLight(CPoint point,iObj*& pLight);
	void SelectLight(CPoint point,iObj*& pLight);
	
	void AddLightColor(iObj* pLight,cColor cr);
	void SetLightColor(iObj* pLight,cColor cr);
	void AddLightRange(iObj* pLight,float fRange);
	void SetLightRange(iObj* pLight,float fRange);
	void CaptureLight();
/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
	bool m_bTrace;
	void ToggleLightTrace();

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
};

#endif // !defined(AFX_CSTATUSLIGHTSTATIC_H__E6D8F10A_FCD8_4121_9555_B3EC12497052__INCLUDED_)
