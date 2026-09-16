#pragma once

#include "cGroundObj.h"

class cSelect :	public cGroundObj
{
public:
	cSelect(cGraphics* p);
	virtual ~cSelect();
	virtual void				Init(bool bCreateBuffer);

	stSelectParam& GetSelectParam(){return *((stSelectParam*) m_pParam);};
	HRESULT setParam(stObjParam& pm) ;
	SelectEnum GetType(){return GetSelectParam().eSelect;}
	float GetSelectWidth(){return GetSelectParam().wSelect;}
	float GetSelectHeight(){return GetSelectParam().hSelect;}
	cPoint GetOffsetEnd(){return GetSelectParam().ptOffsetEnd;}
//Draw
	BOOL	IsShow(){return TRUE;}
	HRESULT Draw();
	bool	CheckDisplay(const cRect* rc) ;

	virtual BOOL				Contain(POINT pt){return FALSE;}
	virtual BOOL				Contain(cPoint pt){return FALSE;}
	//包含在一个屏幕矩形中
	BOOL						ContainedScreen(RECT rc){return FALSE;}
	//包含在一个地图矩形中
	virtual BOOL				ContainedGround(cRect rc){return FALSE;}
	//包含在一个菱形中
	virtual BOOL				ContainedDiamond(cRect rc){return FALSE;}	
};
