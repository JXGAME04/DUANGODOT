//---------------------------------------------------------------------------
// File:	KFaction.cpp
// Editor:	Guve
// Desc:	Faction Class
//---------------------------------------------------------------------------

#include	"KCore.h"
//#include	"MyAssert.h"
#include	"KIniFile.h"
#include	"KSkills.h"
#include	"KFaction.h"
#include	"CoreUseNameDef.h"


KFaction	g_Faction;


//---------------------------------------------------------------------------
//	khëi t¹o th«ng tin m«n ph¸i
//---------------------------------------------------------------------------
BOOL	KFaction::Init()
{
	KIniFile	Ini;
	char		szSection[80], szBuffer[32];
	char		szSeries[series_num][16] = {"S_GOLD", "S_WOOD", "S_WATER", "S_FIRE", "S_EARTH"};
	char		szCamp[camp_num][16] = {"C_BEGIN", "C_JUSTICE", "C_EVIL", "C_BALANCE", "C_FREE", "C_ANIMAL", "C_EVENT"};
	int			i, j;

	for (i = 0; i < MAX_FACTION; i++)
	{
		m_sAttribute[i].m_nIndex = i;
		m_sAttribute[i].m_nSeries = series_metal;
		m_sAttribute[i].m_nCamp = camp_justice;
		m_sAttribute[i].m_szCodeName[0] = 0;
		m_sAttribute[i].m_szName[0] = 0;
	}

	if ( !Ini.Load(FACTION_FILE) )
		return FALSE;

	// m«n ph¸i
	for (i = 0; i < MAX_FACTION; i++)
	{
		sprintf(szSection, "%d", i);
		Ini.GetString(szSection, "Series", "", szBuffer, sizeof(szBuffer));
		// ngò hµnh
		for (j = 0; j < series_num; ++j)
		{
			if (!strcmp(szBuffer, szSeries[j]))
			{
				m_sAttribute[i].m_nSeries = j;
				break;
			}
		}
		_ASSERT(j < series_num);
		Ini.GetString(szSection, "Name", "", m_sAttribute[i].m_szCodeName, sizeof(m_sAttribute[i].m_szCodeName));
		Ini.GetString(szSection, "ShowName", "", m_sAttribute[i].m_szName, sizeof(m_sAttribute[i].m_szName));
		Ini.GetString(szSection, "Camp", "", szBuffer, sizeof(szBuffer));
		for (j = 0; j < camp_num; j++)
		{
			if (strcmp(szBuffer, szCamp[j]) == 0)
			{
				m_sAttribute[i].m_nCamp = j;
				break;
			}
		}
		_ASSERT(j < camp_num);
	}

	return TRUE;
}

int	KFaction::GetID(char *lpszName)
{
	if ( !lpszName || !lpszName[0])
		return -1;
	for (int i = 0; i < MAX_FACTION; ++i)
	{
		if (strcmpi(lpszName, m_sAttribute[i].m_szName) == 0)
			return i;
	}
	return -1;
}

int	KFaction::GetIDByCodeName(char *lpszName)
{
	if ( !lpszName || !lpszName[0])
		return -1;
	for (int i = 0; i < MAX_FACTION; ++i)
	{
		if (strcmpi(lpszName, m_sAttribute[i].m_szCodeName) == 0)
			return i;
	}
	return -1;
}

int	KFaction::GetCamp(int nFactionID)
{
	if (nFactionID < 0 || nFactionID >= MAX_FACTION)
		return -1;
	return m_sAttribute[nFactionID].m_nCamp;
}
