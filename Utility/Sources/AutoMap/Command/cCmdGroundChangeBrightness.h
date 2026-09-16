// cCmdGroundChangeBrightness.h: interface for the cCmdGroundChangeBrightness class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDGROUNDCHANGEBRIGHTNESS_H__225D323C_F131_403C_BBD4_4C51389F4E60__INCLUDED_)
#define AFX_CCMDGROUNDCHANGEBRIGHTNESS_H__225D323C_F131_403C_BBD4_4C51389F4E60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdGroundChangeBrightness : public cCommand  
{
public:
	cCmdGroundChangeBrightness(iCommandMachine* p);
	virtual ~cCmdGroundChangeBrightness();

	stCommand_ChangeGroundBrightness& GetParam()
	{return  *( (stCommand_ChangeGroundBrightness*) GetBuffer() );};

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;

	cColor m_crColor;
	void SetGroundBrightness(cColor crColor);
	BOOL Merge(iCommand* p);
};

#endif // !defined(AFX_CCMDGROUNDCHANGEBRIGHTNESS_H__225D323C_F131_403C_BBD4_4C51389F4E60__INCLUDED_)
