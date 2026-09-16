// cCmdCameraScale.h: interface for the cCmdCameraScale class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDCAMERASCALE_H__7DC77004_5659_4C6E_A344_D976AF1C8F0F__INCLUDED_)
#define AFX_CCMDCAMERASCALE_H__7DC77004_5659_4C6E_A344_D976AF1C8F0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdCameraScale : public cCommand  
{
public:
	cCmdCameraScale(iCommandMachine* p);
	virtual ~cCmdCameraScale();

	stCommand_ScaleCamera& GetParam()
	{return  *( (stCommand_ScaleCamera*) GetBuffer() );};

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;

	float m_fScale;
	void SetGroundScale(float scale);
	BOOL Merge(iCommand* p);
};

#endif // !defined(AFX_CCMDCAMERASCALE_H__7DC77004_5659_4C6E_A344_D976AF1C8F0F__INCLUDED_)
