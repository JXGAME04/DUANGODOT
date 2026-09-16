#include "KCore.h"
#include <math.h>
#include "KRegion.h"
#include "KMath.h"
#include "KNpc.h"
#include "KPlayer.h"
#include "KNpcSet.h"
#include "KObjSet.h"
#include "KPlayerSet.h"
#include "KMissleSet.h"
#ifndef _STANDALONE
#include "crtdbg.h"
#endif
#include "Scene/ObstacleDef.h"
#ifdef _SERVER
//#include "KNetServer.h"
//#include "../MultiServer/Heaven/Interface/iServer.h"
#endif
#ifndef _SERVER
#include "scene/KScenePlaceC.h"
#include <queue>
#include <algorithm>
#endif
#include "KSubWorld.h"

#ifndef TOOLVERSION
KSubWorld	SubWorld[MAX_SUBWORLD];
#else 
CORE_API KSubWorld	SubWorld[MAX_SUBWORLD];
#endif

#define		defLOGIC_CELL_WIDTH			32
#define		defLOGIC_CELL_HEIGHT		32

#ifdef _SERVER
KGlobalMissionArray g_GlobalMissionArray;
#endif

KSubWorld::KSubWorld()
{
	m_Region = NULL;
	m_nRegionWidth	= 512 / 32;
	m_nRegionHeight	= 1024 / 32;
	m_nCellWidth = defLOGIC_CELL_WIDTH;
	m_nCellHeight = defLOGIC_CELL_HEIGHT;
	m_dwCurrentTime = 0;
	m_SubWorldID = -1;
	m_nWeather = WEATHERID_NOTHING;
#ifndef _SERVER
	m_nWorldRegionWidth = 5;
	m_nWorldRegionHeight = 3;
	//memset(m_ClientRegionIdx, 0, sizeof(this->m_ClientRegionIdx));
	m_ClientRegionIdx = 0;
	memset(this->m_szMapPath, 0, sizeof(this->m_szMapPath));
#endif
#ifdef _SERVER
	m_nWorldRegionWidth = 0;
	m_nWorldRegionHeight = 0;
	m_pWeatherMgr = NULL;
	//MissionArryÖÐ0ËäÈ»ÎÞÐ§£¬µ«»¹ÊÇ¼ÓÉÏÈ¥£¬ÈÝ´í.TotalCount Îª¿Õ¼ä¼õ1£¬ËùÒÔ²»»áÔ½½ç
	for (int i = 0; i <= m_MissionArray.GetTotalCount(); i ++)
	{
		m_MissionArray.m_Data[i].SetOwner(this);
	}
#endif
	m_nTotalRegion = m_nWorldRegionWidth * m_nWorldRegionHeight;
#ifndef _SERVER
	m_hLoadPathGrid = NULL;
    m_bStopThread = FALSE;
	m_nTargetX = 0;
	m_nTargetY = 0;
	m_bHavePath = FALSE;
	m_uPaintTime = 0;
#endif
}

KSubWorld::~KSubWorld()
{
	if (m_Region)
	{
		delete [] m_Region;
		m_Region = NULL;
	}

#ifdef _SERVER
	if(m_pWeatherMgr)
	{
		delete m_pWeatherMgr;
		m_pWeatherMgr = NULL;
	}
#endif
#ifndef _SERVER
	if(m_hLoadPathGrid)
	{
		m_bStopThread = TRUE;
		WaitForSingleObject(m_hLoadPathGrid, INFINITE);
        CloseHandle(m_hLoadPathGrid);
        m_hLoadPathGrid = NULL;
	}
	m_bHavePath = FALSE;
#endif
}

#ifndef _SERVER

DWORD WINAPI LoadPathGrid(void* pParam)
{
	KSubWorld* pSW = (KSubWorld*)pParam;
	pSW->ProcLoadPathGrid();
	return 0;
}

struct AStarNode
{
    int id;
    int f; // g + h
};

struct AStarCompare
{
    bool operator()(const AStarNode& a, const AStarNode& b) const
    {
        return a.f > b.f;
    }
};

struct BlockSize { int w, h; };

BlockSize gbsizes[] = {
    {16, 32},
    //{16, 16},
    {8, 16},
    //{8, 8},
    {4, 8},
    //{4, 4},
    {2, 4},
    //{2, 2},
    {1, 2},
    {1, 1}
};
const int NUM_SIZES = sizeof(gbsizes)/sizeof(BlockSize);

inline void AddNeighbourUnique(int* arr, int& count, int maxCount, int value)
{
    for (int i = 0; i < count; ++i)
    {
        if (arr[i] == value)
            return;
    }
    if (count < maxCount)
    {
        arr[count++] = value;
    }
}

void KSubWorld::ProcLoadPathGrid()
{
	if(m_bStopThread)
		return;
	int nAllCellW = m_nGridW*m_nRegionWidth;
	int nAllCellH = m_nGridH*m_nRegionHeight;
	int nAllCell  = nAllCellW * nAllCellH;
	char	File[MAX_PATH];
	int		ObstacleInfo[REGION_GRID_WIDTH][REGION_GRID_HEIGHT];
	for(int h=0; h < m_nGridH; ++h)
	{
		if(m_bStopThread)
			return;
		for(int w=0; w < m_nGridW; ++w)
		{
			if(m_bStopThread)
				return;
			sprintf(File, "%s\\v_%03d\\%03d_Region_C.dat", m_szPathName,
						m_nRegionBeginY+h, m_nRegionBeginX+w);
			memset(ObstacleInfo, 0, sizeof(ObstacleInfo));
			KPakFile	Data;
			if (Data.Open(File))
			{
				unsigned int uMaxElemFile = 0;
				Data.Read(&uMaxElemFile, sizeof(unsigned int));
				KCombinFileSection	ElemFile[REGION_ELEM_FILE_COUNT] = { 0 };
				if (uMaxElemFile > REGION_ELEM_FILE_COUNT)
				{
					Data.Read(&ElemFile[0], sizeof(KCombinFileSection) * REGION_ELEM_FILE_COUNT);
					Data.Seek(sizeof(KCombinFileSection) * (uMaxElemFile - REGION_ELEM_FILE_COUNT), FILE_CURRENT);
				}
				else
				{
					Data.Read(&ElemFile[0], sizeof(KCombinFileSection) * uMaxElemFile);
				}
				unsigned int uOffsetAhead = sizeof(unsigned int) + sizeof(KCombinFileSection) * uMaxElemFile;
				if (ElemFile[REGION_OBSTACLE_FILE_INDEX].uLength >= sizeof(ObstacleInfo))
				{
					Data.Seek(uOffsetAhead + ElemFile[REGION_OBSTACLE_FILE_INDEX].uOffset, FILE_BEGIN);
					Data.Read((LPVOID)ObstacleInfo, sizeof(ObstacleInfo));
				}
				else
				{
					memset(ObstacleInfo, 0, sizeof(int)*REGION_GRID_WIDTH*REGION_GRID_HEIGHT);
				}
			}
			else
			{
				sprintf(File, "%s\\v_%03d\\%03d_OBSTACLE.DAT", m_szPathName,
						m_nRegionBeginY+h, m_nRegionBeginX+w);
				if (Data.Open(File))
				{
					unsigned int uSize = Data.Size();
					if(uSize >= sizeof(ObstacleInfo))
						Data.Read((LPVOID)ObstacleInfo, sizeof(ObstacleInfo));
				}
			}
			int regx = w*REGION_GRID_WIDTH;
			int regy = h*REGION_GRID_HEIGHT;
			for(int y=0;y<REGION_GRID_HEIGHT;++y)
			{
				for(int x=0;x<REGION_GRID_WIDTH;++x)
				{
					int id = (regy+y)*nAllCellW + regx+x;
					m_GridNode[id].x = m_nRegionBeginX*REGION_GRID_WIDTH + regx+x;
					m_GridNode[id].y = m_nRegionBeginY*REGION_GRID_HEIGHT + regy+y;
					int lInfo = ObstacleInfo[x][y];
					int lType = (lInfo >> 4) & 0x0000000f;
					lInfo &= 0x0000000f;
					if((lType >= Obstacle_LT && lType <= Obstacle_RB) || lInfo == 0)
					m_GridNode[id].obs = 0;
					else
					m_GridNode[id].obs = 1;
					m_GridNode[id].parentId = id;
					m_pTempCover[id] = FALSE;
				}
			}
		}
	}
	for (int si = 0; si < NUM_SIZES; ++si)
	{
		if(m_bStopThread)
			return;
		int bw = gbsizes[si].w;
		int bh = gbsizes[si].h;
	
		for (int y = 0; y + bh <= nAllCellH; ++y)
		{
			if(m_bStopThread)
				return;
			for (int x = 0; x + bw <= nAllCellW; ++x)
			{
				if(m_bStopThread)
					return;
				int id0 = y * nAllCellW + x;
	
				if (m_pTempCover[id0])
					continue;
	
				int type = m_GridNode[id0].obs;
				bool ok = true;
				int xx,yy;
				for ( yy = y; yy < y + bh && ok; ++yy)
				{
					for ( xx = x; xx < x + bw; ++xx)
					{
						int id = yy * nAllCellW + xx;
						if (m_pTempCover[id] || m_GridNode[id].obs != type)
						{
							ok = false;
							break;
						}
					}
				}
	
				if (!ok)
					continue;
	
				int parentId = id0;
				m_GridNode[parentId].parentId = parentId;
				m_GridNode[parentId].w = bw;
				m_GridNode[parentId].h = bh;
				for ( yy = y; yy < y + bh; ++yy)
				{
					for ( xx = x; xx < x + bw; ++xx)
					{
						int id = yy * nAllCellW + xx;
						m_pTempCover[id] = TRUE;
						m_GridNode[id].parentId = parentId;
					}
				}
			}
		}
	}
	//connecting neighbours
	const int DIRS[8][2] =
    {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };
	for (int parentId = 0; parentId < nAllCell; ++parentId)
	{
		if(m_bStopThread)
			return;
		VGridNode &node = m_GridNode[parentId];
	
		if (node.parentId != parentId)	//kh«ng ph¶i cell tæng
			continue;
	
		if (node.obs)
			continue;        // block nµy cã obstacle
	
		int px = parentId % nAllCellW;
		int py = parentId / nAllCellW;
		int bw = node.w;
		int bh = node.h;
	
		// m¶ng id connect tèi ®a 100
		int tmpNeighbours[100];
		int tmpCount = 0;
	
		// DuyÖt ngoµi biªn cña block
		for (int cy = py; cy < py + bh; ++cy)
		{
			if(m_bStopThread)
				return;
			for (int cx = px; cx < px + bw; ++cx)
			{
				if(m_bStopThread)
				return;
				if (!(cx == px || cx == px + bw - 1 ||
					cy == py || cy == py + bh - 1))
					continue; // bá qua ruét cña block, chØ xö lý viÒn
				// check 8 h­íng
				for (int d = 0; d < 8; ++d)
				{
					int dx = DIRS[d][0];
					int dy = DIRS[d][1];
	
					int nx = cx + dx;
					int ny = cy + dy;
					//ngoµi map th× bá qua
					if (nx < 0 || nx >= nAllCellW || ny < 0 || ny >= nAllCellH)
						continue;
	
					int nid = ny * nAllCellW + nx;
					int nParent = m_GridNode[nid].parentId;
	
					if (nParent == parentId)	//cïng block
						continue;
	
					// neighbour cã obs
					if (m_GridNode[nParent].obs)
						continue;
	
					// check 4 gãc, v­íng 1 trong 2 « th× bá qua
					if (dx != 0 && dy != 0)
					{
						int sx1 = cx + dx;	//b­íc ngang
						int sy1 = cy;
						int sx2 = cx;
						int sy2 = cy + dy;	//b­íc däc
						//nÕu 1 trong 2 « ra ngoµi map th× xem nh­ v­íng
						if (sx1 < 0 || sx1 >= nAllCellW || sy1 < 0 || sy1 >= nAllCellH)
							continue;
						if (sx2 < 0 || sx2 >= nAllCellW || sy2 < 0 || sy2 >= nAllCellH)
							continue;
	
						int sid1 = sy1 * nAllCellW + sx1;
						int sid2 = sy2 * nAllCellW + sx2;
						//nÕu 1 trong 2 « cã obs th× bÞ v­íng
						if (m_GridNode[sid1].obs || m_GridNode[sid2].obs)
							continue;
					}
					// t×m ®­îc neighbour hîp lÖ
					AddNeighbourUnique(tmpNeighbours, tmpCount, 100, nParent);
				}
			}
		}
	
		if (tmpCount == 0)
			continue;
	
		node.connStart = (int)m_vNeighbour.size();
		node.connCount = tmpCount;
	
		for (int i = 0; i < tmpCount; ++i)
		{
			VGridNeighbour nb;
			nb.toParentId = tmpNeighbours[i];
			nb.cost = g_GetDistance(node.x*32 + node.w*32/2, node.y*32 + node.h*32/2,
						m_GridNode[nb.toParentId].x*32 + m_GridNode[nb.toParentId].w*32/2,
						m_GridNode[nb.toParentId].y*32 + m_GridNode[nb.toParentId].h*32/2);
			m_vNeighbour.push_back(nb);
		}
	}
}

int KSubWorld::BlockHeuristic(int aId, int bId)
{
    const VGridNode& a = m_GridNode[aId];
    const VGridNode& b = m_GridNode[bId];

    int ax = a.x * 32 + a.w * 32 / 2;
    int ay = a.y * 32 + a.h * 32 / 2;
    int bx = b.x * 32 + b.w * 32 / 2;
    int by = b.y * 32 + b.h * 32 / 2;

    return g_GetDistance(ax, ay, bx, by);
}

int KSubWorld::FindPath_Block(int startParentId, int goalParentId)
{
    m_vRetPath.clear();

    if (startParentId == goalParentId)
    {
        m_vRetPath.push_back(startParentId);
        return 1;
    }

    int nAllCellW = m_nGridW * m_nRegionWidth;
    int nAllCellH = m_nGridH * m_nRegionHeight;
    int nodeCount = nAllCellW * nAllCellH;

    const int INF = 0x3f3f3f3f;

    std::vector<int> gCost(nodeCount, INF);
    std::vector<int> cameFrom(nodeCount, -1);
    std::vector<unsigned char> closed(nodeCount, 0);

    std::priority_queue<
        AStarNode,
        std::vector<AStarNode>,
        AStarCompare> open;

    gCost[startParentId] = 0;

    AStarNode s;
    s.id = startParentId;
    s.f  = BlockHeuristic(startParentId, goalParentId);
    open.push(s);

    bool found = false;
	int bestId   = -1;
    int bestH    = INF;
    while (!open.empty())
    {
        AStarNode cur = open.top();
        open.pop();
        int id = cur.id;
        if (closed[id])
            continue;
		
		int hToGoal = BlockHeuristic(id, goalParentId);
        if (hToGoal < bestH)
        {
            bestH  = hToGoal;
            bestId = id;
        }
		
        if (id == goalParentId)
        {
            found = true;
            break;
        }
        closed[id] = 1;
        VGridNode& node = m_GridNode[id];

        if (node.parentId != id || node.obs != 0)
            continue;

        if (node.connStart < 0 || node.connCount == 0)
            continue;

        int startIdx = node.connStart;
        int cnt      = node.connCount;

        for (int i = 0; i < cnt; ++i)
        {
            int nb = m_vNeighbour[startIdx + i].toParentId;

            VGridNode& nbNode = m_GridNode[nb];
            if (nbNode.parentId != nb || nbNode.obs != 0)
                continue;

            if (closed[nb])
                continue;

            int newG = gCost[id] + m_vNeighbour[startIdx + i].cost;
            if (newG < gCost[nb])
            {
                gCost[nb]    = newG;
                cameFrom[nb] = id;

                int h = BlockHeuristic(nb, goalParentId);

                AStarNode nn;
                nn.id = nb;
                nn.f  = newG + h;
                open.push(nn);
            }
        }
    }

    if (!found)
    {
        // goal unreachable
        if (bestId == -1)
            return 0;

        // reconstruct path
        int curId = bestId;
        while (curId != -1)
        {
            m_vRetPath.push_back(curId);
            curId = cameFrom[curId];
        }
        std::reverse(m_vRetPath.begin(), m_vRetPath.end());
        return 2;
    }
	
    int curId = goalParentId;
    while (curId != -1)
    {
        m_vRetPath.push_back(curId);
        curId = cameFrom[curId];
    }

    std::reverse(m_vRetPath.begin(), m_vRetPath.end());
    return 1;
}

int KSubWorld::FindPath_NpcObs(int startParentId, int goalParentId)
{
    m_vRetPath.clear();

    if (startParentId == goalParentId)
    {
        m_vRetPath.push_back(startParentId);
        return 1;
    }

    int nAllCellW = m_nGridW * m_nRegionWidth;
    int nAllCellH = m_nGridH * m_nRegionHeight;
    int nodeCount = nAllCellW * nAllCellH;

    const int INF = 0x3f3f3f3f;

    std::vector<int> gCost(nodeCount, INF);
    std::vector<int> cameFrom(nodeCount, -1);
    std::vector<unsigned char> closed(nodeCount, 0);

    std::priority_queue<
        AStarNode,
        std::vector<AStarNode>,
        AStarCompare> open;

    gCost[startParentId] = 0;

    AStarNode s;
    s.id = startParentId;
    s.f  = BlockHeuristic(startParentId, goalParentId);
    open.push(s);

    bool found = false;
	int bestId   = -1;
    int bestH    = INF;
    while (!open.empty())
    {
        AStarNode cur = open.top();
        open.pop();
        int id = cur.id;
        if (closed[id])
            continue;
		
		int hToGoal = BlockHeuristic(id, goalParentId);
        if (hToGoal < bestH)
        {
            bestH  = hToGoal;
            bestId = id;
        }
		
        if (id == goalParentId)
        {
            found = true;
            break;
        }

        closed[id] = 1;
        VGridNode& node = m_GridNode[id];

        if (node.parentId != id || node.obs != 0)
            continue;

        if (node.connStart < 0 || node.connCount == 0)
            continue;

        int startIdx = node.connStart;
        int cnt      = node.connCount;

        for (int i = 0; i < cnt; ++i)
        {
            int nb = m_vNeighbour[startIdx + i].toParentId;

            VGridNode& nbNode = m_GridNode[nb];
            if (nbNode.parentId != nb || nbNode.obs != 0)
                continue;

            if (closed[nb])
                continue;
			//reject cells with npc obstacles
			if(nbNode.w < 4 && nbNode.h < 8)
			{
				int nRegion, nMapX, nMapY, nOffX, nOffY;
				bool bObsNpc = true;
				for(int yy = 0;yy < (int)nbNode.h; ++yy)
				{
					if(!bObsNpc)
						break;
					for(int xx = 0;xx < (int)nbNode.w; ++xx)
					{
						Mps2Map((nbNode.x + xx)*32, (nbNode.y + yy)*32, &nRegion, &nMapX, &nMapY, &nOffX, &nOffY);
						if(nRegion >= 0 && m_Region[nRegion].GetRef(nMapX, nMapY, obj_npc) <= 0)
						{
							bObsNpc = false;
							break;
						}
					}
				}
				if(bObsNpc)
					continue;
			}
            int newG = gCost[id] + m_vNeighbour[startIdx + i].cost;
            if (newG < gCost[nb])
            {
                gCost[nb]    = newG;
                cameFrom[nb] = id;

                int h = BlockHeuristic(nb, goalParentId);

                AStarNode nn;
                nn.id = nb;
                nn.f  = newG + h;
                open.push(nn);
            }
        }
    }

    if (!found)
    {
        // goal unreachable
        if (bestId == -1)
            return 0;

        // reconstruct path
        int curId = bestId;
        while (curId != -1)
        {
            m_vRetPath.push_back(curId);
            curId = cameFrom[curId];
        }
        std::reverse(m_vRetPath.begin(), m_vRetPath.end());
        return 2;
    }

    int curId = goalParentId;
    while (curId != -1)
    {
        m_vRetPath.push_back(curId);
        curId = cameFrom[curId];
    }

    std::reverse(m_vRetPath.begin(), m_vRetPath.end());
    return 1;
}

int KSubWorld::FindFreeBlockAround(int nMainId, int nNearX, int nNearY)
{
    int nAllCellW = m_nGridW * m_nRegionWidth;
    int nAllCellH = m_nGridH * m_nRegionHeight;

    VGridNode& b = m_GridNode[nMainId];
	//täa ®é cña block gèc
    int bx0 = nMainId % nAllCellW;
    int by0 = nMainId / nAllCellW;
    int bx1 = bx0 + b.w;  // exclusive
    int by1 = by0 + b.h;  // exclusive
	//ngoµi map -> thôt vµo trong
    int sx0 = bx0 - 1; if (sx0 < 0)           sx0 = 0;
    int sy0 = by0 - 1; if (sy0 < 0)           sy0 = 0;
    int sx1 = bx1;     if (sx1 >= nAllCellW) sx1 = nAllCellW - 1;
    int sy1 = by1;     if (sy1 >= nAllCellH) sy1 = nAllCellH - 1;

    int bestId   = -1;
    int bestDist = 0x7fffffff;
	//vÞ trÝ thùc cña block gèc
    //int cx = b.x * 32 + b.w * 32 / 2;
    //int cy = b.y * 32 + b.h * 32 / 2;

    for (int y = sy0; y <= sy1; ++y)
    {
        for (int x = sx0; x <= sx1; ++x)
        {
            if (x >= bx0 && x < bx1 &&
                y >= by0 && y < by1)	//bªn trong block gèc th× bá qua
            {
                continue;
            }

            int pid = y * nAllCellW + x;
            if (m_GridNode[pid].obs)
                continue;
            pid = m_GridNode[pid].parentId;
            if (m_GridNode[pid].obs)
                continue;

            VGridNode& nb = m_GridNode[pid];
            int px = nb.x * 32 + nb.w * 32 / 2;
            int py = nb.y * 32 + nb.h * 32 / 2;

            int d = g_GetDistance(px, py, nNearX, nNearY);

            if (d < bestDist)
            {
                bestDist = d;
                bestId   = pid;
            }
        }
    }

    return bestId;
}

int KSubWorld::FindPath(int nX, int nY, bool bCheckNpc)
{
	StopPath();
	if(!m_bHavePath || !m_nGridTotal || nX <= 0 || nY <= 0)
		return -1;
	if(m_hLoadPathGrid && WaitForSingleObject(m_hLoadPathGrid, 0) == WAIT_TIMEOUT)
		return -2;
	if(!Player[CLIENT_PLAYER_INDEX].m_nIndex)
		return -1;
	int nWidth = m_nGridW*REGION_GRID_WIDTH;
	int nHeight = m_nGridH*REGION_GRID_HEIGHT;
	int cx = nX/32-m_nRegionBeginX*REGION_GRID_WIDTH;
	int cy = nY/32-m_nRegionBeginY*REGION_GRID_HEIGHT;
	if(cx < 0)
		cx = 0;
	else if(cx >= nWidth)
		cx = nWidth - 1;
	if(cy < 0)
		cy = 0;
	else if(cy >= nHeight)
		cy = nHeight - 1;
	
	int tid = cy*nWidth + cx;
	if(tid < 0 || tid >= m_nGridTotal*512)
		return -1;
	tid = m_GridNode[tid].parentId;
	int nx, ny;
	Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].GetMpsPos(&nx, &ny);
	int cnx = nx/32-m_nRegionBeginX*REGION_GRID_WIDTH;
	int cny = ny/32-m_nRegionBeginY*REGION_GRID_HEIGHT;
	if(cnx < 0)
		cnx = 0;
	else if(cnx >= nWidth)
		cnx = nWidth - 1;
	if(cny < 0)
		cny = 0;
	else if(cny >= nHeight)
		cny = nHeight - 1;
	
	int sid = cny*nWidth + cnx;
	if(sid < 0 || sid >= m_nGridTotal*512)
		return -1;
	sid = m_GridNode[sid].parentId;
	if(m_GridNode[sid].obs)
	{
		sid = FindFreeBlockAround(sid, nx, ny);
		if(sid < 0)
			return -1;
	}
	int nRet;
	if(bCheckNpc)
		nRet = FindPath_NpcObs(sid, tid);
	else
		nRet = FindPath_Block(sid, tid);
	if(nRet <= 0)
		StopPath();
	else
	{
		if(nRet == 2)
		{
			tid = m_vRetPath[m_vRetPath.size()-1];
			m_nTargetX = m_GridNode[tid].x*32 + m_GridNode[tid].w*32/2;
			m_nTargetY = m_GridNode[tid].y*32 + m_GridNode[tid].h*32/2;
		}
		else
		{
			m_nTargetX = nX;
			m_nTargetY = nY;
		}
	}
	return nRet;
}

#endif

int KSubWorld::FindRegion(int nRegion)
{
	for (int i = 0; i < m_nTotalRegion; i++)
	{
		if (m_Region[i].m_RegionID == nRegion)
			return i;
	}
	return -1;
}

#ifndef _SERVER
int KSubWorld::FindFreeRegion(int nX, int nY)
{
	int i;
	if (nX == 0 && nY == 0)
	{
		for ( i = 0; i < m_nTotalRegion; i++)
		{
			if (m_Region[i].m_RegionID == -1)
			{
				m_bCheckedRegion[i] = TRUE;
				return i;
			}
		}
	}
	else
	{
		for ( i = 0; i < m_nTotalRegion; i++)
		{
			if(m_bCheckedRegion[i])
				continue;
			if (m_Region[i].m_RegionID == -1)
			{
				m_bCheckedRegion[i] = TRUE;
				return i;
			}
			int nRegoinX = LOWORD(m_Region[i].m_RegionID);
			int nRegoinY = HIWORD(m_Region[i].m_RegionID);
			if ((nX - nRegoinX) * (nX - nRegoinX) + (nY - nRegoinY) * (nY - nRegoinY) > 8)	//far > 2 cell
			{
				m_bCheckedRegion[i] = TRUE;
				return i;
			}
		}
		for ( i = 0; i < m_nTotalRegion; i++)
		{
			if(m_bCheckedRegion[i])
				continue;
			int nRegoinX = LOWORD(m_Region[i].m_RegionID);
			int nRegoinY = HIWORD(m_Region[i].m_RegionID);
			if ((nX - nRegoinX) * (nX - nRegoinX) + (nY - nRegoinY) * (nY - nRegoinY) > 2)	//far > 1 cell
			{
				m_bCheckedRegion[i] = TRUE;
				return i;
			}
		}
	}
	return -1;
}
#endif

extern int nActiveRegionCount;

void KSubWorld::Activate()
{
	if (m_SubWorldID == -1)
		return;
	m_dwCurrentTime ++;

#ifndef _SERVER
	g_ScenePlace.SetCurrentTime(m_dwCurrentTime);
	NpcSet.ClearActivateFlagOfAllNpc();
#endif

	for (int i = 0; i < m_nTotalRegion; i++)
	{
		if (m_Region[i].IsActive())
		{
//			g_DebugLog("[Region]%d Activating", i);
			m_Region[i].Activate();
			nActiveRegionCount++;
		}
	}

#ifdef _SERVER
	KIndexNode* pNode = (KIndexNode *)m_NoneRegionNpcList.GetHead();
	while(pNode)
	{
		Npc[pNode->m_nIndex].Activate();
		pNode = (KIndexNode *)pNode->GetNext();
	}

	if(m_pWeatherMgr)
	{
		int nWeather = m_pWeatherMgr->Activate();
		if(m_nWeather != nWeather)
		{
			m_nWeather = nWeather;
			SYNC_WEATHER weatherMsg;
			weatherMsg.ProtocolType = s2c_changeweather;
			weatherMsg.WeatherID = (BYTE)m_nWeather;
			BroadCast((const char*)&weatherMsg, sizeof(SYNC_WEATHER));
		}
	}
	
		m_MissionArray.Activate();

#endif
#ifndef _SERVER
	if(m_nTargetX && m_nTargetY && m_vRetPath.size() > 0)
	{
		int nNpcIdx = Player[CLIENT_PLAYER_INDEX].m_nIndex;
		if(!nNpcIdx)
		{
			StopPath();
			return;
		}
		int nX, nY;
		Npc[nNpcIdx].GetMpsPos(&nX, &nY);
		int nStep = (int)m_vRetPath.size();
		if(m_nCurStep == 0)
		{
			++m_nCurStep;
			if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
			{
				if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 6)
				m_uStepDelayTime = timeGetTime() + 13000;
				else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 10)
				m_uStepDelayTime = timeGetTime() + 8000;
				else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 20)
				m_uStepDelayTime = timeGetTime() + 5000;
				else
				m_uStepDelayTime = timeGetTime() + 3500;
			}
			else
			{
				if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
				m_uStepDelayTime = timeGetTime() + 13000;
				else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
				m_uStepDelayTime = timeGetTime() + 8000;
				else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
				m_uStepDelayTime = timeGetTime() + 5000;
				else
				m_uStepDelayTime = timeGetTime() + 3500;
			}
		}
		if(m_nCurStep >= nStep-1)
		{
			if(g_GetDistance(nX, nY, m_nTargetX, m_nTargetY) < 64)
				return;
			if(m_nCurStep > nStep-1)
			{
				if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
				{
					Npc[nNpcIdx].SendCommand(do_walk, m_nTargetX, m_nTargetY);
					SendClientCmdWalk(m_nTargetX, m_nTargetY);
				}
				else
				{
					Npc[nNpcIdx].SendCommand(do_run, m_nTargetX, m_nTargetY);
					SendClientCmdRun(m_nTargetX, m_nTargetY);
				}
			}
			else
			{
				int nBlockid = m_vRetPath[m_nCurStep];
				if(m_GridNode[nBlockid].w < 4 || m_GridNode[nBlockid].h < 4)
				{
					++m_nCurStep;
					if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
					{
						if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 6)
						m_uStepDelayTime = timeGetTime() + 13000;
						else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 10)
						m_uStepDelayTime = timeGetTime() + 8000;
						else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 20)
						m_uStepDelayTime = timeGetTime() + 5000;
						else
						m_uStepDelayTime = timeGetTime() + 3500;
					}
					else
					{
						if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
						m_uStepDelayTime = timeGetTime() + 13000;
						else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
						m_uStepDelayTime = timeGetTime() + 8000;
						else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
						m_uStepDelayTime = timeGetTime() + 5000;
						else
						m_uStepDelayTime = timeGetTime() + 3500;
					}
					if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
					{
						Npc[nNpcIdx].SendCommand(do_walk, m_nTargetX, m_nTargetY);
						SendClientCmdWalk(m_nTargetX, m_nTargetY);
					}
					else
					{
						Npc[nNpcIdx].SendCommand(do_run, m_nTargetX, m_nTargetY);
						SendClientCmdRun(m_nTargetX, m_nTargetY);
					}
					return;
				}
				int bX = m_GridNode[nBlockid].x*32;
				int bY = m_GridNode[nBlockid].y*32;
				int eX = m_GridNode[nBlockid].x*32 + m_GridNode[nBlockid].w*32;
				int eY = m_GridNode[nBlockid].y*32 + m_GridNode[nBlockid].h*32;
				if(nX >= bX && nX <= eX && nY >= bY && nY <= eY)
				{
					++m_nCurStep;
					if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
					{
						if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 6)
						m_uStepDelayTime = timeGetTime() + 13000;
						else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 10)
						m_uStepDelayTime = timeGetTime() + 8000;
						else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 20)
						m_uStepDelayTime = timeGetTime() + 5000;
						else
						m_uStepDelayTime = timeGetTime() + 3500;
					}
					else
					{
						if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
						m_uStepDelayTime = timeGetTime() + 13000;
						else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
						m_uStepDelayTime = timeGetTime() + 8000;
						else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
						m_uStepDelayTime = timeGetTime() + 5000;
						else
						m_uStepDelayTime = timeGetTime() + 3500;
					}
				}
				int dX = m_GridNode[nBlockid].x*32 + m_GridNode[nBlockid].w*32/2;
				int dY = m_GridNode[nBlockid].y*32 + m_GridNode[nBlockid].h*32/2;
				if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
				{
					Npc[nNpcIdx].SendCommand(do_walk, dX, dY);
					SendClientCmdWalk(dX, dY);
				}
				else
				{
					Npc[nNpcIdx].SendCommand(do_run, dX, dY);
					SendClientCmdRun(dX, dY);
				}
			}
		}
		else
		{
			int nBlockid = m_vRetPath[m_nCurStep];
			int dX = m_GridNode[nBlockid].x*32 + m_GridNode[nBlockid].w*32/2;
			int dY = m_GridNode[nBlockid].y*32 + m_GridNode[nBlockid].h*32/2;
			if(g_GetDistance(nX, nY, dX, dY) < 64)
			{
				++m_nCurStep;
				if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
				{
					if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 6)
					m_uStepDelayTime = timeGetTime() + 13000;
					else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 10)
					m_uStepDelayTime = timeGetTime() + 8000;
					else if(Npc[nNpcIdx].m_CurrentWalkSpeed <= 20)
					m_uStepDelayTime = timeGetTime() + 5000;
					else
					m_uStepDelayTime = timeGetTime() + 3500;
				}
				else
				{
					if(Npc[nNpcIdx].m_CurrentRunSpeed <= 6)
					m_uStepDelayTime = timeGetTime() + 13000;
					else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 10)
					m_uStepDelayTime = timeGetTime() + 8000;
					else if(Npc[nNpcIdx].m_CurrentRunSpeed <= 20)
					m_uStepDelayTime = timeGetTime() + 5000;
					else
					m_uStepDelayTime = timeGetTime() + 3500;
				}
				if(m_nCurStep < nStep)
				{
					nBlockid = m_vRetPath[m_nCurStep];
					dX = m_GridNode[nBlockid].x*32 + m_GridNode[nBlockid].w*32/2;
					dY = m_GridNode[nBlockid].y*32 + m_GridNode[nBlockid].h*32/2;
				}
				else
				{
					dX = m_nTargetX;
					dY = m_nTargetY;
				}
			}
			if (!Player[CLIENT_PLAYER_INDEX].m_RunStatus)
			{
				Npc[nNpcIdx].SendCommand(do_walk, dX, dY);
				SendClientCmdWalk(dX, dY);
			}
			else
			{
				Npc[nNpcIdx].SendCommand(do_run, dX, dY);
				SendClientCmdRun(dX, dY);
			}
		}
		if(m_uStepDelayTime < timeGetTime())
		{
			FindPath(m_nTargetX, m_nTargetY, true);
		}
	}
#endif
}

int KSubWorld::GetDistance(int nRx1, int nRy1, int nRx2, int nRy2)
{
	return (int)sqrt((nRx1 - nRx2) * (nRx1 - nRx2) + (nRy1 - nRy2) * (nRy1 - nRy2));
}

void KSubWorld::Map2Mps(int nR, int nX, int nY, int nDx, int nDy, int *nRx, int *nRy)
{
/*
#ifdef TOOLVERION
	*nRx = nX;
	*nRy = nY;
	return;
#endif
*/
//#ifndef _SERVER
//	_ASSERT(nR >= 0 && nR < 9);
//#endif
	//_ASSERT(nR >= 0);

	if (nR < 0 || nR >= m_nTotalRegion)
	{
		*nRx = 0;
		*nRy = 0;
		return;
	}

	int x, y;
	
	x = m_Region[nR].m_nRegionX;
	y = m_Region[nR].m_nRegionY;
	
	x += nX * m_nCellWidth;
	y += nY * m_nCellHeight;
	
	x += (nDx >> 10);
	y += (nDy >> 10);
	
	*nRx = x;
	*nRy = y;
}

void KSubWorld::Map2Mps(int nRx, int nRy, int nX, int nY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nRx * REGION_GRID_WIDTH + nX) * defLOGIC_CELL_WIDTH + (nDx >> 10);
	*pnY = (nRy * REGION_GRID_HEIGHT + nY) * defLOGIC_CELL_HEIGHT + (nDy >> 10);
}

void KSubWorld::Mps2Map(int Rx, int Ry, int * nR, int * nX, int * nY, int *nDx, int * nDy)
{
	if (m_nCellWidth == 0 || m_nCellHeight == 0 || m_nRegionWidth == 0 || m_nRegionHeight == 0)
		return;
	int x = Rx / (m_nRegionWidth * m_nCellWidth);
	int	y = Ry / (m_nRegionHeight * m_nCellHeight);

	*nX = 0;
	*nY = 0;
	*nDx = 0;
	*nDy = 0;
#ifdef _SERVER
	// ·Ç·¨µÄ×ø±ê
	if (x >= m_nWorldRegionWidth + m_nRegionBeginX || y >= m_nWorldRegionHeight + m_nRegionBeginY || x < m_nRegionBeginX || y < m_nRegionBeginY)
	{
		*nR = -1;
		return;
	}
#endif
	// ·Ç·¨µÄ×ø±ê
#ifdef _SERVER
	*nR = GetRegionIndex(MAKELONG(x, y));
#else
	int nRegionID = MAKELONG(x, y);
	*nR = FindRegion(nRegionID);
	if (*nR == -1)
		return;
#endif
	if (*nR >= m_nTotalRegion)
	{
		*nR = -1;
		return;
	}

	x = Rx - m_Region[*nR].m_nRegionX;
	y = Ry - m_Region[*nR].m_nRegionY;

	*nX = x / m_nCellWidth;
	*nY = y / m_nCellHeight;

	*nDx = (x - *nX * m_nCellWidth) << 10;
	*nDy = (y - *nY * m_nCellHeight) << 10;
}

BYTE	KSubWorld::TestBarrier(int nMpsX, int nMpsY)
{
	if (m_nCellWidth == 0 || m_nCellHeight == 0 || m_nRegionWidth == 0 || m_nRegionHeight == 0)
		return 0xff;

	int x = nMpsX / (m_nRegionWidth * m_nCellWidth);
	int	y = nMpsY / (m_nRegionHeight * m_nCellHeight);
#ifdef _SERVER
	// ·Ç·¨µÄ×ø±ê
	if (x >= m_nWorldRegionWidth + m_nRegionBeginX || y >= m_nWorldRegionHeight + m_nRegionBeginY || x < m_nRegionBeginX || y < m_nRegionBeginY)
		return 0xff;
	int nRegion = GetRegionIndex(MAKELONG(x, y));
#else
	int nRegion = FindRegion(MAKELONG(x, y));
	if (nRegion == -1)
		return 0xff;
#endif
	if (nRegion >= m_nTotalRegion)
		return 0xff;

	x = nMpsX - m_Region[nRegion].m_nRegionX;
	y = nMpsY - m_Region[nRegion].m_nRegionY;

	int nCellX = x / m_nCellWidth;
	int nCellY = y / m_nCellHeight;

	int nOffX = x - nCellX * m_nCellWidth;
	int nOffY = y - nCellY * m_nCellHeight;

#ifndef _SERVER
	BYTE bRet = (BYTE)g_ScenePlace.GetObstacleInfo(nMpsX, nMpsY);
	if (bRet != Obstacle_NULL)
		return bRet;
	return m_Region[nRegion].GetBarrier(nCellX, nCellY, nOffX, nOffY);
#endif
#ifdef _SERVER
	return m_Region[nRegion].GetBarrier(nCellX, nCellY, nOffX, nOffY);
#endif
}

BYTE KSubWorld::TestBarrier(int nRegion, int nMapX, int nMapY, int nDx, int nDy, int nChangeX, int nChangeY)
{
	int nOldMapX = nMapX;
	int nOldMapY = nMapY;
	int nOldRegion = nRegion;

	nDx += nChangeX;
	nDy += nChangeY;

	if (nDx < 0)
	{
		nDx += (m_nCellWidth << 10);
		nMapX--;
	}
	else if (nDx >= (m_nCellWidth << 10))
	{
		nDx -= (m_nCellWidth << 10);
		nMapX++;
	}
	
	if (nDy < 0)
	{
		nDy += (m_nCellHeight << 10);
		nMapY--;
	}
	else if (nDy >= (m_nCellHeight << 10))
	{
		nDy -= (m_nCellHeight << 10);
		nMapY++;
	}
	
	if (nMapX < 0)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_LEFT] == -1)
			return 0xff;

		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_LEFT];
		nMapX += m_nRegionWidth;
	}
	else if (nMapX >= m_nRegionWidth)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_RIGHT] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_RIGHT];
		nMapX -= m_nRegionWidth;
	}
	
	if (nMapY < 0)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_UP] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_UP];;
		nMapY += m_nRegionHeight;
	}
	else if (nMapY >= m_nRegionHeight)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_DOWN] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_DOWN];
		nMapY -= m_nRegionHeight;
	}

	int nXf, nYf;
	nXf = (nDx >> 10);
	nYf = (nDy >> 10);

#ifdef TOOLVERSION
	return Obstacle_NULL;
#endif
#ifndef _SERVER

	int nMpsX, nMpsY;
	Map2Mps(nRegion, nMapX, nMapY, nDx, nDy, &nMpsX, &nMpsY);
	BYTE bRet = (BYTE)g_ScenePlace.GetObstacleInfo(nMpsX, nMpsY);
	if (bRet != Obstacle_NULL)
		return bRet;
//	if (nMapX == nOldMapX && nMapY == nOldMapY && nRegion == nOldRegion)
//		return Obstacle_NULL;

	return m_Region[nRegion].GetBarrier(nMapX, nMapY, nXf, nYf);
#else
//	if (nMapX == nOldMapX && nMapY == nOldMapY && nRegion == nOldRegion)
//		return 0;
	return m_Region[nRegion].GetBarrier(nMapX, nMapY, nXf, nYf);
#endif
}

BYTE KSubWorld::TestBarrierMin(int nRegion, int nMapX, int nMapY, int nDx, int nDy, int nChangeX, int nChangeY)
{
	int nOldMapX = nMapX;
	int nOldMapY = nMapY;
	int nOldRegion = nRegion;

	nDx += nChangeX;
	nDy += nChangeY;

	if (nDx < 0)
	{
		nDx += (m_nCellWidth << 10);
		nMapX--;
	}
	else if (nDx >= (m_nCellWidth << 10))
	{
		nDx -= (m_nCellWidth << 10);
		nMapX++;
	}
	
	if (nDy < 0)
	{
		nDy += (m_nCellHeight << 10);
		nMapY--;
	}
	else if (nDy >= (m_nCellHeight << 10))
	{
		nDy -= (m_nCellHeight << 10);
		nMapY++;
	}
	
	if (nMapX < 0)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_LEFT] == -1)
			return 0xff;

		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_LEFT];
		nMapX += m_nRegionWidth;
	}
	else if (nMapX >= m_nRegionWidth)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_RIGHT] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_RIGHT];
		nMapX -= m_nRegionWidth;
	}
	
	if (nMapY < 0)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_UP] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_UP];;
		nMapY += m_nRegionHeight;
	}
	else if (nMapY >= m_nRegionHeight)
	{
		if (m_Region[nRegion].m_nConnectRegion[DIR_DOWN] == -1)
			return 0xff;
		nRegion = m_Region[nRegion].m_nConnectRegion[DIR_DOWN];
		nMapY -= m_nRegionHeight;
	}

#ifndef _SERVER

	int nMpsX, nMpsY;
	Map2Mps(nRegion, nMapX, nMapY, nDx, nDy, &nMpsX, &nMpsY);
	BYTE bRet = (BYTE)g_ScenePlace.GetObstacleInfoMin(nMpsX, nMpsY, nDx & 0x000003ff, nDy & 0x000003ff);
	if (bRet != Obstacle_NULL)
		return bRet;

	if (nMapX == nOldMapX && nMapY == nOldMapY && nRegion == nOldRegion)
		return Obstacle_NULL;
	return m_Region[nRegion].GetBarrierMin(nMapX, nMapY, nDx, nDy, TRUE);

#else

	if (nMapX == nOldMapX && nMapY == nOldMapY && nRegion == nOldRegion)
	{
		return m_Region[nRegion].GetBarrierMin(nMapX, nMapY, nDx, nDy, FALSE);
	}

	return m_Region[nRegion].GetBarrierMin(nMapX, nMapY, nDx, nDy, TRUE);
#endif
}

DWORD KSubWorld::GetTrap(int nMpsX, int nMpsY)
{
	int nRegion, nMapX, nMapY, nOffX, nOffY;
	Mps2Map(nMpsX, nMpsY, &nRegion, &nMapX, &nMapY, &nOffX, &nOffY);
	if (nRegion == -1)
		return 0;
	return m_Region[nRegion].GetTrap(nMapX, nMapY);
}

BYTE KSubWorld::GetBarrier(int nMpsX, int nMpsY)
{
#ifdef _SERVER
	int nRegion, nMapX, nMapY, nOffX, nOffY;
	Mps2Map(nMpsX, nMpsY, &nRegion, &nMapX, &nMapY, &nOffX, &nOffY);
	
	if (nRegion == -1)
		return 0xff;
	return m_Region[nRegion].GetBarrier(nMapX, nMapY, (nOffX >> 10), (nOffY) >> 10);
#else
	return (BYTE)g_ScenePlace.GetObstacleInfo(nMpsX, nMpsY);
#endif
}

#ifdef _SERVER
BOOL KSubWorld::LoadMap(int nId)
{
	KIniFile	IniFile;
	char		szPathName[FILE_NAME_LENGTH];
	char		szFileName[FILE_NAME_LENGTH];
	char		szKeyName[FILE_NAME_LENGTH];

	m_SubWorldID = nId;
	g_SetFilePath("\\settings");
	IniFile.Load("MapList.ini");
	sprintf(szKeyName, "%d", nId);
	IniFile.GetString("List", szKeyName, "", szPathName, sizeof(szPathName));
	
	sprintf(szKeyName, "%d_NpcSeriesMetal", nId);
	IniFile.GetInteger("List", szKeyName, 0, &m_nSeriesRate[0]);
	sprintf(szKeyName, "%d_NpcSeriesWood", nId);
	IniFile.GetInteger("List", szKeyName, 0, &m_nSeriesRate[1]);
	sprintf(szKeyName, "%d_NpcSeriesWater", nId);
	IniFile.GetInteger("List", szKeyName, 0, &m_nSeriesRate[2]);
	sprintf(szKeyName, "%d_NpcSeriesFire", nId);
	IniFile.GetInteger("List", szKeyName, 0, &m_nSeriesRate[3]);
	sprintf(szKeyName, "%d_NpcSeriesEarth", nId);
	IniFile.GetInteger("List", szKeyName, 0, &m_nSeriesRate[4]);
	
	g_SetFilePath("\\maps");
	sprintf(szFileName, "%s.wor", szPathName);
	if (!IniFile.Load(szFileName))
		return FALSE;
	
	int nIsInDoor;
	IniFile.GetInteger("MAIN", "IsInDoor", 0, &nIsInDoor);
	if(!nIsInDoor)
	{
		m_pWeatherMgr = new KWeatherMgr;
		m_pWeatherMgr->InitFromIni(IniFile);
	}

	m_nRegionWidth = 512 / 32;
	m_nRegionHeight = 1024 / 32;

	RECT	sRect;
	IniFile.GetRect("MAIN", "rect", &sRect);
	m_nRegionBeginX = sRect.left;
	m_nRegionBeginY = sRect.top;
	m_nWorldRegionWidth = sRect.right - sRect.left + 1;
	m_nWorldRegionHeight = sRect.bottom - sRect.top + 1;
	m_nTotalRegion = m_nWorldRegionWidth * m_nWorldRegionHeight;
	
	m_nCellWidth = defLOGIC_CELL_WIDTH;
	m_nCellHeight = defLOGIC_CELL_HEIGHT;

	if (m_nTotalRegion <= 0 || m_nWorldRegionWidth <= 0 || m_nWorldRegionHeight <= 0)
		return FALSE;

	m_Region = new KRegion[m_nTotalRegion];

	char	szPath[MAX_PATH];
	sprintf(szPath, "%s", szPathName);
	int		nX, nY, nIdx;

	// ¼ÓÔØµØÍ¼ÕÏ°­£¬²¢Á¬½Ó¸÷Region
	for (nY = 0; nY < m_nWorldRegionHeight; nY++)
	{
		for (nX = 0; nX < m_nWorldRegionWidth; nX++)
		{
			char	szRegionPath[128];

			g_SetFilePath(szPath);
			nIdx = nY * m_nWorldRegionWidth + nX;
			if (m_Region[nIdx].Load(nX + m_nRegionBeginX, nY + m_nRegionBeginY))
			{
				m_Region[nIdx].Init(m_nRegionWidth, m_nRegionHeight);
				m_Region[nIdx].m_nIndex = nIdx;
			}
			for (int i = 0; i < 8; i++)
			{
				short nTmpX, nTmpY;
				nTmpX = (short)LOWORD(m_Region[nIdx].m_nConRegionID[i]) - m_nRegionBeginX;
				nTmpY = (short)HIWORD(m_Region[nIdx].m_nConRegionID[i]) - m_nRegionBeginY;
				if (nTmpX < 0 || nTmpY < 0 || nTmpX >= m_nWorldRegionWidth || nTmpY >= m_nWorldRegionHeight)
				{
					m_Region[nIdx].m_nConnectRegion[i] = -1;
					continue;
				}
				int nConIdx = nTmpY * m_nWorldRegionWidth + nTmpX;
				m_Region[nIdx].m_nConnectRegion[i] = nConIdx;
			}
		}
	}

	//sprintf(szPath, "\\maps\\%s", szPathName);
	for (nY = 0; nY < m_nWorldRegionHeight; nY++)
	{
		for (nX = 0; nX < m_nWorldRegionWidth; nX++)
		{
			g_SetFilePath(szPath);
			m_Region[nY * m_nWorldRegionWidth + nX].LoadObject(m_nIndex,nX + m_nRegionBeginX, nY + m_nRegionBeginY);
		}
	}

	return TRUE;
}
#endif

#ifndef _SERVER
BOOL KSubWorld::LoadMap(int nId, int nRegion)
{
	static int	nXOff[14] = {0, -1, -1, -1, 0,  1, 1, 1, -2,-2, -2, 2, 2, 2};
	static int	nYOff[14] = {1,  1,  0, -1,-1, -1, 0, 1,  1, 0, -1,-1, 0, 1};
	//KIniFile	IniFile;

	if (!m_Region)
	{
		m_Region = new KRegion[MAX_REGION];
	}
	bool bLoadNew = false;
	if (nId != m_SubWorldID)
	{
		StopPath();
		m_uPaintTime = timeGetTime() + 2000;
		bLoadNew = true;
		SubWorld[0].Close();
		g_ScenePlace.ClosePlace();
		// NpcSet.RemoveAll(Player[CLIENT_PLAYER_INDEX].m_nIndex); -- later finish it. spe

		//char	szKeyName[32], szPathName[FILE_NAME_LENGTH];

		//g_SetFilePath("\\settings");
		//IniFile.Load("MapList.ini");
		//sprintf(szKeyName, "%d", nId);
		//IniFile.GetString("List", szKeyName, "", szPathName, sizeof(szPathName));
		//sprintf(m_szMapPath, "\\maps\\%s", szPathName);

		g_ScenePlace.OpenPlace(nId);
		m_SubWorldID = nId;
		m_nRegionWidth = KScenePlaceRegionC::RWPP_AREGION_WIDTH / 32;
		m_nRegionHeight = KScenePlaceRegionC::RWPP_AREGION_HEIGHT / 32;
		m_nCellWidth = 32;
		m_nCellHeight = 32;
		Npc[Player[CLIENT_PLAYER_INDEX].m_nIndex].m_RegionIndex = -1;
		//pathgrid
		m_nGridTotal = 0;
		KIniFile	IniFile;
		char		szFileName[FILE_NAME_LENGTH];
		char		szKeyName[FILE_NAME_LENGTH];
		
		g_SetFilePath("\\settings");
		IniFile.Load("MapList.ini");
		sprintf(szKeyName, "%d", nId);
		IniFile.GetString("List", szKeyName, "", m_szPathName, sizeof(m_szPathName));
				
		g_SetFilePath("\\maps");
		sprintf(szFileName, "%s.wor", m_szPathName);
		if (IniFile.Load(szFileName))
		{
			RECT	sRect;
			IniFile.GetRect("MAIN", "rect", &sRect);
			m_nRegionBeginX = sRect.left;
			m_nRegionBeginY = sRect.top;
			m_nGridW = sRect.right - sRect.left + 1;
			m_nGridH = sRect.bottom - sRect.top + 1;
			m_nGridTotal = m_nGridW * m_nGridH;
		}
	}
	int nX = LOWORD(nRegion);
	int nY = HIWORD(nRegion);

	int nIdx = FindRegion(nRegion);

	if (nIdx < 0)
	{
		nIdx = m_ClientRegionIdx;

		if (m_Region[nIdx].Load(nX, nY))
		{
			m_Region[nIdx].m_nIndex = nIdx;
			m_Region[nIdx].Init(m_nRegionWidth, m_nRegionHeight);
			//m_Region[nIdx].LoadObject(0, nX, nY, m_szMapPath);
		}
	}
	g_ScenePlace.SetFocusPosition(m_Region[nIdx].m_nRegionX, m_Region[nIdx].m_nRegionY, 0);
	m_ClientRegionIdx = nIdx;
	memset(&m_bCheckedRegion[0], 0, sizeof(BOOL)*MAX_REGION);
	m_bCheckedRegion[nIdx] = TRUE;
	int i;
	for ( i = 0; i < 8; i++)
	{
		int nConIdx = FindRegion(m_Region[nIdx].m_nConRegionID[i]);

		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			_ASSERT(nConIdx >= 0);

			if (m_Region[nConIdx].Load(nX + nXOff[i], nY + nYOff[i]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
				//m_Region[nConIdx].LoadObject(0, nX + nXOff[i], nY + nYOff[i], m_szMapPath);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		//m_ClientRegionIdx[i + 1] = nConIdx;
		m_Region[nIdx].m_nConnectRegion[i] = nConIdx;
	}

	if (m_Region[nIdx].m_nConnectRegion[0] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[0] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[1] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[7] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[4] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[1];
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[3] = m_Region[nIdx].m_nConnectRegion[2];
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[5] = m_Region[nIdx].m_nConnectRegion[6];
		m_Region[m_Region[nIdx].m_nConnectRegion[0]].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[7];
	}

	if (m_Region[nIdx].m_nConnectRegion[1] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[0] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[1] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[7] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[4] = m_Region[nIdx].m_nConnectRegion[2];
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[3] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[5] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[0];
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConRegionID[2]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[8], nY + nYOff[8]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = -1;
			m_Region[nConIdx].m_nConnectRegion[2] = -1;
			m_Region[nConIdx].m_nConnectRegion[3] = -1;
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = m_Region[nIdx].m_nConnectRegion[2];
			m_Region[nConIdx].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[1];
			m_Region[nConIdx].m_nConnectRegion[7] = -1;
		}
	}

	if (m_Region[nIdx].m_nConnectRegion[2] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[0] = m_Region[nIdx].m_nConnectRegion[1];
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[1] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[7] = m_Region[nIdx].m_nConnectRegion[0];
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[4] = m_Region[nIdx].m_nConnectRegion[3];
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[3] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[5] = m_Region[nIdx].m_nConnectRegion[4];
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[6] = nIdx;
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConRegionID[2]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[9], nY + nYOff[9]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = -1;
			m_Region[nConIdx].m_nConnectRegion[2] = -1;
			m_Region[nConIdx].m_nConnectRegion[3] = -1;
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = m_Region[nIdx].m_nConnectRegion[3];
			m_Region[nConIdx].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[2];
			m_Region[nConIdx].m_nConnectRegion[7] = m_Region[nIdx].m_nConnectRegion[1];
		}
		if (m_Region[nIdx].m_nConnectRegion[1] >= 0)
		{
			m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[3] = nConIdx;
			m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[1] =
				m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2];
			if(nConIdx >= 0)
			{
				m_Region[nConIdx].m_nConnectRegion[0] =
					m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2];
				if(m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2] >= 0)
				{
					m_Region[m_Region[m_Region[nIdx].m_nConnectRegion[1]].m_nConnectRegion[2]].
						m_nConnectRegion[4] = nConIdx;
				}
			}
		}
	}

	if (m_Region[nIdx].m_nConnectRegion[3] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[0] = m_Region[nIdx].m_nConnectRegion[2];
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[1] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[2] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[3] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[4] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[5] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[4];
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[7] = nIdx;
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConRegionID[2]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[10], nY + nYOff[10]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[2] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = -1;
			m_Region[nConIdx].m_nConnectRegion[2] = -1;
			m_Region[nConIdx].m_nConnectRegion[3] = -1;
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = -1;
			m_Region[nConIdx].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[3];
			m_Region[nConIdx].m_nConnectRegion[7] = m_Region[nIdx].m_nConnectRegion[2];
		}
		if (m_Region[nIdx].m_nConnectRegion[2] >= 0)
		{
			m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[3] = nConIdx;
			m_Region[m_Region[nIdx].m_nConnectRegion[3]].m_nConnectRegion[1] =
				m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2];
			if(nConIdx >= 0)
			{
				m_Region[nConIdx].m_nConnectRegion[0] =
					m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2];
				if(m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2] >= 0)
				{
					m_Region[m_Region[m_Region[nIdx].m_nConnectRegion[2]].m_nConnectRegion[2]].
						m_nConnectRegion[4] = nConIdx;
				}
			}
		}
	}

	if (m_Region[nIdx].m_nConnectRegion[4] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[0] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[1] = m_Region[nIdx].m_nConnectRegion[2];
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[3];
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[3] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[4] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[5] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[6] = m_Region[nIdx].m_nConnectRegion[5];
		m_Region[m_Region[nIdx].m_nConnectRegion[4]].m_nConnectRegion[7] = m_Region[nIdx].m_nConnectRegion[6];
	}

	if (m_Region[nIdx].m_nConnectRegion[5] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[0] = m_Region[nIdx].m_nConnectRegion[6];
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[1] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[4];
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[3] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[4] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[5] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[7] = -1;
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConRegionID[6]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[11], nY + nYOff[11]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = m_Region[nIdx].m_nConnectRegion[6];
			m_Region[nConIdx].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[5];
			m_Region[nConIdx].m_nConnectRegion[3] = -1;
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = -1;
			m_Region[nConIdx].m_nConnectRegion[6] = -1;
			m_Region[nConIdx].m_nConnectRegion[7] = -1;
		}
	}

	if (m_Region[nIdx].m_nConnectRegion[6] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[0] = m_Region[nIdx].m_nConnectRegion[7];
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[1] = m_Region[nIdx].m_nConnectRegion[0];
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[2] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[3] = m_Region[nIdx].m_nConnectRegion[4];
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[4] = m_Region[nIdx].m_nConnectRegion[5];
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[5] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[7] = -1;
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConRegionID[6]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[12], nY + nYOff[12]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = m_Region[nIdx].m_nConnectRegion[7];
			m_Region[nConIdx].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[6];
			m_Region[nConIdx].m_nConnectRegion[3] = m_Region[nIdx].m_nConnectRegion[5];
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = -1;
			m_Region[nConIdx].m_nConnectRegion[6] = -1;
			m_Region[nConIdx].m_nConnectRegion[7] = -1;
		}
		if (m_Region[nIdx].m_nConnectRegion[5] >= 0)
		{
			m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[7] = nConIdx;
			m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[5] =
				m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6];
			if(nConIdx >= 0)
			{
				m_Region[nConIdx].m_nConnectRegion[4] =
					m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6];
				if(m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6] >= 0)
				{
					m_Region[m_Region[m_Region[nIdx].m_nConnectRegion[5]].m_nConnectRegion[6]].
						m_nConnectRegion[0] = nConIdx;
				}
			}
		}
	}
	
	if (m_Region[nIdx].m_nConnectRegion[7] >= 0)
	{
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[0] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[1] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[0];
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[3] = nIdx;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[4] = m_Region[nIdx].m_nConnectRegion[6];
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[5] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[6] = -1;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[7] = -1;
		int nConIdx = FindRegion(m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConRegionID[6]);
		if (nConIdx < 0)
		{
			nConIdx = FindFreeRegion(nX, nY);
			if (m_Region[nConIdx].Load(nX + nXOff[13], nY + nYOff[13]))
			{
				m_Region[nConIdx].m_nIndex = nConIdx;
				m_Region[nConIdx].Init(m_nRegionWidth, m_nRegionHeight);
			}
			else
			{
				m_Region[nConIdx].m_nIndex = -1;
				m_Region[nConIdx].m_RegionID = -1;
				nConIdx = -1;
			}
		}
		else
			m_bCheckedRegion[nConIdx] = TRUE;
		m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[6] = nConIdx;
		if(nConIdx >= 0)
		{
			m_Region[nConIdx].m_nConnectRegion[0] = -1;
			m_Region[nConIdx].m_nConnectRegion[1] = -1;
			m_Region[nConIdx].m_nConnectRegion[2] = m_Region[nIdx].m_nConnectRegion[7];
			m_Region[nConIdx].m_nConnectRegion[3] = m_Region[nIdx].m_nConnectRegion[6];
			m_Region[nConIdx].m_nConnectRegion[4] = -1;
			m_Region[nConIdx].m_nConnectRegion[5] = -1;
			m_Region[nConIdx].m_nConnectRegion[6] = -1;
			m_Region[nConIdx].m_nConnectRegion[7] = -1;
		}
		if (m_Region[nIdx].m_nConnectRegion[6] >= 0)
		{
			m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[7] = nConIdx;
			m_Region[m_Region[nIdx].m_nConnectRegion[7]].m_nConnectRegion[5] =
				m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6];
			if(nConIdx >= 0)
			{
				m_Region[nConIdx].m_nConnectRegion[4] =
					m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6];
				if(m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6] >= 0)
				{
					m_Region[m_Region[m_Region[nIdx].m_nConnectRegion[6]].m_nConnectRegion[6]].
						m_nConnectRegion[0] = nConIdx;
				}
			}
		}
	}
	if(bLoadNew && m_nGridTotal)
	{
		bool bLoadData = false;
		char szFile[MAX_PATH];
		sprintf(szFile, "%d.fp", m_SubWorldID);
		KFile File;
		if(File.Open(szFile))
		{
			UINT uVersion;
			File.Read(&uVersion, sizeof(UINT));
			if(uVersion == FINDPATH_VERSION)
			{
				File.Read(&uVersion, sizeof(UINT));
				File.Read(m_GridNode, uVersion);
				File.Read(&uVersion, sizeof(UINT));
				if (uVersion > 0)
				{
					m_vNeighbour.resize(uVersion);
					File.Read(&m_vNeighbour[0], sizeof(VGridNeighbour)*uVersion);
					m_bHavePath = TRUE;
				}
				bLoadData = true;
			}
		}
		if(!bLoadData)
		{
			//m_hLoadPathGrid = CreateThread(NULL, 0,
			//	LoadPathGrid, this, 0, NULL);
			//WaitForSingleObject(m_hLoadPathGrid, INFINITE);
			ProcLoadPathGrid();
			g_CreatePath("\\maps");
			g_SetFilePath("\\maps");
			sprintf(szFile, "%d.fp", m_SubWorldID);
			int nAllCell  = m_nGridW*m_nRegionWidth * m_nGridH*m_nRegionHeight;
			KFile WFile;
			WFile.Create(szFile);
			UINT uVersion = FINDPATH_VERSION;
			WFile.Write(&uVersion, sizeof(UINT));
			uVersion = sizeof(VGridNode)*nAllCell;
			WFile.Write(&uVersion, sizeof(UINT));
			WFile.Write(m_GridNode, sizeof(VGridNode)*nAllCell);
			uVersion = m_vNeighbour.size();
			WFile.Write(&uVersion, sizeof(UINT));
			if (uVersion > 0)
				WFile.Write(&m_vNeighbour[0], sizeof(VGridNeighbour)*uVersion);
			WFile.Close();
			m_bHavePath = TRUE;
		}
	}
	return TRUE;
}
#endif

void KSubWorld::LoadTrap()
{
}

void KSubWorld::MessageLoop()
{
	KWorldMsgNode	Msg;
	while(m_WorldMessage.Get(&Msg))
	{
		ProcessMsg(&Msg);
	}
}

void KSubWorld::ProcessMsg(KWorldMsgNode *pMsg)
{
	switch(pMsg->m_dwMsgType)
	{
	case GWM_NPC_DEL:
#ifndef _SERVER
		if (Npc[pMsg->m_nParam[0]].m_RegionIndex >= 0)
		{
			int nIdx = pMsg->m_nParam[0];
			int nSubWorld = Npc[nIdx].m_SubWorldIndex;
			int nRegion = Npc[nIdx].m_RegionIndex;
			SubWorld[nSubWorld].m_Region[nRegion].RemoveNpc(nIdx);
			SubWorld[nSubWorld].m_Region[nRegion].DecRef(Npc[nIdx].m_MapX, Npc[nIdx].m_MapY, obj_npc);
		}
		NpcSet.Remove(pMsg->m_nParam[0]);
#else
		{
			int nIdx = pMsg->m_nParam[0];
			int nSubWorld = Npc[nIdx].m_SubWorldIndex;
			int nRegion = Npc[nIdx].m_RegionIndex;
			
			SubWorld[nSubWorld].m_Region[nRegion].RemoveNpc(nIdx);
			SubWorld[nSubWorld].m_Region[nRegion].DecRef(Npc[nIdx].m_MapX, Npc[nIdx].m_MapY, obj_npc);
			NpcSet.Remove(nIdx);
		}
#endif
		break;
/*	case GWM_NPC_CHANGE_REGION:
		NpcChangeRegion(pMsg->m_nParam[0], pMsg->m_nParam[1], pMsg->m_nParam[2]);
		break;*/
	case GWM_OBJ_DEL:
#ifndef _SERVER
		if (Object[pMsg->m_nParam[0]].m_nRegionIdx >= 0)
			SubWorld[Object[pMsg->m_nParam[0]].m_nSubWorldID].m_Region[Object[pMsg->m_nParam[0]].m_nRegionIdx].RemoveObj(pMsg->m_nParam[0]);
		ObjSet.Remove(pMsg->m_nParam[0]);
#else
		SubWorld[Object[pMsg->m_nParam[0]].m_nSubWorldID].m_Region[Object[pMsg->m_nParam[0]].m_nRegionIdx].RemoveObj(pMsg->m_nParam[0]);
		ObjSet.Remove(pMsg->m_nParam[0]);
#endif
		break;
	case GWM_OBJ_CHANGE_REGION:
		ObjChangeRegion(pMsg->m_nParam[0], pMsg->m_nParam[1], pMsg->m_nParam[2]);
		break;
	case GWM_MISSLE_CHANGE_REGION:
		MissleChangeRegion(pMsg->m_nParam[0], pMsg->m_nParam[1], pMsg->m_nParam[2]);
		break;
	case GWM_MISSLE_DEL:
		if (Missle[pMsg->m_nParam[0]].m_nRegionId >= 0)
		{
			SubWorld[Missle[pMsg->m_nParam[0]].m_nSubWorldId].m_Region[Missle[pMsg->m_nParam[0]].m_nRegionId].RemoveMissle(pMsg->m_nParam[0]);
			MissleSet.Remove(pMsg->m_nParam[0]);
		}
		break;
/*	case GWM_PLAYER_CHANGE_REGION:
		PlayerChangeRegion(pMsg->m_nParam[0], pMsg->m_nParam[1], pMsg->m_nParam[2]);
		break;*/
	default:
		break;
	}	
}

#ifdef _SERVER
void KSubWorld::NpcChangeRegion(int nSrcRnidx, int nDesRnIdx, int nIdx)
{
	if (nIdx <= 0 || nIdx >= MAX_NPC)
		return;

	Npc[nIdx].SetActiveFlag(TRUE);
	if (nSrcRnidx == -1)
	{
		_ASSERT(nDesRnIdx >= 0);
		if (nDesRnIdx >= 0)
		{
			m_Region[nDesRnIdx].AddNpc(nIdx);
			Npc[nIdx].m_RegionIndex = nDesRnIdx;
		}
		return;
	}
	else if (nSrcRnidx == VOID_REGION)
	{
		Npc[nIdx].m_Node.Remove();
		Npc[nIdx].m_Node.Release();
	}
	else if (nSrcRnidx >= 0)
	{
		SubWorld[Npc[nIdx].m_SubWorldIndex].m_Region[nSrcRnidx].RemoveNpc(nIdx);
		if (nDesRnIdx != VOID_REGION)	// ²»ÊÇ¼ÓÈëµ½ËÀÍöÖØÉúÁ´±í
			Npc[nIdx].m_RegionIndex = -1;
	}

	if (nDesRnIdx >= 0)
	{
		m_Region[nDesRnIdx].AddNpc(nIdx);
		Npc[nIdx].m_RegionIndex = nDesRnIdx;
	}
	else if (nDesRnIdx == VOID_REGION)
	{
		m_NoneRegionNpcList.AddTail(&Npc[nIdx].m_Node);
		Npc[nIdx].m_Node.AddRef();
	}
}
#endif

#ifndef _SERVER
void KSubWorld::NpcChangeRegion(int nSrcRnidx, int nDesRnIdx, int nIdx)
{
	int		nSrc, nDest;

	if (nSrcRnidx == -1)
	{
		nDest = SubWorld[0].FindRegion(nDesRnIdx);
		if (nDest < 0)
			return;
		m_Region[nDest].AddNpc(nIdx);
		Npc[nIdx].m_dwRegionID = m_Region[nDest].m_RegionID;
		Npc[nIdx].m_RegionIndex = nDest;
		return;
	}

	nSrc = SubWorld[0].FindRegion(nSrcRnidx);
	if (nSrc >= 0)
		SubWorld[0].m_Region[nSrc].RemoveNpc(nIdx);

	nDest = SubWorld[0].FindRegion(nDesRnIdx);

	KIndexNode *pNode = &Npc[nIdx].m_Node;
	if (nDest >= 0)
	{
		m_Region[nDest].AddNpc(nIdx);
		
		if (Player[CLIENT_PLAYER_INDEX].m_nIndex == nIdx)
			LoadMap(m_SubWorldID, m_Region[nDest].m_RegionID);
		Npc[nIdx].m_dwRegionID = m_Region[nDest].m_RegionID;
		Npc[nIdx].m_RegionIndex = nDest;
	}
	else if (Player[CLIENT_PLAYER_INDEX].m_nIndex == nIdx)
	{
		LoadMap(m_SubWorldID, nDesRnIdx);
		nDest = SubWorld[0].FindRegion(nDesRnIdx);
		if (nDest >= 0)
		{
			m_Region[nDest].AddNpc(nIdx);
			Npc[nIdx].m_dwRegionID = m_Region[nDest].m_RegionID;
			Npc[nIdx].m_RegionIndex = nDest;
		}
	}
}
#endif

void KSubWorld::ObjChangeRegion(int nSrcRnidx, int nDesRnIdx, int nIdx)
{
#ifdef _SERVER
	_ASSERT(nSrcRnidx >= 0);
#endif
	if (nDesRnIdx == -1)
	{
		SubWorld[Object[nIdx].m_nSubWorldID].m_Region[nSrcRnidx].RemoveObj(nIdx);
//		ObjSet.Remove(nIdx);
		return;
	}
	if (nSrcRnidx == -1)
	{
		m_Region[nDesRnIdx].AddObj(nIdx);
		return;
	}
	KIndexNode* pNode = (KIndexNode *)m_Region[nSrcRnidx].GetObjNode(nIdx);
	if (pNode)
	{
		pNode->Remove();
		m_Region[nDesRnIdx].m_ObjList.AddTail(pNode);
	}
}

void KSubWorld::MissleChangeRegion(int nSrcRnidx, int nDesRnIdx, int nIdx)
{
	if (nIdx <= 0 || nIdx >= MAX_MISSLE)
		return;
	if (nSrcRnidx == -1)
	{
		m_Region[nDesRnIdx].AddMissle(nIdx);
		return;
	}
	if(nSrcRnidx == nDesRnIdx)
		return;
	SubWorld[Missle[nIdx].m_nSubWorldId].m_Region[nSrcRnidx].RemoveMissle(nIdx);

	if (nDesRnIdx == -1)
	{
		MissleSet.Remove(nIdx);
		return;
	}
	
	m_Region[nDesRnIdx].AddMissle(nIdx);
}

#ifdef _SERVER
void KSubWorld::PlayerChangeRegion(int nSrcRnidx, int nDesRnIdx, int nIdx)
{
	if (nIdx <= 0 || nIdx >= MAX_PLAYER)
		return;

	if (nSrcRnidx < 0)
		return;

	RemovePlayer(nSrcRnidx, nIdx);
	
	if (nDesRnIdx == -1)
	{
		//PlayerSet.PrepareRemove(Player[nIdx].m_nNetConnectIdx);
		printf("Player change region to not exist..., kill it\n");
		g_pServer->ShutdownClient(Player[nIdx].m_nNetConnectIdx);
		return;
	}

	if (nDesRnIdx >= 0)
	{
		AddPlayer(nDesRnIdx, nIdx);
	}
/* ÒòÎªRemovePlayerºÍAddPlayerÒÑ¾­×öÁË´ÓÁ´±íÖÐÇå³ý½Úµã£¬ÐÞ¸ÄÖÜ±ß¾Å¸öREGIONµÄÒýÓÃ¼ÆÊýµÄ²Ù×÷ÁË
	KIndexNode* pNode = &Player[nIdx].m_Node;
	if (pNode->m_Ref > 0)
	{
		pNode->Remove();
		pNode->Release();
	}
	m_Region[nSrcRnidx].m_nActive--;
	if (m_Region[nSrcRnidx].m_nActive < 0)
		m_Region[nSrcRnidx].m_nActive = 0;
	
	for (int i = 0; i < 8; i++)
	{
		if (m_Region[nSrcRnidx].m_nConnectRegion[i] < 0)
			continue;
		m_Region[m_Region[nSrcRnidx].m_nConnectRegion[i]].m_nActive--;
		if (m_Region[m_Region[nSrcRnidx].m_nConnectRegion[i]].m_nActive < 0)
			m_Region[m_Region[nSrcRnidx].m_nConnectRegion[i]].m_nActive = 0;
	}

	if (pNode->m_Ref == 0)
	{
		m_Region[nDesRnIdx].m_PlayerList.AddTail(pNode);
		pNode->AddRef();
	}

	m_Region[nDesRnIdx].m_nActive++;
	for (i = 0; i < 8; i++)
	{
		if (m_Region[nDesRnIdx].m_nConnectRegion[i] < 0)
			continue;
		m_Region[m_Region[nDesRnIdx].m_nConnectRegion[i]].m_nActive++;
		if (m_Region[m_Region[nDesRnIdx].m_nConnectRegion[i]].m_nActive < 0)
			m_Region[m_Region[nDesRnIdx].m_nConnectRegion[i]].m_nActive = 0;
	}
*/
}
#endif

void KSubWorld::GetMps(int *nX, int *nY, int nSpeed, int nDir, int nMaxDir /* = 64 */)
{
	*nX += (g_DirCos(nDir, nMaxDir) * nSpeed) >> 10;
	*nY += (g_DirSin(nDir, nMaxDir) * nSpeed) >> 10;
}
#ifdef _SERVER
BOOL KSubWorld::SendSyncData(int nIdx, int nClient)
{
	WORLD_SYNC	WorldSync;
	WorldSync.ProtocolType = (BYTE)s2c_syncworld;
	WorldSync.Region = m_Region[Npc[nIdx].m_RegionIndex].m_RegionID;
	WorldSync.Weather = m_nWeather;
	WorldSync.Frame = m_dwCurrentTime;
	WorldSync.SubWorld = m_SubWorldID;

	if (SUCCEEDED(g_pServer->PackDataToClient(nClient, (BYTE*)&WorldSync, sizeof(WORLD_SYNC))))
	{
		return TRUE;
	}
	else
	{
		printf("player Packing world sync data failed...\n");
		return FALSE;
	}
}
#endif

#ifndef _SERVER
void KSubWorld::LoadCell()
{
}
#endif

#ifdef TOOLVERSION
int CORE_API g_ScreenX  = 0;
int CORE_API g_ScreenY  = 0;
#endif

#ifndef _SERVER
#include "../../Represent/iRepresent/iRepresentshell.h"
extern struct iRepresentShell*	g_pRepresent;
void KSubWorld::Paint()
{
	if((DWORD)m_uPaintTime > timeGetTime())
		return;
	int nIdx = Player[CLIENT_PLAYER_INDEX].m_nIndex;

	int nX, nY;

	Map2Mps(Npc[nIdx].m_RegionIndex, Npc[nIdx].m_MapX, Npc[nIdx].m_MapY, Npc[nIdx].m_OffX, Npc[nIdx].m_OffY, &nX, &nY);
	
//	m_nScreenX = nX - 400;
//	m_nScreenY = nY - 300;

#ifdef TOOLVERSION
	g_ScreenX = nX;
	g_ScreenY = nY;
#endif

	//for (int i = 0; i < m_nTotalRegion; i++)
	//{
	//	m_Region[i].Paint();
	//}
	if(g_pRepresent && m_bHavePath && m_nGridTotal && (!m_hLoadPathGrid || WaitForSingleObject(m_hLoadPathGrid, 0) == WAIT_OBJECT_0))
	{
		KRURect	Rect;
		Rect.Color.Color_dw = 0xff00ff00;
		int regx = (nX/512-m_nRegionBeginX)*REGION_GRID_WIDTH;
		int regy = (nY/1024-m_nRegionBeginY)*REGION_GRID_HEIGHT;
		for(int y=0;y<REGION_GRID_HEIGHT;++y)
		{
			for(int x=0;x<REGION_GRID_WIDTH;++x)
			{
				int id=(regy+y)*m_nGridW*REGION_GRID_WIDTH + regx+x;
				if(m_GridNode[id].obs)
				{
					id = m_GridNode[id].parentId;
					Rect.oPosition.nX = m_GridNode[id].x*32;
					Rect.oPosition.nY = m_GridNode[id].y*32;
					Rect.oEndPos.nX = Rect.oPosition.nX + m_GridNode[id].w*32;
					Rect.oEndPos.nY = Rect.oPosition.nY + m_GridNode[id].h*32;
					g_pRepresent->DrawPrimitives(1, &Rect, RU_T_RECT, false);
				}
			}
		}
		regx = nX/32-m_nRegionBeginX*REGION_GRID_WIDTH;
		regy = nY/32-m_nRegionBeginY*REGION_GRID_HEIGHT;
		int nid = regy*m_nGridW*REGION_GRID_WIDTH + regx;
		nid = m_GridNode[nid].parentId;
		Rect.Color.Color_dw = 0xffff0000;
		Rect.oPosition.nX = m_GridNode[nid].x*32;
		Rect.oPosition.nY = m_GridNode[nid].y*32;
		Rect.oEndPos.nX = Rect.oPosition.nX + m_GridNode[nid].w*32;
		Rect.oEndPos.nY = Rect.oPosition.nY + m_GridNode[nid].h*32;
		g_pRepresent->DrawPrimitives(1, &Rect, RU_T_RECT, false);
	}
}

void KSubWorld::Mps2Screen(int *Rx, int *Ry)
{
}

void KSubWorld::Screen2Mps(int *Rx, int *Ry)
{
}
#endif

#ifdef _SERVER
int KSubWorld::GetRegionIndex(int nRegionID)
{
	int nRet;
	short nX = (short)LOWORD(nRegionID) - m_nRegionBeginX;
	short nY = (short)HIWORD(nRegionID) - m_nRegionBeginY;

	if (nX < 0 || nY < 0 || nX >= m_nWorldRegionWidth || nY >= m_nWorldRegionHeight)
		return -1;

	nRet = nY * m_nWorldRegionWidth + nX;
	return nRet;
}
#endif

void KSubWorld::Close()
{
	if (m_SubWorldID == -1)
		return;

	for (int i = 0; i < m_nTotalRegion; i++)
	{
		m_Region[i].Close();
	}
	m_WorldMessage.Clear();
	m_nIndex = -1;
	m_SubWorldID = -1;
#ifndef _SERVER
	if(m_hLoadPathGrid)
	{
		m_bStopThread = TRUE;
		WaitForSingleObject(m_hLoadPathGrid, INFINITE);
        CloseHandle(m_hLoadPathGrid);
        m_hLoadPathGrid = NULL;
		m_bStopThread = FALSE;
	}
	m_bHavePath = FALSE;
	m_vNeighbour.clear();
#endif
}

#ifdef _SERVER
void KSubWorld::LoadObject(char* szPath, char* szFileName)
{
	if (!szFileName || !szFileName[0])
		return;

	KIniFile IniFile;
	// ¼ÓÔØÊÀ½çÖÐµÄ¶¯Ì¬Îï¼þ£¨Npc¡¢Object¡¢Trap£©
	g_SetFilePath(szPath);
	int nObjNumber = 0;
	if (IniFile.Load(szFileName))
	{
		IniFile.GetInteger("MAIN", "elementnum", 0, &nObjNumber);
	}

	int nLength = strlen(szFileName);
	szFileName[nLength - 4] = 0;
	strcat(szPath, "\\");
	strcat(szPath, szFileName);

	char	szSection[32];
	char	szDataFile[32];

	for (int i = 0; i < nObjNumber; i++)
	{
		float	fPos[3];
		int		nPos[3];

		g_SetFilePath(szPath);
		sprintf(szSection, "%d", i);
		IniFile.GetFloat3(szSection, "groundoffset", fPos);
		nPos[0] = (int)(fPos[0] * 32);
		nPos[1] = (int)(fPos[1] * 32);
		nPos[2] = (int)fPos[2];

		szDataFile[0] = 0;
		IniFile.GetString(szSection, "event", "", szDataFile, sizeof(szDataFile));
		strcat(szDataFile, "_data.ini");
		
		KIniFile IniChange;
		int nType = 0, nLevel = 0;
		char	szName[32];
		
		if (!IniChange.Load(szDataFile))
			continue;
		
		IniChange.GetInteger("MAIN", "Type", 0, &nType);
		IniChange.GetInteger("MAIN", "Level", 4, &nLevel);
		IniChange.GetString("MAIN", "mapedit_templatesection", "", szName, sizeof(szName));
		
		switch(nType)
		{
		case kind_normal:		// ÆÕÍ¨Õ½¶·npc
			{
				int nFindNo = g_NpcSetting.FindRow(szName);
				if (nFindNo == -1)
					continue;
				else
					nFindNo = nFindNo - 2;
				
				int nSettingInfo = MAKELONG(nLevel, nFindNo);
				NpcSet.Add(nSettingInfo, m_nIndex, nPos[0], nPos[1]);
			}
			break;
		case kind_player:
			break;
		case kind_partner:
			break;
		case kind_dialoger:		// ÆÕÍ¨·ÇÕ½¶·npc
			break;
		case kind_bird:			// ¿Í»§¶Ëonly
			break;
		case kind_mouse:		// ¿Í»§¶Ëonly
			break;
		default:
			break;
		}
	}
}
#endif

void KSubWorld::AddPlayer(int nRegion, int nIdx)
{
	if (nRegion < 0 || nRegion >= m_nTotalRegion)
	{
		printf("Region:(%d) Player(%d)\n", nRegion, nIdx);
		return;
	}

	if (m_Region[nRegion].AddPlayer(nIdx))
	{
		m_Region[nRegion].m_nActive++;

		for (int i = 0; i < 8; i++)
		{
			int nConRegion = m_Region[nRegion].m_nConnectRegion[i];
			if (nConRegion == -1)
				continue;
			
			m_Region[nConRegion].m_nActive++;
		}
	}
	else
	{
		printf("Region:(%d) Player(%d)\n", nRegion, nIdx);
	}
}

void KSubWorld::RemovePlayer(int nRegion, int nIdx)
{
	if (nRegion < 0 || nRegion >= m_nTotalRegion)
		return;

	if (m_Region[nRegion].RemovePlayer(nIdx))
	{	
		m_Region[nRegion].m_nActive--;		

		if (m_Region[nRegion].m_nActive < 0)
		{
			_ASSERT(0);
			m_Region[nRegion].m_nActive = 0;
		}
		
		for (int i = 0; i < 8; i++)
		{
			int nConRegion = m_Region[nRegion].m_nConnectRegion[i];
			if (nConRegion == -1)
				continue;
			
			m_Region[nConRegion].m_nActive--;

			if (m_Region[nConRegion].m_nActive < 0)
			{
				_ASSERT(0);
				m_Region[nConRegion].m_nActive = 0;
			}
		}
	}
}

void KSubWorld::GetFreeObjPos(POINT& pos)
{
	POINT	posLocal = pos;
	POINT	posTemp;
	int nLayer = 1;

	if (CanPutObj(posLocal))
		return;

	while(1)
	{
		for (int i = 0; i <= nLayer; i++)
		{
			posTemp.y = posLocal.y + i * 32;
			posTemp.x = posLocal.x + (nLayer - i) * 32;
			if (CanPutObj(posTemp))
			{
				pos = posTemp;
				return;
			}

			posTemp.y = posLocal.y + i * 32;
			posTemp.x = posLocal.x - (nLayer - i) * 32;
			if (CanPutObj(posTemp))
			{
				pos = posTemp;
				return;
			}

			posTemp.y = posLocal.y - i * 32;
			posTemp.x = posLocal.x + (nLayer - i) * 32;
			if (CanPutObj(posTemp))
			{
				pos = posTemp;
				return;
			}

			posTemp.y = posLocal.y - i * 32;
			posTemp.x = posLocal.x - (nLayer - i) * 32;
			if (CanPutObj(posTemp))
			{
				pos = posTemp;
				return;
			}
		}
		nLayer++;
		if (nLayer >= 10)
			break;
	}
	return;
}

BOOL KSubWorld::CanPutObj(POINT pos)
{
	int nRegion, nMapX, nMapY, nOffX, nOffY;
	Mps2Map(pos.x, pos.y, &nRegion, &nMapX, &nMapY, &nOffX, &nOffY);

	if (nRegion >= 0 
		&& !m_Region[nRegion].GetBarrier(nMapX, nMapY, nOffX, nOffY) 
		&& !m_Region[nRegion].GetRef(nMapX, nMapY, obj_object))
		return TRUE;
	return FALSE;
}

#ifdef _SERVER
void KSubWorld::BroadCast(const char* pBuffer, size_t uSize)
{
	int nIdx = PlayerSet.GetFirstPlayer();
	while(nIdx)
	{
		if (Player[nIdx].m_nNetConnectIdx >= 0 
			&& Player[nIdx].m_nIndex > 0 
			&& Npc[Player[nIdx].m_nIndex].m_SubWorldIndex == m_nIndex)
		{
			g_pServer->PackDataToClient(Player[nIdx].m_nNetConnectIdx, pBuffer, uSize);
		}
		nIdx = PlayerSet.GetNextPlayer();
	}
}

int	KSubWorld::RevivalAllNpc()
{
	KIndexNode * pNode		= NULL;
	size_t	ulCount = 0;
	for (int i = 0; i < m_nTotalRegion; i++)
	{
		KRegion * pCurRegion = &m_Region[i];
		pNode = (KIndexNode *)pCurRegion->m_NpcList.GetHead();
		
		while(pNode)
		{
			int nNpcIdx = pNode->m_nIndex;
			if (!Npc[nNpcIdx].IsPlayer())
			{
				Npc[nNpcIdx].ExecuteRevive();
			}
			pNode = (KIndexNode *)pNode->GetNext();	
		}
	}
	
	pNode = (KIndexNode*)m_NoneRegionNpcList.GetHead();
	
	while(pNode)
	{
		int nNpcIdx = pNode->m_nIndex;
		if (!Npc[nNpcIdx].IsPlayer())
		{
			Npc[nNpcIdx].m_Frames.nTotalFrame = 1;
			Npc[nNpcIdx].m_Frames.nCurrentFrame = 0;
			ulCount ++;
		}
		pNode = (KIndexNode *)pNode->GetNext();	
	}
	
	return ulCount;
}

int KSubWorld::FindNpcFromName(const char * szName)
{
	if (!szName || !szName[0])
		return 0;
	
	KIndexNode * pNode		= NULL;

	int nResult = 0;
	
	for (int i = 0; i < m_nTotalRegion; i++)
	{
		KRegion * pCurRegion = &m_Region[i];
		pNode = (KIndexNode *)pCurRegion->m_NpcList.GetHead();
		
		while(pNode)
		{
			int nNpcIdx = pNode->m_nIndex;
			if (!strcmp(Npc[nNpcIdx].Name, szName))
			{
				nResult = nNpcIdx;
				return nResult;
			}
			pNode = (KIndexNode *)pNode->GetNext();	
		}
	}	
	
	pNode = (KIndexNode*)m_NoneRegionNpcList.GetHead();
	
	while(pNode)
	{
		int nNpcIdx = pNode->m_nIndex;
		if (!strcmp(Npc[nNpcIdx].Name, szName))
		{
			return nNpcIdx;
		}
		pNode = (KIndexNode *)pNode->GetNext();	
	}
	
	return nResult;
}
#endif
