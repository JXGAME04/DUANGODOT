# Microsoft Developer Studio Project File - Name="Graphics" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Graphics - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Graphics.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Graphics.mak" CFG="Graphics - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Graphics - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Graphics - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Client/Graphics", WVCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Graphics - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\..\Headers" /I "..\..\..\swordonline\sources\engine\src" /I "..\..\..\swordonline\Headers" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 dxerr8.lib D3dxof.lib dxguid.lib d3dx8dt.lib d3d8.lib d3dx8.lib Winmm.lib message.lib engine.lib image.lib common.lib mfcs42.lib MSVCRT.lib zip.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"mfcs42.lib MSVCRT.lib" /def:".\Graphics.def" /out:"..\..\run_release\Graphics.dll" /implib:"../../lib/Graphics.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\Headers" /I "..\..\..\swordonline\sources\engine\src" /I "..\..\..\swordonline\Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxerr8.lib D3dxof.lib dxguid.lib d3dx8dt.lib d3d8.lib d3dx8.lib Winmm.lib mfcs42d.lib MSVCRTD.lib ..\..\lib\message.lib ..\..\lib\image.lib ..\..\lib\common.lib ..\..\lib\engine.lib zip.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"mfcs42d.lib MSVCRTD.lib" /def:".\Graphics.def" /out:"..\..\run\Graphics.dll" /implib:"../../lib/Graphics.lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Graphics - Win32 Release"
# Name "Graphics - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Dx Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dx\d3dapp.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Dx\d3dfile.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Dx\d3dfont.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Dx\d3dutil.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Dx\dxutil.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=.\cCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\cColorMap.cpp
# End Source File
# Begin Source File

SOURCE=.\cColorTile.cpp
# End Source File
# Begin Source File

SOURCE=.\cGraphics.cpp
# End Source File
# Begin Source File

SOURCE=.\cGround.cpp
# End Source File
# Begin Source File

SOURCE=.\cMap.cpp
# End Source File
# Begin Source File

SOURCE=.\cMapObstacle.cpp
# End Source File
# Begin Source File

SOURCE=.\cMapRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\cMapSmall.cpp
# End Source File
# Begin Source File

SOURCE=.\cRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\cRegionTile.cpp
# End Source File
# Begin Source File

SOURCE=.\cScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureOne.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureVideoDirection.cpp
# End Source File
# Begin Source File

SOURCE=.\Graphics.cpp
# End Source File
# Begin Source File

SOURCE=.\Graphics.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Graphics.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Dx Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Headers\d3dapp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Headers\d3dfile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Headers\d3dfont.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Headers\d3dutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cCamera.h
# End Source File
# Begin Source File

SOURCE=.\cColorMap.h
# End Source File
# Begin Source File

SOURCE=.\cColorTile.h
# End Source File
# Begin Source File

SOURCE=.\cGraphics.h
# End Source File
# Begin Source File

SOURCE=.\cGround.h
# End Source File
# Begin Source File

SOURCE=.\cMap.h
# End Source File
# Begin Source File

SOURCE=.\cMapObstacle.h
# End Source File
# Begin Source File

SOURCE=.\cMapRegion.h
# End Source File
# Begin Source File

SOURCE=.\cMapSmall.h
# End Source File
# Begin Source File

SOURCE=.\cRegion.h
# End Source File
# Begin Source File

SOURCE=.\cRegionTile.h
# End Source File
# Begin Source File

SOURCE=.\cScreen.h
# End Source File
# Begin Source File

SOURCE=.\cTextureOne.h
# End Source File
# Begin Source File

SOURCE=.\cTextureVideoDirection.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\cTimer.h
# End Source File
# Begin Source File

SOURCE=.\Graphics.h
# End Source File
# Begin Source File

SOURCE=.\GraphicsPublic.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iGraphics.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iPublic.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Graphics.rc2
# End Source File
# End Group
# Begin Group "Œƒ◊÷œ‘ æ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Font\iFont.h
# End Source File
# Begin Source File

SOURCE=.\Font\KCharSet.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Font\KCharSet.h
# End Source File
# Begin Source File

SOURCE=.\Font\KFont.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Font\KFont.h
# End Source File
# Begin Source File

SOURCE=.\Font\KFontD3D.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Font\KFontD3D.h
# End Source File
# Begin Source File

SOURCE=.\Font\KFontData.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Font\KFontData.h
# End Source File
# Begin Source File

SOURCE=.\Font\KFontRes.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Font\KFontRes.h
# End Source File
# Begin Source File

SOURCE=.\Font\KMRU.cpp

!IF  "$(CFG)" == "Graphics - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Graphics - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Font\KMRU.h
# End Source File
# End Group
# Begin Group "Obj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cGroundObj.cpp
# End Source File
# Begin Source File

SOURCE=.\cGroundObj.h
# End Source File
# Begin Source File

SOURCE=.\cLight.cpp
# End Source File
# Begin Source File

SOURCE=.\cLight.h
# End Source File
# Begin Source File

SOURCE=.\cObj.cpp
# End Source File
# Begin Source File

SOURCE=.\cObj.h
# End Source File
# Begin Source File

SOURCE=.\cObstacle.cpp
# End Source File
# Begin Source File

SOURCE=.\cObstacle.h
# End Source File
# Begin Source File

SOURCE=.\cPicture.cpp
# End Source File
# Begin Source File

SOURCE=.\cPicture.h
# End Source File
# Begin Source File

SOURCE=.\cPlane.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlane.h
# End Source File
# Begin Source File

SOURCE=.\cPlaneHor.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlaneHor.h
# End Source File
# Begin Source File

SOURCE=.\cPlaneVer.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlaneVer.h
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerDynamic.h
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerMesh.h
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerNormal.cpp
# End Source File
# Begin Source File

SOURCE=.\cPlaneVerNormal.h
# End Source File
# Begin Source File

SOURCE=.\cScreenObj.cpp
# End Source File
# Begin Source File

SOURCE=.\cScreenObj.h
# End Source File
# Begin Source File

SOURCE=.\cSelect.cpp
# End Source File
# Begin Source File

SOURCE=.\cSelect.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iobj.h
# End Source File
# End Group
# Begin Group "Texture"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cTexture.cpp
# End Source File
# Begin Source File

SOURCE=.\cTexture.h
# End Source File
# Begin Source File

SOURCE=.\cTextureCache.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureCache.h
# End Source File
# Begin Source File

SOURCE=.\cTextureMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureMemory.h
# End Source File
# Begin Source File

SOURCE=.\cTextureSpr.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureSpr.h
# End Source File
# Begin Source File

SOURCE=.\cTextureVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureVideo.h
# End Source File
# Begin Source File

SOURCE=.\cTextureVideoOther.cpp
# End Source File
# Begin Source File

SOURCE=.\cTextureVideoOther.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iTexture.h
# End Source File
# End Group
# Begin Group "Graphics 3D"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cCamera3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cCamera3D.h
# End Source File
# Begin Source File

SOURCE=.\cGraphics3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cGraphics3D.h
# End Source File
# Begin Source File

SOURCE=.\cGround3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cGround3D.h
# End Source File
# Begin Source File

SOURCE=.\cScreen3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cScreen3D.h
# End Source File
# End Group
# Begin Group "BTree"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cBNode.cpp
# End Source File
# Begin Source File

SOURCE=.\cBNode.h
# End Source File
# Begin Source File

SOURCE=.\cBNodeRoot.cpp
# End Source File
# Begin Source File

SOURCE=.\cBNodeRoot.h
# End Source File
# Begin Source File

SOURCE=.\cBTree.cpp
# End Source File
# Begin Source File

SOURCE=.\cBTree.h
# End Source File
# End Group
# Begin Group "Simple Class"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\cMesh.h
# End Source File
# Begin Source File

SOURCE=.\cMesh3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cMesh3D.h
# End Source File
# Begin Source File

SOURCE=.\cThread.cpp
# End Source File
# Begin Source File

SOURCE=.\cThread.h
# End Source File
# Begin Source File

SOURCE=.\cUsed.cpp
# End Source File
# Begin Source File

SOURCE=.\cUsed.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iColor.h
# End Source File
# Begin Source File

SOURCE=.\stLine.h
# End Source File
# End Group
# Begin Group "Utility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\coordinate.cpp
# End Source File
# Begin Source File

SOURCE=.\coordinate.h
# End Source File
# Begin Source File

SOURCE=.\mapconvert.cpp
# End Source File
# Begin Source File

SOURCE=.\position.cpp
# End Source File
# Begin Source File

SOURCE=.\position.h
# End Source File
# Begin Source File

SOURCE=.\template.h
# End Source File
# End Group
# Begin Group "Special"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cSpecial.cpp
# End Source File
# Begin Source File

SOURCE=.\cSpecial.h
# End Source File
# Begin Source File

SOURCE=.\cSpecialFirefly.cpp
# End Source File
# Begin Source File

SOURCE=.\cSpecialFirefly.h
# End Source File
# Begin Source File

SOURCE=.\cSpecialScroll.cpp
# End Source File
# Begin Source File

SOURCE=.\cSpecialScroll.h
# End Source File
# Begin Source File

SOURCE=.\cSpecialSign.cpp
# End Source File
# Begin Source File

SOURCE=.\cSpecialSign.h
# End Source File
# Begin Source File

SOURCE=.\cSpecialSnow.cpp
# End Source File
# Begin Source File

SOURCE=.\cSpecialSnow.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
