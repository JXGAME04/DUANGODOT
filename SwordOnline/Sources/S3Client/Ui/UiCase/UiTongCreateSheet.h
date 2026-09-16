/*******************************************************************************
File        : UiTongCreateSheet.h
Creator     : Fyt(Fan Zhanpeng)
create data : 08-29-2003(mm-dd-yyyy)
Description : 创建帮会的表单
********************************************************************************/


#if !defined(AFX_KUITONGCREATESHEET_H__7CC8F62F_9A1C_4AE2_A73B_BC945DE5185F__INCLUDED_)
#define AFX_KUITONGCREATESHEET_H__7CC8F62F_9A1C_4AE2_A73B_BC945DE5185F__INCLUDED_

/*---------------------------*/
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*------------------------------------------*/
#include "../elem/wndtext.h"
#include "../elem/wndedit.h"
#include "../elem/wndimage.h"
#include "../elem/wndbutton.h"

class KUiTongCreateSheet : KWndImage
{
public:
	KUiTongCreateSheet();
	virtual ~KUiTongCreateSheet();

	static        KUiTongCreateSheet* OpenWindow(bool bChangeCamp = false);  
	static        KUiTongCreateSheet* GetIfVisible();
	static void   CloseWindow(bool bDestory = TRUE); 
	static void   LoadScheme(const char* pScheme);   

public:

private:
	static        KUiTongCreateSheet *ms_pSelf;

private:
	void			Initialize();               
	virtual int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);
	void			SetChangeCamp(bool bChangeCamp);
private:
	void          AlignmentButtonCheck(NPCCAMP eSide);
	void          OnDone();                   

private:
	KWndText80		m_TextError;                
	KWndText32		m_TxtTongName;
	KWndEdit32		m_EditTongName;             
	KWndButton		m_BtnOrder, m_BtnNatural;   
	KWndButton		m_BtnChaos;                 

    KWndButton		m_BtnDone, m_BtnCancel;     

	int				m_nSelectSide;              
	char			m_szNameNullString[40];     
	char			m_szAlignmentNullString[40];
	char			m_szChangeFaction[48];
	char			m_szImageChangeCamp[64];
	bool			m_bChangeCamp;
};


#endif // !defined(AFX_KUITONGCREATESHEET_H__7CC8F62F_9A1C_4AE2_A73B_BC945DE5185F__INCLUDED_)
