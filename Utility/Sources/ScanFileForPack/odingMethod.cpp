// odingMethod.cpp: implementation of the CodingMethod class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "odingMethod.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CodingMethod::CodingMethod()
{

}

CodingMethod::~CodingMethod()
{

}

int CodingMethod::Coding(const string &SourceWord) const
{
	
	int Id = 0;
	
	
	try
	{
		
		
		for ( int i = 0; SourceWord[i]; i++ )
		{
			Id = (Id + (i + 1) * SourceWord[i]) % 0x8000000b * 0xffffffef;
		}
	}
	catch(exception& Error)
	{
		string ErrorString =string("Error when Coding .system info:") + Error.what() ;
		
		throw exception(ErrorString.c_str()) ;
	}
	return (Id ^ 0x12345678);
	
}
