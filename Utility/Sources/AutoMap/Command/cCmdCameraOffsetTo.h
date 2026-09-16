// cCmdCameraOffsetTo.h: interface for the cCmdCameraOffsetTo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDCAMERAOFFSETTO_H__91FDCD40_A9D6_435A_B885_D87C76FF786F__INCLUDED_)
#define AFX_CCMDCAMERAOFFSETTO_H__91FDCD40_A9D6_435A_B885_D87C76FF786F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdCameraOffsetTo : public cCommand  
{
public:
	cCmdCameraOffsetTo(iCommandMachine* p);
	virtual ~cCmdCameraOffsetTo();

	stCommand_OffsetCameraTo& GetParam(){return *((stCommand_OffsetCameraTo*)GetBuffer());};
	eDoType	Redo() ;
	eDoType	Do() ;
	eDoType	Undo() ;

protected:
	cPoint  m_ptOldCamera;
	void	OffsetCameraTo(cPoint pt);
};

#endif // !defined(AFX_CCMDCAMERAOFFSETTO_H__91FDCD40_A9D6_435A_B885_D87C76FF786F__INCLUDED_)
