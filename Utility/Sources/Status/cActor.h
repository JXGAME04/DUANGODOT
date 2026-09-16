// cActor.h: interface for the cActor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CACTOR_H__A3B89512_073B_44AD_8C2B_5ECDC332D26B__INCLUDED_)
#define AFX_CACTOR_H__A3B89512_073B_44AD_8C2B_5ECDC332D26B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//the simplyest object
#include "iGraphics.h"

enum eAnimation {ANI_NULL = -1,ANI_WALK,ANI_WAIT,ANI_NUM};
struct stAniInfo
{
	eAnimation eAni;
	char strFile[MAX_CHAR];
	int nFrames;
	int nDirections; //should be 8
};

class cActor  
{
public:
	cActor();
	virtual ~cActor();
//com
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;
	BOOL IsInGround();
protected:
	iGraphics* m_pGraphics;
	iGraphics* GetGraphics(){return m_pGraphics;}
	iCamera* GetCamera(){return m_pGraphics->GetiCamera();}
	iGround* GetGround(){return m_pGraphics->GetiGround();}
	
	float m_fHeight;
	float GetHeight(){return m_fHeight;}
	void SetHeight(float f){m_fHeight = f;}
	iObj* m_pObject;
	iObj* m_pObjectRedraw;
	iObj* m_pShadow;
	iObj* m_pLight;
	bool m_bMainActor; //if true, Camera should follow object!
	bool m_bRedraw;		//if true, redraw use 50% alpha

	BOOL IsSeamObtacle();

//GAME ELEMENT
	int m_nAniInfo;
	eAnimation m_eAni;
	stAniInfo m_aAniInfo[ANI_NUM];
	int GetFrames(){return m_aAniInfo[m_eAni].nFrames;}
	int GetDirections(){return m_aAniInfo[m_eAni].nDirections;}
	int GetFramesOfDirection(){return GetFrames()/GetDirections();}
	const char* GetAnimationFile(eAnimation eAni);
	void ChangeAnimation(eAnimation eAni);
	void IncreaseFrame();
	
	enum eStatus{STATUS_NULL,STATUS_WALK,STATUS_WAIT,STATUS_BLOCK,STATUS_JUMP,STATUS_JUMPBLOCK};
	eStatus m_eStatus;
	enum eDirection{D_N,D_NW,D_W,D_SW,D_S,D_SE,D_E,D_NE,};
	void Turn(eDirection d);
	eDirection m_eDirection;
	cPoint m_ptGoal;
	float m_fSpeed;
	void SetSpeed(float f){m_fSpeed = f;}
	cPoint m_vSpeed;
	
	float m_fSpeedZ0;	//z的初速度
	float m_fSpeedZa;	//z的加速度

	cPoint m_vBlockSpeed;
	int m_nBlock;
	bool m_bClockWise;

	float m_fAniSpeed;
	int	 m_iFrame;
	void Clear();

public:
	void Start();
	virtual BOOL Run();
	void End();
	std::list<cPoint> m_listPath;
	void WalkTo(cPoint pt);
	void JumpTo(cPoint pt);
	void TurnTo(cPoint pt);
	void TurnToSpeed(cPoint pt);
	void Create(iGraphics* p, cPoint ptOffset, LPCSTR szInitAni);
	void SetMainActor(bool b){m_bMainActor = b;}

	void FormatSpeed(cPoint& m_vSpeed);
	BOOL AddSpeed(cPoint m_vSpeed);
	void AddAnimation(eAnimation eAni, const char* file);
	void SetAnimationDirection(int nDirection);

	CString m_strAni;
	void GetCurrentAnimation(CString& str);
	bool m_bSetCurrentAnimation;
	void SetCurrentAnimation(const CString& str);
	void Add();
	void Remove();
	void Offset(cPoint pt);
	void SetPosition(cPoint pt);
	cPoint GetPosition();
	CString m_strName;
	void SetName(LPCSTR s){m_strName = s;}
	LPCSTR GetName(){return m_strName;}

	//light
	void EnableLight(bool bEnable);
	void SetLightRange(float r);
	float GetLightRadius();
	void SetLightColor(cColor cr);
	cColor GetLightColor();

	void SetCameraFollow(bool bFollow);
};

#endif // !defined(AFX_CACTOR_H__A3B89512_073B_44AD_8C2B_5ECDC332D26B__INCLUDED_)
