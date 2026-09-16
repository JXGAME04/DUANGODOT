// cCmdObjRemove.h: interface for the cCmdObjRemove class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDObjREMOVE_H__0055BA65_EDF7_4121_8949_E6A8D8D6CA8B__INCLUDED_)
#define AFX_CCMDObjREMOVE_H__0055BA65_EDF7_4121_8949_E6A8D8D6CA8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdObjRemove : public cCommand  
{
public:
	cCmdObjRemove(iCommandMachine* p);
	virtual ~cCmdObjRemove();

	stCommand_AddObj& GetParam()
	{return  *( (stCommand_AddObj*) GetBuffer() );};

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;
};

#endif // !defined(AFX_CCMDObjREMOVE_H__0055BA65_EDF7_4121_8949_E6A8D8D6CA8B__INCLUDED_)
