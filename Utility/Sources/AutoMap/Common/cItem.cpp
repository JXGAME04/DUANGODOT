// cItem.cpp: implementation of the cItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cItem.h"

#include "iCommon.h"

#include "fileop.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cItem::cItem() 
{
	m_ulCount = 1;
	m_bModify = false;
	m_pMain = NULL;
	m_lFlag = 0;
	m_aSection.resize(64);
}

cItem::~cItem()
{
	if (m_bModify)
		Save();
	ClearAll();
}

void cItem::SetFlag(long lFlag)
{
	m_lFlag |= lFlag;
}

BOOL cItem::IsLoadFile(LPCSTR szFile)
{
	return m_strFile.CompareNoCase(szFile) == 0;
}

//////////////////////////////////////////////////////////////////////
//com interface
//////////////////////////////////////////////////////////////////////

ULONG cItem::AddRef() 
{
	return ++m_ulCount;
}

ULONG cItem::Release() 
{
	int count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

/////////////////////////////////////////////////////////////////////////////
//section
/////////////////////////////////////////////////////////////////////////////

BOOL cItem::IsMain(LPCSTR szSection)
{
	return stricmp(szSection,"MAIN") == 0;
}

stSection* cItem::GetMapSection(LPCSTR szSection,BOOL bRead)
{
	itSectionMap it = m_mapSection.find(szSection);
	if (it == m_mapSection.end())
	{
		if (bRead)
			return NULL;
		stcSection* p = new stcSection;
		m_mapSection[szSection] = p;
		return p;
	}
	return (*it).second;
}

stSection* cItem::GetMainSection(BOOL bRead)
{
	if (!bRead && m_pMain == NULL)
		m_pMain = new stcSection;
	return m_pMain;	
}

void cItem::AddIndexSection(stcSection* p,int index)
{
	if (index >= m_aSection.size())
	{
		int size = max(index+1, (int)(m_aSection.size()*1.5f));
		m_aSection.resize(size,NULL);
	}
	stcSection* pSection = new stcSection;
	*pSection = *p;
	m_aSection[index] = pSection;
}

stSection* cItem::GetIndexSection(unsigned int index, BOOL bRead)
{
	if (bRead)
	{
		if (index < m_aSection.size())
		{
			stcSection* p = m_aSection[index];
			if (p != NULL)
				return p;
		}
		return NULL;
	}

	if (index >= m_aSection.size())
	{
		int size = max(index+1, (int)(m_aSection.size()*1.5f));
		m_aSection.resize(size,NULL);
	}
	stcSection* p = m_aSection[index];
	if (p == NULL)
		p = new stcSection;
	m_aSection[index] = p;
	return p;
}

stSection* cItem::GetSection(LPCSTR szSection, BOOL bRead)
{
	if (IsMain(szSection))
		return GetMainSection(bRead);
	return GetMapSection(szSection,bRead);
}

/////////////////////////////////////////////////////////////////////////////
//file
/////////////////////////////////////////////////////////////////////////////

BOOL cItem::DeleteFile()
{
	ClearAll();
	if (!m_strFile.IsEmpty())
		AppDeleteFolder(m_strFile);
	m_strFile = "";
	m_bModify = FALSE;
	return TRUE;
}

BOOL cItem::Refresh() 
{
	CString s = m_strFile;
	m_strFile.Empty();
	return Load(s);
}

/////////////////////////////////////////////////////////////////////////////
//save
/////////////////////////////////////////////////////////////////////////////

BOOL cItem::SaveModify()
{
	if (m_bModify)
		Save();
	return TRUE;
}

BOOL cItem::SaveModify(LPCSTR szNewFile)
{
	CString s = szNewFile;
	if (s == m_strFile)
		return FALSE;
	if (m_bModify)
		Save();
	return TRUE;
}

BOOL cItem::SaveSection(std::ofstream& of, stcSection& section)
{
	for (itSection it = section.begin(); it!= section.end(); ++it)
		of << (*it).first.c_str() << "=" << (*it).second.c_str() << "\n";
	return TRUE;
}

BOOL cItem::Save()
{
	if (m_strFile.IsEmpty())
		return FALSE;

	CString sFullPath = m_strFile;
	AppGetFullPath(sFullPath);
	AppBuildPath(sFullPath);

	ofstream of;
	of.exceptions(ios::failbit);
	try 
	{
		of.open(sFullPath);
	}
	catch (ios_base::failure f) 
	{
		CString s = f.what();
		s += " 请去掉文件""" + m_strFile + """的只读属性或者释放磁盘空间。";
		AfxMessageBox(s);
		return FALSE;
	}

	if (m_pMain)
	{
		of << "[MAIN]\n";
		SaveSection(of,*m_pMain);
	}

	for (itSectionMap itS = m_mapSection.begin(); itS != m_mapSection.end(); ++itS)
	{
		of << "[" << (*itS).first.c_str() << "]\n";
		SaveSection(of,*((*itS).second));
	}

	for (unsigned int i=0; i< m_aSection.size(); i++)
	{
		if ((m_aSection[i]) == NULL)
			continue;
		of << "[" << i << "]\n";
		SaveSection(of,*m_aSection[i]);
	}
/*
	for (itSectionArray it = m_aSection.begin(); it != m_aSection.end(); ++it)
	{
		if ((*it) == NULL)
			continue;
		of << "[" << i++ << "]\n";
		SaveSection(of,*(*it));
	}
*/
	m_bModify = false;

	return TRUE;	
}

BOOL cItem::Save(LPCSTR szFile)
{
	m_strFile = szFile;
	return Save();
}

/////////////////////////////////////////////////////////////////////////////
//load
/////////////////////////////////////////////////////////////////////////////
BOOL cItem::Reload(LPCSTR szFile)
{
	m_strFile.Empty();
	return Load(szFile);
}

BOOL cItem::Load(LPCSTR szFile) 
{
	if (IsLoadFile(szFile)) //no need load
		return TRUE;
/*
	CString strFile = szFile;
	CString sExt = strFile.Right(3);
	sExt.MakeLower();
	if ( sExt != "txt" &&  sExt != "ini" && sExt != "wor")
	{
		CString strErr;
		strErr = "file " + strFile + " is not a 'txt' or 'ini' file";
		AfxMessageBox(strErr);
		return FALSE;
	}
*/	
	m_strFile = szFile;
	ClearAll();
	m_bModify = false;

	CStdioFile file;
	if (!file.Open(szFile,CFile::modeRead))
		return FALSE;

	CString s;
	bool bContinue = false;
	do
	{
		bool bEnd = false;
		while(s.IsEmpty())
		{
			if (!file.ReadString(s))
			{	
				bEnd = true;
				break;
			}
		}
		if (bEnd == true)
			break;
		
		stcSection& section = *(new stcSection);
		CString strSectionName;
		std::string sSectionName;

		BOOL blist = 0;//2: main 0: index 1: map

		int pos1 = s.Find('[');
		int pos2 = s.Find(']');
		if (pos1 == -1 && pos2 == -1)
		{
			strSectionName = "MAIN";
			sSectionName = strSectionName;
			blist = 2;
		}
		else if (pos1 > pos2)
		{
			return FALSE;
		}
		else 
		{
			ASSERT(pos1 != -1 && pos2 != -1);
			strSectionName = s.Mid(pos1+1,pos2-pos1-1);
			sSectionName = strSectionName;
			if (strSectionName == "MAIN")
				blist = 2;
			else if (strSectionName.GetAt(0) >= '0' && strSectionName.GetAt(0) <= '9')
				blist = 0;
			else
				blist = 1;
			file.ReadString(s);
		}
		bContinue = false;

		BOOL b = true;
		while(b)
		{
			while(s.IsEmpty() && b)
			{
				b = file.ReadString(s);
			}
			if (s.IsEmpty())
				break;

			int pos1 = s.Find('['); 
			int pos2 = s.Find(']');
			if (pos1 != -1 && pos2 != -1)
			{
				bContinue = true;
				break;
			}
			int pos = s.Find('=');
			if (pos == -1)
			{
				s.Empty();
				continue;
			}
			CString strleft = s.Left(pos);
			strleft.TrimRight();
			std::string sleft = strleft;
			CString strright = s.Mid(pos + 1);
			strright.TrimLeft();
			std::string sright = strright;
			section[sleft] = sright;

			b = file.ReadString(s);
		}
		if (blist == 1)
			m_mapSection[sSectionName] = &section;
		else if (blist == 0)
		{
			int index = atoi(strSectionName);
			if (index >= m_aSection.size())
			{
				int size = max(index+1, m_aSection.size()*2);
				m_aSection.resize(size,NULL);
			}
			m_aSection[index] = &section;
		}
		else if (blist == 2)
			m_pMain = &section;
	}while(bContinue);

//	CString sValue;
//	BOOL b;
//	b = GetItemValue(this,"want",sValue);

	return TRUE;	
}

/////////////////////////////////////////////////////////////////////////////
//set and get
/////////////////////////////////////////////////////////////////////////////
BOOL cItem::GetMainValue(LPCSTR szItem,CString& strValue)
{
	if (m_pMain == NULL)
		return FALSE;
	return GetValue(szItem,strValue,*m_pMain);
}

BOOL cItem::SetMainValue(LPCSTR szItem,const CString& strValue)
{
	if (m_pMain == NULL)
		m_pMain = new stcSection;
	return SetValue(szItem,strValue,*m_pMain);
}

BOOL cItem::SetValue(LPCSTR szItem,const CString& strValue,stcSection& section)
{
	m_bModify = true;
	std::string sValue = strValue;
	section[szItem] = sValue;
	return TRUE;
}

BOOL cItem::GetValue(LPCSTR szItem, CString& strValue,stcSection& section)
{
	itSection it = section.find(szItem);
	if (it == section.end())
		return FALSE;
	strValue = (*it).second.c_str();
	return TRUE;
}

BOOL cItem::SetValue(LPCSTR szItem,const CString& strValue, unsigned int index)
{
	if (index >= m_aSection.max_size())
		return FALSE;
	if (index >= m_aSection.size())
	{
		int size = max(index+1, m_aSection.size()*2);
		m_aSection.resize(size,NULL);
	}
	stcSection* p = m_aSection[index];
	if (p == NULL)
		p = new stcSection;
	m_aSection[index] = p;
	return SetValue(szItem,strValue,*p);
}

BOOL cItem::GetValue(LPCSTR szItem,CString& strValue,unsigned int index)
{
	BOOL b = FALSE;
	if (index < m_aSection.size())
	{
		stcSection* p = m_aSection[index];
		if (p != NULL)
			b = GetValue(szItem,strValue,*p);
	}
	if (b == FALSE)
		return GetMainValue(szItem,strValue);
	return TRUE;
}

BOOL cItem::SetValue(LPCSTR szItem,const CString& strValue,LPCSTR szSection)
{
	if (strcmp(szSection,"MAIN") == 0)
		return SetMainValue(szItem,strValue);

	itSectionMap it = m_mapSection.find(szSection);
	stcSection* p;
	if (it == m_mapSection.end())
	{
		p = new stcSection;
		m_mapSection[szSection] = p;
	}
	else
		p = (*it).second;
	SetValue(szItem,strValue,*p);
	return TRUE;	
}

BOOL cItem::GetValue(LPCSTR szItem,CString& strValue,LPCSTR szSection) 
{
	if (strcmp(szSection,"MAIN") == 0)
		return GetMainValue(szItem,strValue);

	itSectionMap it = m_mapSection.find(szItem);
	if (it == m_mapSection.end())
		return FALSE;
	return GetValue(szItem,strValue,*((*it).second));
}

/////////////////////////////////////////////////////////////////////////////
//clear
/////////////////////////////////////////////////////////////////////////////
BOOL ClearSectionKey(stcSection& section, LPCSTR szKey)
{
	itSection it = section.find(szKey);
	if (it == section.end())
		return FALSE;
	section.erase(it);
	return TRUE;
}

BOOL cItem::ClearKey(LPCSTR szItem)
{
	m_bModify = true;
	for (itSectionMap itS = m_mapSection.begin(); itS != m_mapSection.end(); ++itS)
		ClearSectionKey(*((*itS).second),szItem);
	for (unsigned int i=0; i<m_aSection.size(); i++)
//	for (itSectionArray it = m_aSection.begin(); it != m_aSection.end(); ++ it)
	{
		if (m_aSection[i] != NULL)
//		if (*it != NULL)
			ClearSectionKey(*m_aSection[i],szItem);
	}
	if (m_pMain)
		ClearSectionKey((*m_pMain),szItem);
	return  TRUE;
}

BOOL cItem::ClearAllButMain()
{
	m_bModify = true;
	stcSection* p = m_pMain;
	m_pMain = NULL;
	ClearAll();
	m_pMain = p;
	return TRUE;
}

BOOL cItem::ClearAll()
{
	m_bModify = true;
	for (itSectionMap itS = m_mapSection.begin(); itS != m_mapSection.end(); ++itS)
		delete (*itS).second;
	m_mapSection.clear();

	for (unsigned int i=0; i<m_aSection.size(); i++)
	{
		if (m_aSection[i] != NULL)
			delete m_aSection[i];
	}
	m_aSection.clear();
/*
	for (itSectionArray it = m_aSection.begin(); it != m_aSection.end(); ++ it)
	{
		if ((*it) != NULL)
			delete (*it);
	}
*/
	if (m_pMain)
	{
		delete m_pMain;
		m_pMain = NULL;
	}
	return  TRUE;
}

//section
BOOL GetcSectionString(const stcSection* p,LPCSTR sKey, CString& str)
{
	c_itSection it = p->find(sKey);
	if (it != p->end())
	{
		str = (*it).second.c_str();
		return TRUE;
	}
	return FALSE;
}

BOOL SetcSectionString(stcSection* p,LPCSTR sKey, const CString& str)
{
	std::string s = (LPCSTR)str;
	(*p)[sKey] = s;
	return TRUE;
}


_declspec(dllexport) BOOL GetSectionString(const stSection* p,LPCSTR sKey, CString& str)
{
	return GetcSectionString((stcSection*) p,sKey,str);
}

_declspec(dllexport) BOOL SetSectionString(stSection* p,LPCSTR sKey, const CString& str)
{
	return SetcSectionString((stcSection*) p,sKey,str);
}
