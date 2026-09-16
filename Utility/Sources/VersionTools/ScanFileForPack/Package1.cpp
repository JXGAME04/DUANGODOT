// Package1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<iostream>
#include<string>
#include"FilePathContainer.h"
#include"KScanFile.h"
#include<time.h>
#include"SampleProcess.h"
#include<fstream>

using namespace std;

void PutSampleFilePathInContainer(SampleFileExecuter& CollectFileExecute,FilePathContainer& Container)
{
	int Count = CollectFileExecute.GetFilesCount();
	
	//cout<<"文件数量："<<Count<<endl;
	
	
	
	try
	{
		Container<<CollectFileExecute;
		
	}
	catch(exception& e)
	{
		cout<<e.what(); 
		
	}
	
	
	
}

FilePathContainer ConstructFilePathSet(vector<FileHandle>::iterator  _First,vector<FileHandle>::iterator _Last,auto_ptr<CodingMethod>& Coding)
{
	FilePathContainer Result(Coding);
	vector<FileHandle>::iterator Pointer;
	for(Pointer =  _First ; Pointer  != _Last ;Pointer++)
	{
		Result.InsertFilePath((*Pointer). GetPath());
		
		
	}
	return Result;
	
	
	
}
void DisplayContainerContent(FilePathContainer& Container)
{
	
	
	int Count = Container.GetHandleCount(); 
	
	try
	{
		cout<< Container;
	}
	catch(exception& e)
	{
		cout<<e.what(); 
		
	}
	
	cout<<endl;
	
	
	
	
}
void DumpFilePathToFile(string& FileName,FilePathContainer& Container)
{
	fstream OutFile(FileName.c_str());
	if(OutFile.is_open() )
	{
		OutFile << Container;
		
	}
	else
	{
		throw exception("Error when open Out File");
	}
	
	
	
}

void ConstructionFolderArray(vector<string>& FolderArray,int Count,char * FolderName[],char * RootFolder)
{
	cout << "文件夹采集:"<<endl;
	for (int j=0;j<Count;j++)
	{
		
		string FolderFullName = string(RootFolder)  +  string(FolderName[j]); 
		cout <<"文件夹"<< j<<":"<< FolderFullName<<endl;
		FolderArray.push_back (FolderFullName); 
		
		
		
		
	}
	
	cout << endl;
	
	
}


FilePathContainer ProcessFolderFile(vector<string>& FolderArray,const string& RootName,KScanFile& SampleExecuter,	FilePathContainer& Container,auto_ptr<CodingMethod>& SimpleCodingMethod,SampleProcess<vector<FileHandle>::iterator,vector<FileHandle>::iterator>  *  GetRepeatedProcessMethod )
{
	for (int j=0;j<FolderArray.size();j++)
	{
		
		
		SampleExecuter.SetOutFile(""); 
		SampleExecuter.SetRootPath( RootName); 
		SampleExecuter.SetScanPath(string(FolderArray[j])); 
		
		SampleExecuter.Execute(); 
		
		PutSampleFilePathInContainer(SampleExecuter,Container);
	}
	FilePathContainer Temp = Container;
	
	
	
	vector<FileHandle>::iterator End = GetRepeatedProcessMethod->Process(Container.begin(),Container.end(),Temp.begin()); 
	return ConstructFilePathSet(Temp.begin(),End,SimpleCodingMethod);
	
	
}
void DisplayMyInfo()
{
	cout<<endl;
	cout << "作者：doctor C.Q"<<endl;
	cout << "版权所有　：　金山　。西山居"<<endl;
	cout << "MY MSN: changxy@msn.com"<<endl;
	cout << "MY TELE: 603";
	
	
}

void DisplayAppInfo()
{
	cout << "Scaner  1.0"<<endl;
	cout << "用途: 扫描数个文件架中又重复编号的情况."<<endl;
	cout << "开发周期: 3天."<<endl;
    cout << "发明于 2003年5月19号"<<endl;
	cout << "发明人 : 常齐"<<endl<<endl;

	
	
	
	
	
	
	
}
bool CheckAFolder(const char * FolderName)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFind = NULL;
	
	// set findfirstfile to find everthing
	hFind = FindFirstFile(FolderName, &FindData);
	
	// if handle fails, drive is empty...
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	
	// get first entity on drive - check if it's a directory
	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return true;

	}

	return true;

}

void CheckFolderS(vector<string>& FolderArray)
{
	for(int i =0;i<FolderArray.size();i++)
	{
		if(!CheckAFolder(FolderArray[i].c_str()))
		{
			string Error =  FolderArray[i] + string(" is not A exist Folder");
			throw exception(Error.c_str());
		}

	}
	
		
}
int main(int argc, char* argv[])
{
	clock_t g_Start, g_Finish;
	g_Start = clock();
	
	DisplayAppInfo();
	
	cout<<"开始,请耐心等待"<<endl;
	
	
	try
	{
		
		auto_ptr<CodingMethod>  g_SimpleCodingMethod =auto_ptr<CodingMethod>(new CodingMethod);
		
		FilePathContainer g_Container(g_SimpleCodingMethod),g_Temp(g_SimpleCodingMethod);
		
		auto_ptr<KScanFile>   g_ScanFile =auto_ptr<KScanFile>(new KScanFile());
		
		SampleProcess<vector<FileHandle>::iterator,vector<FileHandle>::iterator>  *  GetRepeatedProcessMethod = new SampleProcess<vector<FileHandle>::iterator,vector<FileHandle>::iterator>();
		
		if(CheckAFolder(argv[1]))
		{
			string ErrorString = string(argv[1]) + string("is not valid");
			throw exception(ErrorString.c_str());
		}
		//cout<<"生成文件列表"<<endl;
       
	
        
	
		vector<string> FolderArray;
		ConstructionFolderArray(FolderArray,argc -3 ,&(argv[2]),argv[1]);

		
		CheckFolderS(FolderArray);
        
		FilePathContainer ResultContainer = ProcessFolderFile(FolderArray,string(argv[1]),* g_ScanFile,g_Container,g_SimpleCodingMethod,GetRepeatedProcessMethod);

		cout<<"重复数量:"<<ResultContainer.GetHandleCount()<<endl;  
		
		//DisplayContainerContent(g_Container);

		DisplayContainerContent(ResultContainer);
		
		fstream OutFile;
		OutFile.open(argv[argc - 1],ios::out);
		if(OutFile.is_open() )
		{
			OutFile << ResultContainer;
			
		}
		else
		{
			throw exception("Error when open Out File");
		}
		
		
        
		//DumpFilePathToFile("d:\cq1.txt",ResultContainer);
		
		
	}
	catch(exception& e)
	{
		cout<<"发生异常:"<<e.what()<<endl;
	}
	
	
	g_Finish = clock();
	
	double g_Spent = (double)(g_Finish - g_Start) / CLOCKS_PER_SEC;
	

	
	cout<<"所用时间："<<g_Spent<<endl;
	
	
	DisplayMyInfo();
	
	
	return 0;
}

