// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LiBE_H
#define LiBE_H 1


#include <stdlib.h>


#if defined(WIN32)
#include "Common/stldefs.h"
#include <hash_set>
# define STL_HASH stdext
#include <sstream>
#elif GCC_VERSION_2_95_2
# define STL_HASH std
# include <hash_map>
# include <hash_set>
# include <functional>
# include <strstream>
# define USE_STRSTREAM 1

#elif GCC_VERSION_2_95_3
# define STL_HASH std
# include <hash_map>
# include <hash_set>
# include <functional>
# include <strstream>
# define USE_STRSTREAM 1


#elif GCC_VERSION_2_95_4
# define STL_HASH std
# include <hash_map>
# include <hash_set>
# include <functional>
# include <strstream>
# define USE_STRSTREAM 1

#elif GCC_VERSION_2_96
# define STL_HASH std
# include <hash_map>
# include <hash_set>
# include <functional>
# include <sstream>

#elif GCC_VERSION_3_1_1
# define STL_HASH __gnu_cxx
# include <ext/hash_map>
# include <ext/hash_set>
# include <ext/functional>
# include <sstream>

#elif GCC_VERSION_3_2
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>

#elif GCC_VERSION_3_2_1
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>

#elif GCC_VERSION_3_2_2
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_3_2_3
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_3_3
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_3_3_2
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_3_3_3
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_3_4_0
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif __GCC3__
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#elif GCC_VERSION_4_0_0
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
#else
#define STL_HASH __gnu_cxx
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
#include <sstream>
//#define STL_HASH __gnu_cxx
//#include <ext/hash_map>
//#include <ext/hash_set>
//#include <ext/functional>
//#include <sstream>
//# warning Unknown compiler version; take a look at LiBE.H!

#endif
#endif
