// ReadRecordFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include"RecordProcess.h"
#include"UpdateRelativeFileProcess1.h"
#include<fstream>
#include<iostream>
#include<memory>

using namespace std;

int main(int argc, char* argv[])
{
	try
	{
		
		ifstream in("E:\\updataTest\\Info\\UpdateRelativeFile.ini");
		auto_ptr<UpdateRelativeFileProcess> UpdateRelativeProcess (new UpdateRelativeFileProcess());
		RecordProcess InfoProcess;
		
		InfoProcess.AddProcessFunction(UpdateRelativeProcess.get());
		
		InfoProcess.AnalyseRecordStream(in);

		TextSet FileNameS = UpdateRelativeProcess->GetResult();

		TextSet::iterator Pointer;
		for(Pointer = FileNameS.begin();Pointer!=FileNameS.end();Pointer++)
		{
			cout<<(* Pointer)<<endl;

		}
        

		

	}
	catch(exception& Error)
	{
		cout<<Error.what()<<endl;
	}

	return 0;
}
