/*****************************************************************************************
//	读取打包文件
//	Copyright : Kingsoft 2003
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2003-9-16
*****************************************************************************************/
#include "KWin32.h"
#include "XPackFile.h"
#include "ucl/ucl.h"
#include <crtdbg.h>
#include "KDebug.h"
//#include "KDebug.h"

//一个Pack文件具有的头结构:
struct XPackFileHeader
{
	unsigned char cSignature[4];		//四个字节的文件的头标志，固定为字符串'PACK'
	unsigned int uCount;				//数据的条目数
	unsigned int uIndexTableOffset;	//索引的偏移量
	unsigned int uDataOffset;			//数据的偏移量
	unsigned int uCrc32;				//校验和
	unsigned char cReserved[12];		//保留的字节
};

#define	XPACKFILE_SIGNATURE_FLAG		0x4b434150	//'PACK'

//Pack中对应每个子文件的索引信息项
struct XPackIndexInfo
{
	unsigned int	uId;				//子文件id
	unsigned int	uOffset;			//子文件在包中的偏移位置
	int			lSize;				//子文件的原始大小
	int			lCompressSizeFlag;	//子文件压缩后的大小和压缩方法
										//最高字节表示压缩方法，见XPACK_METHOD
										//低的三个字节表示子文件压缩后的大小
};

//包文件的压缩方式
enum XPACK_METHOD
{
	TYPE_NONE	= 0x00000000,			//没有压缩
	TYPE_UCL	= 0x01000000,			//UCL压缩
	TYPE_BZIP2	= 0x02000000,			//bzip2压缩
	TYPE_FRAME	= 0x10000000,			//使用了独立帧压缩,子文件为spr类型时才可能用到
	TYPE_UCL_2ND	= 0x20000000,		//GUVE same as TYPE_UCL
	TYPE_METHOD_FILTER = 0x0f000000,	//过滤标记
	TYPE_FILTER = 0xff000000,			//过滤标记
};

//pak包中保存的spr帧信息项
struct XPackSprFrameInfo
{
	int lCompressSize;
	int lSize;
};

struct XPackSprFrameInfoEx
{
	unsigned int	uOffset;
	unsigned int	uSize;
	unsigned int	uCompressSizeFlag;
};

XPackFile::XPackElemFileCache	XPackFile::ms_ElemFileCache[MAX_XPACKFILE_CACHE];
int								XPackFile::ms_nNumElemFileCache = 0;

XPackFile::XPackFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_uFileSize = 0;
	m_pIndexList = NULL;
	m_nElemFileCount = 0;
	InitializeCriticalSection(&m_ReadCritical);
}

XPackFile::~XPackFile()
{
	Close();
	DeleteCriticalSection(&m_ReadCritical);
}

//-------------------------------------------------
//功能：打开包文件
//返回：成功与否
//-------------------------------------------------
bool XPackFile::Open(const char* pszPackFileName, int nSelfIndex)
{
	bool bResult = false;
	Close();
	EnterCriticalSection(&m_ReadCritical);
	m_nSelfIndex = nSelfIndex;
	m_hFile = ::CreateFile(pszPackFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	while (m_hFile != INVALID_HANDLE_VALUE)
	{
		m_uFileSize = ::GetFileSize(m_hFile, NULL);
		if (m_uFileSize == 0 || m_uFileSize == INVALID_FILE_SIZE ||
			m_uFileSize <= sizeof(XPackFileHeader))
		{
			break;
		}
		XPackFileHeader	Header;
		DWORD			dwListSize, dwReaded;
		//--读取包文件头--
		if (::ReadFile(m_hFile, &Header, sizeof(Header), &dwReaded, NULL) == FALSE)
			break;
		//--包文件标记与内容的合法性判断--
		if (dwReaded != sizeof(Header) ||
			*(int*)(&Header.cSignature) != XPACKFILE_SIGNATURE_FLAG ||
			Header.uCount == 0 ||
			Header.uIndexTableOffset < sizeof(XPackFileHeader) ||
			Header.uIndexTableOffset >= m_uFileSize ||
			Header.uDataOffset < sizeof(XPackFileHeader) ||
			Header.uDataOffset >= m_uFileSize)
		{
			break;
		}

		//--读取索引信息表--
		dwListSize = sizeof(XPackIndexInfo) * Header.uCount;
		m_pIndexList = (XPackIndexInfo*)malloc(dwListSize);
		if (m_pIndexList == NULL ||
			::SetFilePointer(m_hFile, Header.uIndexTableOffset, NULL, FILE_BEGIN) != Header.uIndexTableOffset)
		{
			break;
		}
		if (::ReadFile(m_hFile, m_pIndexList, dwListSize, &dwReaded, NULL) == FALSE)
			break;
		if (dwReaded != dwListSize)
			break;
		m_nElemFileCount = Header.uCount;
		bResult = true;
		break;
	};
	if (bResult == false)
		Close();
	LeaveCriticalSection(&m_ReadCritical);
	return bResult;
}

//-------------------------------------------------
//功能：关闭包文件
//-------------------------------------------------
void XPackFile::Close()
{
	EnterCriticalSection(&m_ReadCritical);

	if (m_pIndexList)
	{
		//----清除cache中缓存到的（可能）是此包中的子文件----
		for (int i = ms_nNumElemFileCache - 1; i >=0; i--)
		{
			if (ms_ElemFileCache[i].nPackIndex == m_nSelfIndex)
			{
				FreeElemCache(i);
				ms_nNumElemFileCache --;
				for (int j = i; j < ms_nNumElemFileCache; j++)
					ms_ElemFileCache[j] = ms_ElemFileCache[j + 1];
			}
		}
		free (m_pIndexList);
		m_pIndexList = NULL;
	}
	m_nElemFileCount = 0;

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	m_uFileSize = 0;

	LeaveCriticalSection(&m_ReadCritical);
}

//-------------------------------------------------
//功能：释放一个cache结点的内容
//返回：成功与否
//-------------------------------------------------
void XPackFile::FreeElemCache(int nCacheIndex)
{
	_ASSERT(nCacheIndex >= 0 && nCacheIndex < ms_nNumElemFileCache);
	if (ms_ElemFileCache[nCacheIndex].pBuffer)
	{
		free(ms_ElemFileCache[nCacheIndex].pBuffer);
		ms_ElemFileCache[nCacheIndex].pBuffer = NULL;
	}
	ms_ElemFileCache[nCacheIndex].uId = 0;
	ms_ElemFileCache[nCacheIndex].lSize = 0;
	ms_ElemFileCache[nCacheIndex].uRefFlag = 0;
	ms_ElemFileCache[nCacheIndex].nPackIndex = -1;
}

//-------------------------------------------------
//功能：直接读取包文件数据中的数据到缓冲区
//返回：成功与否
//-------------------------------------------------
bool XPackFile::DirectRead(void* pBuffer, unsigned int uOffset, unsigned int uSize) const
{
	bool bResult = false;
	DWORD dwReaded;
	//_ASSERT(pBuffer && m_hFile != INVALID_HANDLE_VALUE);
	if ((pBuffer && m_hFile != INVALID_HANDLE_VALUE) && (uOffset + uSize <= m_uFileSize) &&
		(::SetFilePointer(m_hFile, uOffset, 0, FILE_BEGIN) == uOffset))
	{
		if (ReadFile(m_hFile, pBuffer, uSize, &dwReaded, NULL))
		{
			if (dwReaded == uSize)
				bResult = true;
		}
	}
	return bResult;
}

//-------------------------------------------------
//功能：带解压地读取包文件到缓冲区
//参数：pBuffer --> 缓冲区指针
//		uExtractSize  --> 数据（期望）解压后的大小，pBuffer缓冲区的大小不小于此数
//		lCompressType --> 直接从包中度取得原始（/压缩）大小
//		uOffset  --> 从包中的此偏移位置开始读取
//		uSize    --> 从包中直接读取得（压缩）数据的大小
//返回：成功与否
//-------------------------------------------------
bool XPackFile::ExtractRead(void* pBuffer, unsigned int uExtractSize,
							long lCompressType, unsigned int uOffset, unsigned int uSize) const
{
	_ASSERT(pBuffer);
	bool bResult = false;
	if (lCompressType == TYPE_NONE)
	{
		if (uExtractSize == uSize)
			bResult = DirectRead(pBuffer, uOffset, uSize);
	}
	else
	{
		void*	pReadBuffer = malloc(uSize);
		if (pReadBuffer)
		{
		    if ((lCompressType == TYPE_UCL || lCompressType == TYPE_UCL_2ND) && DirectRead(pReadBuffer, uOffset, uSize))
			{
				unsigned int uDestLength;
				ucl_nrv2b_decompress_8((BYTE*)pReadBuffer, uSize, (BYTE*)pBuffer, &uDestLength, NULL);
				bResult =  (uDestLength == uExtractSize);
			}
			free (pReadBuffer);
			pReadBuffer = NULL;
		}
	}
	return bResult;
}

//-------------------------------------------------
//功能：在索引表中查找子文件项(二分法找)
//返回：如找到返回在索引表中的位置(>=0)，如未找到返回-1
//-------------------------------------------------
int XPackFile::FindElemFile(unsigned int ulId) const
{
	int nBegin, nEnd, nMid;
	nBegin = 0;
	nEnd = m_nElemFileCount - 1;
	while (nBegin <= nEnd)
	{
		nMid = (nBegin + nEnd) / 2;
		if (ulId < m_pIndexList[nMid].uId)
			nEnd = nMid - 1;
		else if (ulId > m_pIndexList[nMid].uId)
			nBegin = nMid + 1;
		else
			break;
	}
	return ((nBegin <= nEnd) ? nMid : -1);
}

//-------------------------------------------------
//功能：查找包内的子文件
//参数：uId --> 子文件的id
//		ElemRef -->如果找到则在此结构里填上子文件的相关信息
//返回：是否找到
//-------------------------------------------------
bool XPackFile::FindElemFile(unsigned int uId, XPackElemFileRef& ElemRef)
{
	ElemRef.nElemIndex = -1;
	if (uId)
	{
		EnterCriticalSection(&m_ReadCritical);
		ElemRef.nCacheIndex = FindElemFileInCache(uId, -1);
		if (ElemRef.nCacheIndex >= 0)
		{
			ElemRef.uId = uId;
			ElemRef.nPackIndex = ms_ElemFileCache[ElemRef.nCacheIndex].nPackIndex;
			ElemRef.nElemIndex = ms_ElemFileCache[ElemRef.nCacheIndex].nElemIndex;
			ElemRef.nSize = ms_ElemFileCache[ElemRef.nCacheIndex].lSize;
			ElemRef.nOffset = 0;
		}
		else
		{
			ElemRef.nElemIndex = FindElemFile(uId);
			if (ElemRef.nElemIndex >= 0)
			{
				ElemRef.uId = uId;
				ElemRef.nPackIndex = m_nSelfIndex;
				ElemRef.nOffset = 0;
				ElemRef.nSize = m_pIndexList[ElemRef.nElemIndex].lSize;
			}
		}
		LeaveCriticalSection(&m_ReadCritical);
	}
	return (ElemRef.nElemIndex >= 0);
}

//-------------------------------------------------
//功能：分配缓冲区，并读包内的子文件的内容到其中
//参数：子文件在包内的索引
//返回：成功则返回缓冲区的指针，否则返回空指针
//-------------------------------------------------
void* XPackFile::ReadElemFile(int nElemIndex) const
{
	_ASSERT(nElemIndex >= 0 && nElemIndex < m_nElemFileCount);
	void*	pDataBuffer = malloc(m_pIndexList[nElemIndex].lSize);
	if (pDataBuffer)
	{
		if (ExtractRead(pDataBuffer,
				m_pIndexList[nElemIndex].lSize,
				(m_pIndexList[nElemIndex].lCompressSizeFlag & TYPE_FILTER),
				m_pIndexList[nElemIndex].uOffset,
				(m_pIndexList[nElemIndex].lCompressSizeFlag & (~TYPE_FILTER))) == false)
		{
			free (pDataBuffer);
			pDataBuffer = NULL;
		}
	}
	return pDataBuffer;
}

//-------------------------------------------------
//功能：在cache里查找子文件
//参数：uId --> 子文件id
//		nDesireIndex --> 在cache中的可能位置
//返回：成功则返回cache节点索引(>=0),失败则返回-1
//-------------------------------------------------
int XPackFile::FindElemFileInCache(unsigned int uId, int nDesireIndex)
{
	if (nDesireIndex >= 0 && nDesireIndex < ms_nNumElemFileCache &&
		uId == ms_ElemFileCache[nDesireIndex].uId)
	{
		ms_ElemFileCache[nDesireIndex].uRefFlag = 0xffffffff;
		return nDesireIndex;
	}

	nDesireIndex = -1;
	for (int i = 0; i < ms_nNumElemFileCache; i++)
	{
		if (uId == ms_ElemFileCache[i].uId)
		{
			ms_ElemFileCache[i].uRefFlag = 0xffffffff;
			nDesireIndex = i;
			break;
		}
	}
	return nDesireIndex;
}

//-------------------------------------------------
//功能：把子文件数据添加到cache
//参数：pBuffer --> 存有子文件数据的缓冲区
//		nElemIndex --> 子文件在包中的索引
//返回：添加到cache的索引位置
//-------------------------------------------------
int XPackFile::AddElemFileToCache(void* pBuffer, int nElemIndex)
{
	_ASSERT(pBuffer && nElemIndex >= 0 && nElemIndex < m_nElemFileCount);
	int nCacheIndex;
	if (ms_nNumElemFileCache < MAX_XPACKFILE_CACHE)
	{	//找到一个空位置
		nCacheIndex = ms_nNumElemFileCache++;
	}
	else
	{	//释放一个旧的cache节点
		nCacheIndex = 0;
		if (ms_ElemFileCache[0].uRefFlag)
			ms_ElemFileCache[0].uRefFlag --;
		for (int i = 1; i < MAX_XPACKFILE_CACHE; i++)
		{
			if (ms_ElemFileCache[i].uRefFlag)
				ms_ElemFileCache[i].uRefFlag --;
			if (ms_ElemFileCache[i].uRefFlag < ms_ElemFileCache[nCacheIndex].uRefFlag)
				nCacheIndex = i;

		}
		FreeElemCache(nCacheIndex);
	}
	ms_ElemFileCache[nCacheIndex].pBuffer = pBuffer;
	ms_ElemFileCache[nCacheIndex].uId = m_pIndexList[nElemIndex].uId;
	ms_ElemFileCache[nCacheIndex].lSize = m_pIndexList[nElemIndex].lSize;
	ms_ElemFileCache[nCacheIndex].nPackIndex = m_nSelfIndex;
	ms_ElemFileCache[nCacheIndex].nElemIndex = nElemIndex;
	ms_ElemFileCache[nCacheIndex].uRefFlag = 0xffffffff;
	return nCacheIndex;
}

//-------------------------------------------------
//功能：读取子文件一定长度的数据到缓冲区
//参数：pBuffer --> 用来读取数据的缓冲区
//		uSize --> 要读取的数据的长度
//返回：成功读取得字节数
//-------------------------------------------------
int XPackFile::ElemFileRead(XPackElemFileRef& ElemRef, void* pBuffer, unsigned uSize)
{
	int nResult = 0;
	if (pBuffer && ElemRef.uId &&ElemRef.nElemIndex >= 0)
	{
		EnterCriticalSection(&m_ReadCritical);

		//--先看是否已经在cache里了---
		ElemRef.nCacheIndex = FindElemFileInCache(ElemRef.uId, ElemRef.nCacheIndex);

		if (ElemRef.nCacheIndex < 0 &&								//在cache里未找到
			ElemRef.nElemIndex < m_nElemFileCount &&
			m_pIndexList[ElemRef.nElemIndex].uId == ElemRef.uId)
		{
			void*	pDataBuffer = ReadElemFile(ElemRef.nElemIndex);
			if (pDataBuffer)
				ElemRef.nCacheIndex = AddElemFileToCache(pDataBuffer, ElemRef.nElemIndex);
		}

		if (ElemRef.nCacheIndex >= 0 &&
			//此下面三项应该展开检查，防止被模块外部改变，引起错误。
			//为效率可考虑省略，但需外部按照规则随便改变ElemRef的内容。
			ElemRef.nPackIndex == ms_ElemFileCache[ElemRef.nCacheIndex].nPackIndex &&
			ElemRef.nElemIndex == ms_ElemFileCache[ElemRef.nCacheIndex].nElemIndex &&
			ElemRef.nSize == ms_ElemFileCache[ElemRef.nCacheIndex].lSize
			)
		{
			//_ASSERT(ElemRef.nPackIndex == ms_ElemFileCache[ElemRef.nCacheIndex].nPackIndex);
			//_ASSERT(ElemRef.nElemIndex == ms_ElemFileCache[ElemRef.nCacheIndex].nElemIndex);
			//_ASSERT(ElemRef.nSize == ms_ElemFileCache[ElemRef.nCacheIndex].lSize);

			if (ElemRef.nOffset < 0)
				ElemRef.nOffset = 0;
			if (ElemRef.nOffset < ElemRef.nSize)
			{
				if (ElemRef.nOffset + (int)uSize <= ElemRef.nSize)
					nResult = uSize;
				else
					nResult = ElemRef.nSize - ElemRef.nOffset;
				memcpy(pBuffer, (char*)ms_ElemFileCache[ElemRef.nCacheIndex].pBuffer + ElemRef.nOffset, nResult);
				ElemRef.nOffset += nResult;
			}
			else
			{
				ElemRef.nOffset = ElemRef.nSize;
			}
		}
		LeaveCriticalSection(&m_ReadCritical);
	}
	return nResult;
}

#define	NODE_INDEX_STORE_IN_RESERVED	2

#define NEWZLIBDEF
#include "zlib.h"
static void CalcSpriteData2Byte(BYTE* pSrcData, int nWidth, int nHeight, UINT& uSize)
{
	uSize = 0;
	BYTE* pDesData = new BYTE[nWidth * nHeight * 3];
	if(!pDesData)
		return;
	BYTE* pDesPtr = pDesData;
	BYTE* pSrcPtr = pSrcData;
	int h = 0;
	int alpha = -1;
	UINT uPixelBatch;
	BYTE palIndex[256];
	while(h < nHeight)
	{
		int w = 0;
		while(w < nWidth)
		{
			if(alpha == -1)
			{
				palIndex[0] = *(pSrcPtr++);
				alpha = *(pSrcPtr++);
				if(alpha < 8)
					alpha = 0;
				else if(alpha >= 248)
					alpha = 255;
				uPixelBatch = 1;
				if(w + 1 >= nWidth)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							*(pDesPtr++) = palIndex[n];
						}
					}
					alpha = -1;
				}
			}
			else
			{
				BYTE pPIndex = *(pSrcPtr++);
				int a = *(pSrcPtr++);
				if(a < 8)
					a = 0;
				else if(a >= 248)
					a = 255;
				if (a != alpha)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							*(pDesPtr++) = palIndex[n];
						}
					}
					alpha = a;
					uPixelBatch = 1;
					palIndex[0] = pPIndex;
					if(w + 1 >= nWidth)
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								*(pDesPtr++) = palIndex[n];
							}
						}
						alpha = -1;
					}
				}
				else
				{
					palIndex[uPixelBatch++] = pPIndex;
					if((uPixelBatch == 255) || (w + 1 >= nWidth))
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								*(pDesPtr++) = palIndex[n];
							}
						}
						alpha = -1;
					}
				}
			}
			++w;
		}
		++h;
	}
	uSize = pDesPtr - pDesData;
	memcpy(pSrcData, pDesData, uSize);
	delete [] pDesData;
	pDesData = NULL;
	
}

SPRHEAD* XPackFile::GetSprHeader(XPackElemFileRef& ElemRef, SPROFFS*& pOffsetTable)
{
	SPRHEAD*		pSpr = NULL;
	bool			bOk = false;

	pOffsetTable = NULL;
	if (ElemRef.uId == 0 || ElemRef.nElemIndex < 0)
		return NULL;

	EnterCriticalSection(&m_ReadCritical);
	if(ElemRef.nElemIndex < m_nElemFileCount &&
		m_pIndexList[ElemRef.nElemIndex].uId == ElemRef.uId)
	{
        //首先检查这个id是什么类型压缩方式
		if ((m_pIndexList[ElemRef.nElemIndex].lCompressSizeFlag & TYPE_FRAME) == 0)
		{
			pSpr = (SPRHEAD*)ReadElemFile(ElemRef.nElemIndex);
			if (pSpr)
			{
				if ((*(int*)&pSpr->Comment[0]) == SPR_COMMENT_FLAG)
				{
					pOffsetTable = (SPROFFS*)(((char*)pSpr) + sizeof(SPRHEAD) + pSpr->Colors * 3);
					while(true)
					{
					SPRHEAD Header;
					memcpy(&Header, pSpr, sizeof(SPRHEAD));
					if(Header.Reserved[1] & 0x100)	//zip
					{
						UINT uNewSize = sizeof(SPRHEAD) + Header.Colors * 3 + sizeof(SPROFFS)*Header.Frames;
						WORD i;
						SPROFFS* pOffTab = (SPROFFS*)pOffsetTable;
						char* pPtr = (char*)(pOffTab + Header.Frames);
						for(i=0;i<Header.Frames; ++i)
						{
							UINT nPerSize = *((UINT*)(pPtr + (pOffTab+i)->Offset));
							uNewSize += nPerSize;
							//g_DebugLog("uNewSize [%u][%X][%u]",i,nPerSize, uNewSize);
						}
						char* pNewSpr = (char*)malloc(uNewSize);
						if(!pNewSpr)
							break;
						char* pRead = pNewSpr;
						Header.Reserved[1] = Header.Reserved[1] & 0xf;
						memcpy(pRead, &Header, sizeof(SPRHEAD));
						pRead += sizeof(SPRHEAD);
						memcpy(pRead, &pSpr[1], Header.Colors * 3 + sizeof(SPROFFS)*Header.Frames);
						pRead += Header.Colors * 3 + sizeof(SPROFFS)*Header.Frames;
						char* pFrStart = pRead;
						uNewSize = 0;
						for(i=0;i<Header.Frames; ++i)
						{
							unsigned long destLen = *((UINT*)pPtr);
							uncompress((Bytef *)pRead, &destLen, (Bytef *)(pPtr+4), pOffTab->Length-4);
							pPtr += pOffTab->Length;
							pOffTab->Offset = pRead - pFrStart;
							SPRFRAME * pF = (SPRFRAME*)pRead;
							pRead += 8;
							CalcSpriteData2Byte((BYTE*)pRead, pF->Width, pF->Height, uNewSize);
							if(!uNewSize)
								break;
							pRead += uNewSize;
							pOffTab->Length = uNewSize + 8;
							pOffTab++;
						}
						if(!uNewSize)
						{
							free(pNewSpr);
							pNewSpr = NULL;
							break;
						}
						pRead = pNewSpr + sizeof(SPRHEAD) + Header.Colors * 3;
						memcpy(pRead, pOffsetTable, sizeof(SPROFFS)*Header.Frames);
						free(pSpr); pSpr = NULL;
						pSpr = (SPRHEAD*)pNewSpr;
						pOffsetTable = (SPROFFS*)(((char*)(pSpr)) + sizeof(SPRHEAD) + Header.Colors * 3);
						
					}
					bOk = true;
					break;
					}
				}
			}
		}
		else
		{
			SPRHEAD Header;
			if (DirectRead(&Header, m_pIndexList[ElemRef.nElemIndex].uOffset, sizeof(SPRHEAD)))
			{
				UINT* p = (UINT*)&Header;
				if ((int)(*(p+2)) == SPR_COMMENT_FLAG)
				{
					unsigned int uReadSize = (*p)* sizeof(XPackSprFrameInfoEx);
					XPackSprFrameInfoEx* pFrInfoBuff = (XPackSprFrameInfoEx*)malloc(uReadSize);
					if (DirectRead(pFrInfoBuff,
						m_pIndexList[ElemRef.nElemIndex].uOffset + *(p+1),	uReadSize))
					{
						if(DirectRead(&Header, m_pIndexList[ElemRef.nElemIndex].uOffset + 8, sizeof(SPRHEAD)))
						{
						pSpr = (SPRHEAD*)malloc(sizeof(SPRHEAD) + Header.Colors * 3 + Header.Frames * sizeof(XPackSprFrameInfoEx));
						if(pSpr)
						{
							memcpy(pSpr, &Header, sizeof(SPRHEAD));
							*((UINT*)pSpr) = SPR_COMMENT_FLAG_TF;
							DirectRead(&pSpr[1], m_pIndexList[ElemRef.nElemIndex].uOffset + 8 + sizeof(SPRHEAD), Header.Colors * 3);
							XPackSprFrameInfoEx* pFrameInfo = (XPackSprFrameInfoEx*)((BYTE*)pSpr + sizeof(SPRHEAD) + Header.Colors * 3);
							memcpy(pFrameInfo, pFrInfoBuff+2, Header.Frames*sizeof(XPackSprFrameInfoEx));
							bOk = true;
						}}
					}
					if(pFrInfoBuff)
					{
						free(pFrInfoBuff);
						pFrInfoBuff = NULL;
					}
				}
				else if (*(int*)&(Header.Comment[0]) == SPR_COMMENT_FLAG)
				{
					unsigned int	u2ListSize = Header.Colors * 3 + Header.Frames * sizeof(XPackSprFrameInfo);
					pSpr = (SPRHEAD*)malloc(sizeof(SPRHEAD) + u2ListSize);
					if (pSpr)
					{
						if (DirectRead((char*)(&pSpr[1]),
							m_pIndexList[ElemRef.nElemIndex].uOffset + sizeof(SPRHEAD),	u2ListSize))
						{
							memcpy(pSpr, &Header, sizeof(SPRHEAD));
							bOk = true;
						}
					}
				}
			}
		}

		if (pSpr)
		{
			if (bOk)
			{
				*((int*)&pSpr->Reserved[NODE_INDEX_STORE_IN_RESERVED]) = ElemRef.nElemIndex;
			}
			else
			{
				free (pSpr);
				pSpr = NULL;
			}
		}
	}
	LeaveCriticalSection(&m_ReadCritical);
    return pSpr;
}

SPRFRAME* XPackFile::GetSprFrame(SPRHEAD* pSprHeader, int nFrame)
{
	SPRFRAME*	pFrame = NULL;
	if (pSprHeader && nFrame >= 0 && nFrame < pSprHeader->Frames)
	{
		EnterCriticalSection(&m_ReadCritical);
		int nNodeIndex = *((int*)&pSprHeader->Reserved[NODE_INDEX_STORE_IN_RESERVED]);
		if (nNodeIndex >= 0 && nNodeIndex < m_nElemFileCount)
		{
			long lCompressType = m_pIndexList[nNodeIndex].lCompressSizeFlag;
			if ((lCompressType & TYPE_FRAME) != 0)
			{
				bool bOk = false;
				if(*((UINT*)pSprHeader) == SPR_COMMENT_FLAG_TF)
				{
					XPackSprFrameInfoEx* pFrameList = (XPackSprFrameInfoEx *)((char*)pSprHeader + sizeof(SPRHEAD) + pSprHeader->Colors * 3);
					
					if (pFrame = (SPRFRAME*)malloc(pFrameList[nFrame].uSize))
					{
						bOk = ExtractRead(pFrame, pFrameList[nFrame].uSize,
						(pFrameList[nFrame].uCompressSizeFlag & TYPE_FILTER), 
						m_pIndexList[nNodeIndex].uOffset + pFrameList[nFrame].uOffset,
						(pFrameList[nFrame].uCompressSizeFlag & (~TYPE_FILTER)));
					}
				}
				else
				{
					lCompressType &= TYPE_METHOD_FILTER;
					long	lTempValue = sizeof(SPRHEAD) + pSprHeader->Colors * 3;
					//读出指定帧的信息
					XPackSprFrameInfo* pFrameList = (XPackSprFrameInfo *)((char*)pSprHeader + lTempValue);
					unsigned long	uSrcOffset = m_pIndexList[nNodeIndex].uOffset + lTempValue +pSprHeader->Frames * sizeof(XPackSprFrameInfo);
					while(nFrame > 0)
					{
						uSrcOffset += (pFrameList++)->lCompressSize;
						nFrame--;
					};
					lTempValue = pFrameList->lSize;
					if (lTempValue < 0)
					{
						lTempValue = -lTempValue;
						if (pFrame = (SPRFRAME*)malloc(lTempValue))
							bOk = DirectRead(pFrame, uSrcOffset, lTempValue);
					}
					else
					{
						if (pFrame = (SPRFRAME*)malloc(lTempValue))
						{
							bOk = ExtractRead(pFrame, lTempValue, lCompressType, uSrcOffset, pFrameList->lCompressSize);
						}
					}
				}
				if (bOk == false && pFrame != NULL)
				{
					free(pFrame);
					pFrame = NULL;
				}
			}
		}
		LeaveCriticalSection(&m_ReadCritical);
	}
	return pFrame;	
}

bool XPackFile::IsTypeFrame(XPackElemFileRef& ElemRef)
{
	if (ElemRef.uId == 0 || ElemRef.nElemIndex < 0 || ElemRef.nElemIndex >= m_nElemFileCount)
		return false;
	if(m_pIndexList[ElemRef.nElemIndex].uId == ElemRef.uId)
	{
		if(m_pIndexList[ElemRef.nElemIndex].lCompressSizeFlag & TYPE_FRAME)
			return true;
	}
	return false;
}

char*  XPackFile::ReadTypeFrame(XPackElemFileRef& ElemRef, unsigned int& uSize)
{
	if (ElemRef.uId == 0 || ElemRef.nElemIndex < 0 || ElemRef.nElemIndex >= m_nElemFileCount)
		return NULL;
	if(m_pIndexList[ElemRef.nElemIndex].uId == ElemRef.uId)
	{
		int lCompressType = m_pIndexList[ElemRef.nElemIndex].lCompressSizeFlag;
		if(!(lCompressType & TYPE_FRAME))
			return NULL;
		SPRHEAD Header;
		if (DirectRead(&Header, m_pIndexList[ElemRef.nElemIndex].uOffset, sizeof(SPRHEAD)))
		{
			UINT* p = (UINT*)&Header;
			if ((int)(*(p+2)) == SPR_COMMENT_FLAG)
			{
				//unsigned int uReadSize = (*p)* sizeof(XPackSprFrameInfoEx);
				unsigned int uFrameTabOffset = *(p+1);
				if(DirectRead(&Header, m_pIndexList[ElemRef.nElemIndex].uOffset + 8, sizeof(SPRHEAD)))
				{
					BYTE* pNextData = (BYTE*)malloc(Header.Colors * 3 + Header.Frames * sizeof(XPackSprFrameInfoEx));
					if(!pNextData)
						return NULL;
					DirectRead(pNextData, m_pIndexList[ElemRef.nElemIndex].uOffset + 8 + sizeof(SPRHEAD), Header.Colors * 3);
					XPackSprFrameInfoEx* pFrameInfo = (XPackSprFrameInfoEx*)(pNextData + Header.Colors * 3);
					DirectRead(pFrameInfo, m_pIndexList[ElemRef.nElemIndex].uOffset + uFrameTabOffset
					+ sizeof(XPackSprFrameInfoEx) * 2, Header.Frames*sizeof(XPackSprFrameInfoEx));
					UINT uTotalSize = sizeof(SPRHEAD) + Header.Colors * 3 + Header.Frames * sizeof(XPackSprFrameInfoEx);
					WORD i;
					for(i = 0; i < Header.Frames; ++i)
					{
						uTotalSize += (pFrameInfo+i)->uSize;
					}
					uSize = uTotalSize;
					char* pNewFile = (char*)malloc(uTotalSize);
					if(!pNewFile)
					{
						free(pNextData);
						pNextData = NULL;
						return NULL;
					}
					char* pReadBuf = pNewFile;
					memcpy(pReadBuf, &Header, sizeof(SPRHEAD));
					pReadBuf += sizeof(SPRHEAD);
					memcpy(pReadBuf, pNextData, Header.Colors * 3);
					pReadBuf += Header.Colors * 3;
					SPROFFS* pOffsTable = (SPROFFS*)pReadBuf;
					UINT nOffSet = 0;
					for(i = 0; i < Header.Frames; ++i)
					{
						pOffsTable->Offset = nOffSet;
						pOffsTable->Length = (pFrameInfo+i)->uSize;
						nOffSet += pOffsTable->Length;
						pOffsTable++;
					}
					pReadBuf += Header.Frames * sizeof(SPROFFS);
					for(i = 0; i < Header.Frames; ++i)
					{
						ExtractRead(pReadBuf, (pFrameInfo+i)->uSize,
						((pFrameInfo+i)->uCompressSizeFlag & TYPE_FILTER), 
						m_pIndexList[ElemRef.nElemIndex].uOffset + (pFrameInfo+i)->uOffset,
						((pFrameInfo+i)->uCompressSizeFlag & (~TYPE_FILTER)));
						pReadBuf += (pFrameInfo+i)->uSize;
					}
					free(pNextData);
					pNextData = NULL;
					return pNewFile;
				}
			}
			else
			{
			UINT uTotalSize = Header.Frames * sizeof(XPackSprFrameInfo);
			XPackSprFrameInfo* pFrameList = (XPackSprFrameInfo*)malloc(uTotalSize);
			if(!pFrameList)
				return NULL;
			if (DirectRead(pFrameList, m_pIndexList[ElemRef.nElemIndex].uOffset + sizeof(SPRHEAD)
				+ Header.Colors * 3, uTotalSize))
			{
				uTotalSize += sizeof(SPRHEAD) + Header.Colors * 3;
				WORD i;
				for(i = 0; i < Header.Frames; ++i)
				{
					int nFrameSize = (pFrameList+i)->lSize;
					if(nFrameSize < 0)
						nFrameSize = -nFrameSize;
					uTotalSize += nFrameSize;
				}
				uSize = uTotalSize;
				char* pNewFile = (char*)malloc(uTotalSize);
				if(!pNewFile)
				{
					free(pFrameList);
					pFrameList = NULL;
					return NULL;
				}
				char* pReadBuf = pNewFile;
				memcpy(pReadBuf, &Header, sizeof(SPRHEAD));
				pReadBuf += sizeof(SPRHEAD);
				DirectRead(pReadBuf, m_pIndexList[ElemRef.nElemIndex].uOffset + sizeof(SPRHEAD),
							Header.Colors * 3);
				pReadBuf += Header.Colors * 3;
				SPROFFS* pOffsTable = (SPROFFS*)pReadBuf;
				UINT nOffSet = 0;
				for(i = 0; i < Header.Frames; ++i)
				{
					pOffsTable->Offset = nOffSet;
					int nFrameSize = (pFrameList+i)->lSize;
					if(nFrameSize < 0)
						nFrameSize = -nFrameSize;
					pOffsTable->Length = nFrameSize;
					nOffSet += nFrameSize;
					pOffsTable++;
				}
				pReadBuf += Header.Frames * sizeof(SPROFFS);
				nOffSet = m_pIndexList[ElemRef.nElemIndex].uOffset + sizeof(SPRHEAD) + Header.Colors * 3
							+ Header.Frames * sizeof(XPackSprFrameInfo);
				lCompressType &= TYPE_METHOD_FILTER;
				for(i = 0; i < Header.Frames; ++i)
				{
					int nFrameSize = (pFrameList+i)->lSize;
					if(nFrameSize < 0)
					{
						nFrameSize = -nFrameSize;
						DirectRead(pReadBuf, nOffSet, nFrameSize);
					}
					else
					{
						ExtractRead(pReadBuf, nFrameSize, lCompressType, nOffSet, (pFrameList+i)->lCompressSize);
					}
					pReadBuf += nFrameSize;
					nOffSet += (pFrameList+i)->lCompressSize;
				}
				free(pFrameList);
				pFrameList = NULL;
				return pNewFile;
			}}
		}
	}
	return NULL;
}
