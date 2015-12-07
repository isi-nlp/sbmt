#ifndef __VARIABLE_H__
#define __VARIABLE_H__

/***************************************************************************
 * Variable.C/h
 * 
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: Variable.h,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include <iostream>

#include "TreeNode.h"

namespace mtrule {
 
  struct Variable {

	 /*************************************************************************
	  * Only static members:
	  *************************************************************************/
	 
	 // When the _var_index member variable is set to NOT_A_VAR
	 // it means that the constituent (lhs or rhs) is not a variable, 
	 // i.e. it is a lexical item (either aligned or unaligned chinese).
	 static const int NOT_A_VAR = -1;
	 // If set to UNALIGNEDC, it means that the RHS element is an unaligned
	 // chinese word.
	 static const int UNALIGNEDC  = -2;
	 // Determine if a given variable index corresponds to a ``normal'' 
	 // variable, i.e. neither NOT_A_VAR nor UNALIGNEDC (i.e. corresponds 
	 // to x0, x1,etc):
	 static bool is_variable_index(int index) { return ( index >= 0); }
  };

}

#endif
