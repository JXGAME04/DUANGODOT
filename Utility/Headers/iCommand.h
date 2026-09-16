#pragma once

#include "ipublic.h"
#include "iColor.h"
#include "iCommon.h"
//#include "iitem.h"
#include <list>
//#include "iGraphics.h"


class iGraphics;
class iAutoGround;
struct stObjParam;
class iObj;

/////////////////////////////////////////////////////////////
//command type
/////////////////////////////////////////////////////////////

enum eCommandType
{
	COM_NULL = -1,
		
	COM_FILE_NEWDOCUMENT,
	COM_FILE_OPENDOCUMENT,
	COM_FILE_SAVEDOCUMENT,

	COM_FILE_DYNAMIC,

	COM_WINDOW_TOGGLEFULLSCREEN,

	COM_GROUND_TOGGLEGRID,
	COM_GROUND_FILLRECT,
	COM_GROUND_CHANGETILE,
	COM_GROUND_CHANGEBRIGHTNESS,
	
	COM_STATUS_CHANGE,
	COM_STATUS_SELECTFILE,

	COM_CAMERA_OFFSET,
	COM_CAMERA_OFFSETTO,
	COM_CAMERA_SCALE,
	COM_CAMERA_SET,

	COM_OBJ_ADD,
	COM_OBJ_REMOVE,
//	COM_OBJ_OFFSET,
	COM_OBJ_SETPARAM,

	COM_MACRO,
	//COM_MACRO = 100000, //dont like this for use command_num as a array bound

	COMMAND_NUN,
};
//if u change this,please change g_strCommand as well

//*******************************************************
//general
struct stCommand
{
	eCommandType	type;
	ULONG			size;
	tm				time;
	DWORD			ms;		//ºÁÃë
	stCommand(){size = sizeof(stCommand);}
};

//*******************************************************
//document
struct stCommand_Document : public stCommand
{
	char		strFile[MAX_CHAR];
	stCommand_Document() {size = sizeof(stCommand_Document);}
};

struct stCommand_Dynamic: public stCommand
{
	enum OperatorEnum{OP_NEW,OP_OPEN,OP_SAVE,};
	int			nOperator;
	char		strFile[MAX_CHAR];
	stCommand_Dynamic() {size = sizeof(stCommand_Dynamic);}
};

//*******************************************************
//ground
struct stCommand_ChangeGround : public stCommand
{
	int				xAutoGround,yAutoGround;
	int				xGround,yGround;
	char			strFile[MAX_CHAR];
	char			strDocument[MAX_CHAR];	
	BOOL			bIgnore;//ºöÂÔµØÍ¼µÄÆ´½Ó
	iAutoGround*	pAutoGround;
	stCommand_ChangeGround() {size = sizeof(stCommand_ChangeGround);}
};

struct stCommand_FillGround : public stCommand
{
	stCommand		stCmd;
	CRect			rect;
	char			strFile[MAX_CHAR];
	char			strDocument[MAX_CHAR];
	iAutoGround*	pAutoGround;
	stCommand_FillGround() {size = sizeof(stCommand_FillGround);}
};

struct stCommand_ChangeGroundBrightness : public stCommand
{
	cColor	crColor;
	stCommand_ChangeGroundBrightness() {size = sizeof(stCommand_ChangeGroundBrightness);}
};

struct stCommand_ChangeRegionColor : public stCommand
{
	cPoint		ptRegion;
	cColor		crNew;
	stCommand_ChangeRegionColor() {size = sizeof(stCommand_ChangeRegionColor);}
};

//*******************************************************
//camera
struct stCommand_OffsetCameraTo : public stCommand
{
	cPoint ptTo;
	stCommand_OffsetCameraTo() {size = sizeof(stCommand_OffsetCameraTo);}
};

struct stCommand_OffsetCamera : public stCommand
{
	cPoint ptTo;
	stCommand_OffsetCamera() {size = sizeof(stCommand_OffsetCamera);}
};

struct stCommand_SetCamera : public stCommand
{
	cPoint ptEye;
	cPoint ptTo;
	float fScale;
	stCommand_SetCamera() {size = sizeof(stCommand_SetCamera);}
};

struct stCommand_ScaleCamera : public stCommand
{
	float			fScale;
	stCommand_ScaleCamera() {size = sizeof(stCommand_ScaleCamera);}
};

//***********************************************************
//obj
struct stCommand_AddObj : public stCommand
{
	iObj*			pObj;
	stCommand_AddObj() {size = sizeof(stCommand_AddObj);}
};

/*
struct stCommand_OffsetObj : public stCommand
{
	iObj*			pObj;
	cPoint			ptOffset;
	stCommand_OffsetObj() {size = sizeof(stCommand_OffsetObj);}
};
*/

struct stCommand_SetObjParam : public stCommand
{
	iObj*			pObj;
	stObjParam*		pParam;
	stCommand_SetObjParam() {size = sizeof(stCommand_SetObjParam);}
};

//***********************************************************
//plane 
struct stCommand_PlaneAddObstacle : public stCommand
{
	iObj*			pPlane;
	iObj*			pObstacle;
	stCommand_PlaneAddObstacle() {size = sizeof(stCommand_PlaneAddObstacle);}
};

/////////////////////////////////////////////////////////////
//do type
/////////////////////////////////////////////////////////////

enum eDoType
{
	DO_NOTHING,
	DO_CONTINUE,
	DO_CANUNDO,
	DO_CANNOTUNDO,
	DO_BREAKUNDO,
	DO_FAILED,
};

/////////////////////////////////////////////////////////////
//command class
/////////////////////////////////////////////////////////////

class iCommand
{
public:
	virtual ULONG			AddRef()  = 0;
	virtual ULONG			Release() = 0;

	virtual eCommandType	GetType() = 0;

	virtual HRESULT			SetParam(stCommand* pCommand) = 0;

//protected		
	virtual eDoType			Undo() = 0;
	virtual eDoType			Redo() = 0;
	virtual eDoType			Do() = 0;

	virtual BOOL			Merge(iCommand* p) = 0;

	virtual HRESULT			GetMessage(CString& str,int e ) = 0;
};

class iCommandMachine
{
public:
	virtual ULONG			AddRef()  = 0;
	virtual ULONG			Release() = 0;

	virtual iGraphics*		GetGraphics() = 0;

	virtual HRESULT			Undo() = 0;
	virtual HRESULT			Redo() = 0;
	virtual BOOL			CanUndo() = 0;
	virtual BOOL			CanRedo() = 0;
	virtual HRESULT			UndoToBegin() = 0;
	virtual HRESULT			RedoToEnd() = 0;
	virtual HRESULT			BreakUndo() = 0;

//they are a pair

	virtual iCommand*		CreateCommand(stCommand* pCmd) = 0;
	virtual	void			BeginMacroCommand() = 0;
	virtual	void			EndMacroCommand() = 0;
	virtual HRESULT 		Command(iCommand* p) = 0;
	virtual HRESULT 		Command(stCommand* pCmd) = 0;

	virtual BOOL			FindCommandMessageString(iCommand* pCom,int e, CString& strMessage) = 0;

	virtual HRESULT			Start(LPCSTR szWorkingFolder) = 0;
	virtual HRESULT			End()  = 0;
};

typedef std::list<iCommand*>			iCommandList;
typedef iCommandList::iterator	itiCommand;

_declspec(dllexport) iCommandMachine* Command_CreateCommandMachine(LPCSTR szAppName, iGraphics* p);
_declspec(dllexport) eCommandType CommandNameToID(const CString& s);
_declspec(dllexport) int OutputNameToID(const CString& s);
