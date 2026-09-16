// KScanFile.cpp: implementation of the KScanFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KScanFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

KScanFile::KScanFile()
:m_sScanPath()
,m_sRootPath()
,m_sOutFile()
,m_pFile(NULL)

{
	for(int i=0;i<FileExtLength;i++)
	{
			m_FileExt[i] =0;


	}



}

KScanFile::~KScanFile()
{

}

void KScanFile::SearchDirectory()
{
WIN32_FIND_DATA FindData;
	HANDLE hFind = NULL;
	
	// set findfirstfile to find everthing
	hFind = FindFirstFile("*", &FindData);

	// if handle fails, drive is empty...
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	
	// get first entity on drive - check if it's a directory
	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		// if so, change to that directory and recursively call SearchDirectory
		if (FindData.cFileName[0] != '.')
		{
			// enter the directory
			SetCurrentDirectory(FindData.cFileName);
			// search the directory
			
			SearchDirectory();
			// go back up one directory level
			SetCurrentDirectory("..");
		}
	}
	else
	{
		if (CheckFileExt(FindData.cFileName))
			OutputFileName(FindData.cFileName);
	}

	while (FindNextFile(hFind, &FindData))
	{
		
		
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindData.cFileName[0] != '.')
			{
				SetCurrentDirectory(FindData.cFileName);
				SearchDirectory();
				SetCurrentDirectory("..");
			}
		}
		else
		{
			if (CheckFileExt(FindData.cFileName))
				OutputFileName(FindData.cFileName);
		}
	}

	FindClose(hFind);

}

BOOL KScanFile::CheckFileExt(LPSTR FileName)
{
		if (memcmp(m_FileExt, &FileName[strlen(FileName) - 3], 3) == 0)
		return FALSE;
	return TRUE;

}

void KScanFile::OutputFileName(LPSTR FileName)
{
	
		
		char szPathName[MAXPATH];
		
		GetCurrentDirectory(MAXPATH, szPathName);
		strcat(szPathName, "\\");
		strcat(szPathName, FileName);
		
		InsertFilePath(string(szPathName));
		
		
		
		strcat(szPathName, "\r\n");
		if(m_pFile !=NULL)
		{
			fprintf(m_pFile, "%s", &szPathName[m_nPathLen]);
			
			
		}



}

void KScanFile::DiscardFileExt(LPSTR lpFileExt)
{
	CopyMemory(m_FileExt, lpFileExt,3);

}

BOOL KScanFile::RunSearch(const char * lpRootPath,const char * lpScanPath,const char * lpOutFile)
{
	if(string(lpRootPath)!= "" && string(lpScanPath)!="")
	{
		
		CleanResult();
		
		if(string(lpOutFile) !="")
		{
			
			m_pFile = fopen(lpOutFile, "wb");
			if (m_pFile == NULL)
				return FALSE;
			m_nPathLen = strlen(lpRootPath);
			if (lpRootPath[m_nPathLen - 1] != '\\')
				m_nPathLen++;
			fprintf(m_pFile, "%s\r\n", lpRootPath);
		}
		SetCurrentDirectory(lpScanPath);
		SearchDirectory();
		if(string(lpOutFile) !="")
		{
			fclose(m_pFile);
		}
		return TRUE;
		
    }
	else
	{
		throw exception("exception : KScanFile Param is empty!!");
		
	}
	
}

void KScanFile::Execute()
{
	RunSearch(m_sRootPath.c_str(),m_sScanPath.c_str(),m_sOutFile.c_str());


}

void KScanFile::SetRootPath(string RootPath)
{
	m_sRootPath = RootPath;

}

void KScanFile::SetOutFile(string OutFile)
{
	m_sOutFile = OutFile;


}

void KScanFile::SetScanPath(string ScanPath)
{
	m_sScanPath =ScanPath;


}
