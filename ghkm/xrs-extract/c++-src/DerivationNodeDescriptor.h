#ifndef __DERIVATIONNODEDESCRIPTOR_H__
#define __DERIVATIONNODEDESCRIPTOR_H__

/***************************************************************************
 * DerivationNodeDescriptor.C/h
 * 
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: DerivationNodeDescriptor.h,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include "TreeNode.h"
#include "hashutils.h"

namespace mtrule {

struct DerivationNodeDescriptor {

  struct hash_desc {
	 size_t operator()(const DerivationNodeDescriptor& d) const {
	   return ((unsigned long)d._tn + (unsigned long)d._lex + d._start + d._end) 
	          % (HASH_INT_HSIZE-1);
	 }
  };

  // Member vars:
  TreeNode* _tn;
  bool _lex;
  int _start, _end;
  // Member functions:
  DerivationNodeDescriptor(TreeNode *tn, bool lex, int start, int end) : 
    _tn(tn), _lex(lex), _start(start), _end(end) {}
  bool operator==(const DerivationNodeDescriptor& d) const {
    return (_tn == d._tn && _lex == d._lex && 
	         _start == d._start && _end == d._end);
  }
};

}

#endif
