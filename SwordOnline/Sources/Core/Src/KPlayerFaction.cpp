//---------------------------------------------------------------------------
// Sword3 Engine (c) 2002 by Kingsoft
//
// File:	KPlayerFaction.cpp
// Date:	2002.09.26
// Code:	±ﬂ≥«¿À◊”
// Desc:	PlayerFaction Class
//---------------------------------------------------------------------------

#include	"KCore.h"
#include	"MyAssert.H"
#include	"GameDataDef.h"
#include	"KFaction.h"
#include	"KPlayerFaction.h"

#define		FACTION_NEW			"S¨ nhÀp giang hÂ"
#define		FACTION_OLD			"Giang hÂ hi÷p kh∏ch"

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫ππ‘Ï∫Ø ˝
//---------------------------------------------------------------------------
KPlayerFaction::KPlayerFaction()
{
	Release();
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫«Âø’
//---------------------------------------------------------------------------
void	KPlayerFaction::Release()
{
	m_nCurFaction = -1;
	m_nFirstAddFaction = -1;
	m_nAddTimes = 0;
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫∏˘æ›ÕÊº“µƒŒÂ–– Ù–‘»∑∂®ÕÊº“√≈≈… ˝æ›
//---------------------------------------------------------------------------
//void	KPlayerFaction::SetSeries(int nSeries)
//{
//	return;
/*
	if (nSeries < series_metal || nSeries >= series_num)
		return;
	int		nID;
	for (int i = 0; i < FACTIONS_PRR_SERIES; i++)
	{
		nID = g_Faction.GetID(nSeries, i);
		_ASSERT(nID >= 0);
		if (m_sSkillOpen[i].m_nID != nID)
		{
			m_sSkillOpen[i].m_nID = nID;
			m_sSkillOpen[i].m_nOpenLevel = 0;
		}
	}
*/
//}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫º”»Î√≈≈…
//---------------------------------------------------------------------------
BOOL	KPlayerFaction::AddFaction(char *lpszFactionName)
{
	return AddFaction(g_Faction.GetID(lpszFactionName));
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫º”»Î√≈≈…
//---------------------------------------------------------------------------
BOOL	KPlayerFaction::AddFaction(int nFactionID)
{
	if (nFactionID < 0 || nFactionID >= MAX_FACTION)
		return FALSE;

	m_nCurFaction = nFactionID;
	m_nAddTimes++;
	if (m_nAddTimes == 1)
		m_nFirstAddFaction = nFactionID;

	return TRUE;
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫¿Îø™µ±«∞√≈≈…
//---------------------------------------------------------------------------
void	KPlayerFaction::LeaveFaction()
{
	m_nCurFaction = -1;
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫ø™∑≈µ±«∞√≈≈…ƒ≥∏ˆµ»º∂µƒººƒ‹
//---------------------------------------------------------------------------
BOOL	KPlayerFaction::OpenCurSkillLevel(int nLevel, KSkillList *pSkillList)
{
	return TRUE;
/*
	if (nLevel < 0 || nLevel >= FACTION_SKILL_LEVEL || !pSkillList)
		return FALSE;
	if (m_nCurFaction < 0)
		return FALSE;

	int		i, j;

	// …Ë∂®µ±«∞ø™∑≈µ»º∂
	for (i = 0; i < FACTIONS_PRR_SERIES; i++)
	{
		if (m_nCurFaction == m_sSkillOpen[i].m_nID)
		{
			m_sSkillOpen[i].m_nOpenLevel = nLevel;
			break;
		}
	}

	// …Ë∂®ÕÊº“ººƒ‹
	for (i = 0; i <= nLevel; i++)
	{
		for (j = 0; j < FACTION_SKILLS_PER_LEVEL; j++)
		{
			if (g_Faction.m_sAttribute[m_nCurFaction].m_nSkill[i][j] <= 0)
				continue;
			pSkillList->Add(g_Faction.m_sAttribute[m_nCurFaction].m_nSkill[i][j], 0);
		}
	}

	return TRUE;
*/
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫ªÒµ√µ±«∞√≈≈…’Û”™
//---------------------------------------------------------------------------
int		KPlayerFaction::GetGurFactionCamp()
{
	if (m_nCurFaction < 0)
	{
		if (m_nAddTimes == 0)
			return camp_begin;
		else
			return camp_free;
	}
	else
	{
		if (g_Faction.GetCamp(m_nCurFaction) >= 0)
			return g_Faction.GetCamp(m_nCurFaction);
		else
			return camp_begin;
	}
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫ªÒµ√µ±«∞√≈≈…
//---------------------------------------------------------------------------
int		KPlayerFaction::GetCurFactionNo()
{
	return m_nCurFaction;
}

//---------------------------------------------------------------------------
//	π¶ƒ‹£∫ªÒµ√µ±«∞√≈≈…
//---------------------------------------------------------------------------
void	KPlayerFaction::GetCurFactionName(char *lpszGetName)
{
	if (!lpszGetName)
		return;

	if (this->m_nCurFaction == -1)
	{
		//if (this->m_nAddTimes == 0)
		//{
			lpszGetName[0] = 0;
		//}
		//else
		//{
		//	strcpy(lpszGetName, FACTION_NEW);
		//}
	}
	else
	{
		strcpy(lpszGetName, g_Faction.m_sAttribute[m_nCurFaction].m_szName);
	}

	return;
}



