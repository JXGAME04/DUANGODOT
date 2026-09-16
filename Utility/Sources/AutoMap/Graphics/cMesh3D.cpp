// cMesh3D.cpp: implementation of the cMesh3D class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGraphics.h"
#include "cMesh3D.h"
#include "cGround.h"
#include "position.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cMesh3D::cMesh3D(cGraphics* p) : cMesh(p)
{
	m_pIB = NULL;
	m_pVB = NULL;

	m_nRun = 0;
	m_fAlpha0 = 0;
	m_nSpeed = 5;
	m_fDegree = 16.f;
	m_bLight = false;
}

cMesh3D::~cMesh3D()
{
	FinalCleanup();	
}

LPDIRECT3DDEVICE8 cMesh3D::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

void cMesh3D::SetOffset(cPoint pt)
{
	cMesh::SetOffset(pt);
	HRESULT hr;
	stVertexGround* pVertex;
	hr = m_pVB->Lock(0,0,(BYTE**)&pVertex,0);
	ASSERT(!FAILED(hr));
	stVertexGround* pv = m_aVertex;
	for (int i=0; i<m_nVertexs; i++)
		*pVertex++ = *pv++;
	hr = m_pVB->Unlock();
	ASSERT(!FAILED(hr));
}

HRESULT cMesh3D::InitDeviceObjects()
{
	cMesh::InitDeviceObjects();
	ASSERT(m_nXVertex && m_nYVertex);
	ASSERT(m_pIB == NULL && m_pVB == NULL);
	int x,y;

	HRESULT hr;
	hr = GetDevice()->CreateVertexBuffer(m_nVertexs*sizeof(stVertexGround), 
										D3DUSAGE_WRITEONLY,
										D3DFVF_GROUNDVERTEX, 
										D3DPOOL_MANAGED, 
 										&m_pVB);
	GetGraphics()->DXError(hr);
	if (FAILED(hr))
		return hr;
	stVertexGround* pVertex;
	hr = m_pVB->Lock(0,0,(BYTE**)&pVertex,0);
	ASSERT(!FAILED(hr));
	stVertexGround* pv = m_aVertex;
	for (int i=0; i<m_nVertexs; i++)
		*pVertex++ = *pv++;
	hr = m_pVB->Unlock();
	ASSERT(!FAILED(hr));

    hr = GetDevice()->CreateIndexBuffer( m_nFaces*3*sizeof(WORD),
                                          D3DUSAGE_WRITEONLY,
                                          D3DFMT_INDEX16, D3DPOOL_MANAGED,
                                          &m_pIB );
	ASSERT(!FAILED(hr));
	WORD* pIndex;
	hr = m_pIB->Lock(0, 0, (BYTE**) &pIndex, 0);
	ASSERT(!FAILED(hr));
	for (x=0; x<m_nXVertex-1; x++)
	for (y=0; y<m_nYVertex-1; y++)
	{
		int index = x * (m_nYVertex) + y;
		*pIndex++ = index;
		*pIndex++ = index+1;
		*pIndex++ = index+(m_nYVertex);
		*pIndex++ = index+1;
		*pIndex++ = index+(m_nYVertex);
		*pIndex++ = index+(m_nYVertex) + 1;
	}
	hr = m_pIB->Unlock();
	ASSERT(!FAILED(hr));
	return hr;
}

//begin and end

HRESULT cMesh3D::RestoreDeviceObjects()
{
	cMesh::RestoreDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cMesh3D::InvalidateDeviceObjects()
{
	cMesh::InvalidateDeviceObjects();
	HRESULT hr = S_OK;
	return hr;	
}

HRESULT cMesh3D::DeleteDeviceObjects()
{
	cMesh::DeleteDeviceObjects();
	HRESULT hr = S_OK;
	RELEASE(m_pVB);
	RELEASE(m_pIB);
	return hr;
}

HRESULT cMesh3D::FinalCleanup()
{
	RELEASE(m_pVB);
	RELEASE(m_pIB);
	return cMesh::FinalCleanup();
}

//绘制局部
/*
HRESULT cMesh3D::DrawSub(cPoint p1,cPoint p2)
{
	return Draw();
}
*/

bool cMesh3D::EndLighting()
{
	int x,y;
	HRESULT hr;
	stVertexGround* pVertex;
	hr = m_pVB->Lock(0,0,(BYTE**)&pVertex,0);
	ASSERT(!FAILED(hr));
	stVertexGround* pV = m_aVertex;

	if (m_eType == MESH_HOR)
	{
		cRect rcIndex = m_rcDisplay;
		GetIndexRect(rcIndex);
		for (x = rcIndex.l; x <= rcIndex.r; x++)
		for (y = rcIndex.t; y <= rcIndex.b; y++)
		{
			stVertexGround& v = pVertex[x*m_nYVertex+y];
			bool b = GetGround()->GetColor(v,v.color);
			if (b)
				m_bLight = true;
		}
	}
	else //ver
	{
		for (x=0; x<m_nXVertex; x++)
		{
			stVertexGround& v = pVertex[x*m_nYVertex];
			DWORD color;
			bool b = GetGround()->GetColor(v,color);//(0,0)点的color!
			if (b)
			{
				m_bLight = true;
				stVertexGround* pV = &pVertex[x*m_nYVertex];
//				for (y=0; y<m_nYVertex; y++)
//					(pV++)->color = color;
				cColor c0(color);
				cColor c1 = m_crLighting;	
				DWORD value1 = c1.Value();
				stLightParam param;
				GetGround()->GetLightParam(v,param);
				float fz = param.ptOffset.z;//光源高度
				float r = param.fRange;//光源强度
				//插值得到其他的顶点值
				const float f = c_fSqrt3 / 2;
				for (y=0; y<m_nYVertex; y++)
				{
					float dz = fabs(pV->sz - fz);
					if (dz < r/f)
					{
						cColor c = c0 - (c0 - c1) * (dz/ (r/f));
						(pV++)->color = c.Value();
					}
					else
						(pV++)->color = value1;
				}
			}
			else //未被照亮
			{
				stVertexGround* pV = &pVertex[x*m_nYVertex];
				for (y=0; y<m_nYVertex; y++)
					(pV++)->color = color;
			}
		}
		//fill the ver
	}
	hr = m_pVB->Unlock();
	return true;
}

HRESULT cMesh3D::DrawHor()
{
	ASSERT(!m_rcDisplay.IsEmpty());
	cRect rcIndex = m_rcDisplay;
	GetIndexRect(rcIndex);
	
	if (!m_bLight && m_eAnimation == ANI_BLEND_NULL && 0)
	{
		stVertexGround v[4];
		int l = rcIndex.l;
		int r = rcIndex.r;
		int t = rcIndex.t;
		int b = rcIndex.b;
		v[0] = m_aVertex[l*m_nYVertex+t];
		v[1] = m_aVertex[r*m_nYVertex+t];
		v[2] = m_aVertex[l*m_nYVertex+b];
		v[3] = m_aVertex[r*m_nYVertex+b];
		long value = m_crLighting.Value();
		for (int i=0; i<4; i++)
			v[i].color = value;
		GetGraphics()->GetcGround()->DrawQuadrangle(v);
		return S_OK;
	}

	HRESULT hr;
	int nXVertex = rcIndex.Width()+1;
	int nYVertex = rcIndex.Height()+1;
	int nFaces = rcIndex.Width() * rcIndex.Height() * 2;

	WORD* pIndex;
	ASSERT(m_pIB);
	hr = m_pIB->Lock(0, 0, (BYTE**) &pIndex, 0);
	ASSERT(!FAILED(hr));
	int x,y;
	for (x=rcIndex.l; x<rcIndex.r; x++)
	for (y=rcIndex.t; y<rcIndex.b; y++)
	{
		int index = x * (m_nYVertex) + y;
		*pIndex++ = index;
		*pIndex++ = index+1;
		*pIndex++ = index+(m_nYVertex);
		*pIndex++ = index+1;
		*pIndex++ = index+(m_nYVertex);
		*pIndex++ = index+(m_nYVertex) + 1;
	}
	hr = m_pIB->Unlock();
	ASSERT(!FAILED(hr));

    GetDevice()->SetVertexShader( D3DFVF_GROUNDVERTEX );
    GetDevice()->SetStreamSource( 0, m_pVB, sizeof(stVertexGround));
    GetDevice()->SetIndices( m_pIB, 0L );
	GetGraphics()->IncreaseFace(nFaces);
	GetDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, m_nVertexs, 0, nFaces);
	return hr;
}

HRESULT cMesh3D::DrawVer()
{
	//use m_rcDisplay to show 
	HRESULT hr = S_OK;
    GetDevice()->SetVertexShader( D3DFVF_GROUNDVERTEX );
    GetDevice()->SetStreamSource( 0, m_pVB, sizeof(stVertexGround));
	GetDevice()->SetIndices( m_pIB, 0L );
	GetGraphics()->IncreaseFace(m_nFaces);
	GetDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, m_nVertexs, 0, m_nFaces);
//	GetDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_nVertexs);
	return hr;
}

HRESULT cMesh3D::Run()
{
//	m_eAnimation = ANI_BLEND_SLIGHT;
	if (m_eAnimation == ANI_BLEND_NULL)
		return S_OK;
	if (m_eType == MESH_VER)
		return S_OK;
	if (m_nRun++ < m_nSpeed)
		return S_OK;
	m_nRun = 0;
	float fAngle0 = c_fPI * 2.f/(float)(m_nXVertex-2);
	m_fAlpha0 += fAngle0;
	switch (m_eAnimation)
	{
	case ANI_BLEND_SLIGHT:
		{
			stVertexGround* pVertex;
			HRESULT hr = m_pVB->Lock(0,0,(BYTE**)&pVertex,0);
			ASSERT(!FAILED(hr));
			stVertexGround* pv = m_aVertex;
			int x,y;
			for (x=1; x<m_nXVertex-1; x++)
			for (y=1; y<m_nYVertex-1; y++)
			{
				#pragma message (ATTENTION("让它更加缓和"))
				float fdiv = m_fDegree;
				if (x == 1 || x == m_nXVertex-2)
					 fdiv *= 2;					
				float z = sin(m_fAlpha0 + (x-1)*fAngle0) / fdiv;
				pVertex[x*m_nYVertex+y].sz = z;
			}
			if (!m_bLight)
			{
				long value = m_crLighting.Value();
				for (int i=0; i<m_nVertexs; i++)
				{
					pVertex->color = value;
					pVertex++;
				}
				//fill the color
			}
			hr = m_pVB->Unlock();
			ASSERT(!FAILED(hr));
		}
		break;
	}
	return S_OK;
}

HRESULT cMesh3D::Draw()
{
	if (!IsDisplay())
		return false;

	Run();

	HRESULT hr;
	if (m_eType == MESH_HOR)
		hr = DrawHor();
	else if (m_eType == MESH_VER)
		hr = DrawVer();
	
	//这个只在编辑器提供就不优化了
	if (m_lMapeditParam == Mapedit_Grid )
	{
		if (m_eType == MESH_HOR)
		{
			stVertexGround v[4];
			int l = 0;
			int r = m_nXVertex-1;
			int t = 0;
			int b = m_nYVertex-1;
			v[0] = m_aVertex[l*m_nYVertex+t];
			v[1] = m_aVertex[r*m_nYVertex+t];
			v[2] = m_aVertex[l*m_nYVertex+b];
			v[3] = m_aVertex[r*m_nYVertex+b];
			for (int i=0; i<4; i++)
				v[i].color = c_lWriteColor;
			cRect rcUV(0,0,1,1);
			FillRectUV(v,rcUV);
			GetGraphics()->SetCommonTexture(cGraphics::TEX_GRID,0);
			GetGraphics()->GetcGround()->DrawQuadrangle(v);
		}
	}
	return hr;
}
