# Microsoft Developer Studio Project File - Name="PackManualUpdate" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=PackManualUpdate - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PackManualUpdate.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PackManualUpdate.mak" CFG="PackManualUpdate - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PackManualUpdate - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "PackManualUpdate - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/PackCombo/PackManualUpdate", BAVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PackManualUpdate - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "PackManualUpdate - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PackManualUpdate - Win32 Release"
# Name "PackManualUpdate - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\PackManualUpdate.cpp
# End Source File
# Begin Source File

SOURCE=.\ZPackFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ZPackFile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "ucl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\ucl-1.01\src\alloc.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\fake16.h"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\getbit.h"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\internal.h"
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

SOURCE=".\ucl-1.01\src\ucl_conf.h"
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

SOURCE=".\ucl-1.01\src\ucl_ptr.h"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_str.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_util.c"
# End Source File
# Begin Source File

SOURCE=".\ucl-1.01\src\ucl_util.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
