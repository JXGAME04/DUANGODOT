// FilePath.h: interface for the FilePath class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEPATH_H__6AACB391_B888_4F42_8534_9CF2FF786158__INCLUDED_)
#define AFX_FILEPATH_H__6AACB391_B888_4F42_8534_9CF2FF786158__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
#include"ci_string1.h"
using namespace std;
class FilePath  
{
public:
	FilePath(const string&  FileFullPath,const string&  FileHalfPath);
	virtual ~FilePath();
    string FullPath;
	string HalfPath;
};

#endif // !defined(AFX_FILEPATH_H__6AACB391_B888_4F42_8534_9CF2FF786158__INCLUDED_)
