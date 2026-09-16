// FilePathContainer.h: interface for the FilePathContainer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEPATHCONTAINER_H__A958F030_A1D9_4C61_A271_E6582CDF3A30__INCLUDED_)
#define AFX_FILEPATHCONTAINER_H__A958F030_A1D9_4C61_A271_E6582CDF3A30__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#pragma warning(disable:4786)


#include<string>
#include<vector>
#include<memory>
#include<iterator>

#include "odingMethod.h"

#include"FileHandle.h"
#include"SampleFileExecuter.h"


using namespace std;


class FilePathContainer  
{
public:
	
	vector<FileHandle>::iterator end();
	
	vector<FileHandle>::iterator begin();
	size_t GetHandleCount();


	void InsertFilePath(const string& FilePath);
	FilePathContainer(auto_ptr<CodingMethod>& FileCoder);
	virtual ~FilePathContainer();
	FileHandle operator [](size_t Index) ;
	FilePathContainer& operator<<(SampleFileExecuter& SampleResult);
	
private:
	vector<FileHandle> m_FilePathSet;
	auto_ptr<CodingMethod> m_Coder;
	


};

inline ostream& operator << (ostream& OutStream,FilePathContainer& Sample)
{
	copy(Sample.begin(),Sample.end(),ostream_iterator<FileHandle>(OutStream,"\n") );

	return OutStream;

}
#endif // !defined(AFX_FILEPATHCONTAINER_H__A958F030_A1D9_4C61_A271_E6582CDF3A30__INCLUDED_)
