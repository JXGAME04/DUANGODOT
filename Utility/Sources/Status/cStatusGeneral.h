// cStatusGeneral.h: interface for the cStatusGeneral class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSGENERAL_H__EFE6F093_49C3_4A4C_900F_702C367FFFEC__INCLUDED_)
#define AFX_CSTATUSGENERAL_H__EFE6F093_49C3_4A4C_900F_702C367FFFEC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cStatusGeneral : public cStatus  
{
public:
	cStatusGeneral();
	virtual ~cStatusGeneral();

	enum	{MAX_SHOW = 32,MAX_TRACE = 32};
	int		m_nShow;
	BOOL	m_aShow[MAX_SHOW];
	int		m_nTrace;
	BOOL	m_aTrace[MAX_TRACE];
	virtual	void	OnSelectedSetting(LPCTSTR szFile);
	void	SetGraphicsShowParam();
	void	SetGraphicsTraceParam();
	BOOL	Create();
};

#endif // !defined(AFX_CSTATUSGENERAL_H__EFE6F093_49C3_4A4C_900F_702C367FFFEC__INCLUDED_)
