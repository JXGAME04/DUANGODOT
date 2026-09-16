# Microsoft Developer Studio Project File - Name="WorldEditor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WorldEditor - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WorldEditor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WorldEditor.mak" CFG="WorldEditor - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WorldEditor - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WorldEditor - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/WorldEditor", BSDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WorldEditor - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 image.lib status.lib common.lib /nologo /subsystem:windows /machine:I386 /out:"../../run_release/WorldEditor.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "WorldEditor - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\Headers" /I "..\..\Headers" /I "..\..\..\Headers" /I "..\..\..\..\Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../lib/image.lib ../../lib/status.lib common.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\..\run\WorldEditor.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "WorldEditor - Win32 Release"
# Name "WorldEditor - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\GraphicsWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\GroundFillRectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\WorldEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldEditor.rc
# End Source File
# Begin Source File

SOURCE=.\WorldEditorDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldEditorView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\GraphicsWnd.h
# End Source File
# Begin Source File

SOURCE=.\GroundFillRectDlg.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WorldEditorDoc.h
# End Source File
# Begin Source File

SOURCE=.\WorldEditorView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\0_0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\155.bmp
# End Source File
# Begin Source File

SOURCE=.\res\b.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\c.bmp
# End Source File
# Begin Source File

SOURCE=.\res\chat.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CURRENTFOLDER.bmp
# End Source File
# Begin Source File

SOURCE=.\res\d.bmp
# End Source File
# Begin Source File

SOURCE=.\res\error.bmp
# End Source File
# Begin Source File

SOURCE=.\res\forbid.bmp
# End Source File
# Begin Source File

SOURCE=.\res\help.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hint.bmp
# End Source File
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

SOURCE=.\res\icon_roa.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_roo.ico
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

SOURCE=.\res\item_spe.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEW2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\next.bmp
# End Source File
# Begin Source File

SOURCE=.\res\OPENFOLDER.bmp
# End Source File
# Begin Source File

SOURCE=.\res\output_a.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_c.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_e.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_h.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_n.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_o.ico
# End Source File
# Begin Source File

SOURCE=.\res\succeed.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tte.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vc.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc1.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc2.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc3.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc4.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc5.ico
# End Source File
# Begin Source File

SOURCE=.\res\vc6.ico
# End Source File
# Begin Source File

SOURCE=.\res\warnning.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WorldEditor.ico
# End Source File
# Begin Source File

SOURCE=.\res\WorldEditor.rc2
# End Source File
# Begin Source File

SOURCE=.\res\WorldEditorDoc.ico
# End Source File
# Begin Source File

SOURCE=".\res\房子.bmp"
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

SOURCE=.\Controls\outputlist.cpp
# End Source File
# Begin Source File

SOURCE=.\Controls\outputlist.h
# End Source File
# Begin Source File

SOURCE=..\Controls\SprStatic.cpp
# End Source File
# Begin Source File

SOURCE=..\Controls\SprStatic.h
# End Source File
# End Group
# Begin Group "Command"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\WorldEditor.h
# End Source File
# Begin Source File

SOURCE=.\WorldEditor.reg
# End Source File
# Begin Source File

SOURCE="..\..\Documents\目录结构.txt"
# End Source File
# End Target
# End Project
