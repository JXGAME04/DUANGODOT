// cImage.cpp: implementation of the cImage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "cImage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cImage::cImage()
{
	m_ulCount = 1;
}

cImage::~cImage()
{

}

//////////////////////////////////////////////////////////////////////
//com

ULONG cImage::AddRef() 
{
	return ++m_ulCount;
}

ULONG cImage::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}
