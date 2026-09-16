// KScanFile.h: interface for the KScanFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KSCANFILE_H__EBD33C7A_B3D5_46B5_987F_325A6A92952F__INCLUDED_)
#define AFX_KSCANFILE_H__EBD33C7A_B3D5_46B5_987F_325A6A92952F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define FileExtLength 8
#define MAXPATH   260

#include<windows.h>
#include"SampleFileExecuter.h"




class KScanFile : public SampleFileExecuter
{
public:
	void SetScanPath(string ScanPath);
	void SetOutFile(string OutFile);
	void SetRootPath(string RootPath);
	void Execute();
		void DiscardFileExt(LPSTR lpFileExt);
	
	KScanFile();
	virtual ~KScanFile();

private:
	BOOL RunSearch(const char * lpRootPath,const char * lpScanPath,const char * lpOutFile);

	FILE*		m_pFile;
	int			m_nPathLen;
	char		m_FileExt[FileExtLength];

	string      m_sRootPath;
    string      m_sScanPath; 
	string      m_sOutFile;
	


	
	void OutputFileName(LPSTR FileName);
	BOOL CheckFileExt(LPSTR FileName);
	void SearchDirectory();
    
};

#endif // !defined(AFX_KSCANFILE_H__EBD33C7A_B3D5_46B5_987F_325A6A92952F__INCLUDED_)
