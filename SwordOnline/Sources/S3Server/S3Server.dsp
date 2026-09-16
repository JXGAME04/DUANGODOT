# Microsoft Developer Studio Project File - Name="S3Server" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=S3Server - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "S3Server.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "S3Server.mak" CFG="S3Server - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "S3Server - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "S3Server - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/S3Server", RRDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "S3Server - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../Engine/src" /I "../Core/src" /I "../Engine/include" /I "../NetWork" /I "..\..\Headers" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_SERVER" /Fr /Yu"KEngine.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib engine.lib coreserver.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"../../lib/"
# SUBTRACT LINK32 /pdb:none /force
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\S3Server.exe ..\..\..\bin\Server\S3Server.exe	copy Release\S3Server.map ..\..\..\bin\Server\S3Server.map	copy Release\S3Server.pdb ..\..\..\bin\Server\S3Server.pdb
# End Special Build Tool

!ELSEIF  "$(CFG)" == "S3Server - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../Engine/src" /I "../Core/src" /I "../Engine/include" /I "../NetWork" /I "..\..\Headers" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_SERVER" /FR /Yu"KEngine.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 engine.lib coreserver.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../lib/"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\S3Server.pdb ..\..\..\bin\Server\S3Server.pdb	copy Debug\S3Server.exe ..\..\..\bin\Server\S3Server.exe	copy Release\S3Server.map ..\..\..\bin\Server\S3Server.map
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "S3Server - Win32 Release"
# Name "S3Server - Win32 Debug"
# Begin Group "程序入口"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Sword3PaySys\XSHANJU\Sword3PaySys\S3DBInterface.h
# End Source File
# Begin Source File

SOURCE=.\S3Server.cpp
# End Source File
# End Group
# Begin Group "控制窗口"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BroadCastWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\BroadCastWnd.h
# End Source File
# Begin Source File

SOURCE=.\ClientConnectListWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientConnectListWnd.h
# End Source File
# Begin Source File

SOURCE=.\ServerLocalControlWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerLocalControlWnd.h
# End Source File
# End Group
# Begin Group "控制服务"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SwordOnLineServer.cpp
# End Source File
# Begin Source File

SOURCE=.\SwordOnLineServer.h
# End Source File
# End Group
# Begin Group "资源"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\S3Server.rc
# End Source File
# Begin Source File

SOURCE=.\SwordOnLine.bmp
# End Source File
# Begin Source File

SOURCE=.\SwordOnLine.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"KEngine.h"
# End Source File
# Begin Source File

SOURCE=..\..\Lib\S3DBInterface_d.lib
# End Source File
# End Target
# End Project
