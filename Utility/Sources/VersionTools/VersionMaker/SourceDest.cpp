// SourceDest.cpp: implementation of the SourceDest class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VersionMaker.h"
#include "SourceDest.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SourceDest::SourceDest(const char * Source,const char * Dest)
:m_sSource(Source)
,m_sDest(Dest)
{

}

SourceDest::~SourceDest()
{

}

string SourceDest::GetSource()
{
	return m_sSource;

}

string SourceDest::GetDest()
{
	return m_sDest;

}
