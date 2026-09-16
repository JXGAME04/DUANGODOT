// PackageFileHead.cpp: implementation of the PackageFileHead class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PackageFileHead.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace PackageCQ
{
	PackageFileHead::PackageFileHead()
		:m_FileTypeString()
		,m_BlockCount(0)
		,m_CompressMethod(0)
	{
		
	}
	
	PackageFileHead::~PackageFileHead()
	{
		
	}
	
	const PackageFileHead& PackageFileHead::operator<< (iostream &PackageDataStream)
	{
		
		char FileTypeChar[FileIDLength];
		
		if(!PackageDataStream.eof())
		{
			PackageDataStream.read(FileTypeChar,FileIDLength);

			

		}
		else
		{
			throw exception("File is end when Read File Head ");
		}
		if(PackageDataStream.bad() )
		{
			throw exception("read error when Read File Head");
		}

		m_FileTypeString = FileTypeChar;
		if(!PackageDataStream.eof())
		{
			PackageDataStream.read((char *)&m_BlockCount,sizeof(m_BlockCount));
			
		
		}
		else
		{
			throw exception("File is end when Read File Head ");
		}
		if(PackageDataStream.bad() )
		{
			throw exception("read error when Read File Head");
		}
		if(!PackageDataStream.eof())
		{
			PackageDataStream.read((char *)&m_CompressMethod,sizeof(m_CompressMethod));
			
		}
		else
		{
			throw exception("File is end when Read File Head ");
		}
		
		if(PackageDataStream.bad() )
		{
			throw exception("read error when Read File Head");
		}
		
		
		
		
		return * this;
		
	}
	
	const string& PackageFileHead::GetFileType() const
	{
		return m_FileTypeString;
		
	}
	
	int PackageFileHead::GetBlockCount() const
	{
		return m_BlockCount;
		
	}
	
	int PackageFileHead::GetCompressMethod() const
	{
		return m_CompressMethod;
		
	}
	
	PackageFileHead& PackageFileHead::operator >>(iostream &PackageDataStream)
	{
		PackageDataStream.write(m_FileTypeString.c_str(),FileIDLength )  ;
		PackageDataStream.write((char *)&m_BlockCount,sizeof(m_BlockCount));
		PackageDataStream.write ((char *)&m_CompressMethod,sizeof(m_CompressMethod));
		

		if(PackageDataStream.bad())
		{
			throw("write File Head Error");
		}

		
		
		
		return  * this;
		
		
		
		
		
	}
	
	PackageFileHead::PackageFileHead(int BlockCount, int CompressMethod, const string &FileTypeString)
	{
		m_CompressMethod = CompressMethod;
		
		m_BlockCount     = BlockCount;
		
		m_FileTypeString = FileTypeString;
		
		
		
		
		
	}
}
