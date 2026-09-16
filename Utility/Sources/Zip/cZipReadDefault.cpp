// cZipReadDefault.cpp: implementation of the cZipReadDefault class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cZipReadDefault.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BOOL cZipReadDefault::Open(LPCSTR szFile) 
{
	CFileException e;
	BOOL b = f.Open(szFile,CFile::modeRead,&e);
	if (!b)
	{
		char szBuffer[1024];
		e.GetErrorMessage(szBuffer,1024);
		OutputDebugString(szBuffer);
		OutputDebugString("\n");
	}
	return b;
}
