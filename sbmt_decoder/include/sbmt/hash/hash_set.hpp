/**
 wrappers for STL hash_set
 */
#ifndef SBMT_HASH_SET_H
#define SBMT_HASH_SET_H 1

#include <utility> // this is just to get stlport macros in

#if defined(_STLPORT_VERSION) || defined(__SGI_STL_PORT)
#   include <hash_set>
#   ifndef stlext_ns_alias_defined
#       define stlext_ns_alias_defined
        namespace stlext = ::std;
        namespace stlextp = ::std;
#   endif


#else

#include <ext/hash_set>
#   ifndef stlext_ns_alias_defined
#       define stlext_ns_alias_defined
        namespace stlext = ::__gnu_cxx;
        namespace stlextp = ::std;
#   endif

#endif

namespace sbmt {

template<class Set, class Iter>
inline void
insert_range(Set& s, Iter begin, Iter end)
{
	for (Iter i = begin; i != end; ++i)
		s.insert(*i);
}

}

#endif // SBMT_HASH_SET_H
