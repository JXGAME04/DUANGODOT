# Microsoft Developer Studio Project File - Name="S3DBInterface" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=S3DBInterface - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "S3DBInterface.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "S3DBInterface.mak" CFG="S3DBInterface - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "S3DBInterface - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "S3DBInterface - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Sword3PaySys/XSHANJU/Sword3PaySys", MCGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "S3DBInterface - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "S3DBInterface___Win32_Release"
# PROP BASE Intermediate_Dir "S3DBInterface___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "S3DBInterface___Win32_Release"
# PROP Intermediate_Dir "S3DBInterface___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "S3DBINTERFACE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\regexp" /I "..\culib" /I "..\mysql++-1.7.1-1-win32-vc++\include" /I "..\mysql\include" /I "..\..\..\Engine\Src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "S3DBINTERFACE_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\RegExp\lib\RegExp.lib ..\culib\lib\culib.lib ..\mysql++-1.7.1-1-win32-vc++\lib\mysql++.lib Ws2_32.lib Winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib LIBCMT.lib engine.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBCD.lib" /out:"S3DBInterface___Win32_Release/S3DBInterface_r.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "S3DBInterface - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "S3DBINTERFACE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\regexp" /I "..\culib" /I "..\mysql++-1.7.1-1-win32-vc++\include" /I "..\mysql\include" /I "..\..\..\Engine\Src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "S3DBINTERFACE_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\RegExp\lib\RegExp.lib ..\culib\lib\culib.lib ..\mysql++-1.7.1-1-win32-vc++\lib\mysql++.lib Ws2_32.lib Winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib LIBCMTD.lib engine.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBCD.lib" /out:"E:\User\douhao\Bin\Server\S3DBInterface_d.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy debug\S3dBInterface.dll ..\..\..\..\..\Bin\Server\S3DBInterface.dll	copy debug\s3dbinterface.lib ..\..\..\..\Lib\S3dBInterface.lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "S3DBInterface - Win32 Release"
# Name "S3DBInterface - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
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

SOURCE=.\S3PAccountUser.h
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
# Begin Source File

SOURCE=.\S3PTestAccount.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestAccountInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestCard.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestCardInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestFriendListDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestRole.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestRoleInfoDAO.h
# End Source File
# Begin Source File

SOURCE=.\S3PTestTaskList.h
# End Source File
# Begin Source File

SOURCE=.\TestRunner.h
# End Source File
# Begin Source File

SOURCE=.\TextTestResult.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\GlobalFun.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
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

SOURCE=.\S3PAccountUser.cpp
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
# Begin Source File

SOURCE=.\S3PTestAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestAccountInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestCard.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestCardInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestFriendListDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestRole.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestRoleInfoDAO.cpp
# End Source File
# Begin Source File

SOURCE=.\S3PTestTaskList.cpp
# End Source File
# Begin Source File

SOURCE=.\TestRunner.cpp
# End Source File
# Begin Source File

SOURCE=.\TextTestResult.cpp
# End Source File
# End Group
# End Target
# End Project
