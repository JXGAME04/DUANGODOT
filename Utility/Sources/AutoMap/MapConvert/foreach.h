
/////////////////////////////////////////////////////////////////////////////
//for each region
/////////////////////////////////////////////////////////////////////////////
typedef int (*doonce)(LPCSTR szFile);
void OutputProgress(LPCSTR szOuput);

class foreach
{
public:
	CString m_strFindFile;		//寻找怎样的file
	CString m_strFindFolder;	//寻找怎样的folder
	doonce m_func;
	class foreach(doonce f) : m_func(f){m_strFindFolder = "*.", m_strFindFile = "*.*";};
	void SetFindFile(LPCSTR szFile){m_strFindFile = szFile;}
	void SetFindFolder(LPCSTR szFolder){m_strFindFolder = szFolder;}

	virtual int doonce_file(LPCSTR szFile)
	{
		OutputProgress(szFile);
		return m_func(szFile);
	}
	virtual int doonce_folder(LPCSTR szFolder)
	{
		OutputProgress(szFolder);
		return m_func(szFolder);
	}
	virtual bool is_foldervalid(LPCSTR szFolder)
	{
		return true;
	}
	int foreach_file(LPCSTR szFolder);
	int foreach_folder(LPCSTR szFolder);
	int foreach_folderandfile(LPCSTR szFolder,int ndepath = 0);
};

class foreachregion : public foreach
{
public:
	class foreachregion(doonce f) : foreach(f){m_strFindFolder = "*.", m_strFindFile = "*.wor";};
	CString m_strFileInRegion;
	void	SetFileInRegion(LPCSTR szFileInRegion){m_strFileInRegion = szFileInRegion;}

	bool is_foldervalid(LPCSTR szFolder);
	int doonce_file(LPCSTR szFile);
	int doonce_folder(LPCSTR szFolder){return doonce_file(szFolder);}
};

int foreach_fileunderfolder(doonce fun, LPCSTR szFindFile, LPCSTR szSrcFolder);
int foreach_folderunderfolder(doonce fun, LPCSTR szSrcFolder);
