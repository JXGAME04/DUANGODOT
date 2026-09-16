// SourceDest.h: interface for the SourceDest class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOURCEDEST_H__A81DBE2F_109F_4D8F_8790_4D7257AFE6D0__INCLUDED_)
#define AFX_SOURCEDEST_H__A81DBE2F_109F_4D8F_8790_4D7257AFE6D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>

using namespace std;

class SourceDest  
{
public:
	string GetDest();
	string GetSource();
	SourceDest(const char * Source,const char * Dest);
	virtual ~SourceDest();

private:
	string m_sDest;
	string m_sSource;
};

#endif // !defined(AFX_SOURCEDEST_H__A81DBE2F_109F_4D8F_8790_4D7257AFE6D0__INCLUDED_)
