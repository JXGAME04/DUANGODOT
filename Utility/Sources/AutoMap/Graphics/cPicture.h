// cPicture.h: interface for the cPicture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPICTURE_H__C78AAFD1_8D49_4F95_B687_76CF766543DD__INCLUDED_)
#define AFX_CPICTURE_H__C78AAFD1_8D49_4F95_B687_76CF766543DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
#include "cScreenObj.h"


//no need load and save for it is not in mapedit

class cPicture  : public cScreenObj
{
public:
	cPicture(cGraphics* p);
	virtual ~cPicture();
	virtual void				Init(bool bCreateBuffer);
	
//param	
	stPictureParam*				GetPictureParam(){return (stPictureParam*) GetObjParam();};
	HRESULT						OnCommand(long eCommand,stObjParam& param);
	HRESULT						setParam(stObjParam& pm) ;
	int							GetLayer(){return GetPictureParam()->nLayer;}

	int							GetiFrame(){return GetPictureParam()->stResA.stRes.iFrame;}
	RECT						GetRectSub(){return GetPictureParam()->rcSub;}

//vertex	
	stVertexScreen m_aVS[4];
	const stVertexScreen*		GetVertexBuffer(){return m_aVS;}	
	int							GetPictureWidth();
	int							GetPictureHeight();
	void						GetScreenRect(cRect& r);
	void						UpdateVertexBuffer();
	void						Offset(CPoint pt);
	
//texture
	iTexture* m_pTexture;
	iTexture*					GetTexture();
//	void						SetTexture(iTexture* p);
//draw
//	bool						IsDraw();
	BOOL						IsShow();
	HRESULT						Draw();
	BOOL						m_bHide;
	void						SetHide(BOOL b){m_bHide = b;}
	BOOL						IsHide(){return m_bHide;};

//根据几何信息判断是否显示
	bool						m_bDisplay;
	bool						IsDisplay(){return m_bDisplay;};
	bool						CheckDisplay(const CRect* rc);
//device
	HRESULT						FinalCleanup();

//contain
	virtual BOOL				Contain(POINT pt){return FALSE;}
	virtual BOOL				Contain(cPoint pt){return FALSE;}
	//包含在一个屏幕矩形中
	BOOL						ContainedScreen(RECT rc){return FALSE;}
	//包含在一个地图矩形中
	virtual BOOL				ContainedGround(cRect rc){return FALSE;}
	//包含在一个菱形中
	virtual BOOL				ContainedDiamond(cRect rc){return FALSE;}
};

#endif // !defined(AFX_CPICTURE_H__C78AAFD1_8D49_4F95_B687_76CF766543DD__INCLUDED_)
