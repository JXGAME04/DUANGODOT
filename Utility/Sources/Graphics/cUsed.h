// cUsed.h: interface for the cUsed class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUSED_H__ACD04FD0_A6BC_4460_8941_4DC91C1C56C5__INCLUDED_)
#define AFX_CUSED_H__ACD04FD0_A6BC_4460_8941_4DC91C1C56C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//enum eUsedType{USED_OFTEN, USED_NORMAL, USED_SOLDEM, USED_NEVER,};
#include "iGraphics.h"

class cUsed  : public iUsed
{
public:
	cUsed(cGraphics* p);
	virtual ~cUsed();
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}

	ULONG GetFrames();
	ULONG m_ulBorn;
	ULONG m_ulBack;
	ULONG m_ulFront;
	int	  m_nUsed;
//	list<ULONG> m_list;
	void Used() ;
	UsedEnum GetUsed() ;
	void Reset();

	CString m_strName;
	void SetName(LPCSTR str){m_strName = str;};
};

#endif // !defined(AFX_CUSED_H__ACD04FD0_A6BC_4460_8941_4DC91C1C56C5__INCLUDED_)
