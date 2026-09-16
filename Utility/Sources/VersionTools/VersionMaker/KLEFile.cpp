//******************************************************************************
/*! \file       KLEFile.cpp
*   \brief      封装了文件相关的操作
*
*   \author		LaiLai
*   \version	1.0
*   \date		01-4-18 17:06:18
*   \sa			KLEFilePathMgr, KLEFileEnumerate
*******************************************************************************/


#include "stdafx.h"
#include "KLEFile.h"

KLEFile::KLEFile()
{
	m_stream = NULL;
} 

KLEFile::~KLEFile()
{
	Close(); 
}

/*!*****************************************************************************
// Function		: KLEFile::KLEFile
// Purpose		: 
// Return		: 
// Argumant		: const char* szName	文件名,相对(相对于当前路径)绝对路径均可,
// Argumant		: const char* szMode	文件打开模式
// Comments		:
// Author		: LaiLai
*****************************************************************************/
KLEFile::KLEFile(const char* szName, const char* szMode)
{ 
	Open(szName, szMode);	
}


/*!*****************************************************************************
// Function		: KLEFile::Close
// Purpose		: 关闭文件流
// Return		: void 
// Comments		:
// Author		: LaiLai
*****************************************************************************/
void KLEFile::Close() 
{ 
	if(m_stream) 
	{
		fclose(m_stream); 
		m_stream = NULL; 
	}	
}

//******************************************************************************
/*! \fn     bool KLEFile::Open(const char* szName, const char* szMode)
*   \brief  打开一个文件
*
*           
*   \param  const char* szName 文件名,相对(相对于当前路径)绝对路径均可,
*   \param  const char* szMode 文件打开模式
*   \return bool               true 成功, false 失败
*   \sa     Close()
*******************************************************************************/
bool KLEFile::Open(const char* szName, const char* szMode)
{
	Close();
	m_stream = fopen(szName, szMode);
	if( NULL == m_stream)
		return false;
	else
		return true;
}

/*!*****************************************************************************
// Function		: KLEFile::Read
// Purpose		: 从文件流当前位置中读取一定长度的字符
// Return		: bool			true 成功, false 失败
// Argumant		: void*			buffer
// Argumant		: unsigned int	count	whole size in bytes
// Comments		:
// Author		: LaiLai
*****************************************************************************/
bool KLEFile::Read(void* buffer, unsigned int count)
{
	if (NULL == m_stream)
		return false;
	if (NULL == buffer)
		return false;

	unsigned int readed = fread(buffer, 1, count, m_stream);
	if( readed != count)
		return false;
	else
		return true;
}

int KLEFile::ReadLine(void *buffer, unsigned int count)
{
	if (NULL == m_stream)
		return 0;
	if (NULL == buffer)
		return 0;
	
	if( NULL == fgets( (char*)buffer, count, m_stream ))
	{
		if ( ferror(m_stream))
			return KLEFS_ERROR;
		else
			return KLEFS_EOF;
	}
	else
	{
		return KLEFS_SUCCESS;
	}
}

/*!*****************************************************************************
// Function		: KLEFile::Write
// Purpose		: 从文件流当前位置写入一定长度的字符
// Return		: bool			true 成功, false 失败
// Argumant		: const void*	buffer
// Argumant		: unsigned int	count	whole size in bytes to be written
// Comments		:
// Author		: LaiLai
*****************************************************************************/
bool KLEFile::Write(const void* buffer, unsigned int count)
{
	if( NULL == m_stream )
		return false;
	if (NULL == buffer)
		return false;

	unsigned int writed = fwrite(buffer, 1, count, m_stream);
	if( writed != count )
		return false;
	else
		return true;
}

//******************************************************************************
/*! \fn     bool KLFile::WriteString(const char* szBuf)
*   \brief  向文件中写入字符串
*   \param  const char* szBuf 参数的描述
*   \return bool 返回值的描述
*******************************************************************************/
bool KLEFile::WriteString(const char* szBuf)
{
	if( NULL == m_stream )
		return false;
	if (NULL == szBuf)
		return false;

	unsigned int nRet = fputs(szBuf, m_stream);
	if( nRet == EOF )
		return false;
	else
		return true;	
}

/*!*****************************************************************************
// Function		: KLEFile::GetCurPos
// Purpose		: 得到文件的当前的操作位置
// Return		: long	相对于文件头的位置
// Comments		:
// Author		: LaiLai
*****************************************************************************/
long KLEFile::GetCurPos()
{
	if( NULL == m_stream )
		return 0;

	return ftell(m_stream);
}

/*!*****************************************************************************
// Function		: KLEFile::Seek
// Purpose		: 设置文件操作位置
// Return		: bool			true 成功, false 失败
// Argumant		: long offset	偏移位置
// Argumant		: int origin	相对的起点
//				:				SEEK_CUR Current position of file pointer
//				:				SEEK_END End of file
//				:				SEEK_SET Beginning of file
// Comments		:
// Author		: LaiLai
*****************************************************************************/
bool KLEFile::Seek(long offset, int origin)
{
	if( NULL == m_stream )
		return false;

	int res = fseek(m_stream, offset, origin);
	if(0 == res)
		return true;
	else
		return false;
}

long KLEFile::GetEndPos()
{
	if (!Seek(0, SEEK_END))
		return GetCurPos();
	else
		return 0;
}