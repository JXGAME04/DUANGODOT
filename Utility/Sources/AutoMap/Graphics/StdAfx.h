// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EA55FEA9_90FB_40DA_B59A_ECA4839D3B27__INCLUDED_)
#define AFX_STDAFX_H__EA55FEA9_90FB_40DA_B59A_ECA4839D3B27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <d3d8.h>
#include <d3dx8.h>
#include <Dxerr8.h>

#undef max
#undef min

#pragma warning (disable: 4786)
#pragma warning (disable: 4503)

#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <limits>
//#include <xutility>

const long MAX_LONG = std::numeric_limits<long>::max();
const long MIN_LONG = std::numeric_limits<long>::min();

//#include "iGraphics.h"
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define _cpp_min min
#define _cpp_max max

class cGraphics;
class cGround;
class cScreen;
class cCamera;

class cThread;
class cMesh;
class cGroundObj;
class cScreenObj;

class cSelect;
class cPlane;
typedef std::list<cPlane*>			cPlaneList;
typedef cPlaneList::iterator	itcPlane;
class cPlaneHor;
typedef std::list<cPlaneHor*>		cPlaneHorList;
typedef cPlaneHorList::iterator	itcPlaneHor;
class cPlaneVer;
typedef std::list<cPlaneVer*>		cPlaneVerList;
typedef cPlaneVerList::iterator	itcPlaneVer;
class cLight;
typedef std::list<cLight*>			cLightList;
typedef cLightList::iterator	itcLight;
class cObstacle;
typedef std::list<cObstacle*>		cObstacleList;
typedef cObstacleList::iterator	itcObstacle;
class cRegion;
typedef std::list<cRegion*>			cRegionList;
typedef cRegionList::iterator	itcRegion;
class cPicture;
typedef std::list<cPicture*>			cPictureList;
typedef cPictureList::iterator	itcPicture;
class cMesh;
typedef std::list<cMesh*> MeshList;
typedef MeshList::iterator itMesh;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__EA55FEA9_90FB_40DA_B59A_ECA4839D3B27__INCLUDED_)
