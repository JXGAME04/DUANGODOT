# Microsoft Developer Studio Project File - Name="Autoupdate" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Autoupdate - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Autoupdate.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Autoupdate.mak" CFG="Autoupdate - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Autoupdate - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Autoupdate - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Utility/Sources/Autoupdate", BPIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Autoupdate - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /out:"Release/Jxonline.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy release\JXOnline.exe ..\..\..\bin\Client\release\JXOnline.exe	copy release\JXOnline.exe ..\..\..\bin\Client\JXOnline.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Autoupdate - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/Jxonline.exe" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy debug\JXOnline.exe ..\..\..\bin\Client\debug\JXOnline.exe	copy debug\JXOnline.exe ..\..\..\bin\Client\JXOnline.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Autoupdate - Win32 Release"
# Name "Autoupdate - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AnnuncePanel.cpp
# End Source File
# Begin Source File

SOURCE=.\Autoupdate.cpp
# End Source File
# Begin Source File

SOURCE=.\Autoupdate.rc
# End Source File
# Begin Source File

SOURCE=.\AutoupdateDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DefaultProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\GameOptionPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ItermProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\ItermProcessTypeCheck.cpp
# End Source File
# Begin Source File

SOURCE=.\lientGameOptionProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\RecordProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\StartDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\webbrowser2.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AnnuncePanel.h
# End Source File
# Begin Source File

SOURCE=.\Autoupdate.h
# End Source File
# Begin Source File

SOURCE=.\AutoupdateDlg.h
# End Source File
# Begin Source File

SOURCE=.\AutoUpdateRes.h
# End Source File
# Begin Source File

SOURCE=.\DefaultProcess.h
# End Source File
# Begin Source File

SOURCE=.\GameOptionPanel.h
# End Source File
# Begin Source File

SOURCE=.\ItermProcess.h
# End Source File
# Begin Source File

SOURCE=.\ItermProcessTypeCheck.h
# End Source File
# Begin Source File

SOURCE=.\lientGameOptionProcess.h
# End Source File
# Begin Source File

SOURCE=.\RecordProcess.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StartDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\webbrowser2.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\icon.ico
# End Source File
# Begin Source File

SOURCE=".\res\新的升级程序_提示下载_底图.bmp"
# End Source File
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Group "Release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\release\engine.lib
# End Source File
# End Group
# Begin Group "debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\debug\engine.lib
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
# Section Autoupdate : {D30C1661-CDAF-11D0-8A3E-00C04FC9E26E}
# 	2:5:Class:CWebBrowser2
# 	2:10:HeaderFile:webbrowser2.h
# 	2:8:ImplFile:webbrowser2.cpp
# End Section
# Section Autoupdate : {8856F961-340A-11D0-A96B-00C04FD705A2}
# 	2:21:DefaultSinkHeaderFile:webbrowser2.h
# 	2:16:DefaultSinkClass:CWebBrowser2
# End Section
