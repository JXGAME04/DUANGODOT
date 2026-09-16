#ifndef _DATA_H_
#define _DATA_H_

#include "windows.h"
#include "GetFileVersion.h"

// ApplyVersion for Update package comment section

const int g_cnFileVersionTypeCount = 5;

typedef int GetVersionFunction(char szFileName[], char szVersion[], int nVersionLen);

extern GetVersionFunction *g_GetFileVersionTable[g_cnFileVersionTypeCount];

int IndexGen(
    const char cszSourcePath[], 
    const char cszDestFileName[], 
    int nVersion, 
    const  char cszRemotePath[],
    const char cszCommonInfoFileName[],
    const char cszFileCommentFileName[],
    const char cszInfoFileName[]
);


#endif   // _DATA_H_