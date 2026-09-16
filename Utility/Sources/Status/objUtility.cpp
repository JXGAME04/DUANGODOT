#include "stdafx.h"
#include "iGraphics.h"
#include "iCommand.h"
#include "cStatus.h"
#include "objUtility.h"
#include "iItem.h"
#include "fileop.h"
#include "planedoodad.h"

void OffsetObjTo(iObj* pObj,CPoint ptScreen,bool bRunnow)
{
	if (pObj == NULL)
		return;

	cPoint ptOffset;
	pObj->GetOffset(ptScreen,ptOffset);
	OffsetObjTo(pObj,ptOffset,bRunnow);
}

void OffsetObjTo(iObj* pObj,cPoint ptOffset,bool bRunnow)
{
	if (pObj == NULL)
		return;

	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_OFFSET);
	param.ptOffset = ptOffset;
	SetObjParam(pObj,param,bRunnow);
}

void OffsetObj(iObj* pObj,cPoint ptOffset,bool bRunnow)
{
	if (pObj == NULL)
		return;

	stGroundObjParam param;
	param.ClearMask();
	pObj->GetParam(param);
	param.SetMask(MASK_GROUNDOBJ, stGroundObjParam::MASK_GROUNDOBJ_OFFSET);
	param.ptOffset += ptOffset;
	OffsetObjTo(pObj,param.ptOffset,bRunnow);
/*
	SetObjParam()
	stCommand_OffsetObj st;
	st.type = COM_OBJ_OFFSET;
	st.size = sizeof(st);
	st.pObj = pObj;
	st.ptOffset = ptOffset;
	cStatus::DoCommand(&st);	
*/
}

void RemoveAndReleaseObj(iObj*& pObj)
{
	RemoveObj(pObj);
	SetObjTrace(pObj,false);
	SetObjSelect(pObj,false);
	RELEASE(pObj);
}

void RemoveObj(iObj* pObj)
{
	if (pObj)
	{
		stCommand_AddObj st;
		st.type = COM_OBJ_REMOVE;
		st.size = sizeof(st);
		pObj->AddRef();
		st.pObj = pObj;
		cStatus::DoCommand(&st);
	}
}

void SetObjSelect(iObj* pObj, bool bSelect)
{
	if (pObj == NULL)
		return;
	stObjParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_MAPEDIT);
	param.lMapeditEnum = Mapedit_Select;
	if (bSelect == TRUE)
		param.eOPMapedit = Bit_Add;
	else
		param.eOPMapedit = Bit_Remove;
//	SetObjParam(pObj,param);
	pObj->SetParam(param);
	SetObjMoving(pObj,bSelect);
}

void SetObjMoving(iObj* pObj, bool bMoving)
{
	if (pObj == NULL)
		return;
	stObjParam param;
	param.ClearMask();
	pObj->GetParam(param);
	if (param.eElement == GT_PLANEHORNORMAL || param.eElement == GT_PLANEHORMESH)
	{
		stPlaneParam param;
		param.ClearMask();
		pObj->GetParam(param);
		if (param.eSort == SORT_HORBELOWVER || param.eSort == SORT_HORGROUND)
		{
			param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_MOVING);
			param.bMoving = bMoving;
			SetObjParam(pObj,param);
//			pObj->SetParam(param);
		}
	}
}

void SetObjTrace(iObj* pObj, bool bTrace)
{
	if (pObj == NULL)
		return;
	stObjParam param;
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_MAPEDIT);
	param.lMapeditEnum = Mapedit_Trace;
	if (bTrace == TRUE)
		param.eOPMapedit = Bit_Add;
	else
		param.eOPMapedit = Bit_Remove;
//	SetObjParam(pObj,param);
	pObj->SetParam(param);
}

void SetObjParam(iObj* pObj,stObjParam& param,bool bRunNow)
{
	if (bRunNow == false)
	{
		stCommand_SetObjParam cmd;
		cmd.size = sizeof(cmd);
		cmd.type = COM_OBJ_SETPARAM;
		pObj->AddRef();
		cmd.pObj = pObj;
		cmd.pParam = &param;
		cStatus::DoCommand(&cmd);
	}
	else
	{
		pObj->setParam(param);
	}
}

void AddObj(iObj* pObj)
{
	stCommand_AddObj st;
	st.type = COM_OBJ_ADD;
	st.size = sizeof(st);
	pObj->AddRef();
	st.pObj=  pObj;
	cStatus::DoCommand(&st);	
}

void AddObjHeight(iObj* pObj,float fHeight)
{
	stPlaneParam param;
	param.ClearMask();
	pObj->GetParam(param);

	param.fHeight += fHeight;
	SetObjHeight(pObj,param.fHeight);
}

void SetObjHeight(iObj* pObj,float fHeight)
{
	stPlaneParam param;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_HEIGHT);
	param.fHeight = fHeight;

	SetObjParam(pObj,param);
}

void SetObjGroupFile(iObj* pObj,LPCSTR szFile,bool bRunnow)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_GROUPFILE);
	strcpy(param.szGroup,szFile);

	SetObjParam(pObj,param,bRunnow);
}

void SetObjID(iObj* pObj,int id,bool bRunnow)
{
	stGroundObjParam param;
	param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_GROUPID);
	param.lGroupID = id;

	SetObjParam(pObj,param,bRunnow);
}

void CreateLight(iGraphics* pGraphics,iObj* & pLight, stLightParam& param)
{
	pGraphics->CreateGraphicsElement(GT_LIGHT,&pLight);
	param.SetMask(MASK_LIGHT,-1);
	pLight->setParam(param);
	param.SetMask(MASK_OBJ,stLightParam::MASK_OBJ_COLOR) ;
	pLight->setParam(param);
}

void CreateObstacle(iGraphics* pGraphics,iObj* & pObstacle, stObstacleParam& param)
{
	pGraphics->CreateGraphicsElement(GT_OBSTACLE,&pObstacle);
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_TYPE | stObstacleParam::MASK_OBSTACLE_POINTINFO);  
	pObstacle->setParam(param);
}

void CreatePlane(iGraphics* pGraphics,iObj* & pPlane, LPCSTR szFile, int nFrame)
{
	ElementEnum gtype ;

	//设置当前坐标
	stPlaneParam param;
	param.ClearMask();

	CString strFile = szFile;
	AppGetShortPath(strFile);

	CString strValue;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	ASSERT(pItem);
	wrap_ptr<iItem> ptr(pItem);
	
	VERIFY(GetItemValue(pItem,GetTablePlaneName(TP_PLANETYPE),strValue,nFrame));
	param.ePlane = (PlaneEnum)StringToPlaneTableValue(TP_PLANETYPE,strValue);
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_TYPE);
	switch (param.ePlane)
	{
	case PLANE_HORNORMAL:
		gtype = GT_PLANEHORNORMAL;
		break;
	case PLANE_HORMESH:
		gtype = GT_PLANEHORMESH;
		break;
	case PLANE_VERNORMAL:
		gtype = GT_PLANEVERNORMAL;
		break;
	case PLANE_VERMESH:
		gtype = GT_PLANEVERMESH;
		break;
	default:
		ASSERT(0);
	}

	pGraphics->CreateGraphicsElement(gtype,&pPlane);

	param.AddMask(stPlaneParam::MASK_PLANE_RESFRAME);
	param.stResA.stRes.iFrame = nFrame;
	pPlane->setParam(param);
	param.SetMask(MASK_GROUNDOBJ,stPlaneParam::MASK_GROUNDOBJ_TEMPLATE);
	strcpy(param.szTemplate,strFile);
	param.iTemplate = nFrame;
	pPlane->setParam(param);
	//根据文件设置属性
	param.SetMask(MASK_OBJ,stPlaneParam::MASK_OBJ_COMMAND);
	param.eCommand = stPlaneParam::COM_PLANE_LOADFROMNAME;
	strcpy(param.szTemplate,strFile);
	pPlane->setParam(param);
	param.eCommand = stPlaneParam::COM_PLANE_UPDATETEXTURE;
	pPlane->setParam(param);	
	param.eCommand = stPlaneParam::COM_PLANE_UPDATEVERTEX;
	pPlane->setParam(param);	
}
