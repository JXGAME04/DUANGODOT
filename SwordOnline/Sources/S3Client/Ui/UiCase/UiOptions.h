/*****************************************************************************************
//	½çÃæ--Ñ¡Ïî½çÃæ
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-2
------------------------------------------------------------------------------------------
*****************************************************************************************/
#pragma once

#include "../Elem/WndLabeledButton.h"
#include "../Elem/WndScrollBar.h"
#include "../Elem/WndList.h"
//#include "../../Engine/Src/LinkStruct.h"

struct KPopupMenuData;

enum	SWORD_ONLINE_OPTION_INDEX
{
	OPTION_I_START = 0,
	OPTION_I_DYNALIGHT = OPTION_I_START,	//s¸ng tèi
	OPTION_I_WEATHER,						//thêi tiÕt
	OPTION_I_PERSPECTIVE,					//phèi c¶nh
	OPTION_I_EXPAND,	//guve tïy chän më réng
	OPTION_INDEX_COUNT,
};

struct KToggleOptionItem
{
	char	szName[32];	//Ãû³Æ
	bool	bInvalid;	//²»¿ÉÓÃ£¨ÓĞĞ§£©
	short	bEnable;	//´ËÑ¡ÏîÊÇ·ñÑ¡ÖĞ
};

class KUiOptions : protected KWndImage
{
public:
	//----½çÃæÃæ°åÍ³Ò»µÄ½Ó¿Úº¯Êı----
	static KUiOptions*	OpenWindow(KWndWindow* pReturn = NULL);//´ò¿ª´°¿Ú£¬·µ»ØÎ¨Ò»µÄÒ»¸öÀà¶ÔÏóÊµÀı
	static KUiOptions*	GetIfVisible();					//Èç¹û´°¿ÚÕı±»ÏÔÊ¾£¬Ôò·µ»ØÊµÀıÖ¸Õë
	static void			CloseWindow();					//¹Ø±Õ´°¿Ú
	static void			LoadScheme(const char* pScheme);//ÔØÈë½çÃæ·½°¸
	
	static void			LoadSetting(bool bReload, bool bUpdate);

//	void				SetPerspective(int);
//	void				SetDynaLight(int);
	void				SetMusicValue(int);
	void				SetSoundValue(int);
	void				SetBrightness(int);
//	void                SwitchWeather();
	void				ToggleOption(int nIndex);	//ÇĞ»»¿ª¹ØĞÍÑ¡Ïî
	static void			TempHide();
private:
	KUiOptions();
	~KUiOptions() {}
	int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);	//´°¿Úº¯Êı
	void	OnScrollBarPosChanged(KWndWindow* pWnd, int nPos);	//ÏìÓ¦¹ö¶¯Ìõ±»ÍÏ¶¯
//	void	PopupSkinMenu();
	void	CancelMenu();
	void	StoreSetting();
	void	UpdateSettingSet(int eSet, bool bOnlyUpdateUi = false);
	void	Initialize();					// ³õÊ¼»¯
	void    PopupSeleteSetMenu(int nX, int nY);	//µ¯³öÑ¡ÔñÅäÖÃ·½°¸µÄ²Ëµ¥
	void	LoadScheme(KIniFile* pIni);	//ÔØÈë½çÃæ·½°¸
	void	UpdateAllToggleBtn();
	void	UpdateAllStatusImg();
	void	PaintOverChild();
private:
	static KUiOptions* m_pSelf;
private:
	KWndWindow* m_pReturn;

	// °´Å¥
	KWndButton		m_ShortcutKeyBtn;	//´ò¿ª¿ì½İ¼üÉè¶¨½çÃæ
	KWndButton		m_CloseBtn;			//¹Ø±Õ°´Å¥
//	KWndLabeledButton	m_SkinBtn;		//½çÃæ·½°¸°´Å¥

	KWndScrollBar	m_BrightnessScroll;	//ÁÁ¶Èµ÷½Ú»¬¿é
	KWndScrollBar	m_BGMValue;			//ÒôÀÖÒôÁ¿»¬¿é
	KWndScrollBar	m_SoundValue;		//ÒôĞ§ÒôÁ¿»¬¿é
	KPopupMenuData*	m_pSkinMenu;

	KWndLabeledButton m_ShortcutSetView;//e...Õâ¸ö±íÊ¾µ±Ç°ËùÑ¡ÔñµÄ¿ì½İ¼ü·½°¸

	int	m_nBrightness, m_nSoundValue, m_nMusicValue;
	int m_nShortcutSet;

#define	MAX_TOGGLE_BTN_COUNT	4
	//KWndScrollBar		m_Scroll;
	KWndLabeledButton	m_ToggleBtn[MAX_TOGGLE_BTN_COUNT];
	KWndImage		m_StatusImage[MAX_TOGGLE_BTN_COUNT];
	unsigned int	m_uEnableTextColor;		//m_ToggleBtn±êÌâÎÄ×ÖµÄÑÕÉ«
	unsigned int	m_uDisableTextColor;	//m_ToggleBtn±êÌâÎÄ×ÖµÄÑÕÉ«
	unsigned int	m_uInvalidTextColor;	//m_ToggleBtn±êÌâÎÄ×ÖµÄÑÕÉ«
	int				m_nStatusEnableFrame;
	int				m_nStatusDisableFrame;
	int				m_nStatusInvalidFrame;

	KToggleOptionItem	m_ToggleItemList[OPTION_INDEX_COUNT];
	int					m_nFirstControlableIndex;
	int					m_nToggleBtnValidCount;
	//int					m_nToggleItemCount;
};
