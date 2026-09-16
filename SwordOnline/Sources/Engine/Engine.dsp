# Microsoft Developer Studio Project File - Name="Engine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Engine - Win32 OutRead Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Engine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Engine.mak" CFG="Engine - Win32 OutRead Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Engine - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Engine - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Engine - Win32 OutRead Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Engine - Win32 OuRead Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SwordOnline/Sources/Engine", BAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Engine - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "include" /I "..\Pack\ucl-1.01\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /D WINVER=0x0500 /YX"KWin32.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib zlib.lib /nologo /dll /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\lib\release	copy release\engine.lib ..\..\lib\release\engine.lib	copy release\engine.lib ..\..\lib\engine.lib	md ..\..\..\bin\client\release\	copy release\engine.dll ..\..\..\bin\client\engine.dll	copy release\engine.pdb ..\..\..\bin\client\engine.pdb	copy release\engine.map ..\..\..\bin\client\engine.map	md ..\..\..\bin\server\release\	copy release\engine.dll ..\..\..\bin\server\engine.dll	copy release\engine.pdb ..\..\..\bin\server\engine.pdb	copy release\engine.map ..\..\..\bin\server\engine.map
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /D "USE_STANDALONE_SPR" /D WINVER=0x0500 /FR /Yu"KWin32.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib /nologo /dll /incremental:no /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\lib\debug	copy debug\engine.lib ..\..\lib\debug\engine.lib	md ..\..\..\bin\client\debug\	copy debug\engine.dll ..\..\..\bin\client\engine.dll	copy debug\engine.dll ..\..\..\bin\client\debug\engine.dll	md ..\..\..\bin\server\debug\	copy debug\engine.dll ..\..\..\bin\server\engine.dll	copy debug\engine.dll ..\..\..\bin\server\debug\engine.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Engine___Win32_OutRead_Debug"
# PROP BASE Intermediate_Dir "Engine___Win32_OutRead_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Engine___Win32_OutRead_Debug"
# PROP Intermediate_Dir "Engine___Win32_OutRead_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /FR /Yu"KWin32.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /D "USE_STANDALONE_SPR" /FR /Yu"KWin32.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib /nologo /dll /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\lib\debug	copy Engine___Win32_OutRead_Debug\Engine.lib ..\..\lib\debug\engine.lib	md ..\..\..\bin\client\debug\	copy Engine___Win32_OutRead_Debug\Engine.dll ..\..\..\bin\client\engine.dll	copy Engine___Win32_OutRead_Debug\Engine.dll ..\..\..\bin\client\debug\engine.dll	md ..\..\..\bin\server\debug\	copy Engine___Win32_OutRead_Debug\Engine.dll ..\..\..\bin\server\engine.dll	copy Engine___Win32_OutRead_Debug\Engine.dll ..\..\..\bin\server\debug\engine.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Engine___Win32_OuRead_Release"
# PROP BASE Intermediate_Dir "Engine___Win32_OuRead_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Engine___Win32_OuRead_Release"
# PROP Intermediate_Dir "Engine___Win32_OuRead_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /I "include" /I "..\Pack\ucl-1.01\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /D WINVER=0x0500 /YX"KWin32.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "include" /I "..\Pack\ucl-1.01\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ENGINE_EXPORTS" /D "USE_STANDALONE_SPR" /YX"KWin32.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib /nologo /dll /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib ddraw.lib dsound.lib dxguid.lib winmm.lib wsock32.lib dinput8.lib /nologo /dll /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md ..\..\lib\release	copy Engine___Win32_OuRead_Release\Engine.lib ..\..\lib\release\engine.lib	md ..\..\..\bin\client\release\	copy Engine___Win32_OuRead_Release\engine.dll ..\..\..\bin\client\engine.dll	copy Engine___Win32_OuRead_Release\engine.dll ..\..\..\bin\client\release\engine.dll	copy Engine___Win32_OuRead_Release\engine.pdb ..\..\..\bin\client\release\engine.pdb	copy Engine___Win32_OuRead_Release\engine.map ..\..\..\bin\client\release\engine.map	md ..\..\..\bin\server\release\	copy Engine___Win32_OuRead_Release\Engine.dll ..\..\..\bin\server\engine.dll	copy Engine___Win32_OuRead_Release\Engine.dll ..\..\..\bin\server\release\engine.dll	copy Engine___Win32_OuRead_Release\Engine.pdb ..\..\..\bin\server\release\engine.pdb	copy Engine___Win32_OuRead_Release\Engine.map ..\..\..\bin\server\release\engine.map
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Engine - Win32 Release"
# Name "Engine - Win32 Debug"
# Name "Engine - Win32 OutRead Debug"
# Name "Engine - Win32 OuRead Release"
# Begin Group "source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Src\DrawSpriteMP.h
# End Source File
# Begin Source File

SOURCE=.\Src\KAutoMutex.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KAviFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmap16.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmapConvert.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBmp2Spr.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBmpFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KBmpFile24.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KCodecLzo.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KColors.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDDraw.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDebug.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDError.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDInput.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBase.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBitmap16.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawFade.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawFont.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawSprite.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawSpriteAlpha.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KDSound.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KEicScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KEicScriptSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFileCopy.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFileDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFilePath.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFindBinTree.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KFont.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KGifFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KGraphics.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KHashList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KHashNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KHashTable.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\Kime.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KIniFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KJpgFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KKeyboard.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KLinkArray.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KLuaScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KLuaScriptSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KLubCmpl_Blocker.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMemBase.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMemClass.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMemClass1.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMemManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMemStack.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMouse.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMp3Music.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Audio.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Movie.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Video.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMpgMusic.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMsgNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMusic.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KMutex.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KOctree.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KOctreeNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPakData.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPakFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPakList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPakMake.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPakTool.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPalette.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPcxFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPolygon.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KPolyRelation.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KRandom.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSafeList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScanDir.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSG_MD5_String.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSG_StringProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSortBinTree.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSortList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSoundCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSprite.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteMaker.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KStepLuaScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KStrBase.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KStrList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KStrNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KTabFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KTabFileCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KTgaFile32.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KThread.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KTimer.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWavCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWavFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWavMusic.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWavSound.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWin32.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KWin32Wnd.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KZipCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KZipData.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KZipFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\KZipList.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\stdafx.cpp
# ADD CPP /Yc"kwin32.h"
# End Source File
# Begin Source File

SOURCE=.\Src\XPackFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\ZPackFile.cpp
# End Source File
# End Group
# Begin Group "header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\Src\Colors.h
# End Source File
# Begin Source File

SOURCE=.\Src\KAutoMutex.h
# End Source File
# Begin Source File

SOURCE=.\Src\KAviFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBinsTree.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBinTreeNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmap.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmap16.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBitmapConvert.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBmp2Spr.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBmpFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KBmpFile24.h
# End Source File
# Begin Source File

SOURCE=.\Src\KCache.h
# End Source File
# Begin Source File

SOURCE=.\Src\KCanvas.h
# End Source File
# Begin Source File

SOURCE=.\Src\KCodec.h
# End Source File
# Begin Source File

SOURCE=.\Src\KCodecLzo.h
# End Source File
# Begin Source File

SOURCE=.\Src\KColors.h
# End Source File
# Begin Source File

SOURCE=.\Src\KCriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDDraw.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDebug.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDError.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDInput.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBase.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBitmap.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawBitmap16.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawFade.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawFont.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawSprite.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDrawSpriteAlpha.h
# End Source File
# Begin Source File

SOURCE=.\Src\KDSound.h
# End Source File
# Begin Source File

SOURCE=.\Src\KEicScript.h
# End Source File
# Begin Source File

SOURCE=.\Src\KEicScriptSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KEngine.h
# End Source File
# Begin Source File

SOURCE=.\Src\KEvent.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFileCopy.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFileDialog.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFilePath.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFindBinTree.h
# End Source File
# Begin Source File

SOURCE=.\Src\KFont.h
# End Source File
# Begin Source File

SOURCE=.\Src\KGifFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KGraphics.h
# End Source File
# Begin Source File

SOURCE=.\Src\KHashList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KHashNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KHashTable.h
# End Source File
# Begin Source File

SOURCE=.\Src\Kime.h
# End Source File
# Begin Source File

SOURCE=.\Src\KIniFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KITabFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KJpgFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KKeyboard.h
# End Source File
# Begin Source File

SOURCE=.\Src\KLinkArray.h
# End Source File
# Begin Source File

SOURCE=.\Src\KList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KLuaScript.h
# End Source File
# Begin Source File

SOURCE=.\Src\KLuaScriptSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KLubCmpl_Blocker.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMemBase.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMemClass.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMemClass1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Tools\Sources\LubCompile\KMemClass1.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMemManager.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMemStack.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMessage.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMouse.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMp3Music.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Audio.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Movie.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMp4Video.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMpgMusic.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMsgNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMusic.h
# End Source File
# Begin Source File

SOURCE=.\Src\KMutex.h
# End Source File
# Begin Source File

SOURCE=.\Src\KNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KOctree.h
# End Source File
# Begin Source File

SOURCE=.\Src\KOctreeNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPakData.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPakFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPakList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPakMake.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPakTool.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPalette.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPcxFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPolygon.h
# End Source File
# Begin Source File

SOURCE=.\Src\KPolyRelation.h
# End Source File
# Begin Source File

SOURCE=.\Src\KRandom.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSafeList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KScanDir.h
# End Source File
# Begin Source File

SOURCE=.\Src\KScript.h
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptCache.h
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KScriptSet.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSG_MD5_String.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSG_StringProcess.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSortBinTree.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSortList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSoundCache.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSprite.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteCache.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteCodec.h
# End Source File
# Begin Source File

SOURCE=.\Src\KSpriteMaker.h
# End Source File
# Begin Source File

SOURCE=.\Src\KStepLuaScript.h
# End Source File
# Begin Source File

SOURCE=.\Src\KStrBase.h
# End Source File
# Begin Source File

SOURCE=.\Src\KStrList.h
# End Source File
# Begin Source File

SOURCE=.\Src\KStrNode.h
# End Source File
# Begin Source File

SOURCE=.\Src\KTabFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KTabFileCtrl.h
# End Source File
# Begin Source File

SOURCE=.\Src\KTgaFile32.h
# End Source File
# Begin Source File

SOURCE=.\Src\KThread.h
# End Source File
# Begin Source File

SOURCE=.\Src\KTimer.h
# End Source File
# Begin Source File

SOURCE=.\Src\KVideo.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWavCodec.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWavFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWavMusic.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWavSound.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWin32.h
# End Source File
# Begin Source File

SOURCE=.\Src\KWin32Wnd.h
# End Source File
# Begin Source File

SOURCE=.\Src\KZipCodec.h
# End Source File
# Begin Source File

SOURCE=.\Src\KZipData.h
# End Source File
# Begin Source File

SOURCE=.\Src\KZipFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\KZipList.h
# End Source File
# Begin Source File

SOURCE=.\Src\LinkStruct.h
# End Source File
# Begin Source File

SOURCE=.\Src\md5.h
# End Source File
# Begin Source File

SOURCE=.\Src\XPackFile.h
# End Source File
# Begin Source File

SOURCE=.\Src\ZPackFile.h
# End Source File
# End Group
# Begin Group "library files"

# PROP Default_Filter "h,lib"
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\debug\JpgLib.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\debug\LuaLibDll.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\debug\mp3lib.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Lib\release\JpgLib.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\release\LuaLibDll.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Lib\release\mp3lib.lib

!IF  "$(CFG)" == "Engine - Win32 Release"

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\Include\JpgLib.h
# End Source File
# Begin Source File

SOURCE=.\Include\LuaLib.h
# End Source File
# Begin Source File

SOURCE=..\..\Lib\mp3lib.h
# End Source File
# End Group
# Begin Group "译码与解码"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\Cryptography\EDOneTimePad.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\Cryptography\EDOneTimePad.h
# End Source File
# End Group
# Begin Group "文字处理"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\Text.cpp
# End Source File
# Begin Source File

SOURCE=.\Src\Text.h
# End Source File
# End Group
# Begin Group "ucl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\ucl\alloc.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\fake16.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\getbit.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\internal.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\io.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2b_99.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2b_d.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2b_ds.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2b_to.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2d_99.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2d_d.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2d_ds.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2d_to.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2e_99.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2e_d.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2e_ds.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\n2e_to.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Include\ucl\ucl.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_conf.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_crc.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_dll.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_init.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_ptr.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_ptr.h
# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_str.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_util.c

!IF  "$(CFG)" == "Engine - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Engine - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OutRead Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Engine - Win32 OuRead Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Src\ucl\ucl_util.h
# End Source File
# Begin Source File

SOURCE=.\Include\ucl\uclconf.h
# End Source File
# Begin Source File

SOURCE=.\Include\ucl\uclutil.h
# End Source File
# End Group
# End Target
# End Project
