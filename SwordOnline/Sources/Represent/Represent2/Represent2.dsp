# Microsoft Developer Studio Project File - Name="Represent2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Represent2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Represent2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Represent2.mak" CFG="Represent2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Represent2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Represent2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Represent/Represent2", AYGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Represent2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT2_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\engine\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT2_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib GdiPlus.lib d3dx9dt.lib d3d9.lib /nologo /dll /map /debug /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\Represent2.dll ..\..\..\..\bin\client\Represent2.dll	copy Release\Represent2.pdb ..\..\..\..\bin\client\Represent2.pdb	copy Release\Represent2.map ..\..\..\..\bin\client\Represent2.map
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Represent2 - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\engine\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT2_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib GdiPlus.lib /nologo /dll /incremental:no /map /debug /machine:I386 /implib:"../../../lib/Represent.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\client\debug\	copy Debug\Represent2.dll ..\..\..\..\bin\client\Represent2.dll	copy Debug\Represent2.dll ..\..\..\..\bin\client\debug\Represent2.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Represent2 - Win32 Release"
# Name "Represent2 - Win32 Debug"
# Begin Group "图资源管理"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\KImageStore2.cpp
# End Source File
# Begin Source File

SOURCE=.\KImageStore2.h
# End Source File
# Begin Source File

SOURCE=..\iRepresent\RepresentUtility.cpp
# End Source File
# End Group
# Begin Group "对外接口实现"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\iRepresent\iRepresentShell.h
# End Source File
# Begin Source File

SOURCE=.\KRepresentShell2.cpp
# End Source File
# Begin Source File

SOURCE=.\KRepresentShell2.h
# End Source File
# End Group
# Begin Group "Font"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\iRepresent\Font\KFont2.cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KFontData.cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Text\TextProcess.cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Text\TextProcess.h
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\UFont..cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\UFont.h
# End Source File
# End Group
# Begin Group "图形操作"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\iRepresent\Image\ImageOperation.cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Image\ImageOperation.h
# End Source File
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Lib\debug\engine.lib

!IF  "$(CFG)" == "Represent2 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Represent2 - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Lib\release\engine.lib

!IF  "$(CFG)" == "Represent2 - Win32 Release"

!ELSEIF  "$(CFG)" == "Represent2 - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\iRepresent\RepresentUtility.h
# End Source File
# End Target
# End Project
