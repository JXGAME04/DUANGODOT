# Microsoft Developer Studio Project File - Name="ResourceManager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ResourceManager - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ResourceManager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ResourceManager.mak" CFG="ResourceManager - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ResourceManager - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ResourceManager - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/Sources/ResourceManager", BDEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ResourceManager - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /I "..\..\..\SwordOnline\sources\core" /I "..\..\..\SwordOnline\Headers" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 engine.lib ..\..\lib\image.lib ..\..\lib\common.lib coretool.lib /nologo /subsystem:windows /machine:I386 /out:"../../run_release/ResourceManager.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ResourceManager - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /I "..\..\..\SwordOnline\sources\core" /I "..\..\..\SwordOnline\Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 engine.lib ..\..\lib\image.lib ..\..\lib\common.lib coretool.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\..\run\ResourceManager.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ResourceManager - Win32 Release"
# Name "ResourceManager - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DivideDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GetSprFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ResourceManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ResourceManager.rc
# End Source File
# Begin Source File

SOURCE=.\ResourceManagerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DivideDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ResourceManager.h
# End Source File
# Begin Source File

SOURCE=.\ResourceManagerDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00005.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00007.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00008.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_CAMERA.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_COMMAND.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_DOODAD.ico
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_DOODADCOMPLEX.ico
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_GROUP.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_LIGHT.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_NPC.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_OBJECT.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_OBSTACLE.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_SETTING.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_TERRA.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDI_ITEM_TRAP.ICO
# End Source File
# Begin Source File

SOURCE=.\res\item_att.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_com.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_doo.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_fol.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_lig.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_obj.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_set.ico
# End Source File
# Begin Source File

SOURCE=.\res\item_spe.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_a.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_e.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_o.ico
# End Source File
# Begin Source File

SOURCE=.\res\ResourceManager.ico
# End Source File
# Begin Source File

SOURCE=.\res\ResourceManager.rc2
# End Source File
# Begin Source File

SOURCE=.\res\Worldeditor.ICO
# End Source File
# End Group
# Begin Group "Control"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Controls\DirTree.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\DirTree.h
# End Source File
# Begin Source File

SOURCE=..\Controls\FileFindEx.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\FileFindEx.h
# End Source File
# Begin Source File

SOURCE=..\Controls\SprStatic.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\SprStatic.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\GetSprFile.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
