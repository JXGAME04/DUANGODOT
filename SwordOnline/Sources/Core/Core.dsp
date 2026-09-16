# Microsoft Developer Studio Project File - Name="Core" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Core - Win32 Client Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Core.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Core.mak" CFG="Core - Win32 Client Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Core - Win32 Server Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Core - Win32 Server Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Core - Win32 Client Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Core - Win32 Client Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Core", FPAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Core___Win32_Server_Debug"
# PROP BASE Intermediate_Dir "Core___Win32_Server_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ServerDebug"
# PROP Intermediate_Dir "ServerDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\engine\src" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /YX"KCore.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W2 /Gm /GX /Zi /Od /I "..\engine\src" /I "..\engine\include" /I "..\Network" /I "..\..\Headers" /D "_DEBUG" /D "_SERVER" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /FR /Yu"KCore.h" /FD /LD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lualibdll.lib Ws2_32.lib Winmm.lib Shlwapi.lib /nologo /dll /incremental:no /map /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libcd.lib" /nodefaultlib:"msvcrtd.lib" /out:"ServerDebug\CoreServer.dll" /pdbtype:sept /libpath:"../../lib/"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\server\debug	md ..\..\lib\debug	copy ServerDebug\CoreServer.map ..\..\..\bin\server\Coreserver.map	copy ServerDebug\CoreServer.pdb ..\..\..\bin\server\Coreserver.pdb	copy ServerDebug\CoreServer.dll ..\..\..\bin\server\Coreserver.dll	copy ServerDebug\CoreServer.map ..\..\..\bin\server\debug\Coreserver.map	copy ServerDebug\CoreServer.pdb ..\..\..\bin\server\debug\Coreserver.pdb	copy ServerDebug\CoreServer.dll ..\..\..\bin\server\debug\Coreserver.dll	copy ServerDebug\CoreServer.lib ..\..\lib\debug\CoreServer.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Core___Win32_Server_Release"
# PROP BASE Intermediate_Dir "Core___Win32_Server_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ServerRelease"
# PROP Intermediate_Dir "ServerRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /D "_SERVER" /YX /FD /c
# ADD CPP /nologo /MT /W2 /GX /O2 /I "..\engine\src" /I "..\engine\include" /I "..\Network" /I "..\..\Headers" /D "NDEBUG" /D "_SERVER" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /FR /Yu"KCore.h" /FD /c
# SUBTRACT CPP /Z<none>
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lualibdll.lib Ws2_32.lib Winmm.lib Shlwapi.lib /nologo /dll /map /debug /machine:I386 /out:"ServerRelease\CoreServer.dll" /ignore:4099
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\server\release	md ..\..\lib\release	copy ServerRelease\CoreServer.map ..\..\..\bin\server\Coreserver.map	copy ServerRelease\CoreServer.pdb ..\..\..\bin\server\Coreserver.pdb	copy ServerRelease\CoreServer.dll ..\..\..\bin\server\Coreserver.dll	copy ServerRelease\CoreServer.lib ..\..\lib\release\CoreServer.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Core___Win32_Client_Debug"
# PROP BASE Intermediate_Dir "Core___Win32_Client_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ClientDebug"
# PROP Intermediate_Dir "ClientDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\engine\src" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /YX"KCore.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\engine\src" /I "..\..\Headers" /I "..\engine\include" /I "..\Network" /I "src" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /D "SWORDONLINE_SHOW_DBUG_INFO" /FR /Yu"KCore.h" /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lualibdll.lib Shlwapi.lib Winmm.lib /nologo /dll /incremental:no /map /debug /machine:I386 /out:"ClientDebug\CoreClient.dll" /pdbtype:sept /libpath:"../../lib/"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\client\debug	md ..\..\lib\debug	copy ClientDebug\CoreClient.pdb ..\..\..\bin\client\CoreClient.pdb	copy ClientDebug\CoreClient.map ..\..\..\bin\client\CoreClient.map	copy ClientDebug\CoreClient.dll ..\..\..\bin\client\CoreClient.dll	copy ClientDebug\CoreClient.pdb ..\..\..\bin\client\debug\CoreClient.pdb	copy ClientDebug\CoreClient.map ..\..\..\bin\client\debug\CoreClient.map	copy ClientDebug\CoreClient.dll ..\..\..\bin\client\debug\CoreClient.dll	copy ClientDebug\CoreClient.lib ..\..\lib\debug\CoreClient.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Core___Win32_Client_Release"
# PROP BASE Intermediate_Dir "Core___Win32_Client_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ClientRelease"
# PROP Intermediate_Dir "ClientRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /D "_SERVER" /YX /FD /c
# ADD CPP /nologo /MD /W2 /GX /Zi /O2 /I "..\engine\src" /I "..\engine\include" /I "..\Network" /I "..\..\Headers" /I "src" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CORE_EXPORTS" /FR /Yu"KCore.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lualibdll.lib Shlwapi.lib Winmm.lib /nologo /dll /map /debug /machine:I386 /out:"ClientRelease\CoreClient.dll" /libpath:"../../lib/"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\..\bin\client\release	md ..\..\lib\release	copy ClientRelease\CoreClient.pdb ..\..\..\bin\client\CoreClient.pdb	copy ClientRelease\CoreClient.map ..\..\..\bin\client\CoreClient.map	copy ClientRelease\CoreClient.dll ..\..\..\bin\client\CoreClient.dll	copy ClientRelease\CoreClient.lib ..\..\lib\release\CoreClient.lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Core - Win32 Server Debug"
# Name "Core - Win32 Server Release"
# Name "Core - Win32 Client Debug"
# Name "Core - Win32 Client Release"
# Begin Group "技能"

# PROP Default_Filter ""
# Begin Group "玩家宠物"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KPartnerSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPartnerSkill.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerPartner.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerPartner.h
# End Source File
# Begin Source File

SOURCE=".\Src\PlayerPartner文档.txt"
# End Source File
# End Group
# Begin Group "偷窃技能"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KThiefSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KThiefSkill.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Src\KMissle.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# ADD CPP /FAs

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KMissle.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMissleMagicAttribsData.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMissleRes.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMissleRes.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMissleSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMissleSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillManager.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSkills.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSkills.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillSpecial.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSkillSpecial.h
# End Source File
# Begin Source File

SOURCE=.\Src\Skill.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Skill.h
# End Source File
# Begin Source File

SOURCE=.\Src\SkillDef.h
# End Source File
# Begin Source File

SOURCE=".\Src\技能系统.txt"
# End Source File
# End Group
# Begin Group "人物"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Src\AI设计.txt"
# End Source File
# Begin Source File

SOURCE=.\Src\KFaction.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFaction.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpc.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpc.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcAI.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcAI.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcAttribModify.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcAttribModify.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcDeathCalcExp.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcDeathCalcExp.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcFindPath.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcFindPath.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcGold.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcGold.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcRes.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcRes.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcResList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcResList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcResNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcResNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcTemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNpcTemplate.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayer.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayer.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerChat.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerChat.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerDBFuns.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerDef.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerFaction.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerFaction.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerMenuState.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerMenuState.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerPK.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerPK.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTask.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTask.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTeam.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTeam.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTong.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTong.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTrade.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPlayerTrade.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSprControl.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSprControl.h
# End Source File
# Begin Source File

SOURCE=".\Src\NPC系统.txt"
# End Source File
# End Group
# Begin Group "物品装备"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KBasPropTbl.CPP
# End Source File
# Begin Source File

SOURCE=.\Src\KBasPropTbl.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBuySell.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBuySell.h
# End Source File
# Begin Source File

SOURCE=.\Src\KInventory.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KInventory.h
# End Source File
# Begin Source File

SOURCE=.\Src\KItem.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KItem.h
# End Source File
# Begin Source File

SOURCE=.\Src\KItemChangeRes.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KItemChangeRes.h
# End Source File
# Begin Source File

SOURCE=.\Src\KItemGenerator.CPP
# End Source File
# Begin Source File

SOURCE=.\Src\KItemGenerator.h
# End Source File
# Begin Source File

SOURCE=.\Src\KItemList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KItemList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KItemSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KItemSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KObj.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KObj.h
# End Source File
# Begin Source File

SOURCE=.\Src\KObjSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KObjSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KViewItem.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KViewItem.h
# End Source File
# Begin Source File

SOURCE=.\Src\MyAssert.H
# End Source File
# End Group
# Begin Group "世界"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KCore.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KCore.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSubWorld.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSubWorld.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSubWorldSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSubWorldSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWeatherMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWeatherMgr.h
# End Source File
# End Group
# Begin Group "地图"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KRegion.h
# End Source File
# End Group
# Begin Group "消息"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KWorldMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWorldMsg.h
# End Source File
# End Group
# Begin Group "数学运算"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KMath.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMath.h
# End Source File
# End Group
# Begin Group "协议"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KGMProcess.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KNewProtocolProcess.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KNewProtocolProcess.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KProtocol.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Headers\KProtocol.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\KProtocolDef.h
# End Source File
# Begin Source File

SOURCE=.\Src\KProtocolProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KProtocolProcess.h
# End Source File
# Begin Source File

SOURCE=.\Protocol.xls
# End Source File
# End Group
# Begin Group "脚本"

# PROP Default_Filter ""
# Begin Group "AI脚本"

# PROP Default_Filter ""
# End Group
# Begin Group "立即性脚本"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\ScriptFuns.cpp
# End Source File
# End Group
# Begin Group "单步性脚本"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\LuaFuns.cpp
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\Src\KScriptValueSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptValueSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSortScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSortScript.h
# End Source File
# Begin Source File

SOURCE=.\Src\LuaFuns.h
# End Source File
# Begin Source File

SOURCE=".\Src\脚本说明.txt"
# End Source File
# End Group
# Begin Group "被外界访问接口"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\CoreObjGenreDef.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\CoreServerDataDef.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\CoreServerShell.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\CoreServerShell.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\CoreShell.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\CoreShell.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\CoreUseNameDef.h
# End Source File
# Begin Source File

SOURCE=.\Src\GameDataDef.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KIndexNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\MsgGenreDef.h
# End Source File
# End Group
# Begin Group "数据库相关"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\S3DBInterface.h
# End Source File
# End Group
# Begin Group "场景"

# PROP Default_Filter ""
# Begin Group "场景中对象与对象树"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\Scene\KIpotBranch.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KIpotBranch.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KIpotLeaf.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KIpotLeaf.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KIpoTree.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# ADD CPP /FAs
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KIpoTree.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\Src\Scene\KScenePlaceC.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KScenePlaceC.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KScenePlaceRegionC.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KScenePlaceRegionC.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KWeather.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\Scene\KWeather.h
# End Source File
# Begin Source File

SOURCE=.\Src\Scene\ObstacleDef.h
# End Source File
# Begin Source File

SOURCE=.\Src\Scene\SceneDataDef.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\SceneMath.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\SceneMath.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\ScenePlaceMapC.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\Scene\ScenePlaceMapC.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "图形引用"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\CoreDrawGameObj.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\CoreDrawGameObj.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ImgRef.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ImgRef.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "魔法属性"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KMagicAttrib.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMagicDesc.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMagicDesc.h
# End Source File
# End Group
# Begin Group "GM管理"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KGMCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KGMCommand.h
# End Source File
# End Group
# Begin Group "任务相关"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KLinkArrayTemplate.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMission.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KMission.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KMissionArray.h

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KTaskFuns.cpp

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# SUBTRACT CPP /WX

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\KTaskFuns.h
# End Source File
# Begin Source File

SOURCE=.\Src\TaskDef.h
# End Source File
# Begin Source File

SOURCE=".\Src\任务系统设计文档.txt"
# End Source File
# End Group
# Begin Group "选项"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KOption.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KOption.h
# End Source File
# End Group
# Begin Group "音乐"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KMapMusic.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMapMusic.h
# End Source File
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Group "Release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\release\engine.lib

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\release\common.lib

!IF  "$(CFG)" == "Core - Win32 Server Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\debug\engine.lib

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\debug\common.lib

!IF  "$(CFG)" == "Core - Win32 Server Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Server Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Core - Win32 Client Debug"

!ELSEIF  "$(CFG)" == "Core - Win32 Client Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "排名"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\KLadder.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KLadder.h
# End Source File
# End Group
# Begin Group "USBKey"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\USBKey\des.h
# End Source File
# Begin Source File

SOURCE=.\Src\USBKey\EPASSAPI.H
# End Source File
# Begin Source File

SOURCE=.\Src\USBKey\epsJO.h
# End Source File
# Begin Source File

SOURCE=.\Src\USBKey\MD5.H
# End Source File
# Begin Source File

SOURCE=.\Src\USBKey\EP1KDL20.LIB
# End Source File
# Begin Source File

SOURCE=.\Src\USBKey\EpsForJoLib.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\Src\stdafx.cpp
# ADD CPP /Yc"KCore.h"
# End Source File
# End Target
# End Project
