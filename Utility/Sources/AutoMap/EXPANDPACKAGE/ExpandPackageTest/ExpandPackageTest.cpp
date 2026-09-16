// ExpandPackageTest.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include<exception>

#define ExpandPackageLibPath "D:\\我的工程\\ExpandPackage\\debug\\ExpandPackage.dll"
#define FunctionName "CombinatPackage"
void DisplayLastErrorInfo()
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
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
	// Free the buffer.
	LocalFree( lpMsgBuf );
}

typedef int (*MYPROC)(char *  ,char * );

int ExpandPackage(char * Source,char * Desc)
{
	HMODULE ExpandPackageLib = LoadLibrary(ExpandPackageLibPath);

    if(NULL == ExpandPackageLib)
	{
		DisplayLastErrorInfo();
		throw exception("Error when LoadLibrary"); 

		

	}
	
	MYPROC ProcAdd  = (MYPROC)GetProcAddress(ExpandPackageLib,FunctionName);
	if(!ProcAdd)
	{
		DisplayLastErrorInfo();
		throw exception("Error when GetProcAddress"); 
	}
	
	int Result = (* ProcAdd)(Source,Desc);

	if(!FreeLibrary(ExpandPackageLib))
	{
		DisplayLastErrorInfo();

		throw exception("Error when FreeLibrary");


	}


	return Result;


}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.

	try
	{
		
		int Result = ExpandPackage("d:\\BBB.pak","d:\\AAA.pak");
		if(!Result)
		{
			MessageBox(NULL,"ExpandPackage 失败","ExpandPackageTest",MB_ICONERROR );

		}
		else 
		{
			MessageBox(NULL,"ExpandPackage 成功","ExpandPackageTest",MB_OK );

		}
	}
	catch(exception & error)
	{
		MessageBox(NULL,error.what(),"ExpandPackageTest",MB_ICONERROR );
		
		
		
	}
	return 0;
}



