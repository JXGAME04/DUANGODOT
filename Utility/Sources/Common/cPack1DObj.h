// cPack1DObj.h: interface for the cPack1DObj class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPACK1DOBJ_H__5928124E_78A6_4CB7_8AA8_C31E7AEFB682__INCLUDED_)
#define AFX_CPACK1DOBJ_H__5928124E_78A6_4CB7_8AA8_C31E7AEFB682__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPack1D.h"

class cPack1DObj : public cPack1D  
{
public:
	cPack1DObj();
	virtual ~cPack1DObj();

	BOOL LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size);
};

#endif // !defined(AFX_CPACK1DOBJ_H__5928124E_78A6_4CB7_8AA8_C31E7AEFB682__INCLUDED_)
