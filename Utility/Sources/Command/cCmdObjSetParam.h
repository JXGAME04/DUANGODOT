// cCmdObjSetParam.h: interface for the cCmdObjSetParam class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDObjSETPARAM_H__75925E5F_CA1C_4E8C_9397_5765284C7060__INCLUDED_)
#define AFX_CCMDObjSETPARAM_H__75925E5F_CA1C_4E8C_9397_5765284C7060__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdObjSetParam : public cCommand  
{
public:
	cCmdObjSetParam(iCommandMachine* p);
	virtual ~cCmdObjSetParam();

	stCommand_SetObjParam& GetParam()
	{return  *( (stCommand_SetObjParam*) GetBuffer() );};
	iObj* GetObj(){return GetParam().pObj;}

	HRESULT	SetParam(stCommand* pCommand);

	long GetMask();
	MaskEnum GetEMask();

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;
	stObjParam* m_pParam;
	void SetObjParam(stObjParam* pParam);
	
	BOOL m_bNextMoving;
	BOOL m_bPrevMoving;

	BOOL Merge(iCommand* p);
};

#endif // !defined(AFX_CCMDObjSETPARAM_H__75925E5F_CA1C_4E8C_9397_5765284C7060__INCLUDED_)
