#include "stdafx.h"
#include "cGraphics.h"
#include "cGraphics3D.h"
#include "cplane.h"
#include "cregion.h"
#include "cground.h"
#include "template.h"

_declspec(dllexport) int updateimageinfo(LPCSTR szPlaneFile)
{
	cPlaneList plist;
	cGraphics3D gra;
	T_LoadPlaneList<cPlaneList,cPlane>(plist,szPlaneFile,&gra);
	for_all(plist,mem_fun<int,cPlane>(&cPlane::MC_GetImageInfo));
	T_SaveList(plist,szPlaneFile);
	for_all(plist,mem_fun<ULONG,cPlane>(&cPlane::Release));
	return 0;
}

_declspec(dllexport) int createlinkhor(LPCSTR szRegionFolder)
{
	cGraphics3D gra;
	cRegion region(&gra);
	region.MC_GetLinkInfo(szRegionFolder);
	return 0;
}
