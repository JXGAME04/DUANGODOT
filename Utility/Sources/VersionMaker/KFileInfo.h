
#ifndef _KFILEINFO_H_
#define _KFILEINFO_H_


using namespace std;

class KFileInfo
{
public:
	string	m_szFileName;
	string	m_szFileCreateTime;

	KFileInfo() : m_szFileName(""), m_szFileCreateTime("") {}
    KFileInfo(string szFileName, string szFileCreateTime) : m_szFileName(szFileName), 
															m_szFileCreateTime(szFileCreateTime) {}

};

inline bool operator == (const KFileInfo& x, const KFileInfo& y)
{
    return (x.m_szFileName == y.m_szFileName);
}

inline bool operator < (const KFileInfo& x, const KFileInfo& y)
{
   return (x.m_szFileName < y.m_szFileName);
}


typedef vector<KFileInfo> KFileInfoVector;

#endif