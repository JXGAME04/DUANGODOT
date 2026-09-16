// cCmdGroundDynamic.h: interface for the cCmdGroundDynamic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDGROUNDDYNAMIC_H__2595403D_568D_4199_AA83_CAD2C816ED66__INCLUDED_)
#define AFX_CCMDGROUNDDYNAMIC_H__2595403D_568D_4199_AA83_CAD2C816ED66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdGroundDynamic : public cCommand  
{
public:
	cCmdGroundDynamic(iCommandMachine* p);
	virtual ~cCmdGroundDynamic();

	stCommand_Dynamic& GetParam(){return *((stCommand_Dynamic*)GetBuffer());};

	eDoType	Do() ;
};

#endif // !defined(AFX_CCMDGROUNDDYNAMIC_H__2595403D_568D_4199_AA83_CAD2C816ED66__INCLUDED_)
