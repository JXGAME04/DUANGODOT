// cCamera3D.h: interface for the cCamera3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCAMERA3D_H__7B346EDD_7000_46BC_9ED5_F60DBED5E217__INCLUDED_)
#define AFX_CCAMERA3D_H__7B346EDD_7000_46BC_9ED5_F60DBED5E217__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCamera.h"
class cCamera3D  : public cCamera
{
public:
	cCamera3D(cGraphics* p);
	virtual ~cCamera3D();

private:
	D3DXVECTOR3 m_vEyePt;       // Attributes for view matrix
	D3DXVECTOR3 m_vLookatPt;
	D3DXVECTOR3 m_vUpVec;
	
	D3DXVECTOR3 m_vView;
	D3DXVECTOR3 m_vCross;
	
	D3DXMATRIX  m_matView;
	D3DXMATRIX  m_matBillboard; // Special matrix for billboarding effects
	
	FLOAT       m_fFOV;         // Attributes for projection matrix
	FLOAT       m_fAspect;
	FLOAT       m_fNearPlane;
	FLOAT       m_fFarPlane;
	D3DXMATRIX  m_matProj;
	
	D3DXMATRIX  m_matScale;
	
public:
	// get 
	
	D3DXVECTOR3 GetEyePt()           { return m_vEyePt; }
	D3DXVECTOR3 GetLookatPt()        { return m_vLookatPt; }
	D3DXVECTOR3 GetUpVec()           { return m_vUpVec; }
	D3DXVECTOR3 GetViewDir()         { return m_vView; }
	D3DXVECTOR3 GetCross()           { return m_vCross; }
	
//	D3DXMATRIX  GetViewMatrix()      { return m_matView; }
//	D3DXMATRIX  GetBillboardMatrix() { return m_matBillboard; }
//	D3DXMATRIX  GetProjMatrix()      { return m_matProj; }
//	D3DXMATRIX  GetScaleMatrix()     { return m_matScale; }
	
	//set matrix	
	virtual VOID SetViewParams( D3DXVECTOR3 &vEyePt, D3DXVECTOR3& vLookatPt,
		D3DXVECTOR3& vUpVec );
	virtual VOID SetProjParams( FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane,
		FLOAT fFarPlane );
	virtual VOID SetScaleParams( FLOAT fScale);

	// device
	LPDIRECT3DDEVICE8 GetDevice();
	virtual HRESULT InitDeviceObjects();

//	void ScreenToGround(float x,float y,float &tx,float &ty);
	void GroundToScreen(float tx,float ty,float tz,float& x, float& y, float& z);
//	void GroundToScreen(float tx,float ty,float& x, float& y);	
	void ScreenToGround(float x,float y, float z,float &tx,float &ty, float &tz)  ;

	D3DXMATRIX m_matWorld,m_matProjAndScale,m_matScreen;
	D3DXMATRIX m_matIWorld,m_matIView,m_matIProjAndScale,m_matIScreen;
	D3DXMATRIX m_matTrans,m_matITrans;
	
	HRESULT SetMatrix();	
	HRESULT UpdateMatrix();
};

#endif // !defined(AFX_CCAMERA3D_H__7B346EDD_7000_46BC_9ED5_F60DBED5E217__INCLUDED_)
