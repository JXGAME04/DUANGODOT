#ifndef GET_FILE_VERSION_H
#define GET_FILE_VERSION_H

#include "Windows.h"


typedef struct tagKAVEngineFileHeader
{
	char  cszDescription[48];
    DWORD dwSizeOfHeader;		
    DWORD dwMajorVersion;		//Major Version
    DWORD dwMinorVersion;		//Minor Version
    DWORD dwOrignMajorVersion;  // Orign PE File Major Version
    DWORD dwOrignMinorVersion;  // Orign PE File Minor Version
    DWORD dwOrignLength;
    DWORD dwCompressFlag;       // defKSCOMPRESS_FLAG
    DWORD dwSizePacked;         // Use for Local Second File Position
    DWORD dwCheckSum;
    char cszReserve[64];
}KAVEngineFileHeader;

typedef struct tagKAE_KAVSIGNDATA_ITEM
{
	DWORD dwPos;	// Current DataItem Pos in Data Area
	DWORD dwLen;	// Length of Data Item
} KAE_KAVSIGNDATA_ITEM;

#define defMAXSIGNDATAITEM          0x20

typedef struct tagKAVSignFileHeader
{
    char  cszDescription[0x50];		    // "Kingsoft AntiVirus Database.   Copyright (c) 1999, 2000 Kingsoft AntiVirus Lab.\x1A"
    DWORD dwVersion;					// dat file version
    DWORD dwDateStamp;				    // Update Date, High 16 bit is year, Low 16 bit is Month(High 8 bit),Day(Low 8 bit)
    DWORD dwProcessVirusNum;			// Number of virus can be process
    WORD  wPackMethod;				    // 0: data is not pack 1: pack by Aplib
    WORD  wEncryptMethod;			    // 0: data is not Encrypt
    DWORD dwCheckSum;				    // CheckSum value for data, CRC32
    DWORD dwDataAreaEntry;			    // Data Area Entry offset in file
    DWORD dwDataAreaLen;				// Total Length of Data Area
    DWORD dwSignDataItemCount;
    KAE_KAVSIGNDATA_ITEM SignDataItem[defMAXSIGNDATAITEM];
} KAVSIGNFILEHEADER;



int FileExists(LPCTSTR lpszFileName);
int GetFileVersion(
    const char cszFileName[], 
    DWORD *pdwProductVersionMS, 
    DWORD *pdwProductVersionLS
);
int GetSignFileVersion(
    const char cszFileName[], 
    DWORD *pdwMajorVersion, 
    DWORD *pdwMinorVersion
);
int GetEngineFileVersion(
    const char cszFileName[], 
    DWORD *pdwMajorVersion, 
    DWORD *pdwMinorVersion
);

int GetProgramVersion(char szFileName[], char szVersion[], int nVersionLen);
int GetEngineFileVersion(char szFileName[], char szVersion[], int nVersionLen);
int GetDrWebFileVersion(char szFileName[], char szVersion[], int nVersionLen);
int GetSignFileVersion(char szFileName[], char szVersion[], int nVersionLen);
int GetVersionByRes(char szFileName[], char szVersion[], int nVersionLen);
int GetFileTimeVersion(char szFileName[], char szVersion[], int nVersionLen);
int GetFileTime(char szFileName[], char szVersion[], int nVersionLen);

int GetAllVersion();



static int inline _IntToStr(int nNum, char *pszStr, int nDigitalNum)
{
	while (nDigitalNum)
	{
        pszStr[nDigitalNum - 1] = '0' + (nNum % 10);

        nNum /= 10;

        nDigitalNum--;
	}

	return true;
}

int inline _StrToInt(char *pszStr)
{
    int i;
    int nLen;
    int nNum = 0;

    nLen = strlen(pszStr);

    for (i = 0; i < nLen; i++)
    {
        if (
            (pszStr[i] >= '0') &&
            (pszStr[i] <= '9')
        )
        {
            nNum = nNum * 10 + (pszStr[i] - '0');
        }
        else
            break;
    }

    return nNum;
}

int inline _IntToStr(int nNum, int nStrSize, char *pszStr)
{
    nStrSize--; // use for last '\0' char

    int i = 0;
    while (nStrSize > 0)
	{
        pszStr[i] = '0' + (short)(nNum % 10);

        nNum /= 10;

        nStrSize--;
        
        i++;

        if (nNum == 0)
            break;
	}
    pszStr[i] = '\0';

    int j = 0;
    while (j < (i / 2))
    {
        char ch = pszStr[j];
        pszStr[j] = pszStr[i - j - 1];
        pszStr[i - j - 1] = ch;

        j++;
    }


	return i;
}
                          
#endif  //GET_FILE_VERSION_H
