#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"
#include "fileop.h"
#include "planedoodad.h"

bool	s_bComboItem[3]  = { false, false, false };
HWND	s_hWnd = NULL;

void CMapConvertDlg::OnDesignmap() 
{
	// TODO: Add your control notification handler code here
	GetFile(m_strDesignMap);
}

void CMapConvertDlg::OnArtmap() 
{
	// TODO: Add your control notification handler code here
	GetFile(m_strArtmap);	
}

BOOL VerifyName(const CString& s1,const CString& s2)
{
	if (s1.IsEmpty() || s2.IsEmpty())
		return FALSE;
	CString sPath,sArt,sDesign,sExt;
	AppParsePathNameExt(s1,sPath,sArt,sExt);
	AppParsePathNameExt(s2,sPath,sDesign,sExt);
	if (sArt != sDesign)
	{
		AfxMessageBox("地图不一致！");
		return FALSE;
	}
	return TRUE;
}

CString g_strSrc,g_strDes;
CString g_aStringDesign[] = 
{
	FILE_NPC,
	FILE_BOX,
	FILE_TRAP,
	"",
};

CString g_aStringArt[] = 
{
	FILE_OBSTACLE,
	FILE_LIGHT,
	FILE_PLANEVER,
	FILE_TERRAIN,
	FILE_PLANEHORBELOWVER,
//	FILE_PLANEVERBELOWGROUND,
	"",
};
CString g_aString[3] =
{
	FILE_NPC,
	FILE_BOX,
	FILE_TRAP,
};

int on_joinart(LPCSTR szFolder)
{

	for (int i=0; i < 3; i++)
	{
		if (s_bComboItem[i] == false)
			continue;
		CString strName = CString(szFolder) + "\\" + g_aString[i];
		CString strDes = g_strDes + strName.Right(strName.GetLength() - g_strSrc.GetLength());

		HANDLE	hSrcFile = CreateFile(strName, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hSrcFile != INVALID_HANDLE_VALUE)
		{
			bool bCopy = false;
			FILETIME	SrcTime, DestTime;
			HANDLE	hDestFile = CreateFile(strDes, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hDestFile != INVALID_HANDLE_VALUE)
			{
				GetFileTime(hSrcFile, NULL, NULL, &SrcTime);
				GetFileTime(hDestFile, NULL, NULL, &DestTime);
				if (CompareFileTime(&SrcTime, &DestTime) > 0)
					bCopy = true;
				CloseHandle(hDestFile);
				hDestFile = NULL;
			}
			else
				bCopy = true;
			CloseHandle(hSrcFile);
			hSrcFile = NULL;
			if (bCopy)
			{
				::SetDlgItemText(s_hWnd, IDC_NOW_FILE, strDes);
				DeleteFile(strDes);
				if (CopyFile(strName, strDes, false) == 0)
				{
					strName = "复制失败:" + strDes;
					::SetDlgItemText(s_hWnd, IDC_NOW_FILE, strName);
				}
			}
		}
	}
	return 0;
}

void CMapConvertDlg::OnJoinArt() 
{
	if (!VerifyName(m_strArtmap,m_strDesignMap))
	{
		return ;
	}
	//copy design's npc obj trap
	CString s = m_strFile;
	m_strFile = m_strDesignMap;
	
	g_strSrc = m_strDesignMap.Left(m_strDesignMap.GetLength() - 4);
	g_strDes = m_strArtmap.Left(m_strArtmap.GetLength() - 4);

	UpdateData(FALSE);
	s_bComboItem[0] = (IsDlgButtonChecked(IDC_COMBO_NPC) == BST_CHECKED);
	s_bComboItem[1] = (IsDlgButtonChecked(IDC_COMBO_OBJ) == BST_CHECKED);
	s_bComboItem[2] = (IsDlgButtonChecked(IDC_COMBO_TRAP) == BST_CHECKED);
	
	s_hWnd = m_hWnd;

	FindEachRegionFolder(on_joinart);
	m_strFile = s;

	UpdateData(FALSE);
	SetDlgItemText(IDC_NOW_FILE, "合并操作结束。");
	MessageBox("合并操作结束。", "向执行艰巨合并任务的策划致敬！");
	s_hWnd = NULL;
}

void CMapConvertDlg::OnJoinDesign() 
{
	if (!VerifyName(m_strArtmap,m_strDesignMap))
	{
		return ;
	}
	//copy design's npc obj trap
	CString s = m_strFile;
	m_strFile = m_strArtmap;
	
	g_strSrc = m_strArtmap.Left(m_strArtmap.GetLength() - 4);
	g_strDes = m_strDesignMap.Left(m_strDesignMap.GetLength() - 4);
	
	UpdateData(FALSE);
	s_bComboItem[0] = (IsDlgButtonChecked(IDC_COMBO_NPC) == BST_CHECKED);
	s_bComboItem[1] = (IsDlgButtonChecked(IDC_COMBO_OBJ) == BST_CHECKED);
	s_bComboItem[2] = (IsDlgButtonChecked(IDC_COMBO_TRAP) == BST_CHECKED);

	s_hWnd = m_hWnd;
	FindEachRegionFolder(on_joinart);
	m_strFile = s;

	UpdateData(FALSE);
	SetDlgItemText(IDC_NOW_FILE, "合并操作结束。");
	MessageBox("合并操作结束。", "向执行艰巨合并任务的策划致敬！");
	s_hWnd = NULL;
}
