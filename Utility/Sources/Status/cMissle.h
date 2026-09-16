// cMissle.h: interface for the cMissle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMISSLE_H__F28E7AC8_F3F7_4CBF_A00D_BD2466834BBE__INCLUDED_)
#define AFX_CMISSLE_H__F28E7AC8_F3F7_4CBF_A00D_BD2466834BBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cActor.h"

class cMissle  : public cActor
{
public:
	cMissle();
	virtual ~cMissle();

	int m_nLife;
	int m_nBlock;
	
	virtual BOOL Run();
};

#endif // !defined(AFX_CMISSLE_H__F28E7AC8_F3F7_4CBF_A00D_BD2466834BBE__INCLUDED_)
