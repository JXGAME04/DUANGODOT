// ***************************************************************************************
// 场景地图中内建对象模板类的实现定义
// Copyright : Kingsoft 2002
// Author    : wooy(wu yue)
// CreateTime: 2002-11-18
// ***************************************************************************************
#include "StdAfx.h"
#include "KSpbioTemplate.h"
#include "../../../SwordOnline/Sources/Core/Src/Scene/SceneDataDef.h"
#include "../../../SwordOnLine/Sources/Engine/Src/KEngine.h"
#include "iImage.h"

#include "fileop.h"

static const struct
{
	const char*			szDesc;
	SPBIO_PROPS_LIST	Def;
}	l_PropsDefMapping[] = 
{
	{ "平行于地表",		SPBIO_P_PLANETYPE_H			},
	{ "平行且切分",		SPBIO_P_PLANETYPE_H_D		},
	{ "垂直不切分",		SPBIO_P_PLANETYPE_V			},
	{ "垂直于地表",		SPBIO_P_PLANETYPE_V_D		},

	{ "不受光",			SPBIO_P_LIT_NONE			},
	{ "按照切分受光",	SPBIO_P_LIT_DIV				},
	{ "按照重心受光",	SPBIO_P_LIT_CENTRE			},
	{ "平行受光",		SPBIO_P_LIT_PARALLEL		},

	{ "不挡光",			SPBIO_P_BLOCK_LIGHT_NONE	},
	{ "按照底边挡光",	SPBIO_P_BLOCK_LIGHT_BOTTOM	},
	{ "按照一个圆挡光",	SPBIO_P_BLOCK_LIGHT_CIRCLE	},

	{ "0.2",			SPBIO_P_BLOCK_L_RADIUS_2	},
	{ "0.3",			SPBIO_P_BLOCK_L_RADIUS_3	},
	{ "0.4",			SPBIO_P_BLOCK_L_RADIUS_4	},
	{ "0.5",			SPBIO_P_BLOCK_L_RADIUS_5	},

	{ "按照点排序",		SPBIO_P_SORTMANNER_POINT	},
	{ "按照边排序",		SPBIO_P_SORTMANNER_LINE		},
	{ "按照树排序",		SPBIO_P_SORTMANNER_TREE		},

};

static const struct
{
	const char*			szSection;
	SPBIO_PROPS_LIST	PropMask;
	int					nStartMappingItem;
	int					nEndMappingItem;
}		l_PropUnits[] = 
{
	{ "planetype",			SPBIO_P_PLANETYPE_MASK,			0,	3	},
	{ "planelightingtype",	SPBIO_P_LIT_MASK,				4,	7	},
	{ "planelightedtype",	SPBIO_P_BLOCK_LIGHT_MASK,		8,	10	},
	{ "planelightedradius",	SPBIO_P_BLOCK_L_RADIUS_MASK,	11, 14	},
	{ "versort",			SPBIO_P_SORTMANNER_MASK,		15, 17	},
};

//##ModelId=3DDD1236014C
KSpbioTemplate::KSpbioTemplate()
{
	m_szImageFile[0] = 0;
	m_nNumFrames  = 0;
	m_nIndex = 0;
	m_uProps = 0;
	m_nCentreX = 0;
	m_nCentreY = 0;
	m_fAngleXY = 0;
	m_bSetSortManner = false;
	m_nAniSpeed = 0;
	m_nImgWidth = 0;
	m_nImgHeight = 0;
}

bool FileNameExceed(HWND hWnd, const char* pFile);

//##ModelId=3DDD1BD60159
bool KSpbioTemplate::Load(const char* pszFile, int nIndex)
{
	KIniFile	Ini;
	
	m_nIndex = nIndex;
	m_nNumFrames = 0;
	m_bSetSortManner = false;
	m_szImageFile[0] = 0;
	m_uProps = 0;
	m_nCentreX = 0;
	m_nCentreY = 0;
	m_fAngleXY = 0;
	m_nAniSpeed = 0;
	m_nImgWidth = 0;
	m_nImgHeight = 0;

	CString str = pszFile;
	AppGetFullPath(str);
	if (!Ini.Load(str))
	{
//		ASSERT(0);
		return false;
	}

	CString	ImgFile;
	ImgFile = pszFile;
	int nPos = ImgFile.ReverseFind('\\');
	ImgFile = ImgFile.Left(nPos + 1);

	if (!LoadFromIniSection(&Ini, "MAIN", ImgFile))
		return false;

	char	Section[16];
	itoa(nIndex, Section, 10);
	if (!LoadFromIniSection(&Ini, Section, ImgFile))
		return false;

	m_nCentreX = -1;
	m_nCentreY = -1;
	int nValue;
	if (Ini.GetInteger(Section, "sprcenterx", 0, &nValue) && nValue != -1)
		m_nCentreX = nValue;
	if (Ini.GetInteger(Section, "sprcentery", 0, &nValue) && nValue != -1)
		m_nCentreY = nValue;
	if (Ini.GetInteger("MAIN", "centerx", 0, &nValue) && nValue != -1)
		m_nCentreX = nValue;
	if (Ini.GetInteger("MAIN", "centery", 0, &nValue) && nValue != -1)
		m_nCentreY = nValue;
	if (Ini.GetInteger(Section, "centerx", 0, &nValue))
		m_nCentreX = nValue;
	if (Ini.GetInteger(Section, "centery", 0, &nValue))
		m_nCentreY = nValue;

	if (Ini.GetInteger(Section, "sprwidth", 0, &nValue))
		m_nImgWidth = nValue;
	if (Ini.GetInteger(Section, "sprheight", 0, &nValue))
		m_nImgHeight = nValue;

	if (m_fAngleXY == 90)
	{
		m_szImageFile[0] = 0;
		MessageBox(AfxGetMainWnd()->m_hWnd, pszFile, "对象排序边某指定角度为直角！",
			MB_OK | MB_ICONERROR);
		return false;
	}
	
	if (m_nCentreX == -1 && m_nCentreY == -1)
	{
		str = m_szImageFile;
		AppGetFullPath(str);

		KSprite	spr;
		SPRFRAME* pFrame = NULL;
		if (spr.Load((char*)(const char*)str) &&
			(pFrame = (SPRFRAME*)spr.GetFrame(m_nIndex)))
		{
			m_nCentreX = spr.GetCenterX() - pFrame->OffsetX;
			m_nCentreY = spr.GetCenterY() - pFrame->OffsetY;
			spr.Free();
		}
		else
		{
			m_nCentreX = 0;
			m_nCentreY = 0;
		}
	}

	if (m_nNumFrames <= 1)
		m_nAniSpeed = 0;
	else if (m_nAniSpeed)
		m_nAniSpeed = 1;

	if ((m_uProps & SPBIO_P_PLANETYPE_MASK) == SPBIO_P_PLANETYPE_V)
	{
		m_uProps &= ~SPBIO_P_SORTMANNER_MASK;
		m_uProps |= SPBIO_P_SORTMANNER_POINT;
	}
	return true;
}

//##ModelId=3DDD1F4D00B4
bool KSpbioTemplate::LoadFromIniSection(class KIniFile* pIni, const char* pszSection,
										const char* pszPath)
{
	//类内部调用，注意让参数传入必为非空，此处略去参数指针为空判断
	char	Buffer[256], Image[128];
	if (pIni->GetString(pszSection, "file", "", Image, sizeof(Image)) &&
		Image[0])
	{
		sprintf(Buffer, "%s%s", pszPath, Image);
		if (strlen(Buffer) < MAX_RESOURCE_FILE_NAME_LEN)
			strcpy(m_szImageFile, Buffer);
		else
		{
			m_szImageFile[0] = 0;
			if (FileNameExceed(AfxGetMainWnd()->m_hWnd, Buffer) == false)
				return false;
		}
	}

	int	nValue;
	if (pIni->GetInteger(pszSection, "frames",  0, &nValue))
		m_nNumFrames = nValue;
	if (pIni->GetInteger(pszSection, "anispeed", 0, &nValue))
		m_nAniSpeed = nValue;

	float fValue;
	if (pIni->GetFloat(pszSection, "anglexy", 0, &fValue))
		m_fAngleXY = fValue;

	Buffer[0] = 0;

	for (int i = 0; i < 5; i++)
	{
		if(pIni->GetString(pszSection, l_PropUnits[i].szSection, "", Buffer, 80))
		{
			m_uProps &= ~l_PropUnits[i].PropMask;
			for (int j = l_PropUnits[i].nStartMappingItem; j <= l_PropUnits[i].nEndMappingItem; j++)
			{
				if (strcmp(l_PropsDefMapping[j].szDesc, Buffer) == 0)
				{
					m_uProps |= l_PropsDefMapping[j].Def;
					break;
				}
			}
			if (i == 4)
				m_bSetSortManner = true;
		}
	}
	return true;
}

//##ModelId=3DE1385502EA
void KSpbioTemplate::Instance(KBuildinObj* pInstance, int& nCentreX, int& nCentreY)
{
	if (pInstance)
	{
		if (m_bSetSortManner)
			pInstance->Props = m_uProps;
		else
			pInstance->Props = m_uProps | (pInstance->Props & SPBIO_P_SORTMANNER_MASK);
		strcpy(pInstance->szImage, m_szImageFile);
		pInstance->nImgNumFrames = m_nNumFrames;
		pInstance->nOrder = 0;
		nCentreX = m_nCentreX;
		nCentreY = m_nCentreY;
		pInstance->fAngleXY = m_fAngleXY;
		pInstance->fNodicalY = 0;
		pInstance->nAniSpeed = m_nAniSpeed;
		pInstance->nFrame = m_nIndex;
		if (m_nImgWidth)
			pInstance->nImgWidth = m_nImgWidth;
		if (m_nImgHeight)
			pInstance->nImgHeight = m_nImgHeight;
	}
}
