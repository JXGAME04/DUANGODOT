// cMapSmall.h: interface for the cMapSmall class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMAPSMALL_H__868A2275_16CC_4017_B8EF_C5BDAB630198__INCLUDED_)
#define AFX_CMAPSMALL_H__868A2275_16CC_4017_B8EF_C5BDAB630198__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cScreenObj.h"
class iImage;

class cMapSmall  : public cScreenObj
{
public:
	cMapSmall(cGraphics* p);
	virtual ~cMapSmall();
	void Init(bool bCreateBuffer);
	void InitVB();

//if click on the small map, should give camera a offset!
	BOOL						IsShow();
	HRESULT						Draw();
	HRESULT						DrawTexture();
	CString						m_strMap;
	LPDIRECT3DTEXTURE8			m_pTexture;

	stVertexScreen				m_aV[4];
	void						RenderTexture();
	BOOL						VerifyTexture(BOOL bForceUpdate = FALSE);

	iImage*						m_pImage;
	HRESULT						LoadBmp();
	BOOL						m_bUpdate;

	CRect						m_rcRegion;			//get 'rect' from the txt

	CRect						m_rcMap;			//rc of the map!
	CRect						m_rcImage;			//the rcMap to whole image
	
	CPoint						m_ptTexture;		//the rcMap to texture

	CPoint						m_ptLoad;			//and load size is always 512*512! and ptLoad always = 32 * n

	void						ScreenToGround(CPoint ptScreen, cPoint& ptGround);
	void						ScreenToGround(int x,int y, long & nx,long & ny);
	void						GroundToScreen(int x,int y, long & nx,long & ny);
	
//param	
	stMapSmallParam*			GetMapParam(){return (stMapSmallParam*) GetObjParam();};
	HRESULT						setParam(stObjParam& param) ;
	HRESULT						OnCommand(long Command,stObjParam& param);
	BOOL						IsFull(){return GetMapParam()->bFull;}

//large map
//	LPDIRECT3DTEXTURE8			m_pTextureLarge;	//also 512 * 512 
//	void						ScreenToGroundLarge(CPoint ptScreen, cPoint& ptGround);
//	BOOL						VerifyTextureLarge();

//notify
	HRESULT						OnFileOpen(LPCSTR szMap);
	HRESULT						OnCameraUpdate();

	HRESULT						FinalCleanup();

};

#endif // !defined(AFX_CMAPSMALL_H__868A2275_16CC_4017_B8EF_C5BDAB630198__INCLUDED_)
