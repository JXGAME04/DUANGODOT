// cPack1DObj.cpp: implementation of the cPack1DObj class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPack1DObj.h"

#include "iItem.h"
#include "iTemplate.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPack1DObj::cPack1DObj()
{

}

cPack1DObj::~cPack1DObj()
{

}

BOOL cPack1DObj::LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;

	wrap_ptr<iItem> ptr(pItem);


	return TRUE;
}
