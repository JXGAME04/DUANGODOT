// cBNodeRoot.cpp: implementation of the cBNodeRoot class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cBNodeRoot.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cBNodeRoot::cBNodeRoot() : cBNode(NULL)
{
	_p1 = cPoint(0,-1000);
	_p2 = cPoint(1,-1000);
}

cBNodeRoot::~cBNodeRoot()
{

}

VersortEnum cBNodeRoot::GetVersortType()
{
	return VERSORT_TREE;
}

void cBNodeRoot::GetSortLine(float& k,float& b)
{
	k = 0;
	b = -1000;
}
