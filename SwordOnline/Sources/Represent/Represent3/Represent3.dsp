# Microsoft Developer Studio Project File - Name="Represent3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Represent3 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Represent3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Represent3.mak" CFG="Represent3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Represent3 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Represent3 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Represent/Represent3", XVJAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Represent3 - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT3_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\engine\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT3_EXPORTS" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib d3dx9dt.lib d3d9.lib winmm.lib dxguid.lib ddraw.lib GdiPlus.lib /nologo /dll /map /debug /machine:I386 /libpath:"..\..\..\Lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\client\release\	copy Release\Represent3.dll ..\..\..\..\bin\client\Represent3.dll	copy Release\Represent3.pdb ..\..\..\..\bin\client\Represent3.pdb	copy Release\Represent3.map ..\..\..\..\bin\client\Represent3.map
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT3_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\engine\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REPRESENT3_EXPORTS" /FR /Yu"precompile.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib d3dx9dt.lib d3d9.lib winmm.lib dxguid.lib ddraw.lib GdiPlus.lib /nologo /dll /incremental:no /map /debug /machine:I386 /libpath:"..\..\..\Lib"
# SUBTRACT LINK32 /profile
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\client\debug\	copy Debug\Represent3.dll ..\..\..\..\bin\client\Represent3.dll	copy Debug\Represent3.pdb ..\..\..\..\bin\client\Represent3.pdb	copy Debug\Represent3.dll ..\..\..\..\bin\client\debug\Represent3.dll	copy Debug\Represent3.pdb ..\..\..\..\bin\client\debug\Represent3.pdb
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Represent3 - Win32 Release"
# Name "Represent3 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\D3D_Device.cpp
# End Source File
# Begin Source File

SOURCE=.\D3D_Shell.cpp

!IF  "$(CFG)" == "Represent3 - Win32 Release"

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3D_Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KCharSet.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KFont3.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KFontData.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KFontRes.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KMRU.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\KRepresentShell3.cpp

!IF  "$(CFG)" == "Represent3 - Win32 Release"

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\precompile.cpp
# ADD CPP /Yc"precompile.h"
# End Source File
# Begin Source File

SOURCE=..\iRepresent\RepresentUtility.cpp

!IF  "$(CFG)" == "Represent3 - Win32 Release"

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\iRepresent\Text\TextProcess.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\TextureRes.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureResMgr.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BaseInclude.h
# End Source File
# Begin Source File

SOURCE=.\D3D_Device.h
# End Source File
# Begin Source File

SOURCE=.\D3D_Shell.h
# End Source File
# Begin Source File

SOURCE=.\D3D_Utils.h
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Font\KFont3.h
# End Source File
# Begin Source File

SOURCE=.\KRepresentShell3.h
# End Source File
# Begin Source File

SOURCE=.\precompile.h
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Text\TextProcess.h
# End Source File
# Begin Source File

SOURCE=..\iRepresent\Text\TextProcessDef.h
# End Source File
# Begin Source File

SOURCE=.\TextureRes.h
# End Source File
# Begin Source File

SOURCE=.\TextureResMgr.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Group "debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Lib\debug\engine.lib

!IF  "$(CFG)" == "Represent3 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Lib\release\engine.lib

!IF  "$(CFG)" == "Represent3 - Win32 Release"

!ELSEIF  "$(CFG)" == "Represent3 - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# End Target
# End Project
