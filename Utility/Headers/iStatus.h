#pragma once

#include "iPublic.h"
#include "iColor.h"

typedef void (*OutputMsgFunc)(int nStatus, LPCTSTR strMsg);
class iGraphics;

enum eStatusType
{
	STATUS_NULL=-1,
	STATUS_GENERAL,

	STATUS_CAMERA,
	
	STATUS_REGION,
	STATUS_LIGHT_STATIC,

	STATUS_TERRAIN_SQUARE,
	STATUS_TERRAIN_DIAMOND,
	STATUS_TERRAIN_ROAD,
	STATUS_DOODAD_SIMPLE,
	STATUS_DOODAD_DYNAMIC,
	STATUS_GROUP,
	STATUS_GROUP_DYNAMIC,

	STATUS_OBSTACLE,
	STATUS_SPECIAL,

	STATUS_TEST,
	STATUS_GAME,

	STATUS_NUM,
};

/*
enum eWantType
{
	WANT_NULL = -1,
	WANT_POINT,
	WANT_RECT,
	WANT_BOOL,
	WANT_STRING,
	WANT_COLOR,
	WANT_INT,
	WANT_FLOAT,
	WANT_CLEAR,
};
*/

enum eMaskSetting{
	MASK_EMPTY	= 0,
	MASK_RECT	= 0x01,
	MASK_POINT	= 0x02,
	MASK_BOOL	= 0x04,
	MASK_STRING	= 0x08,
	MASK_COLOR  = 0x10,
	MASK_INT	= 0x20,
	MASK_FLOAT  = 0x40,
	MASK_CPOINT	= 0x80,
	MASK_CLEAR  = 0x10000000,
};

struct stSetting
{
	long	mask;
	CRect	rect;
	CPoint	pt;
	bool	b;
	std::string str;
	cColor  cr;
	int		n;
	float	f;
	cPoint  point;
};

class iStatusMachine
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual BOOL OnLButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnLButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDown(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonUp(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnRButtonDblClk(UINT nFlags, CPoint point) = 0;
	virtual BOOL OnTimer(UINT nIDEvent) = 0;
	virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) = 0;
	virtual BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	virtual BOOL OnMouseMove(UINT nFlags, CPoint point) = 0;
	virtual BOOL Copy() = 0;
	virtual BOOL Cut() = 0;
	virtual BOOL Paste() =0;
/////////////////////////////////////////////////////////////////////////////
	
	virtual BOOL OnClose() = 0;
	virtual BOOL OnNewDocument(LPCTSTR lpszPathName) = 0;
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) = 0;
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName) = 0;

/////////////////////////////////////////////////////////////////////////////
//window operator
	virtual	BOOL Create(iGraphics* p,CWnd* pWnd,OutputMsgFunc func) = 0;
	virtual BOOL OnLeftSelectedFile(LPCTSTR lpszPathName,int frame) = 0;
	virtual BOOL OnRightSelectedFile(LPCTSTR lpszPathName, int frame) = 0;
	virtual void OnFrameChange(int frame) = 0;
	
	virtual CString	GetNewFile() = 0;
	virtual void	ClearNewFile() = 0;

/////////////////////////////////////////////////////////////////////////////
//redo and undo
	virtual BOOL Undo() = 0;
	virtual BOOL Redo() = 0;
	virtual BOOL CanUndo() = 0;
	virtual BOOL CanRedo() = 0;
	virtual BOOL RedoToEnd() = 0;
	virtual BOOL UndoToBegin() = 0;
};

_declspec(dllexport) iStatusMachine* Status_CreateStatusMachine();
