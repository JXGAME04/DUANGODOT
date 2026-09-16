//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KNpcFindPath.cpp
// Date:	2002.01.06
// Code:	±ß³ÇÀË×Ó
// Desc:	Obj Class
//---------------------------------------------------------------------------

#include	"KCore.h"
#include	<math.h>
#include	"KMath.h"
#include	"KNpcFindPath.h"
#include	"KSubWorld.h"
#include	"KNpc.h"

#define	MAX_FIND_TIMER	30

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º¹¹Ôìº¯Êý
//-------------------------------------------------------------------------
KNpcFindPath::KNpcFindPath()
{
	m_nDestX = 0;
	m_nDestY = 0;
	m_nFindTimer = 0;
	m_nMaxTimeLong = MAX_FIND_TIMER;
	m_nFindState = 0;
	m_nPathSide = 0;
	m_nFindTimes = 0;
	m_NpcIdx = 0;
};

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º³õÊ¼»¯
//	²ÎÊý£ºnNpc :Õâ¸öÑ°Â·ÊÇÊôÓÚÄÄ¸ö npc µÄ
//-------------------------------------------------------------------------
void KNpcFindPath::Init(int nNpc)
{
	m_NpcIdx = nNpc;
	m_nDestX = 0;
	m_nDestY = 0;
	m_nFindTimer = 0;
	m_nMaxTimeLong = MAX_FIND_TIMER;
	m_nFindState = 0;
	m_nPathSide = 0;
	m_nFindTimes = 0;
}
//-------------------------------------------------------------------------
//	¹¦ÄÜ£º´«Èëµ±Ç°×ø±ê¡¢·½Ïò¡¢Ä¿±êµã×ø±ê¡¢ËÙ¶È£¬Ñ°Â·ÕÒµ½ÏÂÒ»²½Ó¦¸Ã×ßµÄ·½Ïò
//	·µ»ØÖµ£»Èç¹û·µ»Ø0£ºÓÐÕÏ°­£¬²»ÄÜ×ßÁË;1£¬ÕÒµ½Ò»¸ö·½Ïò£¬·½ÏòÖµ·ÅÔÚpnGetDir (°´64·½Ïò)£»
//			-1£ºµ½µØÍ¼ÍâÃæÈ¥ÁË
//-------------------------------------------------------------------------
#define		defFIND_PATH_STOP_DISTANCE		64
int	KNpcFindPath::GetDir(int nXpos,int nYpos, int nDir, int nDestX, int nDestY, int nMoveSpeed, int *pnGetDir)
{
	// Èç¹û¾àÀë½Ó½ü£¬ÈÏÎªÒÑ¾­×ßµ½ÁË
	nXpos >>= 10;
	nYpos >>= 10;
	if ( !CheckDistance(nXpos, nYpos, nDestX, nDestY, nMoveSpeed))
	{
		m_nFindTimer = 0;
		m_nFindState = 0;
		m_nFindTimes = 0;
		return 0;
	}
#ifndef _SERVER
	int	nDistance = g_GetDistance(nXpos, nYpos, nDestX, nDestY);
	if(nDistance >= 0 && nDistance <= 4)	//chèng chuyÓn h­íng m_Dir v× ®i qu¸ ®iÓm ®Ých
	{
		m_nFindTimer = 0;
		m_nFindState = 0;
		m_nFindTimes = 0;
		return 0;
	}
#endif
	// Ä¿±êµãÈç¹ûÓÐ±ä»¯£¬È¡ÏûÔ­À´µÄÕÒÂ·×´Ì¬
	if (m_nDestX != nDestX || m_nDestY != nDestY)
	{
		m_nFindTimer = 0;
		m_nFindState = 0;
		m_nFindTimes = 0;
		m_nDestX = nDestX;
		m_nDestY = nDestY;
	}

	int		x, y, nWantDir;
	nWantDir = g_GetDirIndex(nXpos, nYpos, nDestX, nDestY);
	x = g_DirCos(nWantDir, 64) * nMoveSpeed;
	y = g_DirSin(nWantDir, 64) * nMoveSpeed;
	// Èç¹ûÓÐÂ·£¬Ö±½Ó×ß
	int nCheckBarrier = CheckBarrier(x, y);
	if ( nCheckBarrier == 0 )
	{
		m_nFindState = 0;
		*pnGetDir = nWantDir;
		return 1;
	}
	// µØÍ¼±ßÔµ
	else if (nCheckBarrier == 0xff)
	{
		return -1;
	}

	int		i;
	// ´Ó·ÇÕÒÂ·×´Ì¬½øÈëÕÒÂ·×´Ì¬
	if (m_nFindState == 0)
	{
		// Èç¹ûÄ¿±êµãÊÇÕÏ°­¶øÇÒ¾ßÌå¹ý½ü£¬²»ÕÒÁË
#ifdef _SERVER
		nCheckBarrier = SubWorld[Npc[m_NpcIdx].m_SubWorldIndex].TestBarrier(nDestX, nDestY);
#else
		if (Npc[m_NpcIdx].m_RegionIndex >= 0)
			nCheckBarrier = SubWorld[0].TestBarrier(nDestX, nDestY);
		else
			nCheckBarrier = 0xff;
#endif
		if (nCheckBarrier != 0 && !CheckDistance(nXpos, nYpos, nDestX, nDestY, defFIND_PATH_STOP_DISTANCE))
		{
			m_nFindTimes = 0;
			return 0;
		}
		
		// Èç¹ûµÚ¶þ´Î½øÈë¹ÕÍä×´Ì¬£¬²»ÕÒÁË£¨Ö»¹ÕÒ»´ÎÍä£©
		m_nFindTimes++;
		if (m_nFindTimes > 1)
		{
			m_nFindTimes = 0;
			return 0;
		}
		int		nTempDir8, nTempDir64;
		nTempDir8 = Dir64To8(nWantDir) + 8;
		
		// ×ª»»³É 8 ·½Ïòºóµ±Ç°·½ÏòÊÇ·ñ¿ÉÐÐ
		nTempDir64 = Dir8To64(nTempDir8 & 0x07);
		x = g_DirCos( nTempDir64, 64 ) * nMoveSpeed;
		y = g_DirSin( nTempDir64, 64 ) * nMoveSpeed;
		if ( CheckBarrier(x, y) == 0 )
		{
			m_nFindState = 1;
			m_nFindTimer = 0;
			if ((nTempDir64 < nWantDir && nWantDir - nTempDir64 <= 4) || (nTempDir64 > nWantDir && nTempDir64 - nWantDir >= 60))
				m_nPathSide = 0;
			else
				m_nPathSide = 1;
			*pnGetDir = nTempDir64;
			return 1;
		}
		// °´ 8 ·½ÏòÑ°ÕÒ£¬¼ì²é³ýÈ¥ÕýÃæºÍ±³ÃæµÄÁíÍâ 6 ¸ö·½Ïò
		for (i = 1; i < 4; i++)
		{
			nTempDir64 = Dir8To64((nTempDir8 + i) & 0x07);
			x = g_DirCos( nTempDir64, 64 ) * nMoveSpeed;
			y = g_DirSin( nTempDir64, 64 ) * nMoveSpeed;
			if ( CheckBarrier(x, y) == 0 )
			{
				m_nFindState = 1;
				m_nFindTimer = 0;
				m_nPathSide = 1;
				*pnGetDir = nTempDir64;
				return 1;
			}

			nTempDir64 = Dir8To64((nTempDir8 - i) & 0x07);
			x = g_DirCos( nTempDir64, 64 ) * nMoveSpeed;
			y = g_DirSin( nTempDir64, 64 ) * nMoveSpeed;
			if ( CheckBarrier(x, y) == 0 )
			{
				m_nFindState = 1;
				m_nFindTimer = 0;
				m_nPathSide = 0;
				*pnGetDir = nTempDir64;
				return 1;
			}
		}
		return 0;
	}
	// Ô­±¾ÊÇÕÒÂ·×´Ì¬£¬¼ÌÐøÕÒÂ·
	else
	{
		// Èç¹ûÕÒÂ·Ê±¼ä¹ý³¤£¬²»ÕÒÁË
		if (m_nFindTimer >= m_nMaxTimeLong)
		{
			m_nFindState = 0;
			return 0;
		}
		m_nFindTimer++;
		int		nWantDir8, nTempDir64;
		nWantDir8 = Dir64To8(nWantDir) + 8;
		// µ±Ç°·½ÏòÎ»ÓÚÄ¿±ê·½ÏòµÄÓÒ²à
		if (m_nPathSide == 1)
		{
			// ÅÐ¶ÏÊÇ·ñÐèÒª¼ì²âµ±Ç°Ä¿±ê³¯Ïò¶ÔÓ¦µÄ 8 ·½ÏòÉÏ
			nTempDir64 = Dir8To64(nWantDir8 & 0x07);
			if ((nTempDir64 < nWantDir && nWantDir - nTempDir64 <= 4) || (nTempDir64 > nWantDir && nTempDir64 - nWantDir >= 60))
				i = 1;
			else
				i = 0;
			// ¹ÕÍä¹ý³Ì
			for (; i < 4; i++)
			{
				nTempDir64 = Dir8To64((nWantDir8 + i) & 0x07);
				x = g_DirCos( nTempDir64, 64 ) * nMoveSpeed;
				y = g_DirSin( nTempDir64, 64 ) * nMoveSpeed;
				if ( CheckBarrier(x, y) == 0 )
				{
					*pnGetDir = nTempDir64;
					return 1;
				}
			}
			m_nFindState = 0;
			m_nFindTimer = 0;
			return 0;
		}
		// µ±Ç°·½ÏòÎ»ÓÚÄ¿±ê·½ÏòµÄ×ó²à
		else
		{
			// ÅÐ¶ÏÊÇ·ñÐèÒª¼ì²âµ±Ç°Ä¿±ê³¯Ïò¶ÔÓ¦µÄ 8 ·½ÏòÉÏ
			nTempDir64 = Dir8To64(nWantDir8 & 0x07);
			if ((nTempDir64 < nWantDir && nWantDir - nTempDir64 <= 4) || (nTempDir64 > nWantDir && nTempDir64 - nWantDir >= 60))
				i = 0;
			else
				i = 1;
			// ¹ÕÍä¹ý³Ì
			for (; i < 4; i++)
			{
				nTempDir64 = Dir8To64((nWantDir8 - i) & 0x07);
				x = g_DirCos( nTempDir64, 64 ) * nMoveSpeed;
				y = g_DirSin( nTempDir64, 64 ) * nMoveSpeed;
				if ( CheckBarrier(x, y) == 0 )
				{
					*pnGetDir = nTempDir64;
					return 1;
				}
			}
			m_nFindState = 0;
			m_nFindTimer = 0;
			return 0;
		}
	}

	m_nFindState = 0;
	m_nFindTimer = 0;
	return 0;
}

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º	64 ·½Ïò×ª»»Îª 8 ·½Ïò
//-------------------------------------------------------------------------
int		KNpcFindPath::Dir64To8(int nDir)
{
	return ((nDir + 4) >> 3) & 0x07;
}

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º	8 ·½Ïò×ª»»Îª 64 ·½Ïò
//-------------------------------------------------------------------------
int		KNpcFindPath::Dir8To64(int nDir)
{
	return nDir << 3;
}

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º	ÅÐ¶ÏÁ½µã¼äµÄÖ±Ïß¾àÀëÊÇ·ñ´óÓÚ»òµÈÓÚ¸ø¶¨¾àÀë
//	·µ»Ø£º	¾àÀëÐ¡ÓÚ nDistance ·µ»Ø FALSE £¬·ñÔò·µ»Ø TRUE
//-------------------------------------------------------------------------
BOOL	KNpcFindPath::CheckDistance(int x1, int y1, int x2, int y2, int nDistance)
{
	return ( (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) >= nDistance * nDistance );
}

//-------------------------------------------------------------------------
//	¹¦ÄÜ£º	ÅÐ¶ÏÄ³¸öµãÊÇ·ñÊÇÕÏ°­
//-------------------------------------------------------------------------
int	KNpcFindPath::CheckBarrier(int nChangeX, int nChangeY)
{
#ifdef _SERVER
	return SubWorld[Npc[m_NpcIdx].m_SubWorldIndex].TestBarrierMin(Npc[m_NpcIdx].m_RegionIndex, Npc[m_NpcIdx].m_MapX, Npc[m_NpcIdx].m_MapY, Npc[m_NpcIdx].m_OffX, Npc[m_NpcIdx].m_OffY, nChangeX, nChangeY);
#else
	if (Npc[m_NpcIdx].m_RegionIndex >= 0)
		return SubWorld[0].TestBarrierMin(Npc[m_NpcIdx].m_RegionIndex, Npc[m_NpcIdx].m_MapX, Npc[m_NpcIdx].m_MapY, Npc[m_NpcIdx].m_OffX, Npc[m_NpcIdx].m_OffY, nChangeX, nChangeY);
	else
		return 0xff;
#endif
}

