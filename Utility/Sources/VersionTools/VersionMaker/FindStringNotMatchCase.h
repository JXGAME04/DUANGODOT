// FindStringNotMatchCase.h: interface for the FindStringNotMatchCase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FINDSTRINGNOTMATCHCASE_H__0F743D8F_DF6C_455C_B3E9_94A0F0371BF2__INCLUDED_)
#define AFX_FINDSTRINGNOTMATCHCASE_H__0F743D8F_DF6C_455C_B3E9_94A0F0371BF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CompStringNotMatchCase  
{
public:
	FindStringNotMatchCase(const string& Target);
	virtual ~FindStringNotMatchCase();
	bool operator(const string& )
	
private:
	string m_Target;

};

#endif // !defined(AFX_FINDSTRINGNOTMATCHCASE_H__0F743D8F_DF6C_455C_B3E9_94A0F0371BF2__INCLUDED_)
