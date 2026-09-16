// cPack1DPlane.cpp: implementation of the cPack1DPlane class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPack1DPlane.h"

#include "planedoodad.h"
#include "iItem.h"
#include "fileop.h"
#include "iTemplate.h"

cPack1DPlane::cPack1DPlane() 
{
}

cPack1DPlane::~cPack1DPlane()
{
}

void* cPack1DPlane::Locate(void* pStart,int frame)
{
	stTemplateOne* pTemplate = (stTemplateOne*)pStart;
	while(frame--)
	{
		int n = pTemplate->nObstacleNum;
		pTemplate++;
		pTemplate = (stTemplateOne*)( ((char*) (pTemplate)) + sizeof(stTemplateObstacle)*n ) ;
	}
	return pTemplate;
}

BOOL cPack1DPlane::LoadTemplate(LPCSTR szTemplate, char** bufferOut, DWORD& size)
{
	int frame;

	const int MAX_BUFFER = 1024 * 100;
	char buffer[MAX_BUFFER];

	CString strTemplate = szTemplate;
	AppGetFullPath(strTemplate);

	iItem* pItem = Common_CreateItem(strTemplate,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	int nFrame;
	if (!GetItemValue(pItem,"frames",nFrame))
		return FALSE;

	frame = nFrame;

	stTemplateOne* pT = ((stTemplateOne*)(buffer));
	int m_nOffset = 0;

	for (int i=0; i<nFrame; i++)
	{
		stTemplateOne& st = *pT;
		memset(&st,0,sizeof(st));

		int n = 0;
		float f = 0.f;
		CString s;
		VERIFY(GetItemValue(pItem,GetTablePlaneName(TP_PLANETYPE),s,i));
		st.ePlaneType = StringToPlaneTableValue(TP_PLANETYPE,s);

		st.eDynamicType = DYNAMIC_STATIC;
		st.eObstacleType = OBSTACLE_NULL;
		st.nCenterX = 0;
		st.nCenterY = 0;
		
		switch(st.ePlaneType)
		{
		case PLANE_HORNORMAL:
		case PLANE_HORMESH:
			st.eSort = SORT_HORBELOWVER;
			break;
		case PLANE_VERMESH:
		case PLANE_VERDYNAMIC:
			st.eSort = SORT_VER;
			st.eVersort = VERSORT_SEAM;
			break;
		case PLANE_VERNORMAL:
			st.eSort = SORT_VER;
			st.eVersort = VERSORT_POINT;
			st.eLighting = LIGHTING_POINT;
			break;
		}

		if (GetItemValue(pItem,GetTablePlaneName(TP_LIGHTING),s,i))
			st.eLighting = StringToPlaneTableValue(TP_LIGHTING,s);
		if (GetItemValue(pItem,GetTablePlaneName(TP_LIGHTED),s,i))
			st.eLighted = StringToPlaneTableValue(TP_LIGHTED,s);
		if (GetItemValue(pItem,GetTablePlaneName(TP_RADIUS),f,i))
			st.fRadiusLighted = f;
		if (GetItemValue(pItem,GetTablePlaneName(TP_SORT),s,i))
		{
			st.eSort = StringToPlaneTableValue(TP_SORT,s);
			if (st.eSort < 0 || st.eSort >= SORT_NUM)
			{
				st.eSort = SORT_HORBELOWVER;
			}
			if (st.eSort == SORT_HORABOVEVER)
				st.eSort = SORT_HORBELOWVER;
			if (st.eSort == SORT_VER)
			{
				if (st.ePlaneType == PLANE_HORNORMAL || st.ePlaneType == PLANE_HORMESH)
					st.eSort = SORT_HORBELOWVER;
			}
		}
		if (GetItemValue(pItem,GetTablePlaneName(TP_ANGLEXY),f,i))
			st.fAngleXY = f;
//		GetItemValue(pItem,GetTablePlaneName(TP_ANGLEZ),f,i);
		if (GetItemValue(pItem,GetTablePlaneName(TP_CENTERX),n,i))
			st.nCenterX = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_CENTERY),n,i))
			st.nCenterY = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_HEIGHT),f,i))
			st.fHeight = f;
		
		st.eObstacleType  = OBSTACLE_NULL;
		if (GetItemValue(pItem,GetTablePlaneName(TP_OBSTACLETYPE),s,i))
			st.eObstacleType = StringToPlaneTableValue(TP_OBSTACLETYPE,s);
		st.eDynamicType = DYNAMIC_STATIC;
		if (GetItemValue(pItem,GetTablePlaneName(TP_DYNAMICTYPE),s,i))
			st.eDynamicType = StringToPlaneTableValue(TP_DYNAMICTYPE,s);

		GetItemValue(pItem,GetTablePlaneName(TP_TEXTUREUSE),n,i);
		if (GetItemValue(pItem,GetTablePlaneName(TP_KIND),s,i))
			st.eKind = StringToPlaneTableValue(TP_KIND,s);
		if (GetItemValue(pItem,GetTablePlaneName(TP_VERSORT),s,i))
			st.eVersort = StringToPlaneTableValue(TP_VERSORT,s);;
		if (GetItemValue(pItem,GetTablePlaneName(TP_ANISPEED),n,i))
			st.nAnispeed = n;
		st.bAniLoop = TRUE;
		if (GetItemValue(pItem,GetTablePlaneName(TP_ANILOOP),n,i))
			st.bAniLoop = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_SPRWIDTH),n,i))
			st.nSprCenterX = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_SPRHEIGHT),n,i))
			st.nSprCenterY = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERX),n,i))
			st.nSprWidth = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_SPRCENTERY),n,i))
			st.nSprHeight = n;
		if (GetItemValue(pItem,GetTablePlaneName(TP_OBSTACLENUM),n,i))
			st.nObstacleNum = n;

		pT++;
		m_nOffset += sizeof(*pT) ;
		//m_nLength += sizeof(stTemplateOne) ;
		ASSERT(st.nObstacleNum <= 4);
		if (st.nObstacleNum > 0)
		{
			stTemplateObstacle* pTO = ((stTemplateObstacle*) pT);
			for (int j=0; j<st.nObstacleNum; j++)
			{
				stTemplateObstacle& st = *pTO++;
				m_nOffset += sizeof(*pTO);

				CString sKey;
				sKey.Format("obstacletype(%d)",j);
				VERIFY(GetItemValue(pItem,sKey,n,i));
				st.eObstacleType = n;
				sKey.Format("obstaclepointnum(%d)",j);
				VERIFY(GetItemValue(pItem,sKey,n,i));
				st.eObstacleNum = n;
				ASSERT(n > 0);
				sKey.Format("obstaclekind(%d)",j);
				VERIFY(GetItemValue(pItem,sKey,s,i));
				st.eObstacleKind = StringToPlaneTableValue(TP_KIND,s);
				sKey.Format("obstaclepointarray(%d)",j);
				CString sValue;
				VERIFY(GetItemValue(pItem,sKey,sValue,i));
				StringToPointArray(sValue,st.aPoint);
			}
			pT = (stTemplateOne*)pTO;
		}
	}

	char* pBuffer = new char[m_nOffset+sizeof(long)];
	*((long*)pBuffer) = frame;
	memcpy(pBuffer+sizeof(long),buffer,m_nOffset);
	size = m_nOffset + sizeof(long);

	*bufferOut = pBuffer;
	return TRUE;
}

