# Microsoft Developer Studio Project File - Name="Status" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Status - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Status.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Status.mak" CFG="Status - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Status - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Status - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/Status", FKEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Status - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Shlwapi.lib common.lib engine.lib image.lib command.lib graphicsutility.lib automap.lib /nologo /subsystem:windows /dll /machine:I386 /def:".\Status.def" /out:"../../run_release/Status.dll" /implib:"../../lib/Status.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Status - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /I "..\Engine\Src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 automap.lib Shlwapi.lib ../../lib/common.lib engine.lib ../../lib/image.lib ../../lib/command.lib ../../lib/graphicsutility.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\Status.def" /out:"../../run/Status.dll" /implib:"../../lib/Status.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "Status - Win32 Release"
# Name "Status - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\activeiniform.cpp
# End Source File
# Begin Source File

SOURCE=.\cActor.cpp
# End Source File
# Begin Source File

SOURCE=.\cDoodad.cpp
# End Source File
# Begin Source File

SOURCE=.\cMissle.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusDoodadDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusDoodadSimple.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusGeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusGroupDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusLightStatic.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusMachine.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusObstacle.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusSpecial.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainDiamond.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainRoad.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainSquare.cpp
# End Source File
# Begin Source File

SOURCE=.\cStatusTest.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\FileFindEx.cpp
# End Source File
# Begin Source File

SOURCE=.\NpcSettingDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\objUtility.cpp
# End Source File
# Begin Source File

SOURCE=.\ObstacleDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SpecialDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Status.cpp
# End Source File
# Begin Source File

SOURCE=.\Status.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Status.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StringDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\activeiniform.h
# End Source File
# Begin Source File

SOURCE=.\cActor.h
# End Source File
# Begin Source File

SOURCE=.\cDoodad.h
# End Source File
# Begin Source File

SOURCE=.\cMissle.h
# End Source File
# Begin Source File

SOURCE=.\cStatus.h
# End Source File
# Begin Source File

SOURCE=.\cStatusCamera.h
# End Source File
# Begin Source File

SOURCE=.\cStatusDoodadDynamic.h
# End Source File
# Begin Source File

SOURCE=.\cStatusDoodadSimple.h
# End Source File
# Begin Source File

SOURCE=.\cStatusDynamic.h
# End Source File
# Begin Source File

SOURCE=.\cStatusGame.h
# End Source File
# Begin Source File

SOURCE=.\cStatusGeneral.h
# End Source File
# Begin Source File

SOURCE=.\cStatusGroup.h
# End Source File
# Begin Source File

SOURCE=.\cStatusGroupDynamic.h
# End Source File
# Begin Source File

SOURCE=.\cStatusLightStatic.h
# End Source File
# Begin Source File

SOURCE=.\cStatusMachine.h
# End Source File
# Begin Source File

SOURCE=.\cStatusObstacle.h
# End Source File
# Begin Source File

SOURCE=.\cStatusRegion.h
# End Source File
# Begin Source File

SOURCE=.\cStatusSpecial.h
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainDiamond.h
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainRoad.h
# End Source File
# Begin Source File

SOURCE=.\cStatusTerrainSquare.h
# End Source File
# Begin Source File

SOURCE=.\cStatusTest.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iStatus.h
# End Source File
# Begin Source File

SOURCE=.\NpcSettingDlg.h
# End Source File
# Begin Source File

SOURCE=.\ObjDlg.h
# End Source File
# Begin Source File

SOURCE=.\objUtility.h
# End Source File
# Begin Source File

SOURCE=.\ObstacleDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SelectDlg.h
# End Source File
# Begin Source File

SOURCE=.\SpecialDlg.h
# End Source File
# Begin Source File

SOURCE=.\Status.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StringDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Status.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
