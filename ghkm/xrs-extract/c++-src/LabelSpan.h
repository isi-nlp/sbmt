#ifndef _LabelSpan_H_
#define _LabelSpan_H_
#include "TreeNode.h"
//#include "LiBE.h"
//
// added the height (from the root) as a part of the signature.

#include <string>
#include <iostream>

using namespace __gnu_cxx;

////////////////SIGNATURE////////////////////
//! Labeled span signature with node height in the tree.
class LabSpanSig {
public:
	LabSpanSig() : label(""), isInternal(true), span(-1, -1), height(0) {}
	LabSpanSig(mtrule::TreeNode* parent, 
			   const vector<mtrule::TreeNode*>& sibs);
	virtual ~LabSpanSig() {}

	std::string label;

	//! the label is an internal node label.
	bool isInternal;

	pair<int, int> span;

    //! The height of this forest node. Mainly used to prevent loops, i.e., a
    //! forest and node has a child that is the parent OR node of this and node.
    int height;

	virtual size_t hash() const;
	bool operator==(const LabSpanSig& other) const;
	LabSpanSig& operator=(const LabSpanSig& other) { 
		label = other.label; 
		span = other.span; 
		isInternal = other.isInternal; 
		height = other.height; 
		return *this;
	}


	friend std::ostream& operator<<(std::ostream& o, const LabSpanSig& lss) {
		o<<lss.label<<" "<<lss.span.first<<" "<<lss.span.second<<" "<<lss.isInternal<<" "<<lss.height<<std::endl;
		return o;
	}
};

namespace __gnu_cxx{
	template<>
	struct hash<LabSpanSig> {
		size_t operator()(const LabSpanSig& d) const { return d.hash(); }
	};
}

#endif

