#include "UFont.h"
#include "../../../Engine/src/KIniFile.h"
#include "../../../Engine/src/KDebug.h"

#define	defPATH_UFONT_INIFILE	"\\ufont\\fontlist.ini"

static HMODULE	m_hFTdll = NULL;
static FT_Library m_ftLib = NULL;

struct StrConvertCode
{
	unsigned short uChar;
	unsigned int uUCECode;
};

static StrConvertCode sConvertUCEChar[]=
{
	{(BYTE)'µ',0x00E0},
	{(BYTE)'¶',0x1EA3},
	{(BYTE)'·',0x00E3},
	{(BYTE)'¸',0x00E1},
	{(BYTE)'¹',0x1EA1},
	{(BYTE)'¨',0x0103},
	{(BYTE)'»',0x1EB1},
	{(BYTE)'¼',0x1EB3},
	{(BYTE)'½',0x1EB5},
	{(BYTE)'¾',0x1EAF},
	{(BYTE)'Æ',0x1EB7},
	{(BYTE)'©',0x00E2},
	{(BYTE)'Ç',0x1EA7},
	{(BYTE)'È',0x1EA9},
	{(BYTE)'É',0x1EAB},
	{(BYTE)'Ê',0x1EA5},
	{(BYTE)'Ë',0x1EAD},
	{(BYTE)'§',0x0110},
	{(BYTE)'®',0x0111},
	{(BYTE)'Ì',0x00E8},
	{(BYTE)'Î',0x1EBB},
	{(BYTE)'Ï',0x1EBD},
	{(BYTE)'Ð',0x00E9},
	{(BYTE)'Ñ',0x1EB9},
	{(BYTE)'ª',0x00EA},
	{(BYTE)'Ò',0x1EC1},
	{(BYTE)'Ó',0x1EC3},
	{(BYTE)'Ô',0x1EC5},
	{(BYTE)'Õ',0x1EBF},
	{(BYTE)'Ö',0x1EC7},
	{(BYTE)'×',0x00EC},
	{(BYTE)'Ø',0x1EC9},
	{(BYTE)'Ü',0x0129},
	{(BYTE)'Ý',0x00ED},
	{(BYTE)'Þ',0x1ECB},
	{(BYTE)'ß',0x00F2},
	{(BYTE)'á',0x1ECF},
	{(BYTE)'â',0x00F5},
	{(BYTE)'ã',0x00F3},
	{(BYTE)'ä',0x1ECD},
	{(BYTE)'«',0x00F4},
	{(BYTE)'å',0x1ED3},
	{(BYTE)'æ',0x1ED5},
	{(BYTE)'ç',0x1ED7},
	{(BYTE)'è',0x1ED1},
	{(BYTE)'é',0x1ED9},
	{(BYTE)'¬',0x01A1},
	{(BYTE)'ê',0x1EDD},
	{(BYTE)'ë',0x1EDF},
	{(BYTE)'ì',0x1EE1},
	{(BYTE)'í',0x1EDB},
	{(BYTE)'î',0x1EE3},
	{(BYTE)'ï',0x00F9},
	{(BYTE)'ñ',0x1EE7},
	{(BYTE)'ò',0x0169},
	{(BYTE)'ó',0x00FA},
	{(BYTE)'ô',0x1EE5},
	{(BYTE)'­',0x01B0},
	{(BYTE)'õ',0x1EEB},
	{(BYTE)'ö',0x1EED},
	{(BYTE)'÷',0x1EEF},
	{(BYTE)'ø',0x1EE9},
	{(BYTE)'ù',0x1EF1},
	{(BYTE)'ú',0x1EF3},
	{(BYTE)'û',0x1EF7},
	{(BYTE)'ü',0x1EF9},
	{(BYTE)'ý',0x00FD},
	{(BYTE)'þ',0x1EF5},
	{129,0x00C0},
	{130,0x1EA2},
	{131,0x00C3},
	{128,0x00C1},
	{132,0x1EA0},
	{(BYTE)'¡',0x0102},
	{137,0x1EB0},
	{140,0x1EB2},
	{141,0x1EB4},
	{135,0x1EAE},
	{142,0x1EB6},
	{(BYTE)'¢',0x00C2},
	{144,0x1EA6},
	{152,0x1EA8},
	{153,0x1EAA},
	{143,0x1EA4},
	{158,0x1EAC},
	{160,0x00C8},
	{175,0x1EBA},
	{177,0x1EBC},
	{159,0x00C9},
	{178,0x1EB8},
	{(BYTE)'£',0x00CA},
	{180,0x1EC0},
	{186,0x1EC2},
	{191,0x1EC4},
	{179,0x1EBE},
	{197,0x1EC6},
	{0xb1b,0x00CC},
	{0xb1c,0x1EC8},
	{0xb1d,0x0128},
	{0xb1a,0x00CD},
	{0xb1e,0x1ECA},
	{0xb02,0x00D2},
	{0xb03,0x1ECE},
	{0xb04,0x00D5},
	{0xb01,0x00D3},
	{0xb05,0x1ECC},
	{(BYTE)'¤',0x00D4},
	{0xb07,0x1ED2},
	{0xb08,0x1ED4},
	{0xb09,0x1ED6},
	{0xb06,0x1ED0},
	{0xb0a,0x1ED8},
	{(BYTE)'¥',0x01A0},
	{0xb0c,0x1EDC},
	{0xb0d,0x1EDE},
	{0xb0e,0x1EE0},
	{0xb0b,0x1EDA},
	{0xb0f,0x1EE2},
	{0xb11,0x00D9},
	{0xb12,0x1EE6},
	{0xb13,0x0168},
	{0xb10,0x00DA},
	{0xb14,0x1EE4},
	{(BYTE)'¦',0x01AF},
	{0xb16,0x1EEA},
	{0xb17,0x1EEC},
	{0xb18,0x1EEE},
	{0xb15,0x1EE8},
	{0xb19,0x1EF0},
	{0xb20,0x1EF2},
	{0xb21,0x1EF6},
	{0xb22,0x1EF8},
	{0xb1f,0x00DD},
	{0xb23,0x1EF4},
	//spec char
	{133,0x2026},
	{134,0x04AA},
	{136,0x00CB},
	{138,0x00CF},
	{139,0x00CE},
	{145,0x0312},
	{146,0x0313},
	{147,0x201C},
	{148,0x201D},
	{149,0x25CF},
	{150,0x2014},
	{151,0x2013},
	{154,0x00DC},
	{155,0x00DB},
	{156,0x05C0},
	{157,0x02C7},
	{176,0x00B0},
	{192,0x0394},
	{193,0x263C},
	{194,0x25CA},
	{195,0x00EB},
	{196,0x0192},
	{205,0x263A},
	{217,0x0020},
	{218,0x00EF},
	{219,0x00EE},
	{224,0x2665},
	{240,0x266A},
	{255,0x0020},
};

UINT UFontName2Id(const char* lpFileName)
{
	unsigned int id = 0;
	const char *ptr = lpFileName;
	int index = 0;
	while(*ptr)
	{
		if(*ptr >= 'A' && *ptr <= 'Z')
			id = (id + (++index) * (*ptr + 'a' - 'A')) % 0x8000000b * 0xffffffef;
		else
			id = (id + (++index) * (*ptr)) % 0x8000000b * 0xffffffef;
		ptr++;
	}
	return (id ^ 0x12345678);
}

wchar_t g_ConvertOne2UCEChar(unsigned char zuChar, unsigned char zSecChar)
{
	wchar_t wcResult = (unsigned int)zuChar;
	if(wcResult == 0x0B)
		wcResult = 0x0B00 | zSecChar;
	if(wcResult > 0x7Eu)
	{
		for(int i=0;i < sizeof(sConvertUCEChar)/sizeof(StrConvertCode); i++)
		{
			if(wcResult == sConvertUCEChar[i].uChar)
			{
				wcResult = sConvertUCEChar[i].uUCECode;
				break;
			}
		}
	}
	return wcResult;
}

int UFont::Init()
{
	if(m_hFTdll)
		return 0;
	m_hFTdll = LoadLibrary("freetype.dll");
	if(m_hFTdll)
	{
		FT_Init_FreeTypeFunc ftInit = (FT_Init_FreeTypeFunc)GetProcAddress(m_hFTdll, "FT_Init_FreeType");
		if (ftInit)
		{
			if (ftInit(&m_ftLib) == 0)
			{
				FT_New_FaceFunc ftFace = (FT_New_FaceFunc)GetProcAddress(m_hFTdll, "FT_New_Face");
				KIniFile File;
				if(File.Load(defPATH_UFONT_INIFILE))
				{
					int nFontCount;
					File.GetInteger("FontList", "Count", 10, &nFontCount);
					char szPath[128];
					char szBuffer[32];
					for(int i = 0; i < nFontCount; ++i)
					{
						sprintf(szPath, "%d",i);
						File.GetString("FontList", szPath, "", szBuffer, sizeof(szBuffer));
						if(!szBuffer[0])
							continue;
						sprintf(szPath, "ufont/%s", szBuffer);
						char* ptr = NULL;
						if(ptr = strstr(szBuffer, "."))
							*ptr = 0;
						VFTFontInfo sFtInfo;
						strcpy(sFtInfo.name, szBuffer);
						sFtInfo.uNameID = UFontName2Id(szBuffer);
						if (ftFace(m_ftLib, szPath, 0, &sFtInfo.face) == 0)
						{
							m_vecFTFace.push_back(sFtInfo);
						}
					}
					return 0;
				}
			}
		}
	}
	return 1;
}

void UFont::Release()
{
	if(m_hFTdll)
	{
		FT_Done_FaceFunc ftDone = (FT_Done_FaceFunc)GetProcAddress(m_hFTdll, "FT_Done_Face");
		for(size_t i = 0; i < m_vecFTFace.size(); ++i)
		{
			ftDone(m_vecFTFace[i].face);
			for(size_t j = 0; j < m_vecFTFace[i].vecCharSize.size(); ++j)
			{
				if(m_vecFTFace[i].vecCharSize[j])
				{
					delete m_vecFTFace[i].vecCharSize[j];
					m_vecFTFace[i].vecCharSize[j] = NULL;
				}
			}
			m_vecFTFace[i].vecCharSize.clear();
		}
		if(m_ftLib)
		{
			FT_Done_FreeTypeFunc ftDoneLib = (FT_Done_FreeTypeFunc)GetProcAddress(m_hFTdll, "FT_Done_FreeType");
			ftDoneLib(m_ftLib);
		}
		FreeLibrary(m_hFTdll);
		m_hFTdll = NULL;
	}
	m_vecFTFace.clear();
}

int UFont::FindFont(const char* pszName)
{
	if(!m_hFTdll || m_vecFTFace.size() == 0)
		return -1;
	UINT uNameID = UFontName2Id(pszName);
	for(size_t i = 0; i < m_vecFTFace.size(); ++i)
	{
		if(m_vecFTFace[i].uNameID == uNameID)
			return (int)i;
	}
	return -1;
}

BOOL UFont::SetSize(UINT nFontId, UINT nSize)
{
	if(nFontId >= m_vecFTFace.size())
		return FALSE;
	VFTFontInfo& sFtInfo = m_vecFTFace[nFontId];
	FT_Set_Pixel_SizesFunc ftSetFunc = (FT_Set_Pixel_SizesFunc)GetProcAddress(m_hFTdll, "FT_Set_Pixel_Sizes");
	if(!ftSetFunc)
		return FALSE;
	if(ftSetFunc(sFtInfo.face, 0, nSize))
		return FALSE;
	return TRUE;
}

FT_Face UFont::PrivLoadChar(UINT nFontId, FT_ULong uChar)
{
	if(nFontId >= m_vecFTFace.size())
		return NULL;
	VFTFontInfo& sFtInfo = m_vecFTFace[nFontId];
	FT_Load_CharFunc ftCharFunc = (FT_Load_CharFunc)GetProcAddress(m_hFTdll, "FT_Load_Char");
	if(!ftCharFunc)
		return NULL;
	if(ftCharFunc(sFtInfo.face, uChar, FT_LOAD_DEFAULT ))
		return NULL;
	return m_vecFTFace[nFontId].face;
}

FT_Stroker UFont::CreateStroker(int nThick)
{
	FT_Stroker_NewFunc ftStrNewFunc = (FT_Stroker_NewFunc)GetProcAddress(m_hFTdll, "FT_Stroker_New");
	FT_Stroker_SetFunc ftStrSetFunc = (FT_Stroker_SetFunc)GetProcAddress(m_hFTdll, "FT_Stroker_Set");
	if(!ftStrNewFunc || !ftStrSetFunc)
		return NULL;
    FT_Stroker stroker;
    if (ftStrNewFunc(m_ftLib, &stroker) == 0)
    {
        ftStrSetFunc(stroker, nThick*64, FT_STROKER_LINECAP_BUTT, FT_STROKER_LINEJOIN_MITER, 0);
        return stroker;
    }
    return NULL;
}

FT_Glyph UFont::CreateStrokeGlyph(FT_Face pFace, FT_Stroker stroker)
{
	FT_Glyph glyph;
	FT_Get_GlyphFunc pGetGFunc = (FT_Get_GlyphFunc)GetProcAddress(m_hFTdll, "FT_Get_Glyph");
	FT_Glyph_StrokeBorderFunc pGSBorderFunc =
				(FT_Glyph_StrokeBorderFunc)GetProcAddress(m_hFTdll, "FT_Glyph_StrokeBorder");
	FT_Glyph_To_BitmapFunc pGToBitmapFunc =
				(FT_Glyph_To_BitmapFunc)GetProcAddress(m_hFTdll, "FT_Glyph_To_Bitmap");
	if(!pGetGFunc || !pGSBorderFunc || !pGToBitmapFunc)
		return NULL;
	pGetGFunc(pFace->glyph, &glyph);
	pGSBorderFunc(&glyph, stroker, 0, 1);
	pGToBitmapFunc(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
	return glyph;
}

void UFont::ReRenderGlyph(FT_GlyphSlot glyph)
{
	FT_Render_GlyphFunc pRenderGlyph = (FT_Render_GlyphFunc)GetProcAddress(m_hFTdll, "FT_Render_Glyph");
	if(pRenderGlyph)
		pRenderGlyph(glyph, FT_RENDER_MODE_NORMAL);
}

void UFont::DoneGlyph(FT_Glyph glyph)
{
	FT_Done_GlyphFunc pDoneGlyph = (FT_Done_GlyphFunc)GetProcAddress(m_hFTdll, "FT_Done_Glyph");
	if(pDoneGlyph)
		pDoneGlyph(glyph);
}

void UFont::DoneStroker(FT_Stroker stroker)
{
	FT_Stroker_DoneFunc pDoneStroker = (FT_Stroker_DoneFunc)GetProcAddress(m_hFTdll, "FT_Stroker_Done");
	if(pDoneStroker)
		pDoneStroker(stroker);
}

UCharPIXInfo* UFont::LoadChar(UINT nFontId, FT_ULong uChar, UINT uSize)
{
	if(nFontId >= m_vecFTFace.size())
		return NULL;
	UINT uCharSizeID = (UINT)uChar | (uSize << 16);
	int nCharSizePos = FindCharSize(nFontId, uCharSizeID);
	if(nCharSizePos >= 0)
	{
		return m_vecFTFace[nFontId].vecCharSize[nCharSizePos];
	}
	else
	{
		nCharSizePos = - nCharSizePos - 1;
		UCharPIXInfo* node = new UCharPIXInfo;
		node->uId = uCharSizeID;
		m_vecFTFace[nFontId].vecCharSize.insert(m_vecFTFace[nFontId].vecCharSize.begin() + nCharSizePos, node);
		
		FT_Face pFace = PrivLoadChar(nFontId, uChar);
		if(!pFace)
		{
			m_vecFTFace[nFontId].vecCharSize[nCharSizePos] = NULL;
			delete node;
			node = NULL;
			return NULL;
		}
		node->baseline = pFace->size->metrics.ascender >> 6;
		int nThick = uSize/24+1;
		FT_Stroker stroker = CreateStroker(nThick);
		if(stroker)
		{
			FT_Glyph strglyph = CreateStrokeGlyph(pFace, stroker);
			if(strglyph)
			{
				FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)strglyph;
				FT_Bitmap& bmp = bitmap_glyph->bitmap;
				node->str_bitmap_left = bitmap_glyph->left;
				node->str_bitmap_top = bitmap_glyph->top;
				node->str_rows = bmp.rows;
				node->str_width = bmp.width;
				node->str_pitch = bmp.pitch;
				node->str_buffer = new BYTE[bmp.rows*bmp.pitch];
				memcpy(node->str_buffer, bmp.buffer, bmp.rows*bmp.pitch);
				DoneGlyph(strglyph);
			}
			DoneStroker(stroker);
		}
		ReRenderGlyph(pFace->glyph);
		FT_GlyphSlot glyph = pFace->glyph;
		FT_Bitmap& bitmap = glyph->bitmap;
		node->bitmap_left = glyph->bitmap_left;
		node->bitmap_top = glyph->bitmap_top;
		node->rows = bitmap.rows;
		node->width = bitmap.width;
		node->pitch = bitmap.pitch;
		node->buffer = new BYTE[bitmap.rows*bitmap.pitch];
		memcpy(node->buffer, bitmap.buffer, bitmap.rows*bitmap.pitch);
		node->glyph_advance = glyph->advance.x >> 6;
		return node;
	}
	return NULL;
}

int UFont::FindCharSize(UINT nFontId, UINT uCharSizeID)
{
	vector<UCharPIXInfo*>& csvec = m_vecFTFace[nFontId].vecCharSize;
	if(csvec.size() == 0)
		return -1;
	int nNum = csvec.size();
	int nPP = nNum / 2;
	if (csvec[nPP]->uId == uCharSizeID)
		return nPP;
	int nFrom, nTo, nTryRange;
	nTryRange = 8;
	if (csvec[nPP]->uId > uCharSizeID)
	{
		nFrom = 0;
		nTo = nPP - 1;
		nPP -= nTryRange;
	}
	else
	{
		nFrom = nPP + 1;
		nTo = nNum - 1;
		nPP += nTryRange;
	}
	if (nFrom + nTryRange >= nTo)
		nPP = (nFrom + nTo) / 2;

	while (nFrom < nTo)
	{
		if (csvec[nPP]->uId < uCharSizeID)
		{
			nFrom = nPP + 1;
		}
		else if (csvec[nPP]->uId > uCharSizeID)
		{
			nTo = nPP - 1;
		}
		else
		{
			return nPP;
		}
		nPP = (nFrom + nTo) / 2;
	}
	if (nFrom == nTo)
	{
		if (csvec[nPP]->uId > uCharSizeID)
		{
			nPP = - nPP - 1;
		}
		else if (csvec[nPP]->uId < uCharSizeID)
		{
			nPP = - nPP - 2;
		}
	}
	else
	{
		nPP = - nFrom -1;
	}
	return nPP;
}
