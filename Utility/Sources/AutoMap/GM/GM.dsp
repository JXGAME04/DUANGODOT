# Microsoft Developer Studio Project File - Name="GM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=GM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GM.mak" CFG="GM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GM - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "GM - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/GM", XOSAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GM - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../../swordonline/engine/src" /I "../../../swordonline/sources/engine/src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"../../../bin/client/GM.exe" /libpath:"../../../swordonline/lib"

!ELSEIF  "$(CFG)" == "GM - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../SwordOnline/sources/engine/src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../../../swordonline/sources/engine/src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Ws2_32.lib engine.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../../bin/client/GM.exe" /pdbtype:sept /libpath:"../../../swordonline/lib/debug"

!ENDIF 

# Begin Target

# Name "GM - Win32 Release"
# Name "GM - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "GMAPI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\APIChild.h
# End Source File
# Begin Source File

SOURCE=.\APIGameWorld.cpp
# End Source File
# Begin Source File

SOURCE=.\APIGameWorld.h
# End Source File
# Begin Source File

SOURCE=.\APIPlayer.cpp
# End Source File
# Begin Source File

SOURCE=.\APIPlayer.h
# End Source File
# Begin Source File

SOURCE=.\APIRole.cpp
# End Source File
# Begin Source File

SOURCE=.\APIRole.h
# End Source File
# End Group
# Begin Group "AccessDataBase"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\KThread.cpp
# End Source File
# Begin Source File

SOURCE=.\KThread.h
# End Source File
# Begin Source File

SOURCE=.\S3AccessResult.cpp
# End Source File
# Begin Source File

SOURCE=.\S3AccessResult.h
# End Source File
# Begin Source File

SOURCE=.\S3PDB_MSAccess_Connection.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDB_MSAccess_Connection.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConVBC.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConVBC.h
# End Source File
# Begin Source File

SOURCE=.\S3PResultVBC.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PResultVBC.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\EventHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\GameMaster.idl
# ADD MTL /tlb "../../../bin/client/GM.tlb" /h "GameMaster_i.h" /iid "GameMaster_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\GM.cpp
# End Source File
# Begin Source File

SOURCE=.\GM.rc
# End Source File
# Begin Source File

SOURCE=.\GMApp.cpp
# End Source File
# Begin Source File

SOURCE=.\GMCore.cpp
# End Source File
# Begin Source File

SOURCE=.\GMDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CEventProxy.h
# End Source File
# Begin Source File

SOURCE=.\EventHandler.h
# End Source File
# Begin Source File

SOURCE=.\FireEvents.h
# End Source File
# Begin Source File

SOURCE=.\GM.h
# End Source File
# Begin Source File

SOURCE=.\GMConsts.h
# End Source File
# Begin Source File

SOURCE=.\GMCore.h
# End Source File
# Begin Source File

SOURCE=.\GMDlg.h
# End Source File
# Begin Source File

SOURCE=.\GMMessages.h
# End Source File
# Begin Source File

SOURCE=.\HelpContext.h
# End Source File
# Begin Source File

SOURCE=.\msado15.tlh
# End Source File
# Begin Source File

SOURCE=.\msado15.tli
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

SOURCE=.\GameMaster.rgs
# End Source File
# Begin Source File

SOURCE=.\res\GM.ico
# End Source File
# Begin Source File

SOURCE=.\res\GM.rc2
# End Source File
# Begin Source File

SOURCE=.\GM.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
