// DivideDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resourcemanager.h"
#include "DivideDlg.h"

#include "iitem.h"
#include "fileop.h"
#include "planedoodad.h"
#include "iGraphics.h"

#include <math.h>
//const float		c_fPI				= 3.14159265f;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDivideDlg dialog
const int TIMER_UPDATE = 1000;
const int c_nMaxWidth = 400;
const int c_nMaxHeight = 600;

CDivideDlg::CDivideDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDivideDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDivideDlg)
	m_nX = 0;
	m_nY = 0;
	m_bDisplayAlpha = FALSE;
	//}}AFX_DATA_INIT
	m_aCB[TD_DOODADTYPE] = &m_cbDoodadType;
	m_aCB[TD_GEOMETRY] = &m_cbGeometry;
	m_aCB[TD_HEIGHT] = &m_cbHeight;
	m_aCB[TD_OBSTACLE] = &m_cbObstacle;
}


void CDivideDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDivideDlg)
	DDX_Control(pDX, IDC_OBSTACLE, m_cbObstacle);
	DDX_Control(pDX, IDC_HIGH, m_editHigh);
	DDX_Control(pDX, IDC_WIDE, m_editWide);
	DDX_Control(pDX, IDC_SPIN_HEIGHT3, m_spinHigh);
	DDX_Control(pDX, IDC_SPIN_HEIGHT2, m_spinWide);
	DDX_Control(pDX, IDC_SPIN_HEIGHT, m_spinHeight);
	DDX_Control(pDX, IDC_STATIC_HEIGHT, m_scHeight);
	DDX_Control(pDX, IDC_STATIC_WIDTH, m_scWidth);
	DDX_Control(pDX, IDC_STATIC_SPR, m_staticSpr);
	DDX_Control(pDX, IDC_LISTPOINT, m_listPoint);
	DDX_Control(pDX, IDC_SPINY, m_spinY);
	DDX_Control(pDX, IDC_SPINX, m_spinX);
	DDX_Control(pDX, IDC_HEIGHT, m_cbHeight);
	DDX_Control(pDX, IDC_GEOMETRY, m_cbGeometry);
	DDX_Control(pDX, IDC_DOODADTYPE, m_cbDoodadType);
	DDX_Text(pDX, IDC_X, m_nX);
	DDX_Text(pDX, IDC_Y, m_nY);
	DDX_Check(pDX, IDC_DISPLAYALPHA, m_bDisplayAlpha);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDivideDlg, CDialog)
	//{{AFX_MSG_MAP(CDivideDlg)
	ON_BN_CLICKED(IDC_DELETEPOINT, OnDeletepoint)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_ADDPOINT, OnAddpoint)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_LEFT, OnLeft)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	ON_BN_CLICKED(IDC_RIGHT, OnRight)
	ON_WM_KEYDOWN()
	ON_CBN_SELCHANGE(IDC_DOODADTYPE, OnSelchangeDoodadtype)
	ON_CBN_SELCHANGE(IDC_GEOMETRY, OnSelchangeGeometry)
	ON_CBN_EDITCHANGE(IDC_HEIGHT, OnEditchangeHeight)
	ON_CBN_SELCHANGE(IDC_HEIGHT, OnSelchangeHeight)
	ON_BN_CLICKED(ID_DIVIDED, OnDivided)
	ON_CBN_SELCHANGE(IDC_OBSTACLE, OnSelchangeObstacle)
	ON_BN_CLICKED(IDC_DISPLAYALPHA, OnDisplayalpha)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_WM_CANCELMODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDivideDlg message handlers

enum {DOODAD_VER,DOODAD_VERHOR,DOODAD_HORONLY,DOODAD_VERONLY,};
void CDivideDlg::GetDivide(stDivide& divide)
{
	divide.eDoodadType = m_aCB[TD_DOODADTYPE]->GetCurSel();
	divide.eGeometryType = m_aCB[TD_GEOMETRY]->GetCurSel();
	divide.eObstacleType = m_aCB[TD_OBSTACLE]->GetCurSel();
	CString s;
	m_aCB[TD_HEIGHT]->GetWindowText(s);
	StringToValue(s,divide.nHeight);
	m_editHigh.GetWindowText(s);
	StringToValue(s,divide.nHigh);
	m_editWide.GetWindowText(s);
	StringToValue(s,divide.nWide);

	int nHeight = divide.nHeight;
	int nWidth = divide.nHeight;
	if (divide.eDoodadType == 2)
		nHeight /= 2;

	int width,height;
	m_staticSpr.GetSize(width,height);
	if (nHeight != 0)
	{
		divide.nHigh = (height+nHeight-1) / nHeight-1;
		divide.nWide = (height+nWidth-1) / nWidth-1;
	}
	else
	{
		divide.nHigh = 0;
		divide.nWide = 0;
	}


	divide.nPoint = m_listPoint.GetCount();
	for (int i=0; i<divide.nPoint; i++)
	{
		m_listPoint.GetText(i,s);
		StringToValue(s,divide.aPoint[i]);
	}
}

void CDivideDlg::UpdatePointArray()
{
	stDivide divide;
	GetDivide(divide);
	m_staticSpr.SetDivide(divide);
	SaveDivide(divide);
}

void CDivideDlg::SaveDivide(stDivide& divide)
{
	CString sName,sExt;
	AppParseFileName(m_strFile,sName,sExt);
	sName += ".div";
	CFile file;
	VERIFY(AppOpenFile(file,sName,CFile::modeCreate | CFile::modeWrite,"dividedlg"));
	file.Write(&divide,sizeof(divide));
	file.Close();
}

BOOL CDivideDlg::LoadDivide(stDivide& divide)
{
	CString sName,sExt;
	AppParseFileName(m_strFile,sName,sExt);
	sName += ".div";
	CFile file;
	if (file.Open(sName,CFile::modeRead))
	{
		file.Read(&divide,sizeof(divide));
		file.Close();
		if (divide.nHeight < 250)
			divide.nHeight = 400;
		if (divide.nHigh < 0 || divide.nHigh > 9)
			divide.nHigh = 0;
		if (divide.nWide < 0 || divide.nWide > 9)
			divide.nWide = 0;
		return TRUE;
	}
	if (divide.nHigh < 0 || divide.nHigh > 9)
		divide.nHigh = 0;
	if (divide.nWide < 0 || divide.nWide > 9)
		divide.nWide = 0;
	return FALSE;
}

void CDivideDlg::OnAddpoint() 
{
	UpdateData();
	CString s;
	CPoint point(m_nX,m_nY);
	FormatPoint(point);
	ValueToString(point,s);
	m_listPoint.AddString(s);

	UpdatePointArray();
}

void CDivideDlg::OnDeletepoint() 
{
	int index = m_listPoint.GetCurSel();
	if (index != -1)
	{
		m_listPoint.DeleteString(index);
		UpdatePointArray();
	}
}

void CDivideDlg::DivideImage(int w, int h, stImageInfo& info)
{
	iImage* pImage = Image_CreateImage(m_strFile);
	pImage->LoadFile(m_strFile);
	pImage->GetImageInfo(info);
	CString sPath;
	CString sName;
	AppParsePathName(m_strFile,sPath,sName);
	CString strName = sPath + "\\%03d.tga";
	pImage->DivideTo(strName,w,h);
	pImage->Release();
}

void CDivideDlg::DivideVer(int w, int h)
{
	CWaitCursor cursor;
	UpdateData();
	stImageInfo info;
	DivideImage(w,h,info);

	iItem* pItem = Common_CreateItem(m_strTxtFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
//	pItem->ClearAll();
	SetItemValue(pItem,GetTablePlaneName(TP_PLANETYPE),"垂直于地表");			
	SetItemValue(pItem,GetTablePlaneName(TP_LIGHTING),"按照切分受光");			
	SetItemValue(pItem,GetTablePlaneName(TP_LIGHTED),"按照底边挡光");			
	SetItemValue(pItem,GetTablePlaneName(TP_ANGLEZ),"90");
	SetItemValue(pItem,GetTablePlaneName(TP_ANGLEXY),"00");
	SetItemValue(pItem,GetTablePlaneName(TP_SORT),"高于ver");
	if (w != 0 || h != 0)
	{
		int pos = m_strTxtFile.Find('.');
		ASSERT(pos != -1);
		m_strGroupFile = m_strTxtFile.Left(pos) + "_group.txt";
		iItem* pItemTxt = Common_CreateItem(m_strGroupFile,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItemTxt);
		int num = (w+1)*(h+1);
		SetItemValue(pItemTxt,"objnum",num);
		SetItemValue(pItemTxt,"icon","group");
		SetItemValue(pItemTxt,"type","group");
		SetItemValue(pItemTxt,"grouptype","normal");

		int i = 0;
		int type = GT_PLANEVERNORMAL;
		CString s = m_strTxtFile;
		AppGetShortPath(s);
		cPoint point;
		int width = info.width / (w+1);
		int height = info.height / (h+1);

		for (int y = 0; y <= h; y++)
		for (int x = 0; x <= w; x++)
		{
			point.x = (x * width) / c_ptScale.x;
			point.z = ((h-y) * height) / c_ptScale.z;
			SetItemValue(pItemTxt,"planefile",s,i);
			SetItemValue(pItemTxt,"planeframe",i,i);
			SetItemValue(pItemTxt,"elementtype",type,i);
			SetItemValue(pItemTxt,"objoffset",point,i);
			if (y == h)
				SetItemValue(pItem,GetTablePlaneName(TP_SORT),"ver",i);
			i++;
		}	
		pItemTxt->Save();
	}
	pItem->Save();
	OnOK();
}

void CDivideDlg::DivideHor(int w, int h) 
{
	CWaitCursor cursor;
	UpdateData();
	stImageInfo info;
	DivideImage(w,h,info);

	iItem* pItem = Common_CreateItem(m_strTxtFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,GetTablePlaneName(TP_PLANETYPE),"平行于地表");			
	SetItemValue(pItem,GetTablePlaneName(TP_LIGHTING),"按照重心受光");			
	SetItemValue(pItem,GetTablePlaneName(TP_LIGHTED),"不挡光");			
	SetItemValue(pItem,GetTablePlaneName(TP_ANGLEZ),"00");
	SetItemValue(pItem,GetTablePlaneName(TP_ANGLEXY),"00");
	SetItemValue(pItem,GetTablePlaneName(TP_SORT),"低于ver");
	//make group file
	if (w != 0 || h != 0)
	{
		int pos = m_strTxtFile.Find('.');
		ASSERT(pos != -1);
		m_strGroupFile = m_strTxtFile.Left(pos) + "_group.txt";
		iItem* pItem = Common_CreateItem(m_strGroupFile,iItem::FLAG_CREATE);
		wrap_ptr<iItem> ptr(pItem);
		int num = (w+1)*(h+1);
		SetItemValue(pItem,"objnum",num);
		SetItemValue(pItem,"icon","group");
		SetItemValue(pItem,"type","group");
		SetItemValue(pItem,"grouptype","normal");

		int i = 0;
		int type = GT_PLANEHORNORMAL;
		CString s = m_strTxtFile;
		AppGetShortPath(s);
		cPoint point;
		int width = info.width / (w+1);
		int height = info.height / (h+1);

		for (int y = 0; y <= h; y++)
		for (int x = 0; x <= w; x++)
		{
			point.x = (x * width) / c_ptScale.x;
			point.y = ((y-h) * height) / c_ptScale.y;
			SetItemValue(pItem,"planefile",s,i);
			SetItemValue(pItem,"planeframe",i,i);
			SetItemValue(pItem,"elementtype",type,i);
			SetItemValue(pItem,"objoffset",point,i);
			i++;
		}	
		pItem->Save();
	}
	pItem->Save();
	OnOK();
}

void CDivideDlg::OnDivide() 
{
	stDivide divide;
	GetDivide(divide);
	if (divide.eDoodadType == 2)
	{
		DivideHor(divide.nWide,divide.nHigh);
		return;
	}
	if (divide.eDoodadType == 3)
	{
		DivideVer(divide.nWide,divide.nHigh);
		return;
	}
	if (divide.eDoodadType == 1)
	{
		AfxMessageBox("不再支持这种类型!");
		return;
	}
	if (divide.nHeight < 250)
	{
		AfxMessageBox("高度不能低于250!");
		return;
	}

	//但是divide.nPoint == 0指直接使用
	if (divide.nPoint == 0 || divide.nPoint == 1 || divide.nPoint == 2)
	{
		AfxMessageBox("三个点以上才可以切分!");
		return;
	}

	for (int i=0; i<divide.nPoint-1; i++)
	{
		if (divide.aPoint[i+1].x - divide.aPoint[i].x > c_nMaxWidth)
		{
			CString s;
			s.Format("(%d,%d)之间宽度超过了400",i,i+1);
			AfxMessageBox(s);
			return;			
		}
		if (fabs(divide.aPoint[i+1].y - divide.aPoint[i].y) + divide.nHeight > c_nMaxHeight)
		{
			CString s;
			s.Format("(%d,%d)之间高度超过了400",i,i+1);
			AfxMessageBox(s);
			return;			
		}
	}

	int miny = divide.aPoint[0].y - divide.nHigh * divide.nHeight;
	int indexy = 0;
	for (i=0; i<divide.nPoint; i++)
	{
		if (miny > divide.aPoint[i].y - divide.nHigh * divide.nHeight)
		{
			miny = divide.aPoint[i].y - divide.nHigh * divide.nHeight;
			indexy = i;
		}
	}

	if (miny < 0)
	{
		if (divide.aPoint[i].y - divide.nHigh * divide.nHeight < 0)
		{
			CString s;
			s.Format("(%d)点超过了图像,去掉最上面一层么?",indexy);
			if (AfxMessageBox(s,MB_OKCANCEL) == IDOK)
			{
				//increate the image top
				divide.nHigh--;
				CString s;
				s.Format("%d",divide.nHigh);
				m_editHigh.SetWindowText(s);
			}
			else
				return;			
		}
	}

	Divide(divide);
}

void CDivideDlg::Divide(stDivide& divide)
{		
	CString sPath;
	CString sName;
	AppParsePathName(m_strFile,sPath,sName);
	CString strName = sPath + "\\%03d.tga";

	if (divide.nPoint == 0)
	{
		CString strTga = sPath + "\\001.tga";
		AppCopyFile(m_strFile,strTga);
		OnOK();
		return;
	}

	int width,height;
	m_staticSpr.GetSize(width,height);
	//纵向切分了几份
	if (! m_staticSpr.Divide(strName,divide.nHigh))
	{
		AfxMessageBox("切分出错，请检查不要让点超出图象");
		return;
	}
	//经过格式化的divide
	m_staticSpr.GetDivide(divide);

	//
	bool bShadow = false;
	CFileFind f;
	CString strTgb = sPath + "\\1.tgb";
	CString strTga = sPath + "\\999.tga";
	if (f.FindFile(strTgb))
	{
		AppCopyFile(strTgb,strTga);
		bShadow = true;
	}

	ASSERT(!m_strTxtFile.IsEmpty());
	AppBuildPath(m_strTxtFile);
	//生成一个txt文件

	cPoint ptGroupOffset;
	int pos = m_strTxtFile.Find('.');
	ASSERT(pos != -1);
	m_strGroupFile = m_strTxtFile.Left(pos) + "_group.txt";
	iItem* pItem = Common_CreateItem(m_strGroupFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"icon","group");
	SetItemValue(pItem,"type","group");
	SetItemValue(pItem,"grouptype","normal");

	int h = divide.nHigh;
	int nObjNum;
	nObjNum = (divide.nPoint - 1) * (h+1);
	//把阻挡加上
	if (divide.eObstacleType != OBSTACLE_NULL)
		nObjNum++;
	SetItemValue(pItem,"objnum",nObjNum);

	iItem* pItemTxt = Common_CreateItem(m_strTxtFile,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr2(pItemTxt);

	cPoint aPoint[MAX_POINT+2];
	float fAngleXY[MAX_POINT];
	CString sPlane = m_strTxtFile;
	AppGetShortPath(sPlane);

	for (int i=0; i<divide.nPoint-1; i++)
	{
		//计算角度
		if (divide.eGeometryType == 0 && 0)
		{
			if (i & 1)
				fAngleXY[i] = 45;
			else
				fAngleXY[i] = 135;
		}
		else
		{
			float fAngleRenderZ = c_fPI / 6;	//	a渲染时平面与Z轴角度
			float sina = sin(fAngleRenderZ);
			float tgb ;
			float tgr ;							// for example: a = 30, b = 45, then tgr = 0.5;
			float dy = -(divide.aPoint[i+1].y - divide.aPoint[i].y);
			float dx = (divide.aPoint[i+1].x - divide.aPoint[i].x);
			tgr = dy / dx;
			tgb = tgr / sina;
			fAngleXY[i] = atan(tgb);
			if (fAngleXY[i] >= 0)
				fAngleXY[i] = fAngleXY[i];
			else
				fAngleXY[i] = c_fPI + fAngleXY[i];
			fAngleXY[i] = fAngleXY[i] / c_fPI * 180.f;
		}
		//计算offset
		aPoint[i].x = (divide.aPoint[i].x - divide.aPoint[0].x) / c_ptScale.x;
		aPoint[i].y = (divide.aPoint[i].y - divide.aPoint[0].y) / c_ptScale.y;
	}

	for (int y=0; y<=h; y++)
	for (int x=0; x<divide.nPoint-1; x++)
	{
		int index = y * (divide.nPoint-1) + x;
		SetItemValue(pItemTxt,GetTablePlaneName(TP_PLANETYPE),"垂直于地表",index);			
		SetItemValue(pItemTxt,GetTablePlaneName(TP_LIGHTING),"按照切分受光",index);			
		SetItemValue(pItemTxt,GetTablePlaneName(TP_LIGHTED),"按照底边挡光",index);			
		SetItemValue(pItemTxt,GetTablePlaneName(TP_ANGLEZ),"90",index);
		SetItemValue(pItemTxt,GetTablePlaneName(TP_KIND),"组件的垂直部分",index);
//		SetItemValue(pItemTxt,GetTablePlaneName(TP_TEXTUREUSE),"使用全部贴图",index);
		SetItemValue(pItemTxt,GetTablePlaneName(TP_ANGLEXY),fAngleXY[x],index);			
		if (y == 0)
			SetItemValue(pItemTxt,GetTablePlaneName(TP_SORT),"ver",index);
		else
			SetItemValue(pItemTxt,GetTablePlaneName(TP_SORT),"高于ver",index);
		int type = GT_PLANEVERMESH;
		SetItemValue(pItem,"elementtype",type,index);
		cPoint pt = aPoint[x];		
//		pt.z = ((h-y) * c_nMaxHeight - (height - max(divide.aPoint[i].y,divide.aPoint[i+1].y))) / c_ptScale.z;
		pt.z = ((h-y) * divide.nHeight) / c_ptScale.z;
		SetItemValue(pItem,"objoffset",pt,index);
		SetItemValue(pItem,"planefile",sPlane,index);
		SetItemValue(pItem,"planeframe",index,index);
	}

	//障碍的信息
	aPoint[i].x = (divide.aPoint[i].x - divide.aPoint[0].x) / c_ptScale.x;
	aPoint[i].y = (divide.aPoint[i].y - divide.aPoint[0].y) / c_ptScale.y;
	aPoint[i+1].x = ((aPoint[0].x + aPoint[0].y) + (aPoint[i].x - aPoint[i].y))  / 2.f;
	aPoint[i+1].y = ((aPoint[0].x + aPoint[0].y) - (aPoint[i].x - aPoint[i].y))  / 2.f;
	aPoint[i+2] = aPoint[0];
	int nTemp = GT_OBSTACLE;
	if (divide.eObstacleType != OBSTACLE_NULL)
	{
		SetItemValue(pItem,"elementtype",nTemp,nObjNum-1);
		SetItemValue(pItem,"objoffset",aPoint[0],nObjNum-1);
		SetItemValue(pItem,"obstacletype",divide.eObstacleType,nObjNum-1);
		nTemp = divide.nPoint + 2;
		SetItemValue(pItem,"obstaclepointnum",nTemp,nObjNum-1);
		CString sValue;
		PointArrayToString(aPoint,nTemp,sValue);
		SetItemValue(pItem,"obstaclepointarray",sValue,nObjNum-1);
//		SetItemValue(pItem,"obstaclekind","房屋",nObjNum-1);
	}

	//阴影的信息
	int nPoint = (divide.nPoint - 1)*(h+1);
	if (bShadow)
	{
		SetItemValue(pItemTxt,GetTablePlaneName(TP_PLANETYPE),"平行于地表",nPoint);			
		SetItemValue(pItemTxt,GetTablePlaneName(TP_SORT),"低于ver",nPoint);			
		SetItemValue(pItemTxt,GetTablePlaneName(TP_KIND),"组件的阴影",nPoint);
	}
	pItem->Save();
	pItemTxt->Save();
	OnOK();
}

void CDivideDlg::OnCancel() 
{
	CDialog::OnCancel();
}

BOOL CDivideDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ASSERT(!m_strFile.IsEmpty());
	m_staticSpr.SetSprFile(m_strFile);
	m_staticSpr.SetFrame(0,0);

	int width,height;
	m_staticSpr.GetSize(width,height);
	int w = width / c_nMaxWidth;
	int h = height / c_nMaxHeight;
	CString s;
	s.Format("%d",w);
	m_editWide.SetWindowText(s);
	s.Format("%d",h);
	m_editHigh.SetWindowText(s);
	s.Format("(%d)",width);
	m_scWidth.SetWindowText(s);
	s.Format("(%d)",height);
	m_scHeight.SetWindowText(s);
	m_spinY.SetRange(0,height-1);
	m_spinHeight.SetRange(0,height-1);
	m_spinX.SetRange(0,width-1);
	m_spinWide.SetRange(0,9);
	m_spinHigh.SetRange(0,9);

	int i;
	for (i=0; i<TD_NUM; i++)
	{
		int j = 0;
		CString* aStr = GetTableDoodadValue(i);
		while(!aStr[j].IsEmpty())
			m_aCB[i]->AddString(aStr[j++]);

		CString sValue = GetTableDoodadDefaultValue(i);
		int n = StringToDoodadTableValue(i,sValue);
		ASSERT(n != -1);
		m_aCB[i]->SetCurSel(n);
	}
	m_staticSpr.SetShowCenter(TRUE);

	stDivide dv;
	if (LoadDivide(dv))
	{
		m_aCB[TD_DOODADTYPE]->SetCurSel(dv.eDoodadType);
		m_aCB[TD_GEOMETRY]->SetCurSel(dv.eGeometryType);
		m_aCB[TD_OBSTACLE]->SetCurSel(dv.eObstacleType);
		CString s;
		ValueToString(dv.nHeight,s);
		m_aCB[TD_HEIGHT]->SetWindowText(s);
		for (int i=0; i<dv.nPoint; i++)
		{
			ValueToString(dv.aPoint[i],s);
			m_listPoint.AddString(s);
		}
		UpdatePointArray();
	}

	SetTimer(TIMER_UPDATE,100,NULL);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

bool IsKeyDown(UINT vk)
{
	short state = GetKeyState(vk);
	return (HIBYTE(state)!=0 );
}

void CDivideDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == TIMER_UPDATE)
	{
		static int m_nHeight = 0;
		static int m_nWide = 0;
		static int m_nHigh = 0;

		int nHeight;
		int nHigh;
		int nWide;

		{
			CString s;
			m_aCB[TD_HEIGHT]->GetWindowText(s);
			nHeight= atoi(s);
		}
		{
			CString s;
			m_editWide.GetWindowText(s);
			nWide= atoi(s);
		}
		{
			CString s;
			m_editHigh.GetWindowText(s);
			nHigh= atoi(s);
		}

		if (nHeight != m_nHeight || nWide != m_nWide || nHigh != m_nHigh)
		{
			UpdatePointArray();
			m_nHeight = nHeight;
			m_nWide = nWide;
			m_nHigh = nHigh;
		}
		
		UpdateData();
		m_staticSpr.SetCenter(CPoint(m_nX,m_nY));
		if (IsKeyDown(VK_NUMPAD6))
			OnRight();
		if (IsKeyDown(VK_NUMPAD4))
			OnLeft();
		if (IsKeyDown(VK_NUMPAD8))
			OnUp();
		if (IsKeyDown(VK_NUMPAD2))
			OnDown();
		
	}
	CDialog::OnTimer(nIDEvent);
}

void CDivideDlg::OnStaticSpr() 
{
	CPoint pt;
	BOOL b = m_staticSpr.GetCursorOnImage(pt);
	if (!b)
		return;
	m_nX = pt.x;
	m_nY = pt.y;

	int width,height;
	m_staticSpr.GetSize(width,height);
	if (m_nX < 0)
		m_nX = 0;
	if (m_nX > width-1 )
		m_nX = width-1;
	if (m_nY < 0)
		m_nY = 0;
	if (m_nY > height-1)
		m_nY = height-1;
	UpdateData(FALSE);
}

void CDivideDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect r;
	m_staticSpr.GetWindowRect(r);
	CPoint pt;
	GetCursorPos(&pt);
	if (r.PtInRect(pt))
	{
		OnStaticSpr();
	}
	CDialog::OnLButtonDown(nFlags, point);
}

void CDivideDlg::FormatPoint(CPoint& point)
{
	int width = 0;
	int height = 0;
	m_staticSpr.GetSize(width,height);
	if (point.x < 0)
		point.x = 0;
	if (point.y < 0)
		point.y = 0;
	if (point.x >= width)
		point.x = width - 1;
	if (point.y >= height)
		point.y = height - 1;
}

void CDivideDlg::OffsetPoint(CPoint pt)
{
	int index = m_listPoint.GetCurSel();

	if (index != -1)
	{
		CString s;
		m_listPoint.GetText(index,s);
		CPoint point;
		StringToValue(s,point);
		point += pt;
		FormatPoint(point);
		m_listPoint.DeleteString(index);
		ValueToString(point,s);
		int n = m_listPoint.AddString(s);
		m_listPoint.SetCurSel(n);
		UpdatePointArray();
	}
}

void CDivideDlg::OnUp() 
{
	OffsetPoint(CPoint(0,-1));
}

void CDivideDlg::OnLeft() 
{
	OffsetPoint(CPoint(-1,0));
}

void CDivideDlg::OnDown() 
{
	OffsetPoint(CPoint(0,1));
}

void CDivideDlg::OnRight() 
{
	OffsetPoint(CPoint(1,0));
}

void CDivideDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	switch (nChar)
	{
	case VK_NUMPAD6:
		OnRight();
		break;
	case VK_NUMPAD4:
		OnLeft();
		break;
	case VK_NUMPAD8:
		OnUp();
		break;
	case VK_NUMPAD2:
		OnDown();
		break;
	}
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDivideDlg::OnSelchangeDoodadtype() 
{
	UpdateData();
	int i = m_aCB[TD_DOODADTYPE]->GetCurSel();
	UpdateType(i);
	UpdatePointArray();
}

void CDivideDlg::OnSelchangeGeometry() 
{
	UpdatePointArray();
}

void CDivideDlg::OnEditchangeHeight() 
{
	UpdatePointArray();	
}

void CDivideDlg::OnSelchangeHeight() 
{
	UpdatePointArray();	
}

void CDivideDlg::OnDivided() 
{
	// TODO: Add your control notification handler code here
	OnDivide();
}

void CDivideDlg::OnSelchangeObstacle() 
{
	UpdatePointArray();	
}

void CDivideDlg::OnDisplayalpha() 
{
	// TODO: Add your] control notification handler code here
	UpdateData();
	m_staticSpr.SetDisplayAlpha(m_bDisplayAlpha);
}

void CDivideDlg::UpdateType(int i)
{
	switch (i)
	{
	case 0:
	case 1:
		m_aCB[TD_GEOMETRY]->EnableWindow(TRUE);
		break;
	case 2:
	case 3:
		m_aCB[TD_GEOMETRY]->EnableWindow(FALSE);
		//计算w,h
		break;
	}
}

