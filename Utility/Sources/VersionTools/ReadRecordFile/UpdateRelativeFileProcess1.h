// UpdateRelativeFileProcess1.h: interface for the UpdateRelativeFileProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UPDATERELATIVEFILEPROCESS1_H__3F5E9BC8_E58C_45AC_AEC4_D47DAECC58BE__INCLUDED_)
#define AFX_UPDATERELATIVEFILEPROCESS1_H__3F5E9BC8_E58C_45AC_AEC4_D47DAECC58BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#pragma warning(disable:4786)

#include "ItermProcess.h"
#include<vector>
#include<strstream>
using namespace std;



typedef vector<string> TextSet; 

class UpdateRelativeFileProcess : public ItermProcess  
{
public:
	void Reset();
	const string GetType() const;
	
	TextSet GetResult();
	bool ProcessIterm(const string& sType,istream& DataStream);
	UpdateRelativeFileProcess();
	virtual ~UpdateRelativeFileProcess();
private:
    TextSet FileNameSet;
    


};

#endif // !defined(AFX_UPDATERELATIVEFILEPROCESS1_H__3F5E9BC8_E58C_45AC_AEC4_D47DAECC58BE__INCLUDED_)
