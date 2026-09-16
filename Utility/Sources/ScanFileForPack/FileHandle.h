// FileHandle.h: interface for the FileHandle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEHANDLE_H__7111CB60_E647_4927_A7E1_3E79991647B1__INCLUDED_)
#define AFX_FILEHANDLE_H__7111CB60_E647_4927_A7E1_3E79991647B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<string>
#include <iomanip>


using namespace std;


class FileHandle  
{
public:
	string GetPath() const;
	int GetID() const ;
	FileHandle(int FileCode,const string& FilePath);
	virtual ~FileHandle();
	bool operator ==(const FileHandle &SourceHandle) const;
    bool operator <(const FileHandle &SourceHandle) const;
	bool operator >(const FileHandle &SourceHandle) const;
private:
	int m_ID;
	string m_Path;
};
inline ostream& operator << (ostream& strm, const FileHandle& Handle)
{
	const Width = 9;
	strm.exceptions(ios::badbit|ios::failbit ); 
	strm<<hex<<setw(Width)<<Handle.GetID()<<"    "<<Handle.GetPath();
	
	return strm;



}
#endif // !defined(AFX_FILEHANDLE_H__7111CB60_E647_4927_A7E1_3E79991647B1__INCLUDED_)
