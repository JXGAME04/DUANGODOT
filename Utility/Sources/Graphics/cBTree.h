// cBTree.h: interface for the cBTree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CBTREE_H__A69B26FC_E0F0_40FA_B335_79BC2EF2AA22__INCLUDED_)
#define AFX_CBTREE_H__A69B26FC_E0F0_40FA_B335_79BC2EF2AA22__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cPlaneVer;
class cLight;
class cBNode;
#include "GraphicsPublic.h"

class cBTree  
{
public:
	cBTree();
	virtual ~cBTree();
	void clear();

	cBNode* _root;
	enum eOrder{ORDER_PRE,ORDER_IN,ORDER_FALSEIN,ORDER_POST,};
	bool IsEmpty(){return _root == NULL;}

	bool insert(cPlaneVer* value);

	bool remove(cPlaneVer* value);
	bool find(cPlaneVer* p);
	bool remove(long value);
	bool find(long p);

	//help function
	
	void remove_root();
	void remove_root_fromtree();
	
	void build();
	void draw();
};

#endif // !defined(AFX_CBTREE_H__A69B26FC_E0F0_40FA_B335_79BC2EF2AA22__INCLUDED_)
