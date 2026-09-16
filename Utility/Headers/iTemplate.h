#pragma once

#include "iCommon.h"
#include "iPack1d.h"

///////////////////////////////////////////////////////////
//plane的模板,
///////////////////////////////////////////////////////////
struct stTemplateObstacle
{
	char eObstacleType;
	char eObstacleNum;
	char eObstacleKind;
	cPoint aPoint[MAX_POINT];
};

struct stTemplateOne
{
	float fRadiusLighted;
//	float fRadiusObstacle;	//the same to fRaduisLighted
	float fAngleXY;
//	float fAngleZ;			//always 90!
	float fHeight;			//it is useful

	char ePlaneType;
	char eLighting;
	char eLighted;
	char eSort;
	char eVersort;
	char eObstacleType;
	char eDynamicType;
	char eKind;
	unsigned short nAnispeed;			//ms per frame!
	char bAniLoop;

//	float fSpeedPerLoop;
//	float fSpeedPerSecond;

	short nCenterX;
	short nCenterY;

	short nSprWidth;
	short nSprHeight;
	short nSprCenterX;
	short nSprCenterY;

	unsigned char nObstacleNum;	
};

///////////////////////////////////////////////////////////
//animation的模板,用于所有的动画
///////////////////////////////////////////////////////////
struct stAnimationEvent
{
	int iCallbackFrame;		//动画到该帧of each direction回调，
	long lCallbackID;		//if != 0 call back it 
};

struct stAnimation
{
	int nFrames;
	int nDirections;
	int nAniSpeed;			//ms per frame!
	BOOL bAniLoop;			//loop or not
	int nAnimationEvent;	//多少个event
};

///////////////////////////////////////////////////////////
//obj的模板,用于2进制
///////////////////////////////////////////////////////////
struct stObjTemplate
{

};

///////////////////////////////////////////////////////////
//npc的模板,用于2进制
///////////////////////////////////////////////////////////
struct stNpcTemplate
{

};

///////////////////////////////////////////////////////////
//地图编辑器存储的obj的信息,pack以后的
///////////////////////////////////////////////////////////
struct stObjPack
{
	char szKind[MAX_NAME];	//一级模板的索引
	char szEvent[MAX_NAME]; //event + id 二级模板
	long lID;
	cPoint ptOffset;		//offset of the region!
};

_declspec(dllexport) int Template_GetTerrainIndex(LPCSTR szTxt);
