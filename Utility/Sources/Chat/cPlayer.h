// cPlayer.h: interface for the cPlayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLAYER_H__22C2271C_675C_4C28_B4BB_6581EDA456F3__INCLUDED_)
#define AFX_CPLAYER_H__22C2271C_675C_4C28_B4BB_6581EDA456F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class iServerLink;

class cPlayer  
{
public:
	cPlayer();
	virtual ~cPlayer();
	void BindLink(iServerLink* pLink);	
	iServerLink* m_pLink;
	iServerLink* GetLink(){return m_pLink;}
	int m_nID;
	void SetID(int id){m_nID = id;}
	int GetID(){return m_nID;}

	UINT HandleMessage(stMsg* pMsg,DWORD size);

	char m_szAccount[MAX_ACCOUNT];
	BOOL IsAccount(LPCSTR szAccount){return strcmp(szAccount,m_szAccount) == 0;}
	void SetAccount(LPCSTR szAccount){strcpy(m_szAccount,szAccount);}

	//the player's connect may be lost!
	bool m_bConnect;
	bool IsConnect();
	//bTrust = true after the name and password verify
	bool m_bTrust;
	bool IsTrust(){return m_bTrust;}
	//if bLocked the player can not handle the msg, and then push msg to queue
	//bool m_bLock;
	bool IsLocked();
};

UINT PlayerHandleMessage(stMsg* pMsg,DWORD size,void* p,iServerLink* pLink);

#endif // !defined(AFX_CPLAYER_H__22C2271C_675C_4C28_B4BB_6581EDA456F3__INCLUDED_)
