#pragma once

#include "iGraphics.h"

#define EPSILON 0.0001   // Define your own tolerance
#define FLOAT_EQ(x,v) (((v - EPSILON) < x) && (x <( v + EPSILON)))

enum ePoint2Point{PP_On,PP_Up,PP_Down};
enum ePoint2Line{PL_On,PL_Up,PL_Down};
enum ePoint2Seam{PS_On,PS_Up,PS_Down,PS_OnSeamLine};
enum eSeam2Line{SL_On,SL_Up,SL_Down,SL_Cross};
enum eSeam2Seam{SS_On,SS_Up,SS_Down,SS_Cross,SS_CrossSeamLine};

//位置关系
ePoint2Point	Point2Point(cPoint pt, cPoint p);
ePoint2Line		Point2Line(cPoint pt, cPoint p1,cPoint p2);
eSeam2Line		Seam2Line(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2);
eSeam2Seam		Seam2Seam(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2);

//线段的交点
cPoint SeamCrossByLineAt(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2);
cPoint LineCrossLineAt(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2);
CPoint LineCrossLineAt(CPoint pt1, CPoint pt2, CPoint p1,CPoint p2);
bool LineParallelLine(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2);

//两点的距离
float DistanceSquare(cPoint p1,cPoint p2);
float Distance(cPoint p1,cPoint p2);
float DistanceAdd(cPoint p1,cPoint p2);
float DistanceMax(cPoint p1,cPoint p2);
//点到直线的距离
float PointToLineSquare(cPoint p,cPoint p1,cPoint p2);
float PointToLine(cPoint p,cPoint p1,cPoint p2);
float PointToLineSign(cPoint p,cPoint p1,cPoint p2);

//点到线段的距离
float PointToSeam(cPoint p,cPoint p1,cPoint p2);
float PointToSeamSquare(cPoint p,cPoint p1,cPoint p2);

//点在险段经过的格子里
bool PointInSeamTile(cPoint p,cPoint p1,cPoint p2);

//点在多边形内
bool IsPointInQuadrangle(cPoint pt, cPoint point[4]);
//注意，四边形四个点的顺序是交叉的
bool IsPointInPolygon(cPoint pt, cPoint p[], int nPoint);

inline bool IsPointInRect(const cPoint& pt, const cRect& rc)
{
	return !(pt.x < rc.l || pt.x > rc.r || pt.y < rc.t || pt.y > rc.b);
}
inline bool IsPointInRect(const POINT& pt, const cRect& rc)
{
	return !(pt.x < rc.l || pt.x > rc.r || pt.y < rc.t || pt.y > rc.b);
}
inline bool IsVertexInRect(const stVertexGround& v, const RECT& rc)
{
	return !(v.sx < rc.left || v.sx > rc.right || v.sy < rc.top || v.sy > rc.bottom);
}
inline bool IsVertexInRectDisplay(const stVertexGround& v, const cRect& rc)
{
	float sy = v.sy - v.sz * c_fSqrt3;
	return !(v.sx < rc.l || v.sx > rc.r || sy < rc.t || sy > rc.b);
}

//简单的函数
inline void AddPoint(stVertexGround& v, cPoint pt)
{
	v.sx += pt.x;
	v.sy += pt.y;
	v.sz += pt.z;
}
inline void AddPoint(stVertexScreen& v, CPoint pt)
{
	v.sx += pt.x;
	v.sy += pt.y;
}

inline cPoint GetPoint(const stVertexGround& v)
{
	return cPoint(v.sx,v.sy,v.sz);
}

inline void SetPoint(stVertexGround& v,cPoint pt)
{
	v.sx = pt.x;v.sy = pt.y; v.sz = pt.z;
}

inline cPoint GetPoint(const stVertexScreen& v)
{
	return cPoint(v.sx,v.sy,v.sz);
}

inline void SetPoint(stVertexScreen& v,cPoint pt)
{
	v.sx = pt.x;v.sy = pt.y; v.sz = pt.z;
}

inline void FillRect(stVertexGround* av, const cRect& r)
{
	av[0].sx = r.l;
	av[0].sy = r.t;
	av[1].sx = r.r;
	av[1].sy = r.t;
	av[2].sx = r.l;
	av[2].sy = r.b;
	av[3].sx = r.r;
	av[3].sy = r.b;
	for (int i=0; i<4; i++)
		av[i].sz = 0;
}

inline void FillRectUV(stVertexGround* av, const cRect& r)
{
	av[0].tu = r.l;
	av[0].tv = r.t;
	av[1].tu = r.r;
	av[1].tv = r.t;
	av[2].tu = r.l;
	av[2].tv = r.b;
	av[3].tu = r.r;
	av[3].tv = r.b;
}

inline void FillRect(stVertexScreen* av, const cRect& r)
{
	av[0].sx = r.l;
	av[0].sy = r.t;
	av[1].sx = r.r;
	av[1].sy = r.t;
	av[2].sx = r.l;
	av[2].sy = r.b;
	av[3].sx = r.r;
	av[3].sy = r.b;
	for (int i=0; i<4; i++)
	{
		av[i].sz = 0;
		av[i].rhw = 1.f;
	}
}

inline void FillRectUV(stVertexScreen* av, const cRect& r)
{
	av[0].tu = r.l;
	av[0].tv = r.t;
	av[1].tu = r.r;
	av[1].tv = r.t;
	av[2].tu = r.l;
	av[2].tv = r.b;
	av[3].tu = r.r;
	av[3].tv = r.b;
}

//这条线段经过那些的tile,
void GetSeamCrossTile(cPoint p1,cPoint p2,std::list<cPoint> & ptlist);
