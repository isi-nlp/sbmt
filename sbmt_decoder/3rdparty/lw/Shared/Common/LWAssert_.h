// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LW_COMMON_ASSERT_H
#define LW_COMMON_ASSERT_H 1

#ifndef assert
#define assert(e) (void(0))
#endif

namespace LW {

extern void _lw_abort(const char* file, int linenum);

#define LW_ASSERT(x) (void)((x) || (LW::_lw_abort(__FILE__, __LINE__),1))

}
	
#endif // LW_COMMON_ASSERT_H
