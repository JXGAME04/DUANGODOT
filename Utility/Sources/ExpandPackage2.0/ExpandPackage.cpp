// ExpandPackage.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include"Package.h"
#include<string>
#include<vector>


using namespace std;
using namespace PackageCQ;

DataSource AddDataSource(vector<Package * > PackArray)
{
	if(PackArray.size()   >0 )
	{
		
		DataSource ResultSource = PackArray[0]->GetDataSource()  ;
		
		int PackCount = PackArray.size();
		
		for(int i=0;i<PackCount;i++)
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
	for(int i =0 ;i<PackArray.size();i++ )
	{
		delete PackArray[i];

	}
}

void ConstructionPackArray(vector<Package*>& PackArray,int Count,char * FileName[])
{
//	cout << "包文件采集:"<<endl;
	for (int j=0;j<Count;j++)
	{
		Package  * PackFile = new Package(FileName[j]);
		
	//	cout <<"包文件"<< j<<":"<< FileName[j]<<endl;
		PackArray.push_back (PackFile); 
		

		
		
	}

//	cout << endl;
	
	
}

void  ReplaceAFileToOtherFile(const char * SourceFileName,char * DestFileName)
{
	if(!DeleteFile(DestFileName))
	{
		throw exception("error when delete old File" );
	}
	

	if(!MoveFile(SourceFileName,DestFileName))
	{
		throw exception("error when rename file");
	}




}

string GetCurrentTempFileName()
{
	char DirectoryName [MAX_PATH];
	if (GetTempPath(MAX_PATH,DirectoryName) == 0)
	{
		throw exception("Error when Get Temp Path");
	}

	char TempFileName[MAX_PATH];
	
	if(!GetTempFileName(DirectoryName,NULL,0,TempFileName))
	{
		throw exception("Error when Get Temp File Name ");
	}
	
	return TempFileName;
}

__declspec(dllexport) int  CombinatPackage(char * SourceFileName ,char * DescFileName)
{
	vector<Package *> PackageArray;
	try
	{
		char * File[2];
		
		File[0] = SourceFileName;
		File[1] = DescFileName;
		
		
		DataSource ResultSource;
		
		ConstructionPackArray(PackageArray, 2, File);
		
		
		ResultSource = AddDataSource(PackageArray);
		
		//	cout << "数据源样品采集完成:"<<endl<<endl;
		
		//DisplaySource(ResultSource);
		
		
		//	cout << "开始构造新样品:"<<argv[argc -1 ]<<endl<<endl;
		
		
		
		
		string TempFileFullName = GetCurrentTempFileName();
		
		CreatePackFile(TempFileFullName.c_str() ,ResultSource);
		//cout << "新样品构造完成"<<endl<<endl;
		
		FreePackArray(PackageArray);
		
		ReplaceAFileToOtherFile(TempFileFullName.c_str(),DescFileName);
	}
	catch(exception& Error)
	{
		FreePackArray(PackageArray);
		return 0;
	}
	return 1;
	
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

