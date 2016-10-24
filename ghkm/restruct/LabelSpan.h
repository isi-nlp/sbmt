#ifndef _LabelSpan_H_
#define _LabelSpan_H_
#include "TreeNode.h"
#include "LiBE.h"

using namespace STL_HASH;
using namespace std;

////////////////SIGNATURE////////////////////
//! Labeled span signature.
class LabSpanSig {
public:
	LabSpanSig() : label(""), isInternal(true), span(-1, -1) {}
	LabSpanSig(mtrule::TreeNode* parent, 
			   const vector<mtrule::TreeNode*>& sibs);
	virtual ~LabSpanSig() {}

	string label;

	//! the label is an internal node label.
	bool isInternal;

	pair<int, int> span;
	virtual size_t hash() const;
	bool operator==(const LabSpanSig& other) const;
	LabSpanSig& operator=(const LabSpanSig& other) { 
		label = other.label; 
		span = other.span; 
		isInternal = other.isInternal; 
		return *this;
	}


	friend ostream& operator<<(ostream& o, const LabSpanSig& lss) {
		o<<lss.label<<" "<<lss.span.first<<" "<<lss.span.second<<" "<<lss.isInternal<<endl;
		return o;
	}
};

namespace STL_HASH {
	template<>
	struct hash<LabSpanSig> {
		size_t operator()(const LabSpanSig& d) const { return d.hash(); }
	};
}

#endif

