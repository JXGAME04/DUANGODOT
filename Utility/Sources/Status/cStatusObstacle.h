// cStatusObstacle.h: interface for the cStatusObstacle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSOBSTACLE_H__933538F2_AD00_420F_8216_AEDCC023B1B7__INCLUDED_)
#define AFX_CSTATUSOBSTACLE_H__933538F2_AD00_420F_8216_AEDCC023B1B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

enum ObstacleGeometryEnum
{
	OBSTACLE_GEOMETRY_NULL,				//无阻挡
	OBSTACLE_GEOMETRY_POINT,			//点
	OBSTACLE_GEOMETRY_SEAM,				//线段
	OBSTACLE_GEOMETRY_POLYGON,			//多边形
	OBSTACLE_GEOMETRY_USERDEFINE,		//自定义
};

class cStatusObstacle : public cStatus  
{
public:
	cStatusObstacle();
	virtual ~cStatusObstacle();
	void Clear();

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////
	BOOL m_bShowObstacle;
	BOOL m_bSeam;
	ObstacleEnum m_eType;
	float m_fHeight;

//	int m_nOPPlane;
	cPoint m_ptSelectOffset; //选择这个物件的时候存在一个offset

	ObstacleGeometryEnum m_eGeometry;
	int m_nPoint;
	int GetPointNum();
	int m_iPointModify; //修改当前的哪一个点
	CString m_strObstacleCapture;
	cPoint m_aPoint[MAX_POINT];

	BOOL Create();
	BOOL OnSetFocus();
	BOOL OnLostFocus();
	//first select a obstacle, select a plane, 
	void SetObstacleSeam(bool b);

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////
	void OnSelectedSetting(LPCTSTR szFile);
	BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
	BOOL MoveSelect(CPoint pt);
	BOOL OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnCameraMove(CPoint point);

/////////////////////////////////////////////////////////////////////
//obstacle
/////////////////////////////////////////////////////////////////////
	iObj* m_pObstacleLeft;
	iObj* m_pObstacleRight;

	void CreateObstacle(CPoint point,iObj*& pObstacle);
	void SelectObstacle(CPoint point,iObj*& pObstacle);
	void DeleteSelectPoint(iObj* p);
	void AddSelectPoint(iObj* p);

	void SetObstacleGeometry(iObj* pObstacle);
	void OffsetObstalcePoint(iObj* pObstacle,int iPoint,cPoint ptOffset);
	void CaptureObstacle(iObj* pObstacle);
/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
	bool m_bTrace;
	void ToggleObstacleTrace();

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);

	void OnObstacleProperty(iObj* p);

	BOOL Copy();
	void GetObstacleFileString(CString& sFile);
};

#endif // !defined(AFX_CSTATUSOBSTACLE_H__933538F2_AD00_420F_8216_AEDCC023B1B7__INCLUDED_)
