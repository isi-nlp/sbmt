#include "LabelSpan.h"

////////////////////////////////////////////////////////////////
// Label Span Signature.
////////////////////////////////////////////////////////////////

using namespace __gnu_cxx;

size_t LabSpanSig ::hash() const  {
	size_t ret = 0;
	__gnu_cxx::hash<const char*> hc;
	ret += hc(label.c_str());
	ret *= 10;
	ret += span.first*100+span.second;
	ret *= 10;
	ret += isInternal;
    ret *= 10;
    ret += height;
	return ret;
}

bool LabSpanSig ::
operator==(const LabSpanSig & other) const
{
	if(other.label != label) { return false;}
	else if(other.span != span) { return false;}
	else if(other.isInternal != isInternal) { return false;}
	else if(other.height!= height) { return false;}
	else return true;
}

LabSpanSig::
LabSpanSig(mtrule::TreeNode* parent, 
		   const vector<mtrule::TreeNode*>& sibNodes)
: isInternal(true)
{

     // the height of this node.
     height = parent->height();


	vector<mtrule::TreeNode*> sibs = sibNodes;

	vector<mtrule::TreeNode*>::const_iterator i;

	// get the label. all the siblings should have the same 
	// parent.
	mtrule::TreeNode* par = NULL;
	for(i = sibs.begin(); i != sibs.end(); ++i){
		if(! par) { par = (*i)->parent(); }
		assert(par == (*i)->parent());
	}
	
	// if the 'sibs' contains the same pointer as 
	// parent, we are concerned with the word rather
	// than the PoS.
	if(!sibs.size()){
		label = parent->get_word();
		isInternal = false;
	} else {
		label = parent->get_cat();
	}
	if(sibs.size() && parent->get_nb_subtrees() > sibs.size()){
		label += "-BAR"; 
	}
	
	// span
	mtrule::spans the_spans;
	//size_t j;
	for(i = sibs.begin(); i != sibs.end(); ++i){
		the_spans.push_back((*i)->get_span());
	}
	if(!the_spans.size()){
		the_spans.push_back(parent->get_span());
	}
    std::stable_sort(the_spans.begin(),the_spans.end(),mtrule::TreeNode::span_sort());
	span.first = the_spans.begin()->first;
	span.second = the_spans.back().second;
}

