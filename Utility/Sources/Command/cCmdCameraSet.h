// cCmdCameraSet.h: interface for the cCmdCameraSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDCAMERASET_H__28E268CC_4C81_4C52_95AD_A1F935C21500__INCLUDED_)
#define AFX_CCMDCAMERASET_H__28E268CC_4C81_4C52_95AD_A1F935C21500__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdCameraSet : public cCommand  
{
public:
	cCmdCameraSet(iCommandMachine* p);
	virtual ~cCmdCameraSet();

	iCamera::stParam m_param;
	stCommand_SetCamera& GetParam();
	eDoType	Redo() ;
	eDoType	Do() ;
	eDoType	Undo() ;
	void SetCamera(iCamera::stParam param);
};

#endif // !defined(AFX_CCMDCAMERASET_H__28E268CC_4C81_4C52_95AD_A1F935C21500__INCLUDED_)
