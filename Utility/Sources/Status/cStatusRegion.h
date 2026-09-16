// cStatusRegion.h: interface for the cStatusRegion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSREGION_H__CD357313_B491_4A05_8B24_60E6BC4C3FCC__INCLUDED_)
#define AFX_CSTATUSREGION_H__CD357313_B491_4A05_8B24_60E6BC4C3FCC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusRegion : public cStatus  
{
public:
	cStatusRegion();
	virtual ~cStatusRegion();
	void Clear();

//////////////////////////////////////////////////////////////////////
//create
	iRegion* m_pRegion;
	BOOL Create();
	BOOL OnSetFocus();

//////////////////////////////////////////////////////////////////////
//command
	int		m_nTimeRunning;
	int		m_nMinBrightness;
	int		m_nMaxBrightness;
//	cColor	m_crRegion; //crRegion should autoget!
	CString m_strRegionCapture;

//	int		m_nCurrentBrightness;

	BOOL GetRegionColor(cColor& cr);
	void AddRegionColor(cColor cr);
	void CaptureRegionColor();
	void SetRegionColor(cColor cr);
	void ToggleRegionDraw();
	void ToggleRegionTransparent();
	void ToggleRegionBlend();
	void DeleteRegion();
	void AddRegionAniSpeed(bool bAdd);
	void AddRegionAniDegree(bool bAdd);
	iRegion* GetCurrentRegion();
	iRegion* GetNowRegion();
	void SelectRegion(iRegion* pRegion, bool bSelect);
//////////////////////////////////////////////////////////////////////
//virtual call back
	void OnSelectedSetting(LPCTSTR szFile);
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);

//////////////////////////////////////////////////////////////////////
//input
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnTimer(UINT nIDEvent);

	void MoveSelect(CPoint point);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnCameraMove(CPoint point);
	
};

#endif // !defined(AFX_CSTATUSREGION_H__CD357313_B491_4A05_8B24_60E6BC4C3FCC__INCLUDED_)
