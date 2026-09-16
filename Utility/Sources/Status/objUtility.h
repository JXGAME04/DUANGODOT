
#pragma once

class iObj;

#include "iColor.h"

struct stLightParam;
struct stObstacleParam;
struct stObjParam;

void SetObjTrace(iObj* pObj, bool bTrace);
void SetObjMoving(iObj* pObj, bool bMoving);
void SetObjSelect(iObj* pObj, bool bSelect);

void RemoveAndReleaseObj(iObj*& pObj);
void AddObj(iObj* pObj);
void RemoveObj(iObj* pObj);
void AddObjHeight(iObj* pObj,float fHeight);
void SetObjHeight(iObj* pObj,float fHeight);

void SetObjID(iObj* pObj,int id,bool bRunnow = false);
void SetObjGroupFile(iObj* pObj,LPCSTR szFile,bool bRunnow = false);
void OffsetObj(iObj* pObj,cPoint ptOffset,bool bRunnow = false);
void OffsetObjTo(iObj* pObj,cPoint ptOffset,bool bRunnow = false);
void OffsetObjTo(iObj* pObj,CPoint ptOffset,bool bRunnow = false);
void SetObjParam(iObj* pObj,stObjParam& param,bool bRunnow = false);

void CreatePlane(iGraphics* pGraphics,iObj* & pPlane, LPCSTR szFile, int nFrame);
void CreateLight(iGraphics* pGraphics,iObj* & pPlane, stLightParam& param);
void CreateObstacle(iGraphics* pGraphics,iObj* & pPlane, stObstacleParam& param);

