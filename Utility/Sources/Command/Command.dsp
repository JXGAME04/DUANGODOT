# Microsoft Developer Studio Project File - Name="Command" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Command - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Command.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Command.mak" CFG="Command - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Command - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Command - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/Command", JHEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Command - Win32 Release"

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
# ADD LINK32 image.lib /nologo /subsystem:windows /dll /machine:I386 /def:".\Command.def" /out:"../../run_release/Command.dll" /implib:"../../lib/Command.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Command - Win32 Debug"

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
# ADD LINK32 ..\..\lib\automap.lib ..\..\lib\image.lib ..\..\lib\graphics.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\Command.def" /out:"..\..\Run\Command.dll" /implib:"../../lib/Command.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Command - Win32 Release"
# Name "Command - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cCmdCameraOffsetTo.cpp
# End Source File
# Begin Source File

SOURCE=.\cCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\cCommandMachine.cpp
# End Source File
# Begin Source File

SOURCE=.\cMacroCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\Command.cpp
# End Source File
# Begin Source File

SOURCE=.\Command.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Command.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cCmdCameraOffsetTo.h
# End Source File
# Begin Source File

SOURCE=.\cCommand.h
# End Source File
# Begin Source File

SOURCE=.\cCommandMachine.h
# End Source File
# Begin Source File

SOURCE=.\cMacroCommand.h
# End Source File
# Begin Source File

SOURCE=.\Command.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\iCommand.h
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

SOURCE=.\res\Command.rc2
# End Source File
# End Group
# Begin Group "Camera"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cCmdCameraOffset.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdCameraOffset.h
# End Source File
# Begin Source File

SOURCE=.\cCmdCameraScale.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdCameraScale.h
# End Source File
# Begin Source File

SOURCE=.\cCmdCameraSet.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdCameraSet.h
# End Source File
# End Group
# Begin Group "Ground"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cCmdGroundChangeBrightness.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundChangeBrightness.h
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundChangeTile.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundChangeTile.h
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundFillRect.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundFillRect.h
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundToggleGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundToggleGrid.h
# End Source File
# End Group
# Begin Group "Document"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cCmdGroundDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdGroundDynamic.h
# End Source File
# Begin Source File

SOURCE=.\cCmdNewDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdNewDocument.h
# End Source File
# Begin Source File

SOURCE=.\cCmdOpenDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdOpenDocument.h
# End Source File
# Begin Source File

SOURCE=.\cCmdSaveDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdSaveDocument.h
# End Source File
# End Group
# Begin Group "Obj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cCmdObjAdd.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdObjAdd.h
# End Source File
# Begin Source File

SOURCE=.\cCmdObjRemove.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdObjRemove.h
# End Source File
# Begin Source File

SOURCE=.\cCmdObjSetParam.cpp
# End Source File
# Begin Source File

SOURCE=.\cCmdObjSetParam.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
