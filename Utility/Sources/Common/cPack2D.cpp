#include "stdafx.h"
#include "cpack2d.h"

#include "ipublic.h"
#include "iZip.h"

cPack2D::cPack2D(void)
{
	m_ulCount = 1;
	m_pBuffer = NULL;
	m_pIndex = NULL;
}

cPack2D::~cPack2D(void)
{
	delete m_pIndex;
	delete m_pBuffer;
}

//在图形引擎中使用
BOOL cPack2D::Open(LPCSTR szTemplate, iZipRead* p )
{
	iZipRead* pFile = p;
	if (p == NULL)
		pFile = Zip_CreateZipRead(zip_default);
	else
		pFile->AddRef();

	wrap_ptr<iZipRead> ptr(pFile);
		
	delete m_pBuffer;
	m_pBuffer = NULL;
	delete m_pIndex;
	m_pIndex = NULL;

	if (pFile->Open(szTemplate))
	{
		pFile->Read((LPRECT)m_rect,sizeof(RECT));
		m_pIndex = new long[GetIndexLen()];
		pFile->Read(m_pIndex,sizeof(long) * GetIndexLen());
		int m_nLength = pFile->GetLength() - sizeof(RECT) - sizeof(long) * GetIndexLen();
		m_pBuffer = new char[m_nLength];
		pFile->Read(m_pBuffer,m_nLength);
		return TRUE;
	}

	return FALSE;
}

int cPack2D::GetIndexLen()
{
	int n = (m_rect.Width() + 1) * (m_rect.Height() + 1);
	return n;
}

BOOL cPack2D::GetRect(CRect& rc)
{
	rc = m_rect;
	return TRUE;
}

void* cPack2D::Locate(int x, int y)
{
	if (x < m_rect.left || x > m_rect.right || y < m_rect.top || y > m_rect.bottom )
		return NULL;
	
	int off = GetOffset(x,y);	
	long offset = m_pIndex[off];
	if (offset == -1)
		return NULL;
	return (void*) (m_pBuffer+offset);
}

//在转换工具使用
const int MAX_PACK2DSIZE = 40 * 1024 * 1024;
const int MAX_PACK2DREGION = 200;

void cPack2D::SetConvertRect(CRect rc)
{
	m_rect = rc;
	if (m_pBuffer == NULL)
		m_pBuffer = new char[MAX_PACK2DSIZE];
	if (m_pIndex == NULL)
	{
		int n = GetIndexLen();
		m_pIndex = new long[n];
		for (int i=0; i<n; i++)
			m_pIndex[i] = -1;
	}
	m_nOffset = 0;
}

int cPack2D::GetOffset(int x, int y)
{
	return (x-m_rect.left) * (m_rect.Height()+1) + y - m_rect.top;
}

BOOL cPack2D::Convert(int x, int y, void* buffer, ULONG len)
{
	int off = GetOffset(x,y);
	ASSERT(m_pIndex[off] == -1);
	m_pIndex[off] = m_nOffset;
	memcpy(m_pBuffer+m_nOffset,buffer,len);
	m_nOffset += len;
	return TRUE;
}

BOOL cPack2D::Save(LPCSTR szTemplate)
{
	CFile f;
	if (f.Open(szTemplate,CFile::modeCreate | CFile::modeWrite))
	{
		f.Write((LPRECT)m_rect,sizeof(RECT));
		f.Write(m_pIndex,sizeof(long) * GetIndexLen());
		f.Write(m_pBuffer,m_nOffset);
		return TRUE;
	}
	return FALSE;
}

_declspec(dllexport) iPack2D* Common_CreatePack2D()
{
	return new cPack2D;
}
