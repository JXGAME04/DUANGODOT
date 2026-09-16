// cZipRead.cpp: implementation of the cZipRead class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cZipRead.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
cZipRead::cZipRead():m_ulCount(1)
{
	m_pBuffer = NULL;
	m_pPos = NULL;
	m_nLength = 0;
	m_nBufferLength = 0;
};

cZipRead::~cZipRead()
{
	delete m_pBuffer;
};

bool cZipRead::load(LPCSTR szZipFile) 	//load a .zip file from disk
{
	return m_unzipper.OpenZip(szZipFile) != FALSE;
}

bool cZipRead::locate(LPCSTR szFile) 	//locate a file then can read
{
	return m_unzipper.LocateFile(szFile);
}

int	cZipRead::length() 					//length of the file located
{
	return m_unzipper.GetFileLength();
}

int	cZipRead::read(void* buf, int* bufsize)
{
	return m_unzipper.ReadFromFile(buf,bufsize);
}

BOOL cZipRead::Open(LPCSTR szFile)
{
	if (m_unzipper.LocateFile(szFile))
	{
		m_nLength = length();
		if (m_nLength > m_nBufferLength)
		{
			delete m_pBuffer;
			m_pBuffer = new char[m_nLength];
			m_nBufferLength = m_nLength;
		}
		read(m_pBuffer,&m_nLength);
		m_pPos = m_pBuffer;
		return TRUE;
	}
	return FALSE;
}

DWORD cZipRead::GetLength()
{
	return m_nLength;
};

UINT cZipRead::Read(void* lpBuf, UINT count)
{
	memcpy(lpBuf,m_pPos,count);
	m_pPos += count;
	ASSERT(m_pPos <= m_pBuffer + m_nLength);
	return count;
}

