//******************************************************************************
/*! \file       KLEFilePathMgr.h
*   \brief      完成文件路径系统的相关操作,路径
*				
*   \author		lailai
*   \version	1.0
*   \date		00-11-14 16:25:51
*   \sa			参考C RunTime Lib 函数  *_fullpath, _splitpath
*   \todo		
*******************************************************************************/


#ifndef _KLE_FILEPATH_MGR_
#define _KLE_FILEPATH_MGR_

class KLEFilePathMgr  
{
public:
	KLEFilePathMgr();
	virtual ~KLEFilePathMgr();

	//! 检测文件是否存在
	static bool	IsFileExist(const char* szPathName);
	//! 检测路径是否存在
	static bool	IsPathExist(const char* szPathName);
	//! 得到CDROM对应的Driver
	static void GetCDDriver(char* szDriver);
	//! 将一个文件名去掉后缀后,加到一个路径的后面，形成一个新的路径
	static void AddFileNameToPath(char* szPathName, const char* szFileName);
	//! 将两个路径合成结果放在第一个路径中,可灵活使用
	static void ComposePath( char* szFirstPath, const char* szSecondPath);
	//! 将两个路径合成结果放在另一个不同的Buf中,可灵活使用
	static void ComposePath( char* szResult, const char* szFirstPath, const char* szSecondPath);
	//!  由 Driver, Dir, 文件名, 文件扩展名构成一个全路径, 可传入空指针
	static void MakeFullPath(char* szFullPath, const char* szDriver, const char* szDir, const char* szFile, char* szExt);
	//!  由 一个全路径分解为 Driver, Dir, 文件名, 文件扩展名, 可传入空指针
	static void SpilitFullPath(const char* szFullPath, char* szDriver, char* szDir, char* szFile, char* szExt);


	//! 取得本程序的根路径
	inline bool	GetRootPath(char* szPathName) const;

	//! 得到当前路径
	inline bool	GetCurPath(char* szPathName) const;

	//! 设置当前路径,传入的是相对于根目录的相对路径
	void	SetCurPath(const char* szPathName);
	
	//!
	void	GetFullPath(char* szPathName, const char* szFileName) const;

protected:
	// 根目录,应用程序所在的路径, 为绝对路径,结尾处没有'\\'.
	char	m_szRootPath[KLEFMGR_MAXPATH];
	// 当前路径, 为绝对路径,
	char	m_szCurPath[KLEFMGR_MAXPATH];		
};

/*****************************************************************************
// FUNCTION		: KLEFileSys::GetRootPath
// PURPOSE		: 取得本程序的根路径
// RETURN		: bool 
// ARGUMENT		: char* szPathName[in][out]
// COMMENTS		: 
// AUTHOR		: LaiLai
*****************************************************************************/
inline bool KLEFilePathMgr::GetRootPath(char* szPathName) const
{
	if( NULL == szPathName )	return false;
	strcpy(szPathName, m_szRootPath);

	return true;
}

/*****************************************************************************
// FUNCTION		: KLEFileSys::GetCurPath
// PURPOSE		: 取得当前文件路径
// RETURN		: void 
// ARGUMENT		: char* szPathName[in][out]
// COMMENTS		:
// AUTHOR		: LaiLai
*****************************************************************************/
inline bool KLEFilePathMgr::GetCurPath(char* szPathName) const
{
	if( NULL == szPathName ) return false;
	strcpy(szPathName, m_szCurPath);
	return true;
}

#endif 
