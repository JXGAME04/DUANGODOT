// cUsed.cpp: implementation of the cUsed class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cUsed.h"

#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cUsed::cUsed(cGraphics* p)
{
	m_pGraphics = p;
	m_ulBack = GetFrames();
	m_nUsed = 0;
}

cUsed::~cUsed()
{

}

ULONG cUsed::GetFrames()
{
	return GetGraphics()->GetFrames();
}

void cUsed::Reset()
{
	m_ulBack = GetFrames();
	m_ulFront = m_ulBack;
	m_nUsed = 0;
}

const ULONG g_ulFramePerSecond = 30;

void cUsed::Used() 
{
	if (GetFrames() - m_ulBack > 10*g_ulFramePerSecond)
	{
		Reset();
	}
	else
	{
		m_nUsed++;	
		m_ulFront = GetFrames();
	}
}

//enum UsedEnum{USED_JUSTCREATE, USED_OFTEN, USED_NORMAL, USED_SOLDEM, USED_NEVER,};
CString g_aStrUsed[] = 
{
	"justcreate","often","normal","soldem","never","",
};

UsedEnum cUsed::GetUsed() 
{
	ULONG frames = GetFrames();
	ULONG front = m_ulFront;
	ULONG back = m_ulBack;
	UsedEnum e = USED_NORMAL;
	if ( frames - back < 1 * g_ulFramePerSecond)
		e = USED_JUSTCREATE;
	else if ( frames - back < 5 * g_ulFramePerSecond)
		e = USED_OFTEN;
	else if ( frames - back < 20 * g_ulFramePerSecond)
		e = USED_NORMAL;
	else if ( frames - back < 60 * g_ulFramePerSecond)
		e = USED_SOLDEM;
	else
		e = USED_NEVER;
	//trace
/*
	if (!m_strName.IsEmpty())
	{
		CString strMsg;
		strMsg = "resource: " + m_strName + " " + g_aStrUsed[e] + "\n";
		OutputDebugString(strMsg);
	}
*/
	return e;
}
