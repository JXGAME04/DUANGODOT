// cStatusTest.cpp: implementation of the cStatusTest class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusTest.h"
#include "iGraphicsUtility.h"
#include "cActor.h"
#include "fileop.h"

#include "cMissle.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cStatusTest::cStatusTest()
{
	m_pNpc = NULL;
	m_pMenu = NULL;
	m_eRun = RUN_USER;
	for (int i=0; i<TEST_NUM; i++)
	{
		m_aNpcTest[i] = NULL;
	}
}

cStatusTest::~cStatusTest()
{
	Clear();
}

void cStatusTest::Clear()
{
	if (m_pNpc)
	{
		m_pNpc->Remove();
		RELEASE(m_pNpc);
	}
	if (m_pMenu)
	{
		GU_RemoveMenu(GetGraphics(),m_pMenu);
		RELEASE(m_pMenu);
	}
	for (int i=0; i<TEST_NUM; i++)
	{
		if (m_aNpcTest[i])
		{
			m_aNpcTest[i]->Remove();
			RELEASE(m_aNpcTest[i]);
		}
	}
	for(itMissle it = m_listMissle.begin(); it != m_listMissle.end(); ++it)
	{
		cMissle* pMissle = (*it);
		pMissle->Remove();
		RELEASE(pMissle);
	}
	m_listMissle.clear();
}

BOOL cStatusTest::Create()
{
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	m_pNpc = new cActor;
	m_pNpc->Create(GetGraphics(),param.ptTo,"system\\spr\\npc.spr");
	m_pNpc->SetMainActor(true);
	m_pNpc->AddAnimation(ANI_WALK,"system\\spr\\npc.spr");
	m_pNpc->AddAnimation(ANI_WAIT,"system\\spr\\npc.spr");
	m_pNpc->Start();
	m_pNpc->Add();
/*
//test menu
	CreateMenu(GetGraphics(),m_pMenu);
	SetMenuColor(m_pMenu,cColor(255,255,255,255));
	SetMenuSpr(m_pMenu,"system\\spr\\menu.spr",0);
	CPoint pt(100,500);
	OffsetMenuTo(m_pMenu,pt);
	AddMenu(GetGraphics(),m_pMenu);
//*/
	return TRUE;
}

BOOL cStatusTest::OnTimer(UINT nIDEvent)
{
	cStatus::OnTimer(nIDEvent);
	//IncreateObjFrame(m_pNpc,1);
//	return FALSE;
	static nTimer = 0;
	if (m_pNpc == NULL)
		return FALSE;
	
	m_pNpc->Run();
	for(itMissle it = m_listMissle.begin(); it != m_listMissle.end(); ++it)
	{
		cMissle* pMissle = (*it);
		if (pMissle->Run() == FALSE)
		{
			pMissle->Remove();
			RELEASE(pMissle);
			it = m_listMissle.erase(it);
		}
	}
	
	for (int i=0; i<TEST_NUM; i++)
	{
		if (m_aNpcTest[i])
		{
			if (!m_aNpcTest[i]->IsInGround())
			{
				RELEASE(m_aNpcTest[i]);
			}
			else
				m_aNpcTest[i]->Run();
		}
	}

	switch(m_eRun)
	{
	case RUN_RANDOM:
		{
			if (nTimer ++ > 40)
			{
				RandomWalk(m_pNpc);
				for (int i=0; i<TEST_NUM; i++)
				{
					if (m_aNpcTest[i])
					{
						RandomWalk(m_aNpcTest[i]);
					}
				}
				nTimer = 0;
			}
		}
		break;
	case RUN_TESTMAP:
		{
			if (nTimer ++ > 40)
			{
				static timer = 0;
				nTimer = 0;
				timer ++;
				int x,y;
				int ntime = 50;
				if (timer < ntime)
				{
					x = rand() % 500 + 300;
					y = rand() % 300 + 300;
				}
				else if (timer>= ntime && timer < ntime * 2)
				{
					x = rand() % 500;
					y = rand() % 300;
				}
				else if (timer >= ntime * 2)
					timer = 0;
				OnLButtonDown(0,CPoint(x,y));
			}
		}
		break;
	}
	return TRUE;
}

void cStatusTest::RandomWalk(cActor* pObj)
{
	cPoint ptOffset = pObj->GetPosition();
	ptOffset.x += rand() % 64 - 20;
	ptOffset.y += rand() % 64 - 20;
	pObj->WalkTo(ptOffset);
}

BOOL cStatusTest::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_pNpc == NULL)
		return FALSE;

	cPoint pt;
	GetCamera()->ScreenToGround(point,pt);
	m_pNpc->TurnTo(pt);

	cPoint ptNpc = m_pNpc->GetPosition();

	const float c_fMissleZ = 1.f;

	cMissle* pMissle = new cMissle;
	ptNpc.z = c_fMissleZ;
	pMissle->Create(GetGraphics(),ptNpc,"system\\spr\\missle.spr");
	pMissle->SetMainActor(false);
	pMissle->AddAnimation(ANI_WALK,"system\\spr\\missle.spr");
	pMissle->AddAnimation(ANI_WAIT,"system\\spr\\missle.spr");
	pMissle->SetLightRange(4.f);
	pMissle->Start();
	pMissle->Add();

	cPoint ptDes(point.x,point.y);
	ptDes.z = c_fMissleZ;
	GetCamera()->ScreenToGround(ptDes,pt);
	pMissle->WalkTo(pt);
	m_listMissle.push_back(pMissle);

	return TRUE;
}

BOOL cStatusTest::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pNpc == NULL)
		return FALSE;
	cPoint pt;
	GetCamera()->ScreenToGround(point,pt);
	if (IsCtrlDown())
		m_pNpc->JumpTo(pt);
	else
		m_pNpc->WalkTo(pt);

	for (int i=0; i<TEST_NUM; i++)
	{
		cPoint point2;
		point2.x += pt.x + rand() % 16 - 8;
		point2.y += pt.y + rand() % 16 - 8;
		if (m_aNpcTest[i])
		{
			m_aNpcTest[i]->WalkTo(point2);
		}
	}
	return TRUE;
}

void cStatusTest::OffsetObj(cActor* p, cPoint pt)
{
	p->Offset(pt);
}

BOOL cStatusTest::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);

	if (m_pNpc == NULL)
		return FALSE;

	float off = 0.1f;
	if (IsShiftDown())
	{
		off *= 0.1f;
	}
	else if (IsCtrlDown())
	{
		off *= 10.f;
	}
	switch (nChar)
	{
	case 'W':
		OffsetObj(m_pNpc,cPoint(0,-off));
		break;
	case 'X':
	case 'S':
		OffsetObj(m_pNpc,cPoint(0,off));
		break;
	case 'A':
		OffsetObj(m_pNpc,cPoint(-off,0));
		break;
	case 'D':
		OffsetObj(m_pNpc,cPoint(off,0));
		break;
	case 'Q':
		OffsetObj(m_pNpc,cPoint(-off,-off));
		break;
	case 'E':
		OffsetObj(m_pNpc,cPoint(off,-off));
		break;
	case 'Z':
		OffsetObj(m_pNpc,cPoint(-off,off));
		break;
	case 'C':
		OffsetObj(m_pNpc,cPoint(off,off));
		break;
	case 'M':
	case 'R':
		{
			iCamera::stParam param;
			param.mask = 0;
			GetCamera()->GetParam(param);
			if (nChar == 'M')
				m_eRun = RUN_TESTMAP;
			else
				m_eRun = RUN_RANDOM;
			for (int i=0; i<TEST_NUM; i++)
			{
				if (m_aNpcTest[i] == NULL)
				{
					m_aNpcTest[i] = new cActor;
					m_aNpcTest[i]->Create(GetGraphics(),param.ptTo,"system\\spr\\npc.spr");
					m_aNpcTest[i]->AddAnimation(ANI_WALK,"system\\spr\\npc.spr");
					m_aNpcTest[i]->AddAnimation(ANI_WAIT,"system\\spr\\npc.spr");
					m_aNpcTest[i]->Start();
					m_aNpcTest[i]->Add();
				}
			}
			break;
		}
	case 'I':
		{
			iCamera::stParam param;
			param.mask = 0;
			GetCamera()->GetParam(param);
			m_eRun = RUN_TESTMAP;
			CFileFind f;
			CString sFind = "system\\spr\\npc\\npc_*.spr";
			AppGetFullPath(sFind);
			BOOL b = f.FindFile(sFind);
			if (b)
			{
				int nAdd = 10;
				int add = 0;
				bool bAdd = FALSE;
				static CString m_strLast;
				if (!m_strLast.IsEmpty())
				{
					while(b)
					{
						b = f.FindNextFile();
						CString s = f.GetFilePath();
						AppGetShortPath(s);
						if (s == m_strLast)
							break;
					}
				}

				for (int i=0; i<TEST_NUM; i++)
				{
					if (m_aNpcTest[i] == NULL)
					{
						b = f.FindNextFile();
						CString s = f.GetFilePath();
						AppGetShortPath(s);
						m_aNpcTest[i] = new cActor;
						m_aNpcTest[i]->Create(GetGraphics(),param.ptTo,s);
						m_aNpcTest[i]->AddAnimation(ANI_WALK,s);
						m_aNpcTest[i]->AddAnimation(ANI_WAIT,s);
						m_aNpcTest[i]->Start();
						m_aNpcTest[i]->Add();
						add++;
						if (add > nAdd)
						{
							m_strLast = s;
							break;
						}
						if (!b)
						{
							break;
						}
					}
				}
			}
			break;
		}
	case 'U':
		m_eRun = RUN_USER;
		break;
	}
	return FALSE;
}


BOOL cStatusTest::OnNewDocument(LPCTSTR lpszPathName)
{
	BOOL b = cStatus::OnNewDocument(lpszPathName);
	Create();
	return b;
}

BOOL cStatusTest::OnOpenDocument(LPCTSTR lpszPathName)
{
	BOOL b = cStatus::OnOpenDocument(lpszPathName);
	Create();
	return b;
}

BOOL cStatusTest::OnSaveDocument(LPCTSTR lpszPathName)
{
	BOOL b = cStatus::OnSaveDocument(lpszPathName);
	Create();
	return b;
}

