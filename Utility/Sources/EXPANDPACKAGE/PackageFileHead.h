// PackageFileHead.h: interface for the PackageFileHead class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PACKAGEFILEHEAD_H__1E5A6AAF_1702_42F3_8336_D3C12F628B4C__INCLUDED_)
#define AFX_PACKAGEFILEHEAD_H__1E5A6AAF_1702_42F3_8336_D3C12F628B4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
#include<iostream>
using namespace std;
namespace PackageCQ
{
	
#define FileID       "PACKAGE"
#define FileIDLength  8 	
	
	
	class PackageFileHead  
	{
	public:
		PackageFileHead(int BlockCount,int CompressMethod,const string& FileTypeString);
		int GetCompressMethod() const;
		int GetBlockCount() const;
		const string& GetFileType() const;
		PackageFileHead();
		virtual ~PackageFileHead();
		const PackageFileHead& operator<< (iostream &PackageDataStream);
		PackageFileHead& operator >>(iostream &PackageDataStream);
	private:
		string m_FileTypeString;
		unsigned    m_BlockCount;
		int    m_CompressMethod;
		
		
		
	};
}

#endif // !defined(AFX_PACKAGEFILEHEAD_H__1E5A6AAF_1702_42F3_8336_D3C12F628B4C__INCLUDED_)
