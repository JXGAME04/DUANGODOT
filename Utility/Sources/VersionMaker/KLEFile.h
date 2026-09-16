//******************************************************************************
/*! \file       KLEFile.h
*   \brief      封装了文件相关的操作
*
*   \author		LaiLai
*   \version	1.0
*   \date		01-4-18 17:06:18
*   \sa			KLEFilePathMgr, KLEFileEnumerate
*******************************************************************************/

#ifndef _KLE_FILE_H_
#define _KLE_FILE_H_

//! Opens for both reading and writing. (The file must exist.) Binary
#define KLEFSB_OPENMODE_EXIST	"r+b"

//! Opens an empty file for both reading and writing. If the given file 
//! exists, its contents are destroyed. Binary
#define KLEFSB_OPENMODE_CREATE	"w+b"

//! Opens for reading and appending; the appending operation includes the 
//! removal of the EOF marker before new data is written to the file and 
//! the EOF marker is restored after writing is complete; creates the file 
//! first if it doesn’t exist. Binary
#define KLEFSB_OPENMODE_APPEND	"a+b"


//! Opens for both reading and writing. (The file must exist.) Text
#define KLEFST_OPENMODE_EXIST	"r+t"

//! Opens an empty file for both reading and writing. If the given file 
//! exists, its contents are destroyed. Text
#define KLEFST_OPENMODE_CREATE	"w+t"

//! Opens for reading and appending; the appending operation includes the 
//! removal of the EOF marker before new data is written to the file and 
//! the EOF marker is restored after writing is complete; creates the file 
//! first if it doesn’t exist. Text
#define KLEFST_OPENMODE_APPEND	"a+t"



#define	KLEFS_ERROR			-1
#define	KLEFS_EOF			0
#define	KLEFS_SUCCESS		1

//************************************************************************************************************************
/*! \class    KLEFile 
*   \brief    封装了文件相关的操作
*
*   \author   LaiLai
*   \version  1.0
*   \sa       KLEFilePathMgr, KLEFileEnumerate
*************************************************************************************************************************/
class KLEFile  
{
public:
	KLEFile();
	KLEFile(const char* szName, const char* szMode);
	virtual ~KLEFile();

	//! 检查文件当前是否打开
	bool	IsOpened(){ return m_stream ? true : false;}
	//! 打开文件
	bool	Open(const char* szName, const char* szMode);
	//! 读文件
	bool	Read(void *buffer, unsigned int count);
	//! 读一行
	int		ReadLine(void *buffer, unsigned int count);
	//! 写文件
	bool	Write(const void *buffer, unsigned int count);
	//! 写字符串到文件
	bool	WriteString(const char* szBuf);
	//! 得到当前的位置
	long	GetCurPos();
	//! 设置文件操作位置,参数缺省值不要轻易改变，如果要改变，一定要通知组内其他成员
	bool	Seek(long offset, int origin = SEEK_SET);
	//! 关闭文件
	void	Close();
	//! 得到尾部的位置
	long	GetEndPos();

protected:
	//! 文件流指针
	FILE  *m_stream;
};

#endif 
