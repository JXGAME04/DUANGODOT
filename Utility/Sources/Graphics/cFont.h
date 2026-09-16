// cFont.h: interface for the cFont class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFONT_H__9B19BA6E_686B_4FAC_9AE1_A83D68C0B038__INCLUDED_)
#define AFX_CFONT_H__9B19BA6E_686B_4FAC_9AE1_A83D68C0B038__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

/*
For text, the D3DX library provides us with the ID3DXFont interface, 
which works in a similar way to the ID3DXSprite interface. 
You need to pass the D3DXCreateFont() function a standard GDI Font Handle (HFONT), 
and in return you get a pointer to an ID3DXFont interface that has one main function, DrawText(). 
This function mimics the GDI DrawTextEx() very closely, such that the parameters are almost identical. 
In fact, the DirectX 8 SDK has a documentation error on the return value from this function, 
and the actual return value is the same as in the GDI function -- the return value is the text height of the drawn text. 
The visual quality and spacing of text output from this function is excellent, and there are lots of formatting options, 
but the drawback is speed. Under the hood,
the ID3DXFont::DrawText() function actually does what I discussed above, 
which is to create a GDI-compatible bitmap, 
draw the text to the bitmap, and then copy the bitmap to textures, and render the textures to the screen. 
So you get all the sluggishness of the original GDI functions, combined with a lot of overhead -- 
in the end, this function is up to 6 times slower than the original GDI DrawTextEx() was.
*/
/*
The DirectX 8 SDK provides a sample that offers a solution to this -- 
the "3D Text" sample program defines a class called CD3DFont, 
which implements a much faster, though less robust, text solution. 
It uses GDI only at the time of initialization to create a master texture of available characters, 
which is then used to draw each individual character from the texture, not requiring further calls to GDI. 
You lose some nice features such as font kerning and formatting options, 
but you can always customize the CD3DFont routines to meet your needs
*/

class CD3DFont;

class cFont : public iFont  
{
public:
	cFont(cGraphics* p);
	virtual ~cFont();

//com interface	
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;

//Graphics
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}	

//device 
    virtual HRESULT InitDeviceObjects(){return S_OK;};
    virtual HRESULT RestoreDeviceObjects(){return S_OK;};
    virtual HRESULT InvalidateDeviceObjects(){return S_OK;};
    virtual HRESULT DeleteDeviceObjects(){return S_OK;};
//create	
	virtual HRESULT Create(LPCSTR strFontName, DWORD dwHeight, DWORD dwFlags=0L) = 0;
//draw
    virtual HRESULT DrawText(const stParam& param, LPCSTR strText, int nLength) = 0;
};

#endif // !defined(AFX_CFONT_H__9B19BA6E_686B_4FAC_9AE1_A83D68C0B038__INCLUDED_)
