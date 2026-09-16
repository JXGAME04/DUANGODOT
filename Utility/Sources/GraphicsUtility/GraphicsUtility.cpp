// GraphicsUtility.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "GraphicsUtility.h"

#include "iGraphicsUtility.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CGraphicsUtilityApp

BEGIN_MESSAGE_MAP(CGraphicsUtilityApp, CWinApp)
	//{{AFX_MSG_MAP(CGraphicsUtilityApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphicsUtilityApp construction

CGraphicsUtilityApp::CGraphicsUtilityApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGraphicsUtilityApp object

CGraphicsUtilityApp theApp;

//////////////////////////////////////////////////////////////////////////////
//游戏世界专用，将交付的ptGround除以32,将得到的ptGround乘以32
//////////////////////////////////////////////////////////////////////////////
const float c_fScale = 32.f;
_declspec(dllexport) HRESULT GU_CameraScreenToGroundScale(iGraphics* pGraphics, cPoint ptScreen, cPoint& ptGround)
{
	pGraphics->GetiCamera()->ScreenToGround(ptScreen,ptGround);
	ptGround.x *= c_fScale;
	ptGround.y *= c_fScale;
	ptGround.z *= c_fScale;
	return 0;
}

_declspec(dllexport) HRESULT GU_OffsetCameraToScale(iGraphics* pGraphics, cPoint ptOffset)
{
	ptOffset.x /= c_fScale;
	ptOffset.y /= c_fScale;
	ptOffset.z /= c_fScale;
	return GU_OffsetCameraTo(pGraphics,ptOffset);
}

_declspec(dllexport) HRESULT GU_GetCameraOffsetScale(iGraphics* pGraphics, cPoint& ptOffset)
{
	iCamera::stParam param;
	param.mask = 0 ;
	pGraphics->GetiCamera()->GetParam(param);
	ptOffset = param.ptTo;
	ptOffset.x *= c_fScale;
	ptOffset.y *= c_fScale;
	ptOffset.z *= c_fScale;
	return 0;	
}

_declspec(dllexport) HRESULT GU_OffsetLightToScale(iObj* pObj, cPoint ptOffset)
{
	ptOffset.x /= c_fScale;
	ptOffset.y /= c_fScale;
	ptOffset.z /= c_fScale;
	return GU_OffsetLightTo(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_OffsetObjToScale(iObj* pObj, cPoint ptOffset)
{
	ptOffset.x /= c_fScale;
	ptOffset.y /= c_fScale;
	ptOffset.z /= c_fScale;
	return GU_OffsetObjTo(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_OffsetMissleToScale(iObj* pObj, cPoint ptOffset)
{
	ptOffset.x /= c_fScale;
	ptOffset.y /= c_fScale;
	ptOffset.z /= c_fScale;
	return GU_OffsetMissleTo(pObj,ptOffset);
}

//////////////////////////////////////////////////////////////////////////////
//[ground]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_LoadGround(iGraphics* pGraphics,LPCSTR szFile)
{
	HRESULT hr = 0;
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_LOAD;
	strcpy(param.szFile,szFile);
	pGraphics->GetiGround()->SetParam(param,true);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//[camera]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_GetCameraOffset(iGraphics* pGraphics, cPoint& ptTo)
{
	iCamera::stParam param;
	param.mask = 0 ;
	pGraphics->GetiCamera()->GetParam(param);
	ptTo = param.ptTo;
	return 0;	
}

_declspec(dllexport) HRESULT GU_OffsetCameraTo(iGraphics* pGraphics, cPoint ptTo)
{
	iCamera::stParam param;
	param.ptTo = ptTo;
	param.mask = iCamera::MASK_TO ;
	pGraphics->GetiCamera()->SetParam(param,true);
	param.mask = iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	pGraphics->GetiCamera()->SetParam(param,true);
	return 0;	
}

_declspec(dllexport) HRESULT GU_CameraScreenToGround(iGraphics* pGraphics, cPoint ptScreen, cPoint& ptGround)
{
	pGraphics->GetiCamera()->ScreenToGround(ptScreen,ptGround);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//[Select]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_SetSelectType(iObj* pObj, SelectEnum e)
{
	stSelectParam param;
	param.SetMask(MASK_SELECT,stSelectParam::MASK_SELECT_TYPE);
	param.eSelect = (SelectEnum )e;
	return pObj->setParam(param);	
}

_declspec(dllexport) HRESULT GU_OffsetSelectTo(iObj* pObj, cPoint ptTo)
{
	return GU_OffsetObjTo(pObj,ptTo);
}

_declspec(dllexport) HRESULT GU_SetSelectSize(iObj* pObj, float w, float h)
{
	stSelectParam param;
	param.SetMask(MASK_SELECT,stSelectParam::MASK_SELECT_SIZE);	
	param.wSelect = w;
	param.hSelect = h;
	return pObj->setParam(param);
}

_declspec(dllexport) HRESULT GU_SetSelectOffsetEnd(iObj* pObj, cPoint ptEnd)
{
	stSelectParam param;
	param.SetMask(MASK_SELECT,stSelectParam::MASK_SELECT_OFFSETEND);	
	param.ptOffsetEnd = ptEnd;
	return pObj->setParam(param);		
}

_declspec(dllexport) HRESULT GU_ChangeSelectColor(iObj* pObj, cColor crColor)
{
	return GU_ChangeObjColor(pObj,crColor);
}

//////////////////////////////////////////////////////////////////////////////
//[map]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_GetMapClickPos(iObj* pObj, CPoint ptScreen, cPoint& ptGround)
{
	stMapParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COMMAND);
	param.eCommand = stMapParam::COM_MAP_GETCLICKPOS;
	param.ptScreenClick = ptScreen;
	HRESULT hr = pObj->GetParam(param);	
	if (!FAILED(hr))
		ptGround = param.ptGroundClick;
	return hr;
}

_declspec(dllexport) HRESULT GU_GetMapSmallClickPos(iObj* pObj, CPoint ptScreen, cPoint& ptGround)
{
	stMapSmallParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COMMAND);
	param.eCommand = stMapSmallParam::COM_MAP_GETCLICKPOS;
	param.ptScreenClick = ptScreen;
	HRESULT hr = pObj->GetParam(param);	
	if (!FAILED(hr))
		ptGround = param.ptGroundClick;
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//[obstacle]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_SetObstaclePointinfo(iObj* pObj, stObstacleParam param)
{
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);  
	return pObj->setParam(param);
}

_declspec(dllexport) HRESULT GU_SetObstacleType(iObj* pObj, ObstacleEnum e)
{
	stObstacleParam param;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_TYPE);
	param.eObstacle = e;
	return pObj->setParam(param);	
}

_declspec(dllexport) HRESULT GU_SetObstacleRadius(iObj* pObj, float r)
{
	stObstacleParam param;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_RADIUS);
	param.fRadius = r;
	return pObj->setParam(param);	
}

_declspec(dllexport) HRESULT GU_SetObstacleSelectPoint(iObj* pObj, int i)
{
	stObstacleParam param;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_SELECTPOINT);
	param.iPoint = i;
	return pObj->setParam(param);	
}

//////////////////////////////////////////////////////////////////////////////
//[light]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_CreateLight(iGraphics* pGraphcis,iObj*& pObj, float fRange , cColor cr )
{
	HRESULT hr;
	hr = pGraphcis->CreateGraphicsElement(GT_LIGHT,&pObj);
	stLightParam param;
	param.SetMask(MASK_OBJ,stLightParam::MASK_OBJ_COLOR);
	param.crColor = cr;
	pObj->setParam(param);
	param.SetMask(MASK_LIGHT,stLightParam::MASK_LIGHT_RANGE);
	param.fRange = fRange;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_AddLight(iGraphics* pGraphcis, iObj* pObj)
{
	return GU_AddObj(pGraphcis,pObj);
}

_declspec(dllexport) HRESULT GU_RemoveLight(iGraphics* pGraphcis, iObj* pObj)
{
	return GU_RemoveObj(pGraphcis,pObj);
}

_declspec(dllexport) HRESULT GU_SetEnvironmentBrightness(iGraphics* pGraphics, int nBrightness)
{
	iGround::stParam st;
	st.crColor.r = nBrightness;
	st.crColor.g = nBrightness;
	st.crColor.b = nBrightness;
	st.crColor.a = 255;
	st.mask = iGround::MASK_COLOR;
	pGraphics->GetiGround()->SetParam(st,true);
	return 0;
}

_declspec(dllexport) HRESULT GU_OffsetLightTo(iObj* pObj, cPoint ptOffset)
{
	return GU_OffsetObjTo(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_SetLightRange(iObj* pObj, float fRange)
{
	if (pObj == NULL)
		return FALSE;
	stLightParam param;
	param.SetMask(MASK_LIGHT,stLightParam::MASK_LIGHT_RANGE);
	param.fRange = fRange;
	return pObj->setParam(param);
}

//////////////////////////////////////////////////////////////////////////////
//[obj]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_CreateObj(iGraphics* pGraphcis,iObj*& pObj)
{
	HRESULT hr;
	hr = pGraphcis->CreateGraphicsElement(GT_PLANEVERDYNAMIC,&pObj);
	stPlaneVerParam param;
	param.SetMask(MASK_PLANE,
		stPlaneParam::MASK_PLANE_ANGLEXY | stPlaneParam::MASK_PLANE_ANGLEZ | stPlaneParam::MASK_PLANE_LIGHTING| stPlaneParam::MASK_PLANE_LIGHTED| stPlaneParam::MASK_PLANE_CENTER | stPlaneParam::MASK_PLANE_TYPE);
	param.ePlane = PLANE_VERDYNAMIC;
	param.fAngleXY = 0;
	param.eLighting = LIGHTING_POINT;
	param.eLighted = LIGHTED_NOLIGHTED;
	param.ptCenter = cPoint(-1,-1);
	pObj->setParam(param);
/*
	param.mask |= stPlaneParam::MASK_MAPEDIT;
	param.lMapeditEnum = Mapedit_Select|Mapedit_Trace;
	param.eOPMapedit = Bit_Add;
*/
	param.SetMask(MASK_GROUNDOBJ,stPlaneParam::MASK_GROUNDOBJ_DYNAMIC);
	param.eDynamic = DYNAMIC_NOSAVE;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_AddObj(iGraphics* pGraphcis, iObj* pObj)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	pGraphcis->GetiGround()->AddiObj(pObj,true);
	return hr;
}

_declspec(dllexport) HRESULT GU_RemoveObj(iGraphics* pGraphcis, iObj* pObj)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	pGraphcis->GetiGround()->RemoveiObj(pObj,true);
	return hr;
}

_declspec(dllexport) HRESULT GU_GetObjHeight(iObj* pObj, float& fHeight)
{
	cPoint ptOffset;
	HRESULT hr = GU_GetObjOffset(pObj,ptOffset);
	if (!FAILED(hr))
		fHeight = ptOffset.z;
	return hr;
}

_declspec(dllexport) HRESULT GU_GetObjOffset(iObj* pObj, cPoint& ptOffset)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stGroundObjParam param;
	param.ClearMask();
	hr = pObj->GetParam(param);
	if (!FAILED(hr))
		ptOffset = param.ptOffset;
	return hr;
}

_declspec(dllexport) HRESULT GU_GetObjID(iObj* pObj, long& ID)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stGroundObjParam param;
	param.ClearMask();
	hr = pObj->GetParam(param);
	if (!FAILED(hr))
		ID = param.lID;
	return hr;
}

_declspec(dllexport) HRESULT GU_GetObjDynamic(iObj* pObj, DynamicEnum& lDynamic)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stGroundObjParam param;
	param.ClearMask();
	hr = pObj->GetParam(param);
	if (!FAILED(hr))
		lDynamic = param.eDynamic;
	return hr;
}

_declspec(dllexport) HRESULT GU_OffsetObjTo(iObj* pObj, cPoint ptOffset)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_OFFSET);
	param.ptOffset = ptOffset;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_OffsetObjTo(iObj* pObj,CPoint ptScreen)
{
	if (pObj == NULL)
		return -1;

	cPoint ptOffset;
	pObj->GetOffset(ptScreen,ptOffset);	
	return GU_OffsetObjTo(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_IncreateObjFrame(iObj* pObj,int nFrame)
{
	return -1;
}

_declspec(dllexport) HRESULT GU_ChangeObjColor(iObj* pObj,cColor cr)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stObjParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COLOR);
	param.crColor = cr;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_SetObjDynamic(iObj* pObj,DynamicEnum eDynamic)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stPlaneParam::MASK_GROUNDOBJ_DYNAMIC);
	param.eDynamic = eDynamic;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_ChangeObjFrame(iObj* pObj,int nFrame)
{
	return -1;
}

_declspec(dllexport) HRESULT GU_ChangeObjAnimation(iObj* pObj,LPCSTR szSpr)
{
	return -1;
}

_declspec(dllexport) HRESULT GU_SetObjKind(iObj* pObj,KindEnum eKind)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stObstacleParam::MASK_GROUNDOBJ_KIND);
	param.eKind = eKind;
	return pObj->setParam(param);
}

_declspec(dllexport) HRESULT GU_SetObjID(iObj* pObj,long lID)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stObstacleParam::MASK_GROUNDOBJ_ID);
	param.lID = lID;
	return pObj->setParam(param);
}


_declspec(dllexport) HRESULT GU_SetObjNick(iObj* pObj,LPCSTR szNick)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_EVENT);
	strcpy(param.szEvent,szNick);
	return pObj->setParam(param);
}

_declspec(dllexport) HRESULT GU_SetObjEvent(iObj* pObj,LPCSTR szEvent)
{
	return GU_SetObjNick(pObj,szEvent);
}

_declspec(dllexport) HRESULT GU_SetPlaneLayer(iObj* pObj,int nLayer)
{
	stPlaneParam param;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_LAYER);
	param.nLayer = nLayer;
	return pObj->setParam(param);
}

_declspec(dllexport) HRESULT GU_ChangeObjAnimationArray(iObj* pObj,stResourceArray& stResArray)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerDynamicParam param;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_RESOURCE);		
	param.stResArray = stResArray;
	stResourceArray& stRes = param.stResArray;
	stRes.stDevice.format = D3DFMT_A4R4G4B4;
	stRes.stDevice.pool = D3DPOOL_MANAGED;
	stRes.stDevice.eMedia = MEDIA_VIDEO;
	stRes.stDevice.eUsage = USAGE_ANIMATION_ACTOR;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	pObj->setParam(param);

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//[missle]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_CreateMissle(iGraphics* pGraphcis,iObj*& pObj)
{
	HRESULT hr;
	hr = pGraphcis->CreateGraphicsElement(GT_PLANEVERNORMAL,&pObj);
	stPlaneVerParam param;
	param.SetMask(MASK_PLANE,
		stPlaneParam::MASK_PLANE_ANGLEXY | stPlaneParam::MASK_PLANE_ANGLEZ | 
		stPlaneParam::MASK_PLANE_LIGHTING| stPlaneParam::MASK_PLANE_LIGHTED| stPlaneParam::MASK_PLANE_CENTER | stPlaneParam::MASK_PLANE_TYPE);
	param.ePlane = PLANE_VERNORMAL;
	param.fAngleXY = 0;
	param.eLighting = LIGHTING_NOLIGHTING;
	param.eLighted = LIGHTED_NOLIGHTED;
	param.ptCenter = cPoint(-1,-1);	//use the spr center!
	param.eDynamic = DYNAMIC_NOSAVE;
	pObj->setParam(param);

	param.SetMask(MASK_GROUNDOBJ,stPlaneParam::MASK_GROUNDOBJ_DYNAMIC);
	param.eDynamic = DYNAMIC_NOSAVE;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_AddMissle(iGraphics* pGraphcis, iObj* pObj)
{
	return GU_AddObj(pGraphcis,pObj);
}

_declspec(dllexport) HRESULT GU_RemoveMissle(iGraphics* pGraphcis, iObj* pObj)
{
	return GU_RemoveObj(pGraphcis,pObj);
}

_declspec(dllexport) HRESULT GU_GetMissleOffset(iObj* pObj, cPoint& ptOffset)
{
	return GU_GetObjOffset(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_OffsetMissleTo(iObj* pObj, cPoint ptOffset)
{
	return GU_OffsetObjTo(pObj,ptOffset);
}

_declspec(dllexport) HRESULT GU_IncreateMissleFrame(iObj* pObj,int nFrame)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerParam param;
	param.ClearMask();
	pObj->GetParam(param);

	int index = param.stResA.stRes.iFrame + nFrame;
	GU_ChangeMissleFrame(pObj,index);
	return hr;
}

_declspec(dllexport) HRESULT GU_ChangeMissleDirection(iObj* pObj,int nDirection)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerParam param;
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_CHANGEDIRECTION;
	param.nDirection = nDirection;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_ChangeMissleFrame(iObj* pObj,int nFrame)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerParam param;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_RESFRAME);
	param.stResA.stRes.iFrame = nFrame;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_ChangeMissleAnimation(iObj* pObj,LPCSTR szSpr)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerParam param;
	param.SetMask(MASK_PLANE, stPlaneParam::MASK_PLANE_RESOURCE);		
	stResourceA& stRes = param.stResA;
	strcpy(stRes.stRes.szResource,szSpr);
	stRes.stDevice.format = D3DFMT_A4R4G4B4;
	stRes.stDevice.pool = D3DPOOL_MANAGED;
	stRes.stDevice.eMedia = MEDIA_VIDEOONE;
	stRes.stDevice.eUsage = USAGE_ANIMATION_ACTOR;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	pObj->setParam(param);
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	pObj->setParam(param);
	
	return hr;
}

_declspec(dllexport) HRESULT GU_ChangeMissleAniSpeed(iObj* pObj,int nAniSpeed, BOOL bAniLoop)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPlaneVerParam param;
	param.SetMask(MASK_PLANE, stPlaneParam::MASK_PLANE_ANISPEED);			
	param.nAniSpeed = nAniSpeed;
	param.bAniLoop = bAniLoop;
	pObj->setParam(param);
	return hr;
}


//////////////////////////////////////////////////////////////////////////////
//[menu]
//////////////////////////////////////////////////////////////////////////////
_declspec(dllexport) HRESULT GU_GetSprSize(iGraphics* pGraphcis, LPCSTR szSpr, long& width, long& height)
{
	return 0;
}
_declspec(dllexport) HRESULT GU_GetSprSize(iGraphics* pGraphcis, LPCSTR szSpr, int frame, long& width, long& height)
{
	return 0;
}
_declspec(dllexport) HRESULT GU_GetSprAlpha(iGraphics* pGraphcis, LPCSTR szSpr, int frame, POINT pt, BYTE& alpha)
{
	return 0;
}
_declspec(dllexport) HRESULT GU_GetSprInfo(iGraphics* pGraphcis, LPCSTR szSpr, int& frame, int& nInterval)
{
	return 0;
}

_declspec(dllexport) HRESULT GU_GetSprReferenceSpot(iGraphics* pGraphcis, LPCSTR szSpr, int& h, int& v)
{
	return 0;
}

_declspec(dllexport) HRESULT GU_CreateMenu(iGraphics* pGraphcis,iObj*& pObj)
{
	HRESULT hr = pGraphcis->CreateGraphicsElement(GT_PICTURE,&pObj);
	return hr;
}

_declspec(dllexport) HRESULT GU_AddMenu(iGraphics* pGraphcis, iObj* pObj)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	if (pGraphcis->GetiScreen())
	pGraphcis->GetiScreen()->AddiObj(pObj);
	return hr;
}

_declspec(dllexport) HRESULT GU_RemoveMenu(iGraphics* pGraphcis, iObj* pObj)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	if (pGraphcis->GetiScreen())
	pGraphcis->GetiScreen()->RemoveiObj(pObj);
	return hr;
}

_declspec(dllexport) HRESULT GU_OffsetMenuTo(iObj* pObj, POINT ptOffset)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPictureParam param;
	param.SetMask(MASK_SCREENOBJ,stPictureParam::MASK_SCREENOBJ_OFFSET);
	param.ptOffset = ptOffset;
	pObj->setParam(param);

	param.SetMask(MASK_OBJ,stPictureParam::MASK_OBJ_COMMAND);
	param.eCommand = stPictureParam::COM_PICTURE_UPDATEVERTEX;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_SetMenuSpr(iObj* pObj, LPCSTR szFile, int iFrame)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stPictureParam param;
	param.SetMask(MASK_PICTURE,stPictureParam::MASK_PICTURE_RESOURCE);
	stResourceA& stRes = param.stResA;
	stRes.stDevice.eMedia = MEDIA_VIDEO;
	stRes.stDevice.eUsage = USAGE_MENU;
	stRes.stDevice.format = D3DFMT_A4R4G4B4;
	stRes.stDevice.pool = D3DPOOL_MANAGED;
	strcpy(stRes.stRes.szResource,szFile);
	stRes.stRes.iFrame = iFrame;
	stRes.stRes.idResource = 0;
	pObj->setParam(param);
	return hr;
}

_declspec(dllexport) HRESULT GU_SetMenuColor(iObj* pObj, cColor cr)
{
	HRESULT hr = 0;
	if (pObj == NULL)
		return hr;
	stObjParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COLOR);
	param.crColor = cr;
	pObj->setParam(param);
	return hr;
}

