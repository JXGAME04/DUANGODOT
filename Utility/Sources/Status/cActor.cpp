// cActor.cpp: implementation of the cActor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cActor.h"
#include <math.h>
#include "iGraphicsUtility.h"
#include "iImage.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
float Distance(cPoint p1,cPoint p2)
{
	cPoint p = p1 - p2;
	return (p.x * p.x + p.y * p.y + p.z * p.z);
}

cActor::cActor()
{
	m_ulCount = 1;

//	m_fSpeed = 0.2f;
//速度跟障碍的检测密切相关
	m_fSpeed = 0.3f;
	m_eStatus = STATUS_NULL;
	m_bMainActor = true;
	m_nAniInfo = 0;
	m_fAniSpeed = 1.f;
	
	m_bMainActor = false;
	m_bSetCurrentAnimation = true;
	m_pObject = NULL;
	m_pShadow = NULL;
	m_bRedraw = false;
	m_pObjectRedraw = NULL;
	m_pLight = NULL;
	m_eAni = ANI_NULL;
	m_iFrame = 0;
	m_fHeight = 0;
}

cActor::~cActor()
{
	Clear();
}

/**************************************************************************/
//com
/**************************************************************************/
ULONG cActor::AddRef() 
{
	return ++m_ulCount;
}

ULONG cActor::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

void cActor::TurnToSpeed(cPoint vSpeed)
{
	if (vSpeed.x == 0)
	{
		if (vSpeed.y > 0)
			Turn(D_S);
		else
			Turn(D_N);
		return;
	}

	float tga = vSpeed.y / vSpeed.x;
	const float pi = 3.14159265f;
	float alpha = atan(tga) / pi * 180.f;
	const float c_alpha = 22.5f;
	float index = alpha / c_alpha;

	int d = 0;
	if (index > -4 && index <= -3)
		d = D_S;
	else if (index > -3 && index <= -1)
		d = D_SE;
	else if (index > -1 && index <= 1)
		d = D_E;
	else if (index > 1 && index <= 3)
		d = D_NE;
	else if (index > 3 && index < 4)
		d = D_N;

	if (vSpeed.x < 0)
		d += 4;
	if (d >= 8)
		d -= 8;
	Turn((eDirection )d);
}

void cActor::TurnTo(cPoint pt)
{
	cPoint ptOffsetGround;
	GU_GetObjOffset(m_pObject,ptOffsetGround);
	cPoint vSpeed = pt - ptOffsetGround;
	TurnToSpeed(vSpeed);
}

void cActor::JumpTo(cPoint pt)
{
	cPoint ptOffsetGround;
	GU_GetObjOffset(m_pObject,ptOffsetGround);
	
	if (ptOffsetGround != pt)
	{
		m_vSpeed = pt - ptOffsetGround;
		float f = m_fSpeed / sqrt(m_vSpeed.x*m_vSpeed.x+m_vSpeed.y*m_vSpeed.y+m_vSpeed.z*m_vSpeed.z);
		m_vSpeed.x *= f;
		m_vSpeed.y *= f;
		m_vSpeed.z *= f;
		m_ptGoal = pt;

		m_eStatus = STATUS_JUMP;

		//计算z初速度和加速度
		//加速度恒定,初速度有最大值
		m_fSpeedZa = -0.04f;
		const int c_fMaxSpeed = 1.25f;
		
		float t = 1/f;
		m_fSpeedZ0 = - m_fSpeedZa * t / 2.f;

		//m_fSpeedZ0 = 1.f;
		if (m_fSpeedZ0 > c_fMaxSpeed)
			m_fSpeedZ0 = c_fMaxSpeed;

		TurnTo(pt);
	}
}

void cActor::WalkTo(cPoint pt)
{
	if (m_eStatus == STATUS_JUMP)
		return;

	cPoint ptOffsetGround;
	GU_GetObjOffset(m_pObject,ptOffsetGround);
	if (ptOffsetGround != pt)
	{
		m_listPath.clear();
		std::list<cPoint> listPath;
		if ( GetGround()->FindWalkPath(listPath,ptOffsetGround,pt) )
		{
			pt = listPath.front();
			listPath.pop_front();
			m_listPath = listPath;

			m_vSpeed = pt - ptOffsetGround;
			float f = m_fSpeed / sqrt(m_vSpeed.x*m_vSpeed.x+m_vSpeed.y*m_vSpeed.y+m_vSpeed.z*m_vSpeed.z);
			m_vSpeed.x *= f;
			m_vSpeed.y *= f;
			m_vSpeed.z *= f;
			m_ptGoal = pt;

			if (m_eStatus == STATUS_WALK || m_eStatus == STATUS_BLOCK)
				m_bClockWise = !m_bClockWise;
			m_eStatus = STATUS_WALK;

			ChangeAnimation(ANI_WALK);
			
			//get the vSpeed to direction
			TurnTo(pt);
		}
		else
		{

		}
	}
}

void cActor::AddAnimation(eAnimation eAni, const char* file)
{
	ASSERT(m_nAniInfo < ANI_NUM);
	m_aAniInfo[m_nAniInfo].eAni = eAni;
	strcpy(m_aAniInfo[m_nAniInfo].strFile ,file);
	iImage* pImage = Image_CreateImage(file);
	pImage->LoadFileInfo(file);
	stImageInfo info;
	pImage->GetImageInfo(info);
	m_aAniInfo[m_nAniInfo].nFrames = info.nFrames;
	m_aAniInfo[m_nAniInfo].nDirections = info.nDirections;
	pImage->Release();
	m_nAniInfo ++;
}

const char* cActor::GetAnimationFile(eAnimation eAni)
{
	for (int i=0; i<m_nAniInfo; i++)
	{
		if (m_aAniInfo[i].eAni == eAni)
			return m_aAniInfo[i].strFile;
	}
	ASSERT(0);
	return NULL;
}

void cActor::ChangeAnimation(eAnimation eAni)
{
	m_eAni = eAni;
}

void cActor::Turn(eDirection ed)
{
	m_eDirection = ed;
	SetAnimationDirection(ed);
}

void cActor::Clear()
{
	RELEASE(m_pObject);
	RELEASE(m_pShadow);
	RELEASE(m_pObjectRedraw);
	RELEASE(m_pLight);
}

void cActor::Create(iGraphics* p, cPoint ptOffset, LPCSTR szInitAni)
{
	m_pGraphics = p;

//npc
	GU_CreateMissle(GetGraphics(),m_pObject);
	ASSERT(m_pObject);
	GU_ChangeMissleAnimation(m_pObject,szInitAni);
	GU_ChangeMissleFrame(m_pObject,0);
	GU_OffsetObjTo(m_pObject,ptOffset);
	GU_SetObjDynamic(m_pObject,DYNAMIC_NOSAVE);

//shadow
	CString sAniShadow = szInitAni;
	sAniShadow = sAniShadow.Left(sAniShadow.GetLength() - 4) + "b.spr";
	CString strSpr = sAniShadow;
	AppGetFullPath(strSpr);
	CFileFind f;
	if (f.FindFile(strSpr))
	{
		GU_CreateMissle(GetGraphics(),m_pShadow);
		ASSERT(m_pShadow);
		GU_ChangeMissleAnimation(m_pShadow,sAniShadow);
		GU_ChangeMissleFrame(m_pShadow,0);
		GU_OffsetObjTo(m_pShadow,ptOffset);
		GU_SetObjDynamic(m_pShadow,DYNAMIC_NOSAVE);
	}

	GU_CreateLight(GetGraphics(),m_pLight);
	GU_SetLightRange(m_pLight,8.f);
	GU_SetObjDynamic(m_pLight,DYNAMIC_NOSAVE);
	ptOffset.y += 0.01f;
	GU_OffsetObjTo(m_pLight,ptOffset);
}

void cActor::SetAnimationDirection(int nDirection)
{
	int curframe = m_iFrame;
	int frameofdir = GetFramesOfDirection();
	int iframe = curframe % frameofdir;
	int index = nDirection * frameofdir + iframe;
	m_iFrame = index;
	GU_ChangeMissleFrame(m_pObject,index);
	GU_ChangeMissleFrame(m_pShadow,index);
	GU_ChangeMissleFrame(m_pObjectRedraw,index);
}

void cActor::Start()
{
	if (m_eStatus == STATUS_NULL)
	{
		ChangeAnimation(ANI_WAIT);
		Turn(D_N);

		m_eStatus = STATUS_WAIT;
	}
}

void cActor::IncreaseFrame()
{
	int dir = m_eDirection;
	if (m_eDirection >= GetDirections())
		dir = 0;
	static float fAni = 0.f;
	fAni ++;
	if (fAni >= m_fAniSpeed)
	{
		fAni -= m_fAniSpeed;

		int curframe = m_iFrame;
		int frameofdir = GetFramesOfDirection();
		int iframe = curframe % frameofdir;
		iframe++;
		if (iframe >= frameofdir)
			iframe = 0;
		int index = dir * frameofdir + iframe;
		m_iFrame = index;

		GU_ChangeMissleFrame(m_pObject,index);
		GU_ChangeMissleFrame(m_pShadow,index);
		GU_ChangeMissleFrame(m_pObjectRedraw,index);
	}
}

BOOL cActor::IsInGround()
{
	return m_pObject->IsInGraphics();
}

float DistanceSquare(cPoint p1,cPoint p2)
{
	return (p1.x-p2.x)*(p1.x-p2.x)+ (p1.y-p2.y)*(p1.y-p2.y) + (p1.z-p2.z)*(p1.z-p2.z);
}

void cActor::FormatSpeed(cPoint& m_vSpeed)
{
	cPoint ptOffset = GetPosition();
	ptOffset += m_vSpeed;
	cPoint ptOffsetDec = GetPosition();
	ptOffsetDec -= m_vSpeed;
	if (DistanceSquare(ptOffsetDec,m_ptGoal) < DistanceSquare(ptOffset,m_ptGoal))
	{
		m_vSpeed.x = -m_vSpeed.x;
		m_vSpeed.y = -m_vSpeed.y;
		m_bClockWise = !m_bClockWise;
	}
}

BOOL cActor::AddSpeed(cPoint m_vSpeed)
{
	float f = m_fSpeed / sqrt(m_vSpeed.x*m_vSpeed.x+m_vSpeed.y*m_vSpeed.y+m_vSpeed.z*m_vSpeed.z);
	m_vSpeed.x *= f;
	m_vSpeed.y *= f;
	m_vSpeed.z *= f;
	cPoint ptOffset = GetPosition();
	ptOffset += m_vSpeed;

	if (IsSeamObtacle() && !GetGraphics()->GetiGround()->CheckObstacle(ptOffset,m_vBlockSpeed,m_bClockWise) == OBSTACLE_NULL) 
	{
		return FALSE;
	}
	TurnToSpeed(m_vSpeed);
	SetPosition(ptOffset);
	IncreaseFrame();					
	return TRUE;
}

BOOL cActor::Run()
{
	if (m_bSetCurrentAnimation == false)
	{
		SetCurrentAnimation(m_strAni);
		m_bSetCurrentAnimation = true;
	}
	switch (m_eStatus)
	{
	case STATUS_BLOCK:
		{
			if (!IsSeamObtacle())
			{
				ChangeAnimation(ANI_WAIT);
				Turn(m_eDirection);
				m_eStatus = STATUS_WAIT;
				break;
			}
			if (GetPosition() == m_ptGoal)
			{
				m_eStatus = STATUS_WAIT;
				break;
			}

			if (++m_nBlock > 10)
			{
				m_nBlock = 0;
				cPoint ptOffset = GetPosition();
				m_vSpeed = m_ptGoal - ptOffset;
				float f = m_fSpeed / sqrt(m_vSpeed.x*m_vSpeed.x+m_vSpeed.y*m_vSpeed.y+m_vSpeed.z*m_vSpeed.z);
				m_vSpeed.x *= f;
				m_vSpeed.y *= f;
				m_vSpeed.z *= f;
				ptOffset+=m_vSpeed;
				cPoint ptSpeed;
				if (GetGraphics()->GetiGround()->CheckObstacle(ptOffset,ptSpeed,m_bClockWise) == OBSTACLE_NULL) 
				{
					TurnToSpeed(m_vSpeed);
					m_eStatus = STATUS_WALK;
				}
			}
			else
			{
				cPoint ptOld = m_vBlockSpeed;
				if (!AddSpeed(m_vBlockSpeed))
				{
//					if (AddSpeed(ptOld))
//					{
//						m_vBlockSpeed = ptOld;
//					}
//					else
//					{
					if (ptOld == m_vBlockSpeed)
					{
						m_vBlockSpeed.x = -m_vBlockSpeed.x;
						m_vBlockSpeed.y = -m_vBlockSpeed.y;
					}
					if (!AddSpeed(m_vBlockSpeed))
					{
						ChangeAnimation(ANI_WAIT);
						Turn(m_eDirection);
						m_eStatus = STATUS_WAIT;
					}
//					}
				}
			}
			//recompute the speed to goal,
			//if can walk then status_walk
			//else add speed
			break;
		}
	case STATUS_WALK:
		{
			cPoint ptOffset = GetPosition();
			if (Distance(ptOffset,m_ptGoal) > 0.1f)
			{
				ptOffset+=m_vSpeed;
				if (!GetGraphics()->GetiGround()->BlockObstacle(ptOffset,OBSTACLE_BLOCKWALK))
				{
					SetPosition(ptOffset);
					IncreaseFrame();
				}
				else
				{
					FormatSpeed(m_vBlockSpeed);
					m_eStatus = STATUS_BLOCK;
					m_nBlock = 0;
				}
			}
			else
			{
				if (m_listPath.empty())
				{
					ChangeAnimation(ANI_WAIT);
					Turn(m_eDirection);
					m_eStatus = STATUS_WAIT;
				}
				else
				{
					cPoint ptNext = m_listPath.front();
					m_listPath.pop_front();
					WalkTo(ptNext);
				}
			}
			break;
		}
	case STATUS_WAIT:
		break;
	case STATUS_JUMP:
		{
			cPoint ptOffset = GetPosition();
			if (Distance(ptOffset,m_ptGoal) > 0.1f && (ptOffset.z > 0.f || m_fSpeedZ0 > 0) )
			{
				ptOffset+=m_vSpeed;
				ptOffset.z += m_fSpeedZ0;
				if (ptOffset.z < 0)
					ptOffset.z = 0;
				m_fSpeedZ0 += m_fSpeedZa;
				if (!GetGraphics()->GetiGround()->BlockObstacle(ptOffset,OBSTACLE_BLOCKJUMP))
				{
					SetPosition(ptOffset);
					IncreaseFrame();
				}
				else
				{
					m_eStatus = STATUS_JUMPBLOCK;
					break;
				}
			}
			else
			{
				ChangeAnimation(ANI_WAIT);
				Turn(m_eDirection);
				m_eStatus = STATUS_WAIT;
			}
			break;
		}
	case STATUS_JUMPBLOCK:
		{
			cPoint ptOffset = GetPosition();
			if (ptOffset.z > 0.f)
			{
				ptOffset.z += m_fSpeedZ0;
				if (ptOffset.z < 0)
					ptOffset.z = 0;
				m_fSpeedZ0 += m_fSpeedZa;
				SetPosition(ptOffset);
				IncreaseFrame();
			}
			else
			{
				ChangeAnimation(ANI_WAIT);
				Turn(m_eDirection);
				m_eStatus = STATUS_WAIT;
			}
			break;
		}
	}
	return TRUE;
}

void cActor::End()
{
	m_eStatus = STATUS_NULL;
}

cPoint cActor::GetPosition()
{
	cPoint ptOffset;
	GU_GetObjOffset(m_pObject,ptOffset);
	return ptOffset;
}

void cActor::Offset(cPoint pt)
{
	cPoint ptOffsetGround;
	GU_GetObjOffset(m_pObject,ptOffsetGround);
	ptOffsetGround += pt;
	SetPosition(ptOffsetGround);
}

void cActor::SetPosition(cPoint pt)
{
	GU_OffsetObjTo(m_pObject,pt);
	GU_OffsetObjTo(m_pObjectRedraw,pt);
	pt.y += 0.01f;
	GU_OffsetObjTo(m_pLight,pt);
	pt.z = 0;
	GU_OffsetObjTo(m_pShadow,pt);
	if (m_bMainActor)
		GU_OffsetCameraTo(GetGraphics(),pt);
}

void cActor::GetCurrentAnimation(CString& str)
{
	str = m_strAni;
}

void cActor::SetCurrentAnimation(const CString& str)
{
	ASSERT(!str.IsEmpty());
	m_bSetCurrentAnimation = false;
	GU_ChangeMissleAnimation(m_pObject,str);
	CString sAniShadow = str;
	sAniShadow = sAniShadow.Left(sAniShadow.GetLength() - 4) + "b.spr";
	GU_ChangeMissleAnimation(m_pObject,sAniShadow);
	GU_ChangeMissleAnimation(m_pObjectRedraw,str);
	m_bSetCurrentAnimation = true;
}

void cActor::Add()
{
	GU_AddObj(GetGraphics(),m_pObject);
	GU_AddObj(GetGraphics(),m_pShadow);
	GU_AddObj(GetGraphics(),m_pObjectRedraw);
	GU_AddObj(GetGraphics(),m_pLight);
}

void cActor::Remove()
{
	GU_RemoveObj(GetGraphics(),m_pObject);
	GU_RemoveObj(GetGraphics(),m_pShadow);
	GU_RemoveObj(GetGraphics(),m_pObjectRedraw);
	GU_RemoveObj(GetGraphics(),m_pLight);
}

void cActor::EnableLight(bool bEnable)
{
}

void cActor::SetLightRange(float r)
{
	GU_SetLightRange(m_pLight,r);
}

float cActor::GetLightRadius()
{
	return 0;
}

void cActor::SetLightColor(cColor cr)
{
	GU_ChangeObjColor(m_pLight,cr);
}

cColor cActor::GetLightColor()
{
	return cColor(0,0,0,0);
}


void cActor::SetCameraFollow(bool bFollow)
{
	m_bMainActor = true;
}

BOOL cActor::IsSeamObtacle()
{
	iGround::stParam param;
	param.mask= 0;
	GetGround()->GetParam(param);
	return param.eObstalceType == OT_SEAM;
}
