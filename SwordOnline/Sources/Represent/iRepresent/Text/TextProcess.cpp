
/*****************************************************************************************
//	ÎÄ±¾´®´¦Àí£º»ñÈ¡ÎÄ±¾´®ÖÐµ¥Ò»ÐÔÖÊµÄÍ¬Ò»ÐÐµÄ×Ó´®£¬ÒÔ¼°ÎÄ±¾´®×ÜµÄº¬ÊýÄ¿¡£
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-8-29
*****************************************************************************************/
#include <crtdbg.h>
#include "TextProcess.h"
#include "../../../Engine/src/Text.h"
#include "../Font/iFont.h"

#ifndef SUCCEEDED
#define SUCCEEDED(Status) ((long)(Status) >= 0)
#endif

extern IInlinePicEngineSink* g_pIInlinePicSinkRP;	//Ç¶ÈëÊ½Í¼Æ¬µÄ´¦Àí½Ó¿Ú[wxb 2003-6-20]

union	TP_COLOR
{
	struct {unsigned char b, g, r, a; } Color_b;
	unsigned int	Color_dw;
};

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¹¹Ôìº¯Êý
//--------------------------------------------------------------------------
KTextProcess::KTextProcess()
{
	m_pBuffer  = NULL;
	m_nCount   = 0;
	m_nMaxLineLen = 0;
	m_nReadPos = 0;
	m_nCurrentLineLen = 0;
	m_fCurrentLineLen = 0;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º¹¹Ôìº¯Êý
//--------------------------------------------------------------------------
KTextProcess::KTextProcess(const char* pBuffer, int nCount, int nLineLen)
{
	SetContent(pBuffer, nCount, nLineLen);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÉèÖÃÒª´¦ÀíµÄÎÄ±¾´®ÄÚÈÝ
//--------------------------------------------------------------------------
void KTextProcess::SetContent(const char* pBuffer, int nCount, int nLineLen)
{
	if (pBuffer && nCount > 0)
	{
		m_pBuffer  = pBuffer;
		m_nCount   = nCount;
		m_nMaxLineLen = nLineLen;
	}
	else
	{
		m_pBuffer  = NULL;
		m_nCount   = 0;
		m_nMaxLineLen = 0;
	}
	m_nCurrentLineLen = 0;
	m_fCurrentLineLen = 0;
	m_nReadPos = 0;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£º»ñÈ¡×Ö·û´®Àïµ¥Ò»ÐÔÖÊÍ¬ÐÐµÄÒ»¸ö×Ó´®
//	²ÎÊý£ºint& nSimplexStartPos --> ×Ó´®Í·Ò»¸öÏÔÊ¾×Ö·ûÔÚ×Ö·û´®ÖÐµÄÎ»ÖÃ
//		¡¡KTP_CTRL& HeadCtrl    --> ×Ó´®¿ªÊ¼Ç°µÄ¿ØÖÆ
//		¡¡KTP_CTRL& TailCtrl    --> ×Ó´®½áÊøºóµÄ¿ØÖÆ
//	×¢ÊÍ£º´Ëº¯Êý±àÂëÓëGBK¹æ·¶Ïà¹Ø
//		¶à´Îµ÷ÓÃGetSimplexText½«Öð¸ö»ñÈ¡×Ö·û´®ÖÐÍ¬ÐÐµÄµ¥Ò»ÐÔÖÊµÄ×Ó´®£¬ÔÚ×Ö·û´®
//	±»±éÀ÷Íê±ÏÖ®ºó½«ÔÚÉèÖÃµ÷ÓÃ²ÎÊýTailCtrlÖÐµÄÐÐÄ©¿ØÖÆÂëÎªÎÄ±¾´®½áÊø¿ØÖÆÂë¡£ÏÂ
//	´ÎÔÙµ÷ÓÃÓÖ½«ÖØÐÂ´ÓÎÄ±¾´®ÆðÊ¼´¦»ñÈ¡×Ó´®¡£
//--------------------------------------------------------------------------
//Check vµ tr¶ vÒ ngay 1 Ctrl kÕ tiÕp, cïng víi sè l­îng ch÷
int KTextProcess::GetSimplexText(int &nSimplexStartPos, KTP_CTRL& HeadCtrl, KTP_CTRL& TailCtrl, int nFontSize)
{
	int nBeforeLen = 0,
		nVisibleChars = 0;	//´Ë´Î»ñÈ¡µ½µÄÏÔÊ¾×Ö·ûµÄ¸öÊý
	nSimplexStartPos = m_nReadPos;
	HeadCtrl.cCtrl = KTC_INVALID;
	TailCtrl.cCtrl = KTC_INVALID;

	unsigned char cCode;
	int bVNFont = TIsTCVNText();
	if (m_nReadPos >= m_nCount)
		goto KS_EXIT;
	nBeforeLen = m_nCurrentLineLen;
	while(m_nReadPos < m_nCount)
	{
		cCode = m_pBuffer[m_nReadPos];
		//ÅÐ¶ÏÍ·Ò»¸ö×Ö·ûÊÇ·ñÊÇ¿ØÖÆ·ûºÅ
		if (!bVNFont && cCode > 0x80)	//¿ÉÄÜÊÇÖÐÎÄ×Ö·û
		{	//×Ö·û´®ÏÈ±»´¦Àí¹ý£¬²»»á³öÏÖµ¥¸öµ¥×Ö½ÚÊýÖµ´óÓÚ0x80µÄ×Ö·û
			nVisibleChars += 2;
			m_fCurrentLineLen += 2;
			m_nCurrentLineLen = (int)(m_fCurrentLineLen + 0.9999);
			if (m_nMaxLineLen == 0 || m_nCurrentLineLen < m_nMaxLineLen)
			{
				m_nReadPos += 2;
			}
			else if(m_nCurrentLineLen == m_nMaxLineLen || m_nCurrentLineLen == 0)
			{
				m_nReadPos += 2;
				TailCtrl.cCtrl = KTC_ENTER;
				break;
			}
			else
			{
				nVisibleChars -= 2;
				m_nCurrentLineLen -= 2;
				m_fCurrentLineLen -= 2;
				TailCtrl.cCtrl = KTC_ENTER;
				break;
			}
		}
		else if (cCode == KTC_INLINE_PIC)	//[wxb 2003-6-20]
		{
			TailCtrl.cCtrl	= KTC_INLINE_PIC;
			TailCtrl.wParam	= *((unsigned short*)(m_pBuffer + m_nReadPos + 1));
			if (g_pIInlinePicSinkRP)
			{
				int cx, cy;
				if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(TailCtrl.wParam, cx, cy)))
				{
					m_fCurrentLineLen += (float)cx * 2 / nFontSize;
					m_nCurrentLineLen = (int)(m_fCurrentLineLen + 0.9999);
					if (m_nMaxLineLen == 0 || m_nCurrentLineLen < m_nMaxLineLen)
					{
						m_nReadPos += 1 + sizeof(unsigned short);
					}
					else if (m_nCurrentLineLen == m_nMaxLineLen || m_nCurrentLineLen == 0)
					{
						m_nReadPos += 1 + sizeof(unsigned short);
						break;
					}
					else
					{
						m_fCurrentLineLen -= (float)cx * 2 / nFontSize;
						m_nCurrentLineLen = (int)(m_fCurrentLineLen + 0.9999);
						TailCtrl.cCtrl = KTC_ENTER;
						break;
					}
				}
				else
				{
					m_nReadPos += 1 + sizeof(unsigned short);
				}
			}
			else
				m_nReadPos += 1 + sizeof(unsigned short);
			break;
		}
		else if (cCode == KTC_COLOR || cCode == KTC_BORDER_COLOR)//ÑÕÉ«¿ØÖÆ
		{
			if (m_nCurrentLineLen > nBeforeLen || HeadCtrl.cCtrl != KTC_INVALID)
			{
				(*(int*)(&TailCtrl)) = *(int*)(m_pBuffer + m_nReadPos);
				m_nReadPos += 4;
				break;
			}
			else
			{
				(*(int*)(&HeadCtrl)) = *(int*)(m_pBuffer + m_nReadPos);
				m_nReadPos += 4;
				nSimplexStartPos = m_nReadPos;
			}
		}
		else if (cCode == KTC_COLOR_RESTORE || cCode == KTC_BORDER_RESTORE)
		{
			m_nReadPos += 1;
			if (m_nCurrentLineLen > nBeforeLen || HeadCtrl.cCtrl != KTC_INVALID)
			{
				TailCtrl.cCtrl = cCode;
				break;
			}
			else
			{
				HeadCtrl.cCtrl = cCode;
				nSimplexStartPos = m_nReadPos;
			}
		}
		else if (cCode == KTC_ENTER)
		{
			TailCtrl.cCtrl = KTC_ENTER;
			m_nReadPos ++;
			break;
		}
		else
		{
			m_nCurrentLineLen ++;
			m_fCurrentLineLen += 1;
			nVisibleChars ++;
			m_nReadPos ++;
		}
		if (m_nMaxLineLen)
		{
			if (m_nCurrentLineLen >= m_nMaxLineLen)
			{
				TailCtrl.cCtrl = KTC_ENTER;
				break;
			}
			else if (!bVNFont && m_nCurrentLineLen && m_nCurrentLineLen + 3 >= m_nMaxLineLen)
			{
				const char* pNext = TGetSecondVisibleCharacterThisLine((const char*)m_pBuffer, m_nReadPos, m_nCount);
				if (pNext && TIsCharacterNotAlowAtLineHead(pNext))
				{
					TailCtrl.cCtrl = KTC_ENTER;
					break;
				}
			}
		}
	}

	nBeforeLen = m_nCurrentLineLen - nBeforeLen;

KS_EXIT:
	if (m_nReadPos >= m_nCount)
	{
		if (KTC_INLINE_PIC == TailCtrl.cCtrl)
			return nVisibleChars;
		TailCtrl.cCtrl = KTC_TAIL;
		m_nReadPos = 0;
		m_nCurrentLineLen = 0;
		m_fCurrentLineLen = 0;
	}
	else if (TailCtrl.cCtrl == KTC_ENTER)
	{
		m_nCurrentLineLen = 0;
		m_fCurrentLineLen = 0;
	}
	
	return (nVisibleChars);
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÌøµ½Ö¸¶¨ÐÐ¿ªÊ¼´¦
//	²ÎÊý£ºint nLineIndex --> Ö¸¶¨ÐÐµÄÐÐºÅ£¨´Ó0¿ªÊ¼£©
//		¡¡KTP_CTRL& Ctrl --> Ö¸¶¨ÐÐ¿ªÊ¼Ç°µÄ¿ØÖÆ
//	·µ»Ø£ºÈç¹ûÎÄ±¾´®ÖÐ´æÔÚ£¬Ôò·µ»ØÎÄ±¾´®ÖÐÖ¸¶¨ÐÐ¿ªÊ¼´¦µÄÎ»ÖÃ¡£
//		¡¡Èç¹ûÖ¸¶¨ÐÐ²»´æÔÚ£¬Ôò·µ»Ø-1£¬²¢°ÑÎÄ±¾´®µÄ¶ÁÈ¡Ö¸ÕëÉèÖÃÔÚ±¾×Ö·û´®¿ªÊ¼´¦¡£
//--------------------------------------------------------------------------
//§i ®Õn dßng cÇn vÏ
int KTextProcess::SeekToSpecialLine(int nLineIndex, KTP_CTRL& Ctrl1, KTP_CTRL&Ctrl2,
									int& nSkipedHalfLines, int nFontSize, int bPicPackInSingleLine)
{
	m_nReadPos = 0;
	int nSimplexTextPos;
	KTP_CTRL HeadCtrl, TailCtrl;

	TP_COLOR	PreFontColor, PreBorderColor;
	PreFontColor.Color_b.a = 0;
	PreBorderColor.Color_b.a = 0;
	TailCtrl.cCtrl = Ctrl1.cCtrl = KTC_INVALID;
	Ctrl2.cCtrl = KTC_INVALID;
	int nLinesSpan = 0, nLineHeight = 0;
	unsigned char cPrevTailCtrl = 0xFF;

	nSkipedHalfLines = 0;
	while (nLineIndex > 0)
	{
		cPrevTailCtrl = TailCtrl.cCtrl;
		if (0 == nLinesSpan || KTC_ENTER == TailCtrl.cCtrl)
		{
			GetCurLineHeight(nLinesSpan, nLineHeight, nFontSize, bPicPackInSingleLine);
			Backup();
		}
		GetSimplexText(nSimplexTextPos, HeadCtrl, TailCtrl, nFontSize);
		switch(HeadCtrl.cCtrl)
		{
		case KTC_COLOR:
			if (Ctrl1.cCtrl == KTC_COLOR)
			{
				PreFontColor.Color_b.r = Ctrl1.cParam0;
				PreFontColor.Color_b.g = Ctrl1.cParam1;
				PreFontColor.Color_b.b = Ctrl1.cParam2;
				PreFontColor.Color_b.a = 0xFF;
			}
			Ctrl1 = HeadCtrl;
			break;
		case KTC_COLOR_RESTORE:
			if (PreFontColor.Color_b.a)
			{
				Ctrl1.cCtrl = KTC_COLOR;
				Ctrl1.cParam0 = PreFontColor.Color_b.r;
				Ctrl1.cParam1 = PreFontColor.Color_b.g;
				Ctrl1.cParam2 = PreFontColor.Color_b.b;
			}
			else
				Ctrl1.cCtrl = KTC_INVALID;
			break;
		case KTC_BORDER_COLOR:
			if (Ctrl2.cCtrl == KTC_BORDER_COLOR)
			{
				PreBorderColor.Color_b.r = Ctrl2.cParam0;
				PreBorderColor.Color_b.g = Ctrl2.cParam1;
				PreBorderColor.Color_b.b = Ctrl2.cParam2;
				PreBorderColor.Color_b.a = 0xFF;
			}
			Ctrl2 = HeadCtrl;
			break;
		case KTC_BORDER_RESTORE:
			if (PreBorderColor.Color_b.a)
			{
				Ctrl2.cCtrl = KTC_BORDER_COLOR;
				Ctrl2.cParam0 = PreBorderColor.Color_b.r;
				Ctrl2.cParam1 = PreBorderColor.Color_b.g;
				Ctrl2.cParam2 = PreBorderColor.Color_b.b;
			}
			else
				Ctrl2.cCtrl = KTC_INVALID;
			break;
		}

		switch (TailCtrl.cCtrl)
		{
		case KTC_COLOR:
			if (Ctrl1.cCtrl == KTC_COLOR)
			{
				PreFontColor.Color_b.r = Ctrl1.cParam0;
				PreFontColor.Color_b.g = Ctrl1.cParam1;
				PreFontColor.Color_b.b = Ctrl1.cParam2;
				PreFontColor.Color_b.a = 0xFF;
			}
			Ctrl1 = TailCtrl;
			break;
		case KTC_COLOR_RESTORE:
			if (PreFontColor.Color_b.a)
			{
				Ctrl1.cCtrl = KTC_COLOR;
				Ctrl1.cParam0 = PreFontColor.Color_b.r;
				Ctrl1.cParam1 = PreFontColor.Color_b.g;
				Ctrl1.cParam2 = PreFontColor.Color_b.b;
			}
			else
				Ctrl1.cCtrl = KTC_INVALID;
			break;
		case KTC_BORDER_COLOR:
			if (Ctrl2.cCtrl == KTC_BORDER_COLOR)
			{
				PreBorderColor.Color_b.r = Ctrl2.cParam0;
				PreBorderColor.Color_b.g = Ctrl2.cParam1;
				PreBorderColor.Color_b.b = Ctrl2.cParam2;
				PreBorderColor.Color_b.a = 0xFF;
			}
			Ctrl2 = TailCtrl;
			break;
		case KTC_BORDER_RESTORE:
			if (PreBorderColor.Color_b.a)
			{
				Ctrl2.cCtrl = KTC_BORDER_COLOR;
				Ctrl2.cParam0 = PreBorderColor.Color_b.r;
				Ctrl2.cParam1 = PreBorderColor.Color_b.g;
				Ctrl2.cParam2 = PreBorderColor.Color_b.b;
			}
			else
				Ctrl2.cCtrl = KTC_INVALID;
			break;
		case KTC_ENTER:
			nLineIndex -= nLinesSpan;
			if (nLineIndex < 0)
			{
				Restore();
				nSkipedHalfLines = -nLineIndex;
			}
			break;
		case KTC_INLINE_PIC:
			Ctrl2.cCtrl = KTC_INLINE_PIC;
			Ctrl2.wParam = TailCtrl.wParam;
			break;
		case KTC_TAIL:
			Ctrl1.cCtrl = KTC_INVALID;
			Ctrl2.cCtrl = KTC_INVALID;
			if (KTC_INLINE_PIC == cPrevTailCtrl)
			{
				nLineIndex -= nLinesSpan;
				if (nLineIndex < 0)
				{
					Restore();
					nSkipedHalfLines = -nLineIndex;
				}
				break;
			}
			m_nReadPos = 0;
			return -1;
			break;
		}
	}
	return m_nReadPos;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÔÚÖ¸¶¨µÄÎ»ÖÃ»æÖÆ×Ö·û´®ÖÐÖ¸¶¨µÄÄÚÈÝ
//	·µ»Ø£ºÊµ¼Ê»æÖÆµÄÎÄ×ÖÐÐÊý
//--------------------------------------------------------------------------
int KTextProcess::DrawTextLine(iFont* pFont, int nFontSize, KOutputTextParam* pParam)
{
	_ASSERT(pParam);
	if (pFont == NULL || pParam->nNumLine <= 0)
		return 0;

	KTP_CTRL	PreCtrl1, PreCtrl2, TailCtrl;
	int			nCount = 0, nPos = 0;
	int			x = pParam->nX, y = pParam->nY, nLineDrawed = 0;

	int			nHalfIndex = 0, nFontHalfWidth[2];
	int			nSkipedHalfLines = 0;	//ÒòÎªÇ¶ÈëÊ½Í¼Æ¬Ôì³ÉµÄ¿ç°ëÐÐÏÖÏó
	nFontHalfWidth[0] = nFontSize / 2;
	nFontHalfWidth[1] = (nFontSize + 1) / 2;

	TP_COLOR	PreFontColor, CurFontColor;
	TP_COLOR	PreBorderColor, CurBorderColor;
	unsigned char nAlpha = (pParam->Color & 0xff000000) >> 24;
	PreFontColor.Color_dw = CurFontColor.Color_dw = pParam->Color;
	PreBorderColor.Color_dw = CurBorderColor.Color_dw = pParam->BorderColor;

#define	_SET_CURRENT_COLOR(c)				\
	{	PreFontColor = CurFontColor;				\
		CurFontColor.Color_b.r = c.cParam0;	\
		CurFontColor.Color_b.g = c.cParam1;	\
		CurFontColor.Color_b.b = c.cParam2;	}
#define	_SET_CURRENT_BORDER_COLOR(c)				\
	{	PreBorderColor = CurBorderColor;						\
		CurBorderColor.Color_b.r = c.cParam0;			\
		CurBorderColor.Color_b.g = c.cParam1;			\
		CurBorderColor.Color_b.b = c.cParam2;			\
		pFont->SetBorderColor(CurBorderColor.Color_dw);	}

	if (SeekToSpecialLine(pParam->nSkipLine, PreCtrl1, PreCtrl2, nSkipedHalfLines, nFontSize, pParam->bPicPackInSingleLine) < 0)
		return 0;
	
	if (PreCtrl1.cCtrl == KTC_COLOR)
		_SET_CURRENT_COLOR(PreCtrl1);
	if (PreCtrl2.cCtrl == KTC_BORDER_COLOR)
		_SET_CURRENT_BORDER_COLOR(PreCtrl2);

	int nSpanLines = 0, nLineHeight = 0;
	int bFirstLine = 1;	//true
	do
	{
		if (0 == nSpanLines || KTC_ENTER == TailCtrl.cCtrl)
		{
			//[wxb 2003-6-20][wxb 2003-7-10]
			GetCurLineHeight(nSpanLines, nLineHeight, nFontSize, pParam->bPicPackInSingleLine);
			if (nSpanLines > 1)
			{
				_ASSERT(nSpanLines * nFontSize - nLineHeight >= 0);
				switch (pParam->nVertAlign)
				{
				case 1:	//¾ÓÉÏ
					if (nLineHeight > nFontSize)
						y += (nLineHeight - nFontSize) / 2;
					break;
				case 2:	//¾ÓÏÂ
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
					if (nLineHeight > nFontSize)
						y -= (nLineHeight - nFontSize) / 2;
					break;
				case 0:	//¾ÓÖÐ
				default:
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
					break;
				}
			}
		}
		nCount = GetSimplexText(nPos, PreCtrl1, TailCtrl, nFontSize);

		switch(PreCtrl1.cCtrl)
		{
		case KTC_COLOR:
			_SET_CURRENT_COLOR(PreCtrl1);
			break;
		case KTC_BORDER_COLOR:
			_SET_CURRENT_BORDER_COLOR(PreCtrl1);
			break;
		case KTC_COLOR_RESTORE:
			CurFontColor = PreFontColor;
			break;
		case KTC_BORDER_RESTORE:
			CurBorderColor = PreBorderColor;
			pFont->SetBorderColor(CurBorderColor.Color_dw);
			break;
		}

		if (nCount)
		{
			pFont->OutputText(m_pBuffer + nPos, nCount, x, y, CurFontColor.Color_dw, 0);
			x += nCount / 2 * nFontSize;
			if (nCount % 2)
			{
				x += nFontHalfWidth[nHalfIndex];
				nHalfIndex ^= 1;
			}
		}

		switch(TailCtrl.cCtrl)
		{
		case KTC_ENTER:
			x = pParam->nX;
			y += nFontSize;

			if (nSpanLines > 1)
			{
				//[wxb 2003-6-20][wxb 2003-7-10]
				switch (pParam->nVertAlign)
				{
				case 1:	//¾ÓÉÏ
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
					if (nLineHeight > nFontSize)
						y -= (nLineHeight - nFontSize) / 2;
					break;
				case 2:	//¾ÓÏÂ
					if (nLineHeight > nFontSize)
						y += (nLineHeight - nFontSize) / 2;
					break;
				case 0:	//¾ÓÖÐ
				default:
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
					break;
				}
			}
			nLineDrawed += nSpanLines - (bFirstLine ? nSkipedHalfLines : 0);
			if (nLineDrawed >= pParam->nNumLine)
				TailCtrl.cCtrl = KTC_TAIL;	//ÎªÁËÖÐÖ¹Ñ­»·
			else
				nHalfIndex = 0;

			bFirstLine = 0;	//false
			nSkipedHalfLines = 0;
			break;
		case  KTC_COLOR:
			_SET_CURRENT_COLOR(TailCtrl);
			break;
		case KTC_BORDER_COLOR:
			_SET_CURRENT_BORDER_COLOR(TailCtrl);
			break;
		case KTC_COLOR_RESTORE:
			CurFontColor = PreFontColor;
			break;
		case KTC_BORDER_RESTORE:
			CurBorderColor = PreBorderColor;
			pFont->SetBorderColor(CurBorderColor.Color_dw);
			break;
		case KTC_INLINE_PIC: //[wxb 2003-6-20][wxb 2003-7-10]
			if (g_pIInlinePicSinkRP)
			{
				int cx, cy;
				if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(TailCtrl.wParam, cx, cy)))
				{
					int nPicY = y;
					if (nSkipedHalfLines && cy > nFontSize)
					{
						switch (pParam->nVertAlign)
						{
							case 1:	//¾ÓÉÏ
								if (nLineHeight > nFontSize)
									nPicY -= (nLineHeight - nFontSize) / 2;
								break;
							case 2:	//¾ÓÏÂ
								nPicY -= (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
								if (nLineHeight > nFontSize)
									nPicY += (nLineHeight - nFontSize) / 2;
								break;
							case 0:	//¾ÓÖÐ
							default:
								nPicY -= (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
								break;
						}
						if (cy > (nSpanLines - nSkipedHalfLines) * nFontSize)
							nPicY -= cy - (nSpanLines - nSkipedHalfLines) * nFontSize;
						else
							nPicY -= (int)(((cy - (nSpanLines - nSkipedHalfLines) * nFontSize)) * 1.5);
					}
					else
						nPicY -= (cy - nFontSize) / 2;
					if (SUCCEEDED(g_pIInlinePicSinkRP->DrawPic(TailCtrl.wParam, x, nPicY, nAlpha)))
					{
						x += cx;
					}
				}
			}
			break;
		}
	}while(TailCtrl.cCtrl != KTC_TAIL);
	if (x > pParam->nX)
		nLineDrawed += nSpanLines - (bFirstLine ? nSkipedHalfLines : 0);
	return nLineDrawed;
}


//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÅÐ¶ÏÄ³Ò»¸ö×ø±ê£¬ÔÚÖ¸¶¨µÄÎ»ÖÃÊä³öµÄ×Ö·û´®ÖÐ£¬ËùÕ¼µÄÆ«ÒÆÁ¿
//	·µ»Ø£ºÀëÖ¸¶¨×ø±ê×î½üµÄ×Ö·ûÆ«ÒÆ(×Ö·ûÆ«ÒÆ£ºÓÐn¸ö×Ö·û£¬ÄÇ¸ö¾ÍÓÐ0¡«nÒ»¹²n+1¸öÆ«ÒÆ)
//	×¢Òâ£ºÈç¹û³¬³ö×Ö·ûËùÕ¼µÄ¾ØÐÎÔò·µ»ØµÄÆ«ÒÆÎªÀëÖ¸¶¨×ø±ê×î½üµÄÒ»¸ö×Ö·ûµÄÆ«ÒÆ
//		  ±¾º¯Êý²»ÄÜ¸ú DrawTextLine(..) º¯ÊýÇ¶Ì×Ê¹ÓÃ
//	[wxb 2003-7-21]
//--------------------------------------------------------------------------
int KTextProcess::TransXYPosToCharOffset(int nX, int nY, iFont* pFont, int nFontSize, KOutputTextParam* pParam)
{
	_ASSERT(pParam);
	if (pFont == NULL || pParam->nNumLine <= 0)
		return 0;

	KTP_CTRL	PreCtrl1, PreCtrl2, TailCtrl;
	int			nCount = 0, nPos = 0;
	int			x = pParam->nX, y = pParam->nY, nLineDrawed = 0;

	int			nHalfIndex = 0, nFontHalfWidth[2];
	int			nSkipedHalfLines = 0;	//ÒòÎªÇ¶ÈëÊ½Í¼Æ¬Ôì³ÉµÄ¿ç°ëÐÐÏÖÏó
	nFontHalfWidth[0] = nFontSize / 2;
	nFontHalfWidth[1] = (nFontSize + 1) / 2;

	TP_COLOR	PreFontColor, CurFontColor;
	TP_COLOR	PreBorderColor, CurBorderColor;

	PreFontColor.Color_dw = CurFontColor.Color_dw = pParam->Color;
	PreBorderColor.Color_dw = CurBorderColor.Color_dw = 0xff000000;

#define	_SET_CURRENT_COLOR(c)				\
	{	PreFontColor = CurFontColor;				\
		CurFontColor.Color_b.r = c.cParam0;	\
		CurFontColor.Color_b.g = c.cParam1;	\
		CurFontColor.Color_b.b = c.cParam2;	}
#define	_SET_CURRENT_BORDER_COLOR(c)				\
	{	PreBorderColor = CurBorderColor;						\
		CurBorderColor.Color_b.r = c.cParam0;			\
		CurBorderColor.Color_b.g = c.cParam1;			\
		CurBorderColor.Color_b.b = c.cParam2;			\
		pFont->SetBorderColor(CurBorderColor.Color_dw);	}

	if (SeekToSpecialLine(pParam->nSkipLine, PreCtrl1, PreCtrl2, nSkipedHalfLines, nFontSize, pParam->bPicPackInSingleLine) < 0)
		return 0;
	
	if (PreCtrl1.cCtrl == KTC_COLOR)
		_SET_CURRENT_COLOR(PreCtrl1);
	if (PreCtrl2.cCtrl == KTC_BORDER_COLOR)
		_SET_CURRENT_BORDER_COLOR(PreCtrl2);

	int nSpanLines = 0, nLineHeight = 0;
	int bFirstLine = 1;			//true
	int nOffsetXThisLine = 0, nOldX = 0, nOldReadPos = 0;
	do
	{
		if (0 == nSpanLines || KTC_ENTER == TailCtrl.cCtrl)
		{
			//[wxb 2003-6-20][wxb 2003-7-10]
			GetCurLineHeight(nSpanLines, nLineHeight, nFontSize, pParam->bPicPackInSingleLine);
			if (nSpanLines > 1)
			{
				_ASSERT(nSpanLines * nFontSize - nLineHeight >= 0);
				switch (pParam->nVertAlign)
				{
				case 1:	//¾ÓÉÏ
					if (nLineHeight > nFontSize)
						y += (nLineHeight - nFontSize) / 2;
					break;
				case 2:	//¾ÓÏÂ
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
					if (nLineHeight > nFontSize)
						y -= (nLineHeight - nFontSize) / 2;
					break;
				case 0:	//¾ÓÖÐ
				default:
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
					break;
				}
			}

			nOffsetXThisLine = -1;	//Ã¿Ò»ÐÐ¿ªÊ¼µÄÊ±ºòÇå¿Õ
		}
		nOldReadPos = m_nReadPos;
		nCount = GetSimplexText(nPos, PreCtrl1, TailCtrl, nFontSize);

		switch(PreCtrl1.cCtrl)
		{
		case KTC_COLOR:
			_SET_CURRENT_COLOR(PreCtrl1);
			break;
		case KTC_BORDER_COLOR:
			_SET_CURRENT_BORDER_COLOR(PreCtrl1);
			break;
		case KTC_COLOR_RESTORE:
			CurFontColor = PreFontColor;
			break;
		case KTC_BORDER_RESTORE:
			CurBorderColor = PreBorderColor;
			pFont->SetBorderColor(CurBorderColor.Color_dw);
			break;
		}

		if (nCount)
		{
			pFont->OutputText(m_pBuffer + nPos, nCount, x, y, CurFontColor.Color_dw, 0);
			nOldX = x;
			x += nCount / 2 * nFontSize;
			if (nCount % 2)
			{
				x += nFontHalfWidth[nHalfIndex];
				nHalfIndex ^= 1;
			}

			//¼ÆËãµãÖÐµÄÎ»ÖÃ
			if (nOffsetXThisLine != -1)
			{
				if (nX <= nOldX)
					nOffsetXThisLine = nOldReadPos;
				else if (nX < x)
					nOffsetXThisLine = nOldReadPos + (nX - nOldX + nFontSize / 4) * 2 / nFontSize;
			}
		}

		switch(TailCtrl.cCtrl)
		{
		case KTC_ENTER:
			x = pParam->nX;
			y += nFontSize;

			if (nSpanLines > 1)
			{
				//[wxb 2003-6-20][wxb 2003-7-10]
				switch (pParam->nVertAlign)
				{
				case 1:	//¾ÓÉÏ
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
					if (nLineHeight > nFontSize)
						y -= (nLineHeight - nFontSize) / 2;
					break;
				case 2:	//¾ÓÏÂ
					if (nLineHeight > nFontSize)
						y += (nLineHeight - nFontSize) / 2;
					break;
				case 0:	//¾ÓÖÐ
				default:
					y += (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
					break;
				}
			}
			nLineDrawed += nSpanLines - (bFirstLine ? nSkipedHalfLines : 0);
			if (nLineDrawed >= pParam->nNumLine)
				TailCtrl.cCtrl = KTC_TAIL;	//ÎªÁËÖÐÖ¹Ñ­»·
			else
				nHalfIndex = 0;

			bFirstLine = 0;			//false
			nSkipedHalfLines = 0;	//false
			break;
		case  KTC_COLOR:
			_SET_CURRENT_COLOR(TailCtrl);
			break;
		case KTC_BORDER_COLOR:
			_SET_CURRENT_BORDER_COLOR(TailCtrl);
			break;
		case KTC_COLOR_RESTORE:
			CurFontColor = PreFontColor;
			break;
		case KTC_BORDER_RESTORE:
			CurBorderColor = PreBorderColor;
			pFont->SetBorderColor(CurBorderColor.Color_dw);
			break;
		case KTC_INLINE_PIC: //[wxb 2003-6-20][wxb 2003-7-10]
			if (g_pIInlinePicSinkRP)
			{
				int cx, cy;
				if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(TailCtrl.wParam, cx, cy)))
				{
					int nPicY = y;
					if (nSkipedHalfLines && cy > nFontSize)
					{
						switch (pParam->nVertAlign)
						{
							case 1:	//¾ÓÉÏ
								if (nLineHeight > nFontSize)
									nPicY -= (nLineHeight - nFontSize) / 2;
								break;
							case 2:	//¾ÓÏÂ
								nPicY -= (nSpanLines - nSkipedHalfLines - 1) * nFontSize;
								if (nLineHeight > nFontSize)
									nPicY += (nLineHeight - nFontSize) / 2;
								break;
							case 0:	//¾ÓÖÐ
							default:
								nPicY -= (nSpanLines - nSkipedHalfLines - 1) * nFontSize / 2;
								break;
						}
						if (cy > (nSpanLines - nSkipedHalfLines) * nFontSize)
							nPicY -= cy - (nSpanLines - nSkipedHalfLines) * nFontSize;
						else
							nPicY -= (int)(((cy - (nSpanLines - nSkipedHalfLines) * nFontSize)) * 1.5);
					}
					else
						nPicY -= (cy - nFontSize) / 2;
					if (SUCCEEDED(g_pIInlinePicSinkRP->DrawPic(TailCtrl.wParam, x, nPicY)))
					{
						nOldX = x;
						x += cx;
						//¼ÆËãµãÖÐµÄÎ»ÖÃ
						if (nOffsetXThisLine != -1)
						{
							if (nX <= nOldX)
								nOffsetXThisLine = nOldReadPos;
							else if (nX < x)
								nOffsetXThisLine = nOldReadPos + (nX - nOldX + cx / 2) / cx;
						}
					}
				}
			}
			break;
		}

		//ÅÐ¶ÏÊÇ·ñ¾ÍµãÔÚÕâÒ»ÐÐ
		if (KTC_ENTER == TailCtrl.cCtrl && y > nY || KTC_TAIL == TailCtrl.cCtrl)
		{
			if (nOffsetXThisLine != -1)
				return nOffsetXThisLine;
			else
				return m_nReadPos;
		}

	}while(TailCtrl.cCtrl != KTC_TAIL);
	if (x > pParam->nX)
		nLineDrawed += nSpanLines - (bFirstLine ? nSkipedHalfLines : 0);

	_ASSERT(0);
	return nOffsetXThisLine;
}


//[wxb 2003-6-20]
void KTextProcess::GetCurLineHeight(int& nSpanLines, int& nHeight, int nFontSize, int bPicPackInSingleLine/*=FALSE*/)
{
	unsigned short wIndex = 0;
	nHeight = nFontSize;
	//¸ÄÎªÍ¼Æ¬ÈçºÎ¶¼Ö»Õ¼Ò»ÐÐ¸ß£¬ËùÒÔ×¢ÊÍÏÂÃæ´úÂë
	if (g_pIInlinePicSinkRP && !bPicPackInSingleLine)
	{
		Backup();
		int bVNFont = TIsTCVNText();
		unsigned char cCode;
		while(m_nReadPos < m_nCount)
		{
			cCode = m_pBuffer[m_nReadPos];
			//ÅÐ¶ÏÍ·Ò»¸ö×Ö·ûÊÇ·ñÊÇ¿ØÖÆ·ûºÅ
			if (!bVNFont && cCode > 0x80)	//¿ÉÄÜÊÇÖÐÎÄ×Ö·û
			{	//×Ö·û´®ÏÈ±»´¦Àí¹ý£¬²»»á³öÏÖµ¥¸öµ¥×Ö½ÚÊýÖµ´óÓÚ0x80µÄ×Ö·û
				m_fCurrentLineLen += 2;
				m_nCurrentLineLen = (int)(m_fCurrentLineLen + 0.9999);
				if (m_nMaxLineLen == 0 || m_nCurrentLineLen < m_nMaxLineLen)
				{
					m_nReadPos += 2;
				}
				else if(m_nCurrentLineLen == m_nMaxLineLen || m_nCurrentLineLen == 0)
				{
					m_nReadPos += 2;
					break;
				}
				else
				{
					m_nCurrentLineLen -= 2;
					m_fCurrentLineLen -= 2;
					break;
				}
			}
			else if (cCode == KTC_INLINE_PIC)	//[wxb 2003-6-20]
			{
				wIndex	= *((unsigned short*)(m_pBuffer + m_nReadPos + 1));
				m_nReadPos += 1 + sizeof(unsigned short);
				if (g_pIInlinePicSinkRP)
				{
					int cx, cy;
					if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(wIndex, cx, cy)))
					{
						if (cy > nHeight)
							nHeight = cy;
						m_fCurrentLineLen += cx;
						m_nCurrentLineLen = (int)(m_fCurrentLineLen + 0.9999);
					}
				}
			}
			else if (cCode == KTC_COLOR || cCode == KTC_BORDER_COLOR)//ÑÕÉ«¿ØÖÆ
				m_nReadPos += 4;
			else if (cCode == KTC_COLOR_RESTORE || cCode == KTC_BORDER_RESTORE)
				m_nReadPos += 1;
			else if (cCode == KTC_ENTER)
			{
				m_nReadPos ++;
				break;
			}
			else
			{
				m_nCurrentLineLen ++;
				m_fCurrentLineLen += 1;
				m_nReadPos ++;
			}
			if (m_nMaxLineLen)
			{
				if (m_nCurrentLineLen >= m_nMaxLineLen)
					break;
				else if (!bVNFont && m_nCurrentLineLen && m_nCurrentLineLen + 3 >= m_nMaxLineLen)
				{
					const char* pNext = TGetSecondVisibleCharacterThisLine((const char*)m_pBuffer, m_nReadPos, m_nCount);
					if (pNext && TIsCharacterNotAlowAtLineHead(pNext))
						break;
				}
			}
		}

		Restore();
	}
	nSpanLines = (nHeight % nFontSize) ? nHeight / nFontSize + 1 : nHeight / nFontSize;
}
