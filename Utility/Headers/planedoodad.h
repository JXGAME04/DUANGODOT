#pragma once
#include "iCommon.h"

/////////////////////////////////////////////////////////////////////
//plane and doodad
/////////////////////////////////////////////////////////////////////
const int MAX_VALUE_ITEM = 32;

_declspec(dllexport) int StringToPlaneTableValue(int table, const CString& strValue);
_declspec(dllexport) CString GetTablePlaneName(int table);
_declspec(dllexport) CString* GetTablePlaneValue(int e);
_declspec(dllexport) LPCSTR GetTablePlaneValue(int table, int index);

_declspec(dllexport) int StringToDoodadTableValue(int table, const CString& strValue);
_declspec(dllexport) CString GetTableDoodadName(int table);
_declspec(dllexport) CString* GetTableDoodadValue(int e);
_declspec(dllexport) CString GetTableDoodadDefaultValue(int e);

#define FILE_TEMPLATE_INDEX "\\template\\index.txt"
#define FILE_TEMPLATE_INDEX_NPC "\\template\\index_npc.txt"
#define FILE_TEMPLATE_ZIP "\\template\\template.zip"
#define FILE_TEMPLATE_ZIP_NPC "\\template\\template_npc.zip"
#define FILE_TEMPLATE_TERRAININDEX "\\template\\terrainindex.txt"

#define FOLDER_SPR "\\spr"
#define FOLDER_SPR_TERRAIN "\\spr\\terrain"

#define FILE_PLANEHORLINK "PlaneHorLink.ini"
#define FILE_PROPERTY "Region.ini"
#define FILE_TERRAIN "Region.dat"
#define FILE_PLANEVER "PlaneVer.ini"
#define FILE_PLANEVERBELOWGROUND "PlaneVerBelowGround.ini"
#define FILE_PLANEHORBELOWVER "PlaneHorBelowVer.ini"
#define FILE_LIGHT "light.ini"
#define FILE_OBSTACLE "obstacle.ini"
#define FILE_NPC "npc.ini"
#define FILE_BOX "box.ini"
#define FILE_TRAP "trap.ini"
#define FILE_CLIENTONLYNPC "npc.ini"
#define FILE_CLIENTONLYBOX "box.ini"
#define FILE_CLIENTONLYTRAP "trap.ini"
#define FILE_PLANEVEROBSTACLE "PlaneVer_Obstacle.ini"
#define FILE_PLANEHORBELOWVEROBSTACLE "PlaneHorBelowVer_Obstacle.ini"

#define PKFILE_PROPERTY "Region.pak"
#define PKFILE_TERRAIN "terrain.pak"
#define PKFILE_PLANEVER "PlaneVer.pak"
#define PKFILE_PLANEVERBELOWGROUND "PlaneVerBelowGround.pak"
#define PKFILE_PLANEHORBELOWVER "PlaneHor.pak"
#define PKFILE_LIGHT "light.pak"
#define PKFILE_OBSTACLE "obstacle.pak"
#define PKFILE_NPC "npc.pak"
#define PKFILE_BOX "box.pak"
#define PKFILE_TRAP "trap.pak"
#define PKFILE_CLIENTONLYTRAP "clientonlytrap.pak"
#define PKFILE_CLIENTONLYNPC "clientonlynpc.pak"
#define PKFILE_CLIENTONLYBOX "clientonlybox.pak"

#define FILE_CAMERA "camera.txt"
#define FILE_SPECIAL "special.txt"
#define FILE_WOR "wor.txt"

#define PKFILE_CAMERA "camera.pak"
#define PKFILE_SPECIAL "special.pak"
#define PKFILE_WOR "wor.pak"

#define BINFILE_OBSTACLE "obstacle.bin"
#define BINFILE_TRAP "trap.bin"
#define BINFILE_CLIENTONLYTRAP "clientonlytrap.bin"
#define BINFILE_TRAP_SCRIPT "trapscript.ini"
