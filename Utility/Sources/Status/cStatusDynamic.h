// cStatusDynamic.h: interface for the cStatusDynamic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSDYNAMIC_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_)
#define AFX_CSTATUSDYNAMIC_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"
class CNpcSettingDlg;

class cStatusDynamic : public cStatus
{
public:
	cStatusDynamic();
	virtual ~cStatusDynamic();

//	BOOL RenameNick(iObj* pPlane,LPCSTR szNewNick);
	BOOL GetDefaultEvent();

	CNpcSettingDlg *m_pSettingDlg;
	BOOL ShowNpcSetting(iObj* pPlane,const CString& strTemplate, const CString& strSection,const CString& strID);
	BOOL ShowBoxSetting(iObj* pPlane,const CString& strTemplate, const CString& strSection,const CString& strID);
	BOOL ShowTrapSetting(iObj* pPlane,const CString& strTemplate, const CString& strSection,const CString& strID);
	BOOL ChangeTrapScript(iObj* pPlane);
	BOOL GetTrapScript(iObj* pPlane, CString &strScriptFile);

	void ObjPropertyExtra(iObj* pPlane);

//virtual event	
	int m_nNpc,m_nBox;
	virtual BOOL Create();
	void SaveID();
	BOOL CreateFromFolder(LPCSTR szFolder);
	void InitEvent();
//	virtual BOOL OnSelectedFile(LPCTSTR szFile,int nFrame);
//	CString m_strCopyDataFile;
//	virtual void CreatePlane(CPoint point,iObj* &pPlane, int nFrame);
//	virtual void SelectPlane(CPoint point,iObj* & pPlane);
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
//	BOOL OnLButtonDblClk(UINT nFlags, CPoint point);
//property
	DynamicEnum GetObjDynamic(iObj* pPlane);
	BOOL SetObjNick(iObj* pPlane,const CString& strNick);
	BOOL SetObjTemplate(iObj* pPlane,const CString& strTemplate,const CString& strSection);
	BOOL GetObjNick(iObj* pPlane,CString& strNick);
	BOOL GetObjTemplate(iObj* pPlane,CString& strTemplate, CString& strSection);
	void ChangeObjProperty(iObj* pPlane);
//dynamic

	CString m_strEvent;
	CString m_strTrapScript;
	CString GetEvent(){return m_strEvent;}
	void SetEvent(LPCSTR sz);
	void OnSelectedSetting(LPCTSTR szFile,stSetting& st);
	int m_nTrapID;
	int GetTrapID(){return m_nTrapID;}
	void SetTrapID(int nTrapID){m_nTrapID = nTrapID;}
//file
//	BOOL GetIniFile(const CString& sEvent,CString& sini);
//	BOOL GetObjIniFile(iObj* p,CString& sini);
	BOOL GetObjScriptFile(iObj* p,CString& sScript);

	virtual BOOL GetOffsetOfPlane(iObj* p,cPoint& pt);

	BOOL m_bModify;
	BOOL IsModify(){return m_bModify;}
	BOOL AfterCreatePlane(iObj* pPlane,const CString& m_strSelectFile,const CString& m_strCopyDataFile);
};

#endif // !defined(AFX_CSTATUSDYNAMIC_H__4016F43B_A6EB_4EE9_A2D3_98234EC11773__INCLUDED_)
