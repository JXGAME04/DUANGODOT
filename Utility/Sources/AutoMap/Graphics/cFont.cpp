// cFont.cpp: implementation of the cFont class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cFont.h"

#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cFont::cFont(cGraphics* p)
{
	m_ulCount = 1;
	m_pGraphics = p;
}

cFont::~cFont()
{
}

/**************************************************************************/
//com interface	
/**************************************************************************/
ULONG cFont::AddRef() 
{
	return ++m_ulCount;
}

ULONG cFont::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

