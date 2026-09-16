// cPlaneVerMesh.h: interface for the cPlaneVerMesh class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANEVERMESH_H__C5BA1923_B3ED_4A0F_BA67_121D725BC880__INCLUDED_)
#define AFX_CPLANEVERMESH_H__C5BA1923_B3ED_4A0F_BA67_121D725BC880__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPlaneVer.h"

class cPlaneVerMesh : public cPlaneVer  
{
public:
	cPlaneVerMesh(cGraphics* p);
	virtual ~cPlaneVerMesh();
	virtual void			Init(bool bCreateBuffer);

	LightingEnum			GetLightingType(){return LIGHTING_SEAM;};
	HRESULT					FinalCleanup();
	
	void					GetSortPoint(cPoint& p);

	MeshList				m_listMesh;
	MeshList				m_listMeshOld;
	void					ClearMesh();
	virtual bool			BeginBuildTree();
	virtual bool			EndBuildTree();
	bool					BeginLighting(cColor cr);
	bool					EndLighting();

	HRESULT					BuildMesh(cPoint p1,cPoint p2);

	cMesh*					FindMesh(cPoint p1,cPoint p2,MeshList& mlist);
	HRESULT					DrawSub(cPoint p1,cPoint p2);
};

#endif // !defined(AFX_CPLANEVERMESH_H__C5BA1923_B3ED_4A0F_BA67_121D725BC880__INCLUDED_)
