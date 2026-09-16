/*******************************************************************************
// FileName			:	KLEFilePathMgr.cpp
// FileAuthor		:	LaiLai
// FileCreateDate	:	00-11-14 16:35:04
// FileDescription	:	完成文件路径系统的相关操作,路径
//					:	需要根据需求不断完善	
// Revision Count	:	
*******************************************************************************/

#include "stdafx.h"
#include "KLEFilePathMgr.h"


/*****************************************************************************
// FUNCTION		: KLEFilePathMgr::KLEFilePathMgr
// PURPOSE		: 构造函数
// RETURN		: 
// COMMENTS		: 初始化当前路径,应该从注册表中读
// AUTHOR		: LaiLai
*****************************************************************************/
KLEFilePathMgr::KLEFilePathMgr()
{
	memset(m_szRootPath, 0, sizeof(m_szRootPath));
	memset(m_szCurPath, 0, sizeof(m_szCurPath));

	GetCurrentDirectory(KLEFMGR_MAXPATH, m_szRootPath);
	strcpy(m_szCurPath, m_szRootPath);
}

KLEFilePathMgr::~KLEFilePathMgr()
{

}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::SetCurPath
// Purpose		: 设置当前文件路径
// Return		: void 
// Argumant		: char* szPathName[in]	传入的是相对于根目录的相对路径
// Comments		: 如果传入的是 NULL 指针,则将原始的根目录设为当前目录
//				: 确保最后一个字符是 '\\'
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::SetCurPath(const char* szPathName)
{
	if( NULL == szPathName )
	{
		strcpy(m_szCurPath, m_szRootPath);
	}
	else
	{
		// 根据输入的路径头部是否有'\\',做不同的处理
		if('\\' == szPathName[0]) 
			sprintf(m_szCurPath, "%s%s", m_szRootPath, szPathName);
		else
			sprintf(m_szCurPath, "%s%s%s", m_szRootPath, "\\", szPathName);
	}
	// 确保路径的尾部包含'\\'
    int nlen = strlen(m_szCurPath) - 1;
	if (m_szCurPath[nlen] != '\\')
		lstrcat(m_szCurPath, "\\");
	
	//设置为当前工作目录
	SetCurrentDirectory(m_szCurPath);
}


/*!*****************************************************************************
// Function		: KLEFilePathMgr::AddFileNameToPath
// Purpose		: 将一个文件名去掉后缀后,加到一个路径的后面，形成一个新的路径
// Return		: void 
// Argumant		: char* szPathName[in][out]		原始路径/返回路径
// Argumant		: const char* szFileName[in]	带扩展名的文件名
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::AddFileNameToPath(char* szPathName, const char* szFileName)
{
    char    szFileNoExt[KLEFMGR_MAXFILE];
	char*	szExt = NULL;

    if( NULL == szFileName) return;
    if( NULL == szPathName) return;
    
	strcpy(szFileNoExt, szFileName);
	szExt = strrchr(szFileNoExt, '.');
	if(NULL == szExt) return;
	szExt[0] = 0;
    
	int nlen = strlen(szPathName);
	if ('\\' != szPathName[nlen-1])
		strcat(szPathName, "\\");

    strcat(szPathName, szFileNoExt);
    strcat(szPathName, "\\");
}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::GetCDDriver
// Purpose		: 得到CDROM对应的Driver
// Return		: void 
// Argument		: char* szDriver[in][out]	返回CDDOM的驱动器盘符
// Comments		: 系统具备多个CRROM时，返回第一个找到的
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::GetCDDriver(char* szDriver)
{
	lstrcpy(szDriver, "C:");
	for (int i = 0; i < 24; szDriver[0]++, i++)
	{
		if (GetDriveType(szDriver) == DRIVE_CDROM)
			return;
	}
}

/*!*****************************************************************************
// Function		: IsFileExist
// Purpose		: 检查指定的文件是否存在
// Return		: bool 
// Argumant		: char szPathName	文件的全路径名
// Comments		: 需要转为用 C RunTime 实现
// Author		: LaiLai
*****************************************************************************/
bool KLEFilePathMgr::IsFileExist(const char* szPathName)
{
	WIN32_FIND_DATA		FindData;
	HANDLE				hFindHandle;

	hFindHandle = FindFirstFile(szPathName, &FindData);
	if (hFindHandle == INVALID_HANDLE_VALUE)
		return false;
	FindClose(hFindHandle);
	return true;
}

//******************************************************************************
/*! \fn     bool KLEFilePathMgr::IsPathExist()
*   \brief  检测路径是否存在
*   \return bool 返回值的描述
*******************************************************************************/
bool KLEFilePathMgr::IsPathExist(const char* szPathName)
{
	WIN32_FIND_DATA		FindData;
	HANDLE				hFindHandle;
	char				szBuf[KLELNAMELEN];

	KLEFilePathMgr::ComposePath(szBuf, szPathName, "\\*");
	hFindHandle = FindFirstFile(szBuf, &FindData);
	if (hFindHandle == INVALID_HANDLE_VALUE)
		return false;
	FindClose(hFindHandle);
	return true;
}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::ComposePath
// Purpose		: 将两个路径合成结果放在第一个路径中
// Return		: void 
// Argumant		: char*			szFirstPath	[in][out]
// Argumant		: const char*	szSecondPath[in]
// Comments		: szFirstPath的尾部,szSecondPath的头部是否包含'\\'都可以
//				: 根据第一个路径结尾处和第一个路径起始处是否有"\\"做不同的处理
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::ComposePath( char* szFirstPath, const char* szSecondPath)
{
	if (NULL == szFirstPath)	return;
	if (NULL == szSecondPath)	return;

	int nLen = strlen(szFirstPath);
	if ('\\' == szFirstPath[nLen-1])
	{
		if ('\\' == szSecondPath[0])
		{
			szFirstPath[nLen-1]	= 0;		
		}
		strcat(szFirstPath, szSecondPath);
	}
	else if ('\\' == szSecondPath[0])
	{
		strcat(szFirstPath, szSecondPath);
	}
	else
	{
		strcat(szFirstPath, "\\");
		strcat(szFirstPath, szSecondPath);
	}

	return;
}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::ComposePath
// Purpose		: 将两个路径合成结果放在另一个不同的Buf中
// Return		: void 
// Argumant		: char*			szResult	[out]		结果路径,外部分配
// Argumant		: const char*	szFirstPath	[in]		第一路径
// Argumant		: const char*	szSecondPath[in]		第二路径
// Comments		: szFirstPath的尾部,szSecondPath的头部是否包含'\\'都可以
//				: 根据第一个路径结尾处和第一个路径起始处是否有"\\"做不同的处理
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::ComposePath( char* szResult, const char* szFirstPath, const char* szSecondPath)
{
	if (NULL == szResult)		return;
	if (NULL == szFirstPath)	return;
	if (NULL == szSecondPath)	return;

	int nLen = strlen(szFirstPath);
	if ('\\' == szFirstPath[nLen-1])
	{
		strcpy(szResult, szFirstPath);
		if ('\\' == szSecondPath[0])
		{
			szResult[nLen-1] = 0;
		}
		strcat(szResult, szSecondPath);
	}
	else if ('\\' == szSecondPath[0])
	{
		sprintf(szResult, "%s%s", szFirstPath, szSecondPath);
	}
	else
	{
		sprintf(szResult, "%s%s%s", szFirstPath, "\\", szSecondPath);
	}

	return;
}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::MakeFullPath
// Purpose		: 由 Driver, Dir, 文件名, 文件扩展名构成一个全路径, 可传入空指针
// Return		: void 
// Argumant		: char*			szFullPath[in][out]		要合成的全路径Buffer
// Argumant		: const char*	szDriver[in]
// Argumant		: const char*	szDir[in]
// Argumant		: const char*	szFile[in]
// Argumant		: char*			szExt[in]
// Comments		:	成功调用实例
//				:	_makepath( path_buffer, "c", "\\sample\\crt\\", "makepath", "c" );  
//				:	_makepath( path_buffer, "c:", "\\sample\\crt\\", "makepath", ".c" ); 
//				:	_makepath( path_buffer, "c", "\\sample\\crt\\", "makepath.c", NULL ); 
//				:	_makepath( path_buffer, NULL, "c:\\sample\\crt\\", "makepath.c", NULL ); 
//				:	_makepath( path_buffer, "c", "sample\\crt", "makepath", "c" );  
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::MakeFullPath(char* szFullPath, const char* szDriver, const char* szDir, const char* szFile, char* szExt)
{
	_makepath(szFullPath, szDriver, szDir, szFile, szExt);
}


/*!*****************************************************************************
// Function		: KLEFilePathMgr::SpilitFullPath
// Purpose		: 由 一个全路径分解为 Driver, Dir, 文件名, 文件扩展名, 可传入空指针
// Return		: void 
// Argumant		: const char*	szFullPath[in]
// Argumant		: char*			szDriver[in][out]	如"c:"
// Argumant		: char*			szDir[in][out]
// Argumant		: char*			szFile[in][out]
// Argumant		: char*			szExt[in][out]		如".exe"
// Comments		: 灵活的调用可得到不同的结果
//				: path_buffer =  "c:\\sample\\crt\\makepath.c");
//				: _splitpath( path_buffer, drive, NULL, NULL, ext ); // 仅仅得到 Driver 和Ext
//				: path_buffer =  "\\sample\\crt\\makepath.c");
//				: _splitpath( path_buffer, drive, dir, fname, ext ); // driver 不受影响
//				: path_buffer = "sample\\crt\\makepath");
//				: _splitpath( path_buffer, drive, dir, fname, ext ); // driver 和 ext 不受影响
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::SpilitFullPath(const char* szFullPath, char* szDriver, 
									char* szDir, char* szFile, char* szExt)
{
	 _splitpath(szFullPath, szDriver, szDir, szFile, szExt);
}

/*!*****************************************************************************
// Function		: KLEFilePathMgr::GetFullPath
// Purpose		: 传入文件名,根据当前路径返回全路径名
// Return		: void 
// Argumant		: char*			szPathName[out]	要返回的真实路径名
// Argumant		: const char*	szFileName[in]	传入的虚拟路径名
// Comments		: 取得真实路径名
// Author		: LaiLai
*****************************************************************************/
void KLEFilePathMgr::GetFullPath(char* szPathName, const char* szFileName) const
{
	if (szFileName[0] == '\\')
	{
		lstrcpy(szPathName, m_szRootPath);
		lstrcat(szPathName, szFileName);
		return;
	}

	// 包含驱动器号,已经是全路径了
	if (szFileName[1] == ':')
	{
		lstrcpy(szPathName, szFileName);
		return;
	}

	if (m_szCurPath[1] == ':')
	{
		lstrcpy(szPathName, m_szCurPath);
	}
	else
	{
		lstrcpy(szPathName, m_szRootPath);
		lstrcat(szPathName, m_szCurPath);
	}

	if (szPathName[lstrlen(szPathName)-1] != '\\')
		lstrcat(szPathName,"\\");
	lstrcat(szPathName, szFileName);
}

