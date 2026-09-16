/*******************************************************************************
// FileName			:	KLEFileEnumerate.cpp
// FileAuthor		:	LaiLai
// FileCreateDate	:	01-3-13 11:25:57
// FileDescription	:	:	根据指定的根目录,及文件扩展名表,还有相关的选项
//					:   (比如是否递归调用子目录， Exclude 一些目录)
//					:	遍历调用所用找到的文件,执行一个回调函数,完成一个功能.
//					:	比如文件格式改变后,一次性完成所有的替换.

//					:	因该将所有类中的成员变量放到一个结构中(FileEnumOption)
//					:	m_szRootFolder没有用处因该去掉
//					:	还差文件过滤没有完成(Filter)

//	调用的步骤		:	书写特定的回调函数
//					:	设置 File Enumerate Option
//					:	调用 EnumFileInFolder
// Revision Count	:	
*******************************************************************************/

#include "StdAfx.h"
#include "KLEFileEnumerate.h"
#include "KLEFilePathMgr.h"
#include<string>

using namespace std;

KLEFileEnumerate::KLEFileEnumerate() : m_blIncludeSubFolder(true),
										m_szIncludeExt(NULL),
										m_szExcludeDir(NULL),
										m_szRootFolder(NULL),
										m_pFileEnumCallback(NULL),
										m_pParam(NULL)
{
	
}

KLEFileEnumerate::KLEFileEnumerate(bool blIncludeSubFolder, const char* szIncludeExt,
								   const char* szExcludeDir, const char* szRoot) : 
									m_blIncludeSubFolder(blIncludeSubFolder),
									m_szIncludeExt(NULL),
									m_szExcludeDir(NULL),
									m_szRootFolder(NULL),
									m_pFileEnumCallback(NULL),
									m_pParam(NULL)
										
{	
	int		nLen;

	if (szRoot)	
	{
		nLen = strlen(szRoot);
		m_szRootFolder = new char[nLen + 1];
		strcpy(m_szRootFolder, szRoot);
	}

	if (szIncludeExt)
	{
		nLen			= strlen(szIncludeExt);
		m_szIncludeExt	= new char[nLen + 1];
		strcpy(m_szIncludeExt, szIncludeExt);
	}

	if (szExcludeDir)
	{
		nLen			= strlen(szExcludeDir);
		m_szExcludeDir	= new char[nLen + 1];
		strcpy(m_szExcludeDir, szExcludeDir);
	}

}

KLEFileEnumerate::~KLEFileEnumerate()
{
	SAFE_DELGRP(m_szRootFolder);
	SAFE_DELGRP(m_szIncludeExt);
	SAFE_DELGRP(m_szExcludeDir);
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::SetRootFolder
// Purpose		: 设置要搜寻的起始路径
// Return		: void 
// Argumant		: const char* szRoot
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::SetRootFolder( const char* szRoot)
{
	int		nLen;

	if (szRoot)	
	{
		nLen = strlen(szRoot);
		m_szRootFolder = new char[nLen + 1];
		strcpy(m_szRootFolder, szRoot);
	}	
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::EnableSubFolder
// Purpose		: 指明是否搜寻子目录
// Return		: void 
// Argumant		: bool blIncludeSubFolder
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::EnableSubFolder(bool blIncludeSubFolder)
{
	m_blIncludeSubFolder	= blIncludeSubFolder;
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::SetIncludeFileExt
// Purpose		: 设置需要枚举的的文件扩展名(不包含'.'),多个扩展名之间用逗号隔开
// Return		: void 
// Argumant		: const char* szIncludeExt
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::SetIncludeFileExt(const char* szIncludeExt)
{
	int		nLen;

	if (szIncludeExt)	
	{
		nLen = strlen(szIncludeExt);
		m_szIncludeExt = new char[nLen + 1];
		strcpy(m_szIncludeExt, szIncludeExt);
	}
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::SetExcludeDir
// Purpose		: 设置不需要搜寻的路径(绝对路径),多个路径之间用逗号隔开
// Return		: void 
// Argumant		: const char* szExcludeDir
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::SetExcludeDir(const char* szExcludeDir)
{
	int		nLen;

	if (szExcludeDir)
	{
		nLen			= strlen(szExcludeDir);
		m_szExcludeDir	= new char[nLen + 1];
		strcpy(m_szExcludeDir, szExcludeDir);
	}
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::SetCallBack
// Purpose		: 设置回调函数
// Return		: void 
// Argumant		: LPFILEENUMCALLBACK pFileEnumCallBack
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::SetCallBack(LPFILEENUMCALLBACK pFileEnumCallBack)
{
	if (NULL == pFileEnumCallBack)	return;
	m_pFileEnumCallback = pFileEnumCallBack;
}

void KLEFileEnumerate::SetParam(void* pParam)
{
	m_pParam = pParam;
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::EnumFileInFolder
// Purpose		: 根据选项Enumerate所用合适的文件,调用回调函数完成特定的功能
// Return		: void 
// Argumant		: const char* szRoot
// Comments		: 对于多层目录是一个递归调用的过程
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::EnumFileInFolder(const char* szRoot)
{ 
	HANDLE				hFindFile;
	WIN32_FIND_DATA		Win32FindData;
	char				szPathName[KLEFMGR_MAXPATH];

	if (NULL == szRoot) return;

	KLEFilePathMgr::ComposePath(szPathName, szRoot, "\\*");
	hFindFile = FindFirstFile(szPathName, &Win32FindData);
	if (INVALID_HANDLE_VALUE == hFindFile)
		return;

	AnalysisFile(szRoot, Win32FindData);
	
	while(FindNextFile(hFindFile, &Win32FindData))
	{	
		AnalysisFile(szRoot, Win32FindData);	
	}

	return;
}

/*!*****************************************************************************
// Function		: KLEFileEnumerate::AnalysisFile
// Purpose		: 
// Return		: void 
// Argumant		: const char*				szRoot
// Argumant		: const WIN32_FIND_DATA&	Win32FindData
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFileEnumerate::AnalysisFile(const char* szRoot, const WIN32_FIND_DATA& Win32FindData)
{
	char	szPathName[KLEFMGR_MAXPATH];
	
	KLEFilePathMgr::ComposePath(szPathName, szRoot, Win32FindData.cFileName);
	if (FILE_ATTRIBUTE_DIRECTORY & Win32FindData.dwFileAttributes)
	{
		if ((0 != strcmp(Win32FindData.cFileName, ".")) && 
				(0 != strcmp(Win32FindData.cFileName, "..")))
			EnumFileInFolder(szPathName);
	}
	else
	{
		(*m_pFileEnumCallback)(szPathName, m_pParam);
	}
}
