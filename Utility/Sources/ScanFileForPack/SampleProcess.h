// SampleProcess.h: interface for the SampleProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SAMPLEPROCESS_H__F773A2E5_BFA2_4DAC_872E_8A871E026EFF__INCLUDED_)
#define AFX_SAMPLEPROCESS_H__F773A2E5_BFA2_4DAC_872E_8A871E026EFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Arithmetic.h"
template <class  InputIterator ,class OutputIterator> 
class SampleProcess  
{
public:
	SampleProcess()
	{
	}
	virtual ~SampleProcess()
	{
	}
	virtual OutputIterator Process(InputIterator _First,InputIterator _Last,OutputIterator _DestBeg)
	{
		return SelectElemCountNotOne(_First,_Last,_DestBeg);
	}

};

#endif // !defined(AFX_SAMPLEPROCESS_H__F773A2E5_BFA2_4DAC_872E_8A871E026EFF__INCLUDED_)
