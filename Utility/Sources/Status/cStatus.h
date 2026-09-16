// cStatus.h: interface for the cStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUS_H__4296EB73_8ECC_4B05_8488_3542052BEB25__INCLUDED_)
#define AFX_CSTATUS_H__4296EB73_8ECC_4B05_8488_3542052BEB25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iColor.h"
#include "iStatus.h"
#include "iGraphics.h"

class iGraphics;
class iGround;
class iScreen;
class iCamera;
class iObj;

class iCommandMachine;
class iCommand;
struct stCommand;

enum ObjKindEnum
{
	OK_NORMAL,
	OK_ROAD,
	OK_ROOM,
};

class cStatus  : public iStatusMachine
{
public:
	cStatus();
	virtual ~cStatus();
	virtual void Clear();

//com interface
	ULONG m_ulCount;
	ULONG AddRef() ;
	ULONG Release() ;
	BOOL				SetShow(long l);
	void				ClearShow(long l);

//Status
	static eStatusType	m_eStatus;
	static eStatusType	GetStatus(){return m_eStatus;}
	static void			SetStatus(eStatusType e){ASSERT(e != STATUS_NULL); m_eStatus = e;}

	static CString		m_strSelectFile;
	static int			m_nFrame;
	static LPCSTR		GetSelectFile(){return (LPCSTR)m_strSelectFile;}
	static void			SetSelectFile(LPCSTR s,int n){m_strSelectFile = s;m_nFrame = n;}
	static int			GetSelectFileFrames();
	static BOOL			IsSelected(){return !m_strSelectFile.IsEmpty();}
	static int			GetSelectFileDirections();
	static int			GetFrame(){return m_nFrame;}
	
	static eMaskSetting	FindFileAndGetValue(LPCSTR szFile,LPCSTR szPath,stSetting& set);

	static eStatusType	GetFileType(LPCSTR szFile);

	void				OnFrameChange(int frame){m_nFrame = frame;};

	static CString		m_strNewFile;
	void				SetNewFile(LPCSTR s){m_strNewFile = s;}
	CString				GetNewFile(){return m_strNewFile;}
	void				ClearNewFile(){m_strNewFile.Empty();}

//	static BOOL			GetFileValue(LPCSTR szFile,LPCSTR sKey, CString& sValue);

	static BOOL			Create(iGraphics* p,CWnd* pWnd, iCommandMachine* pCommandMachine);
//////////////////////////////////////////////////////////////////////
//keyboard and mouse
	static CWnd*		m_pWnd;
	static OutputMsgFunc AppOutputMsg;
// Mouse
	static BOOL			GetMousePos(CPoint& pt);
// Keyboard
	static bool			IsKeyDown(UINT vk);
	static bool			IsCtrlDown();
	static bool			IsShiftDown();
	static bool			IsAltDown();
//////////////////////////////////////////////////////////////////////
//CommandMachine
	static iCommandMachine* m_pCommandMachine;
	static iCommandMachine* GetCommandMachine(){return m_pCommandMachine;};
	
	static void				DoCommand(stCommand* pCommand);
	static void				GetCommandMessage(iCommand* p);
	static void				OutputMsg(int nStatus, LPCTSTR strMsg);
	static void				BeginMacroCommand();
	static void				EndMacroCommand();
//////////////////////////////////////////////////////////////////////
	static stSelect			m_stSelect;


//////////////////////////////////////////////////////////////////////
//Graphics
	static iGraphics*	m_pGraphics;
	static iGraphics*	GetGraphics();
	static iGround*		GetGround();
	static iScreen*		GetScreen();
	static iCamera*		GetCamera();
	static iObj*		GetSelect();
	static iObj*		GetMap();
	static iObj*		GetMapSmall();
	BOOL SetUpdateRegionTexture(BOOL b);
	BOOL UpdateRegionTexture();
	cPoint GetCameraFocus();

//////////////////////////////////////////////////////////////////////
//create
	virtual BOOL Create();
	virtual	BOOL Create(iGraphics* p,CWnd* pWnd,OutputMsgFunc func){return FALSE;};
	virtual BOOL OnLostFocus();
	virtual BOOL OnSetFocus(){return FALSE;};
//////////////////////////////////////////////////////////////////////
//input
	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnLButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnRButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL OnTimer(UINT nIDEvent);
	virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL IsIdle();
	virtual BOOL OnCameraMove(CPoint point);
	virtual BOOL Copy();
	virtual BOOL Cut();
	virtual BOOL Paste();

/////////////////////////////////////////////////////////////////////////////
//map
	BOOL		 IsGraphicsShow(long mask);
	BOOL		 ClickOnMap(CPoint ptScreen); //是否点击了小地图
/////////////////////////////////////////////////////////////////////////////
//Document and commands
	static CString	m_strDocument;
	static CString	GetDocumentName(){return m_strDocument;}
	static BOOL		IsDocumentEmpty(){return m_strDocument.IsEmpty();}

	BOOL		 Save();
	virtual BOOL OnNewDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnClose();

	virtual BOOL Undo();
	virtual BOOL Redo();
	virtual BOOL CanUndo();
	virtual BOOL CanRedo();
	virtual BOOL RedoToEnd();
	virtual BOOL UndoToBegin();

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//Settings
	stSetting	m_stSetting;
	void		ClearSetting(){m_stSetting.mask = 0;}
	bool		IsSettingValid(){return m_stSetting.mask != 0;}
	stSetting	GetSetting(){return m_stSetting;};
	void		SetSetting(const stSetting& st);

/////////////////////////////////////////////////////////////////////////////
//onSelected
	virtual	BOOL	OnRightSelectedFile(LPCTSTR lpszPathName, int frame);
	virtual	BOOL	OnRightSelectedCommand(LPCTSTR szFile);
	eMaskSetting	OnRightSelectedSetting(LPCTSTR szFile);

	virtual	BOOL	OnLeftSelectedFile(LPCTSTR lpszPathName, int frame);
	virtual	BOOL	OnLeftSelectedCommand(LPCTSTR szFile);
	eMaskSetting	OnLeftSelectedSetting(LPCTSTR szFile);

	virtual	void	OnSelectedSetting(LPCTSTR szFile);
	virtual	BOOL	OnSelectedFile(LPCTSTR lpszPathName, int frame);

	static eMaskSetting	GetFileSetting(LPCTSTR szFile, stSetting& st);

/////////////////////////////////////////////////////////////////////////////
//utility
	BOOL m_bToggleFullscreen;
	void ToggleFullscreen() ;
//	Grid
	void ToggleGrid();
//	trace
	void ToggleTrace(int e);
	void ToggleDraw(long l);
//help function
	BOOL GetAString(CString& str,LPCSTR szTile);

//brightness
	void AddWindSpeed(cPoint ptSpeed);
	void SetGroundBrightness(int n);
	int GetGroundBrightness();
	void SetGroundColor(cColor cr);
	cColor GetGroundColor();
//	Camera
	BOOL OffsetCameraTo(cPoint pt);
	void CameraOffsetScreen(int x,int y);
	void CameraOffset(float x,float y);
	void CameraScale(float f);

	void ClearSelect();
	void MoveiSelect(int e, CPoint point);
	void MoveiSelect(int e, cPoint point);

	static eMaskSetting WantNameToID(const CString& s);
	static eStatusType StatusNameToID(const CString& s);

//	virtual BOOL GetObjIniFile(iObj* p,CString& sini){return FALSE;};
	static void GetGroundEvent(CString& str);
	static BOOL GetIniFile(const CString& strEvent,long lID,const CString& sDynamic, CString& sini);
	static BOOL cStatus::GetObjIniFile(iObj* p,CString& sini);
//	static BOOL GetIniFile(const CString& strEvent,const CString& sEvent,CString& sini);
//	static BOOL GetObjIniFile(iObj* p,CString& sini);
	static KindEnum GetObjKind(iObj* p);

	void GetRect(CRect& rect);

	void GetWholeRegionMap();
	void CameraPerspective(BOOL b);
	BOOL m_bPerspecitve;
};

eMaskSetting GetValueSetting(const CString& strHelp, stSetting& st);
eMaskSetting SetValueSetting(CString& strHelp, const stSetting& st);
void MaskRect(cRect& rc, cPoint& p1,cPoint& p2);
void MaskRect(CRect& rc, CPoint& p1,CPoint& p2);
void MaskRect(RECT& rc, POINT& p1,POINT& p2);

#endif // !defined(AFX_CSTATUS_H__4296EB73_8ECC_4B05_8488_3542052BEB25__INCLUDED_)
