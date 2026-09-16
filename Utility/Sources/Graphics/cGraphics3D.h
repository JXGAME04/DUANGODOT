// cGraphics3D.h: interface for the cGraphics3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGRAPHICS3D_H__CCB03E50_0333_48E1_A352_979E65202F7B__INCLUDED_)
#define AFX_CGRAPHICS3D_H__CCB03E50_0333_48E1_A352_979E65202F7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cGraphics.h"

class cGraphics3D : public cGraphics , public CD3DApplication
{
public:
	cGraphics3D();
	virtual ~cGraphics3D();
	
	//get 
	LPDIRECT3DDEVICE8 GetDevice3D(){return m_pd3dDevice;}
	void GetScreen(int& x,int &y);
	
	//paint loop
	HRESULT GraphicsRun();
	virtual BOOL IsPaint();
	virtual HRESULT BeginPaint() ;
	virtual HRESULT EndPaint() ;
	
	//factory
	virtual HRESULT CreateGraphicsElement(ElementEnum id,LPVOID ppoint);
	virtual HRESULT GetDevice(LPVOID ppoint) ;
	
	//create device
	virtual HRESULT CreateDevice(const stParam& screen);
	
	//save a bmp
	virtual HRESULT SaveBmp(const char* bmp_name, cRect rect);
	virtual HRESULT SaveBmpBuffer(DWORD* buffer, cRect rect) ;
	
	// Overridable functions for the 3D scene created by the app
    virtual HRESULT ConfirmDevice(D3DCAPS8*,DWORD,D3DFORMAT);
    virtual HRESULT OneTimeSceneInit()                         { return S_OK; }
    virtual HRESULT FrameMove()                                { return S_OK; }
	
    virtual HRESULT InitDeviceObjects();                        
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT FinalCleanup();
    virtual HRESULT CleanDevice(){Cleanup3DEnvironment(); return S_OK;};
	
	virtual HRESULT ToggleFullscreen() ;
	virtual HRESULT ResizeEnvironment();	
	
	HRESULT	PrintScreen(LPCSTR szFile);//½Ø»ñÆÁÄ»
	
	HRESULT DXError(HRESULT hr);

//msg haddle
    HRESULT Render();
	LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	HRESULT SaveBmpBuffer32(DWORD* pbuffer, cRect rect);
	HRESULT SaveBmpBuffer16(DWORD* pbuffer, cRect rect);
};


#endif // !defined(AFX_CGRAPHICS3D_H__CCB03E50_0333_48E1_A352_979E65202F7B__INCLUDED_)
