/** TreeProjector is a functor returns an f-tree from an e-tree and the word
 * alignment between e and f .
 */
#ifndef __TreeProjector_H__ 
#define __TreeProjector_H__

#include "Derivation.h"
#include "Tree.h"


namespace mtrule {

//! TreeProjector is a functory returning an f-tree for the input e-tree and
//! the word alignment between e and f.
class TreeProjector : public Derivation {
	typedef Derivation  _base;
public:
	//! Constructor with word alignment.
	TreeProjector(Alignment* a, DB* dbp, bool projSize =false) : _base(a, dbp, NULL),
                                                m_projRuleSize(projSize)	{
		 // -l 0:0
		 set_limits(0, 0);
		 // -U 0
	     RuleInst::set_max_nb_unaligned(0); 
	}

	//! the projector.
	/**
	 * \param eTree   the English tree. It is represented using mtrule::Tree
	 *          so that we can utilize its functionalities.
	 */
	string project(mtrule::Tree& etree);

private:
	 /**
	  * \param s: the tree of rule sizes. corresponding to the nodes in the 
	  *           projected tree.
	  */
	string project(DerivationNode* node);
    Leaves* m_leaves;

	bool m_projRuleSize;

};

}  // namespace mtrule

#endif
