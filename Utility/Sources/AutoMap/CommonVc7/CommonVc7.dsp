# Microsoft Developer Studio Project File - Name="CommonVc7" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CommonVc7 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CommonVc7.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CommonVc7.mak" CFG="CommonVc7 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CommonVc7 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CommonVc7 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/CommonVc7", OEKAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CommonVc7 - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 zip.lib /nologo /subsystem:windows /dll /machine:I386 /def:".\CommonVc7.def" /out:"../../run_vc7/CommonVc7.dll" /implib:"../../lib/CommonVc7.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "CommonVc7 - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zip.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../run_vc7/CommonVc7.dll" /implib:"../../lib/CommonVc7.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "CommonVc7 - Win32 Release"
# Name "CommonVc7 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Common\cClipboard.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cFps.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cItem.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cItemCache.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\Common.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\CommonVc7.cpp
# End Source File
# Begin Source File

SOURCE=.\CommonVc7.def

!IF  "$(CFG)" == "CommonVc7 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "CommonVc7 - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CommonVc7.rc
# End Source File
# Begin Source File

SOURCE=..\Common\cPack1D.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cPack1DAnimation.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cPack1DNpc.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cPack1DObj.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cPack1DPlane.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\cPack2D.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\fileop.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\planedoodad.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\Common\stringvalue.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Common\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\Common\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\CommonVc7.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
