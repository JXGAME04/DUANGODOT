// cBNodeRoot.h: interface for the cBNodeRoot class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CBNODEROOT_H__74949C01_5D3D_4054_8E0E_8D90F9A32543__INCLUDED_)
#define AFX_CBNODEROOT_H__74949C01_5D3D_4054_8E0E_8D90F9A32543__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cBNode.h"

class cBNodeRoot : public cBNode  
{
public:
	cBNodeRoot();
	virtual ~cBNodeRoot();
	virtual VersortEnum GetVersortType();
	virtual void GetSortLine(float& k,float& b);
};

#endif // !defined(AFX_CBNODEROOT_H__74949C01_5D3D_4054_8E0E_8D90F9A32543__INCLUDED_)
