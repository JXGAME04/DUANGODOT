//---------------------------------------------------------------------------
// File:	KFaction.cpp
// Editor:	Guve
// Desc:	Faction Class
//---------------------------------------------------------------------------

#ifndef KFACTION_H
#define KFACTION_H

#include "GameDataDef.h"

//#define		FACTIONS_PRR_SERIES				2			// ÎåÐÐÃ¿¸öÏµµÄÃÅÅÉÊý
#define		MAX_FACTION						13	//(FACTIONS_PRR_SERIES * series_num)	// ×ÜµÄÃÅÅÉÊý

class KFaction
{
public:
	struct	SFactionAttirbute
	{
		int		m_nIndex;					// thø tù
		int		m_nSeries;					// ngò hµnh
		int		m_nCamp;					// phe
		char	m_szCodeName[16];			// tªn trong script
		char	m_szName[32];				// tªn tiÕng ViÖt
	} m_sAttribute[MAX_FACTION];			// m¶ng struct toµn bé th«ng tin cña tÊt c¶ ph¸i

public:
	BOOL		Init();								// khëi t¹o
	int			GetID(char *lpszName);				// lÊy id m«n ph¸i by tªn tiÕng ViÖt
	int			GetIDByCodeName(char *lpszName);	// lÊy id m«n ph¸i by tªn code script
	int			GetCamp(int nFactionID);			// lÊy phe 1 2 3
};

extern	KFaction	g_Faction;
#endif
