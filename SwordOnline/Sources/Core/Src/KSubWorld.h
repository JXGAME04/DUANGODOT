#ifndef	KWorldH
#define	KWorldH

#ifdef _SERVER
#define	MAX_SUBWORLD	80
#else
#define	MAX_SUBWORLD	1
#endif
#define	VOID_REGION		-2
//-------------------------------------------------------------
#include "KEngine.h"
#include "KRegion.h"
#include "KWeatherMgr.h"

#ifdef _SERVER
#include "KMission.h"
#include "KMissionArray.h"
#define MAX_SUBWORLD_MISSIONCOUNT 10
#define MAX_GLOBAL_MISSIONCOUNT 50
typedef KMissionArray <KMission , MAX_TIMER_PERMISSION> KSubWorldMissionArray;
typedef KMissionArray <KMission , MAX_GLOBAL_MISSIONCOUNT> KGlobalMissionArray;
extern KGlobalMissionArray g_GlobalMissionArray;
#endif

//-------------------------------------------------------------
#ifndef _SERVER
#define FINDPATH_VERSION	0	//t¨ng lªn ®Ó cËp nhËt d÷ liÖu map míi
#define MAX_CELL		2400000
struct VGridNode
{
	int parentId;
	int connStart;
    int connCount;
	WORD x;
	WORD y;
	WORD obs;
	BYTE w;
	BYTE h;
	VGridNode()
	{
		w = 1;
		h = 1;
		connStart = -1;
		connCount = 0;
	}
};

struct VGridNeighbour
{
    int toParentId;  // id to m_vNeighbour
	int cost;
};

#endif
#ifndef TOOLVERSION
class KSubWorld
#else

class CORE_API KSubWorld
#endif
{
public:	
	int			m_nIndex;
	int			m_SubWorldID;
#ifdef _SERVER
	KSubWorldMissionArray m_MissionArray;
#endif
	KRegion*	m_Region;
#ifndef _SERVER
	int			m_ClientRegionIdx;//[MAX_REGION];
	char		m_szMapPath[FILE_NAME_LENGTH];
	//KLittleMap	m_cLittleMap;
#endif
	int			m_nWorldRegionWidth;			//	SubWorldÀï¿í¼¸¸öRegion
	int			m_nWorldRegionHeight;			//	SubWorldÀï¸ß¼¸¸öRegion
	int			m_nTotalRegion;					//	SubWorldÀïRegion¸öÊý
	int			m_nRegionWidth;					//	RegionµÄ¸ñ×Ó¿í¶È
	int			m_nRegionHeight;				//	RegionµÄ¸ñ×Ó¸ß¶È
	int			m_nCellWidth;					//	CellµÄÏñËØ¿í¶È
	int			m_nCellHeight;					//	CellµÄÏñËØ¸ß¶È
	int			m_nRegionBeginX;				
	int			m_nRegionBeginY;
	int			m_nWeather;						//	ÌìÆø±ä»¯
	DWORD		m_dwCurrentTime;				//	µ±Ç°Ö¡
	KWorldMsg	m_WorldMessage;					//	ÏûÏ¢
	KList		m_NoneRegionNpcList;			//	²»ÔÚµØÍ¼ÉÏµÄNPC

#ifdef _SERVER
	KWeatherMgr *m_pWeatherMgr;
	int			m_nSeriesRate[series_num];
#endif
#ifndef _SERVER
	HANDLE  m_hLoadPathGrid;
    volatile BOOL m_bStopThread;
#endif
public:
	KSubWorld();
	~KSubWorld();
	void		Activate();
	void		GetFreeObjPos(POINT& pos);
	BOOL		CanPutObj(POINT pos);
	void		ObjChangeRegion(int nSrcRegionIdx, int nDesRegionIdx, int nObjIdx);
	void		MissleChangeRegion(int nSrcRegionIdx, int nDesRegionIdx, int nObjIdx);
	void		AddPlayer(int nRegion, int nIdx);
	void		RemovePlayer(int nRegion, int nIdx);
	void		Close();
	int			GetDistance(int nRx1, int nRy1, int nRx2, int nRy2);						// ÏñËØ¼¶×ø±ê
	void		Map2Mps(int nR, int nX, int nY, int nDx, int nDy, int *nRx, int *nRy);		// ¸ñ×Ó×ø±ê×ªÏñËØ×ø±ê
	static void Map2Mps(int nRx, int nRy, int nX, int nY, int nDx, int nDy, int *pnX, int *pnY);		// ¸ñ×Ó×ø±ê×ªÏñËØ×ø±ê
	void		Mps2Map(int Rx, int Ry, int * nR, int * nX, int * nY, int *nDx, int * nDy);	// ÏñËØ×ø±ê×ª¸ñ×Ó×ø±ê
	void		GetMps(int *nX, int *nY, int nSpeed, int nDir, int nMaxDir = 64);			// È¡µÃÄ³·½ÏòÄ³ËÙ¶ÈÏÂÒ»µãµÄ×ø±ê
	BYTE		TestBarrier(int nMpsX, int nMpsY);
	BYTE		TestBarrier(int nRegion, int nMapX, int nMapY, int nDx, int nDy, int nChangeX, int nChangeY);	// ¼ì²âÏÂÒ»µãÊÇ·ñÎªÕÏ°­
	BYTE		TestBarrierMin(int nRegion, int nMapX, int nMapY, int nDx, int nDy, int nChangeX, int nChangeY);	// ¼ì²âÏÂÒ»µãÊÇ·ñÎªÕÏ°­
	BYTE		GetBarrier(int nMpsX, int nMpsY);											// È¡µÃÄ³µãµÄÕÏ°­ÐÅÏ¢
	DWORD		GetTrap(int nMpsX, int nMpsY);
	void		MessageLoop();
	int			FindRegion(int RegionID);													// ÕÒµ½Ä³IDµÄRegionµÄË÷Òý
#ifdef _SERVER
	int			RevivalAllNpc();//½«µØÍ¼ÉÏËùÓÐµÄNpc°üÀ¨ÒÑËÀÍöµÄNpcÈ«²¿»Ö¸´³ÉÔ­Ê¼×´Ì¬
	void		BroadCast(const char* pBuffer, size_t uSize);
	BOOL		LoadMap(int nIdx);
	void		LoadObject(char* szPath, char* szFile);
	void		NpcChangeRegion(int nSrcRegionIdx, int nDesRegionIdx, int nNpcIdx);
	void		PlayerChangeRegion(int nSrcRegionIdx, int nDesRegionIdx, int nObjIdx);
	BOOL		SendSyncData(int nIdx, int nClient);
	int			GetRegionIndex(int nRegionID);
	int			FindNpcFromName(const char * szName);
#endif
#ifndef _SERVER
	BOOL		LoadMap(int nIdx, int nRegion);
	void		NpcChangeRegion(int nSrcRegionIdx, int nDesRegionIdx, int nNpcIdx);
	void		Paint();
	void		Mps2Screen(int *Rx, int *Ry);
	void		Screen2Mps(int *Rx, int *Ry);
	int			FindFreeRegion(int nX = 0, int nY = 0);
	void		ProcLoadPathGrid();
	int			FindPath(int nX, int nY, bool bCheckNpc = false);
	void		StopPath() {
		m_nTargetX = 0;
		m_nTargetY = 0;
		m_nCurStep = 0;
		m_vRetPath.clear();
	};
	bool HaveTarget(int& x, int& y)
	{
		x = m_nTargetX;
		y = m_nTargetY;
		if(m_vRetPath.size() > 0 && m_nTargetX > 0 && m_nTargetY > 0)
			return true;
		return false;
	}
#endif
private:
	void		LoadTrap();
	void		ProcessMsg(KWorldMsgNode *pMsg);
#ifndef _SERVER
	void		LoadCell();
	BOOL		m_bCheckedRegion[MAX_REGION];
	int			BlockHeuristic(int aId, int bId);
	int			FindPath_Block(int startParentId, int goalParentId);
	int			FindPath_NpcObs(int startParentId, int goalParentId);
	int			FindFreeBlockAround(int nMainId, int nNearX, int nNearY);
	int			m_nGridW;
	int			m_nGridH;
	int			m_nGridTotal;
	int			m_nTargetX;
	int			m_nTargetY;
	int			m_nCurStep;
	UINT		m_uStepDelayTime;
	char		m_szPathName[FILE_NAME_LENGTH];
	std::vector<VGridNeighbour> m_vNeighbour;
	std::vector<int> m_vRetPath;
	VGridNode	m_GridNode[MAX_CELL];
	int		m_pTempCover[MAX_CELL];
	BOOL	m_bHavePath;
	BOOL	m_uPaintTime;
#endif
};

#ifndef TOOLVERSION
extern KSubWorld	SubWorld[MAX_SUBWORLD];
#else 
extern CORE_API KSubWorld	SubWorld[MAX_SUBWORLD];
#endif
#endif
