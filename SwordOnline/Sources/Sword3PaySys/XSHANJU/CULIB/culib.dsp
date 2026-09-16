# Microsoft Developer Studio Project File - Name="culib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=culib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "culib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "culib.mak" CFG="culib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "culib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "culib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/西山居计费系统/源程序/culib", RWEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "culib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "culib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\culib.lib"

!ENDIF 

# Begin Target

# Name "culib - Win32 Release"
# Name "culib - Win32 Debug"
# Begin Source File

SOURCE=.\CppUnitException.h
# End Source File
# Begin Source File

SOURCE=.\estring.h
# End Source File
# Begin Source File

SOURCE=.\Guards.h
# End Source File
# Begin Source File

SOURCE=.\Orthodox.h
# End Source File
# Begin Source File

SOURCE=.\RepeatedTest.h
# End Source File
# Begin Source File

SOURCE=.\Test.h
# End Source File
# Begin Source File

SOURCE=.\TestCaller.h
# End Source File
# Begin Source File

SOURCE=.\TestCase.cpp
# End Source File
# Begin Source File

SOURCE=.\TestCase.h
# End Source File
# Begin Source File

SOURCE=.\TestDecorator.h
# End Source File
# Begin Source File

SOURCE=.\TestFailure.cpp
# End Source File
# Begin Source File

SOURCE=.\TestFailure.h
# End Source File
# Begin Source File

SOURCE=.\TestResult.cpp
# End Source File
# Begin Source File

SOURCE=.\TestResult.h
# End Source File
# Begin Source File

SOURCE=.\TestSetup.h
# End Source File
# Begin Source File

SOURCE=.\TestSuite.cpp
# End Source File
# Begin Source File

SOURCE=.\TestSuite.h
# End Source File
# End Target
# End Project
