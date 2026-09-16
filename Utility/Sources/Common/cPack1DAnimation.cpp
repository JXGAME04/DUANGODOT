// cPack1DAnimation.cpp: implementation of the cPack1DAnimation class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPack1DAnimation.h"
#include "iTemplate.h"
#include "iItem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPack1DAnimation::cPack1DAnimation()
{

}

cPack1DAnimation::~cPack1DAnimation()
{

}

BOOL cPack1DAnimation::LoadTemplate(LPCSTR szFile,char** buffer, DWORD& size)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;

	wrap_ptr<iItem> ptr(pItem);

	stAnimation* pAni = new stAnimation;
	pAni->nDirections = 8;
	VERIFY(GetItemValue(pItem,"frames",pAni->nFrames));
	VERIFY(GetItemValue(pItem,"anispeed",pAni->nAniSpeed));
	VERIFY(GetItemValue(pItem,"aniloop",pAni->bAniLoop));
	pAni->nDirections = 8;
	*buffer = (char*)pAni;
	size = sizeof(stAnimation);		 
	return TRUE;
}
