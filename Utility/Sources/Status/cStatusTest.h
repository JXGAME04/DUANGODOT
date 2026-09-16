// cStatusTest.h: interface for the cStatusTest class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSTEST_H__215C729A_1AD8_42C1_B181_2BDD23CA5A66__INCLUDED_)
#define AFX_CSTATUSTEST_H__215C729A_1AD8_42C1_B181_2BDD23CA5A66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class cActor;
class cMissle;

typedef std::list<cMissle*> MissleList;
typedef MissleList::iterator itMissle;

class cStatusTest : public cStatus  
{
public:
	cStatusTest();
	virtual ~cStatusTest();
	void Clear();

	BOOL Create();
	cActor* m_pNpc;
	MissleList m_listMissle;
	
	iObj* m_pMenu;

	enum {TEST_NUM = 200,};
	cActor* m_aNpcTest[TEST_NUM];

	BOOL OnTimer(UINT nIDEvent);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
	
	void OffsetObj(cActor* p, cPoint pt);
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	BOOL OnNewDocument(LPCTSTR lpszPathName);
	BOOL OnOpenDocument(LPCTSTR lpszPathName);
	BOOL OnSaveDocument(LPCTSTR lpszPathName);
	void RandomWalk(cActor* pObj);

	enum RunEnum{RUN_USER,RUN_TESTMAP,RUN_RANDOM};
	RunEnum m_eRun;
};

#endif // !defined(AFX_CSTATUSTEST_H__215C729A_1AD8_42C1_B181_2BDD23CA5A66__INCLUDED_)
