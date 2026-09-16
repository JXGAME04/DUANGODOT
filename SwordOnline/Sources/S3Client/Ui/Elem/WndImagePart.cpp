/*****************************************************************************************
//	界面窗口体系结构--显示图形局部的窗口
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-7-25
*****************************************************************************************/
#include "KWin32.h"
#include "KIniFile.h"
#include "WndImagePart.h"
#include "KSprite.h"
#include "WndImage.h" //guve

#include "../../../Represent/iRepresent/iRepresentShell.h"
extern iRepresentShell*	g_pRepresentShell;

//--------------------------------------------------------------------------
//	功能：构造函数
//--------------------------------------------------------------------------
KWndImagePart::KWndImagePart()
{
	m_bPartButton = false; //guve
	IR_InitUiImagePartRef(m_Image);
}

void KWndImagePart::Clone(KWndImagePart* pCopy)
{
	if (pCopy)
	{
		KWndWindow::Clone(pCopy);
		pCopy->m_Image	= m_Image;
		pCopy->SetPart(0, 100);
	}
}

//--------------------------------------------------------------------------
//	功能：初始化窗口
//--------------------------------------------------------------------------
int KWndImagePart::Init(KIniFile* pIniFile, const char* pSection)
{
	if (KWndWindow::Init(pIniFile, pSection))
	{
		pIniFile->GetInteger(pSection, "PartType", 0, &m_Image.nDivideFashion);
		if (m_Image.nDivideFashion < IDF_LEFT_TO_RIGHT || m_Image.nDivideFashion > IDF_BOTTOM_TO_TOP)
			m_Image.nDivideFashion = IDF_LEFT_TO_RIGHT;
		m_Image.uImage = 0;
		m_Image.nISPosition = 0;
		int nValue;
		pIniFile->GetInteger(pSection, "ImgType", 0, &nValue);
		if (nValue == 1)
		{
			m_Image.nType = ISI_T_BITMAP16;
			m_Image.bRenderStyle = IMAGE_RENDER_STYLE_OPACITY;
		}
		else
		{
			m_Image.nType = ISI_T_SPR;
			m_Image.bRenderStyle = IMAGE_RENDER_STYLE_ALPHA;
		}
		pIniFile->GetString(pSection, "Image", "" , m_Image.szImage, sizeof(m_Image.szImage));
		//guve
		nValue = 0;
		int nVal2 = 0;
		pIniFile->GetInteger2(pSection, "Horizon", &nValue, &nVal2);
		if(nValue > 0 || nVal2 > 0)
		{
			m_bPartButton = true;
			SetPartHorizon(nValue, nVal2);
		}
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
//	功能：窗体绘制
//--------------------------------------------------------------------------
void KWndImagePart::PaintWindow()
{
	KWndWindow::PaintWindow();
	if (g_pRepresentShell)
	{
		#ifdef _DEBUG
		if (m_Image.szImage[0] == 0 && WND_SHOW_DEBUG_FRAME_TEXT)
		{
			g_pRepresentShell->OutputText(16, m_Caption, -1, m_nAbsoluteLeft + 1, m_nAbsoluteTop + 1, 0xFF000000);
			g_pRepresentShell->OutputText(16, m_Caption, -1, m_nAbsoluteLeft, m_nAbsoluteTop, 0xFFFFFFFF);
		}
		else
		#endif
		{
			if(m_bPartButton && m_pParentWnd)
			{
				m_Image.nFrame = ((KWndImage*)m_pParentWnd)->GetCurFrame();
			}
			m_Image.oPosition.nX = m_nAbsoluteLeft;
			m_Image.oPosition.nY = m_nAbsoluteTop;
			g_pRepresentShell->DrawPrimitives(1, &m_Image, RU_T_IMAGE_PART, true);
		}
	}
}

//--------------------------------------------------------------------------
//	功能：根据部分值与全值的比例关系设置绘制图的哪部分区域
//--------------------------------------------------------------------------
void KWndImagePart::SetPart(int nPartValue, int nFullValue)
{
	if (nFullValue)
		IR_UpdateImagePart(m_Image, nPartValue, nFullValue);
}
//guve
void  KWndImagePart::SetPartHorizon(int nLeft, int nRight)
{
	KImageParam	Param;
	Param.nWidth = 0;
	if (g_pRepresentShell)
			g_pRepresentShell->GetImageParam(m_Image.szImage, &Param, m_Image.nType);
	m_Image.Width = Param.nWidth;
	m_Image.Height = Param.nHeight;
	m_Image.oImgLTPos.nX = nLeft;
	m_Image.oImgLTPos.nY = 0;
	m_Image.oImgRBPos.nX = nRight;
	m_Image.oImgRBPos.nY = m_Image.Height;
	if (m_Image.Width == 0 || m_Image.Height == 0)
		m_Image.szImage[0] = 0;			
}
