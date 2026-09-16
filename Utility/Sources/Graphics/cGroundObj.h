// cGroundObj.h: interface for the cGroundObj class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGROUNDOBJ_H__ED78BB65_4025_4921_BA47_620D96D7D204__INCLUDED_)
#define AFX_CGROUNDOBJ_H__ED78BB65_4025_4921_BA47_620D96D7D204__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cobj.h"

//a groudobj is a obj in the ground
class cGroundObj : public cObj  
{
public:
	cGroundObj(cGraphics* p);
	virtual ~cGroundObj();
	virtual void				Init(bool bCreateBuffer);

//graphics
	cCamera*					GetCamera();
	cGround*					GetGround();
//ground
	BOOL						m_bInGround;
	BOOL						m_bChild;
	BOOL						IsChild(){return m_bChild;}
	BOOL						SetChild(BOOL b){m_bChild = b;return b;}

	BOOL						m_bOffsetOfRegion;
	int							m_xRegion,m_yRegion;
	virtual void				SetRegion(int x,int y);
	bool						SetRegion(CPoint pt){SetRegion(pt.x,pt.y);return true;}
	void						GetRegion(int& x, int& y){x = m_xRegion; y = m_yRegion;}

	BOOL						IsInGround(){return m_bInGround;}
	virtual BOOL				SetInGround(BOOL b){m_bInGround = b;return b;}
	BOOL						IsInGraphics() {return IsInGround();};
//load and save
	HRESULT						LoadPack(void** pParam);
	virtual HRESULT				Load(const stSection* p);
	virtual HRESULT				Save(stSection* p);
	virtual HRESULT				LoadDynamic(LPCSTR szEvent){return S_OK;}; //call it after call load!
//param
	void						SetGroupID(long idGroup){GetGroundObjParam()->lGroupID = idGroup;}
	long						GetGroupID(){return GetGroundObjParam()->lGroupID;}
	stGroundObjParam*			GetGroundObjParam(){return (stGroundObjParam*) GetObjParam();};
	cPoint						GetOffset(){return GetGroundObjParam()->ptOffset;};
	virtual void				SetOffset(const cPoint& pt){GetGroundObjParam()->ptOffset = pt;};
//dynamic
	virtual ElementEnum			GetElementByType(){return GT_UNKNOW;};
	DynamicEnum					GetDynamicType(){return GetGroundObjParam()->eDynamic;}
	BOOL						IsDynamic(DynamicEnum e){return e == GetDynamicType();}
	BOOL						IsStatic(){return DYNAMIC_STATIC == GetDynamicType();}
	BOOL						IsNoSave(){return DYNAMIC_NOSAVE == GetDynamicType();}
	LPCSTR						GetEvent(){return GetGroundObjParam()->szEvent;}
	int							GetID(){return GetGroundObjParam()->lID;}
	LPCSTR						GetGroupFile(){return GetGroundObjParam()->szGroup;}	
	KindEnum					GetKind(){return GetGroundObjParam()->eKind;}
	BOOL						IsKind(long kind){return GetKind() == kind;}
	BOOL						IsMoving(){return GetGroundObjParam()->bMoving;}
//static 
	LPCSTR						GetTemplateFile(){return GetGroundObjParam()->szTemplate;}
	int							GetTemplateIndex(){return GetGroundObjParam()->iTemplate;}

	virtual HRESULT				setParam(stObjParam& pm) ;
	virtual HRESULT				OnCommand(long eCommand,stObjParam& pm);
	virtual	void				Offset(cPoint pt); //改变offset的回调函数
	virtual void				GetOffset(POINT ptScreen,cPoint& ptGround);
	void						CreateID();
//display
	bool						m_bDisplay;
	bool						IsDisplay(){return m_bDisplay;}
	virtual bool				CheckDisplay(const cRect* pRc){return false;}
	virtual bool				BeginLighting(cColor cr){return false;}
	virtual bool				EndLighting(){return false;}
	virtual bool				IsLighting(){return false;}
	virtual bool				Light(cLight* p){return false;}

	virtual HRESULT				BeginPainting(){return S_OK;};
	virtual HRESULT				EndPainting(){return S_OK;};

	virtual	HRESULT				Draw() {return S_OK;}
//modify
	BOOL m_bModify;
	BOOL IsModify(){return m_bModify;}
	void ClearModify(){m_bModify = FALSE;}
	void AddModify(){m_bModify = TRUE;}
};

#endif // !defined(AFX_CGROUNDOBJ_H__ED78BB65_4025_4921_BA47_620D96D7D204__INCLUDED_)
