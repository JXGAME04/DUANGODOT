/*******************************************************************************
// FileName			:	KFont.h
// FileAuthor		:	Wooy
// FileCreateDate	:	2002-9-5
// FileDescription	:	字体类
// Revision Count	:	
*******************************************************************************/
#include "KFont.h"
#include "string.h"

/*!*****************************************************************************
// Purpose		:  构造函数
*****************************************************************************/
KFont::KFont()
{
	m_nFontWidth	= 16;
	m_nFontHeight	= 16;
	m_nLastPosH		= 0;
	m_nLastPosV		= 0;
	m_pDevice		= 0;
	m_bLoaded		= false;
}

/*!*****************************************************************************
// Purpose		:  析构函数
*****************************************************************************/
KFont::~KFont()
{
	Terminate();
}

/*!*****************************************************************************
// Function		: KFont::Init
// Purpose		: 初始化
// Return		: bool 是否成功
// Argumant		: void* pDrawDevice -> 接口的实例的指针
*****************************************************************************/
bool KFont::Init(void* pDrawDevice)
{
	Terminate();
	if ((m_pDevice = pDrawDevice) == 0)
		return false;
	RestoreDeviceObjects();
	return true;
}

/*!*****************************************************************************
// Function		: KFont::Init
// Purpose		: 释放接口实例对象
*****************************************************************************/
void KFont::Release()
{
	delete this;
}

/*!*****************************************************************************
// Function		: KFont::Terminate
// Purpose		: 结束，清除操作
*****************************************************************************/
void KFont::Terminate()
{
	m_pDevice = 0;
	m_bLoaded = false;
	m_Resources.Terminate();
}

/*!*****************************************************************************
// Function		: KFont::Load
// Purpose		: 载入字库
// Return		: bool 是否成功
// Argumant		: cosnt char *pszFontFile 字库文件名
*****************************************************************************/
bool KFont::Load(const char* pszFontFile)
{
	m_Resources.Terminate();
	m_bLoaded = false;
	if (m_pDevice)
	{
		//初始化字体字库资源
		if (m_Resources.Load(pszFontFile))
		{
			m_Resources.GetInfo(m_nFontWidth, m_nFontHeight);
			m_bLoaded = true;
			return true;
		}
	}
	return false;
}

/*!*****************************************************************************
// Function		: KFont::TextOut
// Purpose		: 显示字符串
// Argumant		: cosnt char *pszText 字符串
// Argumant		: int nCount  字符串的长度(BYTE)，默认值为-1，表示此字符串是以'\0'结尾。
// Argumant		: int nX	  字符串显示起点坐标X，如果传入值为KF_FOLLOW，
//							  则此字符串紧接在上次字符串的输出位置之后。
// Argumant		: int nY      字符串显示起点坐标Y, 如果传入值为KF_FOLLOW，
//							  此字符串与前一次输出字符串在同一行的位置。
// Argumant		: uint nColor 字符串显示颜色，默认为黑色，用32bit数以ARGB的格
//							  式表示颜色，每个分量8bit。
// Argumant     : nLineWidth  自动换行的行宽限制，如果其值小于一个全角字符宽度
//							  则不做自动换行处理。默认值为0，既不做自动换行处理。
*****************************************************************************/
void KFont::TextOut(const char* pszText, int nCount/*= KF_ZERO_END*/,
					int nX/*=KF_FOLLOW*/, int nY/*=KF_FOLLOW*/,
					unsigned int nColor/*=0xff000000*/, int nLineWidth/*=0*/)
{
	if (!pszText || !m_bLoaded)
		return;

	unsigned char*	lpByte = (unsigned char*) pszText;
	int				nL, h;
	int				nPos = 0;

	if (nCount == KF_ZERO_END)
		nCount = strlen(pszText);
	if (nX == KF_FOLLOW)
		nX = m_nLastPosH;
	if (nY == KF_FOLLOW)
		nY = m_nLastPosV;

	h = 0;

	if (nLineWidth < m_nFontWidth + m_nFontWidth)
		nLineWidth = 0; //不做自动换行处理

	while (nPos < nCount)
	{
		//*********字符的判断与处理*********
		if (lpByte[nPos] > 0x80 && nPos + 1 < nCount)
		{
			DrawCharacter(nX + h, nY, lpByte[nPos], lpByte[nPos + 1]);
			nPos += 2;
			h += m_nFontWidth;
		}
		else	//西文字符
		{
			nL = lpByte[nPos++];
			if (nL > 0x20 && nL < 0x5F)
				DrawCharacter(nX + h, nY, 0xa3, (0x20 + nL));
			else if (nL >= 0x5F && nL < 0x7F)
				DrawCharacter(nX + h, nY, 0xa3, (0x21 + nL));
			
			if (nL != 0x0a)
				h += m_nFontWidth / 2;
			else
			{
				h = 0;
				nY += m_nFontHeight;
			}
		}
		if (nLineWidth && h > nLineWidth)
		{
			if (nPos < nCount && lpByte[nPos] == 0x0a)	//处理自动换行位置后恰好紧接一个换行符号
				nPos++;
			h = 0;
			nY += m_nFontHeight;
		}
	}
	m_nLastPosH = nX + h;
	m_nLastPosV = nY;
}

/*!*****************************************************************************
// Function		: KFont::TextOut
// Purpose		: 绘制单个字符
// Argumant		: unsigned char cFirst 字符编码的前个字节
// Argumant		: unsigned char cNext  字符编码的后个字节
*****************************************************************************/
void KFont::DrawCharacter(int x, int y, unsigned char cFirst, unsigned char cNext) const
{
	if (m_pDevice && m_bLoaded)
	{
		//取得字符在字库里的数据区指针
		m_Resources.GetCharacterData(cFirst, cNext);
//		unsigned char* pCharacterData = 
//		if (pCharacterData)
		{
			//to be finished
		}
	}
}

/*!*****************************************************************************
// Function		: KFont::TextOut
// Purpose		: 在Device由lost state恢复为operational state后KFont作Device相关内容的恢复操作。
*****************************************************************************/
bool KFont::RestoreDeviceObjects()
{
	return true;
}

/*!*****************************************************************************
// Function		: KFont::RestoreDeviceObjects
// Purpose		: 使Device状态切换会受影响的绘图设备相关内容失效
*****************************************************************************/
void KFont::InvalidateDeviceObjects()
{
}