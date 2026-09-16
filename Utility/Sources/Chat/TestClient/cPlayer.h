// cPlayer.h: interface for the cPlayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLAYER_H__49C17A68_85F3_4744_BB3D_93AAFFD24C06__INCLUDED_)
#define AFX_CPLAYER_H__49C17A68_85F3_4744_BB3D_93AAFFD24C06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cPlayer
{
public:
	cPlayer();
	virtual ~cPlayer();
public:
	UINT HandleMessage(stMsg* pMsg, DWORD msgSize);
	char m_sAccount[MAX_ACCOUNT];
	char m_sPassword[MAX_PASSWORD];
	iClient* m_pLink;
	void BindLink(iClient* p){m_pLink = p;}
	UINT Run();
	void Create(int id){
		sprintf(m_sAccount,"%04d",id);
		sprintf(m_sPassword,"%04d",id);
	};
	int m_nTimer;
	enum GameStatus{
		GameStatus_Invalid,GameStatus_Connect,GameStatus_Verify,GameStatus_Disconnect,
	};
	GameStatus m_eStatus;
};

UINT PlayerHandleMessage(stMsg* pMsg, DWORD msgSize, void* pPlayer,iServerLink* pLink);

#endif // !defined(AFX_CPLAYER_H__49C17A68_85F3_4744_BB3D_93AAFFD24C06__INCLUDED_)
