// cCommandMachine.cpp: implementation of the cCommandMachine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCommandMachine.h"
#include "cMacroCommand.h"

#include "cCmdGroundFillRect.h"
#include "cCmdGroundChangeTile.h"
#include "cCmdGroundTogglegrid.h"
#include "cCmdGroundChangeBrightness.h"

#include "cCmdCameraOffset.h"
#include "cCmdCameraOffsetTo.h"
#include "cCmdCameraScale.h"
#include "cCmdCameraSet.h"

#include "cCmdObjAdd.h"
#include "cCmdObjRemove.h"
//#include "cCmdObjOffset.h"
#include "cCmdObjSetParam.h"

#include "cCmdNewDocument.h"
#include "cCmdOpenDocument.h"
#include "cCmdSaveDocument.h"
#include "cCmdGroundDynamic.h"
#include "iGraphics.h"

#include "iItem.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCommandMachine::cCommandMachine(LPCSTR szAppName, iGraphics* p)
{
	m_ulCount = 1;
	m_strAppName = szAppName;
	p->AddRef();
	m_pGraphics = p;
	m_itCommand = m_listCommand.end();

	for (int i=0; i<COMMAND_NUN; i++)
	for (int j=0; j<OUTPUT_NUM; j++)
		m_aCommandMessage[i].aMessageNum[j] = 0;
	m_pMacroCommand = NULL;
}

cCommandMachine::~cCommandMachine()
{
	RELEASE(m_pGraphics);
	ClearUndoCommand();
	for_all(m_listCommandAll,mem_fun<ULONG,iCommand>(&iCommand::Release));
	m_listCommandAll.clear();
	RELEASE(m_pMacroCommand);
}

//////////////////////////////////////////////////////////////////////
//com

ULONG cCommandMachine::AddRef() 
{
	return ++m_ulCount;
}

ULONG cCommandMachine::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}


//////////////////////////////////////////////////////////////////////
//the command's knowledge

BOOL cCommandMachine::FindCommandMessageString(iCommand* pCom,int e, CString& strMessage)
{
	eCommandType com = pCom->GetType();
	if (m_aCommandMessage[com].aMessageNum[e] == 0)
		return FALSE;
	int n = rand() % m_aCommandMessage[com].aMessageNum[e];
	strMessage = m_aCommandMessage[com].aMessageString[e][n];
	return TRUE;
}

HRESULT cCommandMachine::LoadCommandMessage()
{
	return 0;
	
	CString strFile = m_strWorkingFoler + "\\system\\ini\\AppCommandMessage.ini";
	CStdioFile file;
	if (!file.Open(strFile,CFile::modeRead))
		return -1;
	CString s;

	int nItem = 0;
//	int nSubItem = 0;
	int nName = 0;

	int type = -1;

	stCommandMessage msg;
	while(file.ReadString(s))
	{
		if (s.IsEmpty())
			continue;
		s.MakeLower();
		int pos = s.Find("[name]");
		if ( pos != -1)
		{
			if (type != -1)
			{
				m_aCommandMessage[type] = msg;
			}
			s = s.Right(s.GetLength() - pos - 6);
			s.TrimLeft();
			s.TrimRight();
			type = CommandNameToID(s);
			msg.strCommand = s;
			nName++;

			nItem = 0;
			for (int i=0; i<OUTPUT_NUM; i++)
			{
				msg.aMessageNum[i] = 0;
			}
			continue;
		}
		pos = s.Find("[item]");
		if ((pos) != -1)
		{
			CString str = s.Mid(pos+6);
			str.TrimLeft();
			str.TrimRight();
			nItem = OutputNameToID(str);
			ASSERT(nItem != OUTPUT_NULL);
			continue;
		}
		msg.aMessageString[nItem][msg.aMessageNum[nItem]++] = s;
	}

	if (type != -1)
		m_aCommandMessage[type] = msg;

	return 0;
}

//////////////////////////////////////////////////////////////////////
//start
HRESULT cCommandMachine::Start(LPCSTR szWorkingFolder)
{
	m_strWorkingFoler = szWorkingFolder;
	LoadCommandMessage();
	return 0;
}

HRESULT cCommandMachine::End()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
//command
HRESULT cCommandMachine::Command(stCommand* pStCmd)
{
	iCommand* p = CreateCommand(pStCmd);
	HRESULT hr = Command(p);
	p->Release();
	return hr;
}

HRESULT	cCommandMachine::Command(iCommand* p)
{
	eDoType rt = p->Do();
	return AddCommand(p,rt);
}

HRESULT cCommandMachine::AddCommand(iCommand* pCommand,eDoType eDo)
{
	if (eDo == DO_NOTHING || eDo == DO_FAILED)
		return 0;
	pCommand->AddRef();
	m_listCommandAll.push_back(pCommand);
	switch (eDo)
	{
	case DO_CANUNDO:
	case DO_CONTINUE:
		AddUndoCommand(pCommand);
		break;
	case DO_BREAKUNDO:
		ClearUndoCommand();
		break;
	}
	return 0;
}

HRESULT	cCommandMachine::BreakUndo()
{
	ClearUndoCommand();
	return 0;
}

//////////////////////////////////////////////////////////////////////
//redo and undo
HRESULT cCommandMachine::UndoOnce()
{
	if (!CanUndo())
		return FALSE;
	(*--m_itCommand)->Undo();
	return TRUE;
}

HRESULT cCommandMachine::Undo()
{
	if (!CanUndo())
		return FALSE;
	while ( CanUndo() && (*--m_itCommand)->Undo() == DO_CONTINUE );
	return TRUE;
}

HRESULT cCommandMachine::RedoOnce()
{
	if (!CanRedo())
		return FALSE;
	(*m_itCommand++)->Redo();
	return TRUE;
}

HRESULT cCommandMachine::Redo()
{
	if (!CanRedo())
		return FALSE;
	while ( CanRedo() && (*m_itCommand++)->Redo() == DO_CONTINUE );
	return TRUE;
}

void cCommandMachine::AddUndoCommand(iCommand* pCmd)
{
	if (m_pMacroCommand)
	{
		m_pMacroCommand->AddCommand(pCmd);
		return;
	}
	RemoveRedoCommand();
	if (!m_listCommand.empty())
	{
		iCommand* p = m_listCommand.back();
		if (p->GetType() == pCmd->GetType())
		{
			if (p->Merge(pCmd))
				return;
		}
	}
	pCmd->AddRef();
	m_listCommand.push_back(pCmd);
	m_itCommand = m_listCommand.end();
}

void cCommandMachine::RemoveRedoCommand()
{
	for_each(m_itCommand, m_listCommand.end(), mem_fun<ULONG,iCommand>(&iCommand::Release));
	m_listCommand.erase(m_itCommand,m_listCommand.end());
	m_itCommand = m_listCommand.end();
}

void cCommandMachine::ClearUndoCommand()
{
	for_all(m_listCommand, mem_fun<ULONG,iCommand>(&iCommand::Release));
	m_listCommand.clear();
	m_itCommand = m_listCommand.end();
}

BOOL cCommandMachine::CanRedo()
{
	return m_listCommand.size() > 0 && m_itCommand != m_listCommand.end();
}

BOOL cCommandMachine::CanUndo()
{
	return m_listCommand.size() > 0 && m_itCommand != m_listCommand.begin();
}

//////////////////////////////////////////////////////////////////////
//pair
void cCommandMachine::CommandSleep(cCommand* p,cCommand* pPre,DWORD& ms)
{
	return ;
	tm& t = p->GetTime();
	tm& tPre = pPre->GetTime();
	if (t.tm_year == tPre.tm_year && t.tm_yday == tPre.tm_yday
		&& t.tm_hour == tPre.tm_hour && t.tm_min == tPre.tm_min)
		//&& t.tm_sec == t.tm_sec)
	{
		DWORD ms1 = p->GetMS();
		DWORD ms2 = pPre->GetMS();
		ASSERT( abs(ms1 - ms2) < 60 * 1000 ); 
		if (abs(ms1 - ms2) < 300)
		{
			Sleep(abs(ms1 - ms2));
			return;
			ms += abs(ms1 - ms2);
			if (ms < 300)
				return;				
			else
				ms = 0;
		}
	}
	Sleep(300);
}

cCommand* cCommandMachine::GetUndoCommand()
{
	itiCommand it = m_itCommand;
	it --;
	cCommand* p = dynamic_cast<cCommand*>(*it);
	ASSERT(p);
	return p;
}

HRESULT	cCommandMachine::UndoToBegin()
{
	cCommand* pPre = NULL;
	DWORD ms = 0;
	while(CanUndo())
	{
		cCommand* p = GetUndoCommand();
		if (pPre != NULL)
		{
			CommandSleep(p,pPre,ms);
		}
		pPre = p;
		UndoOnce();
		GetGraphics()->GraphicsRun();
	}
	return S_OK;
}

cCommand* cCommandMachine::GetRedoCommand()
{
	itiCommand it = m_itCommand;
	cCommand* p = dynamic_cast<cCommand*>(*it);
	ASSERT(p);
	return p;
}

HRESULT	cCommandMachine::RedoToEnd()
{
	cCommand* pPre = NULL;
	DWORD ms = 0;
	while(CanRedo())
	{
		cCommand* p = GetRedoCommand();
		if (pPre != NULL)
		{
			CommandSleep(p,pPre,ms);
		}
		pPre = p;
		RedoOnce();
		GetGraphics()->GraphicsRun();
	}
	return S_OK;
}

HRESULT	cCommandMachine::Replay()
{
	return RedoToEnd();
}

//////////////////////////////////////////////////////////////////////
//macro
//////////////////////////////////////////////////////////////////////

void cCommandMachine::BeginMacroCommand()
{
	RELEASE(m_pMacroCommand);
	stCommand st;
	st.size = sizeof(st);
	st.type = COM_MACRO;
	m_pMacroCommand = (cMacroCommand*)CreateCommand(&st);
}

void cCommandMachine::EndMacroCommand() 
{
	if (m_pMacroCommand == NULL)
		return ;
	if (m_pMacroCommand->GetCommandNum() == 0)
	{
		RELEASE(m_pMacroCommand);
		return;
	}
	cMacroCommand* p = m_pMacroCommand;
	m_pMacroCommand = NULL;
	AddCommand(p,DO_CANUNDO);
	p->Release();
}

//////////////////////////////////////////////////////////////////////
//factory of command 
iCommand* cCommandMachine::CreateCommand(stCommand* pCommand)
{
//	stCommand* pCommand = (stCommand*)pBuffer;
	iCommand* pCom = NULL;
	switch (pCommand->type)
	{
	case COM_MACRO:
		{
			pCom = new cMacroCommand(this);
			break;
		}
//Document
	case COM_FILE_NEWDOCUMENT:
		{
			pCom = new cCmdNewDocument(this);
			break;
		}
	case COM_FILE_OPENDOCUMENT:
		{
			pCom = new cCmdOpenDocument(this);
			break;
		}
	case COM_FILE_SAVEDOCUMENT:
		{
			pCom = new cCmdSaveDocument(this);
			break;
		}
	case COM_FILE_DYNAMIC:
		{
			pCom = new cCmdGroundDynamic(this);
			break;
		}
//Ground
	case COM_GROUND_FILLRECT:
		{
			pCom = new cCmdGroundFillRect(this);
			break;
		}
	case COM_GROUND_CHANGETILE:
		{
			pCom = new cCmdGroundChangeTile(this);
			break;
		}
	case COM_GROUND_TOGGLEGRID:
		{
			pCom = new cCmdGroundToggleGrid(this);
			break;
		}
//Camera
	case COM_CAMERA_OFFSETTO:
		{
			pCom = new cCmdCameraOffsetTo(this);
			break;
		}
	case COM_CAMERA_OFFSET:
		{
			pCom = new cCmdCameraOffset(this);
			break;
		}
	case COM_CAMERA_SCALE:
		{
			pCom = new cCmdCameraScale(this);
			break;
		}
	case COM_CAMERA_SET:
		{
			pCom = new cCmdCameraSet(this);
			break;
		}
	case COM_GROUND_CHANGEBRIGHTNESS:
		{
			pCom = new cCmdGroundChangeBrightness(this);
			break;
		}
//Obj
	case COM_OBJ_ADD:
		{
			pCom = new cCmdObjAdd(this);
			break;
		}
	case COM_OBJ_REMOVE:
		{
			pCom = new cCmdObjRemove(this);
			break;
		}
/*
	case COM_OBJ_OFFSET:
		{
			pCom = new cCmdObjOffset(this);
			break;
		}
*/
	case COM_OBJ_SETPARAM:
		{
			pCom = new cCmdObjSetParam(this);
			break;
		}
	default:
		ASSERT(0);
	}
	if (pCom)
	{
		pCom->SetParam(pCommand);
		return pCom;
	}
	return NULL;
}

_declspec(dllexport) iCommandMachine* Command_CreateCommandMachine(LPCSTR szAppName, iGraphics* p)
{
	return new cCommandMachine(szAppName,p);
}

_declspec(dllexport) eCommandType CommandNameToID(const CString& s)
{
	static CString s_strCommand[] = 
	{
		"file_newdocument",
		"file_opendocument",
		"file_savedocument",

		"file_loaddynamic",

		"window_togglefullscreen",
		
		"ground_togglegrid",
		"ground_fillrect",
		"ground_changetile",
		"ground_changebrightness",
		"ground_changeregioncolor",

		"status_change",
		"status_selectfile",

		"camera_offset",
		"camera_scale",
		"camera_set",

		"Obj_add",
		"Obj_remove",
		"Obj_offset",
		"Obj_setparam",

		""
	};

	return (eCommandType )NameToID(s_strCommand,s);
}

_declspec(dllexport) int OutputNameToID(const CString& s)
{
	static CString s_strCommandMessageItem[] = 
	{
		"ok",
		"succeed",
		"forbid",
		"warnning",
		"error",
		"hint",
		"help",
		"chat",
		"next",
		"new",
		""
	};
	return NameToID(s_strCommandMessageItem,s);
}
