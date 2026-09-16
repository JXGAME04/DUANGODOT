// cMacroCommand.cpp: implementation of the cMacroCommand class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cMacroCommand.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
cMacroCommand::cMacroCommand(iCommandMachine* p) : cCommand(p)
{

}

cMacroCommand::~cMacroCommand()
{
	for_all(m_list,mem_fun<ULONG,iCommand>(&iCommand::Release));
}

eDoType cMacroCommand::Undo() 
{
	for_all(m_list,mem_fun<eDoType,iCommand>(&iCommand::Undo));
	return DO_CANUNDO;
}

eDoType cMacroCommand::Redo() 
{
	for_all(m_list,mem_fun<eDoType,iCommand>(&iCommand::Redo));
	return DO_CANUNDO;
}

void cMacroCommand::AddCommand(iCommand *p)
{
	p->AddRef();
	m_list.push_back(p);
}

