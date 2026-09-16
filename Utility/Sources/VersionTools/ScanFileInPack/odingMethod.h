// odingMethod.h: interface for the CodingMethod class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODINGMETHOD_H__C0113D2A_65B7_45B4_9EB6_88E404EFEFAF__INCLUDED_)
#define AFX_ODINGMETHOD_H__C0113D2A_65B7_45B4_9EB6_88E404EFEFAF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
using namespace std;


class CodingMethod  
{
public:
	virtual int  Coding(const string &SourceWord) const;
	CodingMethod();
	virtual ~CodingMethod();

};

#endif // !defined(AFX_ODINGMETHOD_H__C0113D2A_65B7_45B4_9EB6_88E404EFEFAF__INCLUDED_)
