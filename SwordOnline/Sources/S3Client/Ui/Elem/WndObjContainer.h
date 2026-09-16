/*****************************************************************************************
//	½çÃæ´°¿ÚÌåÏµ½á¹¹--ÈİÄÉÓÎÏ·¶ÔÏóµÄ´°¿Ú
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-25
*****************************************************************************************/
#pragma once

#include "Windows.h"
#include "WndWindow.h"
#include "../Elem/WndMessage.h"

#define	OBJCONT_S_ENABLE_CLICK_EMPTY	0x00008000
#define	OBJCONT_S_ACCEPT_FREE			0x00004000
#define	OBJCONT_S_HAVEOBJBGCOLOR		0x00002000	//·ÅÓĞÎïÆ·Ê±ÊÇ·ñÓĞ±³¾°É«
#define	OBJCONT_S_TRACE_PUT_POS			0x00001000
#define	OBJCONT_S_DISABLE_PICKPUT		0x00000800	//²»ÔÊĞíÄÃÆğ¶«Î÷
#define	OBJCONT_F_MOUSE_HOVER			0x00000400

struct SBColorPos
{
	int pos1;
	int pos2;
};

//============================
//	mét hép nhá chøa item
//============================
class KWndObjectBox : public KWndWindow
{
public:
	KWndObjectBox();
	virtual int		Init(KIniFile* pIniFile, const char* pSection);	//³õÊ¼»¯
	void	LoadScheme(const char* pScheme);			///ÔØÈë½çÃæ·½°¸
	void	Celar();									//Çå³ı¶ÔÏóÎïÆ·
	void	SetObjectGenre(unsigned int uGenre);		//ÉèÖÃ¿ÉÒÔÈİÄÉµÄ¶ÔÏóµÄÀàĞÍ
	int		GetObject(KUiDraggedObject& Obj) const;		//»ñÈ¡ÈİÄÉµÄ¶ÔÏóĞÅÏ¢
	void	HoldObject(unsigned int uGenre, unsigned int uId, int nDataW, int nDataH);//ÉèÖÃÈİÄÉµÄ¶ÔÏó
	void	Clone(KWndObjectBox* pCopy);
	void	SetContainerId(int nId);
	void	EnablePickPut(bool bEnable);
protected:
	virtual int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);//´°¿Úº¯Êı
	int		DropObject(bool bTestOnly);				//·ÅÖÃÎïÆ·
	void	PaintWindow();							//´°Ìå»æÖÆ
	unsigned int		m_uAcceptableGenre;			//¿É½ÓÄÉµÄ¶ÔÏóÀàĞÍ
	KUiDraggedObject	m_Object;
	int					m_nContainerId;
	UINT			m_uBdNextTime; //mèc thêi gian kÕ tiÕp ®Ó chuyÓn mµu
	float			m_fBdPercent; //tØ lÖ mµu
	float			m_fBdAddPerFrame; //céng thªm tØ lÖ mµu mçi lÇn chuyÓn
	SBColorPos		m_sBClrPos;		//chøa vŞ trİ random b¾t ®Çu cña viÒn s¸ng chİnh
	UINT			m_uBClrNextTime; //mèc thêi gian kÕ tiÕp ®Ó dŞch chuyÓn mµu viÒn chİnh
	int				m_nAddLightPos;
};

//============================
//	nh÷ng c¸i r­¬ng chøa
//============================
class KWndObjectMatrix : public KWndWindow
{
public:
	KWndObjectMatrix();
	virtual ~KWndObjectMatrix();
	virtual int		Init(KIniFile* pIniFile, const char* pSection);	//³õÊ¼»¯
	void			Clear();									//Çå³ıÈ«²¿µÄ¶ÔÏóÎïÆ·
	int				AddObject(KUiDraggedObject* pObject, int nCount);	//Ôö¼Ó¶ÔÏóÎïÆ·
	int				RemoveObject(KUiDraggedObject* pOjbect);			//¼õÉÙÒ»¸ö¶ÔÏóÎïÆ·
	int				GetObject(KUiDraggedObject& Obj, int x, int y) const;//»ñÈ¡ÈİÄÉµÄÄ³¸ö¶ÔÏóĞÅÏ¢
//	int				GetObjects(KUiDraggedObject* pObjects, int nCount) const;//»ñÈ¡ÈİÄÉµÄ¶ÔÏóĞÅÏ¢
	void			EnableTracePutPos(bool bEnable);
	void			SetContainerId(int nId);
	void			EnablePickPut(bool bEnable);
protected:
	void			Clone(KWndObjectMatrix* pCopy);
private:
	virtual int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);	//´°¿Úº¯Êı
	void	PaintWindow();										//´°Ìå»æÖÆ
	int		GetObjectAt(int x, int y);							//»ñµÃÄ³¸öÎ»ÖÃÉÏµÄÎïÆ·µÄË÷Òı
	int		PickUpObjectAt(int x, int y);						//¼ñÆğÄ³¸öÎ»ÖÃÉÏµÄ¶ÔÏó
	int		DropObject(int x, int y, bool bTestOnly);			//·ÅÖÃÎïÆ·
	int		TryDropObjAtPos(const RECT& dor, KUiDraggedObject*& pOverlaped);//³¢ÊÔ·ÅÖÃÎïÆ·
	void	DropObject(int x, int y, KUiDraggedObject* pToPickUpObj);		//·ÅÏÂÎïÆ·

protected:
	int				m_nNumUnitHori;		//ºáÏò¸ñÊı
	int				m_nNUmUnitVert;		//×İÏò¸ñÊı
	int				m_nUnitWidth;		//ºáÏò¸ñ¿í
	int				m_nUnitHeight;		//×İÏò¸ñ¿í
	int				m_nUnitBorder;		//¸ñ×ÓµÄ±ß¿òµÄ¿í¸ß¶È
	int				m_nNumObjects;		//ÈİÄÉµÄ¶ÔÏóµÄÊıÄ¿
	KUiDraggedObject* m_pObjects;		//ÈİÄÉµÄ¶ÔÏóÁĞ±í
	int				m_nMouseOverObj;

	int				m_nPutPosX;
	int				m_nPutPosY;
	int				m_nPutWidth;
	int				m_nPutHeight;
	int				m_nContainerId;
	UINT			m_uBdNextTime; //mèc thêi gian kÕ tiÕp ®Ó chuyÓn mµu
	float			m_fBdPercent; //tØ lÖ mµu
	float			m_fBdAddPerFrame; //céng thªm tØ lÖ mµu mçi lÇn chuyÓn
	SBColorPos*		m_pBClrPos;		//chøa vŞ trİ random b¾t ®Çu cña viÒn s¸ng chİnh
	UINT			m_uBClrNextTime; //mèc thêi gian kÕ tiÕp ®Ó dŞch chuyÓn mµu viÒn chİnh
};

void WndObjContainerInit(KIniFile* pIni);