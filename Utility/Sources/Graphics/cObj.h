// cObj.h: interface for the cObj class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COBJ_H__7CC1A12C_0FF5_4F54_87FF_1EC032DFB611__INCLUDED_)
#define AFX_COBJ_H__7CC1A12C_0FF5_4F54_87FF_1EC032DFB611__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

class cCamera;
class cGround;
class cScreen;

class cObj : public iObj  
{
public:
	cObj(cGraphics* p);
	virtual ~cObj();

//com
	ULONG						m_ulCount;
	ULONG						AddRef() ;
	ULONG						Release() ;
	virtual void				Init(bool bCreateBuffer);

//graphics 
	cGraphics*					m_pGraphics;
	cGraphics*					GetGraphics(){return m_pGraphics;}
	LPDIRECT3DDEVICE8			GetDevice();

//load and save
//	virtual HRESULT				Load(LPCSTR szFile, int index);
//	virtual HRESULT				Save(LPCSTR szFile, int index);
	virtual HRESULT				Load(const stSection* p);
	virtual HRESULT				Save(stSection* p);
	virtual HRESULT				LoadPack(void** pParam);
	virtual HRESULT				LoadBinary(CFile& f);
	virtual HRESULT				SaveBinary(CFile& f);

//param
	virtual HRESULT				GetParam(stObjParam& param) ;
	HRESULT						SetParam(stObjParam& param,bool bRunNow) ;//将消息放入队列
	virtual HRESULT				setParam(stObjParam& param) ;
	virtual HRESULT				OnCommand(long eCommand,stObjParam& param);//将消息放入队列

//element
	stObjParam*					m_pParam;
	stObjParam*					GetObjParam(){return m_pParam;};

	ElementEnum					GetElementType(){return GetObjParam()->eElement;};
	cColor						GetColor() {return GetObjParam()->crColor;};
	void						SetColor(cColor cr){GetObjParam()->crColor = cr;}
	long						GetMapedit() {return GetObjParam()->lMapeditEnum;};
	BOOL						IsMapeditBitSet(int bit) {return GetObjParam()->lMapeditEnum & bit;};

	long						GetParamSize(){return GetObjParam()->size;}
//	long						GetParamMask(){return GetObjParam()->mask;}

//virtual function
	virtual BOOL				IsShow() = 0;
	virtual HRESULT				Draw() = 0;

//device
	virtual HRESULT				InitDeviceObjects(){return S_OK;};
    virtual HRESULT				RestoreDeviceObjects(){return S_OK;};                     
    virtual HRESULT				InvalidateDeviceObjects(){return S_OK;};
    virtual HRESULT				DeleteDeviceObjects(){return S_OK;};
    virtual HRESULT				FinalCleanup(){return S_OK;};
};
#endif // !defined(AFX_COBJ_H__7CC1A12C_0FF5_4F54_87FF_1EC032DFB611__INCLUDED_)
