//////////////////////////////////////////////////////////////////////////////////////
//
//  FileName    :   GetFileVersion.cpp
//  Version     :   1.0
//  Creater     :   Cheng BiTao
//  Date        :   2001-4-2 10:41:04
//  Comment     :   The commmon interface of get Kingsoft antivirus files version  
//
//////////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "stdio.h"
#include "GetFileVersion.h"
#include "data.h"


char g_szProgramPath[MAX_PATH] = {0};
const int g_cnVersionLen                    = 20;
char g_szKAV9XVersion[g_cnVersionLen]       = {0};
char g_szSignDataVersion[g_cnVersionLen]    = {0};
char g_szEngineVersion[g_cnVersionLen]      = {0};
char g_szDrWebVersion[g_cnVersionLen]       = {0};  
char g_szKAExtendVersion[g_cnVersionLen]    = {0};
char g_szKAEPlatVersion[g_cnVersionLen]     = {0};
char g_szKAVDXVersion[g_cnVersionLen]       = {0};
char g_szKAVEXTVersion[g_cnVersionLen]      = {0};
char g_szRescueVersion[g_cnVersionLen]      = {0};
char g_szUpdateVersion[g_cnVersionLen]      = {0};
char g_szKWatchVXDVersion[g_cnVersionLen]   = {0};
char g_szKWatchSYSVersion[g_cnVersionLen]   = {0};
char g_szKAVKRNLVersion[g_cnVersionLen]     = {0};



int FileExists(LPCTSTR lpszFileName)
{
    int nResult                     = false;
    HANDLE hFind                    = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA *pFindFileData  = NULL;


    pFindFileData = new WIN32_FIND_DATA;
    if (pFindFileData == NULL)
        goto Exit0;

    hFind = ::FindFirstFile(lpszFileName, pFindFileData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        if (!((pFindFileData->dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY))
            nResult = true;
     
        ::FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

Exit0:
    if (pFindFileData)
    {
        delete pFindFileData;
        pFindFileData = NULL;
    }

    return nResult;
}

static int inline _GetVersionStringFromInt(int nMajorVersion, int nMinorVersion, char szVersion[], int nVersionLen)
{
    _IntToStr(nMajorVersion >> 16, szVersion, 4);
    szVersion[4] = '.';
    _IntToStr(nMajorVersion & 0x0FFFF, szVersion + 5, 2);
    szVersion[7] = '.';
    _IntToStr(nMinorVersion >> 16, szVersion + 8, 2);
    szVersion[10] = '\0';
    if (nMinorVersion & 0x0FFFF)
    {
        szVersion[10] = '.';
        _IntToStr(nMinorVersion & 0x0FFFF, nVersionLen - 10, szVersion + 11);
    }

    return true;
}

int GetFileVersion(
    const char cszFileName[], 
    DWORD *pdwProductVersionMS, 
    DWORD *pdwProductVersionLS
)
{
    int nResult             = false;
    int nRetCode            = false;
    DWORD dwHandle          = 0;
    DWORD dwFileInfoSize    = 0;
    VS_FIXEDFILEINFO *pFixFileInfo = NULL;
    char *pszFileInfo       = NULL;
    UINT uLen               = 0;

    *pdwProductVersionMS = 0;
    *pdwProductVersionLS = 0;

    dwFileInfoSize = ::GetFileVersionInfoSize((char *)cszFileName, &dwHandle);
    if (dwFileInfoSize == 0)
        goto Exit0;      

    pszFileInfo = new char[dwFileInfoSize];
    if (pszFileInfo == NULL)
        goto Exit0;      

    nRetCode = GetFileVersionInfo((char *)cszFileName, dwHandle, dwFileInfoSize, pszFileInfo);
    if (!nRetCode)
        goto Exit0;

    nRetCode = VerQueryValue(pszFileInfo, "\\", (LPVOID *)&pFixFileInfo, &uLen);
    if (!nRetCode)
        goto Exit0;

    if (uLen > 0)
    {
        *pdwProductVersionMS = pFixFileInfo->dwProductVersionMS;
        *pdwProductVersionLS = pFixFileInfo->dwProductVersionLS;
    }

    nResult = true;

Exit0:
    if (pszFileInfo)
    {
        delete pszFileInfo;
        pszFileInfo = NULL;
    }
    
    return nResult;
}

int GetSignFileVersion(
    const char cszFileName[], 
    DWORD *pdwMajorVersion, 
    DWORD *pdwMinorVersion
)
{
    int nResult                  = false;
    int nRetCode                 = 0;
    int nReadSize                = 0;
    int  nKAVSignFileHeaderSize  = 0;
    HANDLE hFile                 = INVALID_HANDLE_VALUE;
    KAVSIGNFILEHEADER Header;
    WORD wTemp                   = 0;

    *pdwMajorVersion  = 0;
    *pdwMinorVersion  = 0;

    hFile = ::CreateFile(
        cszFileName,  
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL
    );
    if (INVALID_HANDLE_VALUE == hFile)
        goto Exit0;

    nKAVSignFileHeaderSize = sizeof(KAVSIGNFILEHEADER);
    
    nRetCode = ::ReadFile(hFile, (void *)&Header, nKAVSignFileHeaderSize, (DWORD *)&nReadSize, NULL);
    if (!nRetCode || (nReadSize != nKAVSignFileHeaderSize))
        goto Exit0;

    //dwMajorVersion
    wTemp = ((WORD)(Header.dwDateStamp >> 8)) & 0x00FF;
    *pdwMajorVersion = (Header.dwDateStamp & 0xFFFF0000) + wTemp;
        
    //dwMinorVersion
    wTemp = ((WORD)Header.dwDateStamp) & 0x00FF;
    *pdwMinorVersion = wTemp;
    *pdwMinorVersion = *pdwMinorVersion << 16;

    nResult = true;
   
Exit0:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        ::CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return nResult;
}

int GetEngineFileVersion(
    const char cszFileName[], 
    DWORD *pdwMajorVersion, 
    DWORD *pdwMinorVersion
)
{
	int nResult                  = false;
    int nRetCode                 = 0;
    int nReadSize                = 0;
    int nKAVEngineFileHeaderSize = 0;
    HANDLE hFile                 = INVALID_HANDLE_VALUE;
    KAVEngineFileHeader Header;

	*pdwMajorVersion = 0;
	*pdwMinorVersion = 0;

    hFile = ::CreateFile(
        cszFileName,  
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL
    );
    if (INVALID_HANDLE_VALUE == hFile)
        goto Exit0;

    nKAVEngineFileHeaderSize = sizeof(KAVEngineFileHeader);
    
    nRetCode = ::ReadFile(hFile, (void *)&Header, nKAVEngineFileHeaderSize, (DWORD *)&nReadSize, NULL);
    if (!nRetCode || (nReadSize != nKAVEngineFileHeaderSize))
        goto Exit0;

    *pdwMajorVersion = Header.dwOrignMajorVersion;
    *pdwMinorVersion = Header.dwOrignMinorVersion;
        
    nResult = true;

Exit0:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        ::CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

	return nResult;
}

int GetProgramVersion(char szFileName[], char szVersion[], int nVersionLen)
{
    int nRetCode = 0;
    unsigned uMajorVersion = 0;
    unsigned uMinorVersion = 0;

    if (!FileExists(szFileName))
        return false;

    nRetCode = GetFileVersion(szFileName, (DWORD *)&uMajorVersion, (DWORD *)&uMinorVersion);
    if (nRetCode)
        nRetCode = _GetVersionStringFromInt(uMajorVersion, uMinorVersion, szVersion, g_cnVersionLen);
    else 
        nRetCode = _GetVersionStringFromInt(1, 0, szVersion, g_cnVersionLen);

    return true;
}   

int GetEngineFileVersion(char szFileName[], char szVersion[], int nVersionLen)
{
    DWORD dwMajorVersion    = 0;
    DWORD dwMinorVersion    = 0;
    int nRetCode            = 0;

    nRetCode = GetEngineFileVersion(szFileName, &dwMajorVersion, &dwMinorVersion);
    if (!nRetCode)
        return false;

    nRetCode = _GetVersionStringFromInt(
        dwMajorVersion, dwMinorVersion, 
        szVersion, g_cnVersionLen
    );


    return true;
}

int GetDrWebFileVersion(char szFileName[], char szVersion[], int nVersionLen)
{
    int nRetCode            = 0;
    int nLen                = 0;
    DWORD dwMajorVersion    = 0;
    DWORD dwMinorVersion    = 0;

    nRetCode = GetEngineFileVersion(szFileName, &dwMajorVersion, &dwMinorVersion);
    if (!nRetCode)
        return false;

    nRetCode = _GetVersionStringFromInt(
        dwMajorVersion, dwMinorVersion, 
        szVersion, g_cnVersionLen
    );

    return true;
}

int GetSignFileVersion(char szFileName[], char szVersion[], int nVersionLen)
{
    int nRetCode = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;

    nRetCode = GetSignFileVersion(szFileName, &dwMajorVersion, &dwMinorVersion);
    if (!nRetCode) 
        return true;

    nRetCode = _GetVersionStringFromInt(
        dwMajorVersion, dwMinorVersion, 
        szVersion, g_cnVersionLen
    );

    return true;
}

int GetVersionByRes(char szFileName[], char szVersion[], int nVersionLen)
{
   IMAGE_DOS_HEADER DOSHeader;
   IMAGE_VXD_HEADER VXDHeader;
   HANDLE hFile = INVALID_HANDLE_VALUE;
   int nResult = false;
   DWORD dwRead;


   hFile = ::CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
        UINT uReadLen = 0;
        ::ReadFile(hFile, &DOSHeader, sizeof(DOSHeader), &dwRead, NULL);
        if (dwRead == sizeof(DOSHeader))
        {
            if (IMAGE_DOS_SIGNATURE == DOSHeader.e_magic)
            {
                ::SetFilePointer(hFile, DOSHeader.e_lfanew, NULL, FILE_BEGIN);
                ::ReadFile(hFile, &VXDHeader, sizeof(VXDHeader), &dwRead, NULL);
                if (dwRead == sizeof(VXDHeader))
                {
                    if (IMAGE_OS2_SIGNATURE_LE == VXDHeader.e32_magic)
                    {
                        char szBuf[1024];
                        ::SetFilePointer(hFile, VXDHeader.e32_winresoff, NULL, FILE_BEGIN);
                        ::ReadFile(hFile, szBuf, VXDHeader.e32_winreslen, &dwRead, NULL);
                        if (dwRead == VXDHeader.e32_winreslen)
                        {
                            unsigned i = 0;
                            for (; i < dwRead; i++)
                            {
                                if (stricmp(szBuf + i, "FILEVERSION") == 0)
                                {
                                    char *pszVersion = szBuf + i + 11;   // 2001, 2, 23, 27
                                    while (*pszVersion == 0)
                                        pszVersion++;
                                    
                                    int j = 0;
                                    while (*pszVersion)
                                    {
                                        if (*pszVersion == ',')
                                            szVersion[j++] = '.';
                                        else if (*pszVersion != ' ')
                                            szVersion[j++] = *pszVersion;

                                        pszVersion++;

                                    }
                                    szVersion[j] = '\0';
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        ::CloseHandle(hFile);
   }

    return nResult;
}


int GetFileTimeVersion(char szFileName[], char szVersion[], int nVersionLen)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE Handle;
    SYSTEMTIME SystemTime;
    char szTemp[10];

    Handle = ::FindFirstFile(szFileName, &FindFileData);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        ::FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &SystemTime);
        sprintf(szVersion, "%d", SystemTime.wYear);
        strcat(szVersion, ".");
        
        sprintf(szTemp, "%d", SystemTime.wMonth);
        strcat(szVersion, szTemp);
        strcat(szVersion, ".");

        sprintf(szTemp, "%d", SystemTime.wDay);
        strcat(szVersion, szTemp);

        ::FindClose(Handle);
    }
    
    return true;
}

int GetFileTime(char szFileName[], char szVersion[], int nVersionLen)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE Handle;
    SYSTEMTIME SystemTime;
    char szTemp[10];
    
    Handle = ::FindFirstFile(szFileName, &FindFileData);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        ::FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &SystemTime);
        sprintf(szVersion, "%d", SystemTime.wYear);
        strcat(szVersion, ".");
        
        sprintf(szTemp, "%d", SystemTime.wMonth);
        strcat(szVersion, szTemp);
        strcat(szVersion, ".");
        
        sprintf(szTemp, "%d", SystemTime.wDay);
        strcat(szVersion, szTemp);
        strcat(szVersion, " ");

        sprintf(szTemp, "%d", SystemTime.wHour);
        strcat(szVersion, szTemp);
        strcat(szVersion, ":");

        sprintf(szTemp, "%d", SystemTime.wMinute);
        strcat(szVersion, szTemp);
        
        
        ::FindClose(Handle);
    }
    
    return true;
}

int GetAllVersion()
{
    char szSignFileName[MAX_PATH];
    char szFileName[MAX_PATH];
    int  nSignFileNameSize      = 0;
    int  nBaseSignNum           = 0;
    int  nUpdateSignNum         = 0;
    int  nLen                   = 0;
    int  nRetCode               = false;
    int  nResult                = false;
    unsigned uMajorVersion      = 0;
    unsigned uMinorVersion      = 0;
    WIN32_FIND_DATA *pFindData  = NULL;
    HANDLE hFind                = INVALID_HANDLE_VALUE;


    // Get the KAV9X.EXE Version
    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAV9X.EXE");
    GetProgramVersion(szFileName, g_szKAV9XVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAEPlat.DLL");
    GetProgramVersion(szFileName, g_szKAEPlatVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAVDX.EXE");
    GetProgramVersion(szFileName, g_szKAVDXVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAVEXT.DLL");
    GetProgramVersion(szFileName, g_szKAVEXTVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "Rescue.EXE");
    GetProgramVersion(szFileName, g_szRescueVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "Update.DAT");
    GetProgramVersion(szFileName, g_szUpdateVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KWatch.SYS");
    GetProgramVersion(szFileName, g_szKWatchSYSVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KWatch.VXD");
    GetVersionByRes(szFileName, g_szKWatchVXDVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAVKRNL.VXD");
    GetVersionByRes(szFileName, g_szKAVKRNLVersion, g_cnVersionLen);


    
    // Get KAVXXXXX.DAT Version
    strcpy(szFileName, "KAV00000.DAT");    
    strcpy(szSignFileName, g_szProgramPath);
    strcat(szSignFileName, "KAV?????.DAT");

    // Find late Base sign data file
    pFindData = new WIN32_FIND_DATA;
    if (pFindData == NULL)
        goto Exit0;

    hFind = ::FindFirstFile(szSignFileName, pFindData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (stricmp(pFindData->cFileName, szFileName) > 0)
                strcpy(szFileName, pFindData->cFileName);
       
        } while (::FindNextFile(hFind, pFindData));
    
        ::FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }
    
    strcpy(szSignFileName, g_szProgramPath);
    strcat(szSignFileName, szFileName);
    nSignFileNameSize = strlen(szSignFileName) + 1;

    // Find the update sign data file.
    nBaseSignNum = _StrToInt(szSignFileName + nSignFileNameSize - 10);
    nUpdateSignNum = nBaseSignNum;
    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAV00000.UPD");


    while (true)
    {
        nUpdateSignNum++;

        _IntToStr(nUpdateSignNum, szFileName + nSignFileNameSize - 10, 5);

        nRetCode = FileExists(szFileName);
        if (!nRetCode)
        {
            if ((nUpdateSignNum - 1) == nBaseSignNum)
                _IntToStr(nBaseSignNum, szSignFileName + nSignFileNameSize - 10, 5);
            else 
            {
                _IntToStr(nUpdateSignNum - 1, szFileName + nSignFileNameSize - 10, 5);
                strcpy(szSignFileName, szFileName);
            }
            break;
        }
    }

    nRetCode = GetSignFileVersion(szSignFileName, g_szSignDataVersion, g_cnVersionLen);

    // Get KAEngine.Dat version
    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAEngine.DAT");
    nRetCode = GetEngineFileVersion(szFileName, g_szEngineVersion, g_cnVersionLen);

    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "KAExtend.DAT");
    nRetCode = GetEngineFileVersion(szFileName, g_szKAExtendVersion, g_cnVersionLen);
    strcpy(szFileName, g_szProgramPath);
    strcat(szFileName, "DrWeb32.DAT");
    nRetCode = GetDrWebFileVersion(szFileName, g_szDrWebVersion, g_cnVersionLen);

    nResult = true;
Exit0:
    return nResult;
}



GetVersionFunction *g_GetFileVersionTable[g_cnFileVersionTypeCount] = 
{
    GetProgramVersion,          // 0
    GetEngineFileVersion,       // 1
    GetSignFileVersion,         // 2    
    GetVersionByRes,            // 3
    GetFileTimeVersion          // 4
};
