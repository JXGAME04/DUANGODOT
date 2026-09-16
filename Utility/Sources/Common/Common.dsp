# Microsoft Developer Studio Project File - Name="Common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak" CFG="Common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Common - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Common - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Common", FQCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Common - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 zip.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../../run_release/common.dll" /implib:"../../lib/common.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../lib/zip.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../run/common.dll" /implib:"../../lib/common.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Common - Win32 Release"
# Name "Common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cClipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\cFps.cpp
# End Source File
# Begin Source File

SOURCE=.\cItem.cpp
# End Source File
# Begin Source File

SOURCE=.\cItemCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.rc
# End Source File
# Begin Source File

SOURCE=.\cPack1D.cpp
# End Source File
# Begin Source File

SOURCE=.\cPack1DAnimation.cpp
# End Source File
# Begin Source File

SOURCE=.\cPack1DNpc.cpp
# End Source File
# Begin Source File

SOURCE=.\cPack1DObj.cpp
# End Source File
# Begin Source File

SOURCE=.\cPack1DPlane.cpp
# End Source File
# Begin Source File

SOURCE=.\cPack2D.cpp
# End Source File
# Begin Source File

SOURCE=.\fileop.cpp
# End Source File
# Begin Source File

SOURCE=.\planedoodad.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\stringvalue.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Headers\cClipboard.h
# End Source File
# Begin Source File

SOURCE=.\cFps.h
# End Source File
# Begin Source File

SOURCE=.\cItem.h
# End Source File
# Begin Source File

SOURCE=.\cItemCache.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\cLock.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\clog.h
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\cPack1D.h
# End Source File
# Begin Source File

SOURCE=.\cPack1DAnimation.h
# End Source File
# Begin Source File

SOURCE=.\cPack1DNpc.h
# End Source File
# Begin Source File

SOURCE=.\cPack1DObj.h
# End Source File
# Begin Source File

SOURCE=.\cPack1DPlane.h
# End Source File
# Begin Source File

SOURCE=.\cPack2D.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\cSafeQueue.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\cThread.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\cTimer.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\fileop.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iColor.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\ifps.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iitem.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iPack1D.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iPack2D.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iTemplate.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\planedoodad.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\stringvalue.h
# End Source File
# End Group
# End Target
# End Project
