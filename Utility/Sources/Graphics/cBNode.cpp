// cBNode.cpp: implementation of the cBNode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cBNode.h"
#include "position.h"
#include "cPlaneVer.h"
#include "cPlaneVerMesh.h"
#include "cLight.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cBNode::cBNode(cPlaneVer* value) : _plane(value)
{
	_lchild = NULL;
	_rchild = NULL;
	_father = NULL;
	_brother = NULL;
	if (value)
		value->GetSortSeam(_p1,_p2);
}

cBNode::~cBNode()
{
	_lchild = NULL;
	_rchild = NULL;
	_father = NULL;
	_brother = NULL;
	_plane = NULL;
	_llist.clear();
	_rlist.clear();
}

void cBNode::add_child(cBNode* p)
{
	if (p)
		p->set_father(this);
}

void cBNode::set_left(cBNode* p) 
{
	_lchild = p;
	add_child(p);
}

void cBNode::set_right(cBNode* p)
{
	_rchild = p;
	add_child(p);
}

/////////////////////////////////////////////////////////////////////
//list function
/////////////////////////////////////////////////////////////////////

void clear_list_seam(NodeList& nlist) 
{
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		cBNode* p = (*it);
		clear_list_seam(p->_llist);
		clear_list_seam(p->_rlist);
		delete p;
	}
	nlist.clear();
}

void printf_list(NodeList& nlist)
{
	OutputDebugString("\nlist is\n");
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{	
		cPoint p1,p2;
		(*it)->GetSortSeam(p1,p2);
		CString sMsg;
		sMsg.Format("p1(%f,%f) p2(%f,%f)\n", p1.x,p1.y,p2.x,p2.y);
		OutputDebugString(sMsg);
	}
}

//如果在上边即可，因为取的是最低的点
//如果在下边，继续走

//
void insert_list_seam(NodeList& nlist, cBNode* p)
{
	ePoint2Line e;
	cBNode* pFather = NULL;

	if (p->GetVersortType() == VERSORT_POINT)
	{
		cPoint pt;
		p->GetSortPoint(pt);

		for (itNode it = nlist.begin(); it != nlist.end(); ++it)
		{
			if ((*it)->GetVersortType() != VERSORT_SEAM)
				continue;
			cPoint p1,p2;
			(*it)->GetSortSeam(p1,p2);
			ePoint2Line e2 = Point2Line(pt,p1,p2);

			float fsizex = 0.4f;
			float fsizesub = 0.1f;
			float fsizeadd = 4.f;
			if (e2 == PL_Up)
			{
				//在该矩形之内
				fsizex = 0.4f;
				fsizesub = 0.f;
				fsizeadd = 0.f;
			}
			if (	pt.x >= _cpp_min(p1.x,p2.x) - fsizex && pt.x <=  _cpp_max(p1.x,p2.x) + fsizex
				&&  pt.y >= _cpp_min(p1.y,p2.y) - fsizesub && pt.y <=  _cpp_max(p1.y,p2.y) + fsizeadd )
			{
				pFather = *it;
				e = e2;
				if (e == PL_Up)
					break;
			}
		}
	}
	else
	{
		cPoint pt1,pt2;
		p->GetSortSeam(pt1,pt2);
		for (itNode it = nlist.begin(); it != nlist.end(); ++it)
		{
			ASSERT ((*it)->GetVersortType() == VERSORT_SEAM);
			cPoint p1,p2;
			(*it)->GetSortSeam(p1,p2);

				//在该矩形之内
			float fsize = 0.2f;
			if (
				(	pt1.x >= _cpp_min(p1.x,p2.x) - fsize	&& pt1.x <=  _cpp_max(p1.x,p2.x) + fsize
				&&  pt1.y >= _cpp_min(p1.y,p2.y)			&& pt1.y <=  _cpp_max(p1.y,p2.y) )
				&&				
				(	pt2.x >= _cpp_min(p1.x,p2.x) - fsize	&& pt2.x <=  _cpp_max(p1.x,p2.x) + fsize
				&&  pt2.y >= _cpp_min(p1.y,p2.y)			&& pt2.y <=  _cpp_max(p1.y,p2.y) )
			)
			{
				if (PointToLine(pt1,p1,p2) > PointToLine(pt2,p1,p2))
					e = Point2Line(pt1,p1,p2);
				else
					e = Point2Line(pt2,p1,p2);
				pFather = *it;
				break;
			}
		}
	}

	if (pFather)
	{
		switch (e)
		{
		case PL_Up:
		case PL_On:
			p->set_father((pFather));
			insert_list_seam((pFather)->_llist,p);
			return;
		case PL_Down:
			p->set_father((pFather));
			insert_list_seam((pFather)->_rlist,p);
			return;
		}
	}
	
	cPoint pt;
	p->GetSortPoint(pt);
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		cPoint p2;
		(*it)->GetSortPoint(p2);
		if (pt.y < p2.y)
		{
			nlist.insert(it,p);
			return;
		}
	}
	nlist.push_back(p);
}

bool find_list_seam(NodeList& nlist, long n)
{
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		if ((*it)->get_value() == n)
			return true;
		if (find_list_seam((*it)->_llist,n))
			return true;
		if (find_list_seam((*it)->_rlist,n))
			return true;
	}
	return false;
}

bool insert_list_seam(NodeList& nlist, NodeList& ilist)
{
	for (itNode it = ilist.begin(); it != ilist.end(); ++it)
	{
		insert_list_seam(nlist,(*it));
		insert_list_seam(nlist,(*it)->_llist);
		insert_list_seam(nlist,(*it)->_rlist);
	}
	return true;
}

bool remove_list_seam(NodeList& nlist, long n)
{
#pragma message (ATTENTION("not finished"))
	cBNode* p = NULL;
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		if ((*it)->get_value() == n)
		{
			if ((*it)->GetVersortType() == VERSORT_SEAM)
				return true;
			p = (*it);
			nlist.erase(it);
			break;
#pragma message (ATTENTION("how about the child of the node!"))
		}
		if (remove_list_seam((*it)->_llist,n))
			return true;
		if (remove_list_seam((*it)->_rlist,n))
			return true;
	}
	if (p != NULL)
	{
		ASSERT(p->GetVersortType() == VERSORT_POINT);
//		insert_list_seam(nlist,p->_llist);
//		insert_list_seam(nlist,p->_rlist);
//		p->_llist.clear();
//		p->_rlist.clear();
		delete p;
		return true;
	}
	return false;
}

void draw_list_seam(NodeList& nlist)
{
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		draw_list_seam((*it)->_llist);
		(*it)->draw_self();
		draw_list_seam((*it)->_rlist);
	}
}

void build_list_seam(NodeList& nlist)
{
	for (itNode it = nlist.begin(); it != nlist.end(); ++it)
	{
		build_list_seam((*it)->_llist);
		(*it)->build_self();
		build_list_seam((*it)->_rlist);
	}
}

/////////////////////////////////////////////////////////////////////
//utility function
/////////////////////////////////////////////////////////////////////
long cBNode::get_value()
{
	return (long)_plane;
}

VersortEnum cBNode::GetVersortType()
{
	if (_plane)
		return _plane->GetVersortType();
	return VERSORT_POINT;
}

void cBNode::GetSortPoint(cPoint& pt)
{
	_plane->GetSortPoint(pt);
}

void cBNode::GetSortSeam(cPoint& p1,cPoint& p2)
{
	_plane->GetSortSeam(p1,p2);
}

void cBNode::GetSortLine(float& k,float& b)
{
	_plane->GetLineFunction(k,b);
}

/*
				*
               *
              * * 
			 *   *
			*     *
		   *       *
		* * * * * * *
					 *
//当出现此三角形的时候，就死循环,然后堆栈overflow
//但是我们的游戏世界是规则的，不会出现上面的情况
//但是,这样的情况出现了,永远不要去进行禁不起证明的设想
*/

/////////////////////////////////////////////////////////////////////
//clear
/////////////////////////////////////////////////////////////////////
void cBNode::clear()
{
	if (_lchild)
	{
		_lchild->clear();
		_lchild = NULL;
	}
	else
		clear_list_seam(_llist);
	
	if (_brother)
		_brother->clear();
	_brother = NULL;
	
	if (_rchild)
	{
		_rchild->clear();
		_rchild = NULL;
	}
	else
		clear_list_seam(_rlist);

	_plane = NULL;

	delete this;
}

/////////////////////////////////////////////////////////////////////
//find
/////////////////////////////////////////////////////////////////////
bool cBNode::find(long p)
{
	if (get_value() == p)
		return true;
	if (find_list_seam(_llist,p) || find_list_seam(_rlist,p))
		return true;
	if (get_brother() && get_brother()->find(p))
		return true;
	if (get_left() && get_left()->find(p))
		return true;
	if (get_right() && get_right()->find(p))
		return true;
	return false;
}

/////////////////////////////////////////////////////////////////////
//insert
/////////////////////////////////////////////////////////////////////
bool cBNode::insert_point(cBNode* p)
{
	AddEnum eAdd = ADD_NULL;

	if (GetVersortType() == VERSORT_TREE)
	{
		ePoint2Line e;
		if (p->GetVersortType() == VERSORT_POINT)
		{
			cPoint pt;
			p->GetSortPoint(pt);
			e = Point2Line(pt,_p1,_p2);
		}
		else // seam
		{
			cPoint pt1,pt2;
			p->GetSortSeam(pt1,pt2);
			if (PointToLine(pt1,_p1,_p2) > PointToLine(pt2,_p1,_p2))
				e = Point2Line(pt1,_p1,_p2);
			else
				e = Point2Line(pt2,_p1,_p2);
		}
		switch (e)
		{
		case PL_Up:
			if (_lchild == NULL)
				eAdd = ADD_LEFTLIST;
			else
				eAdd = ADD_LEFT;
			break;
		case PL_Down:
		case PL_On:
			if (_rchild == NULL)
				eAdd = ADD_RIGHTLIST;
			else
				eAdd = ADD_RIGHT;
			break;
		}
	}
	else
	{
		ASSERT(0);
	}

	switch(eAdd)
	{
	case ADD_LEFTLIST:
		p->set_father(this);
		insert_list_seam(_llist,p);
		break;
	case ADD_RIGHTLIST:
		p->set_father(this);
		insert_list_seam(_rlist,p);
		break;
	case ADD_LEFT:
		_lchild->insert_point(p);
		break;
	case ADD_RIGHT:
		_rchild->insert_point(p);		
		break;
	}
	return true;
}

bool cBNode::insert_tree(cBNode* p)
{
	ASSERT(p->get_plane() != get_plane());
	ASSERT(p->GetVersortType() == VERSORT_TREE);
	ASSERT(GetVersortType() == VERSORT_TREE);
	float k,_k,b,_b;
	p->GetSortLine(k,b);
	GetSortLine(_k,_b);
	eSeam2Line e;
	cPoint p1,p2;
	p1 = p->_p1;
	p2 = p->_p2;
	if (k == _k && b == _b)
		e = SL_On;
	else
		e = Seam2Line(p1,p2,_p1,_p2);

	if (e == SL_Cross)
	{
		//将该线段切分成两段!
		cPoint point = SeamCrossByLineAt(p1,p2,_p1,_p2);
	
		ASSERT(p1 != point);
		ASSERT(p2 != point);
		cBNode* pUp = new cBNode(p->get_plane());
		pUp->_p1 = p1;
		pUp->_p2 = point;
		cBNode* pDown = new cBNode(p->get_plane());			
		pDown->_p1 = point;
		pDown->_p2 = p2;
		delete p;
		//x 的值的顺序不变

		ePoint2Line e = Point2Line(p1,_p1,_p2);
		if (e == PL_Down)
		{
			cBNode* pTemp = pUp;
			pUp = pDown;
			pDown = pTemp;
		}
		if (!_lchild)
			set_left(pUp);
		else
			_lchild->insert_node(pUp);

		if (!_rchild)
			set_right(pDown);
		else
			_rchild->insert_node(pDown);		
		return true;
	}

	AddEnum eAdd;
	switch(e)
	{
	case SL_Up:
		eAdd = ADD_LEFT;
		break;
	case SL_Down:
		eAdd = ADD_RIGHT;
		break;
	case SL_On:
		eAdd = ADD_BROTHER;
		break;
	}

	switch(eAdd)
	{
	case ADD_LEFT:
		if (!_lchild)
			set_left(p);
		else
			_lchild->insert_tree(p);
		break;
	case ADD_RIGHT:
		if (!_rchild)
			set_right(p);
		else
			_rchild->insert_tree(p);		
		break;
	case ADD_BROTHER:
		insert_brother(p);
		break;
	}

	return true;
}

bool cBNode::insert_node(cBNode* p)
{
	if (p == NULL)
		return true;

	switch(p->GetVersortType())
	{
	case VERSORT_POINT:
	case VERSORT_SEAM:
		{
			return insert_point(p);
		}
	case VERSORT_TREE:
		{
			return insert_tree(p);
		}
	}
	return false;
}

void cBNode::insert_brother(cBNode* p)
{
	if (!_brother)
	{
		_brother = p;
		add_child(p);
	}
	else
		_brother->insert_brother(p);
}

/////////////////////////////////////////////////////////////////////
//remove
/////////////////////////////////////////////////////////////////////
bool cBNode::remove()
{
/*
	remove_fromtree();
	_plane = NULL;
	delete this;
*/
	return true;
}


void cBNode::lchild_leaf()
{
	if (!_lchild)
		return;
	cBNode* p = _rchild;
	ASSERT(p);
	while(p->_lchild)
		p = p->_lchild;
	p->set_left(_lchild);
}

void cBNode::replace_child(cBNode* p,cBNode* pNew)
{
	if (_lchild == p)
	{
		_lchild = pNew;
	}
	else if (_rchild == p)
	{
		_rchild = pNew;
	}
	else if (_brother == p)
	{
		_brother = pNew;
	}
	else
	{
		ASSERT(0);
	}
	if (pNew)
		pNew->set_father(this);
}

bool cBNode::remove_fromtree()
{
	if (_brother)
	{
		_brother->_lchild = _lchild;
		if (_brother->_lchild != NULL)
			_brother->_lchild->set_father(_brother);
		_brother->_rchild = _rchild;
		if (_brother->_rchild != NULL)
			_brother->_rchild->set_father(_brother);
		ASSERT(_brother->_llist.size() == 0);
		ASSERT(_brother->_rlist.size() == 0);
		_brother->_llist = _llist;
		_brother->_rlist = _rlist;
		_llist.clear();
		_rlist.clear();
		get_father()->replace_child(this,_brother);
		_lchild = NULL;
		_rchild = NULL;
		_father = NULL;
		return true;
	}

#pragma message (ATTENTION("here is not right!"))	
	if (!_lchild)
	{
		if (!_llist.empty())
		{
			_lchild = _llist.back();
			_lchild->set_father(this);
			_llist.pop_back();
			_lchild->_llist = _llist;
			_llist.clear();
		}
	}
	if (!_rchild)
	{
		if (!_rlist.empty())
		{
			_rchild = _rlist.front();
			_rchild->set_father(this);
			_rlist.pop_front();
			_rchild->_rlist = _rlist;
			_rlist.clear();
		}
	}
	if (!_rchild)
	{
		if (get_father())
		{
			get_father()->replace_child(this,_lchild);
		}
	}
	else
	{
		if (get_father())
		{
			get_father()->replace_child(this,_rchild);
		}
		lchild_leaf();
	}
	_lchild = NULL;
	_rchild = NULL;
	_father = NULL;
	return true;
}

bool cBNode::remove(long value)
{
	if (get_value() == value)
	{
		remove();
		return true;
	}
	if (remove_list_seam(_llist,value) || remove_list_seam(_rlist,value))
		return true;
	if (get_brother() && get_brother()->remove(value))
		return true;
	if (get_left() && get_left()->remove(value))
		return true;
	if (get_right() && get_right()->remove(value))
		return true;
	return false;
}

/////////////////////////////////////////////////////////////////////
//draw
/////////////////////////////////////////////////////////////////////
void cBNode::build_self()
{
	cPlaneVerMesh* p = dynamic_cast<cPlaneVerMesh*>(get_plane());
	if (p)
		p->BuildMesh(_p1,_p2);
}

void cBNode::build()
{
	if (_lchild)
		_lchild->build();
	else
		build_list_seam(_llist);
	if (_brother)
		_brother->build();
//	draw_self();
	build_self();
	if (_rchild)
		_rchild->build();
	else
		build_list_seam(_rlist);
}

void cBNode::draw_self()
{
	if(get_plane())
	{
		if(get_plane()->IsShow())
			get_plane()->DrawSub(_p1,_p2);
	}
}

void cBNode::draw()
{
	if (_lchild)
		_lchild->draw();
	else
		draw_list_seam(_llist);
	if (_brother)
		_brother->draw();
	draw_self();
	if (_rchild)
		_rchild->draw();
	else
		draw_list_seam(_rlist);
}

void cBNode::drawnum()
{
	if (_lchild)
		_lchild->drawnum();
	if (_brother)
		_brother->drawnum();
	if (get_plane())
	{
		if(get_plane()->IsShow())
			get_plane()->DrawNum(_p1,_p2);
	}
	if (_rchild)
		_rchild->drawnum();
}

