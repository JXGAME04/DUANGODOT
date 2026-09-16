// cSpecialSign.h: interface for the cSpecialSign class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSPECIALSIGN_H__60C3B5D2_9B4C_44D5_9863_0E4FF5DE8389__INCLUDED_)
#define AFX_CSPECIALSIGN_H__60C3B5D2_9B4C_44D5_9863_0E4FF5DE8389__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cSpecial.h"

class cSpecialSign : public cSpecial  
{
public:
	cSpecialSign(cGraphics* p);
	virtual ~cSpecialSign();

//load and save
	HRESULT				LoadBinary(CFile& f);
	HRESULT				SaveBinary(CFile& f);
//run
	BOOL				Active();	
	BOOL				Run(){return TRUE;};

//draw
	cPoint				GetDrawPoint();
	BOOL				IsDraw(DrawEnum e); //是否绘制
	HRESULT				Draw(DrawEnum e);

//load and save
	struct stParam
	{
		ElementEnum			eElement;
		cPoint				ptStart;//卷轴的起始点
		cPoint				ptEnd;//卷轴的起始点
		cColor				crColor;//color
	};
	stParam m_param;
	enum	ParamEnum{
		PARAM_NULL = -1,
		PARAM_ELEMENTTYPE,
		PARAM_START,PARAM_END,PARAM_COLOR,
		PARAM_NUM,
	};

	//virtual
	ElementEnum			GetElementType(){return GT_SPECIAL_SIGN;};
	int					GetItemCount(){return PARAM_NUM;};
	HRESULT				GetValue(int index, CString& strValue);
	HRESULT				SetParam(int index, LPCSTR szValue) ;

	const CString*		GetItemStringArray();
	const CString*		GetDescriptionStringArray() ;
};

#endif // !defined(AFX_CSPECIALSIGN_H__60C3B5D2_9B4C_44D5_9863_0E4FF5DE8389__INCLUDED_)
