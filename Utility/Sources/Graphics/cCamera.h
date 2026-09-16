// cCamera.h: interface for the cCamera class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCAMERA_H__6499B3D5_CB92_4917_9C50_E466159F46A2__INCLUDED_)
#define AFX_CCAMERA_H__6499B3D5_CB92_4917_9C50_E466159F46A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

class cGround;

class cCamera  : public iCamera
{
public:
	cCamera(cGraphics* p);
	virtual ~cCamera();

//com interface
	ULONG m_ulCount;
	ULONG AddRef() ;
	ULONG Release() ;

//begin and end
	virtual HRESULT Begin();
	virtual HRESULT End();

//graphics
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}
	cGround* GetGround();

//get
	float GetAngleZ()     { return m_param.fAngle; }
	float GetAngleX()     { return m_param.fAngleX; }
//param	
	stParam m_param;
	HRESULT GetParam(stParam& param) ;
	HRESULT SetParam(stParam& param, bool bRunNow = false) ;
	HRESULT setParam(stParam& param) ;
	cPoint GetFocus(){return m_param.ptTo;}
//trace
	void Trace();
//save and load
	HRESULT Save(LPCSTR szFile);
	HRESULT Load(LPCSTR szFile);
	void Init();
	HRESULT New();
	bool m_bNotify;
	ULONG UpdateCamera();
		
/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
	virtual VOID SetViewParams( D3DXVECTOR3 &vEyePt, D3DXVECTOR3& vLookatPt,
		D3DXVECTOR3& vUpVec ){};
	virtual VOID SetProjParams( FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane,
		FLOAT fFarPlane ){};
	virtual VOID SetScaleParams( FLOAT fScale){};

	virtual HRESULT InitDeviceObjects(){return S_OK;};
	
	virtual void ScreenToGround(float x,float y, float z,float &tx,float &ty, float &tz) = 0 ;
	virtual void ScreenToGround(stVertexGround p,stVertexGround& pt) ;	
	virtual void ScreenToGround(cPoint p,cPoint& pt) ;	
	virtual void ScreenToGround(CPoint p,cPoint& pt) ;	
	virtual void ScreenToGround(POINT p,cPoint& pt) {ScreenToGround(CPoint(p.x,p.y),pt);};	
	virtual void ScreenToGround(cPoint& pt) ;	

	virtual void GroundToScreen(float tx,float ty,float tz,float& x, float& y, float& z) = 0;
	virtual void GroundToScreen(stVertexGround p,stVertexGround& pt) ;	
	virtual void GroundToScreen(cPoint p,cPoint& pt) ;	
	virtual void GroundToScreen(cPoint p,CPoint& pt) ;	
	virtual void GroundToScreen(cPoint p,POINT& pt) {CPoint point; GroundToScreen(p,point); pt.x = point.x;pt.y = point.y;};	
	virtual void GroundToScreen(cPoint& pt) ;	

	virtual bool IsPointInScreen(cPoint p);

	virtual HRESULT SetMatrix(){return S_OK;}
	virtual HRESULT UpdateMatrix(){return S_OK;}
};

#endif // !defined(AFX_CCAMERA_H__6499B3D5_CB92_4917_9C50_E466159F46A2__INCLUDED_)
