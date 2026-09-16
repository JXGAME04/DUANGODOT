#ifndef KLinkArray_H
#define	KLinkArray_H

class KLinkNode
{
public:
	int nPrev;
	int	nNext;
public:
	KLinkNode() { nPrev = nNext = 0; }
};

class ENGINE_API KLinkArray
{
private:
	KLinkNode*	pNode;
	int nCount;
	int nOwner;
public:
	KLinkArray() { pNode = NULL; nCount = 0; nOwner = 0;}
	~KLinkArray();
	int GetCount() { return nCount; }
	KLinkNode * GetNode(int nIdx){return &pNode[nIdx];};
	void			Init(int nSize, int nOwnerType = 0);
	void			Remove(int nIdx);
	void			Insert(int nIdx);
	int				GetNext(int nIdx) const { 
		if(pNode) return pNode[nIdx].nNext; 
		else return 0;
	}
	int				GetPrev(int nIdx) const { 
		if(pNode) return pNode[nIdx].nPrev; 
		else return 0;
	}
};
#endif
