#ifndef S3ClientH
#define	S3ClientH

#include "KEngine.h"
#include "KWin32App.h"
#include "KMp3Music.h"
#include "../../Core/Src/CoreShell.h"
#include "../../Core/Src/ipc_shared.h"
class KInlinePicSink;
class KMyApp : public KWin32App
{
private:
	KMp3Music			m_Music;
	KDirectSound		m_Sound;
	KTimer				m_Timer;
	INT64				m_GameCounter;
	int					m_PaintStep;
	KInlinePicSink*		m_pInlinePicSink;
	
public:
	KMyApp();
	BOOL				GameInit();
	BOOL				GameLoop(bool bForceCoreLoop);
	BOOL				GameExit();
	bool				InitMapping();
	void				ProcIpcCommand();
	void				SendAllCommand();
	void				ExtAutoLoop(const autoData* pApData);
	void				AddTrayIcon(HWND hWnd, LPCSTR tip);
	void				AddTrayIconHide(HWND hWnd, LPCSTR tip);
	void				AppSendInfoToTool(const void * const pData, const size_t &datalength);
	void				ExtAutoLogin(const IPCAutoLogin* pALg);
protected:
	int					HandleInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

struct KClientCallback : public IClientCallback
{
	int CoreDataChanged(unsigned int uDataId, unsigned int uParam, int nParam);
	void ChannelMessageArrival(DWORD nChannelID, char* szSendName, const char* pMsgBuff, unsigned short nMsgLength, bool bSucc);
	void MSNMessageArrival(char* szSourceName, char* szSendName, const char* pMsgBuff, unsigned short nMsgLength, bool bSucc);
	void NotifyChannelID(char* ChannelName, DWORD channelid, BYTE cost);
	void FriendInvite(char* roleName);
	void AddFriend(char* roleName, BYTE answer);	//添加好友到好友组
	void FriendStatus(char* roleName, BYTE state);	//有关系人的状态,找到第一个为止
	void FriendInfo(char* roleName, char* unitName, char* groupName, BYTE state);	//同步关系人数据
	void AddPeople(char* unitName, char* roleName);	//添加关系人到指定组
	void SendDataToTool(const void * const pData, const size_t &datalength);

};

//guve send to auto func
void SendInfoToTool(const void * const pData, const size_t &datalength);
#endif