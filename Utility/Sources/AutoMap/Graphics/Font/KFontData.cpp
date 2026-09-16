/*******************************************************************************
// FileName			:	KFontData.h
// FileAuthor		:	Wooy
// FileCreateDate	:	2002-9-4
// FileDescription	:	字体图形数据类头文件
// Revision Count	:	
*******************************************************************************/
#include "KFontData.h"
#include "../../../../SwordOnline/Sources/Engine/Src/KWin32.h"
#include "../../../../SwordOnline/Sources/Engine/Src/KPakFile.h"

/*!*****************************************************************************
// Purpose		:  构造函数
*****************************************************************************/
KFontData::KFontData()
{
	m_pFontData = NULL;
	m_dwDataSize = 0;
	m_nFontW = 0;
	m_nFontH = 0;
	memset(m_dwOffs, 0, sizeof(m_dwOffs));
}

/*!*****************************************************************************
// Purpose		:  析构函数
*****************************************************************************/
KFontData::~KFontData()
{
	Terminate();
}

/*!*****************************************************************************
// Function		: KFontData::Init
// Purpose		: 载入字库文件
// Return		: 是否成功
// Argumant		: const char* pszFontFile      -> 字库文件名
*****************************************************************************/
bool KFontData::Load(const char* pszFontFile)
{
	Terminate();
	if (pszFontFile == NULL)
		return false;

	KPakFile	File;
	KFontHead	Header;

	//打开字库文件
	if (!File.Open((char*)pszFontFile))
		return false;

	//读字库文件头结构
	File.Read(&Header, sizeof(Header));

	//检查字库文件ID
		//检查字库文件ID
	if (memcmp(Header.Id, "ASF", 4))
		return false;

	//得到字体信息
	m_dwDataSize = Header.Size;
	m_nCharCount = Header.Count;
	m_nFontW = Header.Width;
	m_nFontH = Header.Height;

	//分配字符点阵信息数据的存储空间
	if ((m_pFontData = (unsigned char*)malloc(m_dwDataSize)) == NULL)
		return false;		

	//读取偏移表
	if (m_nCharCount <= KFR_MAX_NUM_CHAR)
	{
		File.Read(m_dwOffs, m_nCharCount * 4);
	}
	else
	{
		File.Read(m_dwOffs, KFR_MAX_NUM_CHAR * 4);
		m_nCharCount = KFR_MAX_NUM_CHAR;
		File.Seek((m_nCharCount - KFR_MAX_NUM_CHAR) * 4, FILE_CURRENT);
	}

	//读取字库点阵数据
	File.Read(m_pFontData, m_dwDataSize);

	//关闭字库文件
	File.Close();

	return true;
}

/*!*****************************************************************************
// Function		: KFontData::GetInfo
// Purpose		: 得到字体的宽度高度，贴图可以缓存的字符的个数
// Argumant		: int &nWidth	字符宽
// Argumant		: int &nHeight	字符高
*****************************************************************************/
void KFontData::GetInfo(int &nWidth, int &nHeight) const
{
	nWidth  = m_nFontW;
	nHeight = m_nFontH;
}

/*!*****************************************************************************
// Function		: KFontData::Terminate
// Purpose		: 结束，清除操作
*****************************************************************************/
void KFontData::Terminate()
{
	if (m_pFontData)
	{
		free(m_pFontData);
		m_pFontData = NULL;
	}
	m_nCharCount = 0;
	m_dwDataSize = 0;
}

/*!*****************************************************************************
// Function		: KFontData::GetCharacterData
// Purpose		: 取得单个字符的数据区
*****************************************************************************/
unsigned char*	KFontData::GetCharacterData(unsigned char cFirst, unsigned char cNext) const
{
	if (m_pFontData)
	{
		int		nCharIndex;	//字符在字库里的索引
		//GBK编码方式的索引运算
		nCharIndex = (cFirst - 0x81) * 190 + (cNext - 0x40) - (cNext >> 7);
		if (nCharIndex >= 0 && nCharIndex < m_nCharCount && m_dwOffs[nCharIndex])
			return (m_pFontData + m_dwOffs[nCharIndex]);
	}
	return NULL;
}