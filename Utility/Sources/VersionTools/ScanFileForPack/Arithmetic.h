// GetDiffrent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include<algorithm>
#include<vector>
#include<iostream>
using namespace std;

//
template <class  InputIterator ,class OutputIterator> 
inline OutputIterator SelectElemCountNotOne( InputIterator _First, 
											InputIterator _Last, 
											OutputIterator _DestBeg)
{
	const	ConditionCount = 1;
	//先排序。使同单元聚合
	
	if(_DestBeg == NULL)
	{
		throw exception("OutputIterator must not be null when SelectElemCountNotOne");
		
	}
	
	try
	{
		sort(_First,_Last);
	}
	catch(exception& Error)
	{
		string ErrorString = string("sort Error when SelectElemCountNotOne .system info:") +Error.what() ;
		throw exception(ErrorString.c_str());
		
	}
	
	
	InputIterator Pointer,UnitFirst,UnitEnd;
	
	
	UnitFirst = _First; 
	
    
	OutputIterator OutputPoint = _DestBeg;
	
	int UnitElemCount = 0;
	
	for(Pointer = _First;Pointer != _Last;Pointer++)
	{
		
		//同单元条件（值相同的为一个单元）
		try
		{
			
			if((* Pointer) == (* UnitFirst))
			{
				
				
				
				UnitElemCount ++;
				
				
			}
			else
			{    //一个单元结束
				//对一个单元的操作（单元的元素个数多于一就把单元复制给输出）
				UnitEnd = Pointer;
				
				if(UnitElemCount > ConditionCount)
				{
					OutputPoint = copy(UnitFirst,UnitEnd,OutputPoint);
					
				}
				
				//新单元开始
				UnitFirst = UnitEnd = Pointer;
				
				
				UnitElemCount = 1;
				
			}
		}
		catch(exception& Error)
		{
			string ErrorString = string("Error when SelectElemCountNotOne Get gather system info:") + Error.what();
            throw  exception(ErrorString.c_str());		
			
		}
		
		
		
	}
	//循环完后未对最后一个单元作处理
	
	//最后一个单元的操作
	UnitEnd = Pointer;
    
	if(UnitElemCount >ConditionCount)
	{
		
		OutputPoint = copy(UnitFirst,UnitEnd,OutputPoint);
	}
	
	
	
	
	return OutputPoint;
	
	
}