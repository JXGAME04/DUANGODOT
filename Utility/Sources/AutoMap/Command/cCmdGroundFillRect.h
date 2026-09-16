// cCmdGroundFillRect.h: interface for the cCmdGroundFillRect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDGROUNDFILLRECT_H__B9C1D4C2_CD3B_480F_A001_52394F65C339__INCLUDED_)
#define AFX_CCMDGROUNDFILLRECT_H__B9C1D4C2_CD3B_480F_A001_52394F65C339__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"
class iGraphics;
class iAutoGround;

class cCmdGroundFillRect : public cCommand  
{
public:
	cCmdGroundFillRect(iCommandMachine* p);
	virtual ~cCmdGroundFillRect();

	stCommand_FillGround& GetParam();

	CString m_strBackRegion;
	CString m_strBackGround;
	void	GetBackFile(CString& str);

	void	ClearRegion(LPCSTR szFile);
	void	ClearGround(LPCSTR szFile);

	eDoType Undo();
	eDoType Do() ;
	eDoType Redo() ;
	
	HRESULT	SetParam(stCommand* pCommand);

	void GetGroundBackFile();
	void GetRegionBackFile();
	void SetGroundBackFile();
	void SetRegionBackFile();
};

#endif // !defined(AFX_CCMDGROUNDFILLRECT_H__B9C1D4C2_CD3B_480F_A001_52394F65C339__INCLUDED_)
