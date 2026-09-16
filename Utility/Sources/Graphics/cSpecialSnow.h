// cSpecialSnow.h: interface for the cSpecialSnow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSPECIALSNOW_H__F5FC91BE_A4C3_4672_8C45_905B4232AE88__INCLUDED_)
#define AFX_CSPECIALSNOW_H__F5FC91BE_A4C3_4672_8C45_905B4232AE88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cSpecial.h"


class cSpecialSnow : public cSpecial  
{
public:
	cSpecialSnow(cGraphics* p);
	virtual ~cSpecialSnow();

//load and save
	HRESULT				LoadBinary(CFile& f);
	HRESULT				SaveBinary(CFile& f);

protected:
	enum PathEnum{PATH_DEFAULT,PATH_LINE,PATH_WAVE,PATH_SWING,PATH_CIRCLE,};
	struct stParam
	{
//texture data
		ElementEnum			eElement;
		//雪的图片必须一张，但是可能包含几张雪的图片(大小一样)，通过切换贴图坐标来切换图片
		char				szImage[MAX_CHAR];	//雪的图片
		int					nImageFrames;		//不一定正好放下，所以使用frames确定有多少张雪花
		CPoint				ptImageSize;		//雪的图片一帧的大小,如此可以算出当前帧的贴图位置
		int					nImagespeed;		//切换的速度
//phicics data
//		cPoint				ptWindSpeed;		//风的速度
		float				fAcceleration;		//重力加速度
		float				fInitZ;				//从多高下落
		cPoint				ptInitSpeed;		//初始速度

		PathEnum			ePath;				//运动轨迹

		float				fSize;				//一般的雪花有多大
		float				fScale;				//放缩的因子
		cColor				color;
		cRect				rcRange;			//雪覆盖多大的地图(超出判作死亡, z<0 同样)

		int					nNumber;			//粒子的密集程度
		int					nFrameNumber;		//在开始的时候，多少的粒子往下落
	};
	stParam m_param;
	enum	ParamEnum{
		PARAM_NULL = -1,
		PARAM_ELEMENTTYPE,
		PARAM_IMAGE,PARAM_IMAGEFRAME,PARAM_IMAGESIZE,PARAM_IMAGESPEED,
		PARAM_ACCELERATION,PARAM_INITZ,PARAM_INITSPEED,
		PARAM_PATH,PARAM_SIZE,PARAM_SCALE,PARAM_COLOR,PARAM_RANGE,
		PARAM_NUMBER,PARAM_FRAMENUMBER,
		PARAM_NUM,
	};

	struct stSnow
	{
		cPoint ptSpeed;
		cPoint ptPos;
	};
	typedef std::vector<stSnow>	SnowVector;
	typedef SnowVector::iterator itSnow;
	SnowVector			m_aSnow;
//vitrual
	ElementEnum			GetElementType(){return GT_SPECIAL_SNOW;};
	int					GetItemCount(){return PARAM_NUM;};
	virtual HRESULT		GetValue(int index, CString& strValue) ;
	virtual HRESULT		SetParam(int index, LPCSTR szValue) ;

	UINT				m_nStart;//轮到哪一个雪花下落了
	void				InitParam();
//buffer and texture
	void				FindTexture();
	void				CreateBuffer();
	LPDIRECT3DVERTEXBUFFER8 m_pVB;
	iTexture*			m_pTextureSnow;
//run
	BOOL				Active();	
	BOOL				Run();
	void				RunSnow(stSnow& snow);			//单个的snow运动
//draw
	cRect				m_rcGround;//地图上的范围
	BOOL				IsDraw(DrawEnum e); //是否绘制
	HRESULT				Draw(DrawEnum e);
//device
    HRESULT				InvalidateDeviceObjects();
    HRESULT				RestoreDeviceObjects();
    HRESULT				DeleteDeviceObjects();
    HRESULT				FinalCleanup();
//string
	const CString*		GetItemStringArray();
	const CString*		GetDescriptionStringArray() ;
};

#endif // !defined(AFX_CSPECIALSNOW_H__F5FC91BE_A4C3_4672_8C45_905B4232AE88__INCLUDED_)
