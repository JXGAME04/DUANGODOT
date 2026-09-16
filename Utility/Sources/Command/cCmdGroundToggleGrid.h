// cCmdGroundToggleGrid.h: interface for the cCmdGroundToggleGrid class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDGROUNDTOGGLEGRID_H__F03C651D_B4DF_4B82_9FCD_C7DF92717DB8__INCLUDED_)
#define AFX_CCMDGROUNDTOGGLEGRID_H__F03C651D_B4DF_4B82_9FCD_C7DF92717DB8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdGroundToggleGrid : public cCommand  
{
public:
	cCmdGroundToggleGrid(iCommandMachine* p);
	virtual ~cCmdGroundToggleGrid();

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;

	long m_eParam;;
	void SetGroundParam(BitOPEnum e, MapeditParamEnum pm);
};

#endif // !defined(AFX_CCMDGROUNDTOGGLEGRID_H__F03C651D_B4DF_4B82_9FCD_C7DF92717DB8__INCLUDED_)
