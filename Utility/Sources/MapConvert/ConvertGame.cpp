#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"
#include "iItem.h"
#include "iGraphics.h"
#include "..\controls\FileFindEx.h"
#include "math.h"
#include <iomanip>
#include <fstream>
#include <shlwapi.h>
#include "iImage.h"
#include "foreach.h"
#include "KSpbioTemplate.h"
#include "KEngine.h"
#include "KTabFile.h"
#include "izip.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static KTabFile	l_NpcSFile;
static int		l_NpcSFileKindColumn = 0;
static int		l_NpcSFileNameColumn = 0;
static int		l_NpcSFileCampColumn = 0;
static int		l_NpcSFileSeriesColumn = 0;
static int		l_NpcSFileHeadImageColumn = 0;
static int		l_NpcSFileScriptColumn = 0;

static KTabFile	l_ObjDataFile;
static int		l_ObjDataFileKindColumn = 0;
static int		l_ObjDataFileDirColumn = 0;
static int		l_ObjDataFileScriptColumn = 0;
static int		l_ObjDataImgNameColomn = 0;

static KTabFile	l_ScriptListFile;
static int		l_ScriptListFileIdColumn = 0;
static int		l_ScriptListFileNameColumn = 0;

static bool		l_bReportScriptNameError = false;

static POINT	s_nCurrentRegionIndex;

static int	l_bIgnoreFileNameLenError = false;
bool FileNameExceed(HWND hWnd, const char* pFile)
{
	if (l_bIgnoreFileNameLenError)
	{
		CString error;
		error.Format("文件名超长:%s", pFile);
		SetDlgItemText(hWnd, IDC_NOW_FILE, error);
		return true;
	}
	int nRet = MessageBox(hWnd, "文件名超过允许长度，\nAbort退出, Retry继续, Ignore忽略全部这种错误？",
		pFile, MB_ABORTRETRYIGNORE | MB_ICONERROR);
	if (nRet == IDABORT)
		return false;
	if (nRet == IDIGNORE)
		l_bIgnoreFileNameLenError = true;
	return true;
}

#define	REGION_GROUND_FILE			"Region.Dat"			//地表信息文件
#define	REGION_GROUND_COVER_FILE	"PlaneHorBelowVer.ini"	//紧贴地表面的东西的文件
#define	REGION_BUILDIN_OBJ_OLD_FILE	"PlaneVer.ini"			//场景地图内建对象集合信息文件
#define	REGION_LIGHT_OBJ_OLD_FILE	"Light.ini"				//光源信息文件
#define REGION_NPC_OLD_FILE			"Npc.ini"				//场景地图内npc信息文件
#define REGION_OBJ_OLD_FILE			"Box.ini"				//场景地图内Obj信息文件
#define	REGION_TRAP_OLD_FILE		"Trap.ini"
#include "../../../SwordOnline/Sources/Core/Src/Scene/SceneDataDef.h"
#include "../../../SwordOnLine/Sources/Engine/Src/KEngine.h"

void SM_GetLineEndPoint(float x1, float y1, float fAngleXY, int nWidth, int nHeight, float& x2, float& y2);
void SM_GetLineEndPointAndNodicalY(float x1, float y1, float& x2, float& y2,
			int nWidth, float& fAngleXY, float& nNodicalY);
void SM_GetPointFromFloatSting32(char* pString, float& x, float& y, float& z);

#define	ELEM_FILE_COUNT	8

static const char* s_pElemFileName[ELEM_FILE_COUNT] = 
{	REGION_OBSTACLE_FILE, REGION_TRAP_FILE,
	REGION_NPC_FILE_SERVER, REGION_NPC_FILE_CLIENT,
	REGION_OBJ_FILE_SERVER, REGION_OBJ_FILE_CLIENT,
	REGION_GROUND_LAYER_FILE, REGION_BUILDIN_OBJ_FILE
};
static const int	s_nElemFileServerIndex [ELEM_FILE_COUNT] = 
{
	REGION_OBSTACLE_FILE_INDEX,	REGION_TRAP_FILE_INDEX,
	REGION_NPC_FILE_INDEX, -1,
	REGION_OBJ_FILE_INDEX, -1,
	-1, -1
};
static	const int	s_nElemFileClientIndex [ELEM_FILE_COUNT] = 
{
	REGION_OBSTACLE_FILE_INDEX,	REGION_TRAP_FILE_INDEX,
	-1, REGION_NPC_FILE_INDEX,
	-1, REGION_OBJ_FILE_INDEX,
	REGION_GROUND_LAYER_FILE_INDEX, REGION_BUILDIN_OBJ_FILE_INDEX
};

static bool CombinRegionFiles(const char* pszFolder, int nRegionH)
{
	char	szFile[256];
	CFile	ServerFile, ClientFile, ElemFile;
	void*	pBuffer = NULL;

	unsigned int uServerOffset, uClientOffset, uSize, i;

	KCombinFileSection		ServerSections[REGION_ELEM_FILE_COUNT] = { 0 };
	KCombinFileSection		ClientSections[REGION_ELEM_FILE_COUNT] = { 0 };

	#define	MAX_BUFFER_SIZE		0x5000
	pBuffer = malloc(MAX_BUFFER_SIZE);
	if (pBuffer == NULL)
	{
		MessageBox(NULL, "分配缓冲区失败。", "失败", MB_OK | MB_ICONERROR);
		return false;
	}
	sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_SERVER, pszFolder, nRegionH);
	if (!ServerFile.Open(szFile, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		goto OPEN_FILE_ERROR;
	}

	sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_CLIENT, pszFolder, nRegionH);
	if (!ClientFile.Open(szFile, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		goto OPEN_FILE_ERROR;
	}
	
	uServerOffset = REGION_ELEM_FILE_COUNT;
	ServerFile.Write(&uServerOffset, sizeof(unsigned int));
	ServerFile.Write(&ServerSections[0], sizeof(ServerSections));

	uClientOffset = REGION_ELEM_FILE_COUNT;
	ClientFile.Write(&uServerOffset, sizeof(unsigned int));
	ClientFile.Write(&ClientSections[0], sizeof(ClientSections));
	
	uServerOffset = 0;
	uClientOffset = 0;

	for (i = 0; i < ELEM_FILE_COUNT; i++)
	{
		int nSIndex = s_nElemFileServerIndex[i];
		int nCIndex = s_nElemFileClientIndex[i];
		sprintf(szFile,  "%s\\%03d_%s", pszFolder, nRegionH, s_pElemFileName[i]);
		unsigned dword = GetFileAttributes(szFile);
	
		if ((dword & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			if (ElemFile.Open(szFile, CFile::modeRead))
			{
				uSize = ElemFile.GetLength();
				if (nSIndex >= 0)
				{
					ServerSections[nSIndex].uLength = uSize;
					ServerSections[nSIndex].uOffset = uServerOffset;
					uServerOffset += uSize;
				}
				if (nCIndex >= 0)
				{
					ClientSections[nCIndex].uLength = uSize;
					ClientSections[nCIndex].uOffset = uClientOffset;
					uClientOffset += uSize;
				}
				while (uSize > MAX_BUFFER_SIZE)
				{
					ElemFile.Read(pBuffer, MAX_BUFFER_SIZE);
					if (nSIndex >= 0)
						ServerFile.Write(pBuffer, MAX_BUFFER_SIZE);
					if (nCIndex >= 0)
						ClientFile.Write(pBuffer, MAX_BUFFER_SIZE);
					uSize -= MAX_BUFFER_SIZE;
				}
				if (uSize)
				{
					ElemFile.Read(pBuffer, uSize);
					if (nSIndex >= 0)
						ServerFile.Write(pBuffer, uSize);
					if (nCIndex >= 0)
						ClientFile.Write(pBuffer, uSize);
				}
				ElemFile.Close();
			}
			else
				goto OPEN_FILE_ERROR;
		}		
	}

	free(pBuffer);
	for (i = 0; i < ELEM_FILE_COUNT; i++)
	{
		sprintf(szFile,  "%s\\%03d_%s", pszFolder, nRegionH, s_pElemFileName[i]);
		DeleteFile(szFile);
	}
	if (uServerOffset)
	{
		ServerFile.Seek(sizeof(unsigned int), CFile::begin);
		ServerFile.Write(&ServerSections[0], sizeof(ServerSections));
		ServerFile.Flush();
	}
	else
	{
		ServerFile.Abort();
		sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_SERVER, pszFolder, nRegionH);
		DeleteFile(szFile);
	}
	ServerFile.Close();

	if (uClientOffset)
	{
		ClientFile.Seek(sizeof(unsigned int), CFile::begin);
		ClientFile.Write(&ClientSections[0], sizeof(ClientSections));
		ClientFile.Flush();
	}
	else
	{
		ClientFile.Abort();
		sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_CLIENT, pszFolder, nRegionH);
		DeleteFile(szFile);
	}
	ClientFile.Close();
	return true;

OPEN_FILE_ERROR:
	free(pBuffer);
	::MessageBox(NULL, szFile, "打开文件错误", MB_OK | MB_ICONERROR);
	ServerFile.Abort();
	sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_SERVER, pszFolder, nRegionH);
	DeleteFile(szFile);
	ClientFile.Abort();
	sprintf(szFile, "%s\\%03d_"REGION_COMBIN_FILE_NAME_CLIENT, pszFolder, nRegionH);
	DeleteFile(szFile);
	ElemFile.Abort();
	return false;	
}

void CMapConvertDlg::OnBinary()
{
	CWaitCursor cursor;	
	if (!VerifyFile())
		return;
	
	OnClear();
//	if (m_bObstacle)
		OnObstacle();
//	if (m_bPlane)
		OnImage();

	l_bIgnoreFileNameLenError = false;
	CString MapRootPath, RegionPath, SubPath;
	int	nLen = m_strFile.GetLength();
	int pos = m_strFile.ReverseFind('\\');
	MapRootPath = m_strFile.Left(pos);
	pos = MapRootPath.ReverseFind('\\');
	MapRootPath.SetAt(pos, '\0');
	g_SetRootPath(MapRootPath.GetBuffer(pos + 1));

	MapRootPath = m_strFile.Left(nLen - 4);//所选地图目录

	CString	File1, File2, File3;
	bool	bCreatedPath;
	CString ;
	CRect	rc;
	GetRect(rc);	//得到地图的有效范围

	{
		AppGetWorkingFolder(File2);
		File2 += "\\Settings\\NpcS.txt";
		if (!l_NpcSFile.Load((char*)(const char*)File2))
		{
			MessageBox("未能载入\\Settings\\NpcS.txt文件", "错误", MB_OK | MB_ICONERROR);
			return;
		}

		l_NpcSFileKindColumn = l_NpcSFile.FindColumn("Kind");
		l_NpcSFileNameColumn = l_NpcSFile.FindColumn("Name");
		l_NpcSFileCampColumn = l_NpcSFile.FindColumn("Camp");
		l_NpcSFileSeriesColumn = l_NpcSFile.FindColumn("Series");
		l_NpcSFileHeadImageColumn = l_NpcSFile.FindColumn("HeadImage");
		l_NpcSFileScriptColumn = l_NpcSFile.FindColumn("ActionScript");
		if (l_NpcSFileKindColumn < 1 || l_NpcSFileNameColumn < 1 ||
			l_NpcSFileCampColumn < 1 ||	l_NpcSFileSeriesColumn < 1 ||
			l_NpcSFileHeadImageColumn < 1 || l_NpcSFileScriptColumn < 1)
		{
			MessageBox("\\Settings\\NpcS.txt文件内容有错，找不到必须的列数据。", "错误", MB_OK | MB_ICONERROR);
			return;
		}
	}

	{
		AppGetWorkingFolder(File2);
		File2 += "\\Settings\\Obj\\ObjData.txt";
		if (!l_ObjDataFile.Load((char*)(const char*)File2))
		{
			MessageBox("未能载入\\Settings\\Obj\\ObjData.txt", "错误", MB_OK | MB_ICONERROR);
			return;
		}

		l_ObjDataFileKindColumn = l_ObjDataFile.FindColumn("Kind");
		l_ObjDataFileDirColumn = l_ObjDataFile.FindColumn("ImageCurDir");
		l_ObjDataFileScriptColumn = l_ObjDataFile.FindColumn("ScriptName");
		l_ObjDataImgNameColomn = l_ObjDataFile.FindColumn("ImageName");
		if (l_ObjDataFileKindColumn < 1 ||	l_ObjDataFileDirColumn < 1 ||
			l_ObjDataFileScriptColumn < 1 || l_ObjDataImgNameColomn < 1)
		{
			MessageBox("\\Settings\\Obj\\ObjData.txt文件内容有错，找不到必须的列数据。", "错误", MB_OK | MB_ICONERROR);
			return;
		}
	}

	{
		File2 = MapRootPath + "\\ScriptList.txt";
		if (!l_ScriptListFile.Load((char*)(const char*)File2))
		{
			MessageBox("未能载入ScriptLists.txt", "错误", MB_OK | MB_ICONERROR);
			return;
		}

		l_ScriptListFileIdColumn = l_ScriptListFile.FindColumn("TrapId");
		l_ScriptListFileNameColumn = l_ScriptListFile.FindColumn("Script");
		if (l_ScriptListFileIdColumn < 1 || l_ScriptListFileNameColumn < 1)
		{
			MessageBox("ScriptList.txt文件内容有错，找不到必须的列数据。", "错误", MB_OK | MB_ICONERROR);
			return;
		}
		l_bReportScriptNameError = true;
	}

	for (int v = rc.top; v <= rc.bottom; v++)
	{	
		//按区域行转换，同横行的区域合并为同个目录
		bCreatedPath = false;
		for (int h = rc.left; h <= rc.right; h++)
		{
			RegionPath.Format("%s\\%03d_%03d\\", MapRootPath, h, v);
			if (!PathIsDirectory(RegionPath))
				continue;
			if (bCreatedPath == false)
			{
				SubPath.Format("%s\\v_%03d", MapRootPath, v);
				if (PathIsDirectory(SubPath))
					bCreatedPath = true;
				else if (CreateDirectory(SubPath, NULL))
					bCreatedPath = true;
				else
				{
					MessageBox(SubPath, "创建目录失败，转换操作未完成！", MB_OK | MB_ICONERROR);
					goto ERROR_EXIT;
				}
			}

			s_nCurrentRegionIndex.x = h;
			s_nCurrentRegionIndex.y = v;

			{	//把地表文件与紧贴地表的对象的文件合起来				
				File1 = RegionPath + REGION_GROUND_FILE;
				File2 = RegionPath + REGION_GROUND_COVER_FILE;
				File3.Format("%s\\%03d_%s", SubPath, h, REGION_GROUND_LAYER_FILE);
				SetDlgItemText(IDC_NOW_FILE, File3);
				if (!ComposeGroundFile(File1, File2, File3))
					goto ERROR_EXIT;
			}

			{
				//转内建对象必须在转obj之前
				//把立面对象集合的文件转为二进制版
				File1 = RegionPath + REGION_BUILDIN_OBJ_OLD_FILE;
				File3 = RegionPath + REGION_LIGHT_OBJ_OLD_FILE;
				File2.Format("%s\\%03d_%s", SubPath, h, REGION_BUILDIN_OBJ_FILE);
				SetDlgItemText(IDC_NOW_FILE, File2);
				if (!ConvertBuildinObjFile(File1, File3, File2))
					goto ERROR_EXIT;

				//把obj对象集合的文件转为二进制版
				File1 = RegionPath + REGION_OBJ_OLD_FILE;
				File3.Format("%s\\%03d_%s", SubPath, h, REGION_OBJ_FILE_SERVER);
				SetDlgItemText(IDC_NOW_FILE, File3);
				if (!ConvertObjFileBin(MapRootPath, File1, File3, true, File2))
					goto ERROR_EXIT;
				File3.Format("%s\\%03d_%s", SubPath, h, REGION_OBJ_FILE_CLIENT);
				SetDlgItemText(IDC_NOW_FILE, File3);
				if (!ConvertObjFileBin(MapRootPath, File1, File3, false, File2))
					goto ERROR_EXIT;
			}

			{	//复制障碍文件
				File1 = RegionPath + REGION_OBSTACLE_FILE;
				File2.Format("%s\\%03d_%s", SubPath, h, REGION_OBSTACLE_FILE);
				SetDlgItemText(IDC_NOW_FILE, File2);
				MoveFile(File1, File2);
			}

			{
				//把npc对象集合的文件转为二进制版
				File1 = RegionPath + REGION_NPC_OLD_FILE;
				File2.Format("%s\\%03d_%s", SubPath, h, REGION_NPC_FILE_SERVER);
				SetDlgItemText(IDC_NOW_FILE, File2);
				if (!ConvertNpcFileBin(MapRootPath, File1, File2, true))
					goto ERROR_EXIT;
				File2.Format("%s\\%03d_%s", SubPath, h, REGION_NPC_FILE_CLIENT);
				SetDlgItemText(IDC_NOW_FILE, File2);
				if (!ConvertNpcFileBin(MapRootPath, File1, File2, false))
					goto ERROR_EXIT;
			}

			{
				File1 = RegionPath + REGION_TRAP_OLD_FILE;
				File2.Format("%s\\%03d_%s", SubPath, h, REGION_TRAP_FILE);
				SetDlgItemText(IDC_NOW_FILE, File2);
				if (!ConvertTrapFileBin(MapRootPath, File1, File2,
					h * 512, v * 1024))
					goto ERROR_EXIT;
			}

			if (!CombinRegionFiles(SubPath, h))
				goto ERROR_EXIT;

			if (m_bStep9)
				DeleteRegionFolder(RegionPath);
		}
	}

	l_NpcSFile.Clear();
	l_ObjDataFile.Clear();
	l_ScriptListFile.Clear();
	if (m_bBatMap == false)
		MessageBox("地图转换二进制版操作结束。", "操作结束", MB_OK);
	return;

ERROR_EXIT:
	m_bBatMap = false;
	l_NpcSFile.Clear();
	l_ObjDataFile.Clear();
	l_ScriptListFile.Clear();
}

bool CMapConvertDlg::ConvertTrapFileBin(const char*pRootPath, const char* pFile1, const char* pFile2, int nLeft, int nTop)
{
	CFile		BinFile;
	KIniFile	TrapFile;

	KTrapFileHead	Head = { 0 };
	KSPTrap			Trap = { 0 };
	unsigned int	TrapMap[32][16];
	bool			bHeveTrap = false;

	unsigned dword = GetFileAttributes(pFile1);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
	{
		DeleteFile(pFile2);
		CString error;
		error.Format("不存在文件:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		return true;
	}

	if (TrapFile.Load(pFile1) == FALSE)
	{
		MessageBox(pFile1, "打开文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!BinFile.Open(pFile2, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		MessageBox(pFile2, "创建文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	//获得对象数目
	int		nTotalCount;
	TrapFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
	if (nTotalCount <= 0)
	{
		CString error;
		error.Format("%s中无Trap对象.", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		BinFile.Abort();
		DeleteFile(pFile2);
		return true;
	}

	memset(&TrapMap, 0, sizeof(TrapMap));

	int iTrap;
	for (iTrap = 0; iTrap < nTotalCount; iTrap++)
	{
		if (!LoadATrap(&Trap, &TrapFile, iTrap, nLeft, nTop))
			goto ERROR_EXIT;
		if (Trap.uTrapId == 0)
			continue;
		TrapMap[Trap.cY][Trap.cX] = Trap.uTrapId;
		bHeveTrap = true;
	}

	if (bHeveTrap == false)
	{
		BinFile.Abort();
		DeleteFile(pFile2);
	}
	else
	{
		BinFile.Write(&Head, sizeof(Head));
		Trap.uTrapId = 0;
		for (int y = 0; y < 32; y++)
		{
			for (int  x = 0; x < 16; x++)
			{
				if (TrapMap[y][x] == Trap.uTrapId)
				{
					Trap.cNumCell ++;
					continue;
				}
				if (Trap.uTrapId)
				{
					BinFile.Write(&Trap, sizeof(Trap));
					Head.uNumTrap ++;
				}
				Trap.cX = x;
				Trap.cY = y;
				Trap.cNumCell = 1;
				Trap.uTrapId = TrapMap[y][x];
			}
			if (Trap.uTrapId)
			{
				BinFile.Write(&Trap, sizeof(Trap));
				Head.uNumTrap ++;
				Trap.uTrapId = 0;
			}
		}
		
		BinFile.Seek(0, CFile::begin);
		BinFile.Write(&Head, sizeof(Head));
		BinFile.Flush();
	}

	return true;

ERROR_EXIT:
	BinFile.Abort();
	DeleteFile(pFile2);
	return false;
}

bool  CMapConvertDlg::LoadATrap(/*const char* pRootPath,*/
		struct KSPTrap* pTrap, class KIniFile* pFile, int nTrap, int nLeft, int nTop)
{
	_ASSERT(pTrap && pFile);
	pTrap->uTrapId = 0;

	char	ObjSection[8], Buffer[128];
	
	itoa(nTrap, ObjSection, 10);

	int nId, nRow;
	pFile->GetString (ObjSection, "event", "", Buffer, sizeof(Buffer));
	pFile->GetInteger(ObjSection, "id", 0, &nId);

/*	CString	File;
	File.Format("%s\\trap-%s\\%05d.ini", pRootPath, Buffer, nValue);
	KIniFile	TrapEvent;
	if (!TrapEvent.Load(File))
	{
		MessageBox(File, "打开文件错误", MB_OK | MB_ICONERROR);
		return false;
	}

	TrapEvent.GetString("MAIN", "mapedit_templatesection", "", Buffer, sizeof(Buffer));*/

	itoa(nId, Buffer, 10);
	nRow = l_ScriptListFile.FindRow(Buffer);
	if (nRow < 1)
	{
//		MessageBox(Buffer, "不存在指定的Trap模版！", MB_OK | MB_ICONERROR);
		return true;
//		return false;
	}

	l_ScriptListFile.GetString(nRow, l_ScriptListFileNameColumn,
			"", Buffer, sizeof(Buffer));
	if (Buffer[0] == 0)
	{
		if (l_bReportScriptNameError)
		{
			if ((MessageBox("引用了脚本文件名为空的Trap！点YES终止转换过程，点NO忽略所有的相同错误。", "警告",
				MB_YESNO | MB_ICONINFORMATION)) == IDYES)
			{
				return false;
			}				
			l_bReportScriptNameError = false;
		}
		return true;
	}

	char* pName = Buffer;
	if (Buffer[0] == '.')
		pName ++;

	strlwr(pName);
	pTrap->uTrapId = g_FileName2Id(pName);
	
	pFile->GetString(ObjSection, "groundoffset", "", Buffer, sizeof(Buffer));
	float	x, y, z;
	SM_GetPointFromFloatSting32(Buffer, x, y, z);
	pTrap->cX =(unsigned char) (((int)(x - nLeft)) / 32);
	pTrap->cY =(unsigned char) (((int)(y - nTop )) / 32);
	if (pTrap->cX < 16 && pTrap->cY < 32)
		return true;

	sprintf(Buffer, "No:%dTrap 坐标值异常！", nTrap);
	MessageBox(Buffer, "错误", MB_OK | MB_ICONERROR);
	return false;
}

//把地表文件与紧贴地表的对象的文件合起来
bool CMapConvertDlg::ComposeGroundFile(const char* pFile1, const char* pFile2, const char* pFile3)
{
	CFile		ComposeFile, RegionFile;
	KGroundFileHead	Head;

	Head.uNumGrunode = 0;
	Head.uNumObject = 0;
	Head.uObjectDataOffset = sizeof(KGroundFileHead);

	if (!ComposeFile.Open(pFile3, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		MessageBox(pFile3, "创建文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	int	h, v;

	//====转换地表文件中的地表信息=====
	ComposeFile.Write(&Head, sizeof(KGroundFileHead));
	if (RegionFile.Open(pFile1, CFile::modeRead))
	{
		stTile				Old;
		KSPRCrunode::KSPRCrunodeParam	New;
		RegionFile.Seek(16, CFile::begin);
		for (h = 0; h < 16; h++)
		{
			for (v = 0; v < 16; v++)
			{
				RegionFile.Read(&Old, sizeof(stTile));
				Old.szResource[MAX_CHAR - 1] = 0;
				if (Old.szResource[0])
				{
					New.nFileNameLen = strlen(Old.szResource) + 1;
					if (Old.szResource[0] != '\\')
						New.nFileNameLen++;
					if (New.nFileNameLen < MAX_RESOURCE_FILE_NAME_LEN)
					{
						New.h = h;
						New.v = v;	
						New.nFrame = Old.iFrame;
						Head.uNumGrunode ++;
						ComposeFile.Write(&New, sizeof(KSPRCrunode::KSPRCrunodeParam));
						if (Old.szResource[0] == '\\')
						{
							ComposeFile.Write(Old.szResource, New.nFileNameLen);
						}
						else
						{
							ComposeFile.Write("\\", 1);
							ComposeFile.Write(Old.szResource, New.nFileNameLen - 1);
						}
							
						Head.uObjectDataOffset += sizeof(KSPRCrunode::KSPRCrunodeParam) + New.nFileNameLen;
					}
					else if (FileNameExceed(m_hWnd, Old.szResource) == false)
						return false;
				}
			}
			RegionFile.Seek(sizeof(stTile), CFile::current);
		}		
	}

	KSPRCoverGroundObj* pObjects = NULL;
	if (!LoadCoverGroundObj(pFile2, Head.uNumObject, pObjects))
		return false;

	if (Head.uNumGrunode || Head.uNumObject)
	{
		if (Head.uNumObject && pObjects)
		{
			ComposeFile.Write(pObjects, sizeof(KSPRCoverGroundObj) * Head.uNumObject);
			delete[] pObjects;
			pObjects = NULL;
		}
		ComposeFile.Seek(0, CFile::begin);
		ComposeFile.Write(&Head, sizeof(KGroundFileHead));
		ComposeFile.Flush();
	}
	else
	{
		ComposeFile.Abort();
		DeleteFile(pFile3);
	}
	return true;
}


bool CMapConvertDlg::LoadCoverGroundObj(const char* pFile, unsigned int& nCount,
				struct KSPRCoverGroundObj*& pObjects)
{
	KIniFile	ObjFile;
	nCount = 0;
	pObjects = NULL;

	unsigned dword = GetFileAttributes(pFile);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
		return true;	//指定的文件不存在

	if (!ObjFile.Load(pFile))
		return false;

	//获得对象数目
	int		nTotalCount;
	ObjFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
	if (nTotalCount <= 0)
		return true;
	pObjects = new KSPRCoverGroundObj[nTotalCount];
	if (pObjects == NULL)
		return false;

	KSPRCoverGroundObj	Obj = { 0 };
	KIniFile			Template;
	CString				Str;
	char				ObjSection[8], Buffer[128], TemplateName[128];
	static const char* szKind[3] = { "低于地表" , "地表" , "低于ver" };
	int					nValue;

	for (int iObj = 0; iObj < nTotalCount; iObj++)
	{
		itoa(iObj, ObjSection, 10);
		ObjFile.GetString (ObjSection, "template", "", TemplateName, sizeof(TemplateName));
		CString	str = TemplateName;
		AppGetFullPath(str);
		if (!Template.Load(str))
		{
			if (MessageBox("载入指定的对象模板文件失败，是否退出？",
				TemplateName, MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
				goto ERROR_EXIT;
			continue;
		}
		//--获得图形文件名、图形帧索引、图大小--
		Template.GetString("MAIN", "file", "", Buffer, sizeof(Buffer));
		if (Buffer[0] == 0)
		{
			Template.Clear();
			continue;
		}
		Str = TemplateName;
		int nPos = Str.ReverseFind('\\');
		Str = Str.Left(nPos + 1);
		Str += Buffer;
		if (Str.GetLength() >= MAX_RESOURCE_FILE_NAME_LEN)
		{
			if (FileNameExceed(m_hWnd, Str) == false)
				goto ERROR_EXIT;
		}
		strcpy(Obj.szImage, Str);
		ObjFile.GetInteger(ObjSection, "iframe", 0, &nValue);
		Obj.nFrame = nValue;
		ObjFile.GetInteger(ObjSection, "layer", 0, &nValue);
		Obj.nLayer = nValue;
		ObjFile.GetInteger(ObjSection, "texWidth", 0, &nValue);
		Obj.nWidth = nValue;
		ObjFile.GetInteger(ObjSection, "texHeight", 0, &nValue);
		Obj.nHeight = nValue;
		
		char	TempateIndex[8] = "";
		itoa(Obj.nFrame, TempateIndex, 10);
		//--取得对象绘制次序--
		Obj.bOrder = 1;		
		if (!Template.GetString(TempateIndex, "sorttype", szKind[1], Buffer, sizeof(Buffer)))
			Template.GetString("MAIN", "sorttype", szKind[1], Buffer, sizeof(Buffer));
		for (nValue = 0; nValue < 3; nValue++)
		{
			if (strcmp(szKind[nValue], Buffer) == 0)
			{
				Obj.bOrder = nValue;
				break;
			}
		}

		//--得到图形"中点"--
		int	nCentreX, nCentreY;
		Template.GetInteger(TempateIndex, "sprcenterx", 0, &nCentreX);
		Template.GetInteger(TempateIndex, "sprcentery", 0, &nCentreY);
		if (Template.GetInteger("MAIN", "centerx", 0, &nValue))
			nCentreX = nValue;
		if (Template.GetInteger("MAIN", "centery", 0, &nValue))
			nCentreY = nValue;
		if (Template.GetInteger(TempateIndex, "centerx", 0, &nValue))
			nCentreX = nValue;
		if (Template.GetInteger(TempateIndex, "centery", 0, &nValue))
			nCentreY = nValue;
		if (Template.GetInteger(TempateIndex, "sprwidth", 0, &nValue))
			Obj.nWidth = nValue;
		if (Template.GetInteger(TempateIndex, "sprheight", 0, &nValue))
			Obj.nHeight = nValue;
		Template.Clear();
		
		//--得到对象图形坐标--
		ObjFile.GetString(ObjSection, "groundoffset", "", Buffer, sizeof(Buffer));
		float x, y, z;
		SM_GetPointFromFloatSting32(Buffer, x, y, z);
		if ((x < 1024 && x > -1024) || (y < 2048 && y > -2048))
		{
			x += s_nCurrentRegionIndex.x * 512;
			y += s_nCurrentRegionIndex.y * 1024;
		}


		Obj.nPositionX = (int)(x - nCentreX);
		if (nCentreY)
			Obj.nPositionY = (int)(y - nCentreY * 2);
		else
			Obj.nPositionY = (int)(y - Obj.nHeight * 2);
		
		//====排序====
		nValue = nCount - 1;
		while(nValue >= 0)
		{
			if ((pObjects[nValue].bOrder >  Obj.bOrder) ||
				(pObjects[nValue].bOrder == Obj.bOrder &&
				 pObjects[nValue].nLayer >  Obj.nLayer))
			{
				nValue --;
			}
			else
				break;		
		};
		while (nValue >= 0 &&
			pObjects[nValue].bOrder == Obj.bOrder &&
			pObjects[nValue].nLayer == Obj.nLayer &&
			pObjects[nValue].nPositionY > Obj.nPositionY)
		{
			nValue --;
		}
		nValue ++;
		for (int i = nCount; i > nValue; i--)
			pObjects[i] = pObjects[i - 1];
		pObjects[nValue] = Obj;
		nCount++;
	}
	return true;
ERROR_EXIT:
	if (pObjects)
	{
		delete[] pObjects;
		pObjects = NULL;
	}
	nCount = 0;
	return false;
}

bool CMapConvertDlg::ConvertNpcFileBin(const char* pRootPath, const char* pFile1, const char* pFile2, bool bServer)
{
	CFile		BinFile;
	KIniFile	NpcFile;

	KNpcFileHead	Head = { 0 };
	KSPNpc			*pObjects = NULL, *pCurObj = NULL;

	unsigned dword = GetFileAttributes(pFile1);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
	{
		DeleteFile(pFile2);
		CString error;
		error.Format("不存在文件:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		return true;
	}

	if (!BinFile.Open(pFile2, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		MessageBox(pFile2, "创建文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	if (NpcFile.Load(pFile1) == FALSE)
	{
		MessageBox(pFile1, "打开文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		goto ERROR_EXIT;
	}

	//获得对象数目
	int		nTotalCount;
	NpcFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
	if (nTotalCount <= 0)
	{
		CString error;
		error.Format("npc数目小于0:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		BinFile.Abort();
		DeleteFile(pFile2);
		return true;
	}

	pObjects = new KSPNpc[nTotalCount];
	if (pObjects == NULL)
		goto ERROR_EXIT;

	int iNpc;
	pCurObj = pObjects;
	for (iNpc = 0; iNpc < nTotalCount; iNpc++)
	{
		if (!LoadANpc(pRootPath, pCurObj, &NpcFile, iNpc, bServer))
			goto ERROR_EXIT;
		if (pCurObj->nTemplateID < 0)
			continue;
		pCurObj = (KSPNpc*)(((char*)pCurObj) + sizeof(KSPNpc) - sizeof(pCurObj->szScript) + pCurObj->nScriptNameLen);
		Head.uNumNpc ++;
	}

	if (Head.uNumNpc)
	{
		BinFile.Write(&Head, sizeof(Head));
		BinFile.Write(pObjects, ((char*)pCurObj - (char*)pObjects));
		BinFile.Flush();
	}
	else
	{
		BinFile.Abort();
		DeleteFile(pFile2);
	}
	delete[] pObjects;
	return true;

ERROR_EXIT:
	if (pObjects)
		delete[] pObjects;
	BinFile.Abort();
	return false;
}

bool CMapConvertDlg::LoadANpc(const char* pRootPath, struct KSPNpc* pNpc, class KIniFile* pFile, int nNpc, bool bServer)
{
	_ASSERT(pRootPath && pFile && pNpc);
	memset(pNpc, 0, sizeof(KSPNpc));
	pNpc->nTemplateID = -1;

	char	ObjSection[8], Buffer[256];
	
	itoa(nNpc, ObjSection, 10);

	int nValue;
	pFile->GetString (ObjSection, "event", "", Buffer, sizeof(Buffer));
	pFile->GetInteger(ObjSection, "id", 0, &nValue);

	CString	File;
	File.Format("%s\\npc-%s\\%05d.ini", pRootPath, Buffer, nValue);
	KIniFile	NpcEvent;
	if (!NpcEvent.Load(File))
	{
		MessageBox(File, "打开文件错误", MB_OK);
		return false;
	}

	pFile->GetInteger(ObjSection, "iframe", 0, &nValue);
	pNpc->nCurFrame = nValue;

	NpcEvent.GetString("MAIN", "mapedit_templatesection", "", Buffer, sizeof(Buffer));
	pNpc->nTemplateID = l_NpcSFile.FindRow(Buffer) - 2;
	if (pNpc->nTemplateID < 0)
	{
		MessageBox("不存在指定的npc模版！", Buffer, MB_OK | MB_ICONERROR);
		return false;
	}

	l_NpcSFile.GetInteger(pNpc->nTemplateID + 2, l_NpcSFileKindColumn, 0, &nValue);
	if ((!bServer) != (nValue == 4 || nValue == 5))
	{
		pNpc->nTemplateID = -1;
		return true;
	}

	NpcEvent.GetInteger("MAIN", "Level", 1, &nValue);
	pNpc->nLevel = nValue;
	
	if (!NpcEvent.GetInteger("MAIN", "Kind", 3, &nValue))
	{
		l_NpcSFile.GetInteger(pNpc->nTemplateID + 2, l_NpcSFileKindColumn, 3, &nValue);
	}
	pNpc->shKind = nValue;

	if (!NpcEvent.GetString("1", "ActionScript", "", Buffer, sizeof(Buffer)))
	{
		l_NpcSFile.GetString(pNpc->nTemplateID + 2, l_NpcSFileScriptColumn,
			"", Buffer, sizeof(Buffer));
	}
	if (Buffer[0] == '.')
		strcpy(pNpc->szScript, &Buffer[1]);
	else
		strcpy(pNpc->szScript, Buffer);
	pNpc->nScriptNameLen = strlen(pNpc->szScript) + 1;

	if (!NpcEvent.GetString("1", "Name", "", pNpc->szName, sizeof(pNpc->szName)))
	{
		l_NpcSFile.GetString(pNpc->nTemplateID + 2, l_NpcSFileNameColumn,
			"", pNpc->szName, sizeof(pNpc->szName));		
	}

	if (!NpcEvent.GetInteger("1", "HeadImage", 0, &nValue))
	{
		l_NpcSFile.GetInteger(pNpc->nTemplateID + 2, l_NpcSFileHeadImageColumn,
			0, &nValue);
	}
	pNpc->nHeadImageNo = nValue;

	if (!NpcEvent.GetInteger("1", "Camp", 0, &nValue))
	{
		l_NpcSFile.GetInteger(pNpc->nTemplateID + 2, l_NpcSFileCampColumn,
			0, &nValue);
	}
	pNpc->cCamp = nValue;

	if (!NpcEvent.GetInteger("1", "Series", 0, &nValue))
	{
		l_NpcSFile.GetInteger(pNpc->nTemplateID + 2, l_NpcSFileSeriesColumn,
			0, &nValue);
	}
	pNpc->cSeries = nValue;

	pFile->GetString(ObjSection, "groundoffset", "", Buffer, sizeof(Buffer));
	float	x, y, z;
	SM_GetPointFromFloatSting32(Buffer, x, y, z);
	pNpc->Pos.x = (int)x;
	pNpc->Pos.y = (int)y;
	pNpc->Pos.z = (int)z;

	return true;
}

//把一个obj对象加入bio文件
static int InsertABioToBioFile(KBuildinObj* pObj, const char* pBioFile, bool& bInserted)
{
	bInserted = false;
//	if (Obj.szImage[0] == 0)
//		continue;
	
	CFile				BinFile;
	KBuildinObjFileHead Head = { 0 };
	KBuildinObj*		pObjects = NULL;
	int					nDest = -1;
	unsigned int		i;

	if (!BinFile.Open(pBioFile, CFile::modeCreate | CFile::modeNoTruncate |
		CFile::modeReadWrite | CFile::shareExclusive))
	{
		return -1;
	}
	if (int nSize = BinFile.GetLength())
	{
		nSize -= sizeof(KBuildinObjFileHead);
		_ASSERT(nSize >= 0);
		pObjects = (KBuildinObj*)malloc(nSize + sizeof(KBuildinObj));
		if (pObjects == NULL)
		{
			_ASSERT(0);
			return -1;
		}
		BinFile.Read(&Head, sizeof(KBuildinObjFileHead));
		BinFile.Read(pObjects, nSize);

		//====排序====
		for (i = 0; i < Head.nNumBios; i++)
		{
			if (pObj->nFrame != pObjects[i].nFrame ||
				pObj->fAngleXY != pObjects[i].fAngleXY ||
				strcmp(pObj->szImage, pObjects[i].szImage) != 0)
			{
				continue;
			}
			int dx = pObj->oPos1.x - pObjects[i].oPos1.x;
			int dy = pObj->oPos1.y - pObjects[i].oPos1.y;
			int dz = pObj->oPos1.z - pObjects[i].oPos1.z;

			if (dx * dx < 9 && dy * dy < 9 && dz * dz < 9)
			{	
				nDest = i;
				break;
			}
		}
	}

	if (nDest >= 0)
	{
		BinFile.Abort();
		free(pObjects);
		return nDest;
	}

//	int nFirst, nLast;
	if (pObj->nOrder == NOT_ABOVE_HEAD_OBJ)
	{		
		unsigned int uSort = (pObj->Props & SPBIO_P_SORTMANNER_MASK);
		if (uSort == SPBIO_P_SORTMANNER_LINE)
		{
			nDest = Head.nNumBiosPoint + Head.nNumBiosLine;
			Head.nNumBiosLine++;
		}
		else if (uSort == SPBIO_P_SORTMANNER_TREE)
		{
			nDest = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree;
			Head.nNumBiosTree++;
		}
		else
		{
			_ASSERT(0);
			if (pObjects)
				free(pObjects);
			return - 1;
		}
	}
	else
	{
//		if (pObj->nOrder > Head.nMaxAboveHeadObjOrder)
//			Head.nMaxAboveHeadObjOrder = pObj->nOrder;
//		nFirst = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree;
//		nDest = nLast = nFirst + Head.nNumBiosAbove;
		nDest = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree + Head.nNumBiosAbove;
//		while (nDest > nFirst)
//		{
//			if(pObjects[nDest - 1].nOrder > pObj->nOrder)
//				nDest--;
//			else
//				break;
//		};
		Head.nNumBiosAbove ++;
	}

	for (i = Head.nNumBios; i > nDest; i--)
		pObjects[i] = pObjects[i - 1];
	pObjects[nDest] = *pObj;
	Head.nNumBios ++;

	BinFile.SeekToBegin();
	BinFile.Write(&Head, sizeof(Head));
	BinFile.Write(pObjects, sizeof(KBuildinObj) * Head.nNumBios);
	BinFile.Flush();
	BinFile.Close();
	delete[] pObjects;
	bInserted = true;
	return nDest;
}

bool CMapConvertDlg::ConvertObjFileBin(const char* pRootPath, const char* pFile1,
						const char* pFile2, bool bServer, const char* pBioFile)
{
	CFile		BinFile;
	KIniFile	ObjFile;

	KObjFileHead	Head = { 0 };
	KSPObj			*pObjects = NULL, *pCurObj = NULL;

	unsigned dword = GetFileAttributes(pFile1);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
	{
		DeleteFile(pFile2);
		CString error;
		error.Format("不存在文件:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		return true;
	}

	if (!BinFile.Open(pFile2, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		MessageBox(pFile2, "创建文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	if (ObjFile.Load(pFile1) == FALSE)
	{
		MessageBox(pFile1, "打开文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		goto ERROR_EXIT;
	}

	//获得对象数目
	int		nTotalCount;
	ObjFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
	if (nTotalCount <= 0)
	{
		CString error;
		error.Format("npc数目小于0:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		BinFile.Abort();
		DeleteFile(pFile2);
		return true;
	}

	pObjects = new KSPObj [nTotalCount];
	if (pObjects == NULL)
		goto ERROR_EXIT;

	pCurObj = pObjects;
	int iObj;
	for (iObj = 0; iObj < nTotalCount; iObj++)
	{
		KBuildinObj	BuildinObj;
		if (!LoadAObj(pRootPath, pCurObj, &ObjFile, iObj, &BuildinObj, bServer))
			goto ERROR_EXIT;
		if (pCurObj->nTemplateID < 1)
			continue;

		if (pCurObj->bSkipPaint)
		{
			bool bInserted;
			pCurObj->nBioIndex = InsertABioToBioFile(&BuildinObj, pBioFile, bInserted);
			if (pCurObj->nBioIndex < 0)
			{
				MessageBox("把obj对象加入文件的时候失败了！", pBioFile, MB_OK | MB_ICONERROR);
				goto ERROR_EXIT;
			}
			else if (bInserted)
			{
				KSPObj* pOldObj = pObjects;
				for (unsigned int nOldObj = 0; nOldObj < Head.uNumObj; nOldObj++)
				{
					if (pOldObj->bSkipPaint && pOldObj->nBioIndex >= pCurObj->nBioIndex)
						pOldObj->nBioIndex ++;
					pOldObj = (KSPObj*)(((char*)pOldObj) + sizeof(KSPObj) - sizeof(pOldObj->szScript) + pOldObj->nScriptNameLen);
				}
			}
		}

		pCurObj = (KSPObj*)(((char*)pCurObj) + sizeof(KSPObj) - sizeof(pCurObj->szScript) + pCurObj->nScriptNameLen);
		Head.uNumObj ++;
	}

	if (Head.uNumObj)
	{
		BinFile.Write(&Head, sizeof(Head));
		BinFile.Write(pObjects, ((char*)pCurObj - (char*)pObjects));
		BinFile.Flush();
	}
	else
	{
		BinFile.Abort();
		DeleteFile(pFile2);
	}
	delete[] pObjects;
	return true;

ERROR_EXIT:
	if (pObjects)
		delete [] (pObjects);
	BinFile.Abort();
	return false;
}

const char*	l_szObjKind[] = 
{
	"MapObj",
	"Body",
	"Box",
	"Item",
	"Money",
	"LoopSound",
	"RandSound",
	"Light",
	"Door",
	"Trap",
	"Prop"
};

bool CMapConvertDlg::LoadAObj(const char* pRootPath, struct KSPObj* pObj, class KIniFile* pFile, int nObj, KBuildinObj* pBio, bool bServer)
{
	_ASSERT(pRootPath && pFile && pObj);
	memset(pObj, 0, sizeof(KSPObj));
	pObj->nTemplateID = -1;

	char	ObjSection[8], Buffer[256];
	
	itoa(nObj, ObjSection, 10);

	int nValue;
	pFile->GetString (ObjSection, "event", "", Buffer, sizeof(Buffer));
	pFile->GetInteger(ObjSection, "id", 0, &nValue);

	CString	File;
	File.Format("%s\\box-%s\\%05d.ini", pRootPath, Buffer, nValue);
	KIniFile	ObjEvent;
	if (!ObjEvent.Load(File))
	{
		MessageBox(File, "打开文件错误", MB_OK);
		return false;
	}

	ObjEvent.GetString("MAIN", "mapedit_templatesection", "", Buffer, sizeof(Buffer));
	pObj->nTemplateID = l_ObjDataFile.FindRow(Buffer) - 1;
	if (pObj->nTemplateID < 1)
	{
		MessageBox("不存在指定的Obj模版！", Buffer, MB_OK | MB_ICONERROR);
		return false;
	}

	l_ObjDataFile.GetString(pObj->nTemplateID + 1, l_ObjDataFileKindColumn, "", Buffer, sizeof(Buffer));
	nValue = -1;
	for (int i = 0; i < (sizeof(l_szObjKind) / sizeof(const char*)); i++)
	{
		if (strcmp(Buffer, l_szObjKind[i]) == 0)
		{
			nValue = i;
			break;
		}
	}

	if ((nValue < 0) ||
		((!bServer) !=
			(nValue == 0 || nValue == 1 ||
				nValue == 5 || nValue == 6 || nValue == 7) ) )
	{
		pObj->nTemplateID = -1;
		return true;
	}

	ObjEvent.GetInteger("1", "State", 0, &nValue);
	pObj->nState = nValue;

	if (!ObjEvent.GetInteger("1", "ImageCurDir", 0, &nValue))
	{
		l_ObjDataFile.GetInteger(pObj->nTemplateID + 1, l_ObjDataFileDirColumn,
			0, &nValue);
	}
	pObj->nDir = nValue;

	Buffer[0] = 0;
	if (!ObjEvent.GetString("1", "ScriptName", "", Buffer, sizeof(Buffer)))
	{
		l_ObjDataFile.GetString(pObj->nTemplateID + 1, l_ObjDataFileScriptColumn,
			"", Buffer, sizeof(Buffer));
	}
	if (Buffer[0] == '.')
		strcpy(pObj->szScript, &Buffer[1]);
	else
		strcpy(pObj->szScript, Buffer);
	pObj->nScriptNameLen = strlen(pObj->szScript) + 1;

	if (!LoadAAboveGroundObject(pBio, pFile, nObj))
		return false;

	if ((pBio->Props & SPBIO_P_SORTMANNER_MASK) == SPBIO_P_SORTMANNER_POINT)
	{
		pObj->Pos.x = pBio->oPos1.x;
		pObj->Pos.y = pBio->oPos1.y;
		pObj->Pos.z = pBio->oPos1.z;
	}
	else
	{
		pObj->bSkipPaint = true;
		pObj->Pos.x = (pBio->oPos1.x + pBio->oPos2.x) / 2;
		pObj->Pos.z = (pBio->oPos1.z + pBio->oPos2.z) / 2;
		if (pBio->oPos1.y <= pBio->oPos2.y)
			pObj->Pos.y = pBio->oPos1.y;
		else
			pObj->Pos.y = pBio->oPos2.y;

		l_ObjDataFile.GetString(pObj->nTemplateID + 1, l_ObjDataImgNameColomn, "", Buffer, sizeof(Buffer));
		if (strlen(Buffer) >= MAX_RESOURCE_FILE_NAME_LEN)
		{
			if (FileNameExceed(m_hWnd, Buffer) == false)
				return false;
			pObj->nTemplateID = -1;
			return true;
		}
		if (Buffer[0] != '.')
			strcpy(pBio->szImage, Buffer);
		else
			strcpy(pBio->szImage, &Buffer[1]);
	}

	return true;
}

bool CMapConvertDlg::ConvertBuildinObjFile(const char* pFile1,const char* pOldLightFile, const char* pFile2)
{
	CFile		BinFile;
	KIniFile	ObjFile;

	KBuildinObjFileHead Head = { 0 };
	KBuildinObj*		pObjects = NULL;
	KBuildInLightInfo*	pLights = NULL;
	CString error;
	int			iObj;

	if (!BinFile.Open(pFile2, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		MessageBox(pFile2, "创建文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		return false;
	}

	unsigned dword = GetFileAttributes(pFile1);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
	{
		error.Format("不存在文件:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		goto LOAD_LIGHTS;
	}


	if (ObjFile.Load(pFile1) == FALSE)
	{
		MessageBox(pFile1, "打开文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		goto ERROR_EXIT;
	}

	//----获得对象----
	int		nTotalCount;
	ObjFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
	if (nTotalCount <= 0)
	{
		error.Format("对象数目小于0:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
		goto LOAD_LIGHTS;
	}
	pObjects = new KBuildinObj[nTotalCount];
	if (pObjects == NULL)
		goto ERROR_EXIT;

	KBuildinObj	Obj;
	for (iObj = 0; iObj < nTotalCount; iObj++)
	{
		if (!LoadAAboveGroundObject(&Obj, &ObjFile, iObj))
			goto ERROR_EXIT;
		if (Obj.szImage[0] == 0)
			continue;
		
		//====排序====
		int	i, nDest;
	
		if (Obj.nOrder == NOT_ABOVE_HEAD_OBJ)
		{		
			unsigned int uSort = (Obj.Props & SPBIO_P_SORTMANNER_MASK);
			if (uSort == SPBIO_P_SORTMANNER_POINT)
			{
				nDest = Head.nNumBiosPoint;
				Head.nNumBiosPoint++;
			}
			else if (uSort == SPBIO_P_SORTMANNER_LINE)
			{
				nDest = Head.nNumBiosPoint + Head.nNumBiosLine;
				Head.nNumBiosLine++;
			}
			else if (uSort == SPBIO_P_SORTMANNER_TREE)
			{
				nDest = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree;
				Head.nNumBiosTree++;
			}
			else
			{
				Obj.Props = (Obj.Props & ~SPBIO_P_SORTMANNER_MASK) | SPBIO_P_SORTMANNER_POINT;
				nDest = Head.nNumBiosPoint;
				Head.nNumBiosPoint++;
			}
		}
		else
		{
//			if (Obj.nOrder > Head.nMaxAboveHeadObjOrder)
//				Head.nMaxAboveHeadObjOrder = Obj.nOrder;
//			nFirst = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree;
//			nDest = nLast = nFirst + Head.nNumBiosAbove;
			nDest = Head.nNumBiosPoint + Head.nNumBiosLine + Head.nNumBiosTree + Head.nNumBiosAbove;
//			while (nDest > nFirst)
//			{
//				if(pObjects[nDest - 1].nOrder > Obj.nOrder)
//					nDest--;
//				else
//					break;
//			};
			Head.nNumBiosAbove ++;
		}
		
		for (i = Head.nNumBios; i > nDest; i--)
			pObjects[i] = pObjects[i - 1];
		pObjects[nDest] = Obj;
		Head.nNumBios ++;

	}

	//----读光源数据----
LOAD_LIGHTS:
	ObjFile.Clear();

	dword = GetFileAttributes(pOldLightFile);
	if ((dword & FILE_ATTRIBUTE_DIRECTORY))// || dword == INVALID_FILE_ATTRIBUTES)
	{
		error.Format("不存在文件:%s", pFile1);
		SetDlgItemText(IDC_NOW_FILE, error);
	}
	else if (ObjFile.Load(pOldLightFile))
	{
		nTotalCount;
		ObjFile.GetInteger("MAIN", "elementnum", 0, &nTotalCount);
		if (nTotalCount <= 0)
		{
			error.Format("对象数目小于0:%s", pFile1);
			SetDlgItemText(IDC_NOW_FILE, error);
			goto SAVE_SECTION;
		}
		pLights = new KBuildInLightInfo[nTotalCount];
		if (pLights == NULL)
			goto ERROR_EXIT;
		
		for (iObj = 0; iObj < nTotalCount; iObj++)
		{
			if (!LoadALight(&pLights[iObj], &ObjFile, iObj))
				goto ERROR_EXIT;
			Head.nNumBulidinLight ++;
		}	
	}
	else
	{
		MessageBox(pFile1, "打开文件失败，转换操作未完成！", MB_OK | MB_ICONERROR);
		goto ERROR_EXIT;
	}


SAVE_SECTION:

	if (Head.nNumBios || Head.nNumBulidinLight)
	{
		BinFile.Write(&Head, sizeof(Head));
		if (Head.nNumBios)
			BinFile.Write(pObjects, sizeof(KBuildinObj) * Head.nNumBios);
		if (Head.nNumBulidinLight)
			BinFile.Write(pLights, sizeof(KBuildInLightInfo) * Head.nNumBulidinLight);
		BinFile.Flush();
	}
	else
	{
		BinFile.Abort();
		DeleteFile(pFile2);
	}

	if (pLights)
		delete[] pLights;
	if (pObjects)
		delete[] pObjects;

	return true;

ERROR_EXIT:
	if (pLights)
		delete[] pLights;
	if (pObjects)
		delete[] pObjects;
	BinFile.Abort();
	return false;
}

bool CMapConvertDlg::LoadALight(struct KBuildInLightInfo* pLight, class KIniFile* pFile, int nObject)
{
	_ASSERT(pFile && pLight);
	memset(pLight, 0, sizeof(KBuildInLightInfo));

	int		nValue;
	float	fValue;
	char	ObjSection[8], Buffer[128];
	
	itoa(nObject, ObjSection, 10);

	RECT	rc = { 0 };
	pFile->GetRect(ObjSection, "color", &rc);
	pLight->dwColor = ((rc.left & 0xff) << 16) |
		((rc.top * 0xff) << 8) | (rc.right & 0xff) | 0xff000000;

	//取坐标位置
	pFile->GetString(ObjSection, "groundoffset", "", Buffer, sizeof(Buffer));
	float	x, y, z;
	SM_GetPointFromFloatSting32(Buffer, x, y, z);
	pLight->oPos.x = (int)x;
	pLight->oPos.y = (int)y;
	pLight->oPos.z = (int)z;

	//取光的范围
	float fMinRange, fMaxRange;
	pFile->GetFloat(ObjSection, "range", 0, &fMinRange);
	pLight->nMinRange = (int)(fMinRange * 32);

	pFile->GetFloat(ObjSection, "varrangemax", 0, &fMaxRange);
	pLight->nMaxRange = (int)(fMaxRange * 32);

	if (pLight->nMaxRange < pLight->nMinRange)
	{
		nValue = pLight->nMaxRange;
		pLight->nMaxRange = pLight->nMinRange;
		pLight->nMinRange = nValue;
	}

	//计算光的速度
	float	fFrame, fSpeed;
	pFile->GetFloat(ObjSection, "varrangeframe", 0, &fFrame);
	pFile->GetFloat(ObjSection, "varrangespeed", 0, &fSpeed);
	if (fFrame == 0.0)
		fFrame = 1;
	if (fSpeed / fFrame)
		pLight->nCycle = (int)(((fMaxRange - fMinRange) / (fSpeed / fFrame)) * 100);
	else
		pLight->nCycle = (int)((pLight->nMaxRange - pLight->nMinRange) * 100);

	return true;
}
bool CMapConvertDlg::LoadAAboveGroundObject(struct KBuildinObj* pObject, 
			class KIniFile* pFile, int nObject)
{
	_ASSERT(pFile && pObject);
	memset(pObject, 0, sizeof(KBuildinObj));

	int		nHeight, nWidth, nValue, nCentreX, nCentreY;
	float	fValue;
	char	ObjSection[8], Buffer[128];
	
	itoa(nObject, ObjSection, 10);
	pFile->GetInteger(ObjSection, "elementtype", 0, &nValue);
	if (nValue == 1004)
		pObject->Props = SPBIO_P_SORTMANNER_LINE;
	else
		pObject->Props = SPBIO_P_SORTMANNER_POINT;
	pFile->GetString (ObjSection, "template", "", Buffer, sizeof(Buffer));
	pFile->GetInteger(ObjSection, "templateindex", 0, &nValue);
	pFile->GetInteger(ObjSection, "texWidth", 0, &nWidth);
	pFile->GetInteger(ObjSection, "texHeight", 0, &nHeight);
	pObject->nImgWidth = nWidth;
	pObject->nImgHeight = nHeight;

	KSpbioTemplate	Template;
	if (Template.Load(Buffer, nValue) == false)
	{
		return (MessageBox("载入对象模板失败，是否退出？", Buffer,
			MB_OKCANCEL | MB_ICONERROR) != IDOK);
	}
	Template.Instance(pObject, nCentreX, nCentreY);
	
	if (pObject->szImage[0] == 0)
		return true;

	nWidth = pObject->nImgWidth;
	nHeight = pObject->nImgHeight;

	if (nWidth <= 0 || nHeight <= 0)
	{
		char	Info[128];
		sprintf(Info, "Obj:%d, Template:%s", nObject, Buffer);
		MessageBox(Info, "地图版本不对，操作强制中断。", MB_OK);
		return false;
	}
	
	pFile->GetString(ObjSection, "groundoffset", "", Buffer, sizeof(Buffer));
	float	x, y, z;
	SM_GetPointFromFloatSting32(Buffer, x, y, z);
	if ((x < 1024 && x > -1024) || (y < 2048 && y > -2048))
	{
		x += s_nCurrentRegionIndex.x * 512;
		y += s_nCurrentRegionIndex.y * 1024;
	}
	pObject->oPos1.x = (int)x;
	pObject->oPos1.y = (int)y;
	pObject->oPos1.z = (int)z;

	float fAdjustValue;
	if (z >= ABOVE_HEAD_OBJ_ATLEAST_HIGH)
	{
//		pFile->GetInteger(ObjSection, "layer", 0, &nValue);
//		pObject->nOrder = nValue;
		fAdjustValue = -0.5;
		pObject->oPos1.z = (int)(z + fAdjustValue);
	}
	else
	{
		pObject->nOrder = NOT_ABOVE_HEAD_OBJ;
		fAdjustValue = 0;
	}

	//----计算图形位置----
	if (pObject->nAniSpeed && pObject->fAngleXY == 0)
	{
		pObject->Props =  (pObject->Props & ~SPBIO_P_SORTMANNER_MASK) | SPBIO_P_SORTMANNER_POINT;
		if ((pObject->Props & SPBIO_P_BLOCK_LIGHT_MASK) == SPBIO_P_BLOCK_LIGHT_BOTTOM)
		{
			pObject->Props &= ~(SPBIO_P_BLOCK_LIGHT_MASK | SPBIO_P_BLOCK_L_RADIUS_MASK);
			pObject->Props |= SPBIO_P_BLOCK_LIGHT_CIRCLE | SPBIO_P_BLOCK_L_RADIUS_2;
		}
	}

	if ((pObject->Props & SPBIO_P_SORTMANNER_MASK) == SPBIO_P_SORTMANNER_POINT)
	{
		pObject->ImgPos1.x = pObject->oPos1.x - nCentreX;
		pObject->ImgPos1.y = pObject->oPos1.y;
		pObject->ImgPos3.x = pObject->ImgPos1.x + nWidth;
		pObject->ImgPos3.y = pObject->ImgPos1.y;
		if (nCentreY)
		{
			pObject->ImgPos1.z = (int)(z + nCentreY * ONE_OF_COS_30 + fAdjustValue);
			pObject->ImgPos3.z = (int)(z - (nHeight  - nCentreY) * ONE_OF_COS_30 + fAdjustValue);
		}
		else
		{
			pObject->ImgPos1.z = (int)(z + nHeight * ONE_OF_COS_30 + fAdjustValue);
			pObject->ImgPos3.z = (int)(z + fAdjustValue);
		}
		if (pFile->GetFloat(ObjSection, "sortoffy", 0, &fValue))
			pObject->oPos1.y = (int)(y + fValue * 32);
		else
			pObject->oPos1.y = (int)(y);
	}
	else
	{
		if (pObject->fAngleXY == 0)
		{			
			pObject->oPos1.x = (int)(x - nCentreX);
			pObject->oPos2.x = pObject->oPos1.x + nWidth;
			pObject->oPos2.y = pObject->oPos1.y;
			pObject->oPos2.z = pObject->oPos1.z;

			pObject->ImgPos1.x = pObject->oPos1.x;
			pObject->ImgPos1.y = pObject->oPos1.y;
			pObject->ImgPos3.x = pObject->oPos2.x;
			pObject->ImgPos3.y = pObject->oPos2.y;

			if (nCentreY)
			{
				pObject->ImgPos1.z = (int)(z + nCentreY * ONE_OF_COS_30 + fAdjustValue);
				pObject->ImgPos3.z = (int)(z - (nHeight  - nCentreY) * ONE_OF_COS_30 + fAdjustValue);
			}
			else
			{
				pObject->ImgPos1.z = (int)(z + nHeight * ONE_OF_COS_30 + fAdjustValue);
				pObject->ImgPos3.z = (int)(z + fAdjustValue);
			}
			pObject->ImgPos4.x = pObject->ImgPos1.x;
			pObject->ImgPos4.y = pObject->ImgPos1.y;
			pObject->ImgPos4.z = pObject->ImgPos3.z;
			pObject->ImgPos2.x = pObject->ImgPos3.x;
			pObject->ImgPos2.y = pObject->ImgPos3.y;
			pObject->ImgPos2.z = pObject->ImgPos1.z;

			pObject->fNodicalY = y;
		}
		else
		{
			float fValue = (float)(nWidth * tan(pObject->fAngleXY * PI_PER_ANGLE_DEGREE) / 2);
			if (fValue > 0)
				fValue = -fValue;
			float fD1 = (float)((nHeight + fValue)  * ONE_OF_COS_30);
			float fD2 = (float)(nHeight * ONE_OF_COS_30);
			
			float x2, y2;

			SM_GetLineEndPointAndNodicalY(x, y, x2, y2,
				nWidth, pObject->fAngleXY, pObject->fNodicalY);
			pObject->oPos2.x = (int)x2;
			pObject->oPos2.y = (int)y2;
			pObject->oPos2.z = (int)(z + fAdjustValue);
			pObject->ImgPos1.x = (int)x;
			pObject->ImgPos1.y = (int)y;
			pObject->ImgPos2.x = pObject->oPos2.x;
			pObject->ImgPos2.y = pObject->oPos2.y;
			pObject->ImgPos3.x = pObject->ImgPos2.x;
			pObject->ImgPos3.y = pObject->ImgPos2.y;
			pObject->ImgPos4.x = pObject->ImgPos1.x;
			pObject->ImgPos4.y = pObject->ImgPos1.y;
			if (pObject->fAngleXY < 0)
			{
				pObject->ImgPos1.z = (int)(z + fD2 + fAdjustValue);
				pObject->ImgPos2.z = (int)(z + fD1 + fAdjustValue);
				pObject->ImgPos3.z = (int)(z + fD1 - fD2 + fAdjustValue);
				pObject->ImgPos4.z = (int)(z + fAdjustValue);
			}
			else
			{
				pObject->ImgPos1.z = (int)(z + fD1 + fAdjustValue);
				pObject->ImgPos2.z = (int)(z + fD2 + fAdjustValue);
				pObject->ImgPos3.z = (int)(z + fAdjustValue);
				pObject->ImgPos4.z = (int)(z + fD1 - fD2 + fAdjustValue);
			}
		}
		if (pFile->GetFloat(ObjSection, "sortoffy", 0, &fValue))
		{
			pObject->oPos1.y += (int)(fValue * 32);
			pObject->oPos2.y += (int)(fValue * 32);
		}
	}
	
	//处理有些树对象的设定错误
	if ((pObject->Props & SPBIO_P_SORTMANNER_MASK) == SPBIO_P_SORTMANNER_TREE ||
		(pObject->Props & SPBIO_P_BLOCK_LIGHT_MASK) == SPBIO_P_BLOCK_LIGHT_BOTTOM)
	{
		if (strstr(pObject->szImage, "\\游戏资源\\美术图素\\城市\\两湖\\室外\\树木"))
		{
			CString str;
			if ((pObject->Props & SPBIO_P_SORTMANNER_MASK) == SPBIO_P_SORTMANNER_TREE)
			{
				str = pObject->szImage;
				AppGetFullPath(str);
				KSprite	spr;
				SPRFRAME* pFrame = NULL;
				if (spr.Load((char*)(const char*)str) &&
					(pFrame = (SPRFRAME*)spr.GetFrame(pObject->nFrame)))
				{
					pObject->oPos1.x += spr.GetCenterX() - pFrame->OffsetX;
					spr.Free();
				}
				else
				{
					pObject->oPos1.x = (pObject->oPos1.x + pObject->oPos2.x) / 2;
				}
				pObject->Props &= ~SPBIO_P_SORTMANNER_MASK;
				pObject->Props |= SPBIO_P_SORTMANNER_POINT;
			}
			if ((pObject->Props & SPBIO_P_BLOCK_LIGHT_MASK) == SPBIO_P_BLOCK_LIGHT_BOTTOM)
			{
				pObject->Props &= ~SPBIO_P_BLOCK_LIGHT_MASK;
				pObject->Props |= SPBIO_P_BLOCK_LIGHT_CIRCLE;
			}

			//记录下这个错误
			static char op[4] = "wb";
			int nPos = m_strFile.ReverseFind('\\');
			str = m_strFile.Left(nPos + 1);
			str += "树木错误.log";
			FILE *pLogFile = fopen(str, op);
			str.Format("Img:%s,Pos:%d,%d,%d\r\n", pObject->szImage, pObject->oPos1.x / 32,
				pObject->oPos1.y / 32, pObject->oPos1.z / 32);
			fprintf(pLogFile, str);
			fclose(pLogFile);
			op[0] = 'a';
			op[1] = '+';
		}
	}
	return true;
}

void SM_GetPointFromFloatSting32(char* pString, float& x, float& y, float& z)
{
	x = 0;
	y = 0;
	z = 0;
	if (pString == NULL)
		return;

	int  i = 0;
	while (pString[i] != ',' && pString[i] != 0)
		i++;
	if (pString[i] == 0)
	{
		x = (float)(atof(pString) * 32);
		return;
	}
	pString[i] = 0;
	x = (float)(atof(pString));
	x *= 32;

	pString = pString + i + 1;
	i = 0;
	while (pString[i] != ',' && pString[i] != 0)
		i++;
	if (pString[i] == 0)
	{
		y = (float)(atof(pString) * 32);
		return;
	}
	pString[i] = 0;
	y = (float)(atof(pString) * 32);
	
	z = (float)(atof(pString + i + 1) * 32);
}

void SM_GetLineEndPointAndNodicalY(float x1, float y1, float& x2, float& y2,
			int nWidth, float& fAngleXY, float& nNodicalY)
{
	fAngleXY = fAngleXY * PI_PER_ANGLE_DEGREE; //	b渲染时底边与X轴角度
	float fAngleRenderZ = PI / 6.0;				 //	a渲染时平面与Z轴角度

	float sina = (float)sin(fAngleRenderZ);
	float tgb = (float)tan(fAngleXY);
	float tgr = (float)fabs(sina * tgb);					// for example: a = 30, b = 45, then tgr = 0.5;

	float fWidth;
//	float fHeight;

	fWidth = (float)(nWidth / cos(fAngleXY));				// c_ptScale.x;
//	fHeight = (float)(nHeight / cos(fAngleRenderZ));		// c_ptScale.x;
	float fZ = (float)(nWidth * tgr / cos(fAngleRenderZ));	// c_ptScale.x;

	x2 = (float)(x1 + fWidth * cos(fAngleXY)); //v[3].sx = 0 + fWidth * cos(fAngleXY);
	y2 = (float)(y1 - fWidth * sin(fAngleXY)); //v[3].sy = 0 - fWidth * sin(fAngleXY);

	fAngleXY = (y1 - y2) / (x1 - x2);
	nNodicalY = y1 - fAngleXY * x1;
}


void CMapConvertDlg::DeleteRegionFolder(const char* pszPath)
{
	char	szFile[MAX_PATH];
	if (pszPath)
	{
		sprintf(szFile, "%s%s", pszPath, REGION_GROUND_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_GROUND_COVER_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_BUILDIN_OBJ_OLD_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_LIGHT_OBJ_OLD_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_NPC_OLD_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_OBJ_OLD_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%s%s", pszPath, REGION_TRAP_OLD_FILE);
		DeleteFile(szFile);
		sprintf(szFile, "%sobstacle.txt", pszPath);
		DeleteFile(szFile);
		sprintf(szFile, "%sautoground_64.dat", pszPath);
		DeleteFile(szFile);
		sprintf(szFile, "%sautoground_128.dat", pszPath);
		DeleteFile(szFile);		
		sprintf(szFile, "%sRegion.ini", pszPath);
		DeleteFile(szFile);
		sprintf(szFile, "%sPlaneVer_Obstacle.ini", pszPath);
		DeleteFile(szFile);
		sprintf(szFile, "%sobstacle.ini", pszPath);
		DeleteFile(szFile);

		RemoveDirectory(pszPath);
	}
}