// cSpecialScroll.h: interface for the cSpecialScroll class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSPECIALSCROLL_H__03C2C2C0_ABD2_418F_AD2F_4013258FACB7__INCLUDED_)
#define AFX_CSPECIALSCROLL_H__03C2C2C0_ABD2_418F_AD2F_4013258FACB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cSpecial.h"

class cPicture;

class cSpecialScroll : public cSpecial  
{
public:
	cSpecialScroll(cGraphics* p);
	~cSpecialScroll();
//load and save
	HRESULT				LoadBinary(CFile& f);
	HRESULT				SaveBinary(CFile& f);
//run
	BOOL				Active();	
	BOOL				Run();
	void				Clear();
	void				CreatePicture();

//data;
	enum {MAX_IMAGE = 4};
protected:
	struct stParam
	{
		ElementEnum			eElement;
		char				aImage[MAX_IMAGE][MAX_CHAR];//卷轴的图片
		int					aImageFrame[MAX_IMAGE];//卷轴的图片是第几帧
		cPoint				ptStart;//卷轴的起始点
		CPoint				ptSize;	//卷轴的区域大小			
		cPoint				ptSpeed;//卷轴的速度
		bool				bLoop;//是否循环
		cPoint				ptCameraInit;	//最初的时候camera的位置
		cPoint				vSpeedMove; //自身运动的速度
		BOOL				bBackGround; //背景还是前景
	};
	stParam m_param;
	cPicture*			m_aPicture[MAX_IMAGE];	//两张图构成循环!
	int m_nFrame;

private:
	enum	ParamEnum{
		PARAM_NULL = -1,
		PARAM_ELEMENTTYPE,
		PARAM_IMAGE0,PARAM_IMAGE1,PARAM_IMAGE2,PARAM_IMAGE3,
		PARAM_IMAGE0FRAME,PARAM_IMAGE1FRAME,PARAM_IMAGE2FRAME,PARAM_IMAGE3FRAME,
		PARAM_START,PARAM_SIZE,PARAM_SPEED,PARAM_LOOP,PARAM_CAMERASTART,
		PARAM_SPEEDMOVE,PARAM_BACKGROUND,
		PARAM_NUM,
	};

	cPoint				GetDrawPoint();
	cRect				GetScrollRect();

//virtual
	ElementEnum			GetElementType(){return GT_SPECIAL_SCROLL;};
	int					GetItemCount(){return PARAM_NUM;};
	HRESULT				GetValue(int index, CString& strValue);
	HRESULT				SetParam(int index, LPCSTR szValue) ;
	
	const CString*		GetItemStringArray();
	const CString*		GetDescriptionStringArray() ;
};

#endif // !defined(AFX_CSPECIALSCROLL_H__03C2C2C0_ABD2_418F_AD2F_4013258FACB7__INCLUDED_)
