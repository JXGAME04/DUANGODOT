// UpdateRelativeFileProcess1.cpp: implementation of the UpdateRelativeFileProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UpdateRelativeFileProcess1.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

UpdateRelativeFileProcess::UpdateRelativeFileProcess()
:FileNameSet()
{

}

UpdateRelativeFileProcess::~UpdateRelativeFileProcess()
{

}

bool UpdateRelativeFileProcess::ProcessIterm(const string& sType,istream& DataStream)
{
	if(sType == UpdateRelativeFile)
	{
		
	
		string Elem;
		
		if(!DataStream.eof())
		{	
			//throw exception("Stream End when UpdateRelativeFileProcess::ProcessIterm ; File:__FILE__;Line:__LINE__"");
			GetALineFromStream(Elem,DataStream);
		}

		while(!CheckIsEnd(Elem))
		{
			
			FileNameSet.push_back(Elem); 
            Elem = string("");

			if(!DataStream.eof())
			{	
				//throw exception("Stream End when UpdateRelativeFileProcess::ProcessIterm ; File:__FILE__;Line:__LINE__"");
				GetALineFromStream(Elem,DataStream);
			}
			else
			{
				break;
			}
			
			
       	}

		
		


		
		return true;
		


	}
	else
	{
		return false;
	}

}

TextSet UpdateRelativeFileProcess::GetResult()
{
	return FileNameSet;

}





const string UpdateRelativeFileProcess::GetType() const
{
	return UpdateRelativeFile;



}

void UpdateRelativeFileProcess::Reset()
{
	FileNameSet.clear();

}
