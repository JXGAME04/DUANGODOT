# Microsoft Developer Studio Project File - Name="S3DBInterfaceLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=S3DBInterfaceLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "S3DBInterfaceLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "S3DBInterfaceLib.mak" CFG="S3DBInterfaceLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "S3DBInterfaceLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "S3DBInterfaceLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Sword3PaySys/XSHANJU/Sword3PaySys", MCGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "S3DBInterfaceLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "S3DBInterfaceLib___Win32_Release"
# PROP BASE Intermediate_Dir "S3DBInterfaceLib___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "S3DBInterfaceLib___Win32_Release"
# PROP Intermediate_Dir "S3DBInterfaceLib___Win32_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "S3DBInterfaceLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "S3DBInterfaceLib___Win32_Debug"
# PROP BASE Intermediate_Dir "S3DBInterfaceLib___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "S3DBInterfaceLib___Win32_Debug"
# PROP Intermediate_Dir "S3DBInterfaceLib___Win32_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\regexp" /I "..\culib" /I "..\mysql++-1.7.1-1-win32-vc++\include" /I "..\mysql\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "S3DBInterfaceLib - Win32 Release"
# Name "S3DBInterfaceLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\GlobalFun.cpp
# End Source File
# Begin Source File

SOURCE=.\S3DBInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PAccCardHistoryDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PAccountHabitusDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PAccountInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PCard.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PCardHistoryDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PCardInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnector.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketParser.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketPool.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketServer.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipBaseInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipBaseInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipEfficInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipEfficInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipment.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipmentsDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipRequireInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PEquipRequireInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PFightSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PFightSkillDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PFriend.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PFriendListDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PLifeSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PLifeSkillDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PList.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PManipulator.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PRelockAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PResult.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PRole.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PRoleInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PRoleList.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PRow.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PServerListDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTableDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTableInfoCatch.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTableObjList.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTask.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTaskList.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTaskListDAO.cpp
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

SOURCE=.\LoginDef.h
# End Source File
# Begin Source File

SOURCE=.\S3DBInterface.h
# End Source File
# Begin Source File

SOURCE=.\S3PAccCardHistoryDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PAccount.h
# End Source File
# Begin Source File

SOURCE=.\S3PAccountHabitusDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PAccountInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PCard.h
# End Source File
# Begin Source File

SOURCE=.\S3PCardHistoryDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PCardInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnection.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnectionPool.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBConnector.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketParser.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketPool.h
# End Source File
# Begin Source File

SOURCE=.\S3PDBSocketServer.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipBaseInfo.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipBaseInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipEfficInfo.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipEfficInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipment.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipmentsDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipRequireInfo.h
# End Source File
# Begin Source File

SOURCE=.\S3PEquipRequireInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PFightSkill.h
# End Source File
# Begin Source File

SOURCE=.\S3PFightSkillDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PFriend.h
# End Source File
# Begin Source File

SOURCE=.\S3PFriendListDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PLifeSkill.h
# End Source File
# Begin Source File

SOURCE=.\S3PLifeSkillDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PList.h
# End Source File
# Begin Source File

SOURCE=.\S3PManipulator.h
# End Source File
# Begin Source File

SOURCE=.\S3PRelockAccount.h
# End Source File
# Begin Source File

SOURCE=.\S3PResult.h
# End Source File
# Begin Source File

SOURCE=.\S3PRole.h
# End Source File
# Begin Source File

SOURCE=.\S3PRoleInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PRoleList.h
# End Source File
# Begin Source File

SOURCE=.\S3PRow.h
# End Source File
# Begin Source File

SOURCE=.\S3PServerListDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTableDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTableInfoCatch.h
# End Source File
# Begin Source File

SOURCE=.\S3PTableObjList.h
# End Source File
# Begin Source File

SOURCE=.\S3PTask.h
# End Source File
# Begin Source File

SOURCE=.\S3PTaskList.h
# End Source File
# Begin Source File

SOURCE=.\S3PTaskListDAO.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\RegExp\lib\RegExp.lib
# End Source File
# Begin Source File

SOURCE="..\mysql++-1.7.1-1-win32-vc++\lib\mysql++.lib"
# End Source File
# Begin Source File

SOURCE=..\culib\lib\culib.lib
# End Source File
# End Target
# End Project
