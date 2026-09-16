/*******************************************************************************
File        : UiTongDistrib.h
********************************************************************************/

#if !defined(AFX_UITONGDISTRIB_H)
#define AFX_UITONGDISTRIB_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../elem/wndbutton.h"
#include "../elem/wndimage.h"
#include "../elem/wndtext.h"
#include "../elem/wndedit.h"

class KUiTongDistrib : public KWndImage
{
public:
	KUiTongDistrib();
	virtual ~KUiTongDistrib();

	static        KUiTongDistrib* OpenWindow(int nUiType, KWndWindow* pCaller, UINT uParam);   
	static        KUiTongDistrib* GetIfVisible(); 
	static void   CloseWindow(bool bDestory = true);
	static void   LoadScheme(const char* pScheme);  
	virtual int   WndProc(unsigned int uMsg, unsigned int uParam, int nParam);

private:
	static        KUiTongDistrib* ms_pSelf;

private:
	void          Initialize();
	void          OnConfirm();                      

private:
	KWndImage				m_ImgTitleMoney, m_ImgTitleOffer;
	KWndText32				m_TxtDirector, m_TxtManager, m_TxtMember;
	KWndEdit32				m_EditDirector, m_EditManager, m_EditMember;
	KWndText32				m_UnitDirector, m_UnitManager, m_UnitMember;
	KWndButton				m_BtnConfirm;
	KWndButton				m_BtnCancel;
	KWndText80				m_TextError;
	KWndWindow				*m_pMain;

private:
	int						m_nUiType;
	UINT					m_uParam;
};

#endif // !defined(AFX_UITONGDISTRIB_H)
