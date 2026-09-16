
#ifndef __UFONT_H__
#define __UFONT_H__
#include "../../../Engine/Src/KWin32.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftstroke.h>
#include <vector>
using namespace std;

typedef FT_Error (*FT_Init_FreeTypeFunc)(FT_Library*);
typedef FT_Error (*FT_New_FaceFunc)(FT_Library, const char*, FT_Long, FT_Face*);
typedef FT_Error (*FT_Done_FaceFunc)(FT_Face);
typedef FT_Error (*FT_Done_FreeTypeFunc)(FT_Library);
typedef FT_Error (*FT_Set_Pixel_SizesFunc)(FT_Face, FT_UInt, FT_UInt);
typedef FT_Error (*FT_Load_CharFunc)(FT_Face, FT_ULong, FT_Int32);
typedef FT_Error (*FT_Stroker_NewFunc)(FT_Library, FT_Stroker *);
typedef FT_Error (*FT_Stroker_SetFunc)(FT_Stroker, FT_Fixed, FT_Stroker_LineCap, FT_Stroker_LineJoin, FT_Fixed);
typedef FT_Error (*FT_Get_GlyphFunc)( FT_GlyphSlot, FT_Glyph *);
typedef FT_Error (*FT_Glyph_StrokeBorderFunc)( FT_Glyph *, FT_Stroker, FT_Bool, FT_Bool);
typedef FT_Error (*FT_Glyph_To_BitmapFunc)( FT_Glyph*, FT_Render_Mode, const FT_Vector*, FT_Bool);
typedef FT_Error (*FT_Done_GlyphFunc)( FT_Glyph);
typedef FT_Error (*FT_Stroker_DoneFunc)( FT_Stroker);
typedef FT_Error (*FT_Render_GlyphFunc)( FT_GlyphSlot, FT_Render_Mode);

struct UCharPIXInfo
{
	BYTE* str_buffer;
	BYTE* buffer;
	UINT uId;
	int str_rows;
	int str_width;
	int str_pitch;
	int baseline;
	int rows;
	int width;
	int pitch;
	int str_bitmap_left;
	int str_bitmap_top;
	int bitmap_left;
	int bitmap_top;
	int glyph_advance;
	UCharPIXInfo()
	{
		str_buffer = NULL;
		buffer = NULL;
	}
	~UCharPIXInfo()
	{
		if(str_buffer)
		{
			delete [] str_buffer;
			str_buffer = NULL;
		}
		if(buffer)
		{
			delete [] buffer;
			buffer = NULL;
		}
	}
};

struct VFTFontInfo
{
	char			name[32];
	FT_Face			face;
	vector<UCharPIXInfo*>	vecCharSize;
	unsigned int	uNameID;
	VFTFontInfo()
	{
		face = NULL;
	}
};

class UFont
{
public:
	int Init();
	void Release();
	int FindFont(const char* pszName);
	BOOL SetSize(UINT nFontId, UINT nSize);
	FT_Stroker CreateStroker(int nThick);
	FT_Glyph CreateStrokeGlyph(FT_Face pFace, FT_Stroker stroker);
	void ReRenderGlyph(FT_GlyphSlot glyph);
	void DoneGlyph(FT_Glyph glyph);
	void DoneStroker(FT_Stroker stroker);
	UCharPIXInfo* LoadChar(UINT nFontId, FT_ULong uChar, UINT uSize);
private:
	FT_Face PrivLoadChar(UINT nFontId, FT_ULong uChar);
	int FindCharSize(UINT nFontId, UINT uCharSizeID);
private:
	vector<VFTFontInfo> m_vecFTFace;
};

wchar_t g_ConvertOne2UCEChar(unsigned char zuChar, unsigned char zSecChar);

#endif