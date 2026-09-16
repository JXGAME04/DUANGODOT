// cPack1DPlane.h: interface for the cPack1DPlane class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPACK1DPLANE_H__ED339BCC_D5EE_4561_863B_57257292A05D__INCLUDED_)
#define AFX_CPACK1DPLANE_H__ED339BCC_D5EE_4561_863B_57257292A05D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPack1D.h"

class cPack1DPlane : public cPack1D  
{
public:
	cPack1DPlane();
	virtual ~cPack1DPlane();

	void* Locate(void* pStart,int frame);
	BOOL LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size);
};

#endif // !defined(AFX_CPACK1DPLANE_H__ED339BCC_D5EE_4561_863B_57257292A05D__INCLUDED_)
