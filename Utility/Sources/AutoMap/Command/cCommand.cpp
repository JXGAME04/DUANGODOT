// cCommand.cpp: implementation of the cCommand class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCommand.h"
#include "Mmsystem.h"

#pragma comment (lib,"winmm.lib")

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCommand::cCommand(iCommandMachine* p)
{
	m_ulCount = 1;
	m_pMachine = p;
	m_pBuffer = NULL;
}

cCommand::~cCommand()
{
	SAFE_DELETE(m_pBuffer);
}

/**************************************************************************/
//com interface
/**************************************************************************/
ULONG cCommand::AddRef() 
{
	return ++m_ulCount;
}

ULONG cCommand::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

/**************************************************************************/
//graphics utility function
/**************************************************************************/

iGraphics* cCommand::GetGraphics()
{
	return GetMachine()->GetGraphics(); 
}

iGround* cCommand::GetGround()
{
	return GetGraphics()->GetiGround();
}

iScreen* cCommand::GetScreen()
{
	return GetGraphics()->GetiScreen();
}

iCamera* cCommand::GetCamera()
{
	return GetGraphics()->GetiCamera();
}

iObj* cCommand::GetSelect()
{
	return GetGround()->GetiSelect();
}

/**************************************************************************/
//buffer
/**************************************************************************/
stCommand& cCommand::GetParam()
{
	ASSERT(m_pBuffer);
	return *((stCommand*)m_pBuffer);
}

HRESULT	cCommand::SetParam(stCommand* pCommand)
{
	ULONG size = pCommand->size;
	m_pBuffer = new char[size];
	memcpy(m_pBuffer,pCommand,size);
	
	CTime ctime = CTime::GetCurrentTime();

	GetTime() = *(ctime.GetLocalTm());
	GetParam().ms = timeGetTime();

	return 0;
}

tm& cCommand::GetTime()
{
	return GetParam().time;
}

DWORD cCommand::GetMS()
{
	return GetParam().ms;
}

eCommandType cCommand::GetType()
{
	return GetParam().type;
}

ULONG cCommand::GetSize()
{
	return GetParam().size;
}

HRESULT	cCommand::GetMessage(CString& str,int e)
{
	return GetMachine()->FindCommandMessageString(this,e,str);
}
