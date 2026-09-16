//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KPakList.h
// Date:	2000.08.08
// Code:	WangWei(Daphnis)
// Desc:	Header File
// Modify:	Wooy(2003-9-17)
//---------------------------------------------------------------------------
#ifndef KPakList_H
#define KPakList_H

#include "XPackFile.h"
//---------------------------------------------------------------------------


class ENGINE_API KPakList
{
public:
	KPakList();
	~KPakList();
	bool		Open(const char* pPakListFile);
	void		Close();
	//查找包内的子文件
	bool		FindElemFile(unsigned int uId, XPackElemFileRef& ElemRef);
	//查找包内的子文件
	bool		FindElemFile(const char* pszFileName, XPackElemFileRef& ElemRef);
	//读取包内的子文件
	int			ElemFileRead(XPackElemFileRef& ElemRef, void* pBuffer, unsigned uSize);
	//读取spr文件头部或整个spr
	SPRHEAD*	GetSprHeader(XPackElemFileRef& ElemRef, SPROFFS*& pOffsetTable);
	//读取按帧压缩的spr的一帧的数据
	SPRFRAME*	GetSprFrame(int nPackIndex, SPRHEAD* pSprHeader, int nFrame);
	bool		FindElemFileEx(const char* pszFileName, XPackElemFileRef& ElemRef, int nExNum = -1,
				unsigned int byuId = 0);
	bool		IsTypeFrame(XPackElemFileRef& ElemRef);
	char*		ReadTypeFrame(XPackElemFileRef& ElemRef, unsigned int& uSize);
private:
	unsigned int FileNameToId(const char* pszFileName);
private:
	#define MAX_PAK		50
	XPackFile*			m_PakFilePtrList[MAX_PAK];
	int				m_nPakNumber;
};

extern ENGINE_API KPakList* g_pPakList;

#endif
