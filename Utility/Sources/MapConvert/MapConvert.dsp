# Microsoft Developer Studio Project File - Name="MapConvert" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=MapConvert - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MapConvert.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MapConvert.mak" CFG="MapConvert - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MapConvert - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MapConvert - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/Sources/MapConvert", QPGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MapConvert - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ../../lib/image.lib ../../lib/common.lib ../../lib/graphics.lib shlwapi.lib ../../../SwordOnline/lib/engine.lib zip.lib /nologo /subsystem:windows /machine:I386 /out:"../../run_release/MapConvert.exe"

!ELSEIF  "$(CFG)" == "MapConvert - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../lib/image.lib ../../lib/common.lib shlwapi.lib ../../../SwordOnline/lib/engine.lib zip.lib graphics.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../Run/MapConvert.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "MapConvert - Win32 Release"
# Name "MapConvert - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Check.cpp
# End Source File
# Begin Source File

SOURCE=.\Clear.cpp
# End Source File
# Begin Source File

SOURCE=.\Convert.cpp
# End Source File
# Begin Source File

SOURCE=.\ConvertGame.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\FileFindEx.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\FileFindEx.h
# End Source File
# Begin Source File

SOURCE=.\foreach.cpp
# End Source File
# Begin Source File

SOURCE=.\Join.cpp
# End Source File
# Begin Source File

SOURCE=.\KSpbioTemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\MapConvert.cpp
# End Source File
# Begin Source File

SOURCE=.\MapConvert.rc
# End Source File
# Begin Source File

SOURCE=.\MapConvertDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NoUse.cpp
# End Source File
# Begin Source File

SOURCE=.\Offset.cpp
# End Source File
# Begin Source File

SOURCE=.\Pack.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Template.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\foreach.h
# End Source File
# Begin Source File

SOURCE=.\KSpbioTemplate.h
# End Source File
# Begin Source File

SOURCE=.\MapConvert.h
# End Source File
# Begin Source File

SOURCE=.\MapConvertDlg.h
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

SOURCE=.\res\MapConvert.ico
# End Source File
# Begin Source File

SOURCE=.\res\MapConvert.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
