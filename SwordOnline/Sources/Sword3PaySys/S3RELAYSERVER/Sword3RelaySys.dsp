# Microsoft Developer Studio Project File - Name="Sword3RelaySys" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=SWORD3RELAYSYS - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Sword3RelaySys.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Sword3RelaySys.mak" CFG="SWORD3RELAYSYS - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sword3RelaySys - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Sword3RelaySys - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Sword3PaySys/S3AccServer", FQKAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
F90=df.exe
RSC=rc.exe

!IF  "$(CFG)" == "Sword3RelaySys - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /I "..\regexp" /I "..\mysql++-1.7.1-1-win32-vc++\include" /I "..\mysql\include" /I "..\\" /I "d:\dx81sdk\include" /I "..\Toolkits\mysql++-1.7.1-1-win32-vc++\include" /I "..\Toolkits\mysql\include" /I "..\..\..\Headers" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 Ws2_32.lib Winmm.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"LIBCD.lib" /out:"../../../../bin/client/Sword3RelaySys.exe"

!ELSEIF  "$(CFG)" == "Sword3RelaySys - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Toolkits\mysql++-1.7.1-1-win32-vc++\include" /I "..\Toolkits\mysql\include" /I "..\..\..\Headers" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 S3DBInterface.lib KLVideo.lib LuaLib.lib FilterText_StaticLib.lib Engine.lib Rainbow.lib IOCPServer.lib ESClient.lib Heaven.lib Ws2_32.lib Winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"LIBCD.lib" /out:"../../../../bin/client/Sword3RelaySys.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Sword3RelaySys - Win32 Release"
# Name "Sword3RelaySys - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\GlobalFun.cpp
# End Source File
# Begin Source File

SOURCE=.\KThread.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\S3P_MSSQLServer_Result.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDB_MSSQLServer_Connection.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConVBC.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketPool.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PResultVBC.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\GlobalDTD.h
# End Source File
# Begin Source File

SOURCE=.\GlobalFun.h
# End Source File
# Begin Source File

SOURCE=.\KStdAfx.h
# End Source File
# Begin Source File

SOURCE=.\KThread.h
# End Source File
# Begin Source File

SOURCE=.\S3P_MSSQLServer_Result.h
# End Source File
# Begin Source File

SOURCE=.\S3PAccount.h
# End Source File
# Begin Source File

SOURCE=.\S3PDB_MSSQLServer_Connection.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConVBC.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketPool.h
# End Source File
# Begin Source File

SOURCE=.\S3PResultVBC.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
