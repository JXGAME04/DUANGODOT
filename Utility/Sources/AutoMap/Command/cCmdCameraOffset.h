// cCmdCameraOffset.h: interface for the cCmdCameraOffset class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDCAMERAOFFSET_H__BF569EE3_18B3_40D0_8FC3_86FA053155F5__INCLUDED_)
#define AFX_CCMDCAMERAOFFSET_H__BF569EE3_18B3_40D0_8FC3_86FA053155F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdCameraOffset : public cCommand  
{
public:
	cCmdCameraOffset(iCommandMachine* p);
	virtual ~cCmdCameraOffset();

	stCommand_OffsetCamera& GetParam();
	eDoType Do();
	eDoType Redo();
	eDoType Undo();
	BOOL	Merge(iCommand* p);

protected:
	void	OffsetCameraTo(cPoint pt);

	cPoint m_ptStart;
	cPoint m_ptEnd;	
};

#endif // !defined(AFX_CCMDCAMERAOFFSET_H__BF569EE3_18B3_40D0_8FC3_86FA053155F5__INCLUDED_)
