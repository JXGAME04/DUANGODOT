#include "stdafx.h"
#include "position.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define FLOAT_TOOMUCHLARGE

const float c_fMin = 0.01f;
//////////////////////////////////////////////////////////////////////
//位置关系
ePoint2Point Point2Point(cPoint pt, cPoint p)
{
	ePoint2Point e;
	if (pt.y == p.y && pt.x == p.x)
		e = PP_On;
	else if (pt.y == p.y)
	{
		if (pt.x < p.x)
			e = PP_Up;
		else 
			e = PP_Down;
	}
	else 
	{
		if (pt.y < p.y)
			e = PP_Up;
		else 
			e = PP_Down;
	}
	return e;
}

ePoint2Line Point2Line(cPoint pt, cPoint p1,cPoint p2)
{
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif
	ASSERT(!(p1 == p2));
	ASSERT(p1.x <= p2.x);
	//为了浮点乘法的需要,将所有的电先做位移
	p1 -= pt;
	if (fabs(p1.x) < 0.1f && fabs(p1.y) < 0.1f)
		return PL_On;
	p2 -= pt;
	if (fabs(p2.x) < 0.1f && fabs(p2.y) < 0.1f)
		return PL_On;
	pt = cPoint(0,0);

	if (p1.x > p2.x)
	{
		cPoint p = p1;
		p1 = p2;
		p2 = p;
	}

	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;
//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
	float f = y0 * (x2 - x1) - x0 * (y2 - y1) + (x1*y2 - x2*y1);
	if (fabs(f) < c_fMin)
	{
		return PL_On;
	}
	else if (f < 0)
		return PL_Up;
	else
		return PL_Down;
}

eSeam2Line PL2SL(ePoint2Line e1)
{
	eSeam2Line e;
	switch (e1)
	{
	case PL_On:
		e = SL_On;
		break;
	case PL_Up:
		e = SL_Up;
		break;
	case PL_Down:
		e = SL_Down;
		break;
	}
	return e;
}

eSeam2Line	Seam2Line(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
//	ASSERT(!(pt1 == pt2) && !(p1 == p2));
	ePoint2Line e1 = Point2Line(pt1,p1,p2);
	ePoint2Line e2 = Point2Line(pt2,p1,p2);

	eSeam2Line e;
	if (e1 == e2)
	{
		e = PL2SL(e1);
	}
	else
	{
		if (e1 == PL_On)
			e = PL2SL(e2);
		else if (e2 == PL_On)
			e = PL2SL(e1);
		else
			e = SL_Cross;
	}	
	return e;
}

eSeam2Seam	Seam2Seam(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
//	ASSERT(!(pt1 == pt2) && !(p1 == p2));
	eSeam2Line eTemp = Seam2Line(pt1,pt2,p1,p2);
	eSeam2Seam e;
	switch(eTemp)
	{
	case SL_Down:
		e = SS_Down;
		break;
	case SL_Up:
		e = SS_Up;
		break;
	case SL_On:
		{
			float min1,max1;
			float min2,max2;
			if (p1.y == p2.y)
			{
				min1 = _cpp_min(pt1.x,pt2.x);
				min2 = _cpp_min(p1.x,p2.x);
				max1 = _cpp_max(pt1.x,pt2.x);
				max2 = _cpp_max(p1.x,p2.x);
			}
			else
			{
				min1 = _cpp_min(pt1.y,pt2.y);
				min2 = _cpp_min(p1.y,p2.y);
				max1 = _cpp_max(pt1.y,pt2.y);
				max2 = _cpp_max(p1.y,p2.y);
			}
			if (min1 >= max2)
				e = SS_Down;
			else if (max1 <= min2)
				e = SS_Up;
			else if (max1 >= max2 && min1 >= min2)
				e = SS_Down;
			else if (max1 <= max2 && min1 <= min2)
				e = SS_Up;
			else
				e = SS_Up;	//impossible
		}
		break;
	case SL_Cross:
		e = SS_Cross;
		break;
	}
	return e;
}

//////////////////////////////////////////////////////////////////////
cPoint SeamCrossSeamAt(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
	return LineCrossLineAt(pt1,pt2,p1,p2);
}

cPoint SeamCrossByLineAt(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
	return LineCrossLineAt(p1,p2,pt1,pt2);
}

bool LineParallelLine(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
	cPoint _p1 = pt1 - pt2;
	cPoint _p2 = p1 - p2;
	return fabs(_p1.x * _p2.y - _p1.y * _p2.x) < c_fMin;
}

//不平行的时候才能调用该函数
cPoint LineCrossLineAt(cPoint pt1, cPoint pt2, cPoint p1,cPoint p2)
{
	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	cPoint ptOffset = pt1;
	pt2 -= pt1;
	p1 -= pt1;
	p2 -= pt1;
	pt1 = cPoint(0,0);
#endif

	cPoint p;
	float x1,y1,x2,y2,x3,y3,x4,y4;
	x1 = pt1.x; x2 = pt2.x; x3 = p1.x; x4 = p2.x;
	y1 = pt1.y; y2 = pt2.y; y3 = p1.y; y4 = p2.y;
//  直线方程l1：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
//  直线方程l2：
//	y(x4-x3) - x(y4-y3) + (x3y4 - x4y3) = 0 
//	p.y = -( (x1*y2 - x2*y1)(y4 - y3) - (x3*y4 - x4*y3)(y2 - y1) ) /  ( (x2 - x1) * (y4 - y3) - (x4 - x3)(y2 - y1) );

//  it is wrong when function is 
//	y = c
	//ASSERT(x2 != x1 && x4 != x3);
	if (x2 == x1 && x4 == x3)
	{
		p.x = x1;
		p.y = pt2.y;
	}
	else if (x2 == x1)
	{
		p.x = x1;
		p.y = (x1 * (y4 - y3) + (y3*x4 - y4*x3)) / (x4 - x3);
	}
	else if (x4 == x3)
	{
		p.x = x3;
		p.y = (x3 * (y2 - y1) + (y1*x2 - y2*x1)) / (x2 - x1);
	}
	else if (y2 == y1 && y4 == y3)
	{
		p.y = y1;
		p.x = pt2.x;
	}
	else if (y2 == y1)
	{
		p.y = y1;
		p.x = (y1 * (x4 - x3) + (x3*y4 - x4*y3)) / (y4 - y3);
	}
	else if (y4 == y3)
	{
		p.y = y3;
		p.x = (y3 * (x2 - x1) + (x1*y2 - x2*y1)) / (y2 - y1);
	}
	else
	{
		p.y = ((x1*y2 - x2*y1) * (y4 - y3) - (x3*y4 - x4*y3) * (y2 - y1))  /  ( (y2 - y1) * (x4 - x3) - (y4 - y3) * (x2 - x1) );
		p.x = ((x1*y2 - x2*y1) * (x4 - x3) - (x3*y4 - x4*y3) * (x2 - x1))  /  ( (y2 - y1) * (x4 - x3) - (y4 - y3) * (x2 - x1) );
	}

#ifdef FLOAT_TOOMUCHLARGE
	p += ptOffset;
#endif
	return p;
}

CPoint LineCrossLineAt(CPoint pt1, CPoint pt2, CPoint p1,CPoint p2)
{
	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	CPoint ptOffset = pt1;
	pt2 -= pt1;
	p1 -= pt1;
	p2 -= pt1;
	pt1 = CPoint(0,0);
#endif

	CPoint p;
	float x1,y1,x2,y2,x3,y3,x4,y4;
	x1 = pt1.x; x2 = pt2.x; x3 = p1.x; x4 = p2.x;
	y1 = pt1.y; y2 = pt2.y; y3 = p1.y; y4 = p2.y;
//  直线方程l1：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
//  直线方程l2：
//	y(x4-x3) - x(y4-y3) + (x3y4 - x4y3) = 0 
//	p.y = -( (x1*y2 - x2*y1)(y4 - y3) - (x3*y4 - x4*y3)(y2 - y1) ) /  ( (x2 - x1) * (y4 - y3) - (x4 - x3)(y2 - y1) );

//  it is wrong when function is 
//	y = c
	//ASSERT(x2 != x1 && x4 != x3);
	if (x2 == x1 && x4 == x3)
	{
		p.x = x1;
		p.y = pt2.y;
	}
	else if (x2 == x1)
	{
		p.x = x1;
		p.y = (x1 * (y4 - y3) + (y3*x4 - y4*x3)) / (x4 - x3);
	}
	else if (x4 == x3)
	{
		p.x = x3;
		p.y = (x3 * (y2 - y1) + (y1*x2 - y2*x1)) / (x2 - x1);
	}
	else if (y2 == y1 && y4 == y3)
	{
		p.y = y1;
		p.x = pt2.x;
	}
	else if (y2 == y1)
	{
		p.y = y1;
		p.x = (y1 * (x4 - x3) + (x3*y4 - x4*y3)) / (y4 - y3);
	}
	else if (y4 == y3)
	{
		p.y = y3;
		p.x = (y3 * (x2 - x1) + (x1*y2 - x2*y1)) / (y2 - y1);
	}
	else
	{
		p.y = ((x1*y2 - x2*y1) * (y4 - y3) - (x3*y4 - x4*y3) * (y2 - y1))  /  ( (y2 - y1) * (x4 - x3) - (y4 - y3) * (x2 - x1) );
		p.x = ((x1*y2 - x2*y1) * (x4 - x3) - (x3*y4 - x4*y3) * (x2 - x1))  /  ( (y2 - y1) * (x4 - x3) - (y4 - y3) * (x2 - x1) );
	}

#ifdef FLOAT_TOOMUCHLARGE
	p += ptOffset;
#endif
	return p;
}

/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//距离

//两点的距离
float DistanceSquare(cPoint p1,cPoint p2)
{
	return (p1.x-p2.x)*(p1.x-p2.x)+ (p1.y-p2.y)*(p1.y-p2.y) + (p1.z-p2.z)*(p1.z-p2.z);
}

float Distance(cPoint p1,cPoint p2)
{
	return sqrtf(DistanceSquare(p1,p2));
}

float DistanceAdd(cPoint p1,cPoint p2)
{
	cPoint p = p1 - p2;
	return fabs(p.x)+fabs(p.y)+fabs(p.z);
}

float DistanceMax(cPoint p1,cPoint p2)
{
	cPoint p = p1 - p2;
	return _cpp_max(fabs(p.x),_cpp_max(fabs(p.y),fabs(p.z)));
}

//点到直线的距离平方
float PointToLineSquare(cPoint pt,cPoint p1,cPoint p2)
{
	ASSERT(p1.x <= p2.x);

	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif

	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;

//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 

	float ftemp = pt.y*(x2-x1) - pt.x*(y2-y1) + (x1*y2 - x2*y1);
	float f = ftemp * ftemp / ((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
	return f;
}

//带符号的
float PointToLineSign(cPoint pt, cPoint p1,cPoint p2)
{
	ASSERT(p1.x <= p2.x);
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif

	const float minf = 0.1f;
	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;
//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
	float f = y0 * (x2 - x1) - x0 * (y2 - y1) + (x1*y2 - x2*y1);
	return f;
}

//点到线段的距离平方
float PointToSeamSquare(cPoint pt,cPoint p1,cPoint p2)
{
	if (p1.x > p2.x)
	{
		cPoint ptTemp = p1;
		p1 = p2;
		p2 = ptTemp;
	}
	ASSERT(p1.x <= p2.x);

	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif

	if (pt.x >= _cpp_min(p1.x,p2.x) && pt.x <= _cpp_max(p1.x,p2.x) 
	&& pt.y >= _cpp_min(p1.y,p2.y) && pt.y <= _cpp_max(p1.y,p2.y))
		return PointToLineSquare(pt,p1,p2);

	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;

//  直线方程l1：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
//  直线方程l2：
//	y(y2-y1) + x(x2-x1) = y0(y2-y1) + x0(x2-x1); 

//	y((x2-x1)*(x2-x1) + (y2-y1)(y2-y1)) = (y0(y2-y1) + x0(x2-x1))(y2-y1) - (x1y2 - x2y1)(x2-x1);
//	x((x2-x1)*(x2-x1) + (y2-y1)(y2-y1)) = (y0(y2-y1) + x0(x2-x1))(x2-x1) + (x1y2 - x2y1)(y2-y1);
	float x,y;
	y = ( (y0*(y2-y1) + x0*(x2-x1))*(y2-y1) - (x1*y2 - x2*y1)*(x2-x1) ) / ( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	x = ( (y0*(y2-y1) + x0*(x2-x1))*(x2-x1) + (x1*y2 - x2*y1)*(y2-y1) ) / ( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

//	如果垂点在线段上
	if (x >= _cpp_min(p1.x,p2.x) && x <= _cpp_max(p1.x,p2.x) && y >= _cpp_min(p1.y,p2.y) && y <= _cpp_max(p1.y,p2.y))
		return DistanceSquare(pt,cPoint(x,y));
	return _cpp_min(DistanceSquare(pt,p1),DistanceSquare(pt,p2));
}

//点到直线的距离
float PointToLine(cPoint pt,cPoint p1,cPoint p2)
{
	ASSERT(p1.x <= p2.x);

	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif

	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;

//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 

	float f = fabs(pt.y*(x2-x1) - pt.x*(y2-y1) + (x1*y2 - x2*y1) ) / sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
	return f;
}

float PointToSeam(cPoint pt,cPoint p1,cPoint p2)
{
	if (p1.x > p2.x)
	{
		cPoint ptTemp = p1;
		p1 = p2;
		p2 = ptTemp;
	}
	ASSERT(p1.x <= p2.x);

	//为了浮点乘法的需要,将所有的电先做位移
#ifdef FLOAT_TOOMUCHLARGE
	p1 -= pt;
	p2 -= pt;
	pt = cPoint(0,0);
#endif

	if (pt.x >= _cpp_min(p1.x,p2.x) && pt.x <= _cpp_max(p1.x,p2.x) 
	&& pt.y >= _cpp_min(p1.y,p2.y) && pt.y <= _cpp_max(p1.y,p2.y))
		return PointToLine(pt,p1,p2);

	float x0,y0,x1,y1,x2,y2;
	x0=pt.x;y0=pt.y;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;

//  直线方程l1：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
//  直线方程l2：
//	y(y2-y1) + x(x2-x1) = y0(y2-y1) + x0(x2-x1); 

//	y((x2-x1)*(x2-x1) + (y2-y1)(y2-y1)) = (y0(y2-y1) + x0(x2-x1))(y2-y1) - (x1y2 - x2y1)(x2-x1);
//	x((x2-x1)*(x2-x1) + (y2-y1)(y2-y1)) = (y0(y2-y1) + x0(x2-x1))(x2-x1) + (x1y2 - x2y1)(y2-y1);
	float x,y;
	y = ( (y0*(y2-y1) + x0*(x2-x1))*(y2-y1) - (x1*y2 - x2*y1)*(x2-x1) ) / ( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	x = ( (y0*(y2-y1) + x0*(x2-x1))*(x2-x1) + (x1*y2 - x2*y1)*(y2-y1) ) / ( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

//	如果垂点在线段上
	if (x >= _cpp_min(p1.x,p2.x) && x <= _cpp_max(p1.x,p2.x) && y >= _cpp_min(p1.y,p2.y) && y <= _cpp_max(p1.y,p2.y))
		return Distance(pt,cPoint(x,y));
	return _cpp_min(Distance(pt,p1),Distance(pt,p2));
}


/////////////////////////////////////////////////////////////////////////
//点在多边形内
bool IsPointInPolygon(cPoint pt, cPoint p[], int nPoint)
{
	if (nPoint >= MAX_POINT)
		return false;

	bool b0;
	cPoint point[MAX_POINT+1];
	for (int i=0; i<nPoint; i++)
		point[i] = p[i];
	point[nPoint] = p[0];

#ifdef FLOAT_TOOMUCHLARGE
	for (i=0; i<nPoint+1; i++)
		point[i] -= pt;
	pt = cPoint(0,0);	
#endif

	for (i=0; i<nPoint; i++)
	{
		float f = (pt.y - point[i].y) * (point[i+1].x - point[i].x) - (pt.x - point[i].x) * (point[i+1].y - point[i].y)  ;
		if (f == 0)
			return false;
		if (i == 0)
		{
			if (f > 0)
				b0 = true;
			else
				b0 = false;
		}
		else
		{
			if (f > 0 && b0 == false)
				return false;
			if (f < 0 && b0 == true)
				return false;
		}
	}
	return true;
}

bool IsPointInQuadrangle(cPoint pt, cPoint p[4])
{
	cPoint point[5];
	point[0] = p[0];
	point[1] = p[1];
	point[2] = p[3];
	point[3] = p[2];
	point[4] = p[0];
	return IsPointInPolygon(pt,point,4);
}

//这条线段经过那些的tile,
/*
void GetSeamCrossTile(cPoint p1,cPoint p2,list<cPoint> & ptlist)
{
	float x1,y1,x2,y2;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;
	if (p1.x == p2.x)
	{
		int x = p1.x;
		for (int y = ceil(_cpp_min(y2,y1)); y <= floor(_cpp_max(y2,y1)); y++)
			ptlist.push_back(cPoint(x,y));
		return;
	}
	if (p1.y == p2.y)
	{
		int y = p1.y;
		for (int x = ceil(x1); x<=floor(x2); x++)
			ptlist.push_back(cPoint(x,y));
		return;
	}

	list<cPoint> list1;
	ASSERT(p1.x < p2.x);
//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
	float k = (y2-y1)/(x2-x1);
	float b = (x2*y1-x1*y2)/(x2-x1);
	//只有一端进入的不考虑
	for (int x = ceil(x1); x<=floor(x2); x++)
	{
		float fy = k * x + b;
		float y = fy;
		if ( ceil(fy) - fy < fMin)
			y = ceil(fy);
		else if (fy - floor(fy) < fMin)
			y = floor(fy);
		list1.push_back(cPoint(x,y));
	}

	float k1 = (x2-x1)/(y2-y1);
	float b1 = (x1*y2-x2*y1)/(y2-y1);
	for (int y = ceil(_cpp_min(y2,y1)); y <= floor(_cpp_max(y2,y1)); y++)
	{
		float fx = k1 * y + b1;
		float x = fx;
		if ( ceil(fx) - fx < fMin)
			x = ceil(fx);
		else if (fx - floor(fx) < fMin)
			x = floor(fx);
		bool binsert = false;
		for (list<cPoint>::iterator it = list1.begin(); it != list1.end(); ++it)
		{
			if ((*it).x > x)
			{
				binsert = true;
				list1.insert(it,cPoint(x,y));
				break;
			}
		}
		if(!binsert)
			list1.push_back(cPoint(x,y));
	}


	if (list1.empty())
		return;
	if (list1.size() == 1)
	{
		cPoint p0 = list1.front();
		ptlist.push_back(cPoint((int)p0.x,(int)p0.y));
		return;
	}

	cPoint p0 = list1.front();
	cPoint p9 ;
	list<cPoint>::iterator it = list1.begin();
	for (++it; it != list1.end(); ++it)
	{
		p9 = (*it);
		//
		//p0,p1 决定经过哪一个格子
		//
		cPoint pt = cPoint((int)(_cpp_min(p0.x,p9.x)),(int)(_cpp_min(p0.y,p9.y)));
		if ( find(ptlist.begin(),ptlist.end(),pt) == ptlist.end() )
			ptlist.push_back(pt);
		p0 = p9;
	}
}
*/
//这个版本端点也是有效的
_declspec(dllexport) void GetSeamCrossTile(cPoint p1,cPoint p2,std::list<cPoint> & ptlist)
{
#ifdef FLOAT_TOOMUCHLARGE
	cPoint ptOffset = cPoint((int)p1.x,(int)p1.y);
	p2 -= ptOffset;
	p1 -= ptOffset;
#endif

	float x1,y1,x2,y2;
	x1=p1.x;y1=p1.y;x2=p2.x;y2=p2.y;
/*
	if (p1.x == p2.x)
	{
		int x = p1.x;
		for (int y = floor(_cpp_min(y2,y1)); y < ceil(_cpp_max(y2,y1)); y++)
			ptlist.push_back(cPoint(x,y));
		return;
	}

	ASSERT(p1.x < p2.x);
	if (p1.y == p2.y)
	{
		int y = p1.y;
		for (int x = floor(x1); x<ceil(x2); x++)
			ptlist.push_back(cPoint(x,y));
		return;
	}
*/
	std::list<cPoint> list1;
	list1.push_back(p1);
//  直线方程：
//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
	float k = (y2-y1)/(x2-x1);
	float b = (x2*y1-x1*y2)/(x2-x1);
	//只有一端进入的不考虑
	float fMin = 0.00001f;
	for (int x = ceil(x1); x<=floor(x2); x++)
	{
		float fy = k * x + b;
		float y = fy;
		if ( ceil(fy) - fy < fMin)
			y = ceil(fy);
		else if (fy - floor(fy) < fMin)
			y = floor(fy);
		list1.push_back(cPoint(x,y));
	}

	float k1 = (x2-x1)/(y2-y1);
	float b1 = (x1*y2-x2*y1)/(y2-y1);
	for (int y = ceil(_cpp_min(y2,y1)); y <= floor(_cpp_max(y2,y1)); y++)
	{
		float fx = k1 * y + b1;
		float x = fx;
		if ( ceil(fx) - fx < fMin)
			x = ceil(fx);
		else if (fx - floor(fx) < fMin)
			x = floor(fx);
		bool binsert = false;
		for (std::list<cPoint>::iterator it = list1.begin(); it != list1.end(); ++it)
		{
			if ((*it).x > x)
			{
				binsert = true;
				list1.insert(it,cPoint(x,y));
				break;
			}
		}
		if(!binsert)
			list1.push_back(cPoint(x,y));
	}
	list1.push_back(p2);
/*
	if (list1.empty())
		return;
	if (list1.size() == 1)
	{
		cPoint p0 = list1.front();
		ptlist.push_back(cPoint((int)p0.x,(int)p0.y));
		return;
	}
*/
	cPoint p0 = list1.front();
	cPoint p9 ;
	std::list<cPoint>::iterator it = list1.begin();
	for (++it; it != list1.end(); ++it)
	{
		p9 = (*it);
		//
		//p0,p1 决定经过哪一个格子
		//
		cPoint pt = cPoint(floor(_cpp_min(p0.x,p9.x)),floor(_cpp_min(p0.y,p9.y)));
#ifdef FLOAT_TOOMUCHLARGE
		pt += ptOffset;
#endif

		if ( std::find(ptlist.begin(),ptlist.end(),pt) == ptlist.end() )
			ptlist.push_back(pt);
		p0 = p9;
	}
}


bool PointInSeamTile(cPoint p,cPoint p1,cPoint p2)
{
	if (p.x < p1.x-1 && p.x < p2.x-1)
		return false;
	if (p.y < p1.y-1 && p.y < p2.y-1)
		return false;
	if (p.x > p1.x+1 && p.x > p2.x+1)
		return false;
	if (p.y > p1.y+1 && p.y > p2.y+1)
		return false;

	p.x = (int) p.x;
	p.y = (int) p.y;
	p.z = 0;
	std::list<cPoint> ptlist;
	if (p1.x > p2.x)
	{
		cPoint pp;
		pp = p1;
		p1 = p2;
		p2 = pp;
	}
	GetSeamCrossTile(p1,p2,ptlist);
	for (std::list<cPoint>::iterator it = ptlist.begin(); it!= ptlist.end(); ++it)
	{
		if (p == (*it))
			return true;
	}
	return false;
}
