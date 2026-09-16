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
	
	/*int Id = 0;
	
	
	try
	{
		
		
		for ( int i = 0; i< SourceWord.length(); i++ )
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
*/
	
	unsigned long id = 0;
	const char *ptr = SourceWord.c_str();
	int index = 0;
	while(*ptr) {
		if(*ptr >= 'A' && *ptr <= 'Z') id = (id + (++index) * (*ptr + 'a' - 'A')) % 0x8000000b * 0xffffffef;
		else id = (id + (++index) * (*ptr)) % 0x8000000b * 0xffffffef;
		ptr++;
	}
	return (id ^ 0x12345678);

}
