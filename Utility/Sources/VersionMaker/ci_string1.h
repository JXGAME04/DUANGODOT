// ci_string1.h: interface for the ci_string class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CI_STRING1_H__69665902_868F_4F8B_9BC5_C34C94E634C5__INCLUDED_)
#define AFX_CI_STRING1_H__69665902_868F_4F8B_9BC5_C34C94E634C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
using namespace std;
struct ci_char_traits:public char_traits<char>
{
	static bool eq(char c1,char c2)
	{
		return toupper(c1) == toupper(c2);
		
	}
	static bool lt(char c1,char c2)
	{
		return toupper(c1) < toupper(c2);
		
	}

	static int  compare(const * s1,char * s2,size_t n)
	{
		return memicmp(s1,s2,n);

		
	}
	static const char *
	find(const char * s,int n,char a)
	{
		while( n-->0 && toupper(*s) != toupper(a))
		{
			++s;
		}
		return n >0 ? s:0;

	}




};
typedef basic_string<char,ci_char_traits> ci_string;


#endif // !defined(AFX_CI_STRING1_H__69665902_868F_4F8B_9BC5_C34C94E634C5__INCLUDED_)
