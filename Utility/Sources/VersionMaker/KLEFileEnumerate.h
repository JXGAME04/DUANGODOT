/*******************************************************************************
// FileName			:	KLEFileEnumerate.h
// FileAuthor		:	LaiLai
// FileCreateDate	:	01-3-13 11:26:21
// FileDescription	:	根据指定的根目录,及文件扩展名表,还有相关的选项
//					:   (比如是否递归调用子目录， Exclude 一些目录)
//					:	遍历调用所用找到的文件,执行一个回调函数,完成一个功能.
//					:	比如文件格式改变后,一次性完成所有的替换.
// Revision Count	:	
*******************************************************************************/

#ifndef _KLE_FILEENUMARATE_H_
#define _KLE_FILEENUMARATE_H_

//! 传入的是全路径名
typedef void (*LPFILEENUMCALLBACK)(const char* szFileName, void* pParam);

class KLEFileEnumerate  
{
public:
	KLEFileEnumerate();
	KLEFileEnumerate(bool blIncludeSubFolder, const char* szIncludeExt,
					const char* szExcludeDir, const char* szRoot);
	virtual ~KLEFileEnumerate();

	void SetRootFolder( const char* szRoot);
	void EnableSubFolder(bool blIncludeSubFolder);
	void SetIncludeFileExt(const char* szIncludeExt);
	void SetExcludeDir(const char* szExcludeDir);
	void SetCallBack(LPFILEENUMCALLBACK pFileEnumCallBack);
	void SetParam(void* pParam);
	void EnumFileInFolder(const char* szRoot);
	

protected:
	//! 是否搜索子目录
	bool	m_blIncludeSubFolder;
	//! 根目录的文件名
	char*	m_szRootFolder;

	//! 应该在这里使用字符串列表(KLEList, KLEString),更方便更安全
	//! 需要Enumerate的文件扩展名表,多个扩展名之间用逗号隔开
	char*	m_szIncludeExt;
	//! 不需要 Enumerate的文件目录表,多个之间用逗号隔开
	char*	m_szExcludeDir;

	void*	m_pParam;

	LPFILEENUMCALLBACK	m_pFileEnumCallback;

	void AnalysisFile(const char* szRoot, const WIN32_FIND_DATA& Win32FindData);
};

#endif 
