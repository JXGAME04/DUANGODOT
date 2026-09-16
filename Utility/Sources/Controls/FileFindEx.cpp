// FileFindEx.cpp: implementation of the CFileFindEx class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileFindEx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileFindEx::CFileFindEx()
{
	m_bNoDot = true;
	m_bOnlyFile = true;
	m_bOnlyDirectory = false;
	m_eSort = SORT_NAME;
	m_posFile = NULL;
}

CFileFindEx::~CFileFindEx()
{
	Clear();
}

void CFileFindEx::Clear()
{
	for (POSITION pos = m_listFile.GetHeadPosition(); pos != NULL;)
	{
		stFile* pFile = m_listFile.GetNext(pos);
		delete pFile;
	}
	m_listFile.RemoveAll();
}

bool CFileFindEx::FindFile(const CString &s)
{
	if (s.GetAt(s.GetLength() - 1) == '.')
	{
		m_bOnlyDirectory = true;
		m_bOnlyFile = false;
	}

	Clear();
	m_posFile = NULL;
	m_pFile = NULL;

	CFileFind f;
	BOOL b = f.FindFile(s);
	if (!b)
		return false;

//if no * mean find test only!
//	if (s.Find('*') == -1)
//		return true;

	while(b)
	{
		b = f.FindNextFile();
		if (m_bNoDot)
		{
			if (f.IsDots())
				continue;
		}
		if (m_bOnlyFile)
		{
			if (f.IsDirectory())
				continue;
		}
		if (m_bOnlyDirectory)
		{
			if (!f.IsDirectory())
				continue;
		}
		stFile* p = new stFile;
		p->strName = f.GetFileName();
		p->strPath = f.GetFilePath();

		AddFile(p);
	}

	m_posFile = m_listFile.GetHeadPosition();
	if (m_posFile == NULL)
		return false;
	return true;
}

void CFileFindEx::AddFile(stFile* pF)
{
	if (m_eSort == SORT_NOSORT)
	{
		m_listFile.AddTail(pF);
		return ;
	}
	if (m_eSort == SORT_NAME)
	{
		for (POSITION pos = m_listFile.GetHeadPosition();pos !=NULL;)
		{
			POSITION p = pos;
			stFile* pFile = m_listFile.GetNext(pos);

			if(pFile->strName.CompareNoCase(pF->strName) > 0)
			{
				m_listFile.InsertBefore(p,pF);
				return;
			}
		}
		m_listFile.AddTail(pF);
		return;
	}
}

bool CFileFindEx::FindNextFile()
{
	ASSERT(m_posFile);
	m_pFile = m_listFile.GetNext(m_posFile);
	if (m_posFile == NULL)
		return false;
	return true;
}

CString CFileFindEx::GetFileName()
{
	return m_pFile->strName;		
}

CString CFileFindEx::GetFilePath()
{
	return m_pFile->strPath;
}

bool CFileFindEx::FindIndex(int i)
{
	POSITION pos  = m_listFile.FindIndex(i);
	if (pos == NULL)
		return false;
	m_posFile = pos;
	m_pFile = m_listFile.GetNext(m_posFile);
	return true;
}
