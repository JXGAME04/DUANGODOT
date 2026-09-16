#include "Stdafx.h"
#include "Data.h"
#include "Stdio.h"
#include "CRC32.h"
#include "GetFileVersion.h"
#include "conio.h"


const int g_cnVersionCount = 12;
const char g_cszIDUBACRCKey[g_cnVersionCount][20] = 
{
    "StandardCRC",
    "OEMCRC",
    "ZhongKeCRC",
    "HaiXingCRC",
    "IBMCRC",
    "DUBAIICRC",
    "TelecomCRC",
    "DUBA2OEMCRC",
    "DUBA2DIAMONDCRC",
    "DUBA2JingYingCRC",
    "JINGYINGCRC",
    "DUBA2ENGLISHCRC"
};

const char g_cszIDUBACRCValue[g_cnVersionCount][20] = 
{
    "460632014",
    "2254521182",
    "4092532557",
    "1626721120",
    "3140012217",
    "190338143",
    "4058599001",
    "690658592",
    "2272706378",
    "1199582562",
    "3336904998",
    "3810146755"
};

int inline IsNumeric(const char cszStr[], int nLen)
{
    for (int i = 0; i < nLen; i++)
    {
        if (!
            ((cszStr[i] >= '0') && (cszStr[i] <= '9'))
        )
            return false;
    }

    return true;
}


int IsUpdFileName(const char cszFileName[])
{
    int nResult         = false;
    int nFileNameLen    = strlen(cszFileName);

    if (nFileNameLen != 12)
        goto Exit0;

    if (strnicmp(cszFileName, "KAV", 3) != 0)
        goto Exit0;
    
    if (!IsNumeric(cszFileName + 3, 5))
        goto Exit0;
    
    if (strnicmp(cszFileName + nFileNameLen - 4, ".UPD", 4) == 0)
        nResult = true;

Exit0:
    return nResult;    
}

int IsSignFileName(const char cszFileName[])
{
    int nResult         = false;
    int nFileNameLen    = strlen(cszFileName);

    if (nFileNameLen != 12)
        goto Exit0;
    
    if (strnicmp(cszFileName, "KAV", 3) != 0)
        goto Exit0;
    
    if (!IsNumeric(cszFileName + 3, 5))
        goto Exit0;

    if (strnicmp(cszFileName + nFileNameLen - 4, ".DAT", 4) == 0)
        nResult = true;

Exit0:
    return nResult;    
}

UINT GetCRC32(const char cszFile[])
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    UINT uRetCode;
    DWORD dwRead;
    int nLen;
    DWORD dwSize;

    
    hFile = ::CreateFile(cszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = ::GetFileSize(hFile, (unsigned long *)&nLen);

        if (dwSize > 0)
        {
            char *pvBuffer = new char[dwSize + 1];
            if (pvBuffer != NULL)
            {
                ::ReadFile(hFile, pvBuffer, dwSize, &dwRead, NULL);
                
                uRetCode = CRC32(0, pvBuffer, dwSize);

                delete pvBuffer;
            }
        }

        ::CloseHandle(hFile);
    }

    return uRetCode;
}
    

int GetFileSize(const char cszFileName[])
{

    HANDLE hFile = INVALID_HANDLE_VALUE;
    int nLen;
    DWORD dwSize;
    
    
    hFile = ::CreateFile(cszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = ::GetFileSize(hFile, (unsigned long *)&nLen);
        
        ::CloseHandle(hFile);
    }
    
    return dwSize;
}

///////////////////////////////////////////////////////////////////////
// Get files information under the special directory
///////////////////////////////////////////////////////////////////////
int GetFilesInfo(
    const char cszPath[], const char cszSubPath[], 
    const char cszVersion[], const char cszRemotePath[],    
    const char cszDestFileName[],
    const char cszCommentFileName[],
    const char cszInfoFileName[],
    HANDLE fHandle
)
{
    char szPath[MAX_PATH];
    char szTemp[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE Handle;
    char szFileName[MAX_PATH];
    char szLocalPath[MAX_PATH]  = {0};
    char szRemotePath[MAX_PATH] = {0};
    char szFileSize[20]         = {0};
    char szDescription[MAX_PATH]= {0};
    char szItem[MAX_PATH]       = {0};
    char szFileVersion[20]      = {0};
    int  i;
    char szCommentName[MAX_PATH]= {0};
    DWORD dwRetCode             = 0;
    int nBufferLen              = 1024;
    char szBuffer[1024]         = {0};
    char szFileTime[20]         = {0};
    char szFileInfo[200]        = {0};
    DWORD dwWritten             = 0;
    char szCRC[20]              = {0};
    int nCloseHandleFlag        = false;
    int nVersionType            = 0;
    char szValue[MAX_PATH]      = {0};
                 
    strcpy(szPath, cszPath);
    if (szPath[strlen(szPath) - 1] != '\\')
        strcat(szPath, "\\");
    
    strcat(szPath, cszSubPath);
    if (szPath[strlen(szPath) - 1] != '\\')
        strcat(szPath, "\\");

    strcpy(szTemp, szPath);
    strcat(szTemp, "*.*");

    if (fHandle == INVALID_HANDLE_VALUE)
    {
        fHandle = ::CreateFile(cszInfoFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fHandle == INVALID_HANDLE_VALUE)
            return false;
        nCloseHandleFlag = true;
    }

    Handle = ::FindFirstFile(szTemp, &FindFileData);
    if (Handle != INVALID_HANDLE_VALUE)
    {
        do
        {   
            strcpy(szFileName, szPath);
            strcat(szFileName, FindFileData.cFileName);
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if ((strcmp(FindFileData.cFileName, ".") != 0) && (strcmp(FindFileData.cFileName, "..") != 0))
                {
                    char szSubPath[MAX_PATH];
                    if (*cszSubPath == '\0')
                        strcpy(szSubPath, FindFileData.cFileName);
                    else
                        sprintf(szSubPath, "%s\\%s", cszSubPath, FindFileData.cFileName);
                    GetFilesInfo(cszPath, szSubPath, cszVersion, 
                        cszRemotePath, cszDestFileName, 
                        cszCommentFileName, cszInfoFileName, fHandle
                    );
                }
                continue;
            }                   

            // Get file version
            nVersionType = ::GetPrivateProfileInt(FindFileData.cFileName, "VersionType", 0, cszCommentFileName);
            if ((nVersionType >= 0) && (nVersionType < g_cnFileVersionTypeCount))
                g_GetFileVersionTable[nVersionType](szFileName, szFileVersion, 20);
            else
            {
				printf("发现%s不能取版本\n", FindFileData.cFileName);
	
				char ch = _getch();
				
				return false;
			}


            // Get file size
            // szFileSize
            int nFileSize = GetFileSize(szFileName);
            sprintf(szFileSize, "%d", nFileSize);

            strcpy(szItem, FindFileData.cFileName);
            // write index.txt
            ::WritePrivateProfileString(szItem, "FileName", FindFileData.cFileName, cszDestFileName);
            ::WritePrivateProfileString(szItem, "Version", szFileVersion, cszDestFileName); 
            ::WritePrivateProfileString(szItem, "FileSize", szFileSize, cszDestFileName);
            
            *szCRC = '\0';             
            UINT uFileCRC = GetCRC32(szFileName);
            sprintf(szCRC, "%u", uFileCRC);
            ::WritePrivateProfileString(szItem, "CRC", szCRC, cszDestFileName);

            strcpy(szRemotePath, cszRemotePath);
            if (*cszSubPath != '\0')
            {
                strcat(szRemotePath, cszSubPath);
                strcat(szRemotePath, "/");
                for (i = 0; i < (signed)strlen(szRemotePath); i++)
                {
                    if (szRemotePath[i] == '\\')
                        szRemotePath[i] = '/';
                }
            }
            ::WritePrivateProfileString(szItem, "RemotePath", szRemotePath, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "LocalPath", "%PROGRAMDIR%", szLocalPath, MAX_PATH, cszCommentFileName);            
            if (*cszSubPath != '\0')
            {
                char *pszPos;
                pszPos = szLocalPath + strlen(szLocalPath) - 1;
                sprintf(pszPos, "\\%s", cszSubPath);
                strcat(szLocalPath, "%");
            }              
            ::WritePrivateProfileString(szItem, "LocalPath", szLocalPath, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "UpdateFileFlag", "0", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "UpdateFileFlag", szValue, cszDestFileName);

			::GetPrivateProfileString(FindFileData.cFileName, "UpdateFileMethod", "Copy", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "UpdateFileMethod", szValue, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "Reboot", "FALSE", szValue, MAX_PATH, cszCommentFileName);            
            ::WritePrivateProfileString(szItem, "Reboot", szValue, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "CheckVersionMethod", "0", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "CheckVersionMethod", szValue, cszDestFileName);
            
            ::GetPrivateProfileString(FindFileData.cFileName, "SharedNeedReboot", "0", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "SharedNeedReboot", szValue, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "Description", "", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "Description", szValue, cszDestFileName);               
            
            ::GetPrivateProfileString(FindFileData.cFileName, "UpdateRelative", "0", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "UpdateRelative", szValue, cszDestFileName);

            ::GetPrivateProfileString(FindFileData.cFileName, "Belong", "0", szValue, MAX_PATH, cszCommentFileName);
            ::WritePrivateProfileString(szItem, "Belong", szValue, cszDestFileName);
           
            for (i = 1; ; i++)
            {
                sprintf(szCommentName, "Comment%d", i);
                dwRetCode = ::GetPrivateProfileString(szItem, szCommentName, "", szBuffer, nBufferLen + 1, cszCommentFileName); 
                if (*szBuffer == '\0')
                    break;
                
                ::WritePrivateProfileString(szItem, szCommentName, szBuffer, cszDestFileName);
            }
            ::GetPrivateProfileString(FindFileData.cFileName, "NeedDownload", "1", szValue, MAX_PATH, cszCommentFileName);            
            ::WritePrivateProfileString(szItem, "NeedDownload", szValue, cszDestFileName);

            // 
            GetFileTime(szFileName, szFileTime, 20);
            strcpy(szFileInfo, "FileName: ");
            strcat(szFileInfo, FindFileData.cFileName);
            strcat(szFileInfo, "\r\n  FileTime: ");
            strcat(szFileInfo, szFileTime);
            strcat(szFileInfo, "\r\n  FileSize: ");
            strcat(szFileInfo, szFileSize);
            strcat(szFileInfo, "\r\n  CRC32: ");
            strcat(szFileInfo, szCRC);
            strcat(szFileInfo, "\r\n  FileVersion: ");
            strcat(szFileInfo, szFileVersion);
            strcat(szFileInfo, "\r\n");
               
            ::WriteFile(fHandle, szFileInfo, strlen(szFileInfo), &dwWritten, NULL);

        }
        while(::FindNextFile(Handle, &FindFileData));

        ::FindClose(Handle);
    }
    
    if (nCloseHandleFlag)
    {
        ::CloseHandle(fHandle);
        fHandle = INVALID_HANDLE_VALUE;         
    }

    return true;
}

int CopySection(const char cszSource[], const char cszDest[], const char cszSection[])
{
    DWORD   dwRetCode   = 1024;
    int     nBufferSize = 0;
    char    *pszBuffer  = NULL;
    int     nResult     = false;
    int     nRetCode    = 0;

    // Get  the section
    while(true)
    {
        nBufferSize += dwRetCode;
        if (pszBuffer)
        {
            delete []pszBuffer;
            pszBuffer = NULL;
        }

        pszBuffer = new char[nBufferSize];
        if (!pszBuffer)
            goto Exit0;
    
        dwRetCode = ::GetPrivateProfileSection(cszSection, pszBuffer, nBufferSize, cszSource);
        if (dwRetCode != (DWORD)(nBufferSize - 2))
            break;
    }

    // Write the section
    nRetCode = ::WritePrivateProfileSection(cszSection, pszBuffer, cszDest);
    if (!nRetCode)
        goto Exit0;

    nResult = true;
Exit0:
    return nResult;
}


///////////////////////////////////////////////////////////////////////
//     cszSourcePath    :  source file path
//     cszDestFileName  :  Index.txt Full path name
//     nVersion         :  Version
///////////////////////////////////////////////////////////////////////

int IndexGen(
    const char cszSourcePath[], 
    const char cszDestFileName[], 
    int nVersion, 
    const  char cszRemotePath[],
    const char cszCommonInfoFileName[],
    const char cszFileCommentFileName[],
    const char cszInfoFileName[]
)
{
    char szFileName[MAX_PATH]   = {0};
    char szRemotePath[MAX_PATH] = {0};
    char szVersion[1024]        = {0};    
    DWORD dwRetCode             = 0;
    int nBufferLen              = 1024;
    char szBuffer[1024]         = {0};
    int n;
    HANDLE fHandle = INVALID_HANDLE_VALUE;    
    int nFlag = false;
    char szKeyName[MAX_PATH] = {0};
    int nValue;
    char szValue[MAX_PATH]  = {0};

    ::DeleteFile(cszDestFileName);

    // Get RemotePath
    if (cszRemotePath[0] == '\0')
    {
        strcpy(szRemotePath, "Standard/");
    }
    else
    {
        strcpy(szRemotePath, cszRemotePath);
        if (szRemotePath[strlen(szRemotePath) - 1] != '/')
        {
            if (szRemotePath[strlen(szRemotePath) - 1] == '\\')    
                szRemotePath[strlen(szRemotePath) - 1] = '/';     
            else
                strcat(szRemotePath, "/");
        }    
    }
    
    // Get Version
    for (n = 0; ; n++)
    {
        sprintf(szKeyName, "Version%d", n);
        nValue = ::GetPrivateProfileInt("Version", szKeyName, 0, cszCommonInfoFileName);
        if (nValue == 0)
            break;

        if ((nValue & nVersion) == 0)
            continue;

        sprintf(szKeyName, "AppVersion%d", n);
        ::GetPrivateProfileString("Version", szKeyName, "", szValue, MAX_PATH, cszCommonInfoFileName);
        if (szValue[0] == '\0')
            continue;

        if (!nFlag)
        {
            strcpy(szVersion, szValue);
            nFlag = true;
        }
        else
        {
            strcat(szVersion, ",");
            strcat(szVersion, szValue);
        }
    }

    // write the Comment section
    CopySection(cszCommonInfoFileName, cszDestFileName, "COMMENT");
    
    // Write the Install section
    CopySection(cszCommonInfoFileName, cszDestFileName, "Install");
    
    // Write the Version section
    CopySection(cszCommonInfoFileName, cszDestFileName, "Version");
    
    // Write the ExitType section
    CopySection(cszCommonInfoFileName, cszDestFileName, "ExitType");

    CopySection(cszCommonInfoFileName, cszDestFileName, "FileType");
    
    // write the file information
    GetFilesInfo(cszSourcePath, "", szVersion, 
        szRemotePath,
        cszDestFileName, cszFileCommentFileName, cszInfoFileName, fHandle
    );

    return true;
}

