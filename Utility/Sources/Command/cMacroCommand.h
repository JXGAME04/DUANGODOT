// cMacroCommand.h: interface for the cMacroCommand class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMACROCOMMAND_H__091ACCD8_35C1_4C44_A8B3_82932D552BD6__INCLUDED_)
#define AFX_CMACROCOMMAND_H__091ACCD8_35C1_4C44_A8B3_82932D552BD6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cMacroCommand : public cCommand  
{
public:
	cMacroCommand(iCommandMachine* p);
	virtual ~cMacroCommand();
	
	eDoType Undo() ;
	eDoType Redo() ;
	eDoType Do() {return Redo();};

	iCommandList m_list;
	void AddCommand(iCommand* p);
	int GetCommandNum(){return m_list.size();}
};


#endif // !defined(AFX_CMACROCOMMAND_H__091ACCD8_35C1_4C44_A8B3_82932D552BD6__INCLUDED_)
