#ifndef _TTCONFIG_HPP
#define _TTCONFIG_HPP

#include <graehl/shared/config.h>
#include <graehl/shared/myassert.h>
#include <boost/config.hpp>
#include <graehl/shared/tree.hpp>

//using namespace std;
//using namespace boost;

//#include "debugprint.hpp"

#define TT_VERSION 0.0.1

//FIXME: portability issue:
// for debugging purposes - printing union of pointer/small int - output will be WRONG if any heap memory is allocated below this:
//#define MIN_LEGAL_ADDRESS ((void *)0x1000)

#define TT_TRAITS

namespace graehl {

static const rank_type ANYRANK=(rank_type)-1;

}

#ifdef GRAEHL_TEST
//#define DEBUG_TREEIO
#endif

#ifdef DEBUG
#endif



#endif
