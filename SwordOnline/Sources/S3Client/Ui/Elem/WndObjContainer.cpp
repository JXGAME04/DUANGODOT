/*****************************************************************************************
//	界面窗口体系结构--容纳游戏对象的窗口
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-9-25
*****************************************************************************************/
#include "KWin32.h"
#include "KIniFile.h"
#include "../Elem/WndMessage.h"
#include "../elem/wnds.h"
#include "WndObjContainer.h"
#include "../Elem/MouseHover.h"
#include "../../../core/src/CoreObjGenreDef.h"
#include "../../../core/src/CoreShell.h"
#include "../../../core/src/GameDataDef.h"
#include "../../../Represent/iRepresent/iRepresentShell.h"
#include "../../../Represent/iRepresent/KRepresentUnit.h"
#include "../../../Engine/Src/Text.h"
#include "../../../Engine/Src/KRandom.h"
#include "KDebug.h"
extern iCoreShell*	g_pCoreShell;
extern iRepresentShell*	g_pRepresentShell;


unsigned int l_BgColors[] =
{
	0x0a001e13,		//IIEP_NORMAL 一般/正常/可用 0,30,19
	0x0a2c0000,		//IIEP_NOT_USEABLE 不可用/不可装配 44,0,0
	0x0a654915,		//IIEP_SPECIAL 特定的不同情况 101，73，21
	0x0a000636,		//mouse over 0,6,54
	0x0a0000ff,		//放下物品的位置的颜色
	0x0aad8579,
};

#define	defBorderDelayTime	60	//t鑓  vi襫 n襫 mili gi﹜, c祅g cao c祅g ch薽
#define	defBColorDelayTime	110	//t鑓  vi襫 ch輓h mili gi﹜ (ch箉 xung quanh), c祅g cao c祅g ch薽

struct SRGBColor
{
	int r;
	int g;
	int b;
};

static struct UBD_CLR_MAP
{
	SRGBColor		Border;	//m祏 vi襫 n襫
	SRGBColor		Color;	//m祏 ch輓h tr猲 c飊g
	SRGBColor		Light;	//
}BorderColorMap[extype_number-1] =
{
	{{100,80,30},{243,194,70},{255,255,170}},		//Ho祅g Kim
	{{110,110,110},{240,240,240},{255,255,255}},	//B筩h Kim
	{{77,30,100},{188,80,255},{255,170,255}},	//T輒
};

static void DrawBorder(int nExType, int x, int y, int w, int h, float fpercent)
{
	int R = BorderColorMap[nExType].Border.r
		+ (int)(fpercent*(BorderColorMap[nExType].Color.r - BorderColorMap[nExType].Border.r));
	int G = BorderColorMap[nExType].Border.g
		+ (int)(fpercent*(BorderColorMap[nExType].Color.g - BorderColorMap[nExType].Border.g));
	int B = BorderColorMap[nExType].Border.b
		+ (int)(fpercent*(BorderColorMap[nExType].Color.b - BorderColorMap[nExType].Border.b));
	KRURect	rect;
	rect.Color.Color_b.a = 255;
	rect.Color.Color_b.r = R;
	rect.Color.Color_b.g = G;
	rect.Color.Color_b.b = B;
	rect.oEndPos.nX = rect.oPosition.nX = x;
	rect.oEndPos.nY = rect.oPosition.nY = y;
	rect.oEndPos.nZ = rect.oPosition.nZ = 0;
	rect.oEndPos.nX += w;
	rect.oEndPos.nY += h;
	g_pRepresentShell->DrawPrimitives(1, &rect, RU_T_RECT, true);
}

static void DrawLight(int nExType, int x, int y, int w, int h, int nAdd)
{
	RECT rcOld;
	g_pRepresentShell->GetClipRect(rcOld);
	RECT	rc;
	rc.left  = x;
	rc.top   = y;
	rc.right = x+w+1;
	rc.bottom= y+h+1;
	g_pRepresentShell->SetClipRect(rc);
	UINT uColor = 0x30000000 | (BorderColorMap[nExType].Light.r << 16)
		| (BorderColorMap[nExType].Light.g << 8) | BorderColorMap[nExType].Light.b;
	int x1 = x;
	int y1 = y-20;
	int x2 = x1 + w;
	int y2 = y1 - w;
	int x3 = x1;
	int y3 = y1 + 20;
	y1 += nAdd;
	y2 += nAdd;
	y3 += nAdd;
	g_pRepresentShell->FillColorTriangle( x1,  y1,  x2,  y2,  x3,  y3, uColor);
	int x4 = x + w;
	int y4 = y2 + 20;
	g_pRepresentShell->FillColorTriangle( x3,  y3,  x2,  y2,  x4,  y4, uColor);
	
	KRULine line;
	line.Color.Color_dw = 0xff000000 | (BorderColorMap[nExType].Color.r << 16)
	| (BorderColorMap[nExType].Color.g << 8) | BorderColorMap[nExType].Color.b;
	line.oPosition.nX = x1;
	line.oPosition.nY = y1;
	line.oEndPos.nX = x3;
	line.oEndPos.nY = y3;
	g_pRepresentShell->DrawPrimitives(1, &line, RU_T_LINE, true);
	line.oPosition.nX = x2;
	line.oPosition.nY = y2;
	line.oEndPos.nX = x4;
	line.oEndPos.nY = y4;
	g_pRepresentShell->DrawPrimitives(1, &line, RU_T_LINE, true);
	
	if(y2 < y && y3 > y)
	{
		line.oPosition.nX = x2 - (y - y2);
		line.oPosition.nY = y;
		line.oEndPos.nX = x3 + (y3 - y);
		line.oEndPos.nY = y;
		g_pRepresentShell->DrawPrimitives(1, &line, RU_T_LINE, true);
	}
	if(y2 < y+h && y3 > y+h)
	{
		line.oPosition.nX = x2 - ((y+h) - y2);
		line.oPosition.nY = y+h;
		line.oEndPos.nX = x3 + (y3 - (y+h));
		line.oEndPos.nY = y+h;
		g_pRepresentShell->DrawPrimitives(1, &line, RU_T_LINE, true);
	}
	g_pRepresentShell->SetClipRect(rcOld);
	
}

static void DrawBColor(int nExType, int x, int y, int w, int h, int pos1, int pos2)
{
	KRUShadow	rect;
	rect.bUseNewAlpha = FALSE;
	rect.Color.Color_b.r = BorderColorMap[nExType].Color.r;
	rect.Color.Color_b.g = BorderColorMap[nExType].Color.g;
	rect.Color.Color_b.b = BorderColorMap[nExType].Color.b;
	rect.oEndPos.nZ = rect.oPosition.nZ = 0;
	int nRange = w*2 + h*2;
	for(int i=0; i<32; ++i)
	{
		rect.Color.Color_b.a = i;
		int p = pos1 - i;
		if(p < 0)
			p += nRange;
		if(p < w)
		{
			rect.oPosition.nX = p;
			rect.oPosition.nY = 0;
		}
		else if(p <= w+h)
		{
			rect.oPosition.nX = w;
			rect.oPosition.nY = p-w;
		}
		else if(p <= w*2+h)
		{
			rect.oPosition.nX = w-(p-(w+h));
			rect.oPosition.nY = h;
		}
		else
		{
			rect.oPosition.nX = 0;
			rect.oPosition.nY = nRange-p;
		}
		rect.oPosition.nX += x;
		rect.oPosition.nY += y;
		rect.oEndPos.nX = rect.oPosition.nX + 1;
		rect.oEndPos.nY = rect.oPosition.nY + 1;
		g_pRepresentShell->DrawPrimitives(1, &rect, RU_T_SHADOW, true);
		p = pos2 - i;
		if(p < 0)
			p += nRange;
		if(p < w)
		{
			rect.oPosition.nX = p;
			rect.oPosition.nY = 0;
		}
		else if(p <= w+h)
		{
			rect.oPosition.nX = w;
			rect.oPosition.nY = p-w;
		}
		else if(p <= w*2+h)
		{
			rect.oPosition.nX = w-(p-(w+h));
			rect.oPosition.nY = h;
		}
		else
		{
			rect.oPosition.nX = 0;
			rect.oPosition.nY = nRange-p;
		}
		rect.oPosition.nX += x;
		rect.oPosition.nY += y;
		rect.oEndPos.nX = rect.oPosition.nX + 1;
		rect.oEndPos.nY = rect.oPosition.nY + 1;
		g_pRepresentShell->DrawPrimitives(1, &rect, RU_T_SHADOW, true);
	}
}

void WndObjContainerInit(KIniFile* pIni)
{
#define	COLOR_SECTION	"ObjContColor"
	if (pIni)
	{
		char	Buff[16];
		int		nAlpha;
		pIni->GetInteger(COLOR_SECTION, "Alpha", 0, &nAlpha);
		if (nAlpha < 0)
			nAlpha = 0;
		else if (nAlpha > 255)
			nAlpha = 255;
		nAlpha = ((nAlpha << 21) & 0xff000000);
		pIni->GetString(COLOR_SECTION, "NormalColor", "", Buff, sizeof(Buff));
		l_BgColors[0] = (GetColor(Buff) & 0xffffff) | nAlpha;
		pIni->GetString(COLOR_SECTION, "NotUseableColor", "", Buff, sizeof(Buff));
		l_BgColors[1] = (GetColor(Buff) & 0xffffff) | nAlpha;
		pIni->GetString(COLOR_SECTION, "SpecialColor", "", Buff, sizeof(Buff));
		l_BgColors[2] = (GetColor(Buff) & 0xffffff) | nAlpha;
		pIni->GetString(COLOR_SECTION, "MouseOverColor", "", Buff, sizeof(Buff));
		l_BgColors[3] = (GetColor(Buff) & 0xffffff) | nAlpha;
		pIni->GetString(COLOR_SECTION, "PutdownColor", "", Buff, sizeof(Buff));
		l_BgColors[4] = (GetColor(Buff) & 0xffffff) | nAlpha;
		pIni->GetString(COLOR_SECTION, "PriceMarkedColor", "", Buff, sizeof(Buff));
		l_BgColors[5] = (GetColor(Buff) & 0xffffff) | nAlpha;
	}
}

//--------------------------------------------------------------------------
//	功能：构造函数
//--------------------------------------------------------------------------
KWndObjectBox::KWndObjectBox()
{
	m_uAcceptableGenre = CGOG_NOTHING;
	m_Object.uGenre = CGOG_NOTHING;
	m_Object.DataX = 0;
	m_Object.DataY = 0;
	m_nContainerId = 0;
	m_uBdNextTime = 0;
	m_fBdPercent = 0.0f;
	m_fBdAddPerFrame = 0.05f;
	m_uBClrNextTime = 0;
	m_nAddLightPos = 0;
}

void KWndObjectBox::SetContainerId(int nId)
{
	m_nContainerId = nId;
}

//--------------------------------------------------------------------------
//	功能：设置可以容纳的对象的类型
//--------------------------------------------------------------------------
void KWndObjectBox::SetObjectGenre(unsigned int uGenre)
{
	m_uAcceptableGenre = uGenre;
}

//--------------------------------------------------------------------------
//	功能：清除对象物品
//--------------------------------------------------------------------------
void KWndObjectBox::Celar()
{
	m_Object.uGenre = CGOG_NOTHING;
}

//--------------------------------------------------------------------------
//	功能：获取容纳的对象信息
//--------------------------------------------------------------------------
int KWndObjectBox::GetObject(KUiDraggedObject& Obj) const
{
	if ((Obj.uGenre = m_Object.uGenre) != CGOG_NOTHING)
	{
		Obj = m_Object;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
//	功能：设置容纳的对象
//--------------------------------------------------------------------------
void KWndObjectBox::HoldObject(unsigned int uGenre, unsigned int uId, int nDataW, int nDataH)
{
	m_Object.uGenre = uGenre;
	m_Object.uId = uId;
	m_Object.DataW = nDataW;
	m_Object.DataH = nDataH;

	if (g_MouseOver.IsMoseHoverWndObj(this, 0))
	{
		if (m_Object.uGenre != CGOG_NOTHING)
		{
			int x, y;
			Wnd_GetCursorPos(&x, &y);
			SetMouseHoverObjectDesc(this, 0, m_Object.uGenre,
				m_Object.uId, m_nContainerId, x, y);
		}
		else
		{
			g_MouseOver.CancelMouseHoverInfo();
		}
	}
	int nRange = (m_Width - 1)*2 + (m_Height - 1)*2;
	m_sBClrPos.pos1 = g_Random(nRange);
	m_sBClrPos.pos2 = g_Random(nRange);
	if(abs(m_sBClrPos.pos2 - m_sBClrPos.pos1) < 32)
	{
		m_sBClrPos.pos2 = m_sBClrPos.pos1 + 32;
		if(m_sBClrPos.pos2 >= nRange)
			m_sBClrPos.pos2 -= nRange;
	}

}

//--------------------------------------------------------------------------
//	功能：窗体绘制
//--------------------------------------------------------------------------
void KWndObjectBox::PaintWindow()
{
	KWndWindow::PaintWindow();
	if (m_Object.uGenre != CGOG_NOTHING && g_pRepresentShell)
	{
		KRUShadow	Shadow;
		Shadow.bUseNewAlpha = FALSE;
		Shadow.Color.Color_dw = 0;
		if (m_Style & OBJCONT_F_MOUSE_HOVER)
			Shadow.Color.Color_dw = l_BgColors[3];
		else if (m_Style & OBJCONT_S_HAVEOBJBGCOLOR)
		{
			int nGenre = g_pCoreShell->GetGameData(GDI_GET_ITEM_INFO, 0, m_Object.uId);
			if(nGenre == 0)
			{
				KUiObjAtContRegion	Obj;
				Obj.Obj.uGenre = m_Object.uGenre;
				Obj.Obj.uId = m_Object.uId;
				Obj.Region.h = Obj.Region.v = 0;
				Obj.Region.Width = Obj.Region.Height = 0;
				Obj.nContainer = m_nContainerId;
	
				ITEM_IN_ENVIRO_PROP eProp = (ITEM_IN_ENVIRO_PROP)g_pCoreShell->
					GetGameData(GDI_ITEM_IN_ENVIRO_PROP, (unsigned int)&Obj, 0);
				if (eProp == IIEP_NORMAL)
					Shadow.Color.Color_dw = l_BgColors[0];
				else if (eProp == IIEP_NOT_USEABLE)
					Shadow.Color.Color_dw = l_BgColors[1];
				else if (eProp == IIEP_SPECIAL)
					Shadow.Color.Color_dw = l_BgColors[2];
			}
		}
		if (Shadow.Color.Color_dw)
		{
			Shadow.oPosition.nX = m_nAbsoluteLeft;
			Shadow.oPosition.nY = m_nAbsoluteTop;
			Shadow.oEndPos.nX = m_nAbsoluteLeft + m_Width;
			Shadow.oEndPos.nY = m_nAbsoluteTop + m_Height;
			g_pRepresentShell->DrawPrimitives(1, &Shadow, RU_T_SHADOW, true);
		}
		g_pCoreShell->DrawGameObj(m_Object.uGenre, m_Object.uId,
			m_nAbsoluteLeft, m_nAbsoluteTop, m_Width, m_Height, 0);
			
		int nExType = g_pCoreShell->GetGameData(GDI_GET_ITEM_INFO, 1, m_Object.uId);
		if(nExType > 0 && m_nContainerId == UOC_EQUIPTMENT)
		{
			//vi襫 n襫 n籱 di
			DrawBorder(nExType-1, m_nAbsoluteLeft, m_nAbsoluteTop,
			m_Width - 1, m_Height - 1,
			m_fBdPercent);
			////vi襫 s竛g b猲 tr猲
			DrawBColor(nExType-1, m_nAbsoluteLeft, m_nAbsoluteTop,
			m_Width - 1, m_Height - 1,
			m_sBClrPos.pos1, m_sBClrPos.pos2);
			DrawLight(nExType-1, m_nAbsoluteLeft, m_nAbsoluteTop,
			m_Width-1, m_Height-1, m_nAddLightPos);
			//d辌h chuy觧 vi襫 tr猲
			if(m_uBClrNextTime < timeGetTime())
			{
				m_uBClrNextTime = timeGetTime() + defBColorDelayTime;
				int nRange = (m_Width - 1)*2 + (m_Height - 1)*2;
				++m_sBClrPos.pos1;
				++m_sBClrPos.pos2;
				if(m_sBClrPos.pos1 >= nRange)
					m_sBClrPos.pos1 = 0;
				if(m_sBClrPos.pos2 >= nRange)
					m_sBClrPos.pos2 = 0;
			}
		}
		if(m_uBdNextTime < timeGetTime())
		{
			m_uBdNextTime = timeGetTime() + defBorderDelayTime;
			m_fBdPercent += m_fBdAddPerFrame;
			if(m_fBdPercent > 0.6f)
			{
				m_fBdAddPerFrame = -0.05f;
				m_fBdPercent = 0.6f + m_fBdAddPerFrame;
			}
			else if(m_fBdPercent < 0.0f)
			{
				m_fBdAddPerFrame = 0.05f;
				m_fBdPercent = m_fBdAddPerFrame;
			}
		}
			++m_nAddLightPos;
			//if(m_Object.DataH >= 3)
			//	++m_nAddLightPos;
			if(m_nAddLightPos > 200)
				m_nAddLightPos = 0;
	}
}

void KWndObjectBox::Clone(KWndObjectBox* pCopy)
{
	if (pCopy)
	{
		KWndWindow::Clone(pCopy);
		pCopy->m_uAcceptableGenre = m_uAcceptableGenre;
	}
}

//初始化
int	KWndObjectBox::Init(KIniFile* pIniFile, const char* pSection)
{
	if (KWndWindow::Init(pIniFile, pSection))
	{
		int nValue;
		pIniFile->GetInteger(pSection, "EnableClickEmpty", 0, &nValue);
		if (nValue)
			m_Style |= OBJCONT_S_ENABLE_CLICK_EMPTY;
		else
			m_Style &= ~OBJCONT_S_ENABLE_CLICK_EMPTY;

		pIniFile->GetInteger(pSection, "HaveBgColor", 1, &nValue);
		if (nValue)
			m_Style |= OBJCONT_S_HAVEOBJBGCOLOR;
		else
			m_Style &= ~OBJCONT_S_HAVEOBJBGCOLOR;
		return true;
	}
	return false;
}

void KWndObjectBox::EnablePickPut(bool bEnable)
{
	if (bEnable == false)
		m_Style |= OBJCONT_S_DISABLE_PICKPUT;
	else
		m_Style &= ~OBJCONT_S_DISABLE_PICKPUT;
}

//--------------------------------------------------------------------------
//	功能：窗口函数
//--------------------------------------------------------------------------
int KWndObjectBox::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		if (m_pParentWnd)
		{
			if ((m_Style & OBJCONT_S_DISABLE_PICKPUT) == 0)
			{
				if (Wnd_GetDragObj(NULL))
				{
					DropObject(false);
				}
				else if (m_Object.uGenre != CGOG_NOTHING)
				{
					ITEM_PICKDROP_PLACE	Pick;
					Pick.pWnd = this;
					Pick.h = 0;
					Pick.v = 0;
					m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP,
						(unsigned int)&Pick, NULL);
				}
			}
			else if (m_Object.uGenre != CGOG_NOTHING)
			{
				KUiDraggedObject	Obj;
				Obj = m_Object;
				m_pParentWnd->WndProc(WND_N_LEFT_CLICK_ITEM,
					(unsigned int)&Obj, (int)(KWndWindow*)this);
			}
			else if (m_Style & OBJCONT_S_ENABLE_CLICK_EMPTY)
			{
				m_pParentWnd->WndProc(WND_N_LEFT_CLICK_ITEM,
					NULL, (int)(KWndWindow*)this);
			}				
		}
		break;
	case WM_RBUTTONDOWN:
		if (m_pParentWnd)
		{
			if (m_Object.uGenre != CGOG_NOTHING)
			{
				KUiDraggedObject	Obj;
				Obj = m_Object;
				m_pParentWnd->WndProc(WND_N_RIGHT_CLICK_ITEM,
					(unsigned int)&m_Object, (int)(KWndWindow*)this);
			}
			else if (m_Style & OBJCONT_S_ENABLE_CLICK_EMPTY)
			{
				m_pParentWnd->WndProc(WND_N_LEFT_CLICK_ITEM,
					NULL, (int)(KWndWindow*)this);
			}
		}
		break;
	case WM_MOUSEHOVER:
	case WM_MOUSEMOVE:
		m_Style |= OBJCONT_F_MOUSE_HOVER;
		if (m_Object.uGenre != CGOG_NOTHING && g_MouseOver.IsMoseHoverWndObj(this, 0) == 0)
			SetMouseHoverObjectDesc(this, 0, m_Object.uGenre,
				m_Object.uId, m_nContainerId, LOWORD(nParam), HIWORD(nParam));
		break;
	case WND_M_MOUSE_LEAVE:
		m_Style &= ~OBJCONT_F_MOUSE_HOVER;
		KWndWindow::WndProc(uMsg, uParam, nParam);
		break;
	default:
		return KWndWindow::WndProc(uMsg, uParam, nParam);
	}
	return 0;
}

//--------------------------------------------------------------------------
//	功能：放置物品
//--------------------------------------------------------------------------
int KWndObjectBox::DropObject(bool bTestOnly)
{
	KUiDraggedObject	DragObj;
	Wnd_GetDragObj(&DragObj);

	if (m_uAcceptableGenre != CGOG_NOTHING)
	{
		if ((m_uAcceptableGenre != DragObj.uGenre) &&
			((m_uAcceptableGenre & 0xFFFF) != (DragObj.uGenre & 0xFFFF)) || ((m_uAcceptableGenre & 0xFFFF0000) != 0))
		return false;
	}
	if (bTestOnly)
		return true;
	
	ITEM_PICKDROP_PLACE	Pick, Drop;
	Drop.pWnd = this;
	Drop.h = Drop.v = 0;
	if (m_Object.uGenre == CGOG_NOTHING)
		m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP, NULL, (int)&Drop);
	else
	{
		Pick.pWnd = this;
		Pick.h = 0;
		Pick.v = 0;
		m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP, (unsigned int)&Pick, (int)&Drop);
	}		
	return true;
}

#define	NO_MATCHED_PUT_POS			-761209
#define	REPLACE_ITEM_POS(iItem)		(-(iItem) - 1)
#define	REPLACE_ITEM_INDEX(iPos)	((-iPos) - 1)

//--------------------------------------------------------------------------
//	功能：构造函数
//--------------------------------------------------------------------------
KWndObjectMatrix::KWndObjectMatrix()
{
	m_nNumUnitHori = 1;
	m_nNUmUnitVert = 1;
	m_nUnitWidth = 1;
	m_nUnitHeight = 1;
	m_nNumObjects = 0;
	m_pObjects = NULL;
	m_nMouseOverObj = -1;
	m_nPutPosX = NO_MATCHED_PUT_POS;
	m_nContainerId = 0;
	m_uBdNextTime = 0;
	m_fBdPercent = 0.0f;
	m_fBdAddPerFrame = 0.05f;
	m_pBClrPos = NULL;
	m_uBClrNextTime = 0;
}

void KWndObjectMatrix::SetContainerId(int nId)
{
	m_nContainerId = nId;
}

KWndObjectMatrix::~KWndObjectMatrix()
{
	Clear();
}

void KWndObjectMatrix::Clone(KWndObjectMatrix* pCopy)
{
	if (pCopy)
	{
		KWndWindow::Clone(pCopy);
		pCopy->m_nNumUnitHori = m_nNumUnitHori;
		pCopy->m_nNUmUnitVert = m_nNUmUnitVert;
		pCopy->m_nUnitWidth  = m_nUnitWidth;
		pCopy->m_nUnitHeight = m_nUnitHeight;
	}
}

// -------------------------------------------------------------------------
//	功能：初始化窗口
// -------------------------------------------------------------------------
int KWndObjectMatrix::Init(KIniFile* pIniFile, const char* pSection)
{
	if (KWndWindow::Init(pIniFile, pSection))
	{
		pIniFile->GetInteger(pSection, "HUnits", 1, &m_nNumUnitHori);
		pIniFile->GetInteger(pSection, "VUnits", 1, &m_nNUmUnitVert);
		if (m_nNumUnitHori < 1)
			m_nNumUnitHori = 1;
		if (m_nNUmUnitVert < 1)
			m_nNUmUnitVert = 1;
		m_nUnitWidth = m_Width / m_nNumUnitHori;
		if (m_nUnitWidth < 1)
			m_nUnitWidth = 1;
		m_nUnitHeight = m_Height / m_nNUmUnitVert;
		if (m_nUnitHeight < 1)
			m_nUnitHeight = 1;
		int nValue;
		pIniFile->GetInteger(pSection, "HaveBgColor", 1, &nValue);
		if (nValue)
			m_Style |= OBJCONT_S_HAVEOBJBGCOLOR;
		else
			m_Style &= ~OBJCONT_S_HAVEOBJBGCOLOR;
		pIniFile->GetInteger(pSection, "AcceptFree", 0, &nValue);
		if (nValue)
			m_Style |= OBJCONT_S_ACCEPT_FREE;
		else
			m_Style &= ~OBJCONT_S_ACCEPT_FREE;
		pIniFile->GetInteger(pSection, "UnitBorder", 0, &m_nUnitBorder);
		if (m_nUnitBorder >= m_nUnitWidth)
			m_nUnitBorder = m_nUnitWidth - 1;
		if (m_nUnitBorder >= m_nUnitHeight)
			m_nUnitBorder = m_nUnitHeight - 1;
		if (m_nUnitBorder < 0)
			m_nUnitBorder = 0;

		m_nPutPosX = NO_MATCHED_PUT_POS;
		return true;
	}
	return false;
}

// -------------------------------------------------------------------------
// 功能	: 窗体绘制
// -------------------------------------------------------------------------
void KWndObjectMatrix::PaintWindow()
{
	KWndWindow::PaintWindow();
	KRUShadow	Shadow;
	Shadow.bUseNewAlpha = FALSE;
	bool bMoveClr = false;
	if(m_uBClrNextTime < timeGetTime())
	{
		bMoveClr = true;
		m_uBClrNextTime = timeGetTime() + defBColorDelayTime;
	}
	for (int i = 0; i < m_nNumObjects; i++)
	{
		KUiDraggedObject* pObj = &m_pObjects[i];
		Shadow.Color.Color_dw = 0;
		if (i == REPLACE_ITEM_INDEX(m_nPutPosX))
			Shadow.Color.Color_dw = l_BgColors[4];
		else if ((m_Style & OBJCONT_F_MOUSE_HOVER) && m_nMouseOverObj == i)
			Shadow.Color.Color_dw = l_BgColors[3];
		else if (m_Style & OBJCONT_S_HAVEOBJBGCOLOR)
		{
			int nGenre = g_pCoreShell->GetGameData(GDI_GET_ITEM_INFO, 0, pObj->uId);
			if(nGenre == 0)
			{
				KUiObjAtContRegion	Obj;
				Obj.Obj.uGenre = pObj->uGenre;
				Obj.Obj.uId = pObj->uId;
				Obj.Region.h = Obj.Region.v = 0;
				Obj.Region.Width = Obj.Region.Height = 0;
				Obj.nContainer = m_nContainerId;
	
				ITEM_IN_ENVIRO_PROP eProp = (ITEM_IN_ENVIRO_PROP)g_pCoreShell->
					GetGameData(GDI_ITEM_IN_ENVIRO_PROP, (unsigned int)&Obj, 0);
				if (eProp == IIEP_NORMAL)
					Shadow.Color.Color_dw = l_BgColors[0];
				else if (eProp == IIEP_NOT_USEABLE)
					Shadow.Color.Color_dw = l_BgColors[1];
				else if (eProp == IIEP_SPECIAL)
					Shadow.Color.Color_dw = l_BgColors[2];
			}
		}

		int width = m_nUnitWidth * pObj->DataW - m_nUnitBorder * 2;
		int height = m_nUnitHeight * pObj->DataH - m_nUnitBorder * 2;
		Shadow.oPosition.nX = m_nAbsoluteLeft + m_nUnitWidth * pObj->DataX + m_nUnitBorder;
		Shadow.oPosition.nY = m_nAbsoluteTop + m_nUnitHeight * pObj->DataY + m_nUnitBorder;
		if (Shadow.Color.Color_dw)
		{
			Shadow.oEndPos.nX = Shadow.oPosition.nX + width;
			Shadow.oEndPos.nY = Shadow.oPosition.nY + height;
			g_pRepresentShell->DrawPrimitives(1, &Shadow, RU_T_SHADOW, true);
		}

		g_pCoreShell->DrawGameObj(pObj->uGenre, pObj->uId,
			Shadow.oPosition.nX, Shadow.oPosition.nY, width, height, 0);
		int nExType = g_pCoreShell->GetGameData(GDI_GET_ITEM_INFO, 1, pObj->uId);
		if(nExType > 0 && (m_nContainerId == UOC_ITEM_TAKE_WITH
		|| m_nContainerId == UOC_STORE_BOX))
		{
			//vi襫 n襫 n籱 di
			DrawBorder(nExType-1, m_nAbsoluteLeft + m_nUnitWidth * pObj->DataX,
			m_nAbsoluteTop + m_nUnitHeight * pObj->DataY,
			m_nUnitWidth * pObj->DataW - 1, m_nUnitHeight * pObj->DataH - 1,
			m_fBdPercent);
			//vi襫 s竛g b猲 tr猲
			DrawBColor(nExType-1, m_nAbsoluteLeft + m_nUnitWidth * pObj->DataX,
			m_nAbsoluteTop + m_nUnitHeight * pObj->DataY,
			m_nUnitWidth * pObj->DataW - 1, m_nUnitHeight * pObj->DataH - 1,
			m_pBClrPos[i].pos1, m_pBClrPos[i].pos2);
			//d辌h chuy觧 vi襫 tr猲
			if(bMoveClr)
			{
				int nRange = (m_nUnitWidth * pObj->DataW - 1)*2
					+ (m_nUnitHeight * pObj->DataH - 1)*2;
				++m_pBClrPos[i].pos1;
				++m_pBClrPos[i].pos2;
				if(m_pBClrPos[i].pos1 >= nRange)
					m_pBClrPos[i].pos1 = 0;
				if(m_pBClrPos[i].pos2 >= nRange)
					m_pBClrPos[i].pos2 = 0;
			}
		}
	}
	if (m_nPutPosX >= 0)
	{
		Shadow.oPosition.nX = m_nAbsoluteLeft + m_nUnitWidth * m_nPutPosX + m_nUnitBorder;
		Shadow.oPosition.nY = m_nAbsoluteTop + m_nUnitHeight * m_nPutPosY + m_nUnitBorder;
		Shadow.oEndPos.nX = Shadow.oPosition.nX + m_nUnitWidth * m_nPutWidth - m_nUnitBorder * 2;
		Shadow.oEndPos.nY = Shadow.oPosition.nY + m_nUnitHeight * m_nPutHeight - m_nUnitBorder * 2;
		Shadow.Color.Color_dw = l_BgColors[4];
		g_pRepresentShell->DrawPrimitives(1, &Shadow, RU_T_SHADOW, true);
	}
	if(m_uBdNextTime < timeGetTime())
	{
		m_uBdNextTime = timeGetTime() + defBorderDelayTime;
		m_fBdPercent += m_fBdAddPerFrame;
		if(m_fBdPercent > 0.6f)
		{
			m_fBdAddPerFrame = -0.05f;
			m_fBdPercent = 0.6f + m_fBdAddPerFrame;
		}
		else if(m_fBdPercent < 0.0f)
		{
			m_fBdAddPerFrame = 0.05f;
			m_fBdPercent = m_fBdAddPerFrame;
		}
	}
}

// -------------------------------------------------------------------------
// 功能	: 增加一个对象物品
// -------------------------------------------------------------------------
int KWndObjectMatrix::AddObject(KUiDraggedObject* pObject, int nCount)
{
	if (pObject && nCount > 0)
	{
		int	i, nValidCount = 0;
		for (i = 0; i < nCount; i++)
		{
			if (pObject[i].uGenre)
				nValidCount++;
		}
		if (nValidCount)
		{
			KUiDraggedObject* pNewList = (KUiDraggedObject*)realloc(m_pObjects, sizeof(KUiDraggedObject) * (m_nNumObjects + nValidCount));
			SBColorPos* pNewBclr = (SBColorPos*)realloc(m_pBClrPos, sizeof(SBColorPos) * (m_nNumObjects + nValidCount));
			
			if (pNewList)
			{
				m_pObjects = pNewList;
				m_pBClrPos = pNewBclr;
				for (i = 0; i < nCount; i++)
				{
					if (pObject[i].uGenre)
					{
						m_pObjects[m_nNumObjects] = pObject[i];
						int nRange = (m_nUnitWidth * m_pObjects[m_nNumObjects].DataW - 1)*2
							+ (m_nUnitHeight * m_pObjects[m_nNumObjects].DataH - 1)*2;
						m_pBClrPos[m_nNumObjects].pos1 = g_Random(nRange);
						m_pBClrPos[m_nNumObjects].pos2 = g_Random(nRange);
						if(abs(m_pBClrPos[m_nNumObjects].pos2 - m_pBClrPos[m_nNumObjects].pos1) < 32)
						{
							m_pBClrPos[m_nNumObjects].pos2 = m_pBClrPos[m_nNumObjects].pos1 + 32;
							if(m_pBClrPos[m_nNumObjects].pos2 >= nRange)
								m_pBClrPos[m_nNumObjects].pos2 -= nRange;
						}
						m_nNumObjects ++;
					}
				}
			}
			m_nPutPosX = NO_MATCHED_PUT_POS;
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------------------
// 功能	: 减少一个对象物品
// -------------------------------------------------------------------------
int KWndObjectMatrix::RemoveObject(KUiDraggedObject* pObject)
{
	if (pObject)
	{
		m_nPutPosX = NO_MATCHED_PUT_POS;
		m_nMouseOverObj = -1;
		for (int i = 0; i < m_nNumObjects; i++)
		{
			KUiDraggedObject* pHolded = &m_pObjects[i];
			if (pHolded->DataX == pObject->DataX &&
				pHolded->DataY == pObject->DataY)
			{
				if (g_MouseOver.IsMoseHoverWndObj(this, i))
					g_MouseOver.CancelMouseHoverInfo();
				m_nNumObjects --;
				for (; i < m_nNumObjects; i++)
				{
					m_pObjects[i] = m_pObjects[i + 1];
					m_pBClrPos[i] = m_pBClrPos[i + 1];
				}
				return true;
			}
		}
	}
	return false;
}

// -------------------------------------------------------------------------
// 功能	: 获取容纳的某个对象信息
//	返回：对象的数目
// -------------------------------------------------------------------------
int	KWndObjectMatrix::GetObject(KUiDraggedObject& Obj, int x, int y) const
{
	for (int i = 0; i < m_nNumObjects; i++)
	{
		KUiDraggedObject*	pHolded = &m_pObjects[i];
		if (x == pHolded->DataX && y == pHolded->DataY)
		{
			Obj = *pHolded;
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------------------
// 功能	: 获取容纳的对象信息
//	返回：对象的数目
// -------------------------------------------------------------------------
/*int KWndObjectMatrix::GetObjects(KUiGameObject* pObjects, int nCount) const
{
	if (m_nNumObjects <= nCount)
	{
		for (int i = 0; i < m_nNumObjects; i++)
		{
			pObjects[i].uGenre = m_pObjects[i].uGenre;
			pObjects[i].uId = m_pObjects[i].uId;
		}
		return m_nNumObjects;
	}
	return m_nNumObjects;
}*/

//--------------------------------------------------------------------------
//	功能：清除全部的对象物品
//--------------------------------------------------------------------------
void KWndObjectMatrix::Clear()
{
	m_nNumObjects = 0;
	m_nMouseOverObj = -1;
	if (m_pObjects)
	{
		free(m_pObjects);
		m_pObjects = NULL;
	}
	if(m_pBClrPos)
	{
		free(m_pBClrPos);
		m_pBClrPos = NULL;
	}
}

//--------------------------------------------------------------------------
//	功能：窗口函数
//--------------------------------------------------------------------------
int KWndObjectMatrix::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		if ((m_Style & OBJCONT_S_DISABLE_PICKPUT)== 0)
		{
			if (Wnd_GetDragObj(NULL))
				DropObject(LOWORD(nParam), HIWORD(nParam), false);
			else
				PickUpObjectAt(LOWORD(nParam), HIWORD(nParam));
		}
		else if (m_pParentWnd)
		{
			m_nPutPosX = NO_MATCHED_PUT_POS;
			int nObj = GetObjectAt(LOWORD(nParam), HIWORD(nParam));
			if (nObj >= 0)
			{
				KUiDraggedObject	Obj;
				Obj = m_pObjects[nObj];
				m_pParentWnd->WndProc(WND_N_LEFT_CLICK_ITEM,
					(unsigned int)&Obj, (int)(KWndWindow*)this);
			}
		}
		break;
	case WM_RBUTTONDOWN:
		m_nPutPosX = NO_MATCHED_PUT_POS;
		if (m_pParentWnd)
		{
			int nObj = GetObjectAt(LOWORD(nParam), HIWORD(nParam));
			if (nObj >= 0)
			{
				KUiDraggedObject	Obj;
				Obj = m_pObjects[nObj];
				m_pParentWnd->WndProc(WND_N_RIGHT_CLICK_ITEM,
					(unsigned int)&Obj, (int)(KWndWindow*)this);
			}
		}
		break;
	case WM_MOUSEHOVER:
	case WM_MOUSEMOVE:
		m_Style |= OBJCONT_F_MOUSE_HOVER;
		{
			int	x = LOWORD(nParam);
			int y = HIWORD(nParam);
			int nObj = GetObjectAt(x, y);
			m_nMouseOverObj = nObj;
			if (nObj >= 0)
			{
				if (g_MouseOver.IsMoseHoverWndObj(this, nObj) == 0)
				{
					SetMouseHoverObjectDesc(this, nObj, m_pObjects[nObj].uGenre,
						m_pObjects[nObj].uId, m_nContainerId, x, y);
				}
			}
			else
				g_MouseOver.CancelMouseHoverInfo();
			if ((m_Style & OBJCONT_S_TRACE_PUT_POS) && Wnd_GetDragObj(NULL))
			{
				DropObject(LOWORD(nParam), HIWORD(nParam), true);
			}
		}
		break;
	case WND_M_MOUSE_LEAVE:
		m_nPutPosX = NO_MATCHED_PUT_POS;
		m_Style &= ~OBJCONT_F_MOUSE_HOVER;
		KWndWindow::WndProc(uMsg, uParam, nParam);
		break;
	default:
		return KWndWindow::WndProc(uMsg, uParam, nParam);
	}
	return 0;
}

//--------------------------------------------------------------------------
//	功能：获得某个位置上的物品
//--------------------------------------------------------------------------
int KWndObjectMatrix::GetObjectAt(int x, int y)
{
	x = (x - m_nAbsoluteLeft) / m_nUnitWidth;
	y = (y - m_nAbsoluteTop) / m_nUnitHeight;
	for (int i = 0; i < m_nNumObjects; i++)
	{
		KUiDraggedObject*	pHolded = &m_pObjects[i];
		if (x < pHolded->DataX || y < pHolded->DataY ||
			x >= pHolded->DataX + pHolded->DataW ||
			y >= pHolded->DataY + pHolded->DataH)
			continue;
		return i;
	}
	return -1;
}

//--------------------------------------------------------------------------
//	功能：捡起某个位置上的对象
//--------------------------------------------------------------------------
int KWndObjectMatrix::PickUpObjectAt(int x, int y)
{
	int nPicked = GetObjectAt(x, y);
	if (nPicked >= 0)
	{
		ITEM_PICKDROP_PLACE	Pick;
		Pick.pWnd = this;
		Pick.h = m_pObjects[nPicked].DataX;
		Pick.v = m_pObjects[nPicked].DataY;
		m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP, (unsigned int)&Pick, NULL);
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
//	功能：放置物品
//--------------------------------------------------------------------------
int KWndObjectMatrix::DropObject(int x, int y, bool bTestOnly)
{
	KUiDraggedObject	DragObj;

	m_nPutPosX = NO_MATCHED_PUT_POS;
	if (m_Style & OBJCONT_S_ACCEPT_FREE)
	{
		if (bTestOnly == false)
			DropObject(x, y, (KUiDraggedObject*)NULL);
		return true;
	}

	Wnd_GetDragObj(&DragObj);
	if (DragObj.DataW > m_nNumUnitHori || DragObj.DataH > m_nNUmUnitVert)
		return false;

	//转换成格子坐标
	RECT	or;
	KUiDraggedObject* pOverlaped = NULL;

	x = (x - m_nAbsoluteLeft) / m_nUnitWidth;
	y = (y - m_nAbsoluteTop) / m_nUnitHeight;
	if ((or.right = x + (DragObj.DataW + 1) / 2) > m_nNumUnitHori)
		or.right = m_nNumUnitHori;
	if ((or.bottom = y + (DragObj.DataH + 1) / 2) > m_nNUmUnitVert)
		or.bottom = m_nNUmUnitVert;
	if (or.right >= DragObj.DataW)
		or.left = or.right - DragObj.DataW;
	else
	{
		or.left = 0;
		or.right = DragObj.DataW;
	}
	if (or.bottom >= DragObj.DataH)
		or.top = or.bottom - DragObj.DataH;
	else
	{
		or.top = 0;
		or.bottom = DragObj.DataH;
	}

	if (TryDropObjAtPos(or, pOverlaped))
	{
		if (bTestOnly == false)
			DropObject(or.left, or.top, pOverlaped);
		else if (pOverlaped)
		{
			m_nPutPosX = REPLACE_ITEM_POS(pOverlaped - m_pObjects);
		}
		else
		{
			m_nPutPosX = or.left;
			m_nPutPosY = or.top;
			m_nPutWidth = DragObj.DataW;
			m_nPutHeight = DragObj.DataH;
		}
		return true;
	}

	if (DragObj.DataW == 1 && DragObj.DataH == 1)
		return false;

	RECT	Try;
	Try.right = x;
	Try.bottom = y;
	if ((Try.left = x - DragObj.DataW + 1) < 0)
		Try.left = 0;
	if ((Try.top = y - DragObj.DataH + 1) < 0)
		Try.top = 0;

	for (or.left = Try.left; or.left <= Try.right; or.left ++)
	{
		or.right = or.left + DragObj.DataW;
		for (or.top = Try.top; or.top <= Try.bottom; or.top ++)
		{
			or.bottom = or.top + DragObj.DataH;
			if (TryDropObjAtPos(or, pOverlaped))
			{
				if (bTestOnly == false)
					DropObject(or.left, or.top, pOverlaped);
				return true;
			}
		}
	}
	return false;
}

//--------------------------------------------------------------------------
//	功能：尝试放置物品
//--------------------------------------------------------------------------
int KWndObjectMatrix::TryDropObjAtPos(const RECT& dor, KUiDraggedObject*& pOverlaped)
{
	pOverlaped = NULL;
	int i;
	for (i = 0; i < m_nNumObjects; i++)
	{
		KUiDraggedObject*	pHolded = &m_pObjects[i];
		if (pHolded->DataX >= dor.right || pHolded->DataY >= dor.bottom ||
			pHolded->DataX + pHolded->DataW <= dor.left ||
			pHolded->DataY + pHolded->DataH <= dor.top)
			continue;
		if (pOverlaped)
			break;
		pOverlaped = pHolded;
	}
	return (i == m_nNumObjects);
}

//--------------------------------------------------------------------------
//	功能：放下物品
//--------------------------------------------------------------------------
void KWndObjectMatrix::DropObject(int x, int y, KUiDraggedObject* pToPickUpObj)
{
	ITEM_PICKDROP_PLACE	Drop;
	Drop.pWnd = this;
	Drop.h = x;
	Drop.v = y;

	if (pToPickUpObj)
	{
		ITEM_PICKDROP_PLACE	Pick;
		Pick.pWnd = this;
		Pick.h = pToPickUpObj->DataX;
		Pick.v = pToPickUpObj->DataY;
		m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP, (unsigned int)&Pick, (int)&Drop);
	}
	else
		m_pParentWnd->WndProc(WND_N_ITEM_PICKDROP, NULL, (int)&Drop);
}

void KWndObjectMatrix::EnableTracePutPos(bool bEnable)
{
	if (bEnable)
		m_Style |= OBJCONT_S_TRACE_PUT_POS;
	else
		m_Style &= ~OBJCONT_S_TRACE_PUT_POS;
}

void KWndObjectMatrix::EnablePickPut(bool bEnable)
{
	if (bEnable == false)
		m_Style |= OBJCONT_S_DISABLE_PICKPUT;
	else
		m_Style &= ~OBJCONT_S_DISABLE_PICKPUT;
}
