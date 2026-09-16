//////////////////////////////////////////////////////////////////////////////////////
//
//  FileName    :   UpdatePublic.h
//  Version     :   1.0
//  Creater     :   Cheng Bitao
//  Date        :   2002-1-17 11:14:17
//  Comment     :   Define the comment interface of update system
//
//////////////////////////////////////////////////////////////////////////////////////


#ifndef __UPDATE_PUBLIC_H__
#define __UPDATE_PUBLIC_H__ 1

#include "DataDefine.h"
#include "ProcessIndex.h" 

extern CProcessIndex g_ProcessIndex;

//------------------------------------------------------------------------------------


int GetSerialNumberFromRegistry(HKEY hKey, const char cszKeyName[]);

int CheckSerialNumberValidity();

int GetInstallComponetInfo(const char cszIndexTxtFile[]);
int ProcessIndexFile(const char cszFileName[]);
int UpdateFiles();
int UpdateSelf();

int GetHostURL(const char cszFileName[], const char cszHostName[], char szHostURL[]);

#endif  //__UPDATE_PUBLIC_H__

