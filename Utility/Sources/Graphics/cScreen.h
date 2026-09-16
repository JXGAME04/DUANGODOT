// cScreen.h: interface for the cScreen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSCREEN_H__AFA8995F_64F9_4C0E_BF2F_9E16CEDB11A2__INCLUDED_)
#define AFX_CSCREEN_H__AFA8995F_64F9_4C0E_BF2F_9E16CEDB11A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
class cMap;
class cMapSmall;

class cScreen : public iScreen  
{
public:
	cScreen(cGraphics* );
	virtual ~cScreen();
	
//com
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;
//begin and end
	virtual HRESULT Begin();
	virtual HRESULT End();	
//graphics
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}
//update screen's element 
	cPictureList m_listPictureBefore;	//先于ground绘制的picture
	cPictureList m_listPicture;			
//picture
	HRESULT AddiObj(iObj* p);
	HRESULT RemoveiObj(iObj* p);
	void AddcPicture(cPicture* p);
	void RemovecPicture(cPicture* p) ;
//param
	stParam m_param;
	HRESULT GetParam(stParam& param);
	HRESULT SetParam(stParam& param, bool bRunNow = false) ;
	HRESULT setParam(const stParam& param) ;
//trace
	void Trace();
//paint
	void Paint();
	void PaintBefore();
	void PaintSelect();
	void New();
/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
//device
	virtual HRESULT InitDeviceObjects(){return S_OK;};
    virtual HRESULT RestoreDeviceObjects(){return S_OK;};                     
    virtual HRESULT InvalidateDeviceObjects(){return S_OK;};
    virtual HRESULT DeleteDeviceObjects(){return S_OK;};
	virtual HRESULT FinalCleanup();
//draw
	//utility
	virtual void SetTexture(iTexture* pTex, int frame = 0) = 0;
	virtual void Draw(stVertexScreen* pV, int n = 4) = 0;//strip
	virtual void DrawFan(stVertexScreen* pV, int n = 4) = 0;
	virtual void DrawLine(stVertexScreen* pV, int n) = 0;
	virtual void DrawLine(CPoint p1,CPoint p2, cColor cr) = 0;
	virtual void DrawRect(stVertexScreen* pV, cColor cr) = 0;
	virtual void DrawRect(cRect& rc, cColor cr) = 0;
	virtual void DrawPoint(CPoint pt, cColor cr) = 0;	
	//simpleest draw
	virtual void Draw(iTexture* pTex, int frame, CPoint ptScreen) = 0;
	virtual void Draw(iTexture* pTex, int frame, CPoint ptScreen, cColor cr) = 0;

//map
	cMap* m_pMap;
	cMapSmall* m_pMapSmall;
	iObj* GetiMap();
	cMap* GetcMap(){return m_pMap;}
	iObj* GetiMapSmall();
	cMapSmall* GetcMapSmall(){return m_pMapSmall;}

//Environment
	virtual HRESULT SetEnvironment() {return S_OK;};
	virtual HRESULT ClearEnvironment() {return S_OK;};

	friend class cGraphics;
};

#endif // !defined(AFX_CSCREEN_H__AFA8995F_64F9_4C0E_BF2F_9E16CEDB11A2__INCLUDED_)
