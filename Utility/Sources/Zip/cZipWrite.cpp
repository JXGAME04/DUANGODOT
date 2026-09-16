// cZipWrite.cpp: implementation of the cZipWrite class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cZipWrite.h"

#include "zlib\zip.h"
#include "zlib\iowin32.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cZipWrite::cZipWrite()
{
	m_zip = NULL;
	m_ulCount = 1;
}

cZipWrite::~cZipWrite()
{
	if (m_zip)
		close();
}

bool cZipWrite::create(LPCSTR szZipFile) 
{
	m_zip = zipOpen(szZipFile,0);
	return m_zip != NULL;
}

bool cZipWrite::add(LPCSTR szFile,LPCSTR szZipName)
{
	CFile f;
	if (!f.Open(szFile,CFile::modeRead))
		return FALSE;

	int n = zipOpenNewFileInZip(m_zip,szZipName,NULL,
		NULL,0,
		NULL,0,
		NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION
		);
	if (n != ZIP_OK)
		return FALSE;

	char* buffer = new char[f.GetLength()];
	f.Read(buffer,f.GetLength());	
	zipWriteInFileInZip(m_zip,buffer,f.GetLength());
	zipCloseFileInZip(m_zip);
	delete buffer;

	return TRUE;
}

bool cZipWrite::close()
{
	int n = zipClose(m_zip,NULL);
	m_zip = NULL;
	return n == ZIP_OK;
}
