/*******************************************************************************
File        : UiTongAssignBox.h
********************************************************************************/

#if !defined(AFX_UITONGASSIGNBOX_H__1D36E55C_C9D4_44AF_8E20_0FF51BDB2BE0__INCLUDED_)
#define AFX_UITONGASSIGNBOX_H__1D36E55C_C9D4_44AF_8E20_0FF51BDB2BE0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../elem/wndbutton.h"
#include "../elem/WndPureTextBtn.h"
#include "../elem/wndimage.h"
#include "../elem/wndtext.h"

class KUiTongAssignBox : public KWndImage
{
public:
	KUiTongAssignBox();
	virtual ~KUiTongAssignBox();

	static        KUiTongAssignBox* OpenWindow(int nCurFigure, const char* pName, KWndWindow* pCaller, UINT uParam);   
	static        KUiTongAssignBox* GetIfVisible(); 
	static void   CloseWindow(bool bDestory = true);
	static void   LoadScheme(const char* pScheme);  
	virtual int   WndProc(unsigned int uMsg, unsigned int uParam, int nParam);

private:
	static        KUiTongAssignBox* ms_pSelf;

private:
	void          Initialize();
	void          OnConfirm();                      

private:
	KWndText32				m_TitlePlayerName;
	KWndText32				m_TargetName;
	KWndText32				m_TitlePositionName;
	KWndPureTextBtn			m_BtnMember;
	KWndPureTextBtn			m_BtnCaptain;
	KWndPureTextBtn			m_BtnElder;
	KWndButton				m_BtnConfirm;
	KWndButton				m_BtnCancel;
	KWndText80				m_TextError;
	KWndWindow				*m_pMain;

private:
	int						m_nCurrentFigure, m_nSelectFigure;
	UINT					m_uParam;
};

#endif // !defined(AFX_UITONGASSIGNBOX_H__1D36E55C_C9D4_44AF_8E20_0FF51BDB2BE0__INCLUDED_)
