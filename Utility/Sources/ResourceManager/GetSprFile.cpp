#include "stdafx.h"
#include "KEngine.h"
#include "KCore.h"
#include "KNpcResList.h"
#include "KNpcRes.h"
#include "KDebug.h"

KNpcResList g_NpcResList;
								//tab file		kind
BOOL GetSprFile(CString& strSpr,LPCSTR szIni,LPCSTR szSection)
{
	static bool b = true;
	if (b)
	{
		g_NpcResList.Init();
		b = false;
	}
	g_DebugLog("Open TabFile:%s, Section:%s", szIni, szSection);
	CString strIni = szIni;
	if (strIni.Find("Box") >= 0)
	{
		KTabFile TabFile;
		if (!TabFile.Load((LPSTR)szIni)) return FALSE;
		int nRow = TabFile.FindRow((LPSTR)szSection);
		if (nRow < 0 ) return FALSE;
		char ImageFile[100];
		TabFile.GetString(nRow, "ImageName", "", ImageFile, 100);
		if (!ImageFile[0]) return FALSE;
		g_SetFilePath("\\");
		char StrSprFile[200];
		g_GetFullPath(StrSprFile, ImageFile);
		strSpr = StrSprFile;
	}
	else
	{
		
		KTabFile  TabFile;
		char  szResName[200];

		g_DebugLog("Open TabFile:%s, Section:%s", szIni, szSection);
		if (!TabFile.Load((LPSTR)szIni))return FALSE;
		int nResType = 0;
		int nRow = TabFile.FindRow((LPSTR)szSection);
		g_DebugLog("FindRow%d", nRow);
		if (nRow < 0) return FALSE;
		TabFile.GetString(nRow, "NpcResType", "",  szResName, 200);
		g_DebugLog("NecResType %s", szResName);
		if (!szResName[0]) return FALSE;
		
		KNpcRes NpcRes;
		if (!NpcRes.Init(szResName, &g_NpcResList)) return FALSE;
		g_DebugLog("Init Success");
		for (int i  = 0; i < 20 ; i ++)
		g_DebugLog("Image%d %s",i, NpcRes.m_cNpcImage[i].m_szName);
		if (!NpcRes.m_cNpcImage[5].m_szName[0]) 
		{
          NpcRes.SetAction(1);
		g_DebugLog("Can Not Find Any Spr From FightStand,so Change WalkStand Action ,Try To Find Other Spr");
			for (int i  = 0; i < 20 ; i ++)
			g_DebugLog("Image%d %s",i, NpcRes.m_cNpcImage[i].m_szName);
		if (!NpcRes.m_cNpcImage[5].m_szName[0]) return FALSE;
 		}

		g_SetFilePath("\\");
		char StrSprFile[200];
		g_GetFullPath(StrSprFile, NpcRes.m_cNpcImage[5].m_szName);
		strSpr = StrSprFile;
		g_DebugLog("ActImage %s", strSpr);
	}
	return TRUE;
}
