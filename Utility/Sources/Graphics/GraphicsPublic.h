#pragma once

#define RELEASE(a) if(a){a->Release();a=NULL;}
#define ASSERTHR(hr) ASSERT(!FAILED(hr));
#define ASSERT_HR(hr) ASSERT(!FAILED(hr));
#define assert_hr(hr,str) ASSERT(!FAILED(hr));

/////////////////////////////////////////////////////////////////////////////
// CGraphicsApp
// See Graphics.cpp for the implementation of this class
//
#include "iColor.h"

class cGround;
class cScreen;
class cTile;

//void Graphics_SetGraphics(iGraphics*);

//iTexture* Graphics_GetDefaultGround();
//iTexture* Graphics_GetDefaultWall();
extern char g_strError[] ;
#define DX_ERROR(hr) if(FAILED(hr))DXTRACE_ERR(g_strError,hr)
#define TrimAB(t,a,b) if((t)<a)t=a;else if((t) >= b)t=b-1;
float FloatToInt(float a);

enum eRegionDirection
{
	D_0,D_1,D_3,D_5,D_6,D_7,D_9,D_11,D_12
};

enum eNotifyType
{
	NOTIFY_NULL, 
	NOTIFY_POSITION_CHANGING,	//位置将要改变了
	NOTIFY_POSITION_CHANGED,	//位置改变了
	NOTIFY_MOVING_CHANGING,		//物件的活动状态将要改变
	NOTIFY_MOVING_CHANGED,		//物件的活动状态改变了

//	NOTIFY_LAYER,				//物件的排序改变
};

const int c_nMaxPlaneHeight = 6;

typedef std::list<cPoint> cPointList;
typedef cPointList::iterator itcPoint;
