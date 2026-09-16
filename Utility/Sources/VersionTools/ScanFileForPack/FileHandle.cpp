// FileHandle.cpp: implementation of the FileHandle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileHandle.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FileHandle::FileHandle(int FileCode,const string& FilePath)
:m_ID(FileCode),
m_Path(FilePath)
{

}

FileHandle::~FileHandle()
{

}

bool FileHandle::operator ==(const FileHandle &SourceHandle) const
{
	if( m_ID == SourceHandle.m_ID )
	{
		return true;
	}
	else
	{
		return false;


	}
		


}

bool FileHandle::operator <(const FileHandle &SourceHandle) const
{
	if( m_ID < SourceHandle.m_ID )
	{
		return true;
	}
	else
	{
		return false;


	}
}
bool FileHandle::operator >(const FileHandle &SourceHandle) const
{
	if( m_ID > SourceHandle.m_ID )
	{
		return true;
	}
	else
	{
		return false;


	}
}


int FileHandle::GetID() const
{
	return m_ID;

}

string FileHandle::GetPath() const 
{
	return m_Path;

}
