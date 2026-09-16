// UnitePackage.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include"Package.h"
#include<string>
#include<vector>
#include<iostream>
#include<windows.h>
#include <iomanip>

using namespace std;
using namespace PackageCQ;

DataSource AddDataSource(vector<Package * > PackArray)
{
	if(PackArray.size()   >0 )
	{
		
		DataSource ResultSource = PackArray[0]->GetDataSource()  ;
		
		int PackCount = PackArray.size();
		
		for(int i=1;i<PackCount;i++)
		{
			
			ResultSource = ResultSource + PackArray[i]->GetDataSource();
		}
		return ResultSource;
	}
	else
	{
		throw exception ("no source to add");
	}
	
	
	
	
}

void CreatePackFile(const char *FileName ,DataSource& Source)
{
	Package  * Package3 = new Package(Source,FileName);
		
	delete Package3;
		
	



}

void FreePackArray(vector<Package * >& PackArray)
{
	for(int i = 0; i < PackArray.size() ; i++)
	{
		if (PackArray[i])
			delete PackArray[i];
	}
	PackArray.clear();
}

void ConstructionPackArray(vector<Package*>& PackArray,int Count,char * FileName[])
{
	cout << "包文件采集:"<<endl;
	for (int j=0;j<Count;j++)
	{
		Package  * PackFile = new Package(FileName[j]);
		
		cout <<"包文件"<< j<<":"<< FileName[j]<<endl;
		PackArray.push_back (PackFile); 
		

		
		
	}

	cout << endl;
	
	
}
void DisplayErrorInfo(string& ErrorInfo)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	MessageBox( NULL, ErrorInfo.c_str(),(LPCTSTR)lpMsgBuf , MB_OK |MB_ICONWARNING );
	// Free the buffer.
	LocalFree( lpMsgBuf );
	
	
	
}


void  ReplaceAFileToOtherFile(const char * SourceFileName,char * DestFileName)
{
	
	if(!DeleteFile(DestFileName))
	{
		string ErrorInfo = string("DeleteFile(") +  string(DestFileName)+string(")");
		DisplayErrorInfo(ErrorInfo);
		throw exception("error when delete old File" );
	}
	

	if(!MoveFile(SourceFileName,DestFileName))
	{
		string ErrorInfo = string("MoveFile(") +  string(SourceFileName)+string(",") + string(DestFileName) + string(")");
		DisplayErrorInfo(ErrorInfo);
		throw exception("error when rename file");
	}




}

string GetCurrentTempFileName()
{
	char DirectoryName [MAX_PATH];
	if (GetTempPath(MAX_PATH,DirectoryName) == 0)
	{
		string ErrorInfo = "GetTempPath(" +string("MAX_PATH") +",DirectoryName)";
		DisplayErrorInfo(ErrorInfo);
		throw exception("Error when Get Temp Path");
	}

	char TempFileName[MAX_PATH];
	
	if(!GetTempFileName(DirectoryName,"CQ",0,TempFileName))
	{
		string ErrorInfo = "GetTempFileName('" +string(DirectoryName) +"','CQ',0,TempFileName)";
		DisplayErrorInfo(ErrorInfo);
       	throw exception("Error when Get Temp File Name ");
	}
	
	return TempFileName;
}



void DisplaySource(DataSource& File1Source)
{
	
	cout<<"数据块数量:"<<File1Source.GetBlockCount()<<endl; 
	
	
	
	
	const Width = 12;
	for(int i=0;i<File1Source.GetBlockCount();i++)
	{
		cout<<"ID :"<<hex<<setw(Width)<<File1Source[i].GetID()<<"    Length:"<<dec<<File1Source[i].GetLength()<<"     CompressMethod:"<<(int)File1Source[i].GetCompressMethod() ;
		
		
		
		
		cout <<endl;
		
		
	}
	
	
}
void DisplayPackFile(char * filename)
{
	Package   DisplayedPackage(filename);
	
	
	
	DataSource File1Source  = DisplayedPackage.GetDataSource() ;
	
	
	
	DisplaySource(File1Source);
	

}

void DisplayAppInfo()
{
	cout << "UnitPackage 1.0"<<endl;
	cout << "用途: 压缩包的合并."<<endl;
	cout << "开发周期: 3天."<<endl;
    cout << "发明于 2003年5月7号"<<endl;
	cout << "发明人 : 著名的 常齐 先生"<<endl;


	




}
void DisplayMyInfo()
{
	cout << "作者：doctor C.Q"<<endl;
	cout << "版权所有　：　金山　。西山居"<<endl;
	cout << "MY MSN: changxy@msn.com"<<endl;
	cout << "MY TELE: 603";
    		

}

int main(int argc, char* argv[])
{
	clock_t g_start, g_finish;
    int   g_Second;

	g_start = clock(); 

	try
	{
		DisplayAppInfo();
		
		cout << "开始启动包合并工程....10,9,8,7,6,5,4,3,2,1 启动!!!"<<endl<<endl;
		vector<Package *> PackageArray;
		DataSource ResultSource;
		
		ConstructionPackArray(PackageArray,argc -1 ,&argv[1]);
		
		 
		ResultSource = AddDataSource(PackageArray);
		DisplaySource(ResultSource);

		cout << "数据源样品采集完成:"<<endl<<endl;

		

		

		string TempFileFullName = GetCurrentTempFileName();
		
		
		CreatePackFile(TempFileFullName.c_str(),ResultSource);
		

		
		FreePackArray(PackageArray);
		
		ReplaceAFileToOtherFile(TempFileFullName.c_str(),argv[1]);
		
		
		
        cout << "合并后的样品:"<<argv[1]<< endl<<endl;
		DisplayPackFile(argv[1]);
        cout <<"........................................................"<<endl<<endl;

	    DisplayAppInfo();

		DisplayMyInfo();
       
		g_finish = clock();
  
		g_Second = (double)(g_finish - g_start) / CLOCKS_PER_SEC;


		int g_minute = g_Second/60;

		g_Second     =  g_Second - g_minute *60;
		
		int g_hour   = g_minute/60;

		g_minute = g_minute - g_hour * 60;

		








		cout <<"工程运行耗时:" <<g_hour<<"小时"<<g_minute<<"分钟"<<g_Second<<"秒"<< endl;

		cout << "请按任意健并回车退出"<<endl;
		
	


		
		
		
		
		
	}
	catch(exception& error)
	{
		cout << error.what()<< endl; 
	}
	
	
	char t;
	cin >> t;
	return 0;
}
