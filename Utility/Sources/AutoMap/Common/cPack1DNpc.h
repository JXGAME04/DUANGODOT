// cPack1DNpc.h: interface for the cPack1DNpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPACK1DNPC_H__02E31275_EE36_4DDD_BA11_605470C4BD2C__INCLUDED_)
#define AFX_CPACK1DNPC_H__02E31275_EE36_4DDD_BA11_605470C4BD2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPack1D.h"

class cPack1DNpc : public cPack1D  
{
public:
	cPack1DNpc();
	virtual ~cPack1DNpc();

	BOOL LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size);
};

#endif // !defined(AFX_CPACK1DNPC_H__02E31275_EE36_4DDD_BA11_605470C4BD2C__INCLUDED_)
