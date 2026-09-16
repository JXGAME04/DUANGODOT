// cPack1DAnimation.h: interface for the cPack1DAnimation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPACK1DANIMATION_H__2927EFA6_1789_468D_BE6F_99CD826E500B__INCLUDED_)
#define AFX_CPACK1DANIMATION_H__2927EFA6_1789_468D_BE6F_99CD826E500B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPack1D.h"

class cPack1DAnimation : public cPack1D  
{
public:
	cPack1DAnimation();
	virtual ~cPack1DAnimation();

	BOOL LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size);
};

#endif // !defined(AFX_CPACK1DANIMATION_H__2927EFA6_1789_468D_BE6F_99CD826E500B__INCLUDED_)
