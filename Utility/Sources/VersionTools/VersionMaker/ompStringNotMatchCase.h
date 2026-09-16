// ompStringNotMatchCase.h: interface for the CompStringNotMatchCase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OMPSTRINGNOTMATCHCASE_H__D3CBE043_6644_43A7_8FFD_99AAC0C52786__INCLUDED_)
#define AFX_OMPSTRINGNOTMATCHCASE_H__D3CBE043_6644_43A7_8FFD_99AAC0C52786__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
using namespace std;
class CompStringNotMatchCase  
{
public:
	CompStringNotMatchCase(const string& Target);
	virtual ~CompStringNotMatchCase();
	bool operator()(string& Sample )
	{
		if(m_Target.length() == Sample.length())
		{
			string::iterator TargetPointer;
			string::iterator SamplePointer = Sample.begin();

			
			for(TargetPointer = m_Target.begin();TargetPointer != m_Target.end();TargetPointer++)
			{


				if(toupper(*TargetPointer) != toupper(*SamplePointer))
				{
					return false;


				}
				SamplePointer++;


			}
		}
		else
		{
			return false;
		}
		return true;



	}
private:
	string m_Target;

};

#endif // !defined(AFX_OMPSTRINGNOTMATCHCASE_H__D3CBE043_6644_43A7_8FFD_99AAC0C52786__INCLUDED_)
