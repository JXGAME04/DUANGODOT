#pragma once
#include "iPack2D.h"

class iZipRead;

class cPack2D :
	public iPack2D
{
public:
	cPack2D(void);
	virtual ~cPack2D(void);

	ULONG m_ulCount;
	ULONG AddRef() { return ++m_ulCount; };
	ULONG Release() { int n = --m_ulCount; if (n == 0) delete this; return n;};
	int GetOffset(int x, int y);
//在图形引擎中使用
	virtual BOOL Open(LPCSTR szTemplate,iZipRead* p);
	virtual BOOL GetRect(CRect& rc) ;
	virtual void* Locate(int x, int y) ;
//在转换工具使用
	virtual void SetConvertRect(CRect rc) ;
	virtual BOOL Convert(int x,int y, void* buffer, ULONG len) ;
	virtual int GetIndexLen();
	virtual BOOL Save(LPCSTR szTemplate) ;

	CRect m_rect;
	//索引
	long* m_pIndex;
	char* m_pBuffer;
	int m_nOffset;
};
