// SampleFileExecuter.h: interface for the SampleFileExecuter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SAMPLEFILEEXECUTER_H__9B20094F_AD5F_4278_AE04_F0FFE4CB964E__INCLUDED_)
#define AFX_SAMPLEFILEEXECUTER_H__9B20094F_AD5F_4278_AE04_F0FFE4CB964E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#pragma warning(disable:4786)

#include<vector>
#include<string>
#include<exception>



using namespace std;





class SampleFileExecuter  
{
public:
	virtual void Execute();
	vector< string >  GetResult();
	void CleanResult();
	void InsertFilePath(const string& FilePath);
	
	int GetFilesCount() const;
	SampleFileExecuter();
	virtual ~SampleFileExecuter() ;
	string operator[] (int Index) ;
private:
	typedef vector< string > FILEPATHSET;

	FILEPATHSET m_FilePathSet;
	
	


	
};

#endif // !defined(AFX_SAMPLEFILEEXECUTER_H__9B20094F_AD5F_4278_AE04_F0FFE4CB964E__INCLUDED_)
