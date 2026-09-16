
#pragma once

#include "iGraphics.h"

//游戏世界专用，将交付的ptGround除以32,将得到的ptGround乘以32
_declspec(dllexport) HRESULT GU_CameraScreenToGroundScale(iGraphics* pGraphics, cPoint ptScreen, cPoint& ptGround);
_declspec(dllexport) HRESULT GU_OffsetCameraToScale(iGraphics* pGraphics, cPoint ptTo);
_declspec(dllexport) HRESULT GU_GetCameraOffsetScale(iGraphics* pGraphics, cPoint& ptOffset);
_declspec(dllexport) HRESULT GU_OffsetLightToScale(iObj* pObj, cPoint ptOffset);
_declspec(dllexport) HRESULT GU_OffsetObjToScale(iObj* pObj, cPoint ptOffset);
_declspec(dllexport) HRESULT GU_OffsetMissleToScale(iObj* pObj, cPoint ptOffset);

//[ground]
_declspec(dllexport) HRESULT GU_LoadGround(iGraphics* pGraphics,LPCSTR szFile);

//[camera]
_declspec(dllexport) HRESULT GU_GetCameraOffset(iGraphics* pGraphics, cPoint& ptOffset);
_declspec(dllexport) HRESULT GU_OffsetCameraTo(iGraphics* pGraphics, cPoint ptTo);
_declspec(dllexport) HRESULT GU_CameraScreenToGround(iGraphics* pGraphics, cPoint ptScreen, cPoint& ptGround);
//[map]
_declspec(dllexport) HRESULT GU_GetMapClickPos(iObj* pObj, CPoint ptScreen, cPoint& ptGround);
_declspec(dllexport) HRESULT GU_GetMapSmallClickPos(iObj* pObj, CPoint ptScreen, cPoint& ptGround);
//[select]
_declspec(dllexport) HRESULT GU_SetSelectType(iObj* pObj, SelectEnum e);
_declspec(dllexport) HRESULT GU_OffsetSelectTo(iObj* pObj, cPoint ptTo);
_declspec(dllexport) HRESULT GU_SetSelectSize(iObj* pObj, float w, float h);
_declspec(dllexport) HRESULT GU_SetSelectOffsetEnd(iObj* pObj, cPoint ptEnd);
_declspec(dllexport) HRESULT GU_ChangeSelectColor(iObj* pObj, cColor crColor);
//[obstacle]
_declspec(dllexport) HRESULT GU_SetObstaclePointinfo(iObj* pObj, stObstacleParam param);
_declspec(dllexport) HRESULT GU_SetObstacleType(iObj* pObj, ObstacleEnum e);
_declspec(dllexport) HRESULT GU_SetObstacleRadius(iObj* pObj, float r);
_declspec(dllexport) HRESULT GU_SetObstacleSelectPoint(iObj* pObj, int i);
//[light]
_declspec(dllexport) HRESULT GU_CreateLight(iGraphics* pGraphcis,iObj*& pObj, float fRange = 8.f, cColor cr = cColor(255,255,255,255));
_declspec(dllexport) HRESULT GU_AddLight(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_RemoveLight(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_OffsetLightTo(iObj* pObj, cPoint ptOffset);
_declspec(dllexport) HRESULT GU_SetLightRange(iObj* pObj, float fRange);
_declspec(dllexport) HRESULT GU_SetEnvironmentBrightness(iGraphics* pGraphics, int nBrightness);
//[obj]
_declspec(dllexport) HRESULT GU_CreateObj(iGraphics* pGraphcis,iObj*& pObj);
_declspec(dllexport) HRESULT GU_AddObj(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_RemoveObj(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_GetObjOffset(iObj* pObj, cPoint& ptOffset);
_declspec(dllexport) HRESULT GU_GetObjHeight(iObj* pObj, float& fHeight);
_declspec(dllexport) HRESULT GU_OffsetObjTo(iObj* pObj, cPoint ptOffset);
_declspec(dllexport) HRESULT GU_OffsetObjTo(iObj* pObj,CPoint ptScreen);
_declspec(dllexport) HRESULT GU_IncreateObjFrame(iObj* pObj,int nFrame);
_declspec(dllexport) HRESULT GU_ChangeObjFrame(iObj* pObj,int nFrame);
_declspec(dllexport) HRESULT GU_ChangeObjAnimation(iObj* pObj,LPCSTR szSpr);
_declspec(dllexport) HRESULT GU_ChangeObjAnimationArray(iObj* pObj,stResourceArray& stResArray);
_declspec(dllexport) HRESULT GU_ChangeObjColor(iObj* pObj,cColor cr);
_declspec(dllexport) HRESULT GU_SetObjDynamic(iObj* pObj,DynamicEnum eSort);
_declspec(dllexport) HRESULT GU_SetObjNick(iObj* pObj,LPCSTR szNick);
_declspec(dllexport) HRESULT GU_SetObjEvent(iObj* pObj,LPCSTR szEvent);
_declspec(dllexport) HRESULT GU_SetObjKind(iObj* pObj,KindEnum eKind);
_declspec(dllexport) HRESULT GU_SetObjID(iObj* pObj,long lID);
_declspec(dllexport) HRESULT GU_GetObjID(iObj* pObj, long& ID);
_declspec(dllexport) HRESULT GU_SetPlaneLayer(iObj* pObj,int nLayer);
_declspec(dllexport) HRESULT GU_GetObjDynamic(iObj* pObj, DynamicEnum& lDynamic);

//[missle]
_declspec(dllexport) HRESULT GU_CreateMissle(iGraphics* pGraphcis,iObj*& pObj);
_declspec(dllexport) HRESULT GU_AddMissle(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_RemoveMissle(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_GetMissleOffset(iObj* pObj, cPoint& ptOffset);
_declspec(dllexport) HRESULT GU_OffsetMissleTo(iObj* pObj, cPoint ptOffset);
_declspec(dllexport) HRESULT GU_IncreateMissleFrame(iObj* pObj,int nFrame);
_declspec(dllexport) HRESULT GU_ChangeMissleDirection(iObj* pObj,int nDirection);
_declspec(dllexport) HRESULT GU_ChangeMissleFrame(iObj* pObj,int nFrame);
_declspec(dllexport) HRESULT GU_ChangeMissleAnimation(iObj* pObj,LPCSTR szSpr);
_declspec(dllexport) HRESULT GU_ChangeMissleAniSpeed(iObj* pObj,int nAniSpeed, BOOL bAniLoop);
/*
//[menu]
//the normal operator
_declspec(dllexport) HRESULT GU_DrawMenu(iGraphics* pGraphcis,iScreen::stDraw& st);
_declspec(dllexport) HRESULT GU_GetSprSize(iGraphics* pGraphcis, LPCSTR szSpr, long& width, long& height);
_declspec(dllexport) HRESULT GU_GetSprSize(iGraphics* pGraphcis, LPCSTR szSpr, int frame, long& width, long& height);
_declspec(dllexport) HRESULT GU_GetSprAlpha(iGraphics* pGraphcis, LPCSTR szSpr, int frame, POINT pt, BYTE& alpha);
_declspec(dllexport) HRESULT GU_GetSprInfo(iGraphics* pGraphcis, LPCSTR szSpr, int& frame, int& nInterval);
_declspec(dllexport) HRESULT GU_GetSprReferenceSpot(iGraphics* pGraphcis, LPCSTR szSpr, int& h, int& v);
*/
//the new operator
_declspec(dllexport) HRESULT GU_CreateMenu(iGraphics* pGraphcis,iObj*& pObj);
_declspec(dllexport) HRESULT GU_OffsetMenuTo(iObj* pObj, POINT ptOffset);
_declspec(dllexport) HRESULT GU_AddMenu(iGraphics* pGraphcis, iObj* pObj);
_declspec(dllexport) HRESULT GU_SetMenuSpr(iObj* pObj, LPCSTR szFile, int iFrame);
_declspec(dllexport) HRESULT GU_SetMenuColor(iObj* pObj, cColor cr);
_declspec(dllexport) HRESULT GU_RemoveMenu(iGraphics* pGraphcis, iObj* pObj);

