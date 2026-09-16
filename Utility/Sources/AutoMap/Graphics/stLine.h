
#pragma once

enum LineEnum{LINE_POINT,LINE_SEAM,};
struct stLine
{	
	cPlaneVerList listVer;	//在一条直线上的plane
	float k,b;				//直线方程,没有x=c的面,所以总是有效
	cPoint ptStart,ptEnd;	//线段的长度
	float GetLengthSquare(){return (ptStart.x-ptEnd.x)*(ptStart.x-ptEnd.x)+(ptStart.y-ptEnd.y)*(ptStart.y-ptEnd.y);}
	bool operator < (stLine& line)
	{
		return GetLengthSquare() < line.GetLengthSquare();
	}
};
