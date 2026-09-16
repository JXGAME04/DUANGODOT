// cBNode.h: interface for the cBNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CBNODE_H__5B54FDEE_10AF_499D_B0F1_454A2414B657__INCLUDED_)
#define AFX_CBNODE_H__5B54FDEE_10AF_499D_B0F1_454A2414B657__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cPlaneVer;
#include "iColor.h"
#include "iPublic.h"
#include "GraphicsPublic.h"

class cBNode;
typedef std::list<cBNode*> NodeList;
typedef NodeList::iterator itNode;
typedef NodeList::reverse_iterator itrNode;

class cBNode  
{
public:
	cBNode(cPlaneVer* plane);
	virtual ~cBNode();
	void clear();

	NodeList _llist,_rlist;

	cBNode* _father;
	cBNode* get_father() const {return _father;}
	void set_father(cBNode* p){_father = p;}

	cBNode* _lchild;
	cBNode* get_left() const {return _lchild;}
	void set_left(cBNode* p) ;

	cBNode* _rchild;
	cBNode* get_right()const {return _rchild;}
	void set_right(cBNode* p) ;

	cBNode* _brother;
	cBNode* get_brother()const {return _brother;}
	void insert_brother(cBNode* p);

	void add_child(cBNode* p);

	cPlaneVer* _plane;
	cPlaneVer* get_plane()const {return _plane;}
	BOOL	is_plane(){return get_plane() != NULL;}
	cPoint _p1,_p2;

//	cLight* _light;
//	cLight* get_light()const {return _light;}
//	BOOL	is_light(){return get_light() != NULL;}

	//utiliity
	virtual VersortEnum GetVersortType();
	virtual void GetSortPoint(cPoint& pt);
	virtual void GetSortSeam(cPoint& p1,cPoint& p2);
	virtual void GetSortLine(float& k,float& b);
	long get_value();

	//find
	bool find(long value);

	//insert
	enum AddEnum{ADD_NULL,ADD_LEFT,ADD_RIGHT,ADD_BROTHER,ADD_LEFTLIST,ADD_RIGHTLIST,};
	bool insert_node(cBNode* p);
	bool insert_point(cBNode* p);
	bool insert_tree(cBNode* p);

	//remove
	bool remove(long value);
	bool remove();
	bool remove_fromtree();
	//移动左节点，使他成为右节点的最左节点
	void replace_child(cBNode* p,cBNode* pNew);
	void lchild_leaf();

	void build();
	void build_self();

	//draw
	void draw_self();
	void draw();
	void drawnum();
};

#endif // !defined(AFX_CBNODE_H__5B54FDEE_10AF_499D_B0F1_454A2414B657__INCLUDED_)
