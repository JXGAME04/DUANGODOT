
#pragma once

#include <fstream>
#include "fileop.h"

class cLog
{
public:
	std::ofstream m_file;
	void save()
	{
		m_file.flush();
		m_file.close();
	}
	bool create(LPCSTR szFile){
		m_file.open(szFile); 
		if (!m_file.is_open())
		{
			return false;
		}
		write_createinfo();
		return true;
	}
	bool write_log(LPCSTR szString)
	{
		if (!m_file.is_open())
		{
			return false;
		}
		m_file << szString << "\n";
		return true;
	}
	void write_createinfo()
	{
		CTime time = CTime::GetCurrentTime();
		CString sTime = time.Format( "%H:%M, %B %d, %Y" );
		CString sMsg = "create log file at " + sTime + "\n";
		write_log(sMsg);
	}
};

class cAppLog : public cLog
{
public:
	bool create(){
		CString strFile = AfxGetAppName();
		strFile += ".log";
		AppGetFullPath(strFile);
		return cLog::create(strFile);
	}
	void open()
	{
		CString strFile = AfxGetAppName();
		strFile += ".log";
		AppGetFullPath(strFile);
		ShellExecute(NULL,"open",(LPCSTR)strFile,NULL,NULL,SW_SHOW);
	}
};