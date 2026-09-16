#ifndef KMISSLESET_H
#define KMISSLESET_H
#include "KMissle.h"
#include "KLinkArray.h"

class CORE_API KMissleSet
{
private:
	KLinkArray		m_FreeIdx;				//	可用表
	KLinkArray		m_UseIdx;				//	已用表
public:
	void	Init();
	int		Activate();
	int		FindFree();
	int		Add(int SubWorldId, int regionid, int x , int y , int dx = 0 , int dy = 0);
	int		Add(int SubWorldId, int px, int py);
	int		CreateMissile(int nSkillId, int nMissleId, int nLauncher,  int nTargetId ,int nSubWorldId, int nPX, int nPY, int nDir);
	void	Remove(int nIndex);
	void	ClearMissles();
	int		GetCount();
	void	Draw();
#ifndef	_SERVER
	int		GetNextIdx(int nIdx)
	{
		if (nIdx < 0 || nIdx >= MAX_MISSLE)
			return 0;
		return m_UseIdx.GetNext(nIdx);
	}
#endif
};
extern CORE_API KMissleSet MissleSet;
#endif
