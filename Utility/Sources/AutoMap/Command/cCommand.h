// cCommand.h: interface for the cCommand class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCOMMAND_H__0C3D75E9_90E6_4FDB_B09E_7A054019D97D__INCLUDED_)
#define AFX_CCOMMAND_H__0C3D75E9_90E6_4FDB_B09E_7A054019D97D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class iGraphics;
class iGround;
class iScreen;
class iCamera;
class iObj;

#include "iCommand.h"
#include "iGraphics.h"


class cCommand  : public iCommand
{
public:
	cCommand(iCommandMachine* p);
	virtual ~cCommand();
public:
//com interface
	ULONG m_ulCount;
	ULONG AddRef() ;
	ULONG Release() ;

	iCommandMachine*	m_pMachine;
	iCommandMachine*	GetMachine()	{return m_pMachine;}
	iGraphics*			GetGraphics();
	iGround*			GetGround();
	iScreen*			GetScreen();
	iCamera*			GetCamera();
	iObj*				GetSelect();
	
//	virtual BOOL		CanUndo()		{return FALSE;};
//	virtual BOOL		BreakUndo()		{return FALSE;};
	virtual eDoType		Undo()			{return DO_CANUNDO;};
	virtual eDoType		Redo()			{return Do();};
	
	virtual BOOL		Merge(iCommand* p) {return FALSE;};

	LPVOID				m_pBuffer;
	LPVOID				GetBuffer()		{return m_pBuffer;}
	eCommandType		GetType();
	ULONG				GetSize();
	tm&					GetTime();
	stCommand&			GetParam();
	DWORD				GetMS();

	virtual HRESULT		SetParam(stCommand* pCommand);
	virtual HRESULT		GetMessage(CString& str,int e);
};



#endif // !defined(AFX_CCOMMAND_H__0C3D75E9_90E6_4FDB_B09E_7A054019D97D__INCLUDED_)
