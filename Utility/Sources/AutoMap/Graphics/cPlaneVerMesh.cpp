// cPlaneVerMesh.cpp: implementation of the cPlaneVerMesh class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPlaneVerMesh.h"

#include "cGraphics.h"
#include "cMesh.h"
#include "position.h"
#include "cLight.h"
#include "cGround.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlaneVerMesh::cPlaneVerMesh(cGraphics* p) : cPlaneVer(p)
{
}

cPlaneVerMesh::~cPlaneVerMesh()
{
	FinalCleanup();
}

void cPlaneVerMesh::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneVerParam;
	cPlaneVer::Init(false);
	m_pParam->eElement = GT_PLANEVERMESH;
}

void cPlaneVerMesh::ClearMesh()
{
	for_all(m_listMesh, mem_fun<ULONG,cMesh>(&cMesh::Release));
	m_listMesh.clear();
	for_all(m_listMeshOld, mem_fun<ULONG,cMesh>(&cMesh::Release));
	m_listMeshOld.clear();
}

void cPlaneVerMesh::GetSortPoint(cPoint& p)
{
	if (GetVersortType() == VERSORT_POINT)
	{
		cPlaneVer::GetSortPoint(p);
		return;
	}
	cPoint p1,p2;
	GetSortSeam(p1,p2);
	if (p1.y < p2.y)
		p = p1;
	else
		p = p2;
	/*
	p = p1+p2;
	p.x /= 2;
	p.y /= 2;
	p.z /= 2;
	*/
}

bool cPlaneVerMesh::BeginBuildTree()
{
	for_all(m_listMeshOld, mem_fun<ULONG,cMesh>(&cMesh::Release));
	m_listMeshOld.clear();
	m_listMeshOld = m_listMesh;
	m_listMesh.clear();
	return true;
}

bool cPlaneVerMesh::EndBuildTree()
{
	for_all(m_listMeshOld, mem_fun<ULONG,cMesh>(&cMesh::Release));
	m_listMeshOld.clear();
	return true;
}

HRESULT cPlaneVerMesh::FinalCleanup()
{
	ClearMesh();
	ClearTexture();
	return S_OK;
}

cMesh* cPlaneVerMesh::FindMesh(cPoint p1,cPoint p2,MeshList& mlist)
{
	for (itMesh it = mlist.begin(); it != mlist.end(); ++it)
	{
		cPoint pt1,pt2;
		(*it)->GetBottomSeam(pt1,pt2);
		if (p1 == pt1 && p2 == pt2)
			return (*it);
	}
	return NULL;
}

HRESULT	cPlaneVerMesh::BuildMesh(cPoint p1,cPoint p2)
{
	cMesh* p = FindMesh(p1,p2,m_listMeshOld);
	if (p == NULL)
	{
		GetGraphics()->CreateGraphicsElement(GT_MESH,&p);
		stVertexGround v[4];
		GetSubQuadrangle(v,p1,p2);
		p->CreateVer(v);
	}
	else
	{
		m_listMeshOld.remove(p);
	}
	m_listMesh.push_back(p);
	return S_OK;
}

HRESULT	cPlaneVerMesh::DrawSub(cPoint p1,cPoint p2)
{
	cMesh* pMesh = FindMesh(p1,p2,m_listMesh);
//	ASSERT(pMesh);
	if (pMesh != NULL && pMesh->IsLighting())
	{
		GetGround()->SetTexture(GetTexture(0),GetiFrame(0));
		pMesh->Draw();
	}
	else
		cPlaneVer::DrawSub(p1,p2);
	return S_OK;
}

bool cPlaneVerMesh::BeginLighting(cColor cr)
{
	cPlaneVer::BeginLighting(cr);
	for_all(m_listMesh,bind2nd(mem_fun1<bool,cMesh,cColor>(&cMesh::BeginLighting),cr));
	return true;
}

bool cPlaneVerMesh::EndLighting()
{
	cPlaneVer::EndLighting();
	for_all(m_listMesh,mem_fun<bool,cMesh>(&cMesh::EndLighting));
	return true;
}


