// cSpecial.h: interface for the cSpecial class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSPECIAL_H__BA80C242_6665_4F50_8CB2_F891ABE4B28D__INCLUDED_)
#define AFX_CSPECIAL_H__BA80C242_6665_4F50_8CB2_F891ABE4B28D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

class cGraphics;

struct POINTVERTEX
{
    D3DXVECTOR3 v;
    D3DCOLOR    color;
};
#define D3DFVF_POINTVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)
inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }


class cSpecial : public iSpecial  
{
public:
	cSpecial(cGraphics* p);
	virtual ~cSpecial();

//graphics
	cGraphics*			m_pGraphics;
	cGraphics*			GetGraphics(){return m_pGraphics;}
	cScreen*			GetScreen();
	cGround*			GetGround();
	cCamera*			GetCamera();

//com
	ULONG				m_ulCount;
	ULONG				AddRef() ;
	ULONG				Release() ;
//active
	bool				m_bActive;
	BOOL				IsActive(){return m_bActive;};			//是否激活

//device
	HRESULT				InitDeviceObjects(){return S_OK;};
    HRESULT				RestoreDeviceObjects(){return S_OK;};                     
    HRESULT				InvalidateDeviceObjects(){return S_OK;};
    HRESULT				DeleteDeviceObjects(){return S_OK;};
    HRESULT				FinalCleanup(){return S_OK;};

//select
	BOOL				m_bSelect;
	void				Select(BOOL bSelect){m_bSelect = bSelect;};	//被编辑选中
//draw
	BOOL				IsDraw(DrawEnum e){return FALSE;}; //是否绘制
	HRESULT				Draw(DrawEnum e){return S_OK;};
	
//event
	BOOL				IsAcceptEvent(EventEnum e){return FALSE;};
	HRESULT				OnEvent(EventEnum e, long lParam, long wParam){return S_OK;};
	
//load and save
	HRESULT				Load(const stSection* p);
	HRESULT				Save(stSection* p);

//param
	HRESULT				SetParam(LPCSTR szItem, LPCSTR szValue);
	HRESULT				GetParam(LPCSTR szItem, LPSTR szValue);
	HRESULT				GetParam(LPCSTR szItem, CString& szValue);
	HRESULT				GetParam(LPCSTR szItem, std::string& szValue)
	{
		CString s;
		HRESULT hr = GetParam(szItem,s);
		szValue = s;
		return hr;
	}
	HRESULT				GetParam(int index, CString& szItem, CString& szValue, CString& szDescription);
	HRESULT				GetParam(int index, std::string& szItem, std::string& szValue, std::string& szDescription) 
	{
		CString sItem,sValue,sDescription;
		HRESULT hr = GetParam(index,sItem,sValue,sDescription);
		szItem = sItem;
		szValue = sValue;
		szDescription = sDescription;
		return hr;
	}

	virtual HRESULT		GetValue(int index, CString& strValue) = 0;
	virtual HRESULT		SetParam(int index, LPCSTR szValue) = 0;

//utility
	int time;
	virtual cPoint		GetDrawPoint();
	virtual HRESULT		Draw();				//绘制自身为了编辑操作
	virtual BOOL		Contain(cPoint ptGround);
	
//string
	virtual const CString*	GetItemStringArray() = 0;
	virtual const CString*	GetDescriptionStringArray() = 0;

	virtual int			FindIndex(LPCSTR szItem);
	virtual HRESULT		GetDescription(int index, CString& strDescription);
	virtual HRESULT		GetItem(int index, CString& strItem);
};

#endif // !defined(AFX_CSPECIAL_H__BA80C242_6665_4F50_8CB2_F891ABE4B28D__INCLUDED_)
