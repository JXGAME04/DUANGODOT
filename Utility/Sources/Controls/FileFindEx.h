// FileFindEx.h: interface for the CFileFindEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEFINDEX_H__D0B7991E_F086_4B9A_8513_4B20836FC6A7__INCLUDED_)
#define AFX_FILEFINDEX_H__D0B7991E_F086_4B9A_8513_4B20836FC6A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//find file sort by name
#include "afxtempl.h"

class CFileFindEx  
{
public:
	struct stFile{
		CString strName;
		CString strPath;
	};
	
	bool FindIndex(int i);
	CString GetFilePath();
	CString GetFileName();
	bool FindNextFile();
	bool FindFile(const CString& s);
	int GetFileCount(){return m_listFile.GetCount();}
	CFileFindEx();
	virtual ~CFileFindEx();

	void Clear();
	POSITION m_posFile;
	CTypedPtrList <CPtrList, stFile*> m_listFile;
	stFile* m_pFile;

	bool m_bNoDot;
	void SetNoDot(bool b){m_bNoDot= b;}
	bool m_bOnlyFile;
	void SetOnlyFile(bool b){m_bOnlyFile = b;}
	bool m_bOnlyDirectory;
	void SetOnlyDirectory(bool b){m_bOnlyDirectory = b;}
	enum eSort{SORT_NOSORT,SORT_NAME,SORT_DATE};
	eSort m_eSort;
protected:
	void AddFile(stFile* pF);
};

typedef CTypedPtrList <CPtrList, CFileFindEx::stFile*>  FileList;


#endif // !defined(AFX_FILEFINDEX_H__D0B7991E_F086_4B9A_8513_4B20836FC6A7__INCLUDED_)
