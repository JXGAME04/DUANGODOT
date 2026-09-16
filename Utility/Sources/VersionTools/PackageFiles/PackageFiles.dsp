# Microsoft Developer Studio Project File - Name="PackageFiles" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PackageFiles - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PackageFiles.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PackageFiles.mak" CFG="PackageFiles - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PackageFiles - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PackageFiles - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/PackageFiles", PVSAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PackageFiles - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PACKAGEFILES_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PACKAGEFILES_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "PackageFiles - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PACKAGEFILES_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PACKAGEFILES_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PackageFiles - Win32 Release"
# Name "PackageFiles - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "ucl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\ucl-1.01\src\alloc.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\io.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2b_99.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2b_d.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2b_ds.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2b_to.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2d_99.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2d_d.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2d_ds.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2d_to.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2e_99.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2e_d.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2e_ds.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\n2e_to.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_crc.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_dll.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_init.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_ptr.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_str.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_util.c"
# End Source File
# End Group
# Begin Source File

SOURCE=.\PackageFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\PackageFiles.def
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ZPackFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ZSPRPackFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ZPackFile.h
# End Source File
# Begin Source File

SOURCE=.\ZSPRPackFile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\Engine.lib
# End Source File
# End Target
# End Project
