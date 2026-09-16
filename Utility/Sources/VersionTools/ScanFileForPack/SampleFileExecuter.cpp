// SampleFileExecuter.cpp: implementation of the SampleFileExecuter class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SampleFileExecuter.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


SampleFileExecuter::SampleFileExecuter()
:m_FilePathSet()
{
	

}

SampleFileExecuter::~SampleFileExecuter()
{

}

int SampleFileExecuter::GetFilesCount() const 
{
	return m_FilePathSet.size();

}










void SampleFileExecuter::InsertFilePath(const string& FilePath)
{
	try
	{
		
		m_FilePathSet.push_back(FilePath);
	}
	catch(exception& Error)
	{
		string ErrorString = string("Error when SampleExecute Insert.system info: ") + Error.what();
	    throw exception(ErrorString.c_str());
	}

}

void SampleFileExecuter::CleanResult()
{
	m_FilePathSet.clear(); 


}

vector< string >  SampleFileExecuter::GetResult()
{
	return m_FilePathSet;

}

string SampleFileExecuter::operator[] (int Index) 
{
	if(Index >= 0 && Index< GetFilesCount())
	{
		vector<string>::iterator ContainerPointer;
		
		ContainerPointer = m_FilePathSet.begin();
		
		for(size_t i = 0; i < Index ; i ++)
		{
			ContainerPointer++;
		}

		return  * ContainerPointer;



	}
	else
	{
		throw exception("exception: SampleFileExecute is overflow!!");
		
	}
	

}

void SampleFileExecuter::Execute()
{

}
