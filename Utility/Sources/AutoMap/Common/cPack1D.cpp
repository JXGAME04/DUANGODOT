// cPack1D.cpp: implementation of the cPack1D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPack1D.h"

#include "planedoodad.h"
#include "iItem.h"
#include "fileop.h"

cPack1D::cPack1D() 
{
	m_ulCount = 1;

	m_pBuffer = NULL;
	m_pIndex = NULL;
	m_nNum = 0;
	m_nOffset = 0;
}

cPack1D::~cPack1D()
{
	delete m_pBuffer;
	delete m_pIndex;
	for (itTemplateMap it = m_mapTemplate.begin(); it != m_mapTemplate.end(); ++it)
		delete ((*it).second);
}

int cPack1D::GetIndex(LPCSTR szTxt)
{
	int index = 0;
	itStringMap it = m_mapString.find(szTxt);
	if (it != m_mapString.end())
	{
		return (*it).second;
	}
	
	return -1;
}

BOOL cPack1D::Open(LPCSTR szZip, LPCSTR szIndex,iZipRead* pZip)
{
	delete m_pBuffer;
	m_pBuffer = NULL;
	delete m_pIndex;
	m_pIndex = NULL;

	CString sFile,strWork;
	AppGetWorkingFolder(strWork);

	{
		CStdioFile f;
		sFile = strWork + szIndex;
		if (!f.Open(sFile,CFile::modeRead))
			return FALSE;
		CString s;
		int i=0;
		while(f.ReadString(s)) 
			m_mapString[LPCSTR(s)] = i++;
	}

	{
		CFile f;
		sFile = strWork + szZip;
		if (!f.Open(sFile,CFile::modeRead))
			return FALSE;
		f.Read(&m_nNum,sizeof(m_nNum));
		m_pIndex = new long[GetIndexLen()];
		f.Read(m_pIndex,sizeof(long) * GetIndexLen());		
		int m_nLength = f.GetLength() - sizeof(long) - sizeof(long) * GetIndexLen();
		m_pBuffer = new char[m_nLength];
		f.Read(m_pBuffer,m_nLength);
	}
	return TRUE;
}

void* cPack1D::Locate(LPCSTR szFile)
{
	int index = GetIndex(szFile);
	if (index == -1)
		return NULL;
	return Locate(index);
}

void* cPack1D::Locate(int index)
{
	long num = GetIndexLen();
	ASSERT(index < num);
	long offset = m_pIndex[index];
	return (void*) (m_pBuffer+offset);
}

const int MAX_TEMPLATE = 20 * 1024 * 1024;

void cPack1D::SetConvertNum(int n)
{
	m_pBuffer = new char[MAX_TEMPLATE];
	m_nNum = n;
	m_pIndex = new long[m_nNum];
	m_nOffset = 0;
	m_pOffset = (long*)m_pIndex;
}

BOOL cPack1D::Convert(void* buffer, ULONG len)
{
	*m_pOffset++ = m_nOffset;
	memcpy(m_pBuffer+m_nOffset,buffer,len);
	m_nOffset += len;
	return TRUE;
}

BOOL cPack1D::Save(LPCSTR szTemplate)
{
	CFile f;
	if (f.Open(szTemplate,CFile::modeCreate | CFile::modeWrite))
	{
		f.Write(&m_nNum,sizeof(m_nNum));
		f.Write(m_pIndex,sizeof(long)*GetIndexLen());
		f.Write(m_pBuffer,m_nOffset);
		return TRUE;
	}
	return FALSE;
}

void cPack1D::Refresh(LPCSTR szTemplate)
{
	itTemplateMap it = m_mapTemplate.find(szTemplate);
	if (it != m_mapTemplate.end())
	{
		delete ((*it).second);
		m_mapTemplate.erase(it);
	}
}

void* cPack1D::OpenTemplate(LPCSTR szTemplate)
{
	DWORD size;
	itTemplateMap it = m_mapTemplate.find(szTemplate);
	if (it != m_mapTemplate.end())
		return ((*it).second);
	char* pBuffer;
	if (LoadTemplate(szTemplate,&pBuffer,size))
	{
		m_mapTemplate[szTemplate] = pBuffer;
		return pBuffer;
	}
	return NULL;
}

void* cPack1D::LocateTo(LPCSTR szFile)
{
	void* pBuffer ;
	pBuffer = Locate(szFile);
	if (pBuffer == NULL)
		pBuffer = OpenTemplate(szFile);
	return pBuffer;
}

void* cPack1D::LocateToFrame(LPCSTR szFile,int frame) 
{
	void* pStart;
	void* pBuffer ;
	int frames;
	pBuffer = Locate(szFile);
	if (pBuffer == NULL)
		pBuffer = OpenTemplate(szFile);
	if (pBuffer == NULL)
		return NULL;

	long* pLong = (long*)pBuffer;
	frames = *pLong++;
	pStart = (void*)pLong;
	if (frames <= frame)
		return NULL;

	return Locate(pStart,frame);
}

BOOL cPack1D::Convert(LPCSTR szTemplate)
{
	DWORD size;
	char* pBuffer;
	if (LoadTemplate(szTemplate,&pBuffer,size))
		Convert(pBuffer,size);
	return TRUE;
}


#include "cPack1dplane.h"
#include "cPack1dAnimation.h"
#include "cPack1dNpc.h"
#include "cPack1dObj.h"

_declspec(dllexport) iPack1D* Common_GetPack1D(Pack1DEnum e)
{
	static cPack1DPlane s_plane;
	static cPack1DAnimation s_animation;

	switch (e)
	{
	case Pack1D_Plane:
		return &s_plane;
	case Pack1D_Animation:
		return &s_animation;
	}
	return NULL;
}

_declspec(dllexport) iPack1D* Common_CreatePack1D(Pack1DEnum e)
{
	switch (e)
	{
	case Pack1D_Plane:
		return new cPack1DPlane;
	case Pack1D_Animation:
		return new cPack1DAnimation;
	case Pack1D_Npc:
		return new cPack1DNpc;
	case Pack1D_Obj:
		return new cPack1DObj;
	}
	return NULL;
}

_declspec(dllexport) int Template_GetTerrainIndex(LPCSTR szTxt)
{
	static StringMap m_mapTerrainString;
	static bool m_bLoadTerrain = false;
	if (!m_bLoadTerrain)
	{
		CStdioFile f;
		CString sFile,strWork;
		AppGetWorkingFolder(strWork);
		sFile = strWork + FILE_TEMPLATE_TERRAININDEX;
		if (!f.Open(sFile,CFile::modeRead))
			return -1;
		CString s;
		int i=0;
		while(f.ReadString(s)) 
		{
			m_mapTerrainString[LPCSTR(s)] = i++;
		}
		m_bLoadTerrain = TRUE;
	}
	
	int index = 0;
	itStringMap it = m_mapTerrainString.find(szTxt);
	if (it != m_mapTerrainString.end())
	{
		return (*it).second;
	}
	return -1;
}
