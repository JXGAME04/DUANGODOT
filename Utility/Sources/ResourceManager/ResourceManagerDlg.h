// ResourceManagerDlg.h : header file
//

#if !defined(AFX_RESOURCEMANAGERDLG_H__63C4FB53_6709_49B4_9E8B_BE2713B1516F__INCLUDED_)
#define AFX_RESOURCEMANAGERDLG_H__63C4FB53_6709_49B4_9E8B_BE2713B1516F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../controls/dirTree.h"
#include "../controls/sprstatic.h"

/////////////////////////////////////////////////////////////////////////////
// CResourceManagerDlg dialog

class CResourceManagerDlg : public CDialog
{
// Construction
public:
	CResourceManagerDlg(CWnd* pParent = NULL);	// standard constructor
/////////////////////////////////////////////////////////////////////////////
//create
	bool m_bCreate;

//tree
	CString m_strDesRoot;
	CString m_strSrcRoot;
	CString m_strNetRoot;
	BOOL CopyFileToNet(LPCSTR szFile);

/////////////////////////////////////////////////////////////////////////////
//option
//描述了转化什么类型的文件，文件的切分，文件最后生成的方向数
	struct stOption
	{
		CString strKeyword;
		CString strExt;
		int width,height;
		int nDirection;
		int nSort;//0 sort by name, 1 sort by *_0,*_1;
		CString sIcon;
		CString sImageFolder;
		CString sType;
		CString sGameType;
		int		nFullImage;
	};
	enum {MAX_ITEM = 64};
	int m_nOption;
	stOption m_aOption[MAX_ITEM];
	void LoadOption();
	void ConstructTrees();
	BOOL GetOptionByKeyword(LPCSTR keyword,stOption& item);

	void SetStaticFrame(int nDirection,int nFrame);
	void UpdatePlaneInfo(int index);
	void ComboxValidate();
	enum {MAX_COMBOX = 32};
	CComboBox*	m_aCB[MAX_COMBOX];
	BOOL*		m_aAll[MAX_COMBOX];

// Dialog Data
	//{{AFX_DATA(CResourceManagerDlg)
	enum { IDD = IDD_RESOURCEMANAGER_DIALOG };
	CComboBox	m_cbAniSpeed;
	CComboBox	m_cbAniLoop;
	CComboBox	m_cbCenterSetting;
	CSpinButtonCtrl	m_spinY;
	CSpinButtonCtrl	m_spinX;
	CComboBox	m_cbVersort;
	CComboBox	m_cbKind;
	CComboBox	m_cbTextureUse;
	CComboBox	m_cbSave;
	CComboBox	m_cbCenterY;
	CComboBox	m_cbCenterX;
	CComboBox	m_cbPlaneType;
	CComboBox	m_cbRadius;
	CComboBox	m_cbObstacleType;
	CComboBox	m_cbLighted;
	CComboBox	m_cbLayer;
	CComboBox	m_cbHeight;
	CComboBox	m_cbAngleZ;
	CComboBox	m_cbAngleXY;
	CComboBox	m_cbLighting;
	BOOL	m_bVersort;
	BOOL	m_bKind;
	BOOL	m_bTextureUse;
	BOOL	m_bSave;
	BOOL	m_bCenterY;
	BOOL	m_bCenterX;
	BOOL	m_bPlaneType;
	BOOL	m_bRadius;
	BOOL	m_bObstacleType;
	BOOL	m_bLighted;
	BOOL	m_bLayer;
	BOOL	m_bHeight;
	BOOL	m_bAngleZ;
	BOOL	m_bAngleXY;
	BOOL	m_bLighting;
	CDirTree m_treeDes;
	CDirTree m_treeSrc;
	CSpinButtonCtrl	m_spinSpeed;
	CSpinButtonCtrl	m_spinFrame;
	CSpinButtonCtrl	m_spinDirection;
	CSpinButtonCtrl	m_spinAnimation;
	CSprStatic	m_staticSpr;
	UINT	m_nFrame;
	CString	m_strAnimation;
	BOOL	m_bAutoPlay;
	UINT	m_nDirection;
	UINT	m_nSpeed;
	BOOL	m_bShowCenter;
	BOOL	m_bAll;
	BOOL	m_bAlphaFormat;
	BOOL	m_bAlphaSet;
	int		m_nCenterSetting;
	BOOL	m_bAniSpeed;
	BOOL	m_bRemoveShadow;
	BOOL	m_bAniLoop;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResourceManagerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
/////////////////////////////////////////////////////////////////////////////
//file info

/////////////////////////////////////////////////////////////////////////////
//Message handle
	void OnDbSelectFile(LPCSTR sz,CDirTree* pTree);
	void OnSelectFile(LPCSTR sz,CDirTree* pTree);
	CString m_strSelectFile;
	CString m_strSelectFolder;

	void SaveAllFolder(LPCSTR szFile);
	void SaveAllFile(LPCSTR szFile);

//convert
	void AlphaFormatTga(LPCSTR szPath);
	void AlphaFormatTgaFile(LPCSTR szFile);

	int ConvertTgb(LPCSTR szPath);
	
	BOOL ConvertExt(LPCSTR szPath,LPCSTR szExt);
	void ConvertExtFile(LPCSTR szFile, LPCSTR sPath,LPCSTR szExt);
	BOOL ConvertPsd(LPCSTR szPath);
	BOOL ConvertTga(LPCSTR szPath);

	void ConvertFolder(LPCSTR szFolder);
	void ConvertFolderByOption(LPCSTR szFolder,stOption& item,bool bDelete);
	HRESULT SaveImageFile(iImage* p,LPCSTR szImage,stOption* pOption);
	
	void GetSprFileNameByFolder(LPCSTR szFolder, CString& strSpr);

//show image
	void ShowImage(LPCSTR sz);

//show property
	void CreateTxtFile(const CString& strFile,const CString& strIcon,const CString& strType, const CString& strGameType, const CString& strSpr,int frames, int directions);
	CString GetTableDefaultValue(int i, LPCSTR szType);
	void OnProperty(LPCSTR sz);

// Implementation
	void OnEditchangeCenter();
// center set
	CPoint GetCenterByAlpha();


	void OnIni2txt(LPCSTR szIni);
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CResourceManagerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnChangeFrame();
	afx_msg void OnChangeDirection();
	afx_msg void OnAutoplay();
	afx_msg void OnStaticSpr();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnShowcenter();
	afx_msg void OnSave();
	afx_msg void OnEditchangeCenterx();
	afx_msg void OnEditchangeCentery();
	afx_msg void OnDestroy();
	afx_msg void OnSpr2txt();
	afx_msg void OnSelchangePlanetype();
	afx_msg void OnGetcenter();
	afx_msg void OnLog();
	afx_msg void OnIni2txt();
	afx_msg void OnLocate();
	afx_msg void OnAddsprinfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

void GetSprFileNameByFolder(LPCSTR szFolder, CString& strSpr);
void GetTxtFileNameByImage(LPCSTR szImage, CString& sTxt);
void GetRelativeFile(LPCSTR szImage,CString& sRelative);
void GetDesImageFile(LPCSTR szImage,CString& sRelative,CString& sPathName);

enum CenterEnum{CENTER_ALPHA, CENTER_TOPEST, CENTER_BOTTOMEST, CENTER_BOTTOMMIDDLE,CENTER_IMAGECENTER,};
BOOL GetSprFile(CString& strSpr,LPCSTR szIni,LPCSTR szSection);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESOURCEMANAGERDLG_H__63C4FB53_6709_49B4_9E8B_BE2713B1516F__INCLUDED_)
