// FilePathContainer.cpp: implementation of the FilePathContainer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FilePathContainer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FilePathContainer::FilePathContainer(auto_ptr<CodingMethod>& FileCoder)
:m_Coder(FileCoder)
,m_FilePathSet()
{

}

FilePathContainer::~FilePathContainer()
{

}

void FilePathContainer::InsertFilePath(const string& FilePath)
{

		
	try
	{
		
		
		int Code = m_Coder->Coding(FilePath); 
		
		auto_ptr<FileHandle> Handle(new FileHandle(Code,FilePath)); 
		
		
		m_FilePathSet.push_back( (* Handle));
	}
	catch(exception Error)
	{
		string ErrorString = string("Error when InsertFile Path. system Info:") + Error.what();
		throw (exception(ErrorString.c_str()));
	
	}


}




FileHandle FilePathContainer::operator [](size_t Index)
{
	if(Index >= 0 && Index< GetHandleCount())
	{
		vector<FileHandle>::iterator ContainerPointer;
		
		ContainerPointer = m_FilePathSet.begin();
		
		for(size_t i = 0; i < Index ; i ++)
		{
			ContainerPointer++;
		}

		return  * ContainerPointer;



	}
	else
	{
		throw exception("exception :FilePathContainer is  overflow!!");
		
	}
	











}

size_t FilePathContainer::GetHandleCount()
{
	return m_FilePathSet.size(); 

}

FilePathContainer& FilePathContainer::operator <<(SampleFileExecuter& SampleResult)
{
	size_t Count = SampleResult.GetFilesCount();
	
	for(int i = 0; i<Count ; i++)
	{
		InsertFilePath(SampleResult[i] ); 
		
		
		
		
	}
	return * this;
	
	
}

vector<FileHandle>::iterator FilePathContainer::begin()
{
	return m_FilePathSet.begin();

}


vector<FileHandle>::iterator FilePathContainer::end()
{
	return m_FilePathSet.end();

}
