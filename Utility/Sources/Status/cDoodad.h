// cDoodad.h: interface for the cDoodad class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDOODAD_H__F59C5288_AF37_43D1_A771_99E5AF99BB8B__INCLUDED_)
#define AFX_CDOODAD_H__F59C5288_AF37_43D1_A771_99E5AF99BB8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

struct stSelect;

struct stUnit
{
	stUnit(){nLayer = 0;}
	ElementEnum eElement;		//plane, obstacle, light
	iObj* pObj;
	cPoint ptOffset;

	//plane data
	int nFrame;
	CString sName;
	int nLayer;
	//use for copy and paste!
	CString sDataName;

	//obstacle data
	ObstacleEnum eObstacle;
	int nPoint;
	KindEnum eKind;
	enum	{MAX_POINT = 32};
	cPoint	aPoint[MAX_POINT];

	//light data
	stLightParam paramLight;
};

typedef std::list<stUnit> listUnit;
typedef listUnit::iterator itUnit;

//doodad 改进到支持所有obj的集合
class cStatusGroup;

class cDoodad  
{
public:
	cDoodad(cStatusGroup* pGroupStatus);
	virtual ~cDoodad();

	cStatusGroup* m_pGroupStatus;
//com
	ULONG m_ulCount;
	ULONG AddRef();
	ULONG Release();

//graphics
	iGraphics*	GetGraphics();
	iGround*	GetGround();
	iCamera*	GetCamera();

//Obj operator
	listUnit m_listUnit;

	CString GetGroupFile();

	void Clear();
	void Delete();

	void AddLayer(int n);
	void Offset(cPoint pt,bool bRunNow = false);
	void OffsetTo(cPoint pt,bool bRunNow = false);
	void OffsetTo(CPoint pt,bool bRunNow = false);
	void SetID(int idDoodad,bool bRunNow = false);
	void SetGroupFile(LPCSTR szFile,bool bRunNow = false);

	void SetSelect(bool bSelect);
	void SetTrace(bool bTrace);
	void SetMoving(bool bTrace);

	void Select(POINT pt, iObjList& ilist,stSelect& select);
	void Select(cRect rc, iObjList& ilist, stSelect& select);
	void AddSelect(POINT pt, stSelect& select);
	void AddSelect(cRect rc, stSelect& select);
	void Select(POINT pt, stSelect& select);
	void Select(cRect rc, stSelect& select);
	
	void AddList(iObjList& ilist);
	void UpdateOffset();
	void Add();
	//存储一个plane的阻挡信息,存储到plane的模板文件
	void SaveFirstPlane();
	//修改组件的信息
	void SaveFirstDoodad();
	BOOL CopyFileToNet(LPCSTR szFile);

	BOOL GetFirstGroupFile(CString& s);
	
//create
	static int s_nDoodad;
	BOOL Create(CPoint pt,LPCSTR szFile,bool bRefresh);
	void Save();
	CString m_strName;
	void SaveAs(LPCSTR strName);
	void Load(LPCSTR strName);
	int GetID();

	void BeginMacroCommand();
	void EndMacroCommand();
};

#endif // !defined(AFX_CDOODAD_H__F59C5288_AF37_43D1_A771_99E5AF99BB8B__INCLUDED_)
