//---------------------------------------------------------------------------
// Sword3 Engine (c) 2003 by Kingsoft
//
// File:	KViewItem.cpp
// Date:	2003.07.28
// Code:	±ß³ÇÀË×Ó
// Desc:	KViewItem Class
//---------------------------------------------------------------------------

#include	"KCore.h"

#ifndef _SERVER
#include	"CoreShell.h"
#include	"KItemSet.h"
#include	"KItem.h"
#include	"KViewItem.h"

KViewItem	g_cViewItem;

KViewItem::KViewItem()
{
	Init();
}

void	KViewItem::Init()
{
	m_dwNpcID	= 0;
	m_nLevel	= 1;
	m_szName[0]	= 0;
	memset(this->m_sItem, 0, sizeof(this->m_sItem));
}

void	KViewItem::ApplyViewEquip(DWORD dwNpcID)
{
	VIEW_EQUIP_COMMAND	sView;
	sView.ProtocolType = c2s_viewequip;
	sView.m_dwNpcID = dwNpcID;
	if (g_pClient)
		g_pClient->SendPackToServer(&sView, sizeof(sView));
}

void	KViewItem::DeleteAll()
{
	m_dwNpcID	= 0;
	m_nLevel	= 1;
	m_szName[0]	= 0;

	for (int i = 0; i < itempart_num; i ++)
	{
		if (m_sItem[i] > 0)
			ItemSet.Remove(m_sItem[i]);
		m_sItem[i] = 0;
	}
}

void	KViewItem::GetData(BYTE* pMsg)
{
	if (!pMsg)
		return;
	VIEW_EQUIP_SYNC		*pView = (VIEW_EQUIP_SYNC*)pMsg;
	if(pView->nPart == 0)	//lÇn ®Çu nhËn d÷ liÖu
		DeleteAll();
	
	if (pView->m_sInfo.m_nID)
	{
		int nItemIdx = ItemSet.Add(
		pView->m_sInfo.m_btGenre,
		pView->m_sInfo.m_btSeries,
		pView->m_sInfo.m_btLevel,
		(pView->m_sInfo.m_btGenre==item_genpurple)?pView->m_sInfo.m_Count:0,
		pView->m_sInfo.m_btDetail,
		pView->m_sInfo.m_btParticur,
		pView->m_sInfo.m_btMagicLevel);
		if (nItemIdx > 0)
		{
			Item[nItemIdx].SetID(pView->m_sInfo.m_nID);
			m_sItem[pView->nPart] = nItemIdx;
		}
	}
	
	if(pView->nPart == itempart_num-1)	//lÇn cuèi nhËn d÷ liÖu
	{
		KUiPlayerItem	sPlayer;
		m_dwNpcID = pView->m_dwNpcID;
		sPlayer.uId = m_dwNpcID;
		int	nNpcIdx = NpcSet.SearchID(m_dwNpcID);
		if (nNpcIdx > 0)
		{
			m_nLevel = Npc[nNpcIdx].m_Level;
			strcpy(m_szName, Npc[nNpcIdx].Name);
			sPlayer.nIndex = nNpcIdx;
			strcpy(sPlayer.Name, m_szName);
			sPlayer.nData = Npc[nNpcIdx].GetMenuState();
		}
		else
		{
			sPlayer.nIndex = 0;
			sPlayer.Name[0] = 0;
			sPlayer.nData = 0;
		}
		//më UI
		CoreDataChanged(GDCNI_VIEW_PLAYERITEM, (DWORD)&sPlayer, 0);
	}
}





#endif













