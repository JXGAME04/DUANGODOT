// cMesh.cpp: implementation of the cMesh class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGraphics.h"
#include "cMesh.h"
#include "cLight.h"
#include "position.h"
#include "cGround.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cMesh::cMesh(cGraphics* p) 
{
	m_ulCount = 1;

	m_pGraphics = p;
	m_aVertex = NULL;
	m_ptOffset = cPoint(0,0,0);
	m_bLight = false;
	m_bDisplay = true;
	
	m_eAnimation = ANI_BLEND_NULL;
}

cMesh::~cMesh()
{
	FinalCleanup();
}

/**************************************************************************/
//com
/**************************************************************************/
ULONG cMesh::AddRef() 
{
	return ++m_ulCount;
}

ULONG cMesh::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

void cMesh::SetOffset(cPoint pt)
{
	cPoint ptOffset = pt - m_ptOffset ;
	stVertexGround* pv = m_aVertex;
	for (int i=0; i<m_nVertexs; i++)
	{
		AddPoint(*pv,ptOffset);
		pv++;
	}
	m_ptOffset = pt;
}

/**************************************************************************/
//graphics
/**************************************************************************/
cGround* cMesh::GetGround()
{
	return GetGraphics()->GetcGround();
}

/**************************************************************************/
//create
/**************************************************************************/
HRESULT cMesh::CreateHor(stVertexGround* v, int width, int height)
{
	FinalCleanup();

	m_eType = MESH_HOR;

	m_nXVertex = width+1;
	m_nYVertex = height+1;
	m_nFaces = width * height * 2;
	m_nVertexs = (m_nXVertex )*(m_nYVertex );
	m_aVertex = new stVertexGround[m_nVertexs];
	stVertexGround* pV = m_aVertex;
	for (int i=0; i<m_nVertexs; i++)
	{
		if ((*v).sz > 0)
		{
			(*v).sz = (*v).sz;
		}
		*pV++ = *v++;
	}

	return InitDeviceObjects();
}

//该Create必须是一个矩形
HRESULT cMesh::CreateVer(stVertexGround v[4])
{
	FinalCleanup();

	m_eType = MESH_VER;
	
	float dx1 = (v[1].sx - v[0].sx);
	float dy1 = (v[1].sy - v[0].sy);
	float dz1 = (v[1].sz - v[0].sz);
	float d1 = _cpp_max(_cpp_max(fabs(dx1),fabs(dy1/2)),fabs(dz1));
	float dx2 = (v[2].sx - v[0].sx);
	float dy2 = (v[2].sy - v[0].sy);
	float dz2 = (v[2].sz - v[0].sz);
	float d2 = _cpp_max(_cpp_max(fabs(dx2),fabs(dy2/2)),fabs(dz2));

	if (d1 == 0 || d2 == 0)
	{
		ASSERT(0);
		AfxMessageBox("企图生成一个0长0宽的面,请检查面的设置");
		return FALSE;
	}

	m_fXPlane = Distance(GetPoint(v[0]),GetPoint(v[1]));
	m_fYPlane = Distance(GetPoint(v[0]),GetPoint(v[2]));

	int width = ceil(d1);
	int height = ceil(d2);
	m_nXVertex = width + 1;
	m_nYVertex = height + 1;
	float x1 = dx1 / width;
	float y1 = dy1 / width;
	float z1 = dz1 / width;
	float u1 = (v[1].tu - v[0].tu) / width;
	float v1 = (v[1].tv - v[0].tv) / width;

	float x2 = dx2 / height;
	float y2 = dy2 / height;
	float z2 = dz2 / height;
	float u2 = (v[2].tu - v[0].tu) / height;
	float v2 = (v[2].tv - v[0].tv) / height;
	
	m_nFaces = width * height * 2;
	m_nVertexs = (m_nXVertex )*(m_nYVertex );
	m_aVertex = new stVertexGround[m_nVertexs];
	
	stVertexGround* pVertex = m_aVertex;
	int x,y;
	stVertexGround vex;
	for (x=0; x<m_nXVertex; x++)
	for (y=0; y<m_nYVertex; y++)
	{
		vex.sx = v[0].sx + x1 * x + x2 * y;
		vex.sy = v[0].sy + y1 * x + y2 * y;
		vex.sz = v[0].sz + z1 * x + z2 * y;
		vex.tu = v[0].tu + u1 * x + u2 * y;
		vex.tv = v[0].tv + v1 * x + v2 * y;
		vex.color = 0;
		*pVertex++ = vex;
	}

	return InitDeviceObjects();
}

/**************************************************************************/
//paint loop
/**************************************************************************/
bool cMesh::CheckDisplay(const cRect& r)
{
	m_bDisplay = false;
	if (m_eType == MESH_HOR)
	{
		//v是一个矩形
		cRect rcMesh;
		GetGroundRect(rcMesh);
		cRect rc = r;
		m_rcDisplay = rcMesh & rc;
		if (m_rcDisplay.IsEmpty())
			m_bDisplay = FALSE;
		else
			m_bDisplay = TRUE;
	}
	else
	{
		ASSERT(m_eType == MESH_VER);
		const float c_sqrt3 = sqrt(3);
		stVertexGround v[4];
		GetQuadrangle(v);
		float h = v[0].sz - v[2].sz;
		//带高度物体
		if (v[2].sx < r.l && v[3].sx < r.l || 
			v[2].sx > r.r && v[3].sx > r.r ||
			v[2].sy < r.t && v[3].sy < r.t ||
			v[2].sy > r.b + h*c_sqrt3 && v[3].sy > r.b + h*c_sqrt3
			)
			m_bDisplay = false;
		else
			m_bDisplay = true;
	}
	return m_bDisplay;
}

/**************************************************************************/
//utility
/**************************************************************************/
void cMesh::GetBottomSeam(cPoint& p1, cPoint& p2)
{
	ASSERT(m_eType == MESH_VER);
	p1 = GetPoint(m_aVertex[m_nYVertex-1]);
	p2 = GetPoint(m_aVertex[m_nVertexs-1]);
	p1.z = 0;
	p2.z = 0;
}

void cMesh::GetQuadrangle(stVertexGround v[4])
{
	v[0] = m_aVertex[0];
	v[2] = m_aVertex[m_nYVertex-1];
	v[1] = m_aVertex[(m_nXVertex-1) * m_nYVertex];
	v[3] = m_aVertex[m_nVertexs-1];
}

void cMesh::GroundToMesh(float& x,float& y)
{
	ASSERT(m_eType == MESH_HOR);
	cRect rc;
	GetGroundRect(rc);
	x -= rc.l;
	x /= (rc.Width())/(m_nXVertex-1);
	y -= rc.t;
	y /= (rc.Height())/(m_nYVertex-1);
}

void cMesh::GetIndexRect(cRect& rcGround)
{
	GroundToMesh(rcGround.l,rcGround.t);
	GroundToMesh(rcGround.r,rcGround.b);
	rcGround.l = floor(rcGround.l);
	rcGround.t = floor(rcGround.t);
	rcGround.r = ceil(rcGround.r);
	rcGround.b = ceil(rcGround.b);
}

void cMesh::GetGroundRect(cRect& r)
{
	ASSERT(m_eType == MESH_HOR);
	stVertexGround v[4];
	GetQuadrangle(v);
	r.l = v[0].sx;
	r.r = v[0].sx;
	r.t = v[0].sy;
	r.b = v[0].sy;
	for (int i=0; i<4; i++)
	{
		if (r.l > v[i].sx)
			r.l = v[i].sx;
		if (r.r < v[i].sx)
			r.r = v[i].sx;
		if (r.t > v[i].sy)
			r.t = v[i].sy;
		if (r.b < v[i].sy)
			r.b = v[i].sy;
	}
}

/////////////////////////////////////////////////////////////
//light
/////////////////////////////////////////////////////////////
bool cMesh::BeginLighting(cColor cr)
{
	m_bLight = false;
	m_crLighting = cr;
	return true;
}

bool cMesh::EndLighting()
{
	//可以利用寄存器来传递color制，这里进行最简单的复制

	stVertexGround* pV = m_aVertex;
	for (int i=0; i<m_nVertexs; i++)
	{
		bool b = GetGround()->GetColor((*pV),(*pV).color);
		pV++;
		if (b)
			m_bLight = true;
	}
	
	//fill the color!
	return true;
}

/////////////////////////////////////////////////////////////
//device
/////////////////////////////////////////////////////////////
HRESULT cMesh::InitDeviceObjects()
{	
	return S_OK;
}

HRESULT cMesh::RestoreDeviceObjects()
{
	return S_OK;
}

HRESULT cMesh::InvalidateDeviceObjects()
{
	return S_OK;
}

HRESULT cMesh::DeleteDeviceObjects()
{
	return S_OK;
}

HRESULT cMesh::FinalCleanup()
{
	if (m_aVertex)
	{
		delete m_aVertex;
		m_aVertex = NULL;
	}
	m_nVertexs = 0;
	m_nFaces = 0;
	m_nXVertex = 0;
	m_nYVertex = 0;
	return S_OK;
}

HRESULT cMesh::Draw()
{
	return S_OK;
}

/*
bool cMesh::LightHor(cLight* pLight, bool bLightit)
{
	cRect r;
	p->GetLightRect(r);
	cRect rc;
	GetGroundRect(rc);
	cRect rect = r & rc;
	if (rect.IsEmpty())
		return false;

	if (bLightit == false)
		return true;

	//必定被照亮
	GetIndexRect(rect);
	for (int x = rect.l; x<=rect.r; x++)
	for (int y = rect.t; y<=rect.b; y++)
	{
		int i = x * (m_nYVertex) + y;
		p->Light(m_aVertex[i],m_aColor[i]);
	}
	m_bLighting = true;
	return true;
}

bool cMesh::LightVer(cLight* pLight, bool bLightit)
{
	//将底部照亮，通过插值向上
	bool bLight = false;
	cPoint ptLight = pLight->GetOffset();
	float r = pLight->GetRange();
	float height = ptLight.z;
	
	ASSERT(m_nXVertex <= 17);
	for (int x=0; x<m_nXVertex; x++)
	{
		stVertexGround v = m_aVertex[x*m_nYVertex];
		v.sz = height;
		if (pLight->CanLight(v))
		{
			bLight = true;
			if (!bLightit)
				break;
			for (int y=0; y<m_nYVertex; y++)
			{
				int index = x*m_nYVertex+y;
				pLight->Lighting(m_aVertex[index],m_aColor[index]);
			}
		}
	}
	if (bLightit && bLight)
		m_bLighting = true;
	return bLight;
}
*/
/*
bool cMesh::LightVer(cLight* pLight)
{
	cPoint ptLight = pLight->GetOffset();
	float r = pLight->GetRange();
	stVertexGround v[4];
	GetQuadrangle(v);
	D3DXPLANE plane;
	D3DXVECTOR3 v1(v[0].sx,v[0].sy,v[0].sz);
	D3DXVECTOR3 v2(v[1].sx,v[1].sy,v[1].sz);
	D3DXVECTOR3 v3(v[2].sx,v[2].sy,v[2].sz);

	D3DXVECTOR3 v0;
	D3DXPlaneFromPoints(&plane,&v1,&v2,&v3);
	v0.x = plane.a;v0.y = plane.b;v0.z = plane.c;
	D3DXVECTOR3 vLight(ptLight.x,ptLight.y,ptLight.z);
	D3DXVec3Add (&v0,&v0,&vLight);
	D3DXVECTOR3 vI;
	D3DXPlaneIntersectLine(&vI,&plane,&vLight,&v0);
	cPoint pt0(vI.x,vI.y,vI.z);
	float d = DistanceSquare(pt0,ptLight);
	if (d >= r*r)
		return false;
	float dr = sqrtf(r*r - d);
	float dx = dr / (m_fXPlane / (m_nXVertex-1));
	float dy = dr / (m_fYPlane / (m_nYVertex-1));
	float nx,ny;
	if (v1.x != v2.x)
		nx = (vI.x - v1.x) / (v2.x - v1.x) * (m_nXVertex-1);
	else if (v1.y != v2.y)
		nx = (vI.y - v1.y) / (v2.y - v1.y) * (m_nXVertex-1);
	else 
	{
		ASSERT(v2.z != v1.z);
		nx = (vI.z - v1.z) / (v2.z - v1.z) * (m_nXVertex-1);
	}
	if (v1.x != v3.x)
		ny = (vI.x - v1.x) / (v3.x - v1.x) * (m_nYVertex-1);
	else if (v1.y != v3.y)
		ny = (vI.y - v1.y) / (v3.y - v1.y) * (m_nYVertex-1);
	else 
	{
		ASSERT(v3.z != v1.z);
		ny = (vI.z - v1.z) / (v3.z - v1.z) * (m_nYVertex-1);
	}
	cRect rMesh(0,0,m_nXVertex-1,m_nYVertex-1);
	cRect rLight(floor(nx-dx),floor(ny-dy),ceil(nx+dx),ceil(ny+dy));
	cRect rc = rMesh & rLight;
	if (rc.t > rc.b || rc.l > rc.r)
		return false;
	
	//求出光源ptCenter在面V[4]上的投影pt与d = DistanceSquare(pt,p);
	//dr = sqrt(r * r - d)
	//CRect rLight = (pt - dr,pt - dr,pt + dr,pt + dr);
	//CRect rMesh = v[4];
	//CRect r = rLight & rMesh!
	
	bool bLight = false;
	for (int x = rc.l; x<= rc.r; x++)
	for (int y = rc.t; y<= rc.b; y++)
	{
		if (pLight->Light(	m_aVertex[x*m_nYVertex + y],
							m_aColor[x*m_nYVertex + y]))
				bLight = true;
	}
	if (bLight)
		m_bLighting = true;
	return bLight;
	//this is too much cost
}
*/
