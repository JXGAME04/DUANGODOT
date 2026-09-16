// MapConvertDlg.h : header file
//

#if !defined(AFX_MAPCONVERTDLG_H__8727E092_1D51_47BB_9E59_D22AE8568179__INCLUDED_)
#define AFX_MAPCONVERTDLG_H__8727E092_1D51_47BB_9E59_D22AE8568179__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMapConvertDlg dialog
#include "iColor.h"
#include "iGraphics.h"

class CFileFindEx;

#include <memory>

struct stObstacle;

struct stObstacleTile
{
	long aTile[c_nWidthRegion][c_nHeightRegion];
};
struct stTrapTile
{
	long aTile[c_nWidthRegion][c_nHeightRegion];
};

typedef int (*mc_doonce)(LPCSTR szFile);

class CMapConvertDlg : public CDialog
{
// Construction
public:
	CMapConvertDlg(CWnd* pParent = NULL);	// standard constructor
	~CMapConvertDlg(){ClearObstacle();ClearTrap();}

	void OutputProgress(LPCSTR sz){SetDlgItemText(IDC_NOW_FILE,sz);}
	//Common
	BOOL VerifyFile();

	void ReplaceIni(const CString& strFile,const CString&  strSrc,const CString& strDes);
	void ReplaceFolder(const CString& strFile,const CString&  strSrc,const CString& strDes);
	
	BOOL ValidateIni(const CString& strFile);
	BOOL ValidateFolder(const CString& strFile);

	void ClearFolder(const CString& strFile);
	void ClearBin(const CString& strFile);

	void ClearnullIni(const CString& strFile);
	void ClearnullFolder(const CString& strFile);

	void DeleteUnvalidRegionFolder();

	cPoint GetOffsetGround();
	BOOL OffsetCamera();
	void OffsetFolder(LPCSTR szFolder);
	void OffsetFile(LPCSTR szFile);

	enum {MAX_REGION = 400,};

	stObstacleTile* m_aRegion[MAX_REGION][MAX_REGION];
	void ClearObstacle();
	void ModifyObstacle(int xRegion,int yRegion,int offx,int offy,ObstacleEnum e);
	void SaveObstacle();
	void SaveObstacleBmp(LPCSTR szTxt);
	void GetRegionObstacleFile(int x, int y, CString& strRegion);
	BOOL ConvertAObstacle(stObstacle& obs,int xRegion,int yRegion);
	void ConvertObstacleFile(int xRegion,int yRegion,const CString& szFile);
	void ConvertObstacleFolder(const CString& szFolder);

	stTrapTile* m_aTrap[MAX_REGION][MAX_REGION];
	std::vector<CString> m_aTrapScript;
	void ClearTrap();
	void ModifyTrap(int xRegion,int yRegion,int offx,int offy,long lTrap);
	void SaveTrap();
	void SaveTrapBmp(LPCSTR szTxt);
	void GetRegionTrapFile(int x, int y, CString& strRegion);
	BOOL ConvertATrap(stPlaneParam& trap,int xRegion,int yRegion);
	void ConvertTrapFile(int xRegion,int yRegion,const CString& szFile);
	void ConvertTrapFolder(const CString& szFolder);

//	auto_ptr<CFileFindEx> m_pFindRegion;
//	BOOL FindRegion(const CString& strFolder);
//	BOOL FindNextRegion();
	void PackFile(mc_doonce fun, LPCSTR szRaw, LPCSTR szPack, const CString& strFolder, const CRect& rect);
	void FindFileUnderMap(mc_doonce fun,LPCSTR szFindFile);
	void FindFolderUnderMap(mc_doonce fun);
	void SetMapVerson(long l);
	void FindEachRegionFolder(mc_doonce fun);
	void FindEachRegionUpFolder(mc_doonce fun);

private:
	bool ComposeGroundFile(const char* pFile1, const char* pFile2, const char* pFile3);
	bool ConvertBuildinObjFile(const char* pFile1, const char* pOldLightFile, const char* pFile2);
	bool ConvertNpcFileBin(const char* pRootPath, const char* pFile1, const char* pFile2, bool bServer);
	bool ConvertObjFileBin(const char* pRootPath, const char* pFile1, const char* pFile2, bool bServer, const char* pBioFile);
	bool ConvertTrapFileBin(const char*pRootPath, const char* pFile1, const char* pFile2, int nLeft, int nTop);
	bool LoadCoverGroundObj(const char* pFile, unsigned int& nCount,
				struct KSPRCoverGroundObj*& pObjects);
	bool LoadAAboveGroundObject(struct KBuildinObj* pObject,
			class KIniFile* pFile, int nObject);
	bool LoadALight(struct KBuildInLightInfo* pLight, class KIniFile* pFile, int nObject);
	bool LoadANpc(const char* pRootPath, struct KSPNpc* pNpc, class KIniFile* pFile, int nNpc, bool bServer);
	bool LoadAObj(const char* pRootPath, struct KSPObj* pObj, class KIniFile* pFile, int nObj, KBuildinObj* pBio, bool bServer);
	bool LoadATrap(/*const char* pRootPath,*/ struct KSPTrap* pTrap, class KIniFile* pFile, int nTrap, int nLeft, int nTop);

	void BinWorFile(CString& strFile,CRect rect);
	void BinCameraFile(CString& strFile);

	BOOL CheckFileIncluded(const char* pszPath, const char* pszFile);

	void DeleteRegionFolder(const char* pszPath);

private:

// Dialog Data
	//{{AFX_DATA(CMapConvertDlg)
	enum { IDD = IDD_MAPCONVERT_DIALOG };
	CString	m_strFile;
	CString	m_strDes;
	CString	m_strSrc;
	int		m_nRight;
	int		m_nBottom;
	int		m_nEnlarge;
	UINT	m_nDivBottom;
	UINT	m_nDivTop;
	UINT	m_nDivRight;
	UINT	m_nDivLeft;
	CString	m_strNewMap;
	BOOL	m_bGround;
	BOOL	m_bPlane;
	BOOL	m_bNpc;
	BOOL	m_bObj;
	BOOL	m_bTrap;
	BOOL	m_bObstacle;
	BOOL	m_bLight;
	CString	m_strArtmap;
	CString	m_strDesignMap;
	//}}AFX_DATA
	CString m_strInvalid;

	bool	m_bBatMap;
	bool	m_bStep9;		

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapConvertDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void GetRect(CRect& rc);
	void LoadIni();
	void SaveIni();
	void GetFile(CString& m_strFile);
	
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMapConvertDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnReplace();
	afx_msg void OnMap();
	afx_msg void OnValidate();
	afx_msg void OnBrowse();
	afx_msg void OnClear();
	afx_msg void OnClearnull();
	afx_msg void OnIni();
	afx_msg void OnOffset();
	afx_msg void OnObstacle();
	afx_msg void OnBmp();
	afx_msg void OnImage();
	afx_msg void OnLinkhor();
	afx_msg void OnAutoground();
	afx_msg void OnLength();
	afx_msg void OnAlpha();
	afx_msg void OnBinary();
	afx_msg void OnAddMap();
	afx_msg void OnRemoveMap();
	afx_msg void OnStartBat();
	afx_msg void OnZiptemplate();
	afx_msg void OnZipmap();
	afx_msg void OnFillsprinfo();
	afx_msg void OnConvercoord();
	afx_msg void OnCheckspr();
	afx_msg void OnFiletoindex();
	afx_msg void OnZipground();
	afx_msg void OnDiv();
	afx_msg void OnZipnpc();
	afx_msg void OnTrap();
	afx_msg void OnConvernpc();
	afx_msg void OnDesignmap();
	afx_msg void OnArtmap();
	afx_msg void OnJoinArt();
	afx_msg void OnJoinDesign();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void GetAllFiles();
};

int GetFileLength(LPCSTR szFile);
BOOL GetRegionSize2(LPCSTR szName, int& sx, int& sy);
BOOL GetRegionSize(LPCSTR szName, int& sx, int& sy);
BOOL GetFullFolderRegionSize(LPCSTR sName, int& sx, int& sy);
BOOL GetFullFileRegionSize(LPCSTR  sName, int& sx, int& sy);
BOOL IsAnyInDir(const CString& sName);
BOOL IsAnyIniInDir(const CString& sName,LPCSTR strExt);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPCONVERTDLG_H__8727E092_1D51_47BB_9E59_D22AE8568179__INCLUDED_)
