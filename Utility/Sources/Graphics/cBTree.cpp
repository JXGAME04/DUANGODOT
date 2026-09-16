// cBTree.cpp: implementation of the cBTree class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cBTree.h"
#include "cBNode.h"
#include "cBNodeRoot.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cBTree::cBTree() 
{
	_root = NULL;
	_root = new cBNodeRoot;
}

cBTree::~cBTree()
{
	if (_root)
		_root->clear();
}

void cBTree::clear()
{
	if (_root)
		_root->clear();
	_root = NULL;
	_root = new cBNodeRoot;
}

bool cBTree::insert(cPlaneVer* value)
{
	if (! _root)
	{
		ASSERT(0);
		_root = new cBNode(value);
	}
	else
	{
		cBNode* p = new cBNode(value);
		bool b = _root->insert_node(p);
		ASSERT(b);
	}
	return true;
}

void cBTree::remove_root_fromtree()
{
	cBNode* tmp = _root;
	if(_root->_brother)
	{
		_root = _root->_brother;
		_root->_lchild = tmp->_lchild;
		if (_root->_lchild)
			_root->_lchild->set_father(_root);
		_root->_rchild = tmp->_rchild;
		if (_root->_rchild)
			_root->_rchild->set_father(_root);
		_root->set_father(NULL);
	}
	else if(!_root->_rchild)
	{
		_root = _root->_lchild;
		if (_root)
			_root->set_father(NULL);
	}
	else
	{
		_root = _root->_rchild;
		tmp->lchild_leaf();
		if (_root)
			_root->set_father(NULL);
	}
	tmp->_lchild = NULL;
	tmp->_rchild = NULL;
	tmp->_father = NULL;
}

void cBTree::remove_root()
{
	cBNode* tmp = _root;
	remove_root_fromtree();	
	tmp->_plane = NULL;
	delete tmp;
}

bool cBTree::remove(cPlaneVer* value)
{
	return remove((long) value);
}

bool cBTree::find(cPlaneVer* value)
{
	return find((long) value);
}

bool cBTree::find(long p)
{
	if (!_root)
		return false;
	return _root->find(p);
}

bool cBTree::remove(long value)
{
	if (!_root)
		return false;

	if (_root->get_value() == value)
	{
		remove_root();
		return true;
	}
	else
		return _root->remove(value);
}

void cBTree::build()
{
	if (!_root)
		return;
	_root->build();
}

void cBTree::draw()
{
	if (!_root)
		return;
	_root->draw();
//	_root->drawnum();
}

