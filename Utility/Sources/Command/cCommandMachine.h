// cCommandMachine.h: interface for the cCommandMachine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCOMMANDMACHINE_H__518CD46D_8795_435B_AB7D_C4F6009842DF__INCLUDED_)
#define AFX_CCOMMANDMACHINE_H__518CD46D_8795_435B_AB7D_C4F6009842DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iCommand.h"
#include "iItem.h"

class cCommand;
class cMacroCommand;
enum {MAX_MESSAGE = 32};

struct stCommandMessage
{
	int		eCommand;
	CString strCommand;
	int		aMessageNum[OUTPUT_NUM];
	CString	aMessageString[OUTPUT_NUM][MAX_MESSAGE];
};

typedef std::list<stCommandMessage> CommandMessageList;
typedef CommandMessageList::iterator itCommandMessage;

class iGraphics;

class cCommandMachine  : public iCommandMachine
{
public:
	cCommandMachine(LPCSTR szAppName, iGraphics* p);
	virtual ~cCommandMachine();

//com
	ULONG		m_ulCount;
	ULONG		AddRef() ;
	ULONG		Release() ;

	CString		m_strAppName;
	CString		m_strWorkingFoler;
	HRESULT		Start(LPCSTR szWorkingFolder) ;
	HRESULT		End() ;
	iGraphics*	m_pGraphics;
	iGraphics*	GetGraphics(){return m_pGraphics;};

//interface
	iCommand*	CreateCommand(stCommand* pCmd) ;
	HRESULT		Command(iCommand* p);
	HRESULT 	Command(stCommand* pCmd);

//all command
	iCommandList m_listCommandAll;			//use to pharse the input
	HRESULT		AddCommand(iCommand* pCommand,eDoType eDo) ;

//pair
	void		CommandSleep(cCommand* p,cCommand* pPre,DWORD& ms);
	cCommand*	GetUndoCommand();
	cCommand*	GetRedoCommand();
	HRESULT		UndoToBegin();
	HRESULT		RedoToEnd();
	HRESULT		Replay();
	HRESULT		BreakUndo();

//redo and undo
	iCommandList m_listCommand;				//use for redo and undo
	itiCommand	m_itCommand;
	void		AddUndoCommand(iCommand* pCommand) ;
	void		RemoveRedoCommand();
	void		ClearUndoCommand();
	HRESULT		Undo();
	HRESULT		Redo();
	HRESULT		UndoOnce();
	HRESULT		RedoOnce();
	BOOL		CanRedo();
	BOOL		CanUndo();

//macro
	void		BeginMacroCommand() ;
	void		EndMacroCommand() ;
	cMacroCommand* m_pMacroCommand;
	
//the command's knowledge
	stCommandMessage m_aCommandMessage[COMMAND_NUN];
	BOOL	FindCommandMessageString(iCommand* pCom,int  e, CString& strMessage);
	HRESULT LoadCommandMessage();
};

#endif // !defined(AFX_CCOMMANDMACHINE_H__518CD46D_8795_435B_AB7D_C4F6009842DF__INCLUDED_)
