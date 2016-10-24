#include "ForestNodes.h"

using namespace std;

//////////////////////////ANDNode//////////////////////////////////////////

template<class T>
void ANDNode<T> :: copyUp ()
{
	if(_sub_trees.size()) { return;}
	// label
	_base::_cat = data.getSig().label;
	// if this is a leaf, the _cat and _word will be the same.
	if(_base2::size() == 0){ _base::_word = data.getSig().label;
	} else { _base::_word = ""; }

	// span.
	_e_span = data.getSig().span;
	
	// head word indexes --- set them to be dummy. 
	_head_index = 0;
	_head_string_pos = 0;

//	_parent = NULL;
	// parent and children.
	// parent is always the OR node of the real parent.
	//_parent = m_parent;
	typename std::list<ORNode<T>*>::const_iterator it;
	for(it = _base2::begin(); it != _base2::end(); ++it){
		_sub_trees.push_back(*it);
		(*it)->parent() = this;
		(*it)->copyUp();
	}
}

//! returns the span covered by this node.
template<class T>
pair<int, int> ANDNode<T>::getSpan() const
{ return data.getSig().span(); }

//! computes the span of the forest nodes.
template<class T>
void ANDNode<T>::computeSpan(int& wdIndex, set<ANDNode<T>*> & leavesComputed)
{
	if(leavesComputed.find(this) != leavesComputed.end()){ return; }
	leavesComputed.insert(this);

	// if this is a leaf node.
	if(_base2::begin() == _base2::end()){
		data.getSig().span.first  = wdIndex;
		data.getSig().span.second = wdIndex;
		++wdIndex;
	} 
	// if this is an internal node.
	else  {
		//! computes the spans of children.
		typename _base2::const_iterator it ;
		for(it = _base2::begin(); it != _base2::end(); ++it){
			(*it)->computeSpan(wdIndex, leavesComputed);
		}
		pair<int, int>& span = data.getSig().span;
		span.first = (*_base2::begin())->getSpan().first; 
		span.second = (_base2::back())->getSpan().second;
	}
}



//////////////////////////ORNode//////////////////////////////////////////

template<class T>
void ORNode<T> :: copyUp ()
{
	if(_sub_trees.size()) { return;}

	// an OR node must not be empty.
	assert(_base::begin() != _base::end());

	T data = _base::begin()->data;

	// label
	_baseTreeNode::_cat = data.getSig().label;
	// if this is a leaf, the _cat and _word will be the same.
	if(_base::size() == 0){ _baseTreeNode::_word = data.getSig().label;
	} else { _baseTreeNode::_word = ""; }

	// span.
	_e_span = data.getSig().span;
	//cerr<<_e_span.first<<":"<<_e_span.second<<" ++\n";
	
	// head word indexes --- set them to be dummy. 
	_head_index = 0;
	_head_string_pos = 0;

//	_parent = NULL;
	// parent and children.
	// parent is always the OR node of the real parent.
	//_parent = grandParent(); ////
	typename list<ANDNode<T> >::iterator it;
	for(it = _base::begin(); it != _base::end(); ++it){
		_sub_trees.push_back((TreeNode*)&(*it));
		it->parent() = this->parent();
		it->setImdPar(this);
		it->copyUp();
	}
}

//! Assignment.
template<class T>
ANDNode<T>& ANDNode<T>::operator=(const ANDNode<T>& other)
{
	_base::operator=(other);
	_base2::operator=(other);
	_base::setType(other.type());
	data = other.data;
	return *this;
}

//! Assignment.
template<class T>
ORNode<T>& ORNode<T>::operator=(const ORNode& other) 
{
	_base::operator=(other);
	_base::setType(other.type()); 
	id=other.id;
	return *this;
}

//! computes the span of the forest nodes.
template<class T>
void ORNode<T>::computeSpan(int& wdIndex, set<ANDNode<T>*>& leavesComputed)
{
	assert(_base::begin() != _base::end());
	// compute the span of children.
	typename _base::iterator it;
	for(it = _base::begin(); it != _base::end(); ++it){
		(*it).computeSpan(wdIndex, leavesComputed);
	}
}

template<class T>
pair<int, int> ORNode<T>::getSpan() const
{
	assert(_base::begin() != _base::end());

	pair<int, int> ret;
	ret = _base::begin()->data.getSig().span;
	return ret;
}

